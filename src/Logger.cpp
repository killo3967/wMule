//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "Logger.h"
#include "amule.h"
#include "Preferences.h"
#include <common/Macros.h>
#include <common/MacrosProgramSpecific.h>
#include <sstream>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/sstream.h>
#include <wx/filename.h>

namespace {

const wxChar* PERSISTENT_DEFAULT_SEPARATOR = wxT(";");

wxString DetermineLogLevelLabel(bool critical, const wxString& message)
{
	const wxString fatalPrefix = _("FATAL: ");
	if (message.StartsWith(fatalPrefix)) {
		return wxT("FATAL");
	}

	const wxString criticalPrefix = _("CRITICAL: ");
	if (message.StartsWith(criticalPrefix)) {
		return wxT("CRITICAL");
	}

	const wxString warningPrefix = _("WARNING: ");
	if (message.StartsWith(warningPrefix)) {
		return wxT("WARNING");
	}

	const wxString errorPrefix = _("ERROR: ");
	if (message.StartsWith(errorPrefix)) {
		return wxT("ERROR");
	}

	return critical ? wxT("CRITICAL") : wxT("INFO");
}

wxString WrapLogColumn(const wxString& value)
{
	return wxT(" ") + value.Strip(wxString::both) + wxT(" ");
}

wxString DetermineModuleLabel(const wxString& category)
{
	const wxString cleanCategory = category.Strip(wxString::both);
	return cleanCategory.IsEmpty() ? wxString(wxT("CORE")) : cleanCategory;
}

wxString StripKnownLogPrefix(const wxString& message)
{
	const wxString cleanMessage = message.Strip(wxString::both);
	static const wxChar* const prefixes[] = {
		wxT("FATAL: "),
		wxT("CRITICAL: "),
		wxT("ERROR: "),
		wxT("WARNING: ")
	};

	for (const wxChar* prefix : prefixes) {
		if (cleanMessage.StartsWith(prefix)) {
			return cleanMessage.Mid(wxString(prefix).Length()).Strip(wxString::both);
		}
	}

	return cleanMessage;
}

wxString ExtractFileLeaf(const wxString& file)
{
	if (file.IsEmpty()) {
		return wxString();
	}

	wxString leaf = file.AfterLast(wxFileName::GetPathSeparator());
	leaf = leaf.AfterLast(wxT('/'));
	return leaf;
}

wxString BuildOriginLabel(const wxString& file, int line)
{
	const wxString bareFile = ExtractFileLeaf(file).Strip(wxString::both);
	if (bareFile.IsEmpty()) {
		return wxString();
	}

	wxString origin(bareFile);
	if (line > 0) {
		origin << wxT("(") << line << wxT(")");
	}
	return origin;
}

}


DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_LOGLINE)


