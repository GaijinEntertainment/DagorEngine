// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <dataBlockUtils/blkIterator.h>

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_assert.h>

DataBlockIterator &DataBlockIterator::operator--()
{
  if (index > 0)
    --index;
  return *this;
}

DataBlockIterator &DataBlockIterator::operator++()
{
  if (index < dataBlock.blockCount())
    ++index;
  return *this;
}

const DataBlock &DataBlockIterator::operator*() const
{
  static DataBlock stub;
  const DataBlock *blk = index < dataBlock.blockCount() ? dataBlock.getBlock(index) : nullptr;
  return blk ? *blk : stub;
}

bool DataBlockIterator::operator==(const DataBlockIterator &other) const { return index == other.index; }

bool DataBlockIterator::operator!=(const DataBlockIterator &other) const { return index != other.index; }

bool DataBlockIterator::operator<(const DataBlockIterator &other) const { return index < other.index; }

bool DataBlockIterator::operator>(const DataBlockIterator &other) const { return index > other.index; }

DataBlockIterator begin(const DataBlock &blk) { return DataBlockIterator(blk, 0); }

DataBlockIterator end(const DataBlock &blk) { return DataBlockIterator(blk, blk.blockCount()); }
