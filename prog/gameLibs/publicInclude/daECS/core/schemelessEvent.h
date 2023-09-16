//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/event.h>
#include <daECS/core/baseComponentTypes/objectType.h>

namespace danet
{
class BitStream;
}
namespace net
{
class IConnection;
}

namespace ecs
{

struct SchemelessEvent;

struct SchemelessEvent : public ecs::Event
{
protected:
  ecs::Object data; // actual payload (might be empty, thus not requiring destructor)
public:
  const ecs::Object &getData() const { return data; }
  ecs::Object &getRWData() { return data; }
  SchemelessEvent(ecs::event_type_t tp, ecs::Object &&data_);
  // TODO: set correct cast flag instead of EVCAST_BOTH
  SchemelessEvent(ecs::event_type_t tp) : SchemelessEvent(tp, EVFLG_SERIALIZE | EVFLG_SCHEMELESS | EVCAST_BOTH) {}
  SchemelessEvent(ecs::event_type_t tp, ecs::event_flags_t flags);

  static void destroy(Event &e);
  static void move_out(void *__restrict allocateAt, Event &&from);
  static const char *staticName() { return nullptr; }
  static constexpr event_flags_t staticFlags() { return EVFLG_SCHEMELESS | EVFLG_DESTROY; }
};

inline bool is_schemeless_event(const Event &evt) { return (evt.getFlags() & EVFLG_SCHEMELESS) ? true : false; }

} // namespace ecs