CDebugCategory g_debugcats[] = {
	CDebugCategory( logGeneral,		wxT("General") ),
	CDebugCategory( logHasher,		wxT("Hasher") ),
	CDebugCategory( logClient,		wxT("ED2k Client") ),
	CDebugCategory( logLocalClient,		wxT("Local Client Protocol") ),
	CDebugCategory( logRemoteClient,	wxT("Remote Client Protocol") ),
	CDebugCategory( logPacketErrors,	wxT("Packet Parsing Errors") ),
	CDebugCategory( logCFile,		wxT("CFile") ),
	CDebugCategory( logFileIO,		wxT("FileIO") ),
	CDebugCategory( logZLib,		wxT("ZLib") ),
	CDebugCategory( logAICHThread,		wxT("AICH-Hasher") ),
	CDebugCategory( logAICHTransfer,	wxT("AICH-Transfer") ),
	CDebugCategory( logAICHRecovery,	wxT("AICH-Recovery") ),
	CDebugCategory( logListenSocket,	wxT("ListenSocket") ),
	CDebugCategory( logCredits,		wxT("Credits") ),
	CDebugCategory( logClientUDP,		wxT("ClientUDPSocket") ),
	CDebugCategory( logDownloadQueue,	wxT("DownloadQueue") ),
	CDebugCategory( logIPFilter,		wxT("IPFilter") ),
	CDebugCategory( logKnownFiles,		wxT("KnownFileList") ),
	CDebugCategory( logPartFile,		wxT("PartFiles") ),
	CDebugCategory( logSHAHashSet,		wxT("SHAHashSet") ),
	CDebugCategory( logServer,		wxT("Servers") ),
	CDebugCategory( logProxy,		wxT("Proxy") ),
	CDebugCategory( logSearch,		wxT("Searching") ),
	CDebugCategory( logServerUDP,		wxT("ServerUDP") ),
	CDebugCategory( logClientKadUDP,	wxT("Client Kademlia UDP") ),
	CDebugCategory( logKadSearch,		wxT("Kademlia Search") ),
	CDebugCategory( logKadRouting,		wxT("Kademlia Routing") ),
	CDebugCategory( logKadIndex,		wxT("Kademlia Indexing") ),
	CDebugCategory( logKadMain,		wxT("Kademlia Main Thread") ),
	CDebugCategory( logKadPrefs,		wxT("Kademlia Preferences") ),
	CDebugCategory( logPfConvert,		wxT("PartFileConvert") ),
	CDebugCategory( logMuleUDP,		wxT("MuleUDPSocket" ) ),
	CDebugCategory( logThreads,		wxT("ThreadScheduler" ) ),
	CDebugCategory( logThreading,		wxT("Threading" ) ),
	CDebugCategory( logUPnP,		wxT("Universal Plug and Play" ) ),
	CDebugCategory( logKadUdpFwTester,	wxT("Kademlia UDP Firewall Tester") ),
	CDebugCategory( logKadPacketTracking,	wxT("Kademlia Packet Tracking") ),
	CDebugCategory( logKadEntryTracking,	wxT("Kademlia Entry Tracking") ),
	CDebugCategory( logEC,			wxT("External Connect") ),
	CDebugCategory( logHTTP,		wxT("HTTP") ),
	CDebugCategory( logAsio,		wxT("Asio Sockets") )
};


const int categoryCount = itemsof(g_debugcats);



#ifdef __DEBUG__
bool CLogger::IsEnabled( DebugType type ) const
{
	int index = (int)type;

	if ( index >= 0 && index < categoryCount ) {
		const CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		return ( cat.IsEnabled() && thePrefs::GetVerbose() );
	}

	wxFAIL;
	return false;
}
#endif


void CLogger::SetEnabled( DebugType type, bool enabled )
{
	int index = (int)type;

	if ( index >= 0 && index < categoryCount ) {
		CDebugCategory& cat = g_debugcats[ index ];
		wxASSERT( type == cat.GetType() );

		cat.SetEnabled( enabled );
	} else {
		wxFAIL;
	}
}


void CLogger::ConfigurePersistentOutput(const wxString& separator)
{
	m_persistentSeparator = separator.IsEmpty() ? wxString(PERSISTENT_DEFAULT_SEPARATOR) : separator;
}


void CLogger::AddLogLine(
	const wxString& file,
	int line,
	bool critical,
	DebugType type,
	const wxString &str,
	bool toStdout,
	bool toGUI)
{
	wxString msg(str);
	wxString category;
	const wxString level = DetermineLogLevelLabel(critical, msg);
	// handle Debug messages
	if (type != logStandard) {
		if (!critical && !IsEnabled(type)) {
			return;
		}
		if (!critical && thePrefs::GetVerboseLogfile()) {
			// print non critical debug messages only to the logfile
			toGUI = false;
		}
		int index = (int)type;

		if ( index >= 0 && index < categoryCount ) {
			const CDebugCategory& cat = g_debugcats[ index ];
			wxASSERT(type == cat.GetType());

			category = cat.GetName();
		} else {
			wxFAIL;
		}
	}

	const wxString origin = BuildOriginLabel(file, line);

	if (toGUI && !wxThread::IsMain()) {
		// put to background
		CLoggingEvent Event(critical, toStdout, toGUI, msg, level, category, origin);
		AddPendingEvent(Event);
	} else {
		// Try to handle events immediately when possible (to save to file).
		DoLines(msg, critical, toStdout, toGUI, level, category, origin);
	}
}


