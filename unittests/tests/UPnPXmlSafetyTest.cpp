#ifdef ENABLE_UPNP

#include <muleunit/test.h>

#include <UPnPBase.h>
#include <UPnPUrlUtils.h>

#include <cstdlib>

using namespace muleunit;

namespace {

struct MappingResult {
	int code;
	bool success() const { return code == 0; }
};

class FakeUPnPClient : public IUPnPClient
{
public:
	FakeUPnPClient(bool discoveryOk, bool safeUrls, bool igdValid = true)
	:
	m_discoveryOk(discoveryOk),
	m_safeUrls(safeUrls),
	m_igdValid(igdValid),
	m_discovered(false),
	m_deleteCode(0),
	m_mappingIndex(0),
	m_failureStage(UPNP_STAGE_NONE),
	m_lastMappedExternalPort(0),
	m_discoveredCount(0),
	m_filteredCount(0),
	m_supportsAddAny(false),
	m_fallbackAddAnyCode(0),
	m_fallbackExternalPort(0),
	m_fallbackCalled(false)
	{
	}

	std::vector<MappingResult> mappingResults;
	int m_deleteCode;

	void EnableFallback(int fallbackCode, uint16 fallbackExternalPort)
	{
		m_supportsAddAny = true;
		m_fallbackAddAnyCode = fallbackCode;
		m_fallbackExternalPort = fallbackExternalPort;
		m_fallbackCalled = false;
	}

	bool FallbackCalled() const
	{
		return m_fallbackCalled;
	}

	bool Discover(CUPnPDiscoveryInfo& out, wxString& error) override
	{
		m_failureStage = UPNP_STAGE_NONE;
		m_discoveredCount = 0;
		m_filteredCount = 0;
		if (!m_discoveryOk) {
			error = wxT("No IGD disponible");
			m_failureStage = UPNP_STAGE_DISCOVERY_EMPTY;
			return false;
		}
		if (!m_safeUrls) {
			error = wxT("Rejecting control URL outside LAN");
			m_failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
			m_discoveredCount = 1;
			m_filteredCount = 1;
			return false;
		}
		if (!m_igdValid) {
			error = wxT("IGD inválido");
			m_failureStage = UPNP_STAGE_IGD_INVALID;
			m_discoveredCount = 1;
			m_filteredCount = 0;
			return false;
		}
		m_discoveredCount = 1;
		out.routerId = wxT("router-1");
		out.routerName = wxT("Test Router");
		out.adapterId = wxT("192.168.1.100");
		out.descUrl = "http://192.168.1.1/desc.xml";
		out.controlUrl = "http://192.168.1.1/control";
		out.serviceType = "urn:schemas-upnp-org:service:WANIPConnection:1";
		out.discoveredDevices = m_discoveredCount;
		out.filteredDevices = m_filteredCount;
		out.failureStage = UPNP_STAGE_NONE;
		m_discovered = true;
		return true;
	}

	bool AddMapping(const CUPnPPortMapping& mapping, wxString& error) override
	{
		m_failureStage = UPNP_STAGE_NONE;
		m_lastMappedExternalPort = 0;
		if (!m_discovered) {
			error = wxT("Discovery no completado");
			m_failureStage = UPNP_STAGE_DISCOVERY_EMPTY;
			return false;
		}
		if (m_mappingIndex >= mappingResults.size()) {
			error = wxT("No hay resultado simulado");
			m_failureStage = UPNP_STAGE_MAPPING_PRIMARY_FAILED;
			return false;
		}
		const MappingResult res = mappingResults[m_mappingIndex++];
		if (!res.success()) {
			error = wxString::Format(wxT("miniupnpc error %d para puerto %s/%s"), res.code,
			wxString::FromUTF8(mapping.getPort().c_str()), wxString::FromUTF8(mapping.getProtocol().c_str()));
			if ((res.code == 718 || res.code == 713) && m_supportsAddAny) {
				m_fallbackCalled = true;
				if (m_fallbackAddAnyCode == 0) {
					m_failureStage = UPNP_STAGE_NONE;
					const unsigned long parsedPort = std::strtoul(mapping.getPort().c_str(), nullptr, 10);
					if (m_fallbackExternalPort != 0) {
						m_lastMappedExternalPort = m_fallbackExternalPort;
					} else {
						m_lastMappedExternalPort = static_cast<uint16>(parsedPort);
					}
					return true;
				}
				error = wxString::Format(wxT("miniupnpc fallback error %d para puerto %s/%s"), m_fallbackAddAnyCode,
					wxString::FromUTF8(mapping.getPort().c_str()), wxString::FromUTF8(mapping.getProtocol().c_str()));
				m_failureStage = UPNP_STAGE_MAPPING_FALLBACK_FAILED;
				return false;
			}
			m_failureStage = UPNP_STAGE_MAPPING_PRIMARY_FAILED;
			return false;
		}
		m_lastMappedExternalPort = static_cast<uint16>(std::strtoul(mapping.getPort().c_str(), nullptr, 10));
		return true;
	}

	bool DeleteMapping(const CUPnPPortMapping& mapping, wxString& error) override
	{
		if (!m_discovered) {
			error = wxT("Discovery no completado");
			return false;
		}
		if (m_deleteCode != 0) {
			error = wxString::Format(wxT("miniupnpc error %d para borrar %s/%s"), m_deleteCode,
			wxString::FromUTF8(mapping.getPort().c_str()), wxString::FromUTF8(mapping.getProtocol().c_str()));
			return false;
		}
		return true;
	}

