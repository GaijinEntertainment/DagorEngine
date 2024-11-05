// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>

#include "game/riDestr.h"
#include "render/fx/fx.h"
#include "render/fx/effectManager.h"
#include "main/level.h"
#include <ecs/core/entityManager.h>

void ri_destr_start_effect(
  int type, const TMatrix &, const TMatrix &fx_tm, int pool_idx, bool is_player, AcesEffect **locked_fx, const char *effect_template)
{
  if (is_level_unloading() || !(is_level_loading() || is_level_loaded()))
    return;

  if (effect_template && *effect_template != '\0')
  {
    const ecs::Template *tmpl = g_entity_mgr->getTemplateDB().getTemplateByName(effect_template);
    if (!tmpl)
      return;

    int lowestFxQuality = tmpl->getComponent(ECS_HASH("effect__lowestFxQuality")).getOr<int>(0);
    if (int(acesfx::get_fx_target()) < (1 << lowestFxQuality))
      return;

    ecs::ComponentsInitializer attrs;
    attrs[ECS_HASH("transform")] = fx_tm;
    attrs[ECS_HASH("effect__riPoolUsedForColoring")] = pool_idx;
    attrs[ECS_HASH("effect__spawnOnPlayer")].operator=<bool>(is_player);
    g_entity_mgr->createEntityAsync(effect_template, eastl::move(attrs));
    return;
  }
  AcesEffect *lockedFx = acesfx::start_effect(type, fx_tm, TMatrix::IDENT, is_player);
  if (lockedFx)
    lockedFx->setFxScale(length(fx_tm.getcol(0)));
  if (locked_fx)
  {
    *locked_fx = lockedFx;
    if (lockedFx)
      lockedFx->lock();
  }
}
