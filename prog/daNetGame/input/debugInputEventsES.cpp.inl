// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/input/debugInputEvents.h>
#include <daECS/core/coreEvents.h>

ECS_TAG(dev, gameClient)
ECS_REQUIRE(ecs::Tag debugInputEventsTag)
static inline void debug_input_events_init_es_event_handler(const ecs::EventEntityCreated &) { debuginputevents::init(); }

ECS_TAG(dev, gameClient)
ECS_REQUIRE(ecs::Tag debugInputEventsTag)
static inline void debug_input_events_close_es_event_handler(const ecs::EventEntityDestroyed &) { debuginputevents::close(); }