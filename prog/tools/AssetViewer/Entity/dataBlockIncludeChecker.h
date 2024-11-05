// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>

class DataBlockIncludeChecker
{
public:
  static bool DoesBlkUseIncludes(const char *blkFilePath)
  {
    G_ASSERT(!DataBlock::parseIncludesAsParams);
    DataBlock::parseIncludesAsParams = true;

    DataBlock block;
    block.load(blkFilePath);

    G_ASSERT(DataBlock::parseIncludesAsParams);
    DataBlock::parseIncludesAsParams = false;

    return DoesBlkUseIncludesInternal(block);
  }

private:
  static bool DoesBlkUseIncludesInternal(const DataBlock &block)
  {
    for (int i = 0; i < block.blockCount(); ++i)
      if (DoesBlkUseIncludesInternal(*block.getBlock(i)))
        return true;

    for (int i = 0; i < block.paramCount(); ++i)
      if (block.findParam("@include") >= 0)
        return true;

    return false;
  }
};
