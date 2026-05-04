// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/rle.h>
#include <debug/dag_assert.h>
#include <daNet/daNetTypes.h>

#include <util/dag_lag.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>

namespace net
{

namespace delta
{

static const int TWO_BITS_SHIFT = 2;
static const int TWO_BITS_MASK = 3;
static const int SIX_BITS_MASK = 63;
static const int ZR0_BASE_VAL = 3;
static const int ZR1_BASE_VAL = 7;
static const int MIN_UNITS_NUM = 3;
static const int MAX_ZEROS_NUM = 10;

static const int RLE0_PARITY_NO = 111;
static const int RLE0_PARITY_YES = 222;
static const int RLE0_ZERO = 0;
static const int RLE0_FOUR = 4;
static const int RLE0_ZR0 = 0;
static const int RLE0_ZR1 = 3;
static const int RLE0_KI2 = 2;
static const int RLE0_KI6 = 1;
static const int RLE0_KI8 = 4;

//
// this is rather bit algorithm than byte one
// that allows to work on relatively short data about 50 bytes
//
// the operating unit size is two bits
// the main idea is to compress zeros only so it requires many zeros about 50-80%
//
// such amount of zeros is considered to be got as result of xor'ed diff
//
// the number of units is coded into other two bits
// so we have 4 bit codes:
//[zr0, N] for sequence of 3 - 6 zero units
//[zr1, N] for sequence of 7 - 10 zero units
//
// other unit values are kept intact
// so we have 8 bit code for 6 bits of data:
//[ki6, unit1, unit2, unit3] for sequence of 3 non-zero units
// and 4 bit code for 2 bits of data:
//[ki2, unit1] mostly for remainder units
//
// best regular compression is 5:1
//
int rle0ki_compress(dag::Span<uint8_t> dstSlice, dag::ConstSpan<uint8_t> srcSlice)
{
  uint8_t *dst = dstSlice.data();
  int maxDstSize = dstSlice.size();
  const uint8_t *src = srcSlice.data();
  int srcSize = srcSlice.size();

  int accumVal = 0;
  int accumLen = 0;
  int zeroCount = 0;

  if (srcSize == 0)
    return 0;
  if (srcSize < 0)
    return -1;

  int val = 0;

  int offset = 0;
  uint8_t *out = dst;
  if (maxDstSize > 0)
    *dst = 0;
  else
    return -1;

  for (int i = 0; i < srcSize; ++i)
  {
    for (int k = 0; k < RLE0_FOUR; ++k)
    {
      int next = (*src >> (TWO_BITS_SHIFT * k)) & TWO_BITS_MASK;

      if (accumLen >= MIN_UNITS_NUM * TWO_BITS_SHIFT)
      {
        int num = 0;
        int data = 0;
        int current = accumVal & SIX_BITS_MASK;

        switch (current)
        {
            // serie
          case 0: // 00 00 00

            if (next != val || zeroCount == MAX_ZEROS_NUM)
            {
              data = zeroCount < ZR1_BASE_VAL ? RLE0_ZR0 : RLE0_ZR1;
              data |= (zeroCount - (data == RLE0_ZR0 ? ZR0_BASE_VAL : ZR1_BASE_VAL)) << TWO_BITS_SHIFT;
              num = 1;
              accumVal >>= zeroCount * TWO_BITS_SHIFT;
              accumLen -= zeroCount * TWO_BITS_SHIFT;
              zeroCount = 0;
            }
            break;

            // ki2
          case 1: // 00 00 01
          case 2: // 00 00 10
          case 3: // 00 00 11

            if (next == val)
            {
              data = RLE0_KI2 | current << TWO_BITS_SHIFT;
              num = 1;
              accumVal >>= TWO_BITS_SHIFT;
              accumLen -= TWO_BITS_SHIFT;
              break;
            }
            [[fallthrough]];

            // ki6
          default:

            data = RLE0_KI6 | current << TWO_BITS_SHIFT;
            num = 2;
            accumVal = 0;
            accumLen = 0;
            zeroCount = 0;
        }

        if (num)
        {
          *dst |= data << offset;
          if (offset == RLE0_FOUR || num == 2)
          {
            if (++dst - out < maxDstSize)
              *dst = offset == RLE0_FOUR && num == 2 ? data >> RLE0_FOUR : 0;
          }
          if (num == 1)
            offset = RLE0_FOUR - offset;
        }
      }
      // add to accumulator
      val = next;
      accumVal |= val << accumLen;
      accumLen += TWO_BITS_SHIFT;
      zeroCount += !val;
    }
    ++src;
  }

  // last iteration similar to the used in the cycle above
  int pairs = accumLen / TWO_BITS_SHIFT;
  if (pairs >= MIN_UNITS_NUM)
  {
    int num = 0;
    int data = 0;
    int current = accumVal & SIX_BITS_MASK;

    if (current)
    {
      data = RLE0_KI6 | current << TWO_BITS_SHIFT;
      num = 2;
    }
    else
    {
      data = zeroCount < ZR1_BASE_VAL ? RLE0_ZR0 : RLE0_ZR1;
      data |= (zeroCount - (data == RLE0_ZR0 ? ZR0_BASE_VAL : ZR1_BASE_VAL)) << TWO_BITS_SHIFT;
      num = 1;
    }

    *dst |= data << offset;
    if (offset == RLE0_FOUR || num == 2)
    {
      if (++dst - out < maxDstSize)
        *dst = offset == RLE0_FOUR && num == 2 ? data >> RLE0_FOUR : 0;
    }
    if (num == 1)
      offset = RLE0_FOUR - offset;
  }
  else // remainder
  {
    for (int n = 0; n < pairs; ++n)
    {
      val = accumVal & TWO_BITS_MASK;
      accumVal >>= TWO_BITS_SHIFT;
      *dst |= (RLE0_KI2 << offset) | (val << (TWO_BITS_SHIFT + offset));
      if (offset == RLE0_FOUR)
      {
        if (++dst - out < maxDstSize)
          *dst = 0;
      }
      offset = RLE0_FOUR - offset;
    }
  }
  if (offset)
    ++dst;
  if (dst - out >= maxDstSize)
    return -1;

  *dst++ = offset == RLE0_ZERO ? RLE0_PARITY_YES : RLE0_PARITY_NO;
  if (dst - out > maxDstSize)
    return -1;

  return dst - out;
}


int rle0ki_decompress(dag::Span<uint8_t> dstSlice, dag::ConstSpan<uint8_t> srcSlice)
{
  uint8_t *dst = dstSlice.data();
  int maxOriginalSize = dstSlice.size();
  const uint8_t *src = srcSlice.data();
  int compressedSize = srcSlice.size();

  if (compressedSize == 0)
    return 0;
  int limit = compressedSize - 1;
  if (limit < 0)
    return -1;

  uint8_t parity = src[limit];
  if (parity != RLE0_PARITY_NO && parity != RLE0_PARITY_YES)
    return 0;

  int val[] = {0, 0};
  int code = -1;
  int offset = RLE0_ZERO;
  uint8_t *out = dst;
  if (maxOriginalSize > 0)
    *dst = 0;
  else
    return -1;

  int super = -1;
  for (int i = 0; i < limit; ++i)
  {
    int klim = i != limit - 1 || parity == RLE0_PARITY_YES ? 2 : 1;
    for (int k = 0; k < klim; ++k)
    {
      val[0] = (*src >> (k * RLE0_FOUR)) & TWO_BITS_MASK;
      val[1] = (*src >> (TWO_BITS_SHIFT + k * RLE0_FOUR)) & TWO_BITS_MASK;
      int num = 0;

      if (code != RLE0_KI8)
        code = val[0];

      switch (code)
      {
        case RLE0_ZR0:
          num = val[1] + ZR0_BASE_VAL;
          val[0] = val[1] = 0;
          break;

        case RLE0_ZR1:
          num = val[1] + ZR1_BASE_VAL;
          val[0] = val[1] = 0;
          break;

        case RLE0_KI6:
          code = RLE0_KI8;
          super = val[1];
          [[fallthrough]];

        case RLE0_KI2:
          num = 1;
          val[0] = val[1];
          break;

        case RLE0_KI8:
          code = -1;
          num = 2;
          if (super == 0 && super == val[0] && val[0] == val[1])
            num = 39;
          break;
      }

      for (int n = 0; n < num; ++n)
      {
        *dst |= val[n & 1] << (offset * TWO_BITS_SHIFT);
        ++offset;
        if (offset == RLE0_FOUR)
        {
          offset = RLE0_ZERO;
          ++dst;
          int len = dst - out;
          if (len < maxOriginalSize)
            *dst = 0;
          else if (len > maxOriginalSize)
            return -1;
        }
      }
    }
    ++src;
  }

  if (offset)
    ++dst;
  if (dst - out > maxOriginalSize)
    return -1;
  return dst - out;
}

#define BEGIN_PROF(x) \
  BEGIN_LAG(x);       \
  {                   \
    TIME_PROFILE_DEV(x);

#define END_PROF(x) \
  }                 \
  END_LAG(x);

#define PROFILE_CODE(x, code) \
  BEGIN_PROF(x)               \
  code;                       \
  END_PROF(x)

void compress(const danet::BitStream &in, danet::BitStream &out)
{
  int maxSize = in.GetNumberOfBytesUsed() * 2; // compression can't be worse than 1:2
  out.SetWriteOffset(0);
  out.reserveBits(BYTES_TO_BITS(maxSize));
  int len = -1;
  dag::Span<uint8_t> dest(out.GetData(), maxSize);
  PROFILE_CODE(rle0ki_compress, len = rle0ki_compress(dest, in.getSlice()));
  if (len >= 0)
    out.SetWriteOffset(BYTES_TO_BITS(len));
  else
  {
    logerr("rle0ki_compress has been failed on a stream of size %d", in.GetNumberOfBytesUsed());

    danet::BitStream empty;
    out.swap(empty);
  }
}

void decompress(const danet::BitStream &in, danet::BitStream &out)
{
  int maxSize = in.GetNumberOfBytesUsed() * 10; // compression can't be better than 10:1
  out.SetWriteOffset(0);
  out.reserveBits(BYTES_TO_BITS(maxSize));
  int len = -1;
  dag::Span<uint8_t> dest(out.GetData(), maxSize);
  PROFILE_CODE(rle0ki_decompress, len = rle0ki_decompress(dest, in.getSlice()));
  G_ASSERT(len != -1);
  out.SetWriteOffset(BYTES_TO_BITS(len));
}

} // namespace delta

} // namespace net
