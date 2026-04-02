//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


// This define must not conflict with the one in the standard header
#ifndef AMULE_UPNP_H
#define AMULE_UPNP_H


#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <wx/longlong.h>
#include <wx/string.h>

#include "Preferences.h"
#include "UPnPCompatibility.h"


extern std::string stdEmptyString;


class CUPnPPortMapping
{
private:
	std::string m_port;
	std::string m_protocol;
	std::string m_enabled;
	std::string m_description;
	std::string m_key;

public:
	CUPnPPortMapping(
		int port = 0,
		const std::string &protocol = stdEmptyString,
		bool enabled = false,
		const std::string &description = stdEmptyString);
	~CUPnPPortMapping() {}

	const std::string &getPort() const
		{ return m_port; }
	const std::string &getProtocol() const
		{ return m_protocol; }
	const std::string &getEnabled() const
		{ return m_enabled; }
	const std::string &getDescription() const
		{ return m_description; }
	const std::string &getKey() const
		{ return m_key; }
};


struct CUPnPDiscoveryInfo
{
	wxString routerId;
	wxString routerName;
	wxString adapterId;
	std::string descUrl;
	std::string controlUrl;
	std::string serviceType;
	uint32 discoveredDevices;
	uint32 filteredDevices;
	CUPnPFailureStage failureStage;
};


struct CUPnPOperationReport
{
	wxString scope;
	CUPnPLastStatus status;
	int64 timestampUtc;
	wxString routerId;
	wxString routerName;
	wxString adapterId;
	std::vector<CUPnPPersistedMapping> requestedMappings;
	std::vector<CUPnPPersistedMapping> mappedMappings;
	uint32 retryCount;
	wxString lastError;
	uint32 discoveredDevices;
	uint32 filteredDevices;
	CUPnPFailureStage failureStage;
	bool suppressedByPolicy;
	bool suppressedUntilSessionEnd;
};


class IUPnPClient
{
public:
	virtual ~IUPnPClient() {}
	virtual bool Discover(CUPnPDiscoveryInfo& out, wxString& error) = 0;
	virtual bool AddMapping(const CUPnPPortMapping& mapping, wxString& error) = 0;
	virtual bool DeleteMapping(const CUPnPPortMapping& mapping, wxString& error) = 0;
	virtual bool GetExternalIp(wxString& outIp, wxString& error) = 0;
	virtual CUPnPFailureStage GetFailureStage() const { return UPNP_STAGE_NONE; }
	virtual uint16 GetLastMappedExternalPort() const { return 0; }
	virtual uint32 GetDiscoveredDeviceCount() const { return 0; }
	virtual uint32 GetFilteredDeviceCount() const { return 0; }
};


class CUPnPControlPoint
{
public:
	explicit CUPnPControlPoint(unsigned short udpPort);
	CUPnPControlPoint(unsigned short udpPort, std::unique_ptr<IUPnPClient> client);
	~CUPnPControlPoint();

	bool AddPortMappings(
		std::vector<CUPnPPortMapping> &upnpPortMapping,
		std::vector<CUPnPPersistedMapping> *mapped = nullptr);
	bool DeletePortMappings(
		std::vector<CUPnPPortMapping> &upnpPortMapping);
	CUPnPOperationReport ExecuteMappings(
		const std::vector<CUPnPPortMapping> &upnpPortMapping,
		const wxString& scope,
		const CUPnPLastResult& lastResult,
		bool forceRetry);

	bool WanServiceDetected() const
		{ return m_discoverySucceeded; }

private:
	bool EnsureDiscovery(wxString& error);
	void CaptureRouterIdentity(wxString& routerId, wxString& routerName) const;
	void ResetMappingRetryCount()
		{ m_mappingRetryCount = 0; }
	void IncrementMappingRetryCount()
		{ ++m_mappingRetryCount; }
	int GetMappingRetryCount() const
		{ return m_mappingRetryCount; }
	bool HasMappingRetriesLeft() const
		{ return m_mappingRetryCount < m_defaultMaxRetries; }

private:
	static constexpr int m_defaultMaxRetries = 3;
	static constexpr uint32_t m_initialRetryDelayMs = 500;
	std::unique_ptr<IUPnPClient> m_client;
	CUPnPDiscoveryInfo m_discoveryInfo;
	bool m_discoverySucceeded;
	int m_mappingRetryCount;
	wxString m_lastOperationError;
	CUPnPFailureStage m_lastFailureStage;
	uint32 m_lastDiscoveredDevices;
	uint32 m_lastFilteredDevices;
};


CUPnPLastResult ToUPnPLastResult(const CUPnPOperationReport& report);
wxString FormatUPnPOperationSummary(const CUPnPOperationReport& report);
bool ShouldSuppressUPnPRetry(
	const CUPnPLastResult& lastResult,
	const std::vector<CUPnPPersistedMapping>& requested,
	wxLongLong_t now,
	bool forceRetry,
	bool& suppressedUntilSessionEnd,
	wxString& reason);


#endif /* AMULE_UPNP_H */

// File_checked_for_headers
