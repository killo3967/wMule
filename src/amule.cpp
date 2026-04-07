//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#include "amule.h"			// Interface declarations.
#include <common/threading/ThreadGuards.h>
#include "libs/common/ThreadPool.h"

#include <csignal>
#include <cstring>
#include <chrono>
#include <wx/process.h>
#include <wx/sstream.h>
#include "config.h"			// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT,
					//   HAVE_SYS_RESOURCE_H, HAVE_SYS_STATVFS_H, VERSION
					//   and ENABLE_NLS
#include <common/ClientVersion.h>

#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>
#include <wx/msgdlg.h>
#include <wx/socket.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/stopwatch.h>		// Needed for wxStopWatch
#include <wx/textdlg.h>
#include <wx/utils.h>


#include <common/Format.h>		// Needed for CFormat
#include <common/SecretHash.h>
#include <common/Path.h>
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "CanceledFileList.h"
#include "ClientCreditsList.h"		// Needed for CClientCreditsList
#include "ClientList.h"			// Needed for CClientList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket & CMuleUDPSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include <common/FileFunctions.h>	// Needed for CDirIterator
#include "FriendList.h"			// Needed for CFriendList
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "InternalEvents.h"		// Needed for CMuleInternalEvent
#include "IPFilter.h"			// Needed for CIPFilter
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "ListenSocket.h"		// Needed for CListenSocket
#include "Logger.h"			// Needed for CLogger // Do_not_auto_remove
#include "MagnetURI.h"			// Needed for CMagnetURI
#include "OtherFunctions.h"
#include "PartFile.h"			// Needed for CPartFile
#include "PlatformSpecific.h"   // Needed for PlatformSpecific::AllowSleepMode();
#include "Preferences.h"		// Needed for CPreferences
#include "SearchList.h"			// Needed for CSearchList
#include "Server.h"			// Needed for GetListName
#include "ServerList.h"			// Needed for CServerList
#include "ServerConnect.h"              // Needed for CServerConnect
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "Statistics.h"			// Needed for CStatistics
#include "TerminationProcessAmuleweb.h"	// Needed for CTerminationProcessAmuleweb
#include "ThreadTasks.h"
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "UploadBandwidthThrottler.h"

namespace {

wxString NormalizeManualECReason(wxString reason)
{
	reason.Replace(wxT("\r"), wxEmptyString);
	reason.Replace(wxT("\n"), wxT(" "));
	reason.Trim(true).Trim(false);
	if (reason.IsEmpty()) {
		reason = _("(no reason provided)");
	}
	return reason;
}

bool PromptManualECActivation(wxWindow* parent, const wxString& source, wxString* outReason)
{
	const wxString message = CFormat(_(
		"You are enabling External Connections via %s.\n\n"
		"This exposes the remote control interface to your network."
		" Do you want to continue?")) % source;
	int response = theApp->ShowAlert(message, _("External Connections"), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
	if (response != wxYES) {
		return false;
	}

	wxTextEntryDialog reasonDialog(
		parent,
		_("Describe why you are enabling External Connections (this note will be logged):"),
		_("External Connections"),
		wxEmptyString,
		wxTextEntryDialogStyle
	);
	if (reasonDialog.ShowModal() != wxID_OK) {
		return false;
	}

	*outReason = NormalizeManualECReason(reasonDialog.GetValue());
	return true;
}

}
#include "UserEvents.h"
#include "ScopedPtr.h"

#ifdef ENABLE_UPNP
#include "UPnPBase.h"			// Needed for UPnP
#endif

#ifdef __WXMAC__
#include <wx/sysopt.h>			// Do_not_auto_remove
#endif

#ifndef AMULE_DAEMON
	#ifdef __WXMAC__
		#include <CoreFoundation/CFBundle.h>  // Do_not_auto_remove
		#include <wx/osx/core/cfstring.h>  // Do_not_auto_remove
	#endif
	#include <wx/msgdlg.h>

	#include "amuleDlg.h"
#endif


#ifdef HAVE_SYS_RESOURCE_H
	#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
	#include <sys/statvfs.h>  // Do_not_auto_remove
#endif


#ifdef __GLIBC__
# define RLIMIT_RESOURCE __rlimit_resource
#else
# define RLIMIT_RESOURCE int
#endif

#ifdef AMULE_DAEMON
CamuleDaemonApp *theApp;
#else
CamuleGuiApp *theApp;
#endif

static void UnlimitResource(RLIMIT_RESOURCE resType)
{
#if defined(HAVE_GETRLIMIT) && defined(HAVE_SETRLIMIT)
	struct rlimit rl;
	getrlimit(resType, &rl);
	rl.rlim_cur = rl.rlim_max;
	setrlimit(resType, &rl);
#endif
}


static void SetResourceLimits()
{
#ifdef HAVE_SYS_RESOURCE_H
	UnlimitResource(RLIMIT_DATA);
#ifndef __UCLIBC__
	UnlimitResource(RLIMIT_FSIZE);
#endif
	UnlimitResource(RLIMIT_NOFILE);
#ifdef RLIMIT_RSS
	UnlimitResource(RLIMIT_RSS);
#endif
#endif
}

// We store the received signal in order to avoid race-conditions
// in the signal handler.
bool g_shutdownSignal = false;

void OnShutdownSignal( int /* sig */ )
{
	signal(SIGINT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

	g_shutdownSignal = true;

#ifdef AMULE_DAEMON
	theApp->ExitMainLoop();
#endif
}


CamuleApp::CamuleApp()
{
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	StartTickTimer();

	// Initialization
	m_app_state = APP_STATE_STARTING;

	theApp = &wxGetApp();

	clientlist	= nullptr;
	searchlist	= nullptr;
	knownfiles	= nullptr;
	canceledfiles	= nullptr;
	serverlist	= nullptr;
	serverconnect	= nullptr;
	sharedfiles	= nullptr;
	listensocket	= nullptr;
	clientudp	= nullptr;
	clientcredits	= nullptr;
	friendlist	= nullptr;
	downloadqueue	= nullptr;
	uploadqueue	= nullptr;
	ipfilter	= nullptr;
	ECServerHandler = nullptr;
	glob_prefs	= nullptr;
	m_statistics	= nullptr;
	uploadBandwidthThrottler = nullptr;
#ifdef ENABLE_UPNP
	m_upnp		= nullptr;
	m_upnpMappings.resize(4);
#endif
	core_timer	= nullptr;

	m_localip	= 0;
	m_dwPublicIP	= 0;
	webserver_pid	= 0;

	enable_daemon_fork = false;

	// Apparently needed for *BSD
	SetResourceLimits();

#ifdef _MSC_VER
	_CrtSetDbgFlag(0);		// Disable useless memleak debugging
#endif
}

CamuleApp::~CamuleApp()
{
	// Closing the log-file as the very last thing, since
	// wxWidgets log-events are saved in it as well.
	theLogger.CloseLogfile();
}

int CamuleApp::OnExit()
{
	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("Now, exiting main app..."));
	}

	// From wxWidgets docs, wxConfigBase:
	// ...
	// Note that you must delete this object (usually in wxApp::OnExit)
	// in order to avoid memory leaks, wxWidgets won't do it automatically.
	//
	// As it happens, you may even further simplify the procedure described
	// above: you may forget about calling Set(). When Get() is called and
	// there is no current object, it will create one using Create() function.
	// To disable this behaviour DontCreateOnDemand() is provided.
	delete wxConfigBase::Set((wxConfigBase *)nullptr);

	// Save credits
	clientcredits->SaveList();

	// Kill amuleweb if running
	if (webserver_pid) {
		AddLogLineNS(CFormat(_("Terminating amuleweb instance with pid '%ld' ... ")) % webserver_pid);
		wxKillError rc;
		if (wxKill(webserver_pid, wxSIGTERM, &rc) == -1) {
			AddLogLineNS(CFormat(_("Killing amuleweb instance with pid '%ld' ... ")) % webserver_pid);
			if (wxKill(webserver_pid, wxSIGKILL, &rc) == -1) {
				AddLogLineNS(_("Failed"));
			}
		}
	}

	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("wMule OnExit: Terminating core."));
	}

	delete serverlist;
	serverlist = nullptr;

	delete searchlist;
	searchlist = nullptr;

	delete clientcredits;
	clientcredits = nullptr;

	delete friendlist;
	friendlist = nullptr;

	// Destroying CDownloadQueue calls destructor for CPartFile
	// calling CSharedFileList::SafeAddKFile occasionally.
	delete sharedfiles;
	sharedfiles = nullptr;

	delete serverconnect;
	serverconnect = nullptr;

	delete listensocket;
	listensocket = nullptr;

	delete clientudp;
	clientudp = nullptr;

	delete knownfiles;
	knownfiles = nullptr;

	delete canceledfiles;
	canceledfiles = nullptr;

	delete clientlist;
	clientlist = nullptr;

	delete uploadqueue;
	uploadqueue = nullptr;

	delete downloadqueue;
	downloadqueue = nullptr;

	delete ipfilter;
	ipfilter = nullptr;

#ifdef ENABLE_UPNP
	delete m_upnp;
	m_upnp = nullptr;
#endif

	delete ECServerHandler;
	ECServerHandler = nullptr;

	delete m_statistics;
	m_statistics = nullptr;

	delete glob_prefs;
	glob_prefs = nullptr;
	CPreferences::EraseItemList();

	delete uploadBandwidthThrottler;
	uploadBandwidthThrottler = nullptr;

#ifdef ASIO_SOCKETS
	delete m_AsioService;
	m_AsioService = nullptr;
