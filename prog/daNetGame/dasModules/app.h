// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasDataBlock.h>
#include <ecs/scripts/dasEcsEntity.h>
#include <startup/dag_globalSettings.h>

#include "main/circuit.h"
#include "main/main.h"
#include "main/version.h"
#include "main/gameLoad.h"
#include "net/net.h" // net::ConnectParams
#include "main/appProfile.h"
#include "main/physMat.h"

#include "game/gameLauncher.h"

extern int app_profile_get_app_id();

namespace bind_dascript
{
inline const char *get_current_scene(das::Context *context, das::LineInfoArg *at)
{
  eastl::string_view scene{sceneload::get_current_game().sceneName};
  return context->allocateString(scene.begin(), uint32_t(scene.length()), at);
}

inline void exit_game_safe(const char *reason)
{
  logdbg("shutdown because of %s", reason ? reason : "");
  ::exit_game("exit from dascript");
}

inline const DataBlock &dgs_get_game_params()
{
  const DataBlock *res = ::dgs_get_game_params();
  return res ? *res : DataBlock::emptyBlock;
};

inline const char *dgs_get_argv(const char *name) { return ::dgs_get_argv(name, ""); };

inline void dgs_get_argv_all(
  const char *name, const das::TBlock<bool, const char *> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[1];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      int it = 1;
      while (const char *val = ::dgs_get_argv(name, it))
      {
        args[0] = das::cast<const char *>::from(val);
        if (code->evalBool(*context))
          break;
      }
    },
    at);
};

inline void das_switch_scene(
  const char *scene, const das::TArray<char *> &import_scenes, const sceneload::UserGameModeContext &ugm_ctx)
{
  eastl::vector<eastl::string> importScenes(import_scenes.size);
  for (eastl_size_t i = 0; i < import_scenes.size; ++i)
    importScenes[i] = import_scenes[i];
  sceneload::UserGameModeContext ugmCtx = ugm_ctx;

  sceneload::switch_scene(scene, eastl::move(importScenes), eastl::move(ugmCtx));
}

inline void das_switch_scene_and_update(const char *scene) { sceneload::switch_scene_and_update(scene); }

inline void das_connect_to_session(const das::TArray<char *> &server_urls, const sceneload::UserGameModeContext &ugm_ctx)
{
  net::ConnectParams connectParams;
  for (eastl_size_t i = 0; i < server_urls.size; ++i)
    connectParams.serverUrls.push_back(server_urls[i]);
  sceneload::UserGameModeContext ugmCtx = ugm_ctx;
  sceneload::connect_to_session(eastl::move(connectParams), eastl::move(ugmCtx));
}
inline void das_connect_to_session_1(const das::TArray<char *> &server_urls) { das_connect_to_session(server_urls, {}); }

inline void das_switch_scene_2(const char *scene, const das::TArray<char *> &import_scenes)
{
  das_switch_scene(scene, import_scenes, {});
}

inline void das_switch_scene_1(const char *scene) { sceneload::switch_scene(scene, {}); }
} // namespace bind_dascript
