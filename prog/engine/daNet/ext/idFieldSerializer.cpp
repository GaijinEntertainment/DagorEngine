// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daNet/idFieldSerializer.h>
#include <daNet/bitStream.h>

#include <math/dag_bits.h>
#include <debug/dag_assert.h>

#include "sizeCoder.h"

namespace danet
{

IdFieldSerializer32::IdFieldSerializer32() : currWrSz(0), currRdSz(0) { mem_set_0(sizes); }

IdFieldSerializer32::IdFieldSerializer32(const IdFieldSerializer32 &other) : currWrSz(other.currWrSz), currRdSz(other.currRdSz)
{
  mem_copy_from(sizes, other.sizes.data());
}

IdFieldSerializer32 &IdFieldSerializer32::operator=(const IdFieldSerializer32 &other)
{
  currWrSz = other.currWrSz;
  currRdSz = other.currRdSz;
  mem_copy_from(sizes, other.sizes.data());
  return *this;
}

void IdFieldSerializer32::setFieldSize(uint32_t sz)
{
  G_ASSERT(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}

#if DAGOR_DBGLEVEL > 0
void IdFieldSerializer32::checkFieldSize(uint8_t index, BitSize_t sz) const
{
  G_ASSERT(index < currRdSz);
  G_ASSERT(sz == sizes[index]);
}
#endif

void IdFieldSerializer32::writeFieldsSize(BitStream &to) const
{
  G_ASSERT(currWrSz);
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset());
  G_ASSERT(offset <= USHRT_MAX);
  for (uint8_t j = 0; j < currWrSz; ++j)
  {
    G_ASSERT(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(0);
  to.Write((uint16_t)offset);
  to.SetWriteOffset(end);
}

uint32_t IdFieldSerializer32::readFieldsSizeAndFlag(const BitStream &from)
{
  uint32_t fields = 0;
  uint16_t offset = 0;
  BitSize_t start = from.GetReadOffset();
  G_ASSERT((start & 7) == 0); // aligned to byte
  from.Read(offset);
  from.ReadCompressed(fields);
  BitSize_t startBody = from.GetReadOffset();
  from.SetReadOffset(BYTES_TO_BITS(offset) + start);
  currRdSz = static_cast<uint8_t>(__popcount(fields));
  G_ASSERT(currRdSz);
  for (uint8_t j = 0; j < currRdSz; ++j)
  {
    sizes[j] = readSize(from);
    G_ASSERT(sizes[j]);
  }
  from.SetReadOffset(startBody);
  return fields;
}

void IdFieldSerializer32::skipReadingField(uint8_t index, const BitStream &from) const
{
  G_ASSERT(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}


IdFieldSerializer255::IdFieldSerializer255() : currWrId(0), currWrSz(0), currRdSz(0), bitsPerId(0)
{
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::reset()
{
  currWrId = 0;
  currWrSz = 0;
  currRdSz = 0;
  bitsPerId = 0;
  mem_set_0(sizes);
  mem_set_0(indices);
}

void IdFieldSerializer255::setFieldSize(uint32_t sz)
{
  G_ASSERT(currWrSz < MAX_FIELDS_NUM);
  sizes[currWrSz++] = sz;
}

void IdFieldSerializer255::setFieldId(Id id)
{
  G_ASSERT(currWrId < MAX_FIELDS_NUM);
  indices[currWrId++] = id;
}

void IdFieldSerializer255::writeFieldsIndex(BitStream &to, BitSize_t at) const
{
  G_ASSERT(currWrId);
  G_ASSERT((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t endBody = to.GetWriteOffset();
  // we are at the endBody
  G_ASSERT(currWrId == currWrSz);
  to.SetWriteOffset(at + BYTES_TO_BITS(sizeof(uint16_t)) /*offset*/);
  Index maxId = 1; //__bsr(0) === 32
  for (Index j = 0; j < currWrId; ++j)
  {
    if (indices[j] > maxId)
      maxId = indices[j];
  }
  Index bitsPerIdToWrite = __bsr(maxId) + 1;
  Index countToWrite = currWrId | (bitsPerIdToWrite << BITS_PER_COUNT);
  to.Write(countToWrite);
  to.SetWriteOffset(endBody);
  for (Index j = 0; j < currWrId; ++j)
    to.WriteBits(reinterpret_cast<const uint8_t *>(&indices[j]), bitsPerIdToWrite);
}

inline static BitSize_t alingedToByte(BitSize_t count) { return count + 8 - (((count - 1) & 7) + 1); }

bool IdFieldSerializer255::readFieldsIndex(const BitStream &from)
{
  G_ASSERT(currRdSz && bitsPerId);
  BitSize_t startBody = from.GetReadOffset();
  // we are at the startBody so reread offset and check count
  uint16_t offset = 0;
  Index count = 0;
  const BitSize_t rewind = BYTES_TO_BITS(sizeof(offset)) + BYTES_TO_BITS(sizeof(count));
  G_ASSERT(startBody >= rewind);
  from.SetReadOffset(startBody - rewind);
  from.Read(offset);
  from.Read(count);
  G_ASSERT(count == (currRdSz | (bitsPerId << BITS_PER_COUNT)));
  BitSize_t bitsForIndices = alingedToByte(currRdSz * bitsPerId);
  from.SetReadOffset(startBody - rewind + BYTES_TO_BITS(offset) - bitsForIndices);
  for (Index j = 0; j < currRdSz; ++j)
    if (!from.ReadBits(reinterpret_cast<uint8_t *>(&indices[j]), bitsPerId))
    {
      G_ASSERT(false);
      return false;
    }
  from.SetReadOffset(startBody);
  return true;
}

IdFieldSerializer255::Id IdFieldSerializer255::getFieldId(Index index) const { return indices[index]; }

uint32_t IdFieldSerializer255::getFieldSize(Index index) const { return sizes[index]; }

void IdFieldSerializer255::writeFieldsSize(BitStream &to, BitSize_t at) const
{
  G_ASSERT(currWrId == 0 || currWrId == currWrSz);
  G_ASSERT(currWrSz);
  G_ASSERT((at & 7) == 0); // aligned to byte
  to.AlignWriteToByteBoundary();
  BitSize_t offset = BITS_TO_BYTES(to.GetWriteOffset() - at);
  G_ASSERT(offset <= USHRT_MAX);
  for (Index j = 0; j < currWrSz; ++j)
  {
    G_ASSERT(sizes[j]);
    writeSize(to, sizes[j]);
  }
  to.AlignWriteToByteBoundary();
  BitSize_t end = to.GetWriteOffset();
  to.SetWriteOffset(at);
  to.Write((uint16_t)offset);
  to.SetWriteOffset(end);
}

IdFieldSerializer255::Index IdFieldSerializer255::readFieldsSizeAndCount(const BitStream &from, BitSize_t &end)
{
  uint16_t offset = 0;
  Index count = 0;
  BitSize_t start = from.GetReadOffset();
  G_ASSERT((start & 7) == 0); // aligned to byte
  from.Read(offset);
  from.Read(count);
  BitSize_t startBody = from.GetReadOffset();
  from.SetReadOffset(BYTES_TO_BITS(offset) + start);
  currRdSz = count & BIT_MASK_COUNT;
  bitsPerId = count >> BITS_PER_COUNT;
  G_ASSERT(currRdSz);
  for (Index j = 0; j < currRdSz; ++j)
  {
    sizes[j] = readSize(from);
    G_ASSERT(sizes[j]);
  }
  from.AlignReadToByteBoundary();
  end = from.GetReadOffset();
  from.SetReadOffset(startBody);
  return currRdSz;
}

void IdFieldSerializer255::skipReadingField(Index index, const BitStream &from) const
{
  G_ASSERT(index < currRdSz);
  from.SetReadOffset(from.GetReadOffset() + sizes[index]);
}

} // namespace danet
