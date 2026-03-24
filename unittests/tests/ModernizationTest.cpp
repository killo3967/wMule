#include <muleunit/test.h>
#include <Types.h>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>

using namespace muleunit;

DECLARE_SIMPLE(TypesAliasTest);

TEST(TypesAliasTest, UnsignedTypesExist)
{
 ASSERT_EQUALS(sizeof(uint8), 1u);
 ASSERT_EQUALS(sizeof(uint16), 2u);
 ASSERT_EQUALS(sizeof(uint32), 4u);
 ASSERT_EQUALS(sizeof(uint64), 8u);
 ASSERT_EQUALS(sizeof(int8), 1u);
 ASSERT_EQUALS(sizeof(int16), 2u);
 ASSERT_EQUALS(sizeof(int32), 4u);
 ASSERT_EQUALS(sizeof(int64), 8u);
}

TEST(TypesAliasTest, SignedTypesExist)
{
 ASSERT_EQUALS(sizeof(sint8), 1u);
 ASSERT_EQUALS(sizeof(sint16), 2u);
 ASSERT_EQUALS(sizeof(sint32), 4u);
 ASSERT_EQUALS(sizeof(sint64), 8u);
}

TEST(TypesAliasTest, UnsignedTypesCanHoldExpectedRanges)
{
 ASSERT_TRUE(std::numeric_limits<uint8>::max() == 255);
 ASSERT_TRUE(std::numeric_limits<uint16>::max() == 65535);
 ASSERT_TRUE(std::numeric_limits<uint32>::max() == 4294967295u);
 ASSERT_TRUE(std::numeric_limits<uint64>::max() == 18446744073709551615ull);
}

TEST(TypesAliasTest, SignedTypesCanHoldExpectedRanges)
{
 ASSERT_TRUE(std::numeric_limits<sint8>::min() == -128);
 ASSERT_TRUE(std::numeric_limits<sint8>::max() == 127);
 ASSERT_TRUE(std::numeric_limits<sint16>::min() == -32768);
 ASSERT_TRUE(std::numeric_limits<sint16>::max() == 32767);
 ASSERT_TRUE(std::numeric_limits<sint32>::min() == -2147483648);
 ASSERT_TRUE(std::numeric_limits<sint32>::max() == 2147483647);
 ASSERT_TRUE(std::numeric_limits<sint64>::min() == -9223372036854775807ll - 1);
 ASSERT_TRUE(std::numeric_limits<sint64>::max() == 9223372036854775807ll);
}

TEST(TypesAliasTest, TypeAliasesWorkInVariables)
{
 uint8 u8 = 255;
 uint16 u16 = 65535;
 uint32 u32 = 4294967295u;
 uint64 u64 = 18446744073709551615ull;
 int8 i8 = 127;
 int16 i16 = 32767;
 int32 i32 = 2147483647;
 int64 i64 = 9223372036854775807ll;

 ASSERT_EQUALS(u8, 255u);
 ASSERT_EQUALS(u16, 65535u);
 ASSERT_EQUALS(u32, 4294967295u);
 ASSERT_EQUALS(u64, 18446744073709551615ull);
 ASSERT_EQUALS(i8, 127);
 ASSERT_EQUALS(i16, 32767);
 ASSERT_EQUALS(i32, 2147483647);
 ASSERT_EQUALS(i64, 9223372036854775807ll);
}

TEST(TypesAliasTest, SignedTypeAliasesWorkInVariables)
{
 sint8 s8 = 127;
 sint16 s16 = 32767;
 sint32 s32 = 2147483647;
 sint64 s64 = 9223372036854775807ll;

 ASSERT_EQUALS(s8, 127);
 ASSERT_EQUALS(s16, 32767);
 ASSERT_EQUALS(s32, 2147483647);
 ASSERT_EQUALS(s64, 9223372036854775807ll);
}

TEST(TypesAliasTest, ArrayTypesWork)
{
 ArrayOfUInts8 arr8;
 arr8.push_back(255);
 ASSERT_EQUALS(arr8[0], 255u);

 ArrayOfUInts16 arr16;
 arr16.push_back(65535);
 ASSERT_EQUALS(arr16[0], 65535u);

 ArrayOfUInts32 arr32;
 arr32.push_back(4294967295u);
 ASSERT_EQUALS(arr32[0], 4294967295u);

 ArrayOfUInts64 arr64;
 arr64.push_back(18446744073709551615ull);
 ASSERT_EQUALS(arr64[0], 18446744073709551615ull);
}

