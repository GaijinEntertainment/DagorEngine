#include <daNet/bitStream.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>

namespace danet
{

void BitStream::Write(const DataBlock &blk)
{
  if (blk.isEmpty())
    WriteCompressed((uint32_t)0);
  else
  {
    DynamicMemGeneralSaveCB cwr(allocator, 0, 8 << 10);
    cwr.seekto(0);
    cwr.resize(0);

    DataBlock blkCopy(allocator);
    blkCopy.setFrom(&blk); // save to copy, to guarantee write only needed data (not whole namemap)
    blkCopy.saveToStream(cwr);

    uint32_t sz = cwr.size();
    WriteCompressed(sz);
    if (sz)
    {
      AlignWriteToByteBoundary();
      Write((const char *)cwr.data(), sz);
    }
  }
}

bool BitStream::Read(DataBlock &blk) const
{
  blk.reset();
  uint32_t bytesToRead = 0;
  if (!ReadCompressed(bytesToRead))
    return false;
  if (!bytesToRead)
    return true;
  AlignReadToByteBoundary();
  if (readOffset + bytes2bits(bytesToRead) > bitsUsed)
    return false;
  InPlaceMemLoadCB crd(GetData() + bits2bytes(readOffset), bytesToRead);
  bool ret = false;
  if (dblk::load_from_stream(blk, crd, dblk::ReadFlag::ROBUST | dblk::ReadFlag::BINARY_ONLY | dblk::ReadFlag::RESTORE_FLAGS))
  {
    readOffset += bytes2bits(bytesToRead);
    ret = true;
  }
  return ret;
}

void BitStream::swap(BitStream &bs)
{
#define _SWAP(type, l, r) \
  {                       \
    type t = l;           \
    l = r;                \
    r = t;                \
  }
  _SWAP(uint32_t, this->bitsUsed, bs.bitsUsed)
  _SWAP(uint32_t, this->dataOwner, bs.dataOwner)
  _SWAP(uint32_t, this->bitsAllocated, bs.bitsAllocated)
  _SWAP(uint32_t, this->readOffset, bs.readOffset)
  _SWAP(uint8_t *, this->data, bs.data)
  _SWAP(IMemAlloc *, this->allocator, bs.allocator)
#undef _SWAP
}

}; // namespace danet
