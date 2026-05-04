// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <gameRes/dag_gameResHooks.h>
#include <osApiWrappers/dag_critSec.h>
#include "assetManager.h"


static bool (*dabuild_resolve_res_handle)(const char *resname, unsigned class_id, int &out_res_id) = nullptr;

static WinCritSec hooksMutex;
static bool am_resolve_res_handle(const char *resname, unsigned class_id, int &out_res_id)
{
  bool assetFound = false;
  if (!get_asset_manager())
  {
    LOGERR_ONCE("asset__manager not inited, but hooks setup. Something going wrong with entity creation");
    return false;
  }
  if (get_asset_manager()->findAsset(resname, get_exportable_types()))
  {
    assetFound = true;
  }

  if (assetFound)
  {
    WinAutoLock lock(hooksMutex); // this function can be called from any thread, so need to provide TS here
    return dabuild_resolve_res_handle(resname, class_id, out_res_id);
  }
  else
    return false;
}

void install_asset_manager_hooks()
{
  // Override dabuild gamereshooks with custom ones to prevent errors spam and incorrect res_id
  // resolving. Packed game resources should be loaded by default gameResSystem
  G_ASSERT(gamereshooks::resolve_res_handle);
  dabuild_resolve_res_handle = gamereshooks::resolve_res_handle;
  gamereshooks::resolve_res_handle = am_resolve_res_handle;
}
