// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <effectManager/effectManager.h>
#include <effectManager/effectManagerDetails.h>

#include <fx/dag_baseFxClasses.h>
#include <render/lightShadowParams.h>

#define INSIDE_RENDERER 1
#include "../world/private_worldRenderer.h"

namespace acesfx
{
bool g_async_load = true;
} // namespace acesfx


static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }
static ShadowsQuality shadowQuality = ShadowsQuality::SHADOWS_LOW;

void EffectManager::LightEffect::activateShadowExt() const
{
  LightShadowParams lsp = shadowParams;
  if (shadowQuality < ShadowsQuality::SHADOWS_MEDIUM && !lsp.isDynamic)
    lsp.supportsDynamicObjects = false;
  if (shadowQuality < ShadowsQuality::SHADOWS_HIGH)
    lsp.supportsGpuObjects = false;
  get_renderer()->updateLightShadow(extLightId, lsp);
}

void EffectManager::LightEffect::releaseShadowExt() const { get_renderer()->updateLightShadow(extLightId, {}); }

void EffectManager::onSettingsChangedExt() { shadowQuality = get_renderer()->getSettingsShadowsQuality(); }

void EffectManager::initLightEnabledExt() { params.lightEnabled = effectBase->getParam(HUID_LIGHT_PARAMS, NULL) && get_renderer(); }

void EffectManager::destroyLightExt(LightEffect &le)
{
  WorldRenderer *wr = get_renderer();
  G_ASSERT(wr);

  wr->destroyLight(le.extLightId);
}

int EffectManager::addLightExt(LightEffect &le)
{
  WorldRenderer *wr = get_renderer();
  G_ASSERT(wr);

  OmniLight light = OmniLight{le.pos, Color3(&le.params.x), le.params.w, 0, le.getBox()};
  return wr->addOmniLight(eastl::move(light), OmniLightMaskType::OMNI_LIGHT_MASK_NONE);
}

void EffectManager::updateLightExt(LightEffect &le)
{
  WorldRenderer *wr = get_renderer();
  G_ASSERT(wr);

  OmniLight light = wr->getOmniLight(le.extLightId);

  light.setPos(le.pos);
  light.setColor(&le.params.x);
  light.setRadius(le.params.w);
  const TMatrix *box = le.getBox();
  if (box)
    light.setBox(*box);
  else
    light.setDefaultBox();

  // assumes all fx lights are highly dynamic
  wr->setLightWithMask(le.extLightId, light, OmniLightMaskType::OMNI_LIGHT_MASK_NONE, true);
}


// NOTE: sound in vfx is unsupported in DNG
void EffectManager::loadSoundParamsExt() {}
void EffectManager::destroyFxSoundExt(int, bool, bool) {}
void EffectManager::setFxTmSoundExt(BaseEffect &) {}
void EffectManager::setFxVelocitySoundExt(BaseEffect &, const Point3 &) {}
void EffectManager::setFxSoundParamsExt(BaseEffect &, float, float) {}
void EffectManager::tryPlayFxSoundExt(SoundEffect &) {}
void EffectManager::pauseFxSoundExt(BaseEffect &, bool) {}
void EffectManager::startEffectSoundExt(AcesEffect::FxId, BaseEffect &, bool, const Point3 &) {}
void EffectManager::updateFxSoundsExt(float) {}
void EffectManager::clearAllSoundsExt() {}