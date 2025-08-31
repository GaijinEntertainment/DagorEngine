// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stcodeBytecode.h"
#include <shaders/shUtils.h>

// @TODO: strongly doubt this is needed anymore, should be removed
#ifdef PROFILE_OPCODE_USAGE
extern int opcode_usage[2][256];
#endif

dag::ConstSpan<int> StcodeBytecodeCache::findOrPost(Tab<int> &&a_routine, bool dyn)
{
  auto code = eastl::move(a_routine);

  if (!code.size())
    return dag::ConstSpan<int>(0, 0);

  Tab<TabStcode> &cache = dyn ? stcode : stblkcode;
  for (int i = cache.size() - 1; i >= 0; i--)
    if (cache[i].size() == code.size() && mem_eq(code, cache[i].data()))
      return cache[i];

  int l = append_items(cache, 1);
  cache[l] = eastl::move(code);

#ifdef PROFILE_OPCODE_USAGE
  debug_("%s code - ", dyn ? "dynamic" : "stateblock");
  ShUtils::shcod_dump(cache[l]);

  {
    for (int i = 0; i < code.size(); i++)
    {
      int opcode = shaderopcode::getOp(code[i]);
      if (dyn)
        opcode_usage[1][opcode]++;
      else
        opcode_usage[0][opcode]++;

      if (opcode == SHCOD_IMM_REAL || opcode == SHCOD_MAKE_VEC)
        i++;
      else if (opcode == SHCOD_IMM_VEC)
        i += 4;
      else if (opcode == SHCOD_CALL_FUNCTION)
        i += shaderopcode::getOp3p3(code[i]);
    }
  }
#endif

  return cache[l];
}