#endif

	wxSocketBase::Shutdown();	// needed because we also called Initialize() manually

	if (m_app_state!=APP_STATE_STARTING) {
		AddLogLineNS(_("wMule shutdown completed."));
	}

#if wxUSE_MEMORY_TRACING
	AddLogLineNS(_("Memory debug results for wMule exit:"));
	// Log mem debug messages to wxLogStderr
	wxLog* oldLog = wxLog::SetActiveTarget(new wxLogStderr);
	//AddLogLineNS(wxT("**************Classes**************");
	//wxDebugContext::PrintClasses();
	//AddLogLineNS(wxT("***************Dump***************");
	//wxDebugContext::Dump();
	AddLogLineNS(wxT("***************Stats**************"));
	wxDebugContext::PrintStatistics(true);

	// Set back to wxLogGui
	delete wxLog::SetActiveTarget(oldLog);
#endif

	StopTickTimer();

	// Return 0 for successful program termination
	return AMULE_APP_BASE::OnExit();
}


int CamuleApp::InitGui(bool, wxString &)
{
	return 0;
}


//
// Application initialization
//
bool CamuleApp::OnInit()
{
#if wxUSE_MEMORY_TRACING
	// any text before call of Localize_mule needs not to be translated.
	AddLogLineNS(wxT("Checkpoint set on app init for memory debug"));	// debug output
	wxDebugContext::SetCheckpoint();
#endif

	// Forward wxLog events to CLogger
	wxLog::SetActiveTarget(new CLoggerTarget);

	m_localip = StringHosttoUint32(::wxGetFullHostName());

#ifndef __WINDOWS__
	// get rid of sigpipe
	signal(SIGPIPE, SIG_IGN);
#else
	// Handle CTRL-Break
	signal(SIGBREAK, OnShutdownSignal);
#endif
	// Handle sigint and sigterm
	signal(SIGINT, OnShutdownSignal);
	signal(SIGTERM, OnShutdownSignal);

#ifdef __WXMAC__
	// For listctrl's to behave on Mac
	wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), 1);
#endif

	// Handle uncaught exceptions
	InstallMuleExceptionHandler();

	if (!InitCommon(AMULE_APP_BASE::argc, AMULE_APP_BASE::argv)) {
		return false;
	}

	glob_prefs = new CPreferences();

	const bool unsafePref = thePrefs::GetAllowUnsafeInternalDirs();
	const bool unsafeEffective = thePrefs::AllowUnsafeInternalDirsEffective();

	CInternalPathContext internalContext = {
		thePrefs::GetConfigDir(),
		thePrefs::GetTempDir().GetRaw(),
		thePrefs::GetIncomingDir().GetRaw(),
		true,
		unsafePref,
		unsafeEffective
	};

	CInternalPathResult tempValidation = NormalizeInternalDir(EInternalPathKind::TempDir, thePrefs::GetTempDir().GetRaw(), internalContext);
	if (tempValidation.m_ok) {
		thePrefs::SetTempDir(CPath(tempValidation.m_normalizedPath));
		internalContext.m_tempDir = tempValidation.m_normalizedPath;
		if (tempValidation.m_isExternalToBase) {
			AddLogLineCS(wxT("WARNING: Temp directory is external to configuration base (validated)."));
		}
	} else {
		wxString fallbackTemp = GetDefaultInternalPath(EInternalPathKind::TempDir, internalContext);
		CInternalPathResult fallbackTempValidation = NormalizeInternalDir(EInternalPathKind::TempDir, fallbackTemp, internalContext);
		if (fallbackTempValidation.m_ok) {
			AddLogLineCS(CFormat(wxT("Rejected persisted temp directory '%s' (%s); using '%s'."))
				% thePrefs::GetTempDir().GetRaw()
				% InternalPathErrorToString(tempValidation.m_error)
				% fallbackTempValidation.m_normalizedPath);
			thePrefs::SetTempDir(CPath(fallbackTempValidation.m_normalizedPath));
			internalContext.m_tempDir = fallbackTempValidation.m_normalizedPath;
		}
	}

	CInternalPathResult incomingValidation = NormalizeInternalDir(EInternalPathKind::IncomingDir, thePrefs::GetIncomingDir().GetRaw(), internalContext);
	if (incomingValidation.m_ok) {
		thePrefs::SetIncomingDir(CPath(incomingValidation.m_normalizedPath));
		internalContext.m_incomingDir = incomingValidation.m_normalizedPath;
		if (incomingValidation.m_isExternalToBase) {
			AddLogLineCS(wxT("WARNING: Incoming directory is external to configuration base (validated)."));
		}
	} else {
		wxString fallbackIncoming = GetDefaultInternalPath(EInternalPathKind::IncomingDir, internalContext);
		CInternalPathResult fallbackIncomingValidation = NormalizeInternalDir(EInternalPathKind::IncomingDir, fallbackIncoming, internalContext);
		if (fallbackIncomingValidation.m_ok) {
			AddLogLineCS(CFormat(wxT("Rejected persisted incoming directory '%s' (%s); using '%s'."))
				% thePrefs::GetIncomingDir().GetRaw()
				% InternalPathErrorToString(incomingValidation.m_error)
				% fallbackIncomingValidation.m_normalizedPath);
			thePrefs::SetIncomingDir(CPath(fallbackIncomingValidation.m_normalizedPath));
			internalContext.m_incomingDir = fallbackIncomingValidation.m_normalizedPath;
		}
	}

	CPath outDir;
	if (CheckMuleDirectory(wxT("temp"), thePrefs::GetTempDir(), thePrefs::GetConfigDir() + wxT("Temp"), outDir)) {
		thePrefs::SetTempDir(outDir);
	} else {
		return false;
	}

	if (CheckMuleDirectory(wxT("incoming"), thePrefs::GetIncomingDir(), thePrefs::GetConfigDir() + wxT("Incoming"), outDir)) {
		thePrefs::SetIncomingDir(outDir);
	} else {
		return false;
	}

	internalContext.m_tempDir = thePrefs::GetTempDir().GetRaw();
	internalContext.m_incomingDir = thePrefs::GetIncomingDir().GetRaw();

	const wxString persistedOSDir = thePrefs::GetOSDir().GetRaw();
	COSDirValidationOutcome osDirOutcome = NormalizeOSDirWithFallback(persistedOSDir, internalContext);
	if (osDirOutcome.m_result.m_ok) {
		if (osDirOutcome.m_usedFallback) {
			AddLogLineCS(CFormat(wxT("Rejected persisted online signature directory '%s' (%s); using '%s'."))
				% persistedOSDir
				% InternalPathErrorToString(osDirOutcome.m_rejectedError)
				% osDirOutcome.m_result.m_normalizedPath);
		}

		thePrefs::SetOSDir(CPath(osDirOutcome.m_result.m_normalizedPath));
	}

	// Initialize wx sockets (needed for http download in background with Asio sockets)
	wxSocketBase::Initialize();

	// Some sanity check
	if (!thePrefs::UseTrayIcon()) {
		thePrefs::SetMinToTray(false);
	}

	// Build the filenames for the two OS files
	SetOSFiles(thePrefs::GetOSDir().GetRaw());

	// Check if we have the old style locale config
	bool old_localedef = false;
	wxString langId = thePrefs::GetLanguageID();
	if (!langId.IsEmpty() && (langId.GetChar(0) >= '0' && langId.GetChar(0) <= '9')) {
		old_localedef = true;
		thePrefs::SetLanguageID(wxLang2Str(wxLANGUAGE_DEFAULT));
		glob_prefs->Save();
	}

