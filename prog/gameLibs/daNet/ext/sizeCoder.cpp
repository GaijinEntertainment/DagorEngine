
#include "sizeCoder.h"

#include <daNet/bitStream.h>

#include <debug/dag_assert.h>

namespace danet
{

void writeSize(BitStream &to, uint32_t size_in_bits)
{
  G_ASSERT(size_in_bits);
  uint8_t hdr = 0;

  switch (size_in_bits)
  {
    case 1:
      hdr = 1; // 1 bit
      break;
    case 8:
      hdr = 2; // 1 byte
      break;
    case 16:
      hdr = 3; // 2 bytes
      break;
    case 32:
      hdr = 4; // 4 bytes
      break;
    case 64:
      hdr = 5; // 8 bytes
      break;
    case 96:
      hdr = 6; // 12 bytes
      break;
    case 128:
      hdr = 7; // 16 bytes
      break;
    default: hdr = 0; // var int bits
  }

  to.WriteBits(&hdr, 3);
  if (hdr > 0)
    return;

  to.WriteCompressed(size_in_bits);
}

uint32_t readSize(const BitStream &from)
{
  bool ok = true;
  uint8_t hdr = 0;

  ok &= from.ReadBits(&hdr, 3);
  if (!ok)
    return 0;

  switch (hdr)
  {
    case 1: return 1;   // 1 bit
    case 2: return 8;   // 1 byte
    case 3: return 16;  // 2 bytes
    case 4: return 32;  // 4 bytes
    case 5: return 64;  // 6 bytes
    case 6: return 96;  // 12 bytes
    case 7: return 128; // 16 bytes
  }

  G_ASSERT(hdr == 0);
  uint32_t sizeInBits = 0;
  ok &= from.ReadCompressed(sizeInBits);
  return ok ? sizeInBits : 0;
}

} // namespace danet
