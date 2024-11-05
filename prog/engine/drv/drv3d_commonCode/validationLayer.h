// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/vector.h>
#include <ioSys/dag_dataBlock.h>

template <class MESSAGE_T>
eastl::vector<MESSAGE_T> get_ignored_validation_messages(const DataBlock &blk)
{
  eastl::vector<MESSAGE_T> ignoreList;

  if (auto ignoreBlock = blk.getBlockByName("ignore"))
  {
    ignoreList.reserve(ignoreBlock->paramCount());
    for (int i = 0; i < ignoreBlock->paramCount(); i++)
    {
      if (auto ignoreId = ignoreBlock->getInt(i))
      {
        ignoreList.push_back((MESSAGE_T)ignoreId);
      }
    }
  }
  return ignoreList;
}
