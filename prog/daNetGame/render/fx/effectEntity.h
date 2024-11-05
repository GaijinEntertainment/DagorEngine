// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>
#include <generic/dag_span.h>
#include <daECS/core/event.h>

namespace ecs
{
class ResourceRequestCb;
}

class AcesEffect;

struct ScaledAcesEffect
{
  AcesEffect *fx; // can't be null
  float scale;
};

enum class FxSpawnType
{
  World,
  Player,
};

class TheEffect
{
  uint16_t numEffects;
  union
  {
    ScaledAcesEffect *effectsHeap;
    ScaledAcesEffect effectsInline[1];
  };

public:
  TheEffect() = delete;
  TheEffect(AcesEffect &&effect);
  TheEffect(TheEffect &&other);
  TheEffect(ecs::EntityManager &mgr, ecs::EntityId eid);
  TheEffect(const TheEffect &) = delete;
  TheEffect &operator=(const TheEffect &) = delete;
  TheEffect &operator=(TheEffect &&other);
  ~TheEffect();
  void reset();

  static void requestResources(const char *, const ecs::ResourceRequestCb &res_cb);

  dag::Span<ScaledAcesEffect> getEffects()
  {
    return numEffects <= countof(effectsInline) ? dag::Span<ScaledAcesEffect>(effectsInline, numEffects)
                                                : dag::Span<ScaledAcesEffect>(effectsHeap, numEffects);
  }

  dag::ConstSpan<ScaledAcesEffect> getEffects() const
  {
    return numEffects <= countof(effectsInline) ? dag::ConstSpan<ScaledAcesEffect>(effectsInline, numEffects)
                                                : dag::ConstSpan<ScaledAcesEffect>(effectsHeap, numEffects);
  }

private:
  bool addEffect(const char *fxName,
    const TMatrix &tm,
    float fxScale,
    float fx_inst_scale,
    const Color4 &fxColorMult,
    bool is_attached,
    bool with_sound,
    const TMatrix *restriction_box,
    FxSpawnType spawnType);
};

ECS_DECLARE_RELOCATABLE_TYPE(TheEffect);
ECS_UNICAST_EVENT_TYPE(RecreateEffectEvent);