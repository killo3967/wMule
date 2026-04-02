#ifdef ENABLE_UPNP

#include <muleunit/test.h>

#include <UPnPBase.h>

#include <wx/utils.h>
#include <wx/fileconf.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/file.h>

using namespace muleunit;

namespace {

CUPnPPersistedMapping BuildMapping(uint16 port)
{
	CUPnPPersistedMapping mapping;
	mapping.service = wxT("core");
	mapping.protocol = wxT("TCP");
	mapping.internalPort = port;
	mapping.externalPort = port;
	mapping.enabled = true;
	return mapping;
}

} // namespace


DECLARE_SIMPLE(UPnPFeedbackPolicyTest);


TEST(UPnPFeedbackPolicyTest, SuppressesRecentFailures)
{
	std::vector<CUPnPPersistedMapping> requested;
	requested.push_back(BuildMapping(4662));

	CUPnPLastResult last;
	last.scope = wxT("core");
	last.status = UPNP_LAST_FAIL;
	last.timestampUtc = wxGetUTCTimeMillis().GetValue() - (5 * 60 * 1000);
	last.routerId = wxT("router-1");
	last.requested = requested;
	last.mapped.clear();
	last.retryCount = 1;
	last.failureStage = UPNP_STAGE_NONE;
	last.suppressedUntilSessionEnd = false;

	bool suppressedUntilSessionEnd = false;
	wxString reason;
	ASSERT_TRUE(ShouldSuppressUPnPRetry(last, requested, wxGetUTCTimeMillis().GetValue(), false, suppressedUntilSessionEnd, reason));
	ASSERT_FALSE(suppressedUntilSessionEnd);
	ASSERT_TRUE(!reason.IsEmpty());
}


TEST(UPnPFeedbackPolicyTest, DoesNotSuppressWithoutRouterId)
{
	std::vector<CUPnPPersistedMapping> requested;
	requested.push_back(BuildMapping(4662));

	CUPnPLastResult last;
	last.scope = wxT("core");
	last.status = UPNP_LAST_FAIL;
	last.timestampUtc = wxGetUTCTimeMillis().GetValue();
	last.requested = requested;
	last.mapped.clear();
	last.retryCount = 0;
	last.failureStage = UPNP_STAGE_NONE;
	last.suppressedUntilSessionEnd = false;

	bool suppressedUntilSessionEnd = false;
	wxString reason;
	ASSERT_FALSE(ShouldSuppressUPnPRetry(last, requested, wxGetUTCTimeMillis().GetValue(), false, suppressedUntilSessionEnd, reason));
	ASSERT_TRUE(reason.IsEmpty());
}


TEST(UPnPFeedbackPolicyTest, DoesNotSuppressForDifferentMappings)
{
	std::vector<CUPnPPersistedMapping> requested;
	requested.push_back(BuildMapping(4662));

	CUPnPLastResult last;
	last.scope = wxT("core");
	last.status = UPNP_LAST_FAIL;
	last.timestampUtc = wxGetUTCTimeMillis().GetValue();
	last.routerId = wxT("router-1");
	last.requested = requested;
	last.mapped.clear();
	last.retryCount = 2;
	last.failureStage = UPNP_STAGE_NONE;
	last.suppressedUntilSessionEnd = false;

	std::vector<CUPnPPersistedMapping> differentRequested;
	differentRequested.push_back(BuildMapping(4712));

	bool suppressedUntilSessionEnd = false;
	wxString reason;
	ASSERT_FALSE(ShouldSuppressUPnPRetry(last, differentRequested, wxGetUTCTimeMillis().GetValue(), false, suppressedUntilSessionEnd, reason));
}


TEST(UPnPFeedbackPolicyTest, AllowsForcedRetry)
{
	std::vector<CUPnPPersistedMapping> requested;
	requested.push_back(BuildMapping(4712));

	CUPnPLastResult last;
	last.scope = wxT("core");
	last.status = UPNP_LAST_FAIL;
	last.timestampUtc = wxGetUTCTimeMillis().GetValue();
	last.routerId = wxT("router-1");
	last.requested = requested;
	last.mapped.clear();
	last.retryCount = 3;
	last.failureStage = UPNP_STAGE_NONE;
	last.suppressedUntilSessionEnd = true;

	bool suppressedUntilSessionEnd = false;
	wxString reason;
	ASSERT_FALSE(ShouldSuppressUPnPRetry(last, requested, wxGetUTCTimeMillis().GetValue(), true, suppressedUntilSessionEnd, reason));
	ASSERT_FALSE(suppressedUntilSessionEnd);
}


