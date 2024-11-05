// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <UnitTest++/UnitTestPP.h>
#include <daNet/bitStream.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_preprocessor.h>
#include <EASTL/string.h>
#include <generic/dag_tab.h>
#include <EASTL/vector.h>

#define DECL_BITSTREAM_COMPRESSED_TEST(name, num, sz)   \
  TEST(DAG_CONCAT(CheckBitStreamWriteCompressed, name)) \
  {                                                     \
    danet::BitStream bs;                                \
    uint32_t a = num, b = 0;                            \
    bs.WriteCompressed(a);                              \
    CHECK(bs.GetNumberOfBytesUsed() == sz);             \
    CHECK(bs.ReadCompressed(b));                        \
    CHECK_EQUAL(a, b);                                  \
  }

DECL_BITSTREAM_COMPRESSED_TEST(7Bit, (1 << 7) - 1, 1)
DECL_BITSTREAM_COMPRESSED_TEST(100, 0x100, 2)
DECL_BITSTREAM_COMPRESSED_TEST(14Bit, (1 << 14) - 1, 2)
DECL_BITSTREAM_COMPRESSED_TEST(15Bit, (1 << 15) - 1, 3)
DECL_BITSTREAM_COMPRESSED_TEST(21Bit, (1 << 21) - 1, 3)
DECL_BITSTREAM_COMPRESSED_TEST(22Bit, (1 << 22) - 1, 4)
DECL_BITSTREAM_COMPRESSED_TEST(28Bit, (1 << 28) - 1, 4)
DECL_BITSTREAM_COMPRESSED_TEST(100100, 0x100100, 3)

TEST(CheckBitStreamWriteCompressed32BitMax)
{
  danet::BitStream bs;
  uint32_t v = ~0u, r = 0;
  bs.WriteCompressed(v);
  CHECK(bs.ReadCompressed(r));
  CHECK_EQUAL(v, r);
}

TEST(CheckBitStreamWriteCompressed3216BitAll)
{
  for (unsigned a = 0; a < (1 << 16); ++a)
  {
    danet::BitStream bs;
    bs.WriteCompressed(a);
    unsigned b = ~0u;
    CHECK(bs.ReadCompressed(b));
    CHECK_EQUAL(a, b);
  }
}

TEST(CheckBitStreamWriteCompressed1616BitAll)
{
  for (uint16_t a = 0;; ++a)
  {
    danet::BitStream bs;
    bs.WriteCompressed(a);
    uint16_t b = a - 1;
    CHECK(bs.ReadCompressed(b));
    CHECK_EQUAL(a, b);
    if (a == uint16_t(-1))
      break;
  }
}

namespace eastl
{
inline std::ostream &operator<<(std::ostream &stream, const eastl::string &str) { return stream << str.c_str(); }
} // namespace eastl
template <typename S>
static void test_string_impl()
{
  S str(__FUNCTION__), str2;
  danet::BitStream bs;
  bs.Write(str);
  G_VERIFY(bs.Read(str2));
  CHECK_EQUAL(str.length(), str2.length());
  CHECK_EQUAL(str, str2);
}
TEST(sting_arr)
{
  char foo[128];
  strncpy(foo, __FUNCTION__, sizeof(foo));
  eastl::string bar;
  danet::BitStream bs;
  bs.Write(foo);
  G_VERIFY(bs.Read(bar));
  CHECK_EQUAL(strlen(foo), bar.length());
  CHECK_EQUAL(foo, bar);
}
TEST(sting_arr_raw)
{
  eastl::string foo[4], bar[countof(foo)];
  for (int i = 0; i < countof(foo); ++i)
    foo[i] = eastl::string(eastl::string::CtorSprintf(), "%s%d", __FUNCTION__, i);
  danet::BitStream bs;
  bs.Write(foo);
  G_VERIFY(bs.Read(bar));
  for (int i = 0; i < countof(foo); ++i)
    CHECK_EQUAL(foo[i], bar[i]);
}
TEST(String) { test_string_impl<String>(); }
TEST(SimpleString) { test_string_impl<SimpleString>(); }
TEST(ESString) { test_string_impl<eastl::string>(); }

