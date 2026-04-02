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

#include "config.h" // Needed for ENABLE_UPNP

#ifdef ENABLE_UPNP

#include "UPnPBase.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

#include <wx/utils.h>

#include "Logger.h"
#include "UPnPUrlUtils.h"

#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

// Usar las constantes de miniupnpc.h directamente
// UPNP_GetValidIGD retorna:
//   UPNP_NO_IGD (0) = No se encontró IGD
//   UPNP_CONNECTED_IGD (1) = IGD válido y conectado a internet
//   UPNP_PRIVATEIP_IGD (2) = IGD válido pero con dirección privada (no routable)
//   UPNP_DISCONNECTED_IGD (3) = IGD válido pero desconectado de internet
//   UPNP_UNKNOWN_DEVICE (4) = Dispositivo desconocido

std::string stdEmptyString;

namespace {

const wxLongLong_t kUPnPSuppressionWindowMs = 15 * 60 * 1000;

wxString FailureStageLabel(CUPnPFailureStage stage)
{
	switch (stage) {
	case UPNP_STAGE_DISCOVERY_EMPTY:
		return wxT("discovery-empty");
	case UPNP_STAGE_DISCOVERY_FILTERED:
		return wxT("discovery-filtered");
	case UPNP_STAGE_IGD_INVALID:
		return wxT("igd-invalid");
	case UPNP_STAGE_MAPPING_PRIMARY_FAILED:
		return wxT("mapping-primary-failed");
	case UPNP_STAGE_MAPPING_FALLBACK_FAILED:
		return wxT("mapping-fallback-failed");
	case UPNP_STAGE_NONE:
	default:
		return wxT("none");
	}
}


CUPnPPersistedMapping ToPersistedMapping(const CUPnPPortMapping& mapping, uint16 externalPortOverride = 0)
{
	CUPnPPersistedMapping persisted;
	const unsigned long port = std::strtoul(mapping.getPort().c_str(), nullptr, 10);
	persisted.service = wxString::FromUTF8(mapping.getDescription().c_str());
	persisted.protocol = wxString::FromUTF8(mapping.getProtocol().c_str());
	persisted.internalPort = static_cast<uint16>(port);
	if (externalPortOverride != 0) {
		persisted.externalPort = externalPortOverride;
	} else {
		persisted.externalPort = static_cast<uint16>(port);
	}
	persisted.enabled = mapping.getEnabled() == "1";
	return persisted;
}


std::vector<CUPnPPersistedMapping> ToPersistedMappings(const std::vector<CUPnPPortMapping>& mappings)
{
	std::vector<CUPnPPersistedMapping> persisted;
	persisted.reserve(mappings.size());
	for (std::vector<CUPnPPortMapping>::const_iterator it = mappings.begin(); it != mappings.end(); ++it) {
		persisted.push_back(ToPersistedMapping(*it));
	}
	return persisted;
}


bool CompareMappingLess(const CUPnPPersistedMapping& lhs, const CUPnPPersistedMapping& rhs)
{
	int serviceCmp = lhs.service.CmpNoCase(rhs.service);
	if (serviceCmp != 0) {
		return serviceCmp < 0;
	}
	int protocolCmp = lhs.protocol.CmpNoCase(rhs.protocol);
	if (protocolCmp != 0) {
		return protocolCmp < 0;
	}
	if (lhs.internalPort != rhs.internalPort) {
		return lhs.internalPort < rhs.internalPort;
	}
	if (lhs.externalPort != rhs.externalPort) {
		return lhs.externalPort < rhs.externalPort;
	}
	return lhs.enabled && !rhs.enabled;
}


bool HaveSameMappings(const std::vector<CUPnPPersistedMapping>& lhs, const std::vector<CUPnPPersistedMapping>& rhs)
{
	if (lhs.size() != rhs.size()) {
		return false;
	}

	std::vector<CUPnPPersistedMapping> sortedLhs(lhs);
	std::vector<CUPnPPersistedMapping> sortedRhs(rhs);
	std::sort(sortedLhs.begin(), sortedLhs.end(), CompareMappingLess);
	std::sort(sortedRhs.begin(), sortedRhs.end(), CompareMappingLess);

	for (size_t i = 0; i < sortedLhs.size(); ++i) {
		const CUPnPPersistedMapping& left = sortedLhs[i];
		const CUPnPPersistedMapping& right = sortedRhs[i];
		if (left.enabled != right.enabled) {
			return false;
		}
		if (left.internalPort != right.internalPort || left.externalPort != right.externalPort) {
			return false;
		}
		if (left.service.CmpNoCase(right.service) != 0) {
			return false;
		}
		if (left.protocol.CmpNoCase(right.protocol) != 0) {
			return false;
		}
	}

	return true;
}


wxString DescribeMiniupnpcError(int code)
{
	switch (code) {
	case 0: // UPNPCOMMAND_SUCCESS from upnpcommands.h
		return wxEmptyString;
	case 714:
		return wxT("UPnP: el mapeo solicitado no existe (714: NoSuchEntryInArray).");
	case 718:
		return wxT("UPnP: conflicto con una entrada existente (718: ConflictInMappingEntry).");
	case UPNP_DISCONNECTED_IGD: // Equivalent to not connected to internet
		return wxT("El IGD encontrado no está conectado a Internet.");
	case UPNP_PRIVATEIP_IGD: // Valid but reserved address
		return wxT("El IGD encontrado tiene una dirección privada y no es accesible desde Internet.");
	default:
		return wxString::Format(wxT("UPnP error %d: %s"), code, wxString::FromUTF8(strupnperror(code)));
	}
}


bool ShouldAttemptPortFallback(int code)
{
	return code == 718 || code == 713;
}


class CMiniUPnPClient : public IUPnPClient
{
public:
CMiniUPnPClient()
:
m_urls(),
m_data(),
m_lanAddress(),
m_hasUrls(false),
m_failureStage(UPNP_STAGE_NONE),
m_discoveredDevices(0),
m_filteredDevices(0),
m_lastMappedExternalPort(0)
{
	std::memset(&m_urls, 0, sizeof(m_urls));
	std::memset(&m_data, 0, sizeof(m_data));
}

