// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <daNet/bitStream.h>
#include <util/dag_string.h>
#include <util/dag_simpleString.h>
#include <util/dag_preprocessor.h>
#include <EASTL/string.h>
#include <generic/dag_tab.h>
#include <EASTL/vector.h>

static uint32_t ones(uint32_t n) { return (1 << n) - 1; }

TEST_CASE("BitStream compressed read/write identity", "[bitstream][compressed]")
{
  // clang-format off
  const auto[a, sz] = GENERATE(table<uint32_t, uint32_t>({
      {ones(7),  1},
      {0x100,    2},
      {ones(14), 2},
      {ones(15), 3},
      {ones(21), 3},
      {ones(22), 4},
      {ones(28), 4},
      {0x100100, 3}
    }));
  // clang-format on

  danet::BitStream bs;
  bs.WriteCompressed(a);
  CHECK(bs.GetNumberOfBytesUsed() == sz);
  uint32_t b = 0;
  CHECK(bs.ReadCompressed(b));
  CHECK(a == b);
}

TEST_CASE("BitStream write compressed 32 bit max", "[bitstream][compressed]")
{
  danet::BitStream bs;
  const uint32_t v = ~0u;
  bs.WriteCompressed(v);
  uint32_t r = 0;
  CHECK(bs.ReadCompressed(r));
  CHECK(v == r);
}

TEST_CASE("BitStream 32-bit compressed all 16-bit values read/write identity", "[bitstream][compressed]")
{
  const uint32_t a = static_cast<uint32_t>(GENERATE(range(0, 1 << 16)));
  danet::BitStream bs;
  bs.WriteCompressed(a);
  uint32_t b = ~0u;
  CHECK(bs.ReadCompressed(b));
  CHECK(a == b);
}

TEST_CASE("Bit stream 16-bit compressed all 16-bit values read/write", "[bitstream][compressed]")
{
  const uint16_t a = static_cast<uint16_t>(GENERATE(range(0, 1 << 16)));

  danet::BitStream bs;
  bs.WriteCompressed(a);
  uint16_t b = a - 1;
  CHECK(bs.ReadCompressed(b));
  CHECK(a == b);
}

namespace eastl
{
inline std::ostream &operator<<(std::ostream &stream, const eastl::string &str) { return stream << str.c_str(); }
} // namespace eastl

TEST_CASE("Stack array string read/write", "[bitstream][string]")
{
  char foo[128];
  strncpy(foo, __FUNCTION__, sizeof(foo));
  eastl::string bar;
  danet::BitStream bs;
  bs.Write(foo);
  REQUIRE(bs.Read(bar));
  CHECK(strlen(foo) == bar.length());
  CHECK(foo == bar);
}

TEST_CASE("Test multiple strings read/write identity", "[bitstream][string]")
{
  danet::BitStream bs;

  eastl::string foo[4];
  for (int i = 0; i < countof(foo); ++i)
    foo[i] = eastl::string(eastl::string::CtorSprintf(), "%s%d", __FUNCTION__, i);
  bs.Write(foo);

  eastl::string bar[countof(foo)];
  REQUIRE(bs.Read(bar));

  for (int i = 0; i < countof(foo); ++i)
    CHECK(foo[i] == bar[i]);
}

TEMPLATE_TEST_CASE("Test various string types read/write identity", "[bitstream][string]", SimpleString, String, eastl::string)
{
  TestType str(__FUNCTION__);

  danet::BitStream bs;
  bs.Write(str);

  TestType str2;
  REQUIRE(bs.Read(str2));

  CHECK(str == str2);
}

TEMPLATE_TEST_CASE("Test various vector types read/write identity", "[bitstream]", Tab<int>, eastl::vector<int>, dag::Vector<int>)
{
  danet::BitStream bs;

  TestType arr;
  constexpr int n = 3;
  arr.resize(n);
  for (int i = 0; i < n; ++i)
    arr[i] = i;
  bs.Write(arr);

  TestType arr2;
  REQUIRE(bs.Read(arr2));

  CHECK(arr.size() == arr2.size());
  for (int i = 0; i < eastl::min(arr.size(), arr2.size()); ++i)
    CHECK(arr[i] == arr2[i]);
}

TEMPLATE_TEST_CASE("Test various vector of string types read/write identity", "[bitstream]", Tab<String>, eastl::vector<eastl::string>)
{
  danet::BitStream bs;

  TestType arr;
  constexpr int n = 4;
  arr.resize(n);
  char tmp[128];
  for (int i = 0; i < n; ++i)
  {
    snprintf(tmp, sizeof(tmp), "%s%d", __FUNCTION__, i);
    arr[i] = tmp;
  }
  bs.Write(arr);

  TestType arr2;
  REQUIRE(bs.Read(arr2));

  CHECK(arr.size() == arr2.size());
  for (int i = 0; i < eastl::min(arr.size(), arr2.size()); ++i)
    CHECK(arr[i] == arr2[i]);
}

TEST_CASE("RWBitAligned", "[bitstream]")
{
  const uint8_t zero = 0, one = 1, two = 2;
  int i = GENERATE(range(0, 8));

  danet::BitStream bs;
  bs.WriteBits(&zero, i);
  uint32_t pos = bs.GetWriteOffset();
  bs.WriteBits(&one, 2);
  bs.SetWriteOffset(pos);
  bs.WriteBits(&two, 2);

  uint8_t r = 0;
  bs.SetReadOffset(pos);
  G_VERIFY(bs.ReadBits(&r, 2));
  CHECK((int)two == (int)r);
}

TEST_CASE("RWBitAlignedByteSize", "[bitstream]")
{
  int j = GENERATE(range(0, 4));

  danet::BitStream bs;
  uint8_t a[2] = {0xff, 0xff};
  bs.WriteBits(a, 16);

  bs.SetWriteOffset(2);
  uint8_t b[2] = {0xcc, (uint8_t)j};
  bs.WriteBits(b, 16);

  uint8_t c[2] = {0xff, 0xff};
  bs.SetReadOffset(2);
  bs.ReadBits(c, 16);

  CHECK(0xcc == (int)c[0]);
  CHECK(j == (int)c[1]);
}

TEST_CASE("RWBitAlignedByteFit", "[bitstream]")
{
  int j = GENERATE(range(0, 4));

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

  CHECK(0xcc == (int)c[0]);
  CHECK(j == (int)c[1]);
  CHECK(bVal);
}

TEST_CASE("RWBitAlignedByteBorder", "[bitstream]")
{
  int j = GENERATE(range(0, 4));

  danet::BitStream bs;
  uint8_t a[2] = {0xff, 0xff};
  bs.WriteBits(a, 16);

  bs.SetWriteOffset(7);
  uint8_t b[2] = {0xcc, (uint8_t)j};
  bs.WriteBits(b, 10);

  uint8_t c[2] = {0xff, 0xff};
  bs.SetReadOffset(7);
  bs.ReadBits(c, 10);

  CHECK(0xcc == (int)c[0]);
  CHECK(j == (int)c[1]);
}
