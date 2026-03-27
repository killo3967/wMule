#include <muleunit/test.h>

#include <kademlia/net/KadPacketGuards.h>

#include <MemFile.h>

#include <cstdint>
#include <vector>

using namespace muleunit;
using namespace KadPacketGuards;

DECLARE_SIMPLE(KadHandlerFuzzTest);

namespace
{
	void RunPayloadWindowTest(size_t totalSize)
	{
		std::vector<uint8_t> buffer(totalSize);
		for (size_t i = 0; i < totalSize; ++i) {
			buffer[i] = static_cast<uint8_t>(i & 0xFF);
		}
		for (size_t consumed = 0; consumed <= totalSize; ++consumed) {
		CMemFile file(buffer.data(), buffer.size());
			if (consumed > 0) {
				file.Read(buffer.data(), consumed);
			}
			const size_t remaining = totalSize - consumed;
			EnsureKadPayload(file, remaining, __FUNCTION__);
			ASSERT_RAISES(wxString, EnsureKadPayload(file, remaining + 1, __FUNCTION__));
		}
	}
}

TEST(KadHandlerFuzzTest, SlidingWindowPayloadChecks)
{
	RunPayloadWindowTest(32);
	RunPayloadWindowTest(128);
}

TEST(KadHandlerFuzzTest, FuzzTagCountsAcrossRange)
{
	for (uint32_t tags = 0; tags <= MAX_KAD_TAGS; ++tags) {
		EnsureKadTagCount(tags, __FUNCTION__);
	}
	ASSERT_RAISES(wxString, EnsureKadTagCount(MAX_KAD_TAGS + 1, __FUNCTION__));
	ASSERT_RAISES(wxString, EnsureKadTagCount(MAX_KAD_TAGS + 50, __FUNCTION__));
}