	~CMiniUPnPClient() override
	{
		FreeUPNPUrls(&m_urls);
	}

	bool Discover(CUPnPDiscoveryInfo& out, wxString& error) override
	{
		FreeUPNPUrls(&m_urls);
		std::memset(&m_urls, 0, sizeof(m_urls));
		std::memset(&m_data, 0, sizeof(m_data));
		m_lanAddress.clear();
		m_hasUrls = false;
		m_failureStage = UPNP_STAGE_NONE;
		m_discoveredDevices = 0;
		m_filteredDevices = 0;
		m_lastMappedExternalPort = 0;

		int discoverErr = 0;
		UPNPDev* devlist = upnpDiscover(2000, nullptr, nullptr, 0, 0, 2, &discoverErr);
		if (devlist == nullptr) {
			m_failureStage = UPNP_STAGE_DISCOVERY_EMPTY;
			error = wxString::Format(wxT("No se encontraron dispositivos UPnP en LAN (discover err=%d)."), discoverErr);
			return false;
		}

		size_t safeDevices = 0;
		int rejectedDevices = 0;
		for (UPNPDev* dev = devlist; dev != nullptr; dev = dev->pNext) {
			++m_discoveredDevices;
			std::string descCandidate = dev->descURL ? dev->descURL : stdEmptyString;
			std::string ignored;
			if (AssignLanUrlOrClear("descURL", descCandidate, ignored, nullptr)) {
				++safeDevices;
			} else {
				++rejectedDevices;
			}
		}
		m_filteredDevices = static_cast<uint32>(rejectedDevices);

		if (safeDevices == 0) {
			freeUPNPDevlist(devlist);
			m_failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
			error = wxT("Descubrimiento UPnP descartado: ningún IGD con descURL LAN válido.");
			return false;
		}
		if (rejectedDevices > 0) {
			AddDebugLogLineC(logUPnP, wxString::Format(wxT("UPnP: descartando %d dispositivos con descURL fuera de LAN antes de validar IGD"), rejectedDevices));
		}

		std::vector<UPNPDev> filtered;
		filtered.reserve(safeDevices);
		for (UPNPDev* dev = devlist; dev != nullptr; dev = dev->pNext) {
			std::string descCandidate = dev->descURL ? dev->descURL : stdEmptyString;
			std::string ignored;
			if (AssignLanUrlOrClear("descURL", descCandidate, ignored, nullptr)) {
				filtered.push_back(*dev);
			}
		}
		for (size_t i = 0; i + 1 < filtered.size(); ++i) {
			filtered[i].pNext = &filtered[i + 1];
		}
		if (!filtered.empty()) {
			filtered.back().pNext = nullptr;
		}

        UPNPDev* filteredList = filtered.empty() ? nullptr : &filtered.front();

		char lanaddr[64] = {0};
		char wanaddr[64] = {0};
		const int igd = UPNP_GetValidIGD(filteredList, &m_urls, &m_data,
			lanaddr, static_cast<int>(sizeof(lanaddr)),
			wanaddr, static_cast<int>(sizeof(wanaddr)));
		freeUPNPDevlist(devlist);

		// Mapear códigos de retorno de UPNP_GetValidIGD a nuestros manejos de error
		if (igd == UPNP_DISCONNECTED_IGD) { // Equivalent to not connected to internet
			error = DescribeMiniupnpcError(igd);
			m_failureStage = UPNP_STAGE_IGD_INVALID;
			FreeUPNPUrls(&m_urls);
			return false;
		}
		if (igd <= 0 || igd == UPNP_NO_IGD || igd == UPNP_PRIVATEIP_IGD) { // Tratar privados como error para ahora
			error = DescribeMiniupnpcError(igd);
			m_failureStage = UPNP_STAGE_IGD_INVALID;
			FreeUPNPUrls(&m_urls);
			return false;
		}
		std::string descUrl = m_urls.ipcondescURL ? m_urls.ipcondescURL : stdEmptyString;
		std::string controlUrl = m_urls.controlURL ? m_urls.controlURL : stdEmptyString;
		std::string warning;
		if (!AssignLanUrlOrClear("descURL", descUrl, descUrl, &warning)) {
			error = wxString::FromUTF8(warning.c_str());
			m_failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
			if (m_filteredDevices == 0) {
				m_filteredDevices = m_discoveredDevices;
			}
			FreeUPNPUrls(&m_urls);
			return false;
		}
		if (!AssignLanUrlOrClear("controlURL", controlUrl, controlUrl, &warning)) {
			error = wxString::FromUTF8(warning.c_str());
			m_failureStage = UPNP_STAGE_DISCOVERY_FILTERED;
			if (m_filteredDevices == 0) {
				m_filteredDevices = m_discoveredDevices;
			}
			FreeUPNPUrls(&m_urls);
			return false;
		}

		m_lanAddress = lanaddr;
		m_hasUrls = true;

		out.routerId = wxString::FromUTF8(m_data.urlbase);
		if (out.routerId.IsEmpty()) {
			out.routerId = wxString::FromUTF8(controlUrl.c_str());
		}
		out.routerName = wxString::FromUTF8(m_data.presentationurl);
		if (out.routerName.IsEmpty()) {
			out.routerName = wxString::FromUTF8(m_data.first.servicetype);
		}
		out.adapterId = wxString::FromUTF8(m_lanAddress.c_str());
		out.descUrl = descUrl;
		out.controlUrl = controlUrl;
		out.serviceType = std::string(m_data.first.servicetype);
		out.discoveredDevices = m_discoveredDevices;
		out.filteredDevices = m_filteredDevices;
		out.failureStage = UPNP_STAGE_NONE;
		m_failureStage = UPNP_STAGE_NONE;
		return true;
	}