TEST(TypesAliasTest, ListTypesWork)
{
 ListOfUInts32 list;
 list.push_back(100);
 list.push_back(200);
 ASSERT_EQUALS(list.size(), 2u);
}


DECLARE_SIMPLE(BoolModernizationTest);

TEST(BoolModernizationTest, TrueIsTrue)
{
 bool b = true;
 ASSERT_TRUE(b == true);
 ASSERT_TRUE(b);
 ASSERT_TRUE(b != false);
 ASSERT_TRUE(b != 0);
}

TEST(BoolModernizationTest, FalseIsFalse)
{
 bool b = false;
 ASSERT_TRUE(b == false);
 ASSERT_TRUE(!b);
 ASSERT_TRUE(b != true);
 ASSERT_TRUE(b == 0);
}

TEST(BoolModernizationTest, BoolNegation)
{
 bool t = true;
 bool f = false;
 ASSERT_TRUE(!t == f);
 ASSERT_TRUE(!f == t);
}

TEST(BoolModernizationTest, BoolAnd)
{
 ASSERT_TRUE(true && true);
 ASSERT_TRUE(!(true && false));
 ASSERT_TRUE(!(false && true));
 ASSERT_TRUE(!(false && false));
}

TEST(BoolModernizationTest, BoolOr)
{
 ASSERT_TRUE(true || true);
 ASSERT_TRUE(true || false);
 ASSERT_TRUE(false || true);
 ASSERT_TRUE(!(false || false));
}

TEST(BoolModernizationTest, BoolXor)
{
 ASSERT_TRUE(true ^ false);
 ASSERT_TRUE(false ^ true);
 ASSERT_TRUE(!(true ^ true));
 ASSERT_TRUE(!(false ^ false));
}

TEST(BoolModernizationTest, BoolEquality)
{
 bool a = true;
 bool b = true;
 bool c = false;
 ASSERT_TRUE(a == b);
 ASSERT_TRUE(a != c);
 ASSERT_TRUE(b != c);
}


DECLARE_SIMPLE(NullptrModernizationTest);

TEST(NullptrModernizationTest, NullptrIsNullPointer)
{
 int* p = nullptr;
 ASSERT_TRUE(p == nullptr);
 ASSERT_TRUE(!p);
}

TEST(NullptrModernizationTest, NullptrComparisons)
{
 int* p1 = nullptr;
 int* p2 = nullptr;
 ASSERT_TRUE(p1 == p2);
 ASSERT_TRUE(p1 == nullptr);
 ASSERT_TRUE(nullptr == p1);
}

TEST(NullptrModernizationTest, NullptrNotEqualToValidPointer)
{
 int value = 42;
 int* p = &value;
 ASSERT_TRUE(p != nullptr);
 ASSERT_TRUE(nullptr != p);
}

TEST(NullptrModernizationTest, NullptrCanBeAssigned)
{
 int* p = nullptr;
 ASSERT_TRUE(p == nullptr);
 int value = 42;
 p = &value;
 ASSERT_TRUE(p != nullptr);
 ASSERT_TRUE(*p == 42);
 p = nullptr;
 ASSERT_TRUE(p == nullptr);
}

TEST(NullptrModernizationTest, NullptrInConditional)
{
 int* p = nullptr;
 if (p) {
  ASSERT_TRUE(false);
 }
 p = new int(42);
 if (!p) {
  ASSERT_TRUE(false);
 }
 delete p;
}

TEST(NullptrModernizationTest, NullptrWorksWithSmartPointers)
{
 std::unique_ptr<int> up1;
 std::unique_ptr<int> up2(nullptr);
 ASSERT_TRUE(up1.get() == nullptr);
 ASSERT_TRUE(up2.get() == nullptr);
}


DECLARE_SIMPLE(StringLiteralTest);

