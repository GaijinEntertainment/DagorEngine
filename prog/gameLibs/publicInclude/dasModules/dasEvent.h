//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>

namespace bind_dascript
{

struct DasEvent : public ecs::Event
{
  DasEvent() = delete;

  static constexpr const char *staticName() { return "DasEvent"; }

  void set(ecs::event_type_t tp, ecs::event_size_t l, ecs::event_flags_t fl)
  {
    type = tp;
    length = l;
    flags = fl;
  }
};

}; // namespace bind_dascript