void CLogger::AddLogLine(
	const wxString &file,
	int line,
	bool critical,
	DebugType type,
	const std::ostringstream &msg)
{
	AddLogLine(file, line, critical, type, wxString(char2unicode(msg.str().c_str())));
}


const CDebugCategory& CLogger::GetDebugCategory( int index )
{
	wxASSERT( index >= 0 && index < categoryCount );

	return g_debugcats[ index ];
}


unsigned int CLogger::GetDebugCategoryCount()
{
	return categoryCount;
}


bool CLogger::OpenLogfile(const wxString & name)
{
	applog = new wxFFileOutputStream(name);
	bool ret = applog->Ok();
	if (ret) {
		FlushApplog();
		m_LogfileName = name;
	} else {
		CloseLogfile();
	}
	return ret;
}


void CLogger::CloseLogfile()
{
	delete applog;
	applog = nullptr;
	m_LogfileName.Clear();
}


void CLogger::OnLoggingEvent(class CLoggingEvent& evt)
{
	DoLines(evt.Message(), evt.IsCritical(), evt.ToStdout(), evt.ToGUI(), evt.Level(), evt.Category(), evt.Origin());
}


void CLogger::DoLines(const wxString & lines, bool critical, bool toStdout, bool toGUI, const wxString& level, const wxString& category, const wxString& origin)
{
	// Remove newspace at end
	wxString bufferline = lines.Strip(wxString::trailing);

	const wxDateTime now = wxDateTime::Now();
	const wxString baseStamp = now.FormatISODate() + wxT(" ") + now.FormatISOTime();
#ifdef CLIENT_GUI
	const wxString guiStamp = baseStamp + wxT(" (remote-GUI)");
	const wxString persistentStamp = baseStamp + wxT(" (remote-GUI)");
#else
	const wxString guiStamp = baseStamp;
	const wxString persistentStamp = baseStamp;
#endif

	// GUI keeps the critical marker, but the logfile should stay neutral.
	const wxString guiPrefix = critical ? wxT("!") : wxT(" ");
	const wxString persistentPrefix = !toGUI ? wxT(".") : wxT(" ");

	if ( bufferline.IsEmpty() ) {
		// If it's empty we just write a blank line with no timestamp.
		DoLine(wxT(" \n"), wxT(" \n"), toStdout, toGUI);
	} else {
		// Split multi-line messages into individual lines
		wxStringTokenizer tokens( bufferline, wxT("\n") );
		const wxString moduleLabel = DetermineModuleLabel(category);
		while ( tokens.HasMoreTokens() ) {
			const wxString token = tokens.GetNextToken();
			const wxString cleanToken = StripKnownLogPrefix(token);
			const wxString guiLine = guiPrefix
				+ WrapLogColumn(guiStamp)
				+ m_persistentSeparator
				+ WrapLogColumn(level)
				+ m_persistentSeparator
				+ WrapLogColumn(moduleLabel)
				+ m_persistentSeparator
				+ WrapLogColumn(origin)
				+ m_persistentSeparator
				+ WrapLogColumn(cleanToken)
				+ wxT("\n");
			const wxString persistentLine = persistentPrefix
				+ WrapLogColumn(persistentStamp)
				+ m_persistentSeparator
				+ WrapLogColumn(level)
				+ m_persistentSeparator
				+ WrapLogColumn(moduleLabel)
				+ m_persistentSeparator
				+ WrapLogColumn(origin)
				+ m_persistentSeparator
				+ WrapLogColumn(cleanToken)
				+ wxT("\n");
			DoLine(persistentLine, guiLine, toStdout, toGUI);
		}
	}
}


