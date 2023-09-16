//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <dasModules/dasModulesCommon.h>

#include <gameRes/dag_gameResSystem.h>

namespace bind_dascript
{
inline const char *get_game_resource_name(int res_id, das::Context *ctx)
{
  String res;
  ::get_game_resource_name(res_id, res);
  return ctx->stringHeap->allocateString(res.c_str(), res.length());
}
} // namespace bind_dascript
