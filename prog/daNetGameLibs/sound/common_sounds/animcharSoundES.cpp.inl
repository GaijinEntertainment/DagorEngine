// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/vector_map.h>
#include <EASTL/fixed_string.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/sharedComponent.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <soundSystem/debug.h>
#include <util/dag_convar.h>
#include "animcharSound.h"

#define ANIMCHAR_SOUND_ECS_EVENT ECS_REGISTER_EVENT
ANIMCHAR_SOUND_ECS_EVENTS
#undef ANIMCHAR_SOUND_ECS_EVENT
ECS_REGISTER_EVENT(CmdSoundStepIrq)

using sound_irq_name_t = eastl::fixed_string<char, 32, false>;

namespace cvars
{
static CONSOLE_BOOL_VAL("snd", irq_debug, false);
}
static inline bool is_watched(ecs::EntityId eid)
{
  if (ECS_GET_OR(eid, is_watched_sound, false))
    return true;
  eid = ECS_GET_OR(eid, gun__owner, ecs::INVALID_ENTITY_ID);
  return ECS_GET_OR(eid, is_watched_sound, false);
}

static inline void debug_irq_impl(ecs::EntityId eid, int type, intptr_t p1, intptr_t p2, int offset)
{
  if (!cvars::irq_debug.get())
    return;
  if (is_watched(eid))
    if (const auto *animchar = ECS_GET_NULLABLE(AnimV20::AnimcharBaseComponent, eid, animchar))
      if (const auto *animGraph = animchar->getAnimGraph())
        sndsys::debug_trace_warn("[%d] irq%d (%s) from \"%s\" at frame %d (tick=%d), at %.3f sec (animtime)", offset, type,
          AnimV20::getIrqName(type), animGraph->getBlendNodeName((AnimV20::IAnimBlendNode *)(void *)p1),
          p2 / (AnimV20::TIME_TicksPerSec / 30), p2,
          animchar->getAnimState() ? animchar->getAnimState()->getParam(animGraph->PID_GLOBAL_TIME) : 0);
}
static inline const char *debug_get_node_name(ecs::EntityId eid, intptr_t p1)
{
  if (const auto *animchar = ECS_GET_NULLABLE(AnimV20::AnimcharBaseComponent, eid, animchar))
    if (const auto *animGraph = animchar->getAnimGraph())
      return animGraph->getBlendNodeName((AnimV20::IAnimBlendNode *)(void *)p1);
  return "";
}
static inline void debug_irq(ecs::EntityId eid, int type, intptr_t p1, intptr_t p2, int offset)
{
#if DAGOR_DBGLEVEL > 0
  debug_irq_impl(eid, type, p1, p2, offset);
#else
  G_UNREFERENCED(eid);
  G_UNREFERENCED(type);
  G_UNREFERENCED(p1);
  G_UNREFERENCED(p2);
  G_UNREFERENCED(offset);
#endif
}

template <typename Irq>
class BaseSoundHandler : public ecs::AnimIrqHandler
{
  eastl::vector_map<int /*irq_id*/, Irq> irqs;

protected:
  BaseSoundHandler(ecs::EntityId eid) : ecs::AnimIrqHandler(eid) {}

  __forceinline const Irq *get(int irq_id) const
  {
    const auto it = irqs.find(irq_id);
    return it != irqs.end() ? &it->second : nullptr;
  }

  void registerIrqImpl(const char *irq_name, const Irq &irq, AnimV20::AnimcharBaseComponent &animchar)
  {
    const int irqId = AnimV20::addIrqId(irq_name);
    G_ASSERT(irqId >= 0);
    auto it = irqs.lower_bound(irqId);
    if (it == irqs.end() || it->first != irqId)
    {
      irqs.insert(it, eastl::make_pair(irqId, irq));
      animchar.registerIrqHandler(irqId, this);
    }
  }
};

static bool should_play(ecs::EntityId eid, int irq_id, intptr_t p1, intptr_t p2, intptr_t p3, bool play_backward = false)
{
  G_UNUSED(p1);
  G_UNUSED(p2);
  // p3-offset is from parametric.
  // by default do not play any parametric irq's when moving backwards.
  // this fixes several unfixable problems with weap reloading.
  // parametric irqs from weap reloading is a BIG problem.
  // may spicify this behaviour in sound blk if need.
  const int offset = p3;
  debug_irq(eid, irq_id, p1, p2, offset);
  return offset >= 0 || play_backward;
}