void CLogger::DoLine(const wxString & persistentLine, const wxString & guiLine, bool toStdout, bool GUI_ONLY(toGUI))
{
	{
		wxMutexLocker lock(m_lineLock);
		++m_count;

		// write to logfile
		m_ApplogBuf += persistentLine;
		FlushApplog();

		// write to Stdout
		if (m_StdoutLog || toStdout) {
			printf("%s", (const char*)unicode2char(persistentLine));
		}
	}
#ifndef AMULE_DAEMON
	// write to Listcontrol
	if (toGUI) {
		theApp->AddGuiLogLine(guiLine);
	}
#endif
}


void CLogger::EmergencyLog(const wxString &message, bool closeLog)
{
	fprintf(stderr, "%s", (const char*)unicode2char(message));
	m_ApplogBuf += message;
	FlushApplog();
	if (closeLog && applog) {
		applog->Close();
		applog = nullptr;
	}
}


void CLogger::FlushApplog()
{
	if (applog) { // Wait with output until logfile is actually opened
		wxStringInputStream stream(m_ApplogBuf);
		(*applog) << stream;
		applog->Sync();
		m_ApplogBuf.Clear();
	}
}

CLogger theLogger;

BEGIN_EVENT_TABLE(CLogger, wxEvtHandler)
	EVT_MULE_LOGGING(CLogger::OnLoggingEvent)
END_EVENT_TABLE()


CLoggerTarget::CLoggerTarget()
{
}

void CLoggerTarget::DoLogText(const wxString &msg)
{
	// prevent infinite recursion
	static bool recursion = false;
	if (recursion) {
		return;
	}
	recursion = true;

	// This is much simpler than manually handling all wx log-types.
	if (msg.StartsWith(_("ERROR: ")) || msg.StartsWith(_("WARNING: "))) {
		AddLogLineC(msg);
	} else {
		AddLogLineN(msg);
	}

	recursion = false;
}

CLoggerAccess::CLoggerAccess()
{
	m_bufferlen = 4096;
	m_buffer = new wxCharBuffer(m_bufferlen);
	m_logfile = nullptr;
	Reset();
}


void CLoggerAccess::Reset()
{
	delete m_logfile;
	m_logfile = new wxFFileInputStream(theLogger.GetLogfileName());
	m_pos = 0;
	m_ready = false;
}


CLoggerAccess::~CLoggerAccess()
{
	delete m_buffer;
	delete m_logfile;
}


//
// read a line of text from the logfile if available
// (can't believe there's no library function for this >:( )
//
bool CLoggerAccess::HasString()
{
	while (!m_ready) {
		int c = m_logfile->GetC();
		if (c == wxEOF) {
			break;
		}
		// check for buffer overrun
		if (m_pos == m_bufferlen) {
			m_bufferlen += 1024;
			m_buffer->extend(m_bufferlen);
		}
		m_buffer->data()[m_pos++] = c;
		if (c == '\n') {
			if (m_buffer->data()[0] == '.') {
				// Log-only line, skip
				m_pos = 0;
			} else {
				m_ready = true;
			}
		}
	}
	return m_ready;
}


bool CLoggerAccess::GetString(wxString & s)
{
	if (!HasString()) {
		return false;
	}
	s = wxString(m_buffer->data(), wxConvUTF8, m_pos);
	m_pos = 0;
	m_ready = false;
	return true;
}

// Functions for EC logging

#ifdef __DEBUG__
#include "ec/cpp/ECLog.h"

bool ECLogIsEnabled()
{
	return theLogger.IsEnabled(logEC);
}

void DoECLogLine(const wxString &line)
{
	// without file/line
	theLogger.AddLogLine(wxEmptyString, 0, false, logStandard, line, false, false);
}

#endif /* __DEBUG__ */
// File_checked_for_headers
