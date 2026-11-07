// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_convar.h>
#include <bvh/bvh.h>
#include "render/renderSettings.h"

CONSOLE_FLOAT_VAL("render", tree_wind_anim_distance, 0, 0, 10000);
CONSOLE_FLOAT_VAL("render", tree_wind_anim_fade_range, 0, 0, 1000);
CONSOLE_FLOAT_VAL("render", dagdp_wind_anim_distance, 0, 0, 10000);
CONSOLE_FLOAT_VAL("render", dagdp_wind_anim_fade_range, 0, 0, 1000);

namespace var
{
static ShaderVariableInfo tree_wind_anim_distance("tree_wind_anim_distance", true);
static ShaderVariableInfo tree_wind_anim_fade_range("tree_wind_anim_fade_range", true);
static ShaderVariableInfo dagdp_wind_anim_distance("dagdp_wind_anim_distance", true);
static ShaderVariableInfo dagdp_wind_anim_fade_range("dagdp_wind_anim_fade_range", true);
} // namespace var

static void update_tree_wind_anim_fade_params()
{
  const float dist = tree_wind_anim_distance;
  const float range = max(float(tree_wind_anim_fade_range), 0.001f);
  ShaderGlobal::set_float(var::tree_wind_anim_distance, dist);
  ShaderGlobal::set_float(var::tree_wind_anim_fade_range, range);
  ShaderGlobal::set_float(var::dagdp_wind_anim_distance, dagdp_wind_anim_distance);
  ShaderGlobal::set_float(var::dagdp_wind_anim_fade_range, max(float(dagdp_wind_anim_fade_range), 0.001f));
}

// Read back from the shadervars, so a render.shadervar test moves the BVH cutoff with the fade.
static void update_bvh_tree_anim_max_distance()
{
  static float lastMaxDistance = -1;
  const float dist = ShaderGlobal::get_float(var::tree_wind_anim_distance);
  const float maxDistance = dist > 0 ? dist + ShaderGlobal::get_float(var::tree_wind_anim_fade_range) : 0;
  if (maxDistance == lastMaxDistance)
    return;
  lastMaxDistance = maxDistance;
  bvh::set_tree_anim_max_distance(maxDistance);
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void tree_wind_anim_fade_settings_es(const ecs::Event &)
{
  const DataBlock *gfx = dgs_get_settings()->getBlockByNameEx("graphics");
  tree_wind_anim_distance = gfx->getReal("treeWindAnimDistance", tree_wind_anim_distance);
  tree_wind_anim_fade_range = gfx->getReal("treeWindAnimFadeRange", tree_wind_anim_fade_range);
  dagdp_wind_anim_distance = gfx->getReal("dagdpWindAnimDistance", dagdp_wind_anim_distance);
  dagdp_wind_anim_fade_range = gfx->getReal("dagdpWindAnimFadeRange", dagdp_wind_anim_fade_range);
  update_tree_wind_anim_fade_params();
}

ECS_TAG(render)
static void tree_wind_anim_fade_convar_es(const UpdateStageInfoBeforeRender &)
{
  if (tree_wind_anim_distance.pullValueChange() || tree_wind_anim_fade_range.pullValueChange() ||
      dagdp_wind_anim_distance.pullValueChange() || dagdp_wind_anim_fade_range.pullValueChange())
    update_tree_wind_anim_fade_params();
  update_bvh_tree_anim_max_distance();
}
