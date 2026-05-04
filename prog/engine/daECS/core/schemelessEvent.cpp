// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/schemelessEvent.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>

namespace ecs
{

SchemelessEvent::SchemelessEvent(ecs::event_type_t tp, ecs::Object &&data_) :
  SchemelessEvent(tp, EVFLG_DESTROY | EVFLG_SERIALIZE | EVFLG_SCHEMELESS)
{
  data = eastl::move(data_);
}

SchemelessEvent::SchemelessEvent(ecs::event_type_t tp, ecs::event_flags_t flags_) : ecs::Event(tp, sizeof(SchemelessEvent), flags_)
{
#if DAECS_EXTENSIVE_CHECKS
  ecs::EventsDB::event_id_t eventId = g_entity_mgr->getEventsDb().findEvent(tp);
  if (eventId != ecs::EventsDB::invalid_event_id)
  {
    ecs::event_flags_t eventFlag = g_entity_mgr->getEventsDb().getEventFlags(eventId);
    flags |= eventFlag & ecs::EVFLG_CASTMASK;
  }
  else
  {
    logerr("attempt to create unregistered SchemelessEvent with event type <0x%X>", tp);
    flags |= ecs::EVCAST_BOTH;
  }
#else
  flags |= ecs::EVCAST_BOTH;
#endif
}

void SchemelessEvent::destroy(ecs::EntityManager &, Event &e)
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERT_RETURN(ecs::is_schemeless_event(e), );
#endif
  static_cast<SchemelessEvent &>(e).~SchemelessEvent();
}

void SchemelessEvent::move_out(ecs::EntityManager &, void *__restrict allocateAt, Event &&from)
{
#if DAGOR_DBGLEVEL > 0
  if (!ecs::is_schemeless_event(from))
  {
    logerr("0x%X is not schemeless event", from.getType());
    memcpy(allocateAt, &from, sizeof(Event));
    memset(allocateAt, 0, 4); // overwrite type
    return;
  }
#endif
  new (allocateAt, _NEW_INPLACE) SchemelessEvent(static_cast<SchemelessEvent &&>(from));
  // work around, optimization, to prevent further call for destructor when event is moving from main queue to loadingEntities queue
  // destructor in that case WON'T be called, which is technically UB (but it is optimal behaviour, since we know there is nothing to
  // be done in destructor anyway)
  static_cast<SchemelessEvent &>(from).flags &= ~EVFLG_DESTROY;
}

} // namespace ecs
