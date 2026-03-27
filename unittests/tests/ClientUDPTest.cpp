#include <muleunit/test.h>

#include <ClientUDPGuards.h>

using namespace muleunit;

DECLARE_SIMPLE(ClientUDPTest);

TEST(ClientUDPTest, RejectsPayloadsOverCompressedLimit)
{
	const std::size_t limit = ClientUDPGuards::MAX_KAD_UNCOMPRESSED_PACKET;
	ASSERT_TRUE(ClientUDPGuards::IsCompressedPayloadValid(limit));
	ASSERT_FALSE(ClientUDPGuards::IsCompressedPayloadValid(limit + 1));
}

TEST(ClientUDPTest, RejectsPayloadsOverUncompressedLimit)
{
	const std::size_t limit = ClientUDPGuards::MAX_KAD_UNCOMPRESSED_PACKET;
	ASSERT_TRUE(ClientUDPGuards::IsUncompressedPayloadValid(limit));
	ASSERT_FALSE(ClientUDPGuards::IsUncompressedPayloadValid(limit + 32));
}
