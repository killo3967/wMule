#include <muleunit/test.h>

#include <UPnPUrlUtils.h>

using namespace muleunit;

DECLARE_SIMPLE(UPnPUrlUtilsTest);

TEST(UPnPUrlUtilsTest, AcceptsLanHttpAndHttps)
{
	ASSERT_TRUE(IsSafeLanUrl("http://192.168.1.1/status"));
	ASSERT_TRUE(IsSafeLanUrl("https://10.0.0.2"));
	ASSERT_TRUE(IsSafeLanUrl("http://127.0.0.1"));
}

TEST(UPnPUrlUtilsTest, RejectsUnsupportedSchemes)
{
	ASSERT_FALSE(IsSafeLanUrl("ftp://192.168.1.1"));
	ASSERT_FALSE(IsSafeLanUrl("ws://10.0.0.1"));
}

TEST(UPnPUrlUtilsTest, RejectsPublicHosts)
{
	ASSERT_FALSE(IsSafeLanUrl("http://8.8.8.8"));
	ASSERT_FALSE(IsSafeLanUrl("https://example.com"));
}

TEST(UPnPUrlUtilsTest, ParseUrlExtractsComponents)
{
	std::string scheme;
	std::string host;
	uint16_t port = 0;
	ASSERT_TRUE(ParseUrl("http://192.168.0.5:1234/upnp", scheme, host, port));
	ASSERT_EQUALS(std::string("http"), scheme);
	ASSERT_EQUALS(std::string("192.168.0.5"), host);
	ASSERT_EQUALS(static_cast<uint16_t>(1234), port);
}
