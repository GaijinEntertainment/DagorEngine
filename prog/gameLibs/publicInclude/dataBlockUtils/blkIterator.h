//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DataBlock;

class DataBlockIterator
{
  const DataBlock &dataBlock;
  int index = -1;

public:
  DataBlockIterator(const DataBlock &blk, int idx) : dataBlock(blk), index(idx) {}
  DataBlockIterator &operator--();
  DataBlockIterator &operator++();
  const DataBlock &operator*() const;
  bool operator==(const DataBlockIterator &other) const;
  bool operator!=(const DataBlockIterator &other) const;
  bool operator<(const DataBlockIterator &other) const;
  bool operator>(const DataBlockIterator &other) const;
};

DataBlockIterator begin(const DataBlock &blk);
DataBlockIterator end(const DataBlock &blk);
