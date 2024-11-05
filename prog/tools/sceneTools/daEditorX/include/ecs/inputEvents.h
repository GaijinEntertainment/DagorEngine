//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>

enum class MouseButton
{
  LeftButton,
  RightButton,
  MiddleButton
};

struct OnMouseDown : public ecs::Event
{
  MouseButton button;
  int x, y;
  ECS_BROADCAST_EVENT_DECL(OnMouseDown)
  OnMouseDown(MouseButton button, int x, int y) : ECS_EVENT_CONSTRUCTOR(OnMouseDown), button(button), x(x), y(y) {}
};

struct OnMouseUp : public ecs::Event
{
  MouseButton button;
  int x, y;
  ECS_BROADCAST_EVENT_DECL(OnMouseUp)
  OnMouseUp(MouseButton button, int x, int y) : ECS_EVENT_CONSTRUCTOR(OnMouseUp), button(button), x(x), y(y) {}
};

#define ON_MOUSE_EVENT(EVENT_NAME)                                           \
  struct EVENT_NAME : public ecs::Event                                      \
  {                                                                          \
    int x, y;                                                                \
    ECS_BROADCAST_EVENT_DECL(EVENT_NAME)                                     \
    EVENT_NAME(int x, int y) : ECS_EVENT_CONSTRUCTOR(EVENT_NAME), x(x), y(y) \
    {}                                                                       \
  };

ON_MOUSE_EVENT(OnMouseDownLeft)
ON_MOUSE_EVENT(OnMouseDownRight)
ON_MOUSE_EVENT(OnMouseDownMiddle)
ON_MOUSE_EVENT(OnMouseUpLeft)
ON_MOUSE_EVENT(OnMouseUpRight)
ON_MOUSE_EVENT(OnMouseUpMiddle)

struct OnMouseWheel : public ecs::Event
{
  int delta;
  ECS_BROADCAST_EVENT_DECL(OnMouseWheel)
  OnMouseWheel(int delta) : ECS_EVENT_CONSTRUCTOR(OnMouseWheel), delta(delta) {}
};
