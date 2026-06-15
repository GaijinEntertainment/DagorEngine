// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <math/dag_color.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_convar.h>
#include <bvh/bvh.h>
#include "render/world/bvh.h"
#include "render/renderSettings.h"

CONSOLE_FLOAT_VAL("render", tree_wind_anim_distance, 0, 0, 10000);
CONSOLE_FLOAT_VAL("render", tree_wind_anim_fade_range, 0, 0, 1000);

namespace var
{
static ShaderVariableInfo tree_wind_anim_fade_params("tree_wind_anim_fade_params", true);
}

// can be changed if needed elsewhere, but bvh benefits the most from it
static bool should_use_wind_anim_distance_fade() { return is_bvh_enabled(); }


static void update_tree_wind_anim_fade_params()
{
  if (tree_wind_anim_distance > 0 && should_use_wind_anim_distance_fade())
  {
    float dist = tree_wind_anim_distance;
    float range = max(float(tree_wind_anim_fade_range), 0.001f);
    float inv = 1.0f / range;
    ShaderGlobal::set_float4(var::tree_wind_anim_fade_params, Color4(-inv, 1.0f + dist * inv, 0, 0));
    bvh::set_tree_anim_max_distance(dist + range);
  }
  else
  {
    ShaderGlobal::set_float4(var::tree_wind_anim_fade_params, Color4(0, 1, 0, 0));
    bvh::set_tree_anim_max_distance(0);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
static void tree_wind_anim_fade_settings_es(const ecs::Event &)
{
  const DataBlock *gfx = dgs_get_settings()->getBlockByNameEx("graphics");
  tree_wind_anim_distance = gfx->getReal("treeWindAnimDistance", tree_wind_anim_distance);
  tree_wind_anim_fade_range = gfx->getReal("treeWindAnimFadeRange", tree_wind_anim_fade_range);
  update_tree_wind_anim_fade_params();
}

ECS_TAG(render)
static void tree_wind_anim_fade_convar_es(const UpdateStageInfoBeforeRender &)
{
  if (tree_wind_anim_distance.pullValueChange() || tree_wind_anim_fade_range.pullValueChange())
    update_tree_wind_anim_fade_params();
}
