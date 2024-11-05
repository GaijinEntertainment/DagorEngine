//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>

#include <gameRes/dag_gameResSystem.h>

namespace bind_dascript
{
inline const char *get_game_resource_name(int res_id, das::Context *ctx, das::LineInfoArg *at)
{
  String res;
  ::get_game_resource_name(res_id, res);
  return ctx->allocateString(res.c_str(), res.length(), at);
}
} // namespace bind_dascript