TEST(UPnPFeedbackPolicyTest, FormatsOperationSummary)
{
	CUPnPOperationReport report;
	report.scope = wxT("core");
	report.status = UPNP_LAST_OK;
	report.routerName = wxT("test-router");
	report.adapterId = wxT("192.168.0.2");
	report.requestedMappings.push_back(BuildMapping(4662));
	report.mappedMappings.push_back(BuildMapping(4662));
	report.retryCount = 2;
	report.failureStage = UPNP_STAGE_NONE;

	wxString summary = FormatUPnPOperationSummary(report);
	ASSERT_TRUE(summary.Find(wxT("test-router")) != wxNOT_FOUND);
	ASSERT_TRUE(summary.Find(wxT("status=ok")) != wxNOT_FOUND);
	ASSERT_TRUE(summary.Find(wxT("stage=none")) != wxNOT_FOUND);
	ASSERT_TRUE(summary.Find(wxT("retries=2")) != wxNOT_FOUND);
}


TEST(UPnPFeedbackPolicyTest, FormatsErrorInSummary)
{
	CUPnPOperationReport report;
	report.scope = wxT("core");
	report.status = UPNP_LAST_FAIL;
	report.lastError = wxT("UPnP: conflicto con entrada existente (718)");
	report.failureStage = UPNP_STAGE_MAPPING_PRIMARY_FAILED;

	const wxString summary = FormatUPnPOperationSummary(report);
	ASSERT_TRUE(summary.Find(wxT("status=fail")) != wxNOT_FOUND);
	ASSERT_TRUE(summary.Find(wxT("stage=mapping-primary-failed")) != wxNOT_FOUND);
	ASSERT_TRUE(summary.Find(wxT("718")) != wxNOT_FOUND);
}


TEST(UPnPFeedbackPolicyTest, SummaryReflectsFailureStage)
{
	CUPnPOperationReport report;
	report.scope = wxT("core");
	report.status = UPNP_LAST_FAIL;
	report.failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
	report.discoveredDevices = 3;
	report.filteredDevices = 3;

	const wxString summary = FormatUPnPOperationSummary(report);
	ASSERT_TRUE(summary.Find(wxT("stage=discovery-filtered")) != wxNOT_FOUND);
}


TEST(UPnPFeedbackPolicyTest, PersistsFailureStageInPreferences)
{
	const wxString tempPath = wxFileName::CreateTempFileName(wxT("upnp_prefs"));
	if (!wxFileExists(tempPath)) {
		wxFile file(tempPath, wxFile::write);
		file.Close();
	}
	wxFileConfig config(wxEmptyString, wxEmptyString, tempPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
	wxConfigBase* previous = wxConfigBase::Get();
	wxConfigBase::Set(&config);
	const CUPnPLastResult previousLast = CPreferences::GetLastUPnPResultCore();

	CUPnPLastResult expected = {};
	expected.scope = wxT("core");
	expected.status = UPNP_LAST_FAIL;
	expected.failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
	expected.timestampUtc = 12345;
	expected.routerId = wxT("router-test");
	expected.lastError = wxT("mock error");

	CPreferences::SetLastUPnPResultCore(expected);

	CUPnPLastResult loaded = {};
	CPreferences::LoadUPnPLastResult(wxT("core"), loaded, &config);

	ASSERT_EQUALS(expected.failureStage, loaded.failureStage);
	ASSERT_EQUALS(expected.status, loaded.status);
	ASSERT_EQUALS(expected.routerId, loaded.routerId);
	ASSERT_TRUE(loaded.lastError.Find(wxT("mock error")) != wxNOT_FOUND || !loaded.lastError.IsEmpty());

	wxConfigBase::Set(nullptr);
	CPreferences::SetLastUPnPResultCore(previousLast);
	wxConfigBase::Set(previous);
	wxRemoveFile(tempPath);
}

#endif // ENABLE_UPNP
