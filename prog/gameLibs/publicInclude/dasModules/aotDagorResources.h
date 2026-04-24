//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>

#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>


MAKE_TYPE_FACTORY(GameResourceFactory, GameResourceFactory);

template <>
struct das::isCloneable<::GameResourceFactory> : false_type
{};

namespace bind_dascript
{
inline const char *get_game_resource_name(int res_id, das::Context *ctx, das::LineInfoArg *at)
{
  String res;
  ::get_game_resource_name(res_id, res);
  return ctx->allocateString(res.c_str(), res.length(), at);
}

inline void iterate_gameres_factory_resources(GameResourceFactory *factory, const das::TBlock<void, void *> &block,
  das::Context *context, das::LineInfoArg *at)
{
  factory->iterateGameResources([&](GameResource *res) -> void {
    vec4f arg = das::cast<GameResource *>::from(res);
    context->invoke(block, &arg, nullptr, at);
  });
}

} // namespace bind_dascript
