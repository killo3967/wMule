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

#ifndef KAD_PACKET_GUARDS_H
#define KAD_PACKET_GUARDS_H

#include <cstddef>

#include <wx/wx.h>

#include <common/Format.h>

#include "../../MemFile.h"

namespace KadPacketGuards
{
	constexpr uint16_t MAX_KAD_BOOTSTRAP_CONTACTS = 200;
	constexpr uint16_t MAX_KAD_PUBLISH_RECORDS = 64;
	constexpr uint8_t MAX_KAD_TAGS = 64;

	inline size_t KadBytesAvailable(const CMemFile& file)
	{
		return static_cast<size_t>(file.GetLength() - file.GetPosition());
	}

	inline void EnsureKadPayload(const CMemFile& file, size_t needed, const char* context)
	{
		const size_t available = KadBytesAvailable(file);
		if (available < needed) {
			throw wxString(CFormat(wxT("***NOTE: Truncated %s (need %u bytes, have %u)"))
				% wxString::FromAscii(context)
				% static_cast<uint32>(needed)
				% static_cast<uint32>(available));
		}
	}

	inline void EnsureKadTagCount(uint32_t tags, const char* context)
	{
		if (tags > MAX_KAD_TAGS) {
			throw wxString(CFormat(wxT("***NOTE: Tag count %u exceeds limit in %s"))
				% tags
				% wxString::FromAscii(context));
		}
	}
}

#endif // KAD_PACKET_GUARDS_H