static void debug_anim_param_irq(ecs::EntityId eid, int irq_id, intptr_t p1, intptr_t p2, intptr_t p3)
{
#if DAGOR_DBGLEVEL > 0
  if (irq_id == AnimV20::getDebugAnimParamIrqId())
  {
    if (!is_watched(eid))
      return;
    const float param = safediv(float(p2), float(p3));
    // if (prevParam != param)
    {
      const int offset = p3;
      debug_irq(eid, irq_id, p1, p2, offset);
      const char *nodeName = debug_get_node_name(eid, p1);
      // sendEventImmediate may case something like "[E] : access violation of thread context", may need to replace with sendEvent"
      g_entity_mgr->sendEventImmediate(eid, CmdSoundDebugAnimParam(param, nodeName));
    }
    // prevParam = param;
  }
#else
  G_UNREFERENCED(eid);
  G_UNREFERENCED(irq_id);
  G_UNREFERENCED(p1);
  G_UNREFERENCED(p2);
  G_UNREFERENCED(p3);
#endif
}

template <typename Event>
class GenIrqSoundHandler : public BaseSoundHandler<sound_irq_name_t>
{
  intptr_t irq(int irq_id, intptr_t p1, intptr_t p2, intptr_t p3) override
  {
    debug_anim_param_irq(eid, irq_id, p1, p2, p3);
    if (should_play(eid, irq_id, p1, p2, p3))
    {
      if (auto irqPtr = get(irq_id))
        g_entity_mgr->sendEvent(eid, Event(irqPtr->c_str()));
    }
    return 0;
  }

public:
  GenIrqSoundHandler(ecs::EntityId eid) : BaseSoundHandler(eid) {}

  void registerIrq(const char *irq_name, AnimV20::AnimcharBaseComponent &animchar) { registerIrqImpl(irq_name, irq_name, animchar); }
};


struct TypeIrq
{
  sndsys::str_hash_t type;
  sound_irq_name_t name;
  bool playBackward;
  TypeIrq() = delete;
};

template <typename Event>
class TypeIrqSoundHandler : public BaseSoundHandler<TypeIrq>
{
  intptr_t irq(int irq_id, intptr_t p1, intptr_t p2, intptr_t p3) override
  {
    debug_anim_param_irq(eid, irq_id, p1, p2, p3);
    if (const TypeIrq *irqPtr = get(irq_id))
    {
      if (should_play(eid, irq_id, p1, p2, p3, irqPtr->playBackward))
        g_entity_mgr->sendEvent(eid, Event(irqPtr->name.c_str(), irqPtr->type));
    }
    return 0;
  }

public:
  TypeIrqSoundHandler(ecs::EntityId eid) : BaseSoundHandler(eid) {}

  void registerIrq(const char *irq_name, const char *type, bool play_backward, AnimV20::AnimcharBaseComponent &animchar)
  {
    registerIrqImpl(irq_name, TypeIrq{SND_HASH_SLOW(type), irq_name, play_backward}, animchar);
  }

  void registerDebugAnimParamIrq(AnimV20::AnimcharBaseComponent &animchar)
  {
#if DAGOR_DBGLEVEL > 0
    const int irqId = AnimV20::getDebugAnimParamIrqId();
    animchar.registerIrqHandler(irqId, this);
#else
    G_UNREFERENCED(animchar);
#endif
  }
};

class StepIrqSoundHandler : public ecs::AnimIrqHandler
{
  eastl::vector_map<int /*irq_id*/, eastl::pair<int /*obj_idx*/, bool /*play_backward*/>> map;

  intptr_t irq(int irq_id, intptr_t p1, intptr_t p2, intptr_t p3) override
  {
    debug_anim_param_irq(eid, irq_id, p1, p2, p3);
    const auto it = map.find(irq_id);
    if (it != map.end())
    {
      if (should_play(eid, irq_id, p1, p2, p3, it->second.second /*play_backward*/))
        g_entity_mgr->sendEvent(eid, CmdSoundStepIrq(it->second.first));
    }
    return 0;
  }

public:
  StepIrqSoundHandler(ecs::EntityId eid) : ecs::AnimIrqHandler(eid) {}

  void registerIrq(const char *irq_name, int obj_idx, bool play_backward, AnimV20::AnimcharBaseComponent &animchar)
  {
    const int irqId = AnimV20::addIrqId(irq_name);
    G_ASSERT(irqId >= 0);
    if (auto it = map.lower_bound(irqId); it == map.end() || it->first != irqId)
    {
      map.insert(it, eastl::make_pair(irqId, eastl::make_pair(obj_idx, play_backward)));
      animchar.registerIrqHandler(irqId, this);
    }
  }
};

struct AnimcharSoundHandlers
{
  TypeIrqSoundHandler<CmdSoundIrq> irqHandler;

  AnimcharSoundHandlers(ecs::EntityId eid) : irqHandler(eid) {}
};

struct HumanAnimcharSoundHandlers
{
  TypeIrqSoundHandler<CmdSoundIrq> irqHandler;
  TypeIrqSoundHandler<CmdSoundVoicefxIrq> voicefxHandler;
  GenIrqSoundHandler<CmdSoundMeleeIrq> meleeHandler;
  StepIrqSoundHandler stepHandler;