#ifdef ENABLE_NLS
	// Load localization settings
	Localize_mule();

	if (old_localedef) {
		ShowAlert(_("Your locale has been changed to System Default due to a configuration change. Sorry."), _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
	}
#endif

	// Configure EC for amuled when invoked with ec-config
	if (ec_config) {
		wxString ecReason;
		if (!PromptManualECActivation(nullptr, _("command line (--ec-config)"), &ecReason)) {
			AddLogLineCS(_("External connections remain disabled: --ec-config confirmation was cancelled."));
		} else {
			AddLogLineNS(_("\nEC configuration"));
			CMD4Hash pw = GetPassword(false);
			wxString secret = SecretHash::BuildPBKDF2FromMD5(pw.Encode());
			thePrefs::SetECPass(secret);
			thePrefs::EnableExternalConnections(true);
			AddLogLineNS(_("Password set and external connections enabled."));
			AddLogLineCS(CFormat(_("External connections manually enabled from %s by '%s'. Reason: %s"))
				% _("command line (--ec-config)")
				% wxGetUserId()
				% ecReason);
		}
	}

#ifndef __WINDOWS__
	if (getuid() == 0) {
		wxString msg =
			wxT("Warning! You are running wMule as root.\n")
			wxT("Doing so is not recommended for security reasons,\n")
			wxT("and you are advised to run wMule as an normal\n")
			wxT("user instead.");

		ShowAlert(msg, _("WARNING"), wxCENTRE | wxOK | wxICON_ERROR);

		fprintf(stderr, "\n--------------------------------------------------\n");
		fprintf(stderr, "%s", (const char*)unicode2UTF8(msg));
		fprintf(stderr, "\n--------------------------------------------------\n\n");
	}
#endif

	// Display notification on new version or first run
	wxTextFile vfile( thePrefs::GetConfigDir() + wxT("lastversion") );
	wxString newMule(wxT( VERSION ));

	if ( !wxFileExists( vfile.GetName() ) ) {
		vfile.Create();
	}

	if ( vfile.Open() ) {
		// Check if this version has been run before
		bool found = false;
		for ( size_t i = 0; i < vfile.GetLineCount(); i++ ) {
			// Check if this version has been run before
			if ( vfile.GetLine(i) == newMule ) {
				found = true;
				break;
			}
		}

		// We haven't run this version before?
		if ( !found ) {
			// Insert new at top to provide faster searches
			vfile.InsertLine( newMule, 0 );

			Trigger_New_version( newMule );
		}

		// Keep at most 10 entries
		while ( vfile.GetLineCount() > 10 )
			vfile.RemoveLine( vfile.GetLineCount() - 1 );

		vfile.Write();
		vfile.Close();
	}

	m_statistics = new CStatistics();

	clientlist	= new CClientList();
	friendlist	= new CFriendList();
	searchlist	= new CSearchList();
	knownfiles	= new CKnownFileList();
	canceledfiles	= new CCanceledFileList;
	serverlist	= new CServerList();

	sharedfiles	= new CSharedFileList(knownfiles);
	clientcredits	= new CClientCreditsList();

	// bugfix - do this before creating the uploadqueue
	downloadqueue	= new CDownloadQueue();
	uploadqueue	= new CUploadQueue();
	ipfilter	= new CIPFilter();

	// Creates all needed listening sockets
	wxString msg;
	if (!ReinitializeNetwork(&msg)) {
		AddLogLineNS(wxT("\n"));
		AddLogLineNS(msg);
	}

	// Test if there's any new version
	if (thePrefs::GetCheckNewVersion()) {
		// We use the thread base because I don't want a dialog to pop up.
		CHTTPDownloadThread* version_check =
			new CHTTPDownloadThread(wxT("http://amule.sourceforge.net/lastversion"),
				thePrefs::GetConfigDir() + wxT("last_version_check"), thePrefs::GetConfigDir() + wxT("last_version"), HTTP_VersionCheck, false, false);
		version_check->Create();
		version_check->Run();
	}

	// Create main dialog, or fork to background (daemon).
	InitGui(m_geometryEnabled, m_geometryString);

#ifdef AMULE_DAEMON
	// Need to refresh wxSingleInstanceChecker after the daemon fork() !
	if (enable_daemon_fork) {
		RefreshSingleInstanceChecker();
		// No need to check IsAnotherRunning() - we've done it before.
	}
#endif

	// Has to be created after the call to InitGui, as fork
	// (when using posix threads) only replicates the mainthread,
	// and the UBT constructor creates a thread.
	uploadBandwidthThrottler = new UploadBandwidthThrottler();

#ifdef ASIO_SOCKETS
	m_AsioService = new CAsioService;
#endif

	// Start performing background tasks
	// This will start loading the IP filter. It will start right away.
	// Log is confusing, because log entries from background will only be printed
	// once foreground becomes idle, and that will only be after loading
	// of the partfiles has finished.
	CThreadScheduler::Start();

	// These must be initialized after the gui is loaded.
	if (thePrefs::GetNetworkED2K()) {
		serverlist->Init();
	}
	downloadqueue->LoadMetFiles(thePrefs::GetTempDir());
	sharedfiles->Reload();

	// Ensure that the up/down ratio is used
	CPreferences::CheckUlDlRatio();

	// Load saved friendlist (now, so it can update in GUI right away)
	friendlist->LoadList();

	// The user can start pressing buttons like mad if he feels like it.
	m_app_state = APP_STATE_RUNNING;

	if (!serverlist->GetServerCount() && thePrefs::GetNetworkED2K()) {
		// There are no servers and ED2K active -> ask for download.
		// As we cannot ask in amuled, we just update there
#ifndef AMULE_DAEMON
		if (wxYES == wxMessageBox(
			wxString(
				_("You don't have any server in the server list.\nDo you want wMule to download a new list now?")),
			wxString(_("Server list download")),
			wxYES_NO,
			static_cast<wxWindow*>(theApp->amuledlg)))
#endif
		{
			serverlist->UpdateServerMetFromURL(thePrefs::GetEd2kServersUrl());
		}
	}


	// Autoconnect if that option is enabled
	if (thePrefs::DoAutoConnect()) {
		// IP filter is still loading and will be finished on event.
		// Tell it to autoconnect.
		if (thePrefs::GetNetworkED2K()) {
			ipfilter->ConnectToAnyServerWhenReady();
		}
		if (thePrefs::GetNetworkKademlia()) {
			ipfilter->StartKADWhenReady();
		}
	}

	// Enable GeoIP
#ifdef ENABLE_IP2COUNTRY
	theApp->amuledlg->EnableIP2Country();
#endif

	// Run webserver?
	if (thePrefs::GetWSIsEnabled()) {
		wxString aMuleConfigFile = thePrefs::GetConfigDir() + m_configFile;
		wxString amulewebPath = thePrefs::GetWSPath();

#if defined(__WXMAC__) && !defined(AMULE_DAEMON)
		// For the Mac GUI application, look for amuleweb in the bundle
		CFURLRef amulewebUrl = CFBundleCopyAuxiliaryExecutableURL(
			CFBundleGetMainBundle(), CFSTR("amuleweb"));

		if (amulewebUrl) {
			CFURLRef absoluteUrl = CFURLCopyAbsoluteURL(amulewebUrl);
			CFRelease(amulewebUrl);

			if (absoluteUrl) {
				CFStringRef amulewebCfstr = CFURLCopyFileSystemPath(absoluteUrl, kCFURLPOSIXPathStyle);
				CFRelease(absoluteUrl);
				amulewebPath = wxCFStringRef(amulewebCfstr).AsString(wxLocale::GetSystemEncoding());
			}
		}
#endif

#ifdef __WINDOWS__
#	define QUOTE	wxT("\"")
#else
#	define QUOTE	wxT("\'")
#endif

		wxString cmd =
			QUOTE +
			amulewebPath +
			QUOTE wxT(" ") QUOTE wxT("--amule-config-file=") +
			aMuleConfigFile +
			QUOTE;
		CTerminationProcessAmuleweb *p = new CTerminationProcessAmuleweb(cmd, &webserver_pid);
		webserver_pid = wxExecute(cmd, wxEXEC_ASYNC, p);
		bool webserver_ok = webserver_pid > 0;
		if (webserver_ok) {
			AddLogLineC(CFormat(_("web server running on pid %d")) % webserver_pid);
		} else {
			delete p;
			ShowAlert(_(
				"You requested to run web server on startup, but the amuleweb binary cannot be run. Please install the package containing wMule web server, or compile wMule using --enable-webserver and run make install"),
				_("ERROR"), wxOK | wxICON_ERROR);
		}
	}

	return true;
}

bool CamuleApp::ReinitializeNetwork(wxString* msg, bool forceUPnPRetry)
{
	bool ok = true;
	static bool firstTime = true;

	if (!firstTime) {
		// TODO: Destroy previously created sockets
	}
	firstTime = false;

	// Some sanity checks first
	if (thePrefs::ECPort() == thePrefs::GetPort()) {
		// Select a random usable port in the range 1025 ... 2^16 - 1
		uint16 port = thePrefs::ECPort();
		while ( port < 1024 || port  == thePrefs::GetPort() ) {
			port = (uint16)rand();
		}
		thePrefs::SetECPort( port );

		wxString err =
			wxT("Network configuration failed! You cannot use the same port\n")
			wxT("for the main TCP port and the External Connections port.\n")
			wxT("The EC port has been changed to avoid conflict, see the\n")
			wxT("preferences for the new value.\n");
		*msg << err;

		AddLogLineN(wxEmptyString );
		AddLogLineC(err );
		AddLogLineN(wxEmptyString );

		ok = false;
	}

	if (thePrefs::GetUDPPort() == thePrefs::GetPort() + 3) {
		// Select a random usable value in the range 1025 ... 2^16 - 1
		uint16 port = thePrefs::GetUDPPort();
		while ( port < 1024 || port == thePrefs::GetPort() + 3 ) {
			port = (uint16)rand();
		}
		thePrefs::SetUDPPort( port );

		wxString err =
			wxT("Network configuration failed! You set your UDP port to\n")
			wxT("the value of the main TCP port plus 3.\n")
			wxT("This port has been reserved for the Server-UDP port. The\n")
			wxT("port value has been changed to avoid conflict, see the\n")
			wxT("preferences for the new value\n");
		*msg << err;

		AddLogLineN(wxEmptyString );
		AddLogLineC(err );
		AddLogLineN(wxEmptyString );

		ok = false;
	}

	// Create the address where we are going to listen
	// TODO: read this from configuration file
	amuleIPV4Address myaddr[4];

	// Create the External Connections Socket.
	// Default is 4712.
	// Get ready to handle connections from apps like amulecmd
	if (thePrefs::GetECAddress().IsEmpty() || !myaddr[0].Hostname(thePrefs::GetECAddress())) {
		myaddr[0].AnyAddress();
	}
	myaddr[0].Service(thePrefs::ECPort());
	ECServerHandler = new ExternalConn(myaddr[0], msg);

	// Create the UDP socket TCP+3.
	// Used for source asking on servers.
	if (thePrefs::GetAddress().IsEmpty()) {
		myaddr[1].AnyAddress();
	} else if (!myaddr[1].Hostname(thePrefs::GetAddress())) {
		myaddr[1].AnyAddress();
		AddLogLineC(CFormat(_("Could not bind ports to the specified address: %s"))
			% thePrefs::GetAddress());
	}

	wxString ip = myaddr[1].IPAddress();
	myaddr[1].Service(thePrefs::GetPort()+3);
	serverconnect = new CServerConnect(serverlist, myaddr[1]);
	*msg << CFormat( wxT("*** Server UDP socket (TCP+3) at %s:%u\n") )
		% ip % ((unsigned int)thePrefs::GetPort() + 3u);

	// Create the ListenSocket (aMule TCP socket).
	// Used for Client Port / Connections from other clients,
	// Client to Client Source Exchange.
	// Default is 4662.
	myaddr[2] = myaddr[1];
	myaddr[2].Service(thePrefs::GetPort());
	listensocket = new CListenSocket(myaddr[2]);
	*msg << CFormat( wxT("*** TCP socket (TCP) listening on %s:%u\n") )
		% ip % (unsigned int)(thePrefs::GetPort());
	// Notify(true) has already been called to the ListenSocket, so events may
	// be already coming in.
	if (!listensocket->IsOk()) {
		// If we wern't able to start listening, we need to warn the user
		wxString err;
		err = CFormat(_("Port %u is not available. You will be LOWID\n")) %
			(unsigned int)(thePrefs::GetPort());
		*msg << err;
		AddLogLineC(err);
		err.Clear();
		err = CFormat(
			_("Port %u is not available!\n\nThis means that you will be LOWID.\n\nCheck your network to make sure the port is open for output and input.")) %
			(unsigned int)(thePrefs::GetPort());
		ShowAlert(err, _("ERROR"), wxOK | wxICON_ERROR);
	}

	// Create the UDP socket.
	// Used for extended eMule protocol, Queue Rating, File Reask Ping.
	// Also used for Kademlia.
	// Default is port 4672.
	myaddr[3] = myaddr[1];
	myaddr[3].Service(thePrefs::GetUDPPort());
	clientudp = new CClientUDPSocket(myaddr[3], thePrefs::GetProxyData());
	if (!thePrefs::IsUDPDisabled()) {
		*msg << CFormat( wxT("*** Client UDP socket (extended eMule) at %s:%u") )
			% ip % thePrefs::GetUDPPort();
	} else {
		*msg << wxT("*** Client UDP socket (extended eMule) disabled on preferences");
	}

#ifdef ENABLE_UPNP
	if (!ApplyCoreUPnP(forceUPnPRetry)) {
		ok = false;
	}
#endif

	return ok;
}

