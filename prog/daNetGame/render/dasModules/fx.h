// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/runtime_matrices.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/aotProps.h>

#include "render/fx/effectManager.h"
#include "render/fx/fx.h"
#include "render/fx/effectEntity.h"
#include <drv/3d/dag_decl.h>

MAKE_TYPE_FACTORY(AcesEffect, AcesEffect);
MAKE_TYPE_FACTORY(ScaledAcesEffect, ScaledAcesEffect);
MAKE_TYPE_FACTORY(TheEffect, TheEffect);
MAKE_TYPE_FACTORY(GravityZoneBuffer, ::acesfx::GravityZoneBuffer);

DAS_BIND_ENUM_CAST_98(FxQuality);
DAS_BASE_BIND_ENUM_98(FxQuality, FxQuality, FX_QUALITY_LOW, FX_QUALITY_MEDIUM, FX_QUALITY_HIGH, FX_QUALITY_ALL);

namespace bind_dascript
{
inline void effect_set_emitter_tm(TheEffect &effect, const TMatrix &tm)
{
  for (auto &fx : effect.getEffects())
  {
    if (fx.fx)
      fx.fx->setEmitterTm(tm);
  }
}

inline void effect_set_scale(TheEffect &effect, float scale)
{
  for (auto &fx : effect.getEffects())
  {
    if (fx.fx)
      fx.fx->setFxScale(scale);
  }
}

inline void effect_set_velocity(TheEffect &effect, const Point3 &vel)
{
  for (auto &fx : effect.getEffects())
  {
    if (fx.fx)
      fx.fx->setVelocity(vel);
  }
}

inline void effect_set_spawn_rate(TheEffect &effect, float rate)
{
  for (auto &fx : effect.getEffects())
  {
    if (fx.fx)
      fx.fx->setSpawnRate(rate);
  }
}

inline void effect_set_gravity_tm(TheEffect &effect, const Matrix3 &tm)
{
  for (ScaledAcesEffect &fx : effect.getEffects())
    fx.fx->setGravityTm(tm);
}

inline AcesEffect *start_effect(int fx_type, const TMatrix &emitter_tm, const TMatrix &fx_tm, bool is_player)
{
  FxErrorType fxErr = FX_ERR_NONE;
  AcesEffect *fx = acesfx::start_effect(fx_type, emitter_tm, fx_tm, is_player, &acesfx::defSoundDesc, &fxErr);
  if (fx)
    return fx;

#if DAGOR_DBGLEVEL > 0
  if (fxErr != FX_ERR_HAS_ENOUGH)
    logerr("das: can't create effect with type <%d> err: %d", fx_type, fxErr);
#endif
  return nullptr;
}

inline bool start_effect_block(int fx_type,
  const TMatrix &emitter_tm,
  const TMatrix &fx_tm,
  bool is_player,
  const das::TBlock<void, AcesEffect> &block,
  das::Context *context,
  das::LineInfoArg *at)
{
  AcesEffect *fx = start_effect(fx_type, emitter_tm, fx_tm, is_player);
  if (fx)
  {
    vec4f arg = das::cast<AcesEffect *>::from(fx);
    context->invoke(block, &arg, nullptr, at);
    return true;
  }

  return false;
}

inline void effects_update(float dt)
{
  // NOTE: this function is only supposed to work with CPU particles warm up.
  // Textures are reset to be explicit about dependencies of finish_update.
  acesfx::setDepthTex(nullptr);
  acesfx::setNormalsTex(nullptr);

  acesfx::start_update_prepare(dt, Driver3dPerspective(), 1, 1);
  acesfx::start_update(dt);
  acesfx::finish_update(TMatrix4::IDENT);
}
} // namespace bind_dascript
