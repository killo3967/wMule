#include <muleunit/test.h>

#include <common/Path.h>

#include "InternalPathTestHelpers.h"
#include <wx/filename.h>

using namespace muleunit;

DECLARE_SIMPLE(PathTraversalTest);

TEST(PathTraversalTest, StripsDirectoryComponents)
{
	CPath sanitized = SanitizeFileName(wxT("../dir/..\\nested/file.txt"));
	ASSERT_EQUALS(wxT("file.txt"), sanitized.GetPrintable());
}

TEST(PathTraversalTest, NormalizeRenameTargetRejectsTraversal)
{
	CPath sanitized;
	ASSERT_FALSE(NormalizeRenameTarget(wxT("..\\escape.avi"), sanitized));
}

#ifdef __WINDOWS__
TEST(PathTraversalTest, NormalizeRenameTargetRejectsAbsolutePath)
{
	CPath sanitized;
	ASSERT_FALSE(NormalizeRenameTarget(wxT("C:\\temp\\file.avi"), sanitized));
}
#else
TEST(PathTraversalTest, NormalizeRenameTargetRejectsAbsolutePath)
{
	CPath sanitized;
	ASSERT_FALSE(NormalizeRenameTarget(wxT("/tmp/file.avi"), sanitized));
}
#endif

TEST(PathTraversalTest, NormalizeAbsoluteDirNoTraversalRejectsRelativeConfigDir)
{
	wxString normalized;
	ASSERT_FALSE(NormalizeAbsoluteDirNoTraversal(wxT("relative\\config"), normalized, true));
}

#ifdef __WINDOWS__
TEST(PathTraversalTest, NormalizeAbsoluteFilePathNoTraversalRejectsTraversal)
{
	wxString normalized;
	ASSERT_FALSE(NormalizeAbsoluteFilePathNoTraversal(wxT("C:\\temp\\..\\escape.collection"), normalized));
}
#else
TEST(PathTraversalTest, NormalizeAbsoluteFilePathNoTraversalRejectsTraversal)
{
	wxString normalized;
	ASSERT_FALSE(NormalizeAbsoluteFilePathNoTraversal(wxT("/tmp/../escape.collection"), normalized));
}
#endif

TEST(PathTraversalTest, NormalizeAbsoluteFilePathNoTraversalAcceptsAbsolute)
{
	wxString normalized;
	wxString absolute = wxFileName(wxFileName::GetTempDir(), wxT("safe.collection")).GetFullPath();
	ASSERT_TRUE(NormalizeAbsoluteFilePathNoTraversal(absolute, normalized));
	ASSERT_EQUALS(absolute, normalized);
}

TEST(PathTraversalTest, NormalizeRenameTargetRejectsSeparators)
{
	CPath sanitized;
	ASSERT_FALSE(NormalizeRenameTarget(wxT("dir/file.avi"), sanitized));
}

TEST(PathTraversalTest, NormalizeRenameTargetAcceptsSimpleName)
{
	CPath sanitized;
	ASSERT_TRUE(NormalizeRenameTarget(wxT("safe-name.mp3"), sanitized));
	ASSERT_EQUALS(wxT("safe-name.mp3"), sanitized.GetPrintable());
}

TEST(PathTraversalTest, RemovesControlCharacters)
{
	wxString raw;
	raw << wxT("bad") << wxChar(1) << wxT("name.mp3");
	CPath sanitized = SanitizeFileName(raw);
	ASSERT_EQUALS(wxT("badname.mp3"), sanitized.GetPrintable());
}

TEST(PathTraversalTest, ProvidesFallbackWhenEmpty)
{
	CPath sanitized = SanitizeFileName(wxString());
	ASSERT_EQUALS(wxT("unnamed_file"), sanitized.GetPrintable());
}

TEST(PathTraversalTest, NormalizeSharedPathRejectsRelativeInput)
{
	wxString normalized;
	ASSERT_FALSE(NormalizeSharedPath(wxT("relative/path"), normalized));
}

TEST(PathTraversalTest, NormalizeSharedPathCollapsesDots)
{
	wxString normalized;
#ifdef __WINDOWS__
	ASSERT_TRUE(NormalizeSharedPath(wxT("C:\\incoming\\..\\safe\\file.dat"), normalized));
	ASSERT_EQUALS(wxT("C:\\safe\\file.dat"), normalized);
#else
	ASSERT_TRUE(NormalizeSharedPath(wxT("/var/incoming/../safe/file.dat"), normalized));
	ASSERT_EQUALS(wxT("/var/safe/file.dat"), normalized);
#endif
}

TEST(PathTraversalTest, NormalizeSharedPathUsesBaseDirectory)
{
	wxString normalized;
#ifdef __WINDOWS__
	ASSERT_TRUE(NormalizeSharedPath(wxT("..\\subdir"), normalized, wxT("D:\\wmule")));
	ASSERT_EQUALS(wxT("D:\\subdir"), normalized);
#else
	ASSERT_TRUE(NormalizeSharedPath(wxT("../subdir"), normalized, wxT("/opt/wmule")));
	ASSERT_EQUALS(wxT("/opt/subdir"), normalized);
#endif
}