#ifdef ENABLE_UPNP
void CamuleApp::BuildCoreUPnPMappings()
{
	if (m_upnpMappings.size() < 4) {
		m_upnpMappings.resize(4);
	}

	m_upnpMappings[0] = CUPnPPortMapping(
		thePrefs::ECPort(),
		"TCP",
		thePrefs::GetUPnPECEnabled(),
		"wMule TCP External Connections Socket");
	m_upnpMappings[1] = CUPnPPortMapping(
		thePrefs::GetPort() + 3,
		"UDP",
		thePrefs::GetUPnPEnabled(),
		"wMule UDP socket (TCP+3)");
	m_upnpMappings[2] = CUPnPPortMapping(
		thePrefs::GetPort(),
		"TCP",
		thePrefs::GetUPnPEnabled(),
		"wMule TCP Listen Socket");
	m_upnpMappings[3] = CUPnPPortMapping(
		thePrefs::GetUDPPort(),
		"UDP",
		thePrefs::GetUPnPEnabled(),
		"wMule UDP Extended eMule Socket");
}


void CamuleApp::ResetUPnPControlPoint()
{
	if (m_upnp) {
		m_upnp->DeletePortMappings(m_upnpMappings);
		delete m_upnp;
		m_upnp = nullptr;
	}
}


bool CamuleApp::ApplyCoreUPnP(bool forceRetry)
{
	const bool effectiveForceRetry = forceRetry || thePrefs::ConsumeUPnPForceRetry(wxT("core"));
	if (!thePrefs::GetUPnPEnabled()) {
		ResetUPnPControlPoint();
		CUPnPLastResult disabled = thePrefs::GetLastUPnPResultCore();
		disabled.status = UPNP_LAST_DISABLED;
		disabled.timestampUtc = wxGetUTCTimeMillis().GetValue();
		disabled.requested.clear();
		disabled.mapped.clear();
		disabled.lastError.clear();
		disabled.failureStage = UPNP_STAGE_NONE;
		thePrefs::SetLastUPnPResultCore(disabled);
		return false;
	}

	BuildCoreUPnPMappings();
	ResetUPnPControlPoint();

	bool success = false;
	try {
		m_upnp = new CUPnPControlPoint(thePrefs::GetUPnPTCPPort());
		wxStopWatch count;
		while (count.Time() < 3000 && !m_upnp->WanServiceDetected()) {
		}

		const CUPnPLastResult& lastResult = thePrefs::GetLastUPnPResultCore();
		CUPnPOperationReport report = m_upnp->ExecuteMappings(m_upnpMappings, wxT("core"), lastResult, effectiveForceRetry);
		thePrefs::SetLastUPnPResultCore(ToUPnPLastResult(report));
		if (report.status == UPNP_LAST_OK || report.status == UPNP_LAST_DISABLED) {
			AddLogLineN(FormatUPnPOperationSummary(report));
		} else {
			AddLogLineC(CFormat(wxT("WARNING: %s")) % FormatUPnPOperationSummary(report));
		}
		if (!report.lastError.IsEmpty() && report.status != UPNP_LAST_OK) {
			AddLogLineCS(report.lastError);
		}
		if (report.status == UPNP_LAST_FAIL || report.status == UPNP_LAST_SUPPRESSED) {
			AddLogLineCS(wxT("UPnP did not open the ports automatically. Configure manual forwarding on your router if connectivity is still blocked."));
		}
		success = (report.status == UPNP_LAST_OK);
	} catch (CUPnPException& e) {
		wxString error_msg;
		error_msg << e.what();
		AddLogLineC(error_msg);
		fprintf(stderr, "%s\n", (const char*)unicode2char(error_msg));
		success = false;
	}

	return success;
}
#endif // ENABLE_UPNP

bool CamuleApp::RetryCoreUPnP(bool forceRetry)
{
#ifdef ENABLE_UPNP
	return ApplyCoreUPnP(forceRetry);
#else
	wxUnusedVar(forceRetry);
	return false;
#endif
}

/* Original implementation by Bouc7 of the eMule Project.
   aMule Signature idea was designed by BigBob and implemented
   by Un-Thesis, with design inputs and suggestions from bothie.
*/
void CamuleApp::OnlineSig(bool zero /* reset stats (used on shutdown) */)
{
	// Do not do anything if online signature is disabled in Preferences
	if (!thePrefs::IsOnlineSignatureEnabled() || m_emulesig_path.IsEmpty()) {
		// We do not need to check m_amulesig_path because if m_emulesig_path is empty,
		// that means m_amulesig_path is empty too.
		return;
	}

	// Remove old signature files
	if ( wxFileExists( m_emulesig_path ) ) { wxRemoveFile( m_emulesig_path ); }
	if ( wxFileExists( m_amulesig_path ) ) { wxRemoveFile( m_amulesig_path ); }


	wxTextFile amulesig_out;
	wxTextFile emulesig_out;

	// Open both files if needed
	if ( !emulesig_out.Create( m_emulesig_path) ) {
		AddLogLineC(_("Failed to create OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	if ( !amulesig_out.Create(m_amulesig_path) ) {
		AddLogLineC(_("Failed to create wMule OnlineSig File"));
		// Will never try again.
		m_amulesig_path.Clear();
		m_emulesig_path.Clear();
		return;
	}

	wxString emulesig_string;
	wxString temp;

	if (zero) {
		emulesig_string = wxT("0\xA0.0|0.0|0");
		amulesig_out.AddLine(wxT("0\n0\n0\n0\n0\n0\n0.0\n0.0\n0\n0"));
	} else {
		if (IsConnectedED2K()) {

			temp = CFormat(wxT("%d")) % serverconnect->GetCurrentServer()->GetPort();

			// We are online
			emulesig_string =
				// Connected
				wxT("1|")
				//Server name
				+ serverconnect->GetCurrentServer()->GetListName()
				+ wxT("|")
				// IP and port of the server
				+ serverconnect->GetCurrentServer()->GetFullIP()
				+ wxT("|")
				+ temp;


			// Now for amule sig

			// Connected. State 1, full info
			amulesig_out.AddLine(wxT("1"));
			// Server Name
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetListName());
			// Server IP
			amulesig_out.AddLine(serverconnect->GetCurrentServer()->GetFullIP());
			// Server Port
			amulesig_out.AddLine(temp);

			if (serverconnect->IsLowID()) {
				amulesig_out.AddLine(wxT("L"));
			} else {
				amulesig_out.AddLine(wxT("H"));
			}

		} else if (serverconnect->IsConnecting()) {
			emulesig_string = wxT("0");

			// Connecting. State 2, No info.
			amulesig_out.AddLine(wxT("2\n0\n0\n0\n0"));
		} else {
			// Not connected to a server
			emulesig_string = wxT("0");

			// Not connected, state 0, no info
			amulesig_out.AddLine(wxT("0\n0\n0\n0\n0"));
		}
		if (IsConnectedKad()) {
			if(Kademlia::CKademlia::IsFirewalled()) {
				// Connected. Firewalled. State 1.
				amulesig_out.AddLine(wxT("1"));
			} else {
				// Connected. State 2.
				amulesig_out.AddLine(wxT("2"));
			}
		} else {
			// Not connected.State 0.
			amulesig_out.AddLine(wxT("0"));
		}
		emulesig_string += wxT("\xA");

		// Datarate for downloads
		temp = CFormat(wxT("%.1f")) % (theStats::GetDownloadRate() / 1024.0);

		emulesig_string += temp + wxT("|");
		amulesig_out.AddLine(temp);

		// Datarate for uploads
		temp = CFormat(wxT("%.1f")) % (theStats::GetUploadRate() / 1024.0);

		emulesig_string += temp + wxT("|");
		amulesig_out.AddLine(temp);

		// Number of users waiting for upload
		temp = CFormat(wxT("%d")) % theStats::GetWaitingUserCount();

		emulesig_string += temp;
		amulesig_out.AddLine(temp);

		// Number of shared files (not on eMule)
		amulesig_out.AddLine(CFormat(wxT("%d")) % theStats::GetSharedFileCount());
	}

	// eMule signature finished here. Write the line to the wxTextFile.
	emulesig_out.AddLine(emulesig_string);

	// Now for aMule signature extras

	// Nick on the network
	amulesig_out.AddLine(thePrefs::GetUserNick());

	// Total received in bytes
	amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetTotalReceivedBytes());

	// Total sent in bytes
	amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetTotalSentBytes());

	// amule version
