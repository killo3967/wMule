#ifdef ENABLE_UPNP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <upnp/ixml.h>

#include <muleunit/test.h>

#include <UPnPEventLog.h>
#include <UPnPUrlUtils.h>

using namespace muleunit;

DECLARE_SIMPLE(UPnPXmlSafetyTest);

TEST(UPnPXmlSafetyTest, FormatsNullDocument)
{
	const std::string log = FormatUPnPEventLog("uuid:test", 7, nullptr);
	ASSERT_TRUE(log.find("<null document>") != std::string::npos);
}

TEST(UPnPXmlSafetyTest, FormatsPropertyList)
{
	const char* xml = "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
		"<e:property><ConnectionStatus>Connected</ConnectionStatus></e:property>"
		"</e:propertyset>";
	IXML_Document* doc = ixmlParseBuffer(const_cast<char*>(xml));
	ASSERT_TRUE(doc != nullptr);
	const std::string log = FormatUPnPEventLog("uuid:test", 11, doc);
	ASSERT_TRUE(log.find("ConnectionStatus='Connected'") != std::string::npos);
	ixmlDocument_free(doc);
}

TEST(UPnPXmlSafetyTest, AssignLanUrlOrClear)
{
	std::string destination = "persist";
	std::string warn;
	ASSERT_FALSE(AssignLanUrlOrClear("SCPDURL", "http://203.0.113.7/scpd.xml", destination, &warn));
	ASSERT_TRUE(destination.empty());
	ASSERT_TRUE(warn.find("Rejecting SCPDURL") != std::string::npos);
	ASSERT_TRUE(AssignLanUrlOrClear("SCPDURL", "http://192.168.0.5/service", destination, nullptr));
	ASSERT_EQUALS(std::string("http://192.168.0.5/service"), destination);
}

#endif // ENABLE_UPNP
