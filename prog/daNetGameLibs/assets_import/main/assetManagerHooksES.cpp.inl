// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <gameRes/dag_gameResHooks.h>
#include <osApiWrappers/dag_critSec.h>
#include "assetManager.h"


static bool (*dabuild_resolve_res_handle)(GameResHandle rh, unsigned class_id, int &out_res_id) = nullptr;
static bool (*dabuild_release_game_res2)(GameResHandle rh, dag::Span<GameResourceFactory *> f) = nullptr;

template <typename C>
static void asset_manager_find_asset_ecs_query(C);

static WinCritSec hooksMutex;
static bool am_resolve_res_handle(GameResHandle rh, unsigned class_id, int &out_res_id)
{
  bool assetFound = false;
  if (!get_asset_manager())
  {
    LOGERR_ONCE("asset__manager not inited, but hooks setup. Something going wrong with entity creation");
    return false;
  }
  if (get_asset_manager()->findAsset((const char *)rh, get_exportable_types()))
  {
    assetFound = true;
  }

  if (assetFound)
  {
    WinAutoLock lock(hooksMutex); // this function can be called from any thread, so need to provide TS here
    return dabuild_resolve_res_handle(rh, class_id, out_res_id);
  }
  else
    return false;
}

static bool am_release_game_res2(GameResHandle rh, dag::Span<GameResourceFactory *> f)
{
  int res_id;
  if (am_resolve_res_handle(rh, 0xFFFFFFFFU, res_id))
    return dabuild_release_game_res2(rh, f);
  else
    return false;
}

void install_asset_manager_hooks()
{
  // Override dabuild gamereshooks with custom ones to prevent errors spam and incorrect res_id
  // resolving. Packed game resources should be loaded by default gameResSystem
  G_ASSERT(gamereshooks::resolve_res_handle && gamereshooks::on_release_game_res2);
  dabuild_resolve_res_handle = gamereshooks::resolve_res_handle;
  dabuild_release_game_res2 = gamereshooks::on_release_game_res2;
  gamereshooks::resolve_res_handle = am_resolve_res_handle;
  gamereshooks::on_release_game_res2 = am_release_game_res2;
}
