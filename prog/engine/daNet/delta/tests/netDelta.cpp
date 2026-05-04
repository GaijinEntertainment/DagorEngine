// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <UnitTest++/UnitTestPP.h>
#include <daNet/delta/rle.h>
#include <util/dag_preprocessor.h>

#define DECL_NETDELTA_COMPRESSION_TEST(name, num, sz)                                                                                 \
  TEST(DAG_CONCAT(CheckNetDeltaCompression, name))                                                                                    \
  {                                                                                                                                   \
    uint64_t compressed;                                                                                                              \
    uint32_t a = num, b = 0;                                                                                                          \
    int res = net::delta::rle0ki_compress(make_span((uint8_t *)&compressed, sizeof(compressed)), make_span_const((uint8_t *)&a, sz)); \
    CHECK(res != -1);                                                                                                                 \
    int len = net::delta::rle0ki_decompress(make_span((uint8_t *)&b, sizeof(b)), make_span_const((uint8_t *)&compressed, res));       \
    CHECK(len == sz);                                                                                                                 \
    CHECK_EQUAL(a, b);                                                                                                                \
  }

DECL_NETDELTA_COMPRESSION_TEST(564560008IV, 564560008, 4)
DECL_NETDELTA_COMPRESSION_TEST(3000000001IV, 3000000001, 4)
DECL_NETDELTA_COMPRESSION_TEST(767232024IV, 767232024, 4)
DECL_NETDELTA_COMPRESSION_TEST(460095653IV, 460095653, 4)
DECL_NETDELTA_COMPRESSION_TEST(3428003III, 3428003, 3)
DECL_NETDELTA_COMPRESSION_TEST(3428003IV, 3428003, 4)
DECL_NETDELTA_COMPRESSION_TEST(0, 0, 0)
DECL_NETDELTA_COMPRESSION_TEST(0I, 0, 1)
DECL_NETDELTA_COMPRESSION_TEST(0II, 0, 2)
DECL_NETDELTA_COMPRESSION_TEST(0III, 0, 3)
DECL_NETDELTA_COMPRESSION_TEST(0IV, 0, 4)
DECL_NETDELTA_COMPRESSION_TEST(255I, 255, 1)
DECL_NETDELTA_COMPRESSION_TEST(255II, 255, 2)
DECL_NETDELTA_COMPRESSION_TEST(255III, 255, 3)
DECL_NETDELTA_COMPRESSION_TEST(255IV, 255, 4)

TEST(CheckNetDeltaCompressionDecompressBad)
{
  uint16_t bad = 0, r = 0;
  int len = net::delta::rle0ki_decompress(make_span((uint8_t *)&r, sizeof(r)), make_span_const((const uint8_t *)&bad, sizeof(bad)));
  CHECK(len == 0);
}

TEST(CheckNetDeltaCompressionDecompressOdd)
{
  uint16_t odd = 111 << 8 /*shift for little endian*/, r = 0;
  int len = net::delta::rle0ki_decompress(make_span((uint8_t *)&r, sizeof(r)), make_span_const((const uint8_t *)&odd, sizeof(odd)));
  CHECK(len == 1);
  CHECK(r == 0);
}

TEST(CheckNetDeltaCompressionDecompressEven)
{
  uint16_t even = 222 << 8 /*shift for little endian*/, r = 0;
  int len = net::delta::rle0ki_decompress(make_span((uint8_t *)&r, sizeof(r)), make_span_const((const uint8_t *)&even, sizeof(even)));
  CHECK(len == 2);
  CHECK(r == 0);
}