#ifdef __WINDOWS__
TEST(PathTraversalTest, NormalizeSharedPathSupportsUNC)
{
	wxString normalized;
	ASSERT_TRUE(NormalizeSharedPath(wxT("\\\\server\\share\\folder"), normalized));
	ASSERT_EQUALS(wxT("\\\\server\\share\\folder"), normalized);
}
#endif

TEST(PathTraversalTest, NormalizeCategoryPathRejectsRelativeInput)
{
	wxString normalized;
	ASSERT_FALSE(NormalizeCategoryPath(wxT("relative/path"), normalized));
}

#ifdef __WINDOWS__
TEST(PathTraversalTest, NormalizeCategoryPathAcceptsUNC)
{
	wxString normalized;
	ASSERT_TRUE(NormalizeCategoryPath(wxT("\\\\server\\cat"), normalized));
	ASSERT_EQUALS(wxT("\\\\server\\cat"), normalized);
}
#else
TEST(PathTraversalTest, NormalizeCategoryPathAcceptsAbsoluteUnixPath)
{
	wxString normalized;
	ASSERT_TRUE(NormalizeCategoryPath(wxT("/opt/cat"), normalized));
	ASSERT_EQUALS(wxT("/opt/cat"), normalized);
}
#endif

TEST(PathTraversalTest, IsSubPathDetectsChild)
{
#ifdef __WINDOWS__
	ASSERT_TRUE(IsSubPathOf(CPath(wxT("C:\\incoming")), CPath(wxT("C:\\incoming\\child"))));
	ASSERT_TRUE(IsSubPathOf(CPath(wxT("C:\\incoming")), CPath(wxT("C:\\incoming"))));
#else
	ASSERT_TRUE(IsSubPathOf(CPath(wxT("/var/incoming")), CPath(wxT("/var/incoming/sub"))));
	ASSERT_TRUE(IsSubPathOf(CPath(wxT("/var/incoming")), CPath(wxT("/var/incoming"))));
#endif
}

TEST(PathTraversalTest, IsSubPathRejectsUnrelatedPaths)
{
#ifdef __WINDOWS__
	ASSERT_FALSE(IsSubPathOf(CPath(wxT("C:\\incoming")), CPath(wxT("D:\\incoming\\child"))));
	ASSERT_FALSE(IsSubPathOf(CPath(wxT("C:\\incoming")), CPath(wxT("C:\\temp"))));
#else
	ASSERT_FALSE(IsSubPathOf(CPath(wxT("/var/incoming")), CPath(wxT("/tmp/incoming"))));
	ASSERT_FALSE(IsSubPathOf(CPath(wxT("/var/incoming")), CPath(wxT("/var/tmp"))));
#endif
}

static CInternalPathContext BuildInternalTestContext()
{
	CInternalPathContext context;
	context.m_configDir = JoinPaths(wxFileName::GetTempDir(), wxT("wmule-path-guards"));
	context.m_tempDir = JoinPaths(context.m_configDir, wxT("Temp"));
	context.m_incomingDir = JoinPaths(context.m_configDir, wxT("Incoming"));
	context.m_createIfMissing = true;
	context.m_allowUnsafePreference = false;
	context.m_allowUnsafeOutsideBase = false;
	CPath::MakeDir(CPath(context.m_tempDir));
	CPath::MakeDir(CPath(context.m_incomingDir));
	return context;
}

TEST(PathTraversalTest, NormalizeInternalDirRejectsRelative)
{
	CInternalPathContext context = BuildInternalTestContext();
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::TempDir, wxT("relative\\temp"), context);
	ASSERT_FALSE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::NotAbsolute, result.m_error);
}

TEST(PathTraversalTest, NormalizeInternalDirAcceptsExternalOutsideBaseAndMarksFlag)
{
	CInternalPathContext context = BuildInternalTestContext();
#ifdef __WINDOWS__
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, wxT("D:\\escape"), context);
#else
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, wxT("/opt/escape"), context);
#endif
	ASSERT_TRUE(result.m_ok);
	ASSERT_TRUE(result.m_isExternalToBase);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
}

TEST(PathTraversalTest, NormalizeInternalDirAcceptsExternalRegardlessOfUnsafeFlag)
{
	CInternalPathContext context = BuildInternalTestContext();
	context.m_allowUnsafePreference = true;
	context.m_allowUnsafeOutsideBase = true;
#ifdef __WINDOWS__
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, wxT("D:\\escape"), context);
#else
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, wxT("/opt/escape"), context);
#endif
	ASSERT_TRUE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::None, result.m_error);
	ASSERT_TRUE(result.m_isExternalToBase);
}

TEST(PathTraversalTest, NormalizeInternalDirStillRejectsTraversalInUnsafeMode)
{
	CInternalPathContext context = BuildInternalTestContext();
	context.m_allowUnsafePreference = true;
	context.m_allowUnsafeOutsideBase = true;
	wxString traversal = JoinPaths(context.m_configDir, wxT(".."));
	traversal = JoinPaths(traversal, wxT("escape"));
	CInternalPathResult result = NormalizeInternalDir(EInternalPathKind::IncomingDir, traversal, context);
	ASSERT_FALSE(result.m_ok);
	ASSERT_EQUALS(EInternalPathError::TraversalRejected, result.m_error);
}