#ifdef SVNDATE
	amulesig_out.AddLine(wxT(VERSION) wxT(" ") wxT(SVNDATE));
#else
	amulesig_out.AddLine(wxT(VERSION));
#endif

	if (zero) {
		amulesig_out.AddLine(wxT("0"));
		amulesig_out.AddLine(wxT("0"));
		amulesig_out.AddLine(wxT("0"));
	} else {
        // Total received bytes in session
		amulesig_out.AddLine( CFormat( wxT("%llu") ) %
			theStats::GetSessionReceivedBytes() );

        // Total sent bytes in session
		amulesig_out.AddLine( CFormat( wxT("%llu") ) %
			theStats::GetSessionSentBytes() );

		// Uptime
		amulesig_out.AddLine(CFormat(wxT("%llu")) % theStats::GetUptimeSeconds());
	}

	// Flush the files
	emulesig_out.Write();
	amulesig_out.Write();
} //End Added By Bouc7


#if wxUSE_ON_FATAL_EXCEPTION
// Gracefully handle fatal exceptions and print backtrace if possible
void CamuleApp::OnFatalException()
{
	/* Print the backtrace */
	wxString msg;
	msg	<< wxT("\n--------------------------------------------------------------------------------\n")
		<< wxT("A fatal error has occurred and wMule has crashed.\n")
		<< wxT("Please help us fix this problem by filing an issue on the wMule tracker:\n")
		<< wxT("    https://github.com/wMule/wMule/issues\n")
		<< wxT("Include as much information as possible regarding the\n")
		<< wxT("circumstances of this crash and attach the backtrace below.\n\n")
		<< wxT("----------------------------=| BACKTRACE FOLLOWS: |=----------------------------\n")
		<< wxT("Current version is: ") << FullMuleVersion
		<< wxT("\nRunning on: ") << OSDescription
		<< wxT("\n\n")
		<< get_backtrace(1) // 1 == skip this function.
		<< wxT("\n--------------------------------------------------------------------------------\n");

	theLogger.EmergencyLog(msg, true);
}
#endif


// Sets the localization of wMule
void CamuleApp::Localize_mule()
{
	InitCustomLanguages();
	const wxString langPref = thePrefs::GetLanguageID();
	int langId = wxLANGUAGE_DEFAULT;
	const wxLanguageInfo* resolvedInfo = nullptr;
	if (!langPref.IsEmpty()) {
		resolvedInfo = wxLocale::FindLanguageInfo(langPref);
		if (resolvedInfo) {
			langId = resolvedInfo->Language;
		} else {
			langId = StrLang2wx(langPref);
			resolvedInfo = wxLocale::GetLanguageInfo(langId);
		}
	}
	AddLogLineN(CFormat(wxT("[i18n] Startup language preference='%s' resolvedId=%d canonical='%s'"))
		% langPref
		% langId
		% (resolvedInfo ? resolvedInfo->CanonicalName : wxString(wxT("<null>"))));
	InitLocale(m_locale, langId);
	AddLogLineN(CFormat(wxT("[i18n] Locale init result ok=%d currentLanguage=%d"))
		% (m_locale.IsOk() ? 1 : 0)
		% m_locale.GetLanguage());
	if (!m_locale.IsOk()) {
		AddLogLineN(_("The selected locale seems not to be installed on your box. (Note: I'll try to set it anyway)"));
	}
}


// Displays information related to important changes in wMule.
// Is called when the user runs a new version of wMule
void CamuleApp::Trigger_New_version(wxString new_version)
{
	wxString info = wxT(" --- ") + CFormat(_("This is the first time you run wMule %s")) % new_version + wxT(" ---\n\n");
	if (new_version == wxT("SVN")) {
		info += _("This version is a testing version, updated daily, and\n");
		info += _("we give no warranty it won't break anything, burn your house,\n");
		info += _("or kill your dog. But it *should* be safe to use anyway.\n");
	}

	// General info
	info += wxT("\n");
	info += _("More information, support and new releases are available on the wMule project page:\n");
	info += wxT("    https://github.com/wMule/wMule\n\n");
	info += _("Please report any bugs or issues at:\n");
	info += wxT("    https://github.com/wMule/wMule/issues");

	ShowAlert(info, _("Info"), wxCENTRE | wxOK | wxICON_ERROR);
}


void CamuleApp::SetOSFiles(const wxString& new_path)
{
	if ( thePrefs::IsOnlineSignatureEnabled() ) {
		if ( ::wxDirExists(new_path) ) {
			m_emulesig_path = JoinPaths(new_path, wxT("onlinesig.dat"));
			m_amulesig_path = JoinPaths(new_path, wxT("amulesig.dat"));
		} else {
			ShowAlert(_("The folder for Online Signature files you specified is INVALID!\n OnlineSignature will be DISABLED until you fix it on preferences."), _("ERROR"), wxOK | wxICON_ERROR);
			m_emulesig_path.Clear();
			m_amulesig_path.Clear();
		}
	} else {
		m_emulesig_path.Clear();
		m_amulesig_path.Clear();
	}
}


#ifdef __WXDEBUG__
#ifndef wxUSE_STACKWALKER
#define wxUSE_STACKWALKER 0
#endif
void CamuleApp::OnAssertFailure(const wxChar* file, int line,
				const wxChar* func, const wxChar* cond, const wxChar* msg)
{
	wxString errmsg = CFormat( wxT("Assertion failed: %s:%s:%d: Assertion '%s' failed. %s\nBacktrace follows:\n%s\n") )
		% file % func % line % cond % ( msg ? msg : wxT("") )
		% get_backtrace(2);		// Skip the function-calls directly related to the assert call.
	theLogger.EmergencyLog(errmsg, false);

	if (wxThread::IsMain() && IsRunning()) {
		AMULE_APP_BASE::OnAssertFailure(file, line, func, cond, msg);
	} else {
		abort();
	}
}
#endif


void CamuleApp::OnUDPDnsDone(CMuleInternalEvent& evt)
{
	CServerUDPSocket* socket =reinterpret_cast<CServerUDPSocket*>(evt.GetClientData());
	socket->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnSourceDnsDone(CMuleInternalEvent& evt)
{
	downloadqueue->OnHostnameResolved(evt.GetExtraLong());
}


void CamuleApp::OnServerDnsDone(CMuleInternalEvent& evt)
{
	AddLogLineNS(_("Server hostname notified"));
	serverconnect->OnServerHostnameResolved(evt.GetClientData(), evt.GetExtraLong());
}


void CamuleApp::OnTCPTimer(CTimerEvent& WXUNUSED(evt))
{
	if(!IsRunning()) {
		return;
	}
	serverconnect->StopConnectionTry();
	if (IsConnectedED2K() ) {
		return;
	}
	serverconnect->ConnectToAnyServer();
}


void CamuleApp::OnCoreTimer(CTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section
	static uint64 msPrev1, msPrev5, msPrevSave, msPrevHist, msPrevOS, msPrevKnownMet;
	uint64 msCur = theStats::GetUptimeMillis();
	TheTime = msCur / 1000;

	if (!IsRunning()) {
		return;
	}

#ifndef AMULE_DAEMON
	// Check if we should terminate the app
	if ( g_shutdownSignal ) {
		wxWindow* top = GetTopWindow();

		if ( top ) {
			top->Close(true);
		} else {
			// No top-window, have to force termination.
			wxExit();
		}
	}
#endif

	// There is a theoretical chance that the core time function can recurse:
	// if an event function gets blocked on a mutex (communicating with the
	// UploadBandwidthThrottler) wx spawns a new event loop and processes more events.
	// If CPU load gets high a new core timer event could be generated before the last
	// one was finished and so recursion could occur, which would be bad.
	// Detect this and do an early return then.
	static bool recurse = false;
	if (recurse) {
		return;
	}
	recurse = true;

	if (Threading::ShutdownToken().IsActive()) {
		recurse = false;
		return;
	}

	const bool shouldLogThreading = thePrefs::GetVerboseThreading() || theLogger.IsEnabled(logThreading);
	if (shouldLogThreading && glob_prefs != nullptr) {
		const Threading::StatsSnapshot snapshot = Threading::CaptureStatsSnapshot(*glob_prefs);
		AddDebugLogLineC(logThreading,
			CFormat(wxT("threading pending=%u wakeups=%llu debt=%u partFileTimeouts=%u schedulerDrain=%d uploads=%u waiting=%u downloads=%u drainTimeoutMs=%lu dl=%llu ul=%llu"))
				% snapshot.ownership.pendingTasks
				% static_cast<unsigned long long>(snapshot.ownership.wakeups)
				% snapshot.ownership.throttlerDebt
				% snapshot.ownership.timedOutPartFiles
				% static_cast<int>(snapshot.ownership.lastDrainResult)
				% snapshot.activeUploads
				% snapshot.waitingUploads
				% snapshot.activeDownloads
				% static_cast<unsigned long>(snapshot.prefs.drainTimeout.count())
				% static_cast<unsigned long long>(snapshot.sessionDownloadedBytes)
				% static_cast<unsigned long long>(snapshot.sessionUploadedBytes));
	}

	uploadqueue->Process();
	downloadqueue->Process();
	//theApp->clientcredits->Process();
	theStats::CalculateRates();

	if (msCur-msPrevHist > 1000) {
		// unlike the other loop counters in this function this one will sometimes
		// produce two calls in quick succession (if there was a gap of more than one
		// second between calls to TimerProc) - this is intentional!  This way the
		// history list keeps an average of one node per second and gets thinned out
		// correctly as time progresses.
		msPrevHist += 1000;

		m_statistics->RecordHistory();

	}


	if (msCur-msPrev1 > 1000) {  // approximately every second
		msPrev1 = msCur;
		clientcredits->Process();
		clientlist->Process();

		// Publish files to server if needed.
		sharedfiles->Process();

		if( Kademlia::CKademlia::IsRunning() ) {
			Kademlia::CKademlia::Process();
			if(Kademlia::CKademlia::GetPrefs()->HasLostConnection()) {
				StopKad();
				clientudp->Close();
				clientudp->Open();
				if (thePrefs::Reconnect()) {
					StartKad();
				}
			}
		}

		if( serverconnect->IsConnecting() && !serverconnect->IsSingleConnect() ) {
			serverconnect->TryAnotherConnectionrequest();
		}
		if (serverconnect->IsConnecting()) {
			serverconnect->CheckForTimeout();
		}
		listensocket->UpdateConnectionsStatus();

	}


	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		listensocket->Process();
	}

	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		theStats::Save();
	}

	// Special
	if (msCur - msPrevOS >= thePrefs::GetOSUpdate() * 1000ull) {
		OnlineSig(); // Added By Bouc7
		msPrevOS = msCur;
	}

	if (msCur - msPrevKnownMet >= 30*60*1000/*There must be a prefs option for this*/) {
		// Save Shared Files data
		knownfiles->Save();
		msPrevKnownMet = msCur;
	}


	// Recommended by lugdunummaster himself - from emule 0.30c
	serverconnect->KeepConnectionAlive();

	// Disarm recursion protection
	recurse = false;
}


