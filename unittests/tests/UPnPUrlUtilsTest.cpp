#include <muleunit/test.h>

#include <UPnPUrlUtils.h>

using namespace muleunit;

namespace {

bool ResolveRouterLocal(const std::string& host, uint32_t& outAddr)
{
	if (host == "router.local") {
		outAddr = 0xC0A80101u;
		return true;
	}
	return false;
}

struct ResolverGuard
{
	explicit ResolverGuard(ResolveHostnameForTestFunc resolver)
	{
		SetResolveHostnameForTest(resolver);
	}

	~ResolverGuard()
	{
		SetResolveHostnameForTest(nullptr);
	}
};

} // namespace

DECLARE_SIMPLE(UPnPUrlUtilsTest);

TEST(UPnPUrlUtilsTest, AcceptsLanHttpAndHttps)
{
	ASSERT_TRUE(IsSafeLanUrl("http://192.168.1.1/status"));
	ASSERT_TRUE(IsSafeLanUrl("https://10.0.0.2"));
	ASSERT_TRUE(IsSafeLanUrl("http://127.0.0.1"));
}

TEST(UPnPUrlUtilsTest, AcceptsLanHostname)
{
	ASSERT_TRUE(IsSafeLanUrl("http://localhost:5000/rootDesc.xml"));
}

TEST(UPnPUrlUtilsTest, AcceptsLanHostnameResolvedViaSeam)
{
	ResolverGuard guard(ResolveRouterLocal);
	ASSERT_TRUE(IsSafeLanUrl("http://router.local:5000/rootDesc.xml"));
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

TEST(UPnPUrlUtilsTest, RejectsUnresolvableHostname)
{
	ASSERT_FALSE(IsSafeLanUrl("http://nonexistent.invalid:4900/rootDesc.xml"));
}

TEST(UPnPUrlUtilsTest, RejectsInvalidOrUnsupportedHosts)
{
	ASSERT_FALSE(IsSafeLanUrl("http://[::1]"));
	ASSERT_FALSE(IsSafeLanUrl("http:///missing-host"));
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

TEST(UPnPUrlUtilsTest, AssignLanUrlOrClearProtectsControlUrls)
{
	std::string destination = "persist";
	std::string warn;
	ASSERT_FALSE(AssignLanUrlOrClear("controlURL", "http://203.0.113.7/scpd.xml", destination, &warn));
	ASSERT_TRUE(destination.empty());
	ASSERT_TRUE(warn.find("Rejecting controlURL") != std::string::npos);
	ASSERT_TRUE(AssignLanUrlOrClear("controlURL", "http://192.168.0.5/service", destination, nullptr));
	ASSERT_EQUALS(std::string("http://192.168.0.5/service"), destination);
}
