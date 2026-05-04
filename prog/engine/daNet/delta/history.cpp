// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/delta/history.h>

#include "diff_impl.h"
#include <daNet/delta/rle.h>

namespace net
{

namespace delta
{

void History::store(int packet_num, const danet::BitStream &bin)
{
  danet::BitStream &to = pack[basePacketNo(packet_num)];
  to.SetWriteOffset(0);
  to.reserveBits(bin.GetWriteOffset());
  to.SetWriteOffset(bin.GetWriteOffset());
  memcpy(to.GetData(), bin.GetData(), BITS_TO_BYTES(bin.GetWriteOffset()));
}

danet::BitStream History::getDiff(int packet_num, const danet::BitStream &new_ver) const
{
  return diff_impl(pack[basePacketNo(packet_num)], new_ver);
}

danet::BitStream History::applyPatch(int packet_num, const danet::BitStream &delta) const
{
  return diff_impl(pack[basePacketNo(packet_num)], delta);
}

danet::BitStream get_compressed_delta(const danet::BitStream &base_version, const danet::BitStream &new_ver)
{
  danet::BitStream diff = diff_impl(base_version, new_ver);
  diff.SetWriteOffset(new_ver.GetWriteOffset());
  diff.AlignWriteToByteBoundary();
  danet::BitStream compressed(framemem_ptr());
  net::delta::compress(diff, compressed);
  return compressed;
}

danet::BitStream apply_compressed_patch(const danet::BitStream &base_version, const danet::BitStream &compressed_delta)
{
  danet::BitStream delta(framemem_ptr());
  net::delta::decompress(compressed_delta, delta);
  danet::BitStream result = diff_impl(base_version, delta);
  result.SetWriteOffset(delta.GetWriteOffset());
  result.AlignWriteToByteBoundary();
  return result;
}

void History::clear()
{
  for (int i = 0; i < HIST_MAX; ++i)
    pack[i].Reset();
}

bool History::isEmpty(int packet_num) const { return pack[basePacketNo(packet_num)].GetNumberOfBitsUsed() == 0; }

void History::savePackToStream(danet::BitStream &bs) const
{
  for (int i = 0; i < HIST_MAX; ++i)
    bs.Write(pack[i]);
}

bool History::readPackFromStream(danet::BitStream &bs)
{
  bool isOk = true;
  for (int i = 0; i < HIST_MAX; ++i)
    isOk &= bs.Read(pack[i]);
  return isOk;
}

int64_t History::getDebugAllocSize() const
{
  int64_t result = 0LL;
  for (int i = 0; i < HIST_MAX; ++i)
    result += BITS_TO_BYTES(pack[i].GetNumberOfAllocatedBits());
  return result;
}

} // namespace delta

} // namespace net
