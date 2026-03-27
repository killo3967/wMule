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

#ifndef UPNPURLUTILS_H
#define UPNPURLUTILS_H

#include <cstdint>
#include <string>

class wxString;

bool ParseUrl(const std::string& url, std::string& scheme, std::string& host, uint16_t& port);
bool HostToIPv4(const std::string& host, uint32_t& outAddr);
bool IsPrivateIPv4(uint32_t ip);
bool IsLocalIPv4Host(const std::string& host);
bool IsSafeLanUrl(const std::string& url);
bool AssignLanUrlOrClear(const char* label, const std::string& resolvedUrl, std::string& destination, std::string* outMessage = nullptr);

#endif // UPNPURLUTILS_H