template <typename T>
static void test_arr_int()
{
  T arr, arr2;
  constexpr int n = 3;
  arr.resize(n);
  for (int i = 0; i < n; ++i)
    arr[i] = i;
  danet::BitStream bs;
  bs.Write(arr);
  G_VERIFY(bs.Read(arr2));
  CHECK_EQUAL(arr.size(), arr2.size());
  for (int i = 0; i < arr.size(); ++i)
    CHECK_EQUAL(arr[i], arr2[i]);
}
TEST(TabInt) { test_arr_int<Tab<int>>(); }
TEST(DynTabInt) { test_arr_int<Tab<int>>(); }
TEST(EAVecInt) { test_arr_int<eastl::vector<int>>(); }

template <typename T>
static void test_arr_str()
{
  T arr, arr2;
  constexpr int n = 4;
  arr.resize(n);
  char tmp[128];
  for (int i = 0; i < n; ++i)
  {
    snprintf(tmp, sizeof(tmp), "%s%d", __FUNCTION__, i);
    arr[i] = tmp;
  }
  danet::BitStream bs;
  bs.Write(arr);
  G_VERIFY(bs.Read(arr2));
  CHECK_EQUAL(arr.size(), arr2.size());
  for (int i = 0; i < arr.size(); ++i)
    CHECK_EQUAL(arr[i], arr2[i]);
}
TEST(DynTabStr) { test_arr_str<Tab<String>>(); }
TEST(EAVecStr) { test_arr_str<eastl::vector<eastl::string>>(); }

TEST(RWBitAligned)
{
  uint8_t zero = 0, one = 1, two = 2;
  for (int i = 0; i < 8; ++i)
  {
    danet::BitStream bs;
    bs.WriteBits(&zero, i);
    uint32_t pos = bs.GetWriteOffset();
    bs.WriteBits(&one, 2);
    bs.SetWriteOffset(pos);
    bs.WriteBits(&two, 2);

    uint8_t r = 0;
    bs.SetReadOffset(pos);
    G_VERIFY(bs.ReadBits(&r, 2));
    CHECK_EQUAL((int)two, (int)r);
  }
}

TEST(RWBitAlignedByteSize)
{
  for (int j = 0; j < 4; ++j)
  {
    danet::BitStream bs;
    uint8_t a[2] = {0xff, 0xff};
    bs.WriteBits(a, 16);

    bs.SetWriteOffset(2);
    uint8_t b[2] = {0xcc, (uint8_t)j};
    bs.WriteBits(b, 16);

    uint8_t c[2] = {0xff, 0xff};
    bs.SetReadOffset(2);
    bs.ReadBits(c, 16);

    CHECK_EQUAL(0xcc, (int)c[0]);
    CHECK_EQUAL(j, (int)c[1]);
  }
}

TEST(RWBitAlignedByteFit)
{
  for (int j = 0; j < 4; ++j)
  {
    danet::BitStream bs;
    uint8_t a[2] = {0xff, 0xff};
    bs.WriteBits(a, 16);

    bs.SetWriteOffset(5);
    uint8_t b[2] = {0xcc, (uint8_t)j};
    bs.WriteBits(b, 10);
    bs.SetWriteOffset(16);

    uint8_t c[2] = {0xff, 0xff};
    bs.SetReadOffset(5);
    bs.ReadBits(c, 10);
    bool bVal = bs.ReadBit();

    CHECK_EQUAL(0xcc, (int)c[0]);
    CHECK_EQUAL(j, (int)c[1]);
    CHECK_EQUAL(true, bVal);
  }
}

TEST(RWBitAlignedByteBorder)
{
  for (int j = 0; j < 4; ++j)
  {
    danet::BitStream bs;
    uint8_t a[2] = {0xff, 0xff};
    bs.WriteBits(a, 16);

    bs.SetWriteOffset(7);
    uint8_t b[2] = {0xcc, (uint8_t)j};
    bs.WriteBits(b, 10);

    uint8_t c[2] = {0xff, 0xff};
    bs.SetReadOffset(7);
    bs.ReadBits(c, 10);

    CHECK_EQUAL(0xcc, (int)c[0]);
    CHECK_EQUAL(j, (int)c[1]);
  }
}
