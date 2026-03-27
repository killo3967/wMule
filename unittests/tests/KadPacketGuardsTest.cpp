#include <muleunit/test.h>

#include <kademlia/net/KadPacketGuards.h>

using namespace muleunit;
using namespace KadPacketGuards;

DECLARE_SIMPLE(KadPacketGuardsTest);

TEST(KadPacketGuardsTest, EnsureKadPayloadAllowsExactBytes)
{
	const uint8 buffer[] = {0xAA, 0xBB, 0xCC, 0xDD};
	CMemFile file(buffer, sizeof(buffer));
	EnsureKadPayload(file, sizeof(buffer), __FUNCTION__);
}

TEST(KadPacketGuardsTest, EnsureKadPayloadHonorsFilePosition)
{
	const uint8 buffer[] = {0x01, 0x02, 0x03, 0x04};
	CMemFile file(buffer, sizeof(buffer));
	file.ReadUInt16();
	EnsureKadPayload(file, 2, __FUNCTION__);
	ASSERT_RAISES(wxString, EnsureKadPayload(file, 3, __FUNCTION__));
}

TEST(KadPacketGuardsTest, EnsureKadPayloadFailsWhenTruncated)
{
	const uint8 buffer[] = {0x01, 0x02};
	CMemFile file(buffer, sizeof(buffer));
	file.ReadUInt8();
	ASSERT_RAISES(wxString, EnsureKadPayload(file, 2, __FUNCTION__));
}

TEST(KadPacketGuardsTest, EnsureKadTagCountHonorsMaximum)
{
	EnsureKadTagCount(MAX_KAD_TAGS, __FUNCTION__);
	ASSERT_RAISES(wxString, EnsureKadTagCount(MAX_KAD_TAGS + 1, __FUNCTION__));
}