void CamuleApp::OnFinishedHashing(CHashingEvent& evt)
{
	wxCHECK_RET(evt.GetResult(), wxT("No result of hashing"));

	CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
	CKnownFile* result = evt.GetResult();

	if (owner) {
		// Check if the partfile still exists, as it might have
		// been deleted in the mean time.
		if (downloadqueue->IsPartFile(owner)) {
			// This cast must not be done before the IsPartFile
			// call, as dynamic_cast will barf on dangling pointers.
			dynamic_cast<CPartFile*>(owner)->PartFileHashFinished(result);
		}
	} else {
		static uint64 bytecount = 0;

		if (knownfiles->SafeAddKFile(result, true)) {
			AddDebugLogLineN(logKnownFiles,
				CFormat(wxT("Safe adding file to sharedlist: %s")) % result->GetFileName());
			sharedfiles->SafeAddKFile(result);

			bytecount += result->GetFileSize();
			// If we have added files with a total size of ~3000mb
			if (bytecount >= wxULL(3145728000)) {
				AddDebugLogLineN(logKnownFiles, wxT("Failsafe for crash on file hashing creation"));
				if ( m_app_state != APP_STATE_SHUTTINGDOWN ) {
					knownfiles->Save();
					bytecount = 0;
				}
			}
		} else {
			AddDebugLogLineN(logKnownFiles,
				CFormat(wxT("File not added to sharedlist: %s")) % result->GetFileName());
			delete result;
		}
	}
}


void CamuleApp::OnFinishedAICHHashing(CHashingEvent& evt)
{
	wxCHECK_RET(evt.GetResult(), wxT("No result of AICH-hashing"));

	CKnownFile* owner = const_cast<CKnownFile*>(evt.GetOwner());
	CScopedPtr<CKnownFile> result(evt.GetResult());

	if (result->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE) {
		CAICHHashSet* oldSet = owner->GetAICHHashset();
		CAICHHashSet* newSet = result->GetAICHHashset();

		owner->SetAICHHashset(newSet);
		newSet->SetOwner(owner);

		result->SetAICHHashset(oldSet);
		oldSet->SetOwner(result.get());
	}
}


void CamuleApp::OnFinishedCompletion(CCompletionEvent& evt)
{
	CPartFile* completed = const_cast<CPartFile*>(evt.GetOwner());
	wxCHECK_RET(completed, wxT("Completion event sent for unspecified file"));
	wxASSERT_MSG(downloadqueue->IsPartFile(completed), wxT("CCompletionEvent for unknown partfile."));

	completed->CompleteFileEnded(evt.ErrorOccured(), evt.GetFullPath());
	if (evt.ErrorOccured()) {
		CUserEvents::ProcessEvent(CUserEvents::ErrorOnCompletion, completed);
	}

	// Check if we should execute an script/app/whatever.
	CUserEvents::ProcessEvent(CUserEvents::DownloadCompleted, completed);
}

void CamuleApp::OnFinishedAllocation(CAllocFinishedEvent& evt)
{
	CPartFile *file = evt.GetFile();
	wxCHECK_RET(file, wxT("Allocation finished event sent for unspecified file"));
	wxASSERT_MSG(downloadqueue->IsPartFile(file), wxT("CAllocFinishedEvent for unknown partfile"));

	file->SetStatus(PS_EMPTY);

	if (evt.Succeeded()) {
		if (evt.IsPaused()) {
			file->StopFile();
		} else {
			file->ResumeFile();
		}
	} else {
		AddLogLineN(CFormat(_("Disk space preallocation for file '%s' failed: %s")) % file->GetFileName() % wxString(UTF82unicode(std::strerror(evt.GetResult()))));
		file->StopFile();
	}

	file->AllocationFinished();
};

void CamuleApp::OnNotifyEvent(CMuleGUIEvent& evt)
{
#ifdef AMULE_DAEMON
	evt.Notify();
#else
	if (theApp->amuledlg) {
		evt.Notify();
	}
#endif
}


void CamuleApp::ShutDown()
{
	// Just in case
	PlatformSpecific::AllowSleepMode();

	// Log
	AddDebugLogLineN(logGeneral, wxT("CamuleApp::ShutDown() has started."));

	// Signal the hashing thread to terminate
	m_app_state = APP_STATE_SHUTTINGDOWN;
	Threading::ShutdownToken().Activate();
	AddDebugLogLineN(logThreading, wxT("Thread shutdown token activated"));
	const std::chrono::milliseconds drainTimeout(thePrefs::GetThreadDrainTimeoutMs());
	if (downloadqueue != nullptr) {
		const uint16 fileCount = downloadqueue->GetFileCount();
		for (uint16 i = 0; i < fileCount; ++i) {
			if (CPartFile* part = downloadqueue->GetFileByIndex(i)) {
				part->RequestAsyncTaskShutdown(drainTimeout);
			}
		}
	}

	// Stop ASIO thread
#ifdef ASIO_SOCKETS			// only needed to suppress the log message in non-Asio build
	AddDebugLogLineN(logGeneral, wxT("Terminate ASIO thread."));
	m_AsioService->Stop();
#endif

	StopKad();

	// Kry - Save the sources seeds on app exit
	if (thePrefs::GetSrcSeedsOn()) {
		downloadqueue->SaveSourceSeeds();
	}

	OnlineSig(true); // Added By Bouc7

	// Exit HTTP downloads
	CHTTPDownloadThread::StopAll();

	const Threading::DrainResult schedulerDrain = CThreadScheduler::Drain(drainTimeout);
	const Threading::DrainResult poolDrain = CThreadPool::Instance().Drain(drainTimeout);
	const Threading::DrainResult throttlerDrain = uploadBandwidthThrottler->Drain(drainTimeout);

	// Exit thread scheduler and upload thread
	CThreadScheduler::Terminate();

	AddDebugLogLineN(logGeneral, wxT("Terminate upload thread."));
	uploadBandwidthThrottler->EndThread();

	if (glob_prefs != nullptr) {
		const Threading::StatsSnapshot snapshot = Threading::CaptureStatsSnapshot(*glob_prefs);
		AddDebugLogLineN(logThreading,
			CFormat(wxT("ShutdownSummary pending=%u wakeups=%llu debt=%u partFileTimeouts=%u schedulerDrain=%d poolDrain=%d throttlerDrain=%d uploads=%u waiting=%u downloads=%u drainTimeoutMs=%lu"))
				% snapshot.ownership.pendingTasks
				% static_cast<unsigned long long>(snapshot.ownership.wakeups)
				% snapshot.ownership.throttlerDebt
				% snapshot.ownership.timedOutPartFiles
				% static_cast<int>(schedulerDrain)
				% static_cast<int>(poolDrain)
				% static_cast<int>(throttlerDrain)
				% snapshot.activeUploads
				% snapshot.waitingUploads
				% snapshot.activeDownloads
				% static_cast<unsigned long>(snapshot.prefs.drainTimeout.count()));
	}

	// Close sockets to avoid new clients coming in
	if (listensocket) {
		listensocket->Close();
		listensocket->KillAllSockets();
	}

	if (serverconnect) {
		serverconnect->Disconnect();
	}

	ECServerHandler->KillAllSockets();

#ifdef ENABLE_UPNP
	ResetUPnPControlPoint();
#endif

	// saving data & stuff
	if (knownfiles) {
		knownfiles->Save();
	}

	theStats::Save();

	CPath configFileName = CPath(thePrefs::GetConfigDir() + m_configFile);
	CPath::BackupFile(configFileName, wxT(".bak"));

	if (clientlist) {
		clientlist->DeleteAll();
	}

	// Log
	AddDebugLogLineN(logGeneral, wxT("CamuleApp::ShutDown() has ended."));
}


