// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_dataBlockCommentsDef.h>

class DataBlockCommentChecker
{
public:
  static bool doesBlkHaveComments(const char *blkFilePath)
  {
    const bool originalParseCommentsAsParams = DataBlock::parseCommentsAsParams;
    const bool originalParseIncludesAsParams = DataBlock::parseIncludesAsParams;
    DataBlock::parseCommentsAsParams = true;
    DataBlock::parseIncludesAsParams = true;

    DataBlock block;
    block.load(blkFilePath);

    G_ASSERT(DataBlock::parseCommentsAsParams);
    G_ASSERT(DataBlock::parseIncludesAsParams);
    DataBlock::parseCommentsAsParams = originalParseCommentsAsParams;
    DataBlock::parseIncludesAsParams = originalParseIncludesAsParams;

    return doesBlkHaveCommentsInternal(block);
  }

private:
  static bool doesBlkHaveCommentsInternal(const DataBlock &block)
  {
    for (int i = 0; i < block.paramCount(); ++i)
      if (CHECK_COMMENT_PREFIX(block.getParamName(i)))
        return true;

    for (int i = 0; i < block.blockCount(); ++i)
      if (doesBlkHaveCommentsInternal(*block.getBlock(i)))
        return true;

    return false;
  }
};