TEST(StringLiteralTest, SimpleStringLiterals)
{
 const char* s = "Hello, World!";
 ASSERT_TRUE(s != nullptr);
 ASSERT_TRUE(strlen(s) > 0);
}

TEST(StringLiteralTest, EmptyStringLiterals)
{
 const char* s = "";
 ASSERT_TRUE(s != nullptr);
 ASSERT_EQUALS(strlen(s), 0u);
}

TEST(StringLiteralTest, StringLiteralComparison)
{
 const char* s1 = "test";
 const char* s2 = "test";
 ASSERT_TRUE(strcmp(s1, s2) == 0);
}

TEST(StringLiteralTest, StringLiteralInequality)
{
 const char* s1 = "abc";
 const char* s2 = "xyz";
 ASSERT_TRUE(strcmp(s1, s2) != 0);
}


DECLARE_SIMPLE(StaticAssertTest);

TEST(StaticAssertTest, TypeSizesVerifiedAtCompileTime)
{
 ASSERT_EQUALS(sizeof(int8), sizeof(int8_t));
 ASSERT_EQUALS(sizeof(uint8), sizeof(uint8_t));
 ASSERT_EQUALS(sizeof(int16), sizeof(int16_t));
 ASSERT_EQUALS(sizeof(uint16), sizeof(uint16_t));
 ASSERT_EQUALS(sizeof(int32), sizeof(int32_t));
 ASSERT_EQUALS(sizeof(uint32), sizeof(uint32_t));
 ASSERT_EQUALS(sizeof(int64), sizeof(int64_t));
 ASSERT_EQUALS(sizeof(uint64), sizeof(uint64_t));
}

TEST(StaticAssertTest, SignedUnsignedMatch)
{
  ASSERT_EQUALS(sizeof(sint8), sizeof(int8_t));
  ASSERT_EQUALS(sizeof(sint16), sizeof(int16_t));
  ASSERT_EQUALS(sizeof(sint32), sizeof(int32_t));
  ASSERT_EQUALS(sizeof(sint64), sizeof(int64_t));
}

TEST(StaticAssertTest, UsingDeclarationsEquivalentToTypedef)
{
  int8 i8a = 127;
  uint8 u8a = 255;
  int16 i16a = 32767;
  uint16 u16a = 65535;
  int32 i32a = 2147483647;
  uint32 u32a = 4294967295u;
  int64 i64a = 9223372036854775807ll;
  uint64 u64a = 18446744073709551615ull;

  ASSERT_EQUALS(sizeof(i8a), sizeof(int8_t));
  ASSERT_EQUALS(sizeof(u8a), sizeof(uint8_t));
  ASSERT_EQUALS(sizeof(i16a), sizeof(int16_t));
  ASSERT_EQUALS(sizeof(u16a), sizeof(uint16_t));
  ASSERT_EQUALS(sizeof(i32a), sizeof(int32_t));
  ASSERT_EQUALS(sizeof(u32a), sizeof(uint32_t));
  ASSERT_EQUALS(sizeof(i64a), sizeof(int64_t));
  ASSERT_EQUALS(sizeof(u64a), sizeof(uint64_t));
}

TEST(StaticAssertTest, ArrayContainerTypesWork)
{
  ArrayOfUInts8 arr8;
  ArrayOfUInts16 arr16;
  ArrayOfUInts32 arr32;
  ArrayOfUInts64 arr64;

  arr8.push_back(255);
  arr16.push_back(65535);
  arr32.push_back(4294967295u);
  arr64.push_back(18446744073709551615ull);

  ASSERT_EQUALS(arr8.size(), 1u);
  ASSERT_EQUALS(arr16.size(), 1u);
  ASSERT_EQUALS(arr32.size(), 1u);
  ASSERT_EQUALS(arr64.size(), 1u);

  ASSERT_EQUALS(arr8[0], 255u);
  ASSERT_EQUALS(arr16[0], 65535u);
  ASSERT_EQUALS(arr32[0], 4294967295u);
  ASSERT_EQUALS(arr64[0], 18446744073709551615ull);
}

TEST(StaticAssertTest, ListContainerTypeWorks)
{
  ListOfUInts32 list;
  list.push_back(100);
  list.push_back(200);
  ASSERT_EQUALS(list.size(), 2u);
}