	bool AddMapping(const CUPnPPortMapping& mapping, wxString& error) override
	{
		if (!m_hasUrls) {
			error = wxT("UPnP no está inicializado: no hay IGD válido.");
			m_failureStage = UPNP_STAGE_IGD_INVALID;
			return false;
		}
		m_failureStage = UPNP_STAGE_NONE;
		m_lastMappedExternalPort = 0;

		const int ret = UPNP_AddPortMapping(
			m_urls.controlURL,
			m_data.first.servicetype,
			mapping.getPort().c_str(),
			mapping.getPort().c_str(),
			m_lanAddress.c_str(),
			mapping.getDescription().c_str(),
			mapping.getProtocol().c_str(),
			nullptr,
			"0");

		if (ret != UPNPCOMMAND_SUCCESS) {
			const bool fallbackEligible = ShouldAttemptPortFallback(ret);
			error = DescribeMiniupnpcError(ret);
			m_failureStage = UPNP_STAGE_MAPPING_PRIMARY_FAILED;
			wxString fallbackError;
			bool fallbackTried = false;
			if (fallbackEligible && SupportsAddAnyPortMapping()) {
				fallbackTried = true;
				AddDebugLogLineC(logUPnP, wxString::Format(wxT("UPnP: router rechazó el mismo puerto (error %d), intentando AddAnyPortMapping"), ret));
				if (TryFallbackAddAnyPortMapping(mapping, fallbackError)) {
					m_failureStage = UPNP_STAGE_NONE;
					if (m_lastMappedExternalPort != 0) {
						AddDebugLogLineC(logUPnP, wxString::Format(wxT("UPnP: AddAnyPortMapping aplicó fallback con puerto externo %u"), m_lastMappedExternalPort));
					} else {
						AddDebugLogLineC(logUPnP, wxT("UPnP: AddAnyPortMapping aplicó fallback con puerto externo asignado por el router"));
					}
					return true;
				}
			} else if (fallbackEligible) {
				AddDebugLogLineC(logUPnP, wxT("UPnP: router rechazó el mismo puerto pero la librería miniupnpc no ofrece AddAnyPortMapping."));
			}

			if (fallbackEligible && fallbackTried) {
				m_failureStage = UPNP_STAGE_MAPPING_FALLBACK_FAILED;
			}
			if (fallbackEligible && !fallbackError.IsEmpty()) {
				error = fallbackError;
				AddDebugLogLineC(logUPnP, fallbackError);
			}
			return false;
		}

		m_lastMappedExternalPort = ParseMappingPort(mapping);
		return true;
	}

