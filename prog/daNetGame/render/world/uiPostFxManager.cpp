// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include <util/dag_convar.h>
#include <render/fx/uiPostFxManager.h>
#include <3d/dag_texMgr.h>
#include <math/integer/dag_IBBox2.h>
// #include <render/fx/ui_blurring.h>
#include <shaders/dag_shaderVar.h>
#include "private_worldRenderer.h"
#include <render/renderEvent.h>
#include <daECS/core/entityManager.h>


static bool blur_update_enabled = true;

bool UiPostFxManager::isBloomEnabled()
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  return renderer != NULL && renderer->hasFeature(FeatureRenderFlags::POSTFX);
}

void UiPostFxManager::updateFinalBlurred(const IPoint2 &lt, const IPoint2 &end, int max_mip)
{
  if (lt.x >= end.x || lt.y >= end.y) // empty
    return;
  IBBox2 box(lt, end);
  updateFinalBlurred(&box, &box + 1, max_mip);
}

void UiPostFxManager::setBlurUpdateEnabled(bool val) { blur_update_enabled = val; }

void UiPostFxManager::updateFinalBlurred(const IBBox2 *begin, const IBBox2 *end, int max_mip)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (!renderer || !blur_update_enabled)
    return;
  g_entity_mgr->broadcastEventImmediate(UpdateBlurredUI(begin, end, max_mip));
}
