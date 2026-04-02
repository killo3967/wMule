#include <muleunit/test.h>

#include <common/Path.h>
#include <wx/filename.h>
#include <wx/ffile.h>
#include <wx/filefn.h>

#include "InternalPathTestHelpers.h"

using namespace muleunit;

DECLARE_SIMPLE(InternalPathGuardsTest);

namespace {

wxString UniqueTempSubdir(const wxString& prefix)
{
	static int counter = 0;
	return JoinPaths(wxFileName::GetTempDir(),
		wxString::Format(wxT("wmule-%s-%d"), prefix.c_str(), ++counter));
}

void EnsureCleanDir(const wxString& path)
{
	if (wxFileName::DirExists(path)) {
		wxFileName::Rmdir(path, wxPATH_RMDIR_RECURSIVE);
	}
	if (!wxFileName::DirExists(path)) {
		CPath::MakeDir(CPath(path));
	}
}

}

static CInternalPathContext BuildContext()
{
	wxString config = UniqueTempSubdir(wxT("path-guards-config"));
	wxString incoming = JoinPaths(config, wxT("Incoming"));
	wxString temp = JoinPaths(config, wxT("Temp"));
	EnsureCleanDir(config);
	EnsureCleanDir(incoming);
	EnsureCleanDir(temp);

	CInternalPathContext context = {config, temp, incoming, true, false, false};
	return context;
}

static CInternalPathResult ValidatePreferenceWithFallback(EInternalPathKind kind,
	const wxString& candidate, CInternalPathContext& context);

TEST(InternalPathGuardsTest, AcceptsSubdirectoryInsideConfig)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::TempDir, context.m_tempDir, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(InternalPathGuardsTest, AcceptsExternalTempOutsideBaseAndMarksFlag)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::TempDir,
		JoinPaths(wxFileName::GetTempDir(), wxT("escape-temp")), context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(InternalPathGuardsTest, ExternalTempDoesNotRequireUnsafeGate)
{
	CInternalPathContext context = BuildContext();
	context.m_allowUnsafePreference = true;
	context.m_allowUnsafeOutsideBase = false;
	const wxString external = JoinPaths(wxFileName::GetTempDir(), wxT("escape-temp"));

	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::TempDir, external, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

#ifdef __WINDOWS__
TEST(InternalPathGuardsTest, AcceptsExternalUNCIncomingPath)
{
	CInternalPathContext context = BuildContext();
	const wxString uncLocalBase = UniqueTempSubdir(wxT("unc-incoming"));
	EnsureCleanDir(uncLocalBase);
	wxString extended = wxT("\\\\?\\") + uncLocalBase;
	extended.Replace(wxT("/"), wxT("\\"));

	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, extended, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}
#endif

TEST(InternalPathGuardsTest, RejectsTraversal)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir,
		JoinPaths(context.m_configDir, wxT("..\\escape")), context);
	ASSERT_FALSE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::TraversalRejected, result.m_error);
}

TEST(InternalPathGuardsTest, AcceptsExternalOSDirOutsideIncomingAndMarksFlag)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::OSDir,
		JoinPaths(context.m_configDir, wxT("os-outside")), context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(InternalPathGuardsTest, PreferenceSeamAcceptsExternalTemp)
{
	CInternalPathContext context = BuildContext();
	const wxString externalTemp = JoinPaths(wxFileName::GetTempDir(), wxT("escape"));
	CInternalPathResult result = NormalizePreferencePathForLoad(wxT("/eMule/TempDir"),
		externalTemp, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(InternalPathGuardsTest, PreferencesRetainValidExternalPath)
{
	CInternalPathContext context = BuildContext();
	const wxString externalIncoming = UniqueTempSubdir(wxT("prefs-external"));
	EnsureCleanDir(externalIncoming);
	CInternalPathResult expected = NormalizeInternalDir(EInternalPathKind::IncomingDir,
		externalIncoming, context);
	ASSERT_TRUE(expected.m_ok);

	CInternalPathResult result = ValidatePreferenceWithFallback(EInternalPathKind::IncomingDir,
		externalIncoming, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(expected.m_normalizedPath, result.m_normalizedPath);
}

TEST(InternalPathGuardsTest, ErrorToStringIsStable)
{
	ASSERT_EQUALS(wxT("path outside allowed base"), InternalPathErrorToString(EInternalPathError::OutsideBase));
}

TEST(InternalPathGuardsTest, AcceptsOSDirAtIncomingBase)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::OSDir,
		context.m_incomingDir, context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(InternalPathGuardsTest, OSDirFallbacksWhenExternalPathIsAFile)
{
	CInternalPathContext context = BuildContext();
	const wxString externalBase = UniqueTempSubdir(wxT("osdir-fallback"));
	EnsureCleanDir(externalBase);
	const wxString osDirAsFile = JoinPaths(externalBase, wxT("amule"));
	wxFFile output(osDirAsFile, wxT("w"));
	output.Close();

	COSDirValidationOutcome osDirOutcome = NormalizeOSDirWithFallback(osDirAsFile, context);
	ASSERT_TRUE(osDirOutcome.m_result.m_ok);
	ASSERT_TRUE(osDirOutcome.m_usedFallback);
	ASSERT_EQUALS(EInternalPathError::FileInsteadOfDir, osDirOutcome.m_rejectedError);
	ASSERT_EQUALS(GetDefaultInternalPath(EInternalPathKind::OSDir, context), osDirOutcome.m_result.m_normalizedPath);
	wxRemoveFile(osDirAsFile);
}

TEST(InternalPathGuardsTest, FailsWhenBaseIsInvalid)
{
	CInternalPathContext context = BuildContext();
	context.m_configDir = wxEmptyString;
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::TempDir, context.m_tempDir, context);
	ASSERT_FALSE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::InvalidBase, result.m_error);
}

static CInternalPathResult ValidatePreferenceWithFallback(EInternalPathKind kind,
	const wxString& candidate, CInternalPathContext& context)
{
	CInternalPathResult result = NormalizePreferencePathForLoad(
		kind == EInternalPathKind::TempDir ? wxT("/eMule/TempDir") : wxT("/eMule/IncomingDir"),
		candidate,
		context);
	if (result.m_ok) {
		return result;
	}

	const wxString fallback = GetDefaultInternalPath(kind, context);
	return NormalizeInternalDir(kind, fallback, context);
}

TEST(InternalPathGuardsTest, PreferencesFallbackWhenConfigValueIsCorrupt)
{
	CInternalPathContext context = BuildContext();
	CInternalPathResult result = ValidatePreferenceWithFallback(EInternalPathKind::IncomingDir,
		wxT("relative\\incoming"), context);
	ASSERT_TRUE(result.m_ok);
	ASSERT_EQUALS(GetDefaultInternalPath(EInternalPathKind::IncomingDir, context), result.m_normalizedPath);
}

TEST(InternalPathGuardsTest, PreferencesFallbackWhenPathIsInaccessible)
{
	CInternalPathContext context = BuildContext();
	const wxString blockedPath = JoinPaths(context.m_configDir, wxT("TempAsFile"));
	wxFFile output(blockedPath, wxT("w"));
	output.Close();

	CInternalPathResult result = ValidatePreferenceWithFallback(EInternalPathKind::TempDir, blockedPath, context);
	wxRemoveFile(blockedPath);
	ASSERT_TRUE(result.m_ok);
	ASSERT_EQUALS(GetDefaultInternalPath(EInternalPathKind::TempDir, context), result.m_normalizedPath);
}