	bool DeleteMapping(const CUPnPPortMapping& mapping, wxString& error) override
	{
		if (!m_hasUrls) {
			error = wxT("UPnP no está inicializado: no hay IGD válido.");
			return false;
		}

		const int ret = UPNP_DeletePortMapping(
			m_urls.controlURL,
			m_data.first.servicetype,
			mapping.getPort().c_str(),
			mapping.getProtocol().c_str(),
			nullptr);
		if (ret != UPNPCOMMAND_SUCCESS) {
			error = DescribeMiniupnpcError(ret);
			return false;
		}
		return true;
	}

	bool GetExternalIp(wxString& outIp, wxString& error) override
	{
		if (!m_hasUrls) {
			error = wxT("UPnP no está inicializado: no hay IGD válido.");
			return false;
		}
		char externalIp[40] = {0};
		const int ret = UPNP_GetExternalIPAddress(
			m_urls.controlURL,
			m_data.first.servicetype,
			externalIp);
		if (ret != UPNPCOMMAND_SUCCESS) {
			error = DescribeMiniupnpcError(ret);
			return false;
		}
		outIp = wxString::FromUTF8(externalIp);
		return true;
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
		return m_discoveredDevices;
	}

	uint32 GetFilteredDeviceCount() const override
	{
		return m_filteredDevices;
	}

private:
	bool SupportsAddAnyPortMapping() const
	{
		return true;
	}

	bool TryFallbackAddAnyPortMapping(const CUPnPPortMapping& mapping, wxString& error)
	{
		if (!SupportsAddAnyPortMapping()) {
			return false;
		}
		char reservedPort[6] = {0};
		const int ret = UPNP_AddAnyPortMapping(
			m_urls.controlURL,
			m_data.first.servicetype,
			mapping.getPort().c_str(),
			mapping.getPort().c_str(),
			m_lanAddress.c_str(),
			mapping.getDescription().c_str(),
			mapping.getProtocol().c_str(),
			nullptr,
			"0",
			reservedPort);
		if (ret != UPNPCOMMAND_SUCCESS) {
			error = DescribeMiniupnpcError(ret);
			return false;
		}

		unsigned long parsedPort = std::strtoul(reservedPort, nullptr, 10);
		if (parsedPort == 0 || parsedPort > 65535) {
			error = wxString::Format(wxT("UPnP: AddAnyPortMapping devolvió un puerto inválido (%s)."), wxString::FromUTF8(reservedPort));
			return false;
		}
		m_lastMappedExternalPort = static_cast<uint16>(parsedPort);
		return true;
	}

	uint16 ParseMappingPort(const CUPnPPortMapping& mapping) const
	{
		return static_cast<uint16>(std::strtoul(mapping.getPort().c_str(), nullptr, 10));
	}