bool CamuleApp::AddServer(CServer *srv, bool fromUser)
{
	if ( serverlist->AddServer(srv, fromUser) ) {
		Notify_ServerAdd(srv);
		return true;
	}
	return false;
}


uint32 CamuleApp::GetPublicIP(bool ignorelocal) const
{
	if (m_dwPublicIP == 0) {
		if (Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::GetIPAddress() ) {
			return wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetIPAddress());
		} else {
			return ignorelocal ? 0 : m_localip;
		}
	}

	return m_dwPublicIP;
}


void CamuleApp::SetPublicIP(const uint32 dwIP)
{
	wxASSERT((dwIP == 0) || !IsLowID(dwIP));

	if (dwIP != 0 && dwIP != m_dwPublicIP && serverlist != nullptr) {
		m_dwPublicIP = dwIP;
		serverlist->CheckForExpiredUDPKeys();
	} else {
		m_dwPublicIP = dwIP;
	}
}


wxString CamuleApp::GetLog(bool reset)
{
	wxFile logfile;
	logfile.Open(theLogger.GetLogfileName());
	if ( !logfile.IsOpened() ) {
		return _("ERROR: can't open logfile");
	}
	int len = logfile.Length();
	if ( len == 0 ) {
		return _("WARNING: logfile is empty. Something is wrong.");
	}
	char *tmp_buffer = new char[len + sizeof(wxChar)];
	logfile.Read(tmp_buffer, len);
	memset(tmp_buffer + len, 0, sizeof(wxChar));

	// try to guess file format
	wxString str;
	if (tmp_buffer[0] && tmp_buffer[1]) {
		str = wxString(UTF82unicode(tmp_buffer));
	} else {
		str = wxWCharBuffer((wchar_t *)tmp_buffer);
	}

	delete [] tmp_buffer;
	if ( reset ) {
		theLogger.CloseLogfile();
		if (theLogger.OpenLogfile(theLogger.GetLogfileName())) {
			AddLogLineN(_("Log has been reset"));
		}
		ECServerHandler->ResetAllLogs();
	}
	return str;
}


wxString CamuleApp::GetServerLog(bool reset)
{
	wxString ret = server_msg;
	if ( reset ) {
		server_msg.Clear();
	}
	return ret;
}

wxString CamuleApp::GetDebugLog(bool reset)
{
	return GetLog(reset);
}


void CamuleApp::AddServerMessageLine(wxString &msg)
{
	server_msg += msg + wxT("\n");
	AddLogLineN(CFormat(_("ServerMessage: %s")) % msg);
}



void CamuleApp::OnFinishedHTTPDownload(CMuleInternalEvent& event)
{
	switch (event.GetInt()) {
		case HTTP_IPFilter:
			ipfilter->DownloadFinished(event.GetExtraLong());
			break;
		case HTTP_ServerMet:
			if (serverlist->DownloadFinished(event.GetExtraLong()) && !IsConnectedED2K()) {
				// If successfully downloaded a server list, and are not connected at the moment, try to connect.
				// This happens when no server met is available on startup.
				serverconnect->ConnectToAnyServer();
			}
			break;
		case HTTP_ServerMetAuto:
			serverlist->AutoDownloadFinished(event.GetExtraLong());
			break;
		case HTTP_VersionCheck:
			CheckNewVersion(event.GetExtraLong());
			break;
		case HTTP_NodesDat:
			if (event.GetExtraLong() == HTTP_Success) {

				wxString file = thePrefs::GetConfigDir() + wxT("nodes.dat");
				if (wxFileExists(file)) {
					wxRemoveFile(file);
				}

				if ( Kademlia::CKademlia::IsRunning() ) {
					Kademlia::CKademlia::Stop();
				}

				wxRenameFile(file + wxT(".download"),file);

				Kademlia::CKademlia::Start();
				theApp->ShowConnectionState();
			// cppcheck-suppress duplicateBranch
			} else if (event.GetExtraLong() == HTTP_Skipped) {
				AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % wxT("nodes.dat"));
			} else {
				AddLogLineC(_("Failed to download the nodes list."));
			}
			break;
#ifdef ENABLE_IP2COUNTRY
		case HTTP_GeoIP:
			theApp->amuledlg->IP2CountryDownloadFinished(event.GetExtraLong());
			// If we updated, the dialog is already up. Redraw it to show the flags.
			theApp->amuledlg->Refresh();
			break;
#endif
	}
}

void CamuleApp::CheckNewVersion(uint32 result)
{
	if (result == HTTP_Success) {
		wxString filename = thePrefs::GetConfigDir() + wxT("last_version_check");
		wxTextFile file;

		if (!file.Open(filename)) {
			AddLogLineC(_("Failed to open the downloaded version check file") );
			return;
		} else if (!file.GetLineCount()) {
			AddLogLineC(_("Corrupted version check file"));
		} else {
			wxString versionLine = file.GetFirstLine();
			wxStringTokenizer tkz(versionLine, wxT("."));

			AddDebugLogLineN(logGeneral, wxString(wxT("Running: ")) + wxT(VERSION) + wxT(", Version check: ") + versionLine);

			long fields[] = {0, 0, 0};
			for (int i = 0; i < 3; ++i) {
				if (!tkz.HasMoreTokens()) {
					AddLogLineC(_("Corrupted version check file"));
					return;
				} else {
					wxString token = tkz.GetNextToken();

					if (!token.ToLong(&fields[i])) {
						AddLogLineC(_("Corrupted version check file"));
						return;
					}
				}
			}

			long curVer = make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE);
			long newVer = make_full_ed2k_version(fields[0], fields[1], fields[2]);

			if (curVer < newVer) {
				AddLogLineC(_("You are using an outdated version of wMule!"));
				// cppcheck-suppress zerodiv
				AddLogLineN(CFormat(_("Your wMule version is %i.%i.%i and the latest version is %li.%li.%li")) % VERSION_MJR % VERSION_MIN % VERSION_UPDATE % fields[0] % fields[1] % fields[2]);
				AddLogLineN(_("The latest version can always be found at https://github.com/wMule/wMule"));
				#ifdef AMULE_DAEMON
				AddLogLineCS(CFormat(_("WARNING: Your wMuleD version is outdated: %i.%i.%i < %li.%li.%li"))
					% VERSION_MJR % VERSION_MIN % VERSION_UPDATE % fields[0] % fields[1] % fields[2]);
				#endif
			} else {
				AddLogLineN(_("Your copy of wMule is up to date."));
			}
		}

		file.Close();
		wxRemoveFile(filename);
	} else {
		AddLogLineN(wxT("WARNING: Failed to download the version check file"));
	}

}


bool CamuleApp::IsConnected() const
{
	return (IsConnectedED2K() || IsConnectedKad());
}


bool CamuleApp::IsConnectedED2K() const
{
	return serverconnect && serverconnect->IsConnected();
}


bool CamuleApp::IsConnectedKad() const
{
	return Kademlia::CKademlia::IsConnected();
}


bool CamuleApp::IsFirewalled() const
{
	if (theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID()) {
		return false; // we have an eD2K HighID -> not firewalled
	}

	return IsFirewalledKad(); // If kad says ok, it's ok.
}

bool CamuleApp::IsFirewalledKad() const
{
	return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
			|| Kademlia::CKademlia::IsFirewalled();
}

bool CamuleApp::IsFirewalledKadUDP() const
{
	return !Kademlia::CKademlia::IsConnected()		// not connected counts as firewalled
			|| Kademlia::CUDPFirewallTester::IsFirewalledUDP(true);
}

bool CamuleApp::IsKadRunning() const
{
	return Kademlia::CKademlia::IsRunning();
}

bool CamuleApp::IsKadRunningInLanMode() const
{
	return Kademlia::CKademlia::IsRunningInLANMode();
}

// Kad stats
uint32 CamuleApp::GetKadUsers() const
{
	return Kademlia::CKademlia::GetKademliaUsers();
}

uint32 CamuleApp::GetKadFiles() const
{
	return Kademlia::CKademlia::GetKademliaFiles();
}

uint32 CamuleApp::GetKadIndexedSources() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexSource;
}

uint32 CamuleApp::GetKadIndexedKeywords() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexKeyword;
}

uint32 CamuleApp::GetKadIndexedNotes() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexNotes;
}

uint32 CamuleApp::GetKadIndexedLoad() const
{
	return Kademlia::CKademlia::GetIndexed()->m_totalIndexLoad;
}


// True IP of machine
uint32 CamuleApp::GetKadIPAdress() const
{
	return wxUINT32_SWAP_ALWAYS(Kademlia::CKademlia::GetPrefs()->GetIPAddress());
}

// Buddy status
uint8	CamuleApp::GetBuddyStatus() const
{
	return clientlist->GetBuddyStatus();
}

uint32	CamuleApp::GetBuddyIP() const
{
	return clientlist->GetBuddyIP();
}

uint32	CamuleApp::GetBuddyPort() const
{
	return clientlist->GetBuddyPort();
}

const Kademlia::CUInt128& CamuleApp::GetKadID() const
{
	return Kademlia::CKademlia::GetKadID();
}

