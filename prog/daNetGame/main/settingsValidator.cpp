// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "settingsValidator.h"
#include <EASTL/hash_map.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_log.h>

namespace settings_validator
{
void ensure_no_duplicates(const DataBlock *block, const eastl::string &prefix, bool recursive)
{
  if (recursive)
    for (int i = 0; i < block->blockCount(); i++)
      ensure_no_duplicates(block->getBlock(i),
        eastl::string(eastl::string::CtorSprintf{}, "%s%s/", prefix.c_str(), block->getBlockName()), recursive);

  eastl::hash_map<eastl::string_view, int> countMap;
  for (int i = 0; i < block->paramCount(); i++)
  {
    eastl::string_view paramName = block->getParamName(i);
    countMap[paramName]++;
  }
  for (auto [name, count] : countMap)
  {
    if (count > 1)
      logerr("Duplicate settings param found: %s%s/%s! Count = %d", prefix.c_str(), block->getBlockName(), name.data(), count);
  }
}
} // namespace settings_validator