	UPNPUrls m_urls;
	IGDdatas m_data;
	std::string m_lanAddress;
	bool m_hasUrls;
	CUPnPFailureStage m_failureStage;
	uint32 m_discoveredDevices;
	uint32 m_filteredDevices;
	uint16 m_lastMappedExternalPort;
};

} // namespace


bool ShouldSuppressUPnPRetry(
	const CUPnPLastResult& lastResult,
	const std::vector<CUPnPPersistedMapping>& requested,
	wxLongLong_t now,
	bool forceRetry,
	bool& suppressedUntilSessionEnd,
	wxString& reason)
{
	suppressedUntilSessionEnd = false;
	if (forceRetry) {
		return false;
	}

	const bool lastWasFailure = lastResult.status == UPNP_LAST_FAIL || lastResult.status == UPNP_LAST_SUPPRESSED;
	if (!lastWasFailure) {
		return false;
	}
	if (lastResult.routerId.IsEmpty()) {
		return false;
	}
	if (!HaveSameMappings(lastResult.requested, requested)) {
		return false;
	}
	const wxLongLong_t age = now - static_cast<wxLongLong_t>(lastResult.timestampUtc);
	const bool withinWindow = age >= 0 && age < kUPnPSuppressionWindowMs;
	if (!withinWindow && !lastResult.suppressedUntilSessionEnd) {
		return false;
	}

	suppressedUntilSessionEnd = lastResult.suppressedUntilSessionEnd;
	reason = wxT("UPnP reintento suprimido: el intento previo falló recientemente para el mismo router/puertos.");
	if (lastResult.suppressedUntilSessionEnd) {
		reason = wxT("UPnP reintento suprimido para esta sesión tras el fallo previo. Usa el botón de reintento manual para forzarlo.");
	}
	return true;
}


CUPnPPortMapping::CUPnPPortMapping(
	int port,
	const std::string &protocol,
	bool enabled,
	const std::string &description)
:
m_port(),
m_protocol(protocol),
m_enabled(enabled ? "1" : "0"),
m_description(description),
m_key()
{
	std::ostringstream oss;
	oss << port;
	m_port = oss.str();
	m_key = m_protocol + m_port;
}


CUPnPControlPoint::CUPnPControlPoint(unsigned short udpPort)
:
CUPnPControlPoint(udpPort, std::unique_ptr<IUPnPClient>())
{
}


CUPnPControlPoint::CUPnPControlPoint(unsigned short /*udpPort*/, std::unique_ptr<IUPnPClient> client)
:
m_client(client ? std::move(client) : std::unique_ptr<IUPnPClient>(new CMiniUPnPClient())),
m_discoveryInfo(),
m_discoverySucceeded(false),
m_mappingRetryCount(0),
m_lastOperationError(),
m_lastFailureStage(UPNP_STAGE_NONE),
m_lastDiscoveredDevices(0),
m_lastFilteredDevices(0)
{
	wxString ignoredError;
	EnsureDiscovery(ignoredError);
}


CUPnPControlPoint::~CUPnPControlPoint() = default;


bool CUPnPControlPoint::EnsureDiscovery(wxString& error)
{
	if (m_discoverySucceeded) {
		return true;
	}

	m_lastFailureStage = UPNP_STAGE_NONE;
	m_lastDiscoveredDevices = 0;
	m_lastFilteredDevices = 0;

	CUPnPDiscoveryInfo info;
	if (!m_client->Discover(info, error)) {
		m_lastFailureStage = m_client->GetFailureStage();
		m_lastDiscoveredDevices = m_client->GetDiscoveredDeviceCount();
		m_lastFilteredDevices = m_client->GetFilteredDeviceCount();
		m_discoveryInfo = CUPnPDiscoveryInfo();
		m_discoveryInfo.failureStage = m_lastFailureStage;
		m_discoveryInfo.discoveredDevices = m_lastDiscoveredDevices;
		m_discoveryInfo.filteredDevices = m_lastFilteredDevices;
		return false;
	}
	m_discoveryInfo = info;
	m_discoverySucceeded = true;
	m_lastFailureStage = info.failureStage;
	m_lastDiscoveredDevices = info.discoveredDevices;
	m_lastFilteredDevices = info.filteredDevices;
	return true;
}


