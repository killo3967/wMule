//
// Test stubs for the concurrency suite.
// Provides minimal implementations of the preferences/stats hooks
// required by ThreadGuards without pulling the entire app stack.
//

#include <Preferences.h>
#include <Statistics.h>

#include <algorithm>

uint16 CPreferences::s_maxupload = 0;
uint16 CPreferences::s_maxdownload = 0;
bool CPreferences::s_verboseThreading = false;
uint32 CPreferences::s_threadDrainTimeoutMs = CPreferences::THREAD_DRAIN_TIMEOUT_MIN_MS;

void CPreferences::SetMaxUpload(uint16 in)
{
	s_maxupload = in;
}

void CPreferences::SetMaxDownload(uint16 in)
{
	s_maxdownload = in;
}

void CPreferences::SetThreadDrainTimeoutMs(uint32 val)
{
	const uint32 clamped = std::clamp(val, THREAD_DRAIN_TIMEOUT_MIN_MS, THREAD_DRAIN_TIMEOUT_MAX_MS);
	s_threadDrainTimeoutMs = clamped;
}

ThreadingStatsSnapshot CStatistics::GetThreadingStatsSnapshot()
{
	ThreadingStatsSnapshot snapshot;
	return snapshot;
}