bool CamuleApp::CanDoCallback(uint32 clientServerIP, uint16 clientServerPort)
{
	if (Kademlia::CKademlia::IsConnected()) {
		if (IsConnectedED2K()) {
			if (serverconnect->IsLowID()) {
				if (Kademlia::CKademlia::IsFirewalled()) {
					//Both Connected - Both Firewalled
					return false;
				} else {
					if (clientServerIP == theApp->serverconnect->GetCurrentServer()->GetIP() &&
					   clientServerPort == theApp->serverconnect->GetCurrentServer()->GetPort()) {
						// Both Connected - Server lowID, Kad Open - Client on same server
						// We prevent a callback to the server as this breaks the protocol
						// and will get you banned.
						return false;
					} else {
						// Both Connected - Server lowID, Kad Open - Client on remote server
						return true;
					}
				}
			} else {
				//Both Connected - Server HighID, Kad don't care
				return true;
			}
		} else {
			if (Kademlia::CKademlia::IsFirewalled()) {
				//Only Kad Connected - Kad Firewalled
				return false;
			} else {
				//Only Kad Connected - Kad Open
				return true;
			}
		}
	} else {
		if (IsConnectedED2K()) {
			if (serverconnect->IsLowID()) {
				//Only Server Connected - Server LowID
				return false;
			} else {
				//Only Server Connected - Server HighID
				return true;
			}
		} else {
			//We are not connected at all!
			return false;
		}
	}
}

void CamuleApp::ShowUserCount() {
	uint32 totaluser = 0, totalfile = 0;

	theApp->serverlist->GetUserFileStatus( totaluser, totalfile );

	wxString buffer;

	static const wxString s_singlenetstatusformat = _("Users: %s | Files: %s");
	static const wxString s_bothnetstatusformat = _("Users: E: %s K: %s | Files: E: %s K: %s");

	if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_bothnetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(totalfile) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
	} else if (thePrefs::GetNetworkED2K()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(totaluser) % CastItoIShort(totalfile);
	} else if (thePrefs::GetNetworkKademlia()) {
		buffer = CFormat(s_singlenetstatusformat) % CastItoIShort(Kademlia::CKademlia::GetKademliaUsers()) % CastItoIShort(Kademlia::CKademlia::GetKademliaFiles());
	} else {
		buffer = _("No networks selected");
	}

	Notify_ShowUserCount(buffer);
}


#ifndef ASIO_SOCKETS
void CamuleApp::ListenSocketHandler(wxSocketEvent& event)
{
	{ wxCHECK_RET(listensocket, wxT("Connection-event for nullptr'd listen-socket")); }
	{ wxCHECK_RET(event.GetSocketEvent() == wxSOCKET_CONNECTION,
		wxT("Invalid event received for listen-socket")); }

	if (m_app_state == APP_STATE_RUNNING) {
		listensocket->OnAccept();
	} else if (m_app_state == APP_STATE_STARTING) {
		// When starting up, connection may be made before we are able
		// to handle them. However, if these are ignored, no further
		// connection-events will be triggered, so we have to accept it.
		CLibSocket* socket = listensocket->Accept(false);

		wxCHECK_RET(socket, wxT("nullptr returned by Accept() during startup"));

		socket->Destroy();
	}
}
#endif


void CamuleApp::ShowConnectionState(bool forceUpdate)
{
	static uint8 old_state = (1<<7); // This flag doesn't exist

	uint8 state = 0;

	if (theApp->serverconnect->IsConnected()) {
		state |= CONNECTED_ED2K;
	}

	if (Kademlia::CKademlia::IsRunning()) {
		if (Kademlia::CKademlia::IsConnected()) {
			if (!Kademlia::CKademlia::IsFirewalled()) {
				state |= CONNECTED_KAD_OK;
			} else {
				state |= CONNECTED_KAD_FIREWALLED;
			}
		} else {
			state |= CONNECTED_KAD_NOT;
		}
	}

	if (old_state != state) {
		// Get the changed value
		int changed_flags = old_state ^ state;

		if (changed_flags & CONNECTED_ED2K) {
			// ED2K status changed
			wxString connected_server;
			CServer* ed2k_server = theApp->serverconnect->GetCurrentServer();
			if (ed2k_server) {
				connected_server = ed2k_server->GetListName();
			}
			if (state & CONNECTED_ED2K) {
				// We connected to some server
				const wxString id = theApp->serverconnect->IsLowID() ? _("with LowID") : _("with HighID");

				AddLogLineN(CFormat(_("Connected to %s %s")) % connected_server % id);
			} else {
				// cppcheck-suppress duplicateBranch
				if ( theApp->serverconnect->IsConnecting() ) {
					AddLogLineN(CFormat(_("Connecting to %s")) % connected_server);
				} else {
					AddLogLineN(_("Disconnected from eD2k"));
				}
			}
		}

		if (changed_flags & CONNECTED_KAD_NOT) {
			// cppcheck-suppress duplicateBranch
			if (state & CONNECTED_KAD_NOT) {
				AddLogLineN(wxT("Kad started."));
			} else {
				AddLogLineN(wxT("Kad stopped."));
			}
		}

		if (changed_flags & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
			if (state & (CONNECTED_KAD_OK | CONNECTED_KAD_FIREWALLED)) {
				// cppcheck-suppress duplicateBranch
				if (state & CONNECTED_KAD_OK) {
					AddLogLineN(wxT("Connected to Kad (ok)"));
				} else {
					AddLogLineN(wxT("WARNING: Connected to Kad (firewalled)"));
				}
			} else {
				AddLogLineN(wxT("Disconnected from Kad"));
			}
		}

		old_state = state;

		theApp->downloadqueue->OnConnectionState(IsConnected());
	}

	ShowUserCount();
	Notify_ShowConnState(forceUpdate);
}


#ifndef ASIO_SOCKETS
void CamuleApp::UDPSocketHandler(wxSocketEvent& event)
{
	CMuleUDPSocket* socket = reinterpret_cast<CMuleUDPSocket*>(event.GetClientData());
	wxCHECK_RET(socket, wxT("No socket owner specified."));

	if (IsOnShutDown() || thePrefs::IsUDPDisabled()) return;

	if (!IsRunning()) {
		if (event.GetSocketEvent() == wxSOCKET_INPUT) {
			// Back to the queue!
			theApp->AddPendingEvent(event);
			return;
		}
	}

	switch (event.GetSocketEvent()) {
		case wxSOCKET_INPUT:
			socket->OnReceive(0);
			break;

		case wxSOCKET_OUTPUT:
			socket->OnSend(0);
			break;

		case wxSOCKET_LOST:
			socket->OnDisconnected(0);
			break;

		default:
			wxFAIL;
			break;
	}
}
#endif


void CamuleApp::OnUnhandledException()
{
	// Call the generic exception-handler.
	fprintf(stderr, "\twMule Version: %s\n", (const char*)unicode2char(GetFullMuleVersion()));
	::OnUnhandledException();
}

void CamuleApp::StartKad()
{
	if (!Kademlia::CKademlia::IsRunning() && thePrefs::GetNetworkKademlia()) {
		// Kad makes no sense without the Client-UDP socket.
		if (!thePrefs::IsUDPDisabled()) {
			if (ipfilter->IsReady()) {
				Kademlia::CKademlia::Start();
			} else {
				ipfilter->StartKADWhenReady();
			}
		} else {
			AddLogLineC(_("Kad network cannot be used if UDP port is disabled on preferences, not starting."));
		}
	} else if (!thePrefs::GetNetworkKademlia()) {
		AddLogLineC(_("Kad network disabled on preferences, not connecting."));
	}
}

void CamuleApp::StopKad()
{
	// Stop Kad if it's running
	if (Kademlia::CKademlia::IsRunning()) {
		Kademlia::CKademlia::Stop();
	}
}


void CamuleApp::BootstrapKad(uint32 ip, uint16 port)
{
	if (!Kademlia::CKademlia::IsRunning()) {
		Kademlia::CKademlia::Start();
		theApp->ShowConnectionState();
	}

	Kademlia::CKademlia::Bootstrap(ip, port);
}


void CamuleApp::UpdateNotesDat(const wxString& url)
{
	wxString strTempFilename(thePrefs::GetConfigDir() + wxT("nodes.dat.download"));

	CHTTPDownloadThread *downloader = new CHTTPDownloadThread(url, strTempFilename, thePrefs::GetConfigDir() + wxT("nodes.dat"), HTTP_NodesDat, true, false);
	downloader->Create();
	downloader->Run();
}


void CamuleApp::DisconnectED2K()
{
	// Stop ED2K if it's running
	if (IsConnectedED2K()) {
		serverconnect->Disconnect();
	}
}

bool CamuleApp::CryptoAvailable() const
{
	return clientcredits && clientcredits->CryptoAvailable();
}

uint32 CamuleApp::GetED2KID() const {
	return serverconnect ? serverconnect->GetClientID() : 0;
}

uint32 CamuleApp::GetID() const {
	uint32 ID;

	if( Kademlia::CKademlia::IsConnected() && !Kademlia::CKademlia::IsFirewalled() ) {
		// We trust Kad above ED2K
		ID = ENDIAN_NTOHL(Kademlia::CKademlia::GetIPAddress());
	} else if( theApp->serverconnect->IsConnected() ) {
		ID = theApp->serverconnect->GetClientID();
	} else if ( Kademlia::CKademlia::IsConnected() && Kademlia::CKademlia::IsFirewalled() ) {
		// A firewalled Kad client gets a "1"
		ID = 1;
	} else {
		ID = 0;
	}

	return ID;
}

DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE)
DEFINE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE)
// File_checked_for_headers