bool CUPnPControlPoint::AddPortMappings(
	std::vector<CUPnPPortMapping> &upnpPortMapping,
	std::vector<CUPnPPersistedMapping> *mapped)
{
	m_lastOperationError.Clear();
	m_lastFailureStage = UPNP_STAGE_NONE;
	wxString discoveryError;
	if (!EnsureDiscovery(discoveryError)) {
		m_lastOperationError = discoveryError;
		if (!discoveryError.IsEmpty()) {
			AddDebugLogLineC(logUPnP, discoveryError);
		}
		return false;
	}

	ResetMappingRetryCount();
	if (mapped) {
		mapped->clear();
	}

	size_t expectedEnabled = 0;
	size_t mappedCount = 0;

	for (std::vector<CUPnPPortMapping>::iterator it = upnpPortMapping.begin(); it != upnpPortMapping.end(); ++it) {
		if (it->getEnabled() != "1") {
			continue;
		}
		++expectedEnabled;

		bool mappingSuccess = false;
		wxString lastError;
		do {
			mappingSuccess = m_client->AddMapping(*it, lastError);
			if (mappingSuccess) {
				break;
			}
			if (!HasMappingRetriesLeft()) {
				break;
			}
			uint32_t delay = m_initialRetryDelayMs * (1u << GetMappingRetryCount());
			AddDebugLogLineC(logUPnP, wxString::Format(wxT("UPnP: reintentando mapeo de puerto %s en %u ms"), wxString::FromUTF8(it->getPort().c_str()), delay));
			wxMilliSleep(delay);
			IncrementMappingRetryCount();
		} while (!mappingSuccess && HasMappingRetriesLeft());

		if (mappingSuccess) {
			++mappedCount;
			if (mapped) {
				const uint16 externalPort = m_client->GetLastMappedExternalPort();
				mapped->push_back(ToPersistedMapping(*it, externalPort));
			}
			if (GetMappingRetryCount() > 0) {
				AddDebugLogLineN(logUPnP, wxString::Format(wxT("UPnP: mapeo de puerto %s succeeded after %d retries"), wxString::FromUTF8(it->getPort().c_str()), GetMappingRetryCount()));
			}
		} else {
			if (m_lastOperationError.IsEmpty()) {
				m_lastOperationError = lastError;
			}
			if (!lastError.IsEmpty()) {
				AddDebugLogLineC(logUPnP, lastError);
			}
			if (m_lastFailureStage == UPNP_STAGE_NONE) {
				m_lastFailureStage = m_client->GetFailureStage();
			}
		}
	}

	if (expectedEnabled == 0) {
		return false;
	}

	return mappedCount == expectedEnabled;
}


bool CUPnPControlPoint::DeletePortMappings(
	std::vector<CUPnPPortMapping> &upnpPortMapping)
{
	m_lastOperationError.Clear();
	wxString discoveryError;
	if (!EnsureDiscovery(discoveryError)) {
		m_lastOperationError = discoveryError;
		return false;
	}

	bool allDeleted = true;
	for (std::vector<CUPnPPortMapping>::iterator it = upnpPortMapping.begin(); it != upnpPortMapping.end(); ++it) {
		if (it->getEnabled() != "1") {
			continue;
		}

		wxString lastError;
		if (!m_client->DeleteMapping(*it, lastError)) {
			allDeleted = false;
			if (m_lastOperationError.IsEmpty()) {
				m_lastOperationError = lastError;
			}
			if (!lastError.IsEmpty()) {
				AddDebugLogLineC(logUPnP, lastError);
			}
		}
	}

	return allDeleted;
}


