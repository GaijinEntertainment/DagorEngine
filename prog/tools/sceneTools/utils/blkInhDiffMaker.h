// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>

class DataBlockInhDiffMaker
{
public:
  bool makeDiff(const char *_parent_fn, const char *_source_fn, const char *_result_fn);

private:
  bool diff_datablocks(DataBlock &parent_blk, DataBlock &source_blk, DataBlock &result_blk);
  bool diff_params(DataBlock &parent_blk, DataBlock &source_blk, DataBlock &result_blk);

  DataBlock resBlk;
};