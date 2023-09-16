#include <dataBlockUtils/blkIterator.h>

#include <ioSys/dag_dataBlock.h>
#include <debug/dag_assert.h>

DataBlockIterator &DataBlockIterator::operator--()
{
  G_ASSERT(index > 0);
  --index;
  return *this;
}

DataBlockIterator &DataBlockIterator::operator++()
{
  G_ASSERT(index < dataBlock.blockCount());
  ++index;
  return *this;
}

const DataBlock &DataBlockIterator::operator*() const
{
  G_ASSERT(index < dataBlock.blockCount());

  return *dataBlock.getBlock(index);
}

bool DataBlockIterator::operator==(const DataBlockIterator &other) const { return index == other.index; }

bool DataBlockIterator::operator!=(const DataBlockIterator &other) const { return index != other.index; }

bool DataBlockIterator::operator<(const DataBlockIterator &other) const { return index < other.index; }

bool DataBlockIterator::operator>(const DataBlockIterator &other) const { return index > other.index; }

DataBlockIterator begin(const DataBlock &blk) { return DataBlockIterator(blk, 0); }

DataBlockIterator end(const DataBlock &blk) { return DataBlockIterator(blk, blk.blockCount()); }