CUPnPOperationReport CUPnPControlPoint::ExecuteMappings(
	const std::vector<CUPnPPortMapping> &upnpPortMapping,
	const wxString& scope,
	const CUPnPLastResult& lastResult,
	bool forceRetry)
{
	CUPnPOperationReport report;
	report.scope = scope;
	report.status = UPNP_LAST_UNKNOWN;
	report.timestampUtc = wxGetUTCTimeMillis().GetValue();
	report.retryCount = 0;
	report.suppressedByPolicy = false;
	report.suppressedUntilSessionEnd = false;
	report.failureStage = UPNP_STAGE_NONE;
	report.discoveredDevices = m_lastDiscoveredDevices;
	report.filteredDevices = m_lastFilteredDevices;
	report.requestedMappings = ToPersistedMappings(upnpPortMapping);

	wxString suppressionReason;
	bool suppressedUntilEnd = false;
	if (ShouldSuppressUPnPRetry(lastResult, report.requestedMappings, report.timestampUtc, forceRetry, suppressedUntilEnd, suppressionReason)) {
		report.status = UPNP_LAST_SUPPRESSED;
		report.lastError = suppressionReason;
		report.suppressedByPolicy = true;
		report.suppressedUntilSessionEnd = suppressedUntilEnd;
		report.failureStage = lastResult.failureStage;
		return report;
	}

	std::vector<CUPnPPortMapping> mutableMappings = upnpPortMapping;
	std::vector<CUPnPPersistedMapping> mapped;
	const bool allMapped = AddPortMappings(mutableMappings, &mapped);
	report.mappedMappings.swap(mapped);
	report.retryCount = GetMappingRetryCount();
	report.suppressedUntilSessionEnd = !allMapped;
	report.failureStage = m_lastFailureStage;
	report.discoveredDevices = m_lastDiscoveredDevices;
	report.filteredDevices = m_lastFilteredDevices;

	CaptureRouterIdentity(report.routerId, report.routerName);
	report.adapterId = m_discoveryInfo.adapterId;

	if (allMapped) {
		report.status = UPNP_LAST_OK;
	} else {
		report.status = UPNP_LAST_FAIL;
		report.lastError = m_lastOperationError;
		if (report.lastError.IsEmpty()) {
			report.lastError = wxT("UPnP no pudo abrir todos los puertos solicitados.");
		}
	}

	return report;
}


void CUPnPControlPoint::CaptureRouterIdentity(wxString& routerId, wxString& routerName) const
{
	routerId = m_discoveryInfo.routerId;
	routerName = m_discoveryInfo.routerName;
}


CUPnPLastResult ToUPnPLastResult(const CUPnPOperationReport& report)
{
	CUPnPLastResult result;
	result.scope = report.scope;
	result.status = report.status;
	result.timestampUtc = report.timestampUtc;
	result.routerId = report.routerId;
	result.routerName = report.routerName;
	result.adapterId = report.adapterId;
	result.requested = report.requestedMappings;
	result.mapped = report.mappedMappings;
	result.retryCount = report.retryCount;
	result.lastError = report.lastError;
	result.failureStage = report.failureStage;
	result.suppressedUntilSessionEnd = report.suppressedUntilSessionEnd;
	return result;
}


wxString FormatUPnPOperationSummary(const CUPnPOperationReport& report)
{
	wxString statusLabel;
	switch (report.status) {
	case UPNP_LAST_OK:
		statusLabel = wxT("ok");
		break;
	case UPNP_LAST_FAIL:
		statusLabel = wxT("fail");
		break;
	case UPNP_LAST_DISABLED:
		statusLabel = wxT("disabled");
		break;
	case UPNP_LAST_SUPPRESSED:
		statusLabel = wxT("suppressed");
		break;
	case UPNP_LAST_UNKNOWN:
	default:
		statusLabel = wxT("unknown");
		break;
	}

	wxString routerLabel = report.routerName.IsEmpty() ? report.routerId : report.routerName;
	if (routerLabel.IsEmpty()) {
		routerLabel = wxT("unknown");
	}
	wxString adapterLabel = report.adapterId.IsEmpty() ? wxString(wxT("unknown")) : report.adapterId;
	const wxString failureStage = FailureStageLabel(report.failureStage);

	wxString summary = wxString::Format(
		wxT("UPnP [%s]: status=%s stage=%s router=%s adapter=%s requested=%zu mapped=%zu retries=%u discovered=%u filtered=%u"),
		report.scope,
		statusLabel,
		failureStage,
		routerLabel,
		adapterLabel,
		report.requestedMappings.size(),
		report.mappedMappings.size(),
		report.retryCount,
		report.discoveredDevices,
		report.filteredDevices);

	if (!report.lastError.IsEmpty()) {
		summary << wxT(" error=\"") << report.lastError << wxT("\"");
	}
	if (report.suppressedByPolicy) {
		summary << wxT(" (suppressed)");
	}

	return summary;
}


#endif // ENABLE_UPNP
