//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/*
//Example code
//Step 0: Call the init function once per application
  debuginputevents::init();

//Step 1: Create event handlers in a *ES.cpp.inl file
#include <ecs/input/debugInputEvents.h>

void keyboard_pressed_es_event_handler(const EventDebugKeyboardPressed &evt)
{
  if (evt.get<0>() == HumanInput::DKEY_LEFT)
    debug("Left button pressed");
}

Step 2: Build and run the game in dev or dbg mode, in release the events are turned off!
*/

#include <ecs/core/entityManager.h>
#include <humanInput/dag_hiKeybIds.h>  //For the keyboard enum
#include <humanInput/dag_hiMouseIds.h> //For the mouse enum

#define DEBUG_INPUT_ECS_EVENTS                                \
  DEBUG_INPUT_ECS_EVENT(EventDebugKeyboardPressed, uint32_t)  \
  DEBUG_INPUT_ECS_EVENT(EventDebugKeyboardHeld, uint32_t)     \
  DEBUG_INPUT_ECS_EVENT(EventDebugKeyboardReleased, uint32_t) \
  DEBUG_INPUT_ECS_EVENT(EventDebugMousePressed, uint32_t)     \
  DEBUG_INPUT_ECS_EVENT(EventDebugMouseHeld, uint32_t)        \
  DEBUG_INPUT_ECS_EVENT(EventDebugMouseReleased, uint32_t)    \
  DEBUG_INPUT_ECS_EVENT(EventDebugMouseMoved, Point2)

#define DEBUG_INPUT_ECS_EVENT ECS_BROADCAST_EVENT_TYPE
DEBUG_INPUT_ECS_EVENTS
#undef DEBUG_INPUT_ECS_EVENT

namespace debuginputevents
{
void init();
void close();
} // namespace debuginputevents