  HumanAnimcharSoundHandlers(ecs::EntityId eid) : irqHandler(eid), voicefxHandler(eid), meleeHandler(eid), stepHandler(eid) {}
};

struct AnimcharSound
{
  eastl::unique_ptr<AnimcharSoundHandlers> handlers;
};

struct HumanAnimcharSound
{
  eastl::unique_ptr<HumanAnimcharSoundHandlers> handlers;
};

ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::Object>, "sound_irqs", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::Object>, "human_voice_sound__irqs", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::Object>, "human_melee_sound__irqs", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::Array>, "human_steps_sound__irqs", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bool, "is_watched_sound", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::EntityId, "gun__owner", nullptr, 0);

ECS_DECLARE_RELOCATABLE_TYPE(AnimcharSound);
ECS_REGISTER_RELOCATABLE_TYPE(AnimcharSound, nullptr);

ECS_DECLARE_RELOCATABLE_TYPE(HumanAnimcharSound);
ECS_REGISTER_RELOCATABLE_TYPE(HumanAnimcharSound, nullptr);


template <typename Handler>
static void register_type_irqs(ECS_SHARED(ecs::Object) irqs, Handler &handler, AnimV20::AnimcharBaseComponent &animchar)
{
  for (const auto &it : irqs)
  {
    const char *irqName = it.first.c_str();
    const ecs::Object &irqObj = it.second.get<ecs::Object>();
    const char *type = irqObj.getMemberOr(ECS_HASH("type"), "");
    const bool playBackward = irqObj.hasMember(ECS_HASH("playBackward"));
    handler.registerIrq(irqName, type, playBackward, animchar);
  }
}

template <typename Callable>
static void animchar_sound_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void human_animchar_sound_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
static void human_melee_sound_ecs_query(ecs::EntityId, Callable c);

ECS_TAG(sound)
ECS_ON_EVENT(on_appear)
static void animchar_sound_on_appear_es(
  const ecs::Event &, ecs::EntityId eid, AnimcharSound &animchar_sound, AnimV20::AnimcharBaseComponent &animchar)
{
  animchar_sound.handlers = eastl::make_unique<AnimcharSoundHandlers>(eid);
  animchar_sound.handlers->irqHandler.registerDebugAnimParamIrq(animchar);

  animchar_sound_ecs_query(eid,
    [&](ECS_SHARED(ecs::Object) sound_irqs) { register_type_irqs(sound_irqs, animchar_sound.handlers->irqHandler, animchar); });
}

ECS_TAG(sound)
ECS_ON_EVENT(on_appear)
static void human_animchar_sound_on_appear_es(
  const ecs::Event &, ecs::EntityId eid, HumanAnimcharSound &human_animchar_sound, AnimV20::AnimcharBaseComponent &animchar)
{
  human_animchar_sound.handlers = eastl::make_unique<HumanAnimcharSoundHandlers>(eid);
  human_animchar_sound.handlers->irqHandler.registerDebugAnimParamIrq(animchar);

  animchar_sound_ecs_query(eid,
    [&](ECS_SHARED(ecs::Object) sound_irqs) { register_type_irqs(sound_irqs, human_animchar_sound.handlers->irqHandler, animchar); });

  human_animchar_sound_ecs_query(eid,
    [&](ECS_SHARED(ecs::Object) human_voice_sound__irqs, ECS_SHARED(ecs::Array) human_steps_sound__irqs) {
      register_type_irqs(human_voice_sound__irqs, human_animchar_sound.handlers->voicefxHandler, animchar);

      int idx = -1;
      for (const auto &it : human_steps_sound__irqs)
      {
        ++idx;
        const ecs::Object &irqObj = it.get<ecs::Object>();
        const char *irqName = irqObj.getMemberOr(ECS_HASH("irq"), "");
#if DAGOR_DBGLEVEL > 0
        const char *prefix = (idx & 1) ? "right" : "left";
        if (strncmp(irqName, prefix, strlen(prefix)) != 0)
          logerr("Irq name '%s' in human_steps_sound__irqs should start with '%s'", irqName, prefix);
#endif
        const bool playBackward = irqObj.hasMember(ECS_HASH("playBackward"));
        human_animchar_sound.handlers->stepHandler.registerIrq(irqName, idx, playBackward, animchar);
      }
#if DAGOR_DBGLEVEL > 0
      if (human_steps_sound__irqs.size() & 1)
        logerr("Missing steps in human_steps_sound__irqs. Should contain pairs of steps");
#endif
    });

  human_melee_sound_ecs_query(eid, [&](ECS_SHARED(ecs::Object) human_melee_sound__irqs) {
    for (const auto &it : human_melee_sound__irqs)
      human_animchar_sound.handlers->meleeHandler.registerIrq(it.first.c_str(), animchar);
  });
}
