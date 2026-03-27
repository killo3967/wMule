//
// This file is part of the aMule Project.
//
// Copyright (c) 2026 wMule Team
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

#include "UPnPUrlUtils.h"

#include <algorithm>
#include <cstdlib>

#include <winsock2.h>
#include <ws2tcpip.h>

bool ParseUrl(const std::string& url, std::string& scheme, std::string& host, uint16_t& port)
{
	const std::string::size_type schemeEnd = url.find("://");
	if (schemeEnd == std::string::npos) {
		return false;
	}
	scheme = url.substr(0, schemeEnd);
	const std::string::size_type authorityStart = schemeEnd + 3;
	if (authorityStart >= url.size()) {
		return false;
	}
	std::string::size_type pathStart = url.find('/', authorityStart);
	std::string hostPort = url.substr(authorityStart, pathStart == std::string::npos ? std::string::npos : pathStart - authorityStart);
	if (hostPort.empty()) {
		return false;
	}
	std::string::size_type colonPos = hostPort.rfind(':');
	if (colonPos != std::string::npos) {
		std::string portStr = hostPort.substr(colonPos + 1);
		host = hostPort.substr(0, colonPos);
		if (portStr.empty()) {
			return false;
		}
		char* endptr = nullptr;
		long parsed = strtol(portStr.c_str(), &endptr, 10);
		if (endptr == portStr.c_str() || parsed <= 0 || parsed > 65535) {
			return false;
		}
		port = static_cast<uint16_t>(parsed);
	} else {
		host = hostPort;
		std::string lowerScheme(scheme);
		std::transform(lowerScheme.begin(), lowerScheme.end(), lowerScheme.begin(), ::tolower);
		port = (lowerScheme == "https") ? 443 : 80;
	}
	return !host.empty();
}

bool HostToIPv4(const std::string& host, uint32_t& outAddr)
{
	if (host.empty()) {
		return false;
	}
	std::string lowerHost(host);
	std::transform(lowerHost.begin(), lowerHost.end(), lowerHost.begin(), ::tolower);
	if (lowerHost == "localhost") {
		outAddr = 0x7F000001u;
		return true;
	}
	in_addr addr {};
	if (inet_pton(AF_INET, host.c_str(), &addr) == 1) {
		outAddr = ntohl(addr.s_addr);
		return true;
	}
	return false;
}

bool IsPrivateIPv4(uint32_t ip)
{
	return ((ip & 0xFF000000u) == 0x0A000000u) ||
		((ip & 0xFFF00000u) == 0xAC100000u) ||
		((ip & 0xFFFF0000u) == 0xC0A80000u) ||
		((ip & 0xFFFF0000u) == 0xA9FE0000u) ||
		((ip & 0xFF000000u) == 0x7F000000u);
}

bool IsLocalIPv4Host(const std::string& host)
{
	uint32_t addr = 0;
	if (!HostToIPv4(host, addr)) {
		return false;
	}
	return IsPrivateIPv4(addr);
}

bool IsSafeLanUrl(const std::string& url)
{
	std::string scheme;
	std::string host;
	uint16_t port = 0;
	if (!ParseUrl(url, scheme, host, port)) {
		return false;
	}
	std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
	if (scheme != "http" && scheme != "https") {
		return false;
	}
	return IsLocalIPv4Host(host);
}

bool AssignLanUrlOrClear(const char* label, const std::string& resolvedUrl, std::string& destination, std::string* outMessage)
{
	if (resolvedUrl.empty()) {
		destination.clear();
		return false;
	}
	if (!IsSafeLanUrl(resolvedUrl)) {
		destination.clear();
		if (outMessage) {
			*outMessage = std::string("Rejecting ") + label + " outside LAN: " + resolvedUrl;
		}
		return false;
	}
	destination = resolvedUrl;
	return true;
}
