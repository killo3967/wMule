#include <muleunit/test.h>

#include <common/Path.h>

using namespace muleunit;

DECLARE_SIMPLE(PathTraversalTest);

TEST(PathTraversalTest, StripsDirectoryComponents)
{
	CPath sanitized = SanitizeFileName(wxT("../dir/..\\nested/file.txt"));
	ASSERT_EQUALS(wxT("file.txt"), sanitized.GetPrintable());
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
