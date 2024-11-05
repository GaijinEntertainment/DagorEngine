// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>


ECS_REGISTER_EVENT(EventAnimIrq);


struct AnimIrqToEventComponent : public ecs::AnimIrqHandler
{
  AnimIrqToEventComponent(const ecs::EntityManager &, ecs::EntityId eid) : ecs::AnimIrqHandler(eid) {}

  int addIrqToHandle(AnimV20::AnimcharBaseComponent &animchar, const ecs::string &irq_type)
  {
    int irqType = AnimV20::addIrqId(irq_type.c_str());
    animchar.unregisterIrqHandler(irqType, this); // to avoide duplicate registrations
    animchar.registerIrqHandler(irqType, this);
    return irqType;
  }

  intptr_t irq(int type, intptr_t, intptr_t, intptr_t) override
  {
    g_entity_mgr->sendEvent(eid, EventAnimIrq(type));
    return 0;
  }
};


ECS_DECLARE_BOXED_TYPE(AnimIrqToEventComponent); // Boxed due to "registerIrqHandler" requiring unchanging "this" of component
ECS_REGISTER_BOXED_TYPE(AnimIrqToEventComponent, nullptr);
ECS_AUTO_REGISTER_COMPONENT(AnimIrqToEventComponent, "anim_irq_to_event_converter", nullptr, 0);


ECS_ON_EVENT(on_appear)
static inline void anim_init_irq_listener_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar,
  ecs::Object &anim_irq__eventNames, AnimIrqToEventComponent &anim_irq_to_event_converter)
{
  for (auto &event : anim_irq__eventNames)
    if (ecs::Object *irqEventObj = event.second.getNullable<ecs::Object>())
      if (const ecs::string *irqEventName = irqEventObj->getNullable<ecs::string>(ECS_HASH("irqName")))
        if (int *irqId = irqEventObj->getNullableRW<int>(ECS_HASH("irqId")))
          *irqId = anim_irq_to_event_converter.addIrqToHandle(animchar, *irqEventName);
}
