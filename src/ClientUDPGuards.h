//
// Shared Client UDP guard helpers for Kad packet validation.
//

#ifndef CLIENTUDP_GUARDS_H
#define CLIENTUDP_GUARDS_H

#include <cstddef>

namespace ClientUDPGuards
{
	inline constexpr std::size_t MAX_KAD_UNCOMPRESSED_PACKET = 128 * 1024;

	inline bool IsCompressedPayloadValid(std::size_t compressedSize)
	{
		return compressedSize <= MAX_KAD_UNCOMPRESSED_PACKET;
	}

	inline bool IsUncompressedPayloadValid(std::size_t uncompressedSize)
	{
		return uncompressedSize <= MAX_KAD_UNCOMPRESSED_PACKET;
	}
}

#endif // CLIENTUDP_GUARDS_H