	bool GetExternalIp(wxString& outIp, wxString& error) override
	{
		(void)outIp;
		error = wxT("No implementado en fake");
		return false;
	}

	CUPnPFailureStage GetFailureStage() const override
	{
		return m_failureStage;
	}

	uint16 GetLastMappedExternalPort() const override
	{
		return m_lastMappedExternalPort;
	}

	uint32 GetDiscoveredDeviceCount() const override
	{
		return m_discoveredCount;
	}

	uint32 GetFilteredDeviceCount() const override
	{
		return m_filteredCount;
	}

private:
	bool m_discoveryOk;
	bool m_safeUrls;
	bool m_igdValid;
	bool m_discovered;
	int m_mappingIndex;
	CUPnPFailureStage m_failureStage;
	uint16 m_lastMappedExternalPort;
	uint32 m_discoveredCount;
	uint32 m_filteredCount;
	bool m_supportsAddAny;
	int m_fallbackAddAnyCode;
	uint16 m_fallbackExternalPort;
	bool m_fallbackCalled;
};

} // namespace

DECLARE_SIMPLE(UPnPXmlSafetyTest);

TEST(UPnPXmlSafetyTest, MapsSuccessfully)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	fake->mappingResults.push_back({0});
	fake->mappingResults.push_back({0});

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(4662, "TCP", true, "tcp"));
	mappings.push_back(CUPnPPortMapping(4672, "UDP", true, "udp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_OK, report.status);
	ASSERT_EQUALS(UPNP_STAGE_NONE, report.failureStage);
	ASSERT_EQUALS(static_cast<size_t>(2), report.mappedMappings.size());
	ASSERT_TRUE(report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, RejectsNonLanControlUrl)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, false));
	CUPnPControlPoint controlPoint(0, std::move(fake));

	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(80, "TCP", true, "http"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_DISCOVERY_FILTERED, report.failureStage);
	ASSERT_TRUE(report.lastError.Find(wxT("LAN")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, MarksInvalidIgd)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true, false));
	CUPnPControlPoint controlPoint(0, std::move(fake));

	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(4123, "TCP", true, "tcp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_IGD_INVALID, report.failureStage);
	ASSERT_TRUE(report.lastError.Find(wxT("IGD")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, FailsWhenNoRouterFound)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(false, true));
	CUPnPControlPoint controlPoint(0, std::move(fake));

	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(1234, "TCP", true, "tcp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_DISCOVERY_EMPTY, report.failureStage);
	ASSERT_EQUALS(static_cast<size_t>(0), report.mappedMappings.size());
	ASSERT_TRUE(report.lastError.Find(wxT("IGD")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, ReportsConflictOnSecondMapping)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	fake->mappingResults.push_back({0});
	fake->mappingResults.push_back({718});

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(4711, "TCP", true, "tcp"));
	mappings.push_back(CUPnPPortMapping(4711, "UDP", true, "udp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_MAPPING_PRIMARY_FAILED, report.failureStage);
	ASSERT_EQUALS(static_cast<size_t>(1), report.mappedMappings.size());
	ASSERT_TRUE(report.lastError.Find(wxT("718")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, AppliesFallbackWhenPortConflicts)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	FakeUPnPClient* fakeRaw = fake.get();
	fake->mappingResults.push_back({718});
	fake->EnableFallback(0, 62000);

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(4712, "TCP", true, "tcp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_OK, report.status);
	ASSERT_EQUALS(UPNP_STAGE_NONE, report.failureStage);
	ASSERT_EQUALS(static_cast<size_t>(1), report.mappedMappings.size());
	ASSERT_EQUALS(static_cast<uint16>(62000), report.mappedMappings.front().externalPort);
	ASSERT_TRUE(fakeRaw->FallbackCalled());
}


TEST(UPnPXmlSafetyTest, ReportsFallbackFailure)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	fake->mappingResults.push_back({718});
	fake->EnableFallback(501, 0);

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(5000, "UDP", true, "udp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_MAPPING_FALLBACK_FAILED, report.failureStage);
	ASSERT_TRUE(report.lastError.Find(wxT("501")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, ReportsNoSuchEntryOnAddMapping)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	fake->mappingResults.push_back({714});

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(9090, "TCP", true, "tcp"));

	CUPnPLastResult last = {};
	CUPnPOperationReport report = controlPoint.ExecuteMappings(mappings, wxT("core"), last, false);
	ASSERT_EQUALS(UPNP_LAST_FAIL, report.status);
	ASSERT_EQUALS(UPNP_STAGE_MAPPING_PRIMARY_FAILED, report.failureStage);
	ASSERT_EQUALS(static_cast<size_t>(0), report.mappedMappings.size());
	ASSERT_TRUE(report.lastError.Find(wxT("714")) != wxNOT_FOUND || !report.lastError.IsEmpty());
}


TEST(UPnPXmlSafetyTest, DeleteHandlesMissingEntry)
{
	std::unique_ptr<FakeUPnPClient> fake(new FakeUPnPClient(true, true));
	fake->mappingResults.push_back({0});
	fake->m_deleteCode = 714;

	CUPnPControlPoint controlPoint(0, std::move(fake));
	std::vector<CUPnPPortMapping> mappings;
	mappings.push_back(CUPnPPortMapping(8080, "TCP", true, "web"));

	bool deleted = controlPoint.DeletePortMappings(mappings);
	ASSERT_FALSE(deleted);
}

#endif // ENABLE_UPNP
