// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "touchInput.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/event.h>


using namespace dainput;

TouchInput touch_input;


void TouchInput::pressButton(action_handle_t a)
{
  if (a == BAD_ACTION_HANDLE)
    return;
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_DIGITAL);

  if (a >= pressedButtons.size())
    pressedButtons.resize(a + 1);

  if (!pressedButtons.get(a))
  {
    pressedButtons.set(a);
    dainput::send_action_event(a);
  }
}


void TouchInput::releaseButton(action_handle_t a)
{
  if (a == BAD_ACTION_HANDLE)
    return;
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_DIGITAL);

  if (a >= pressedButtons.size())
    pressedButtons.resize(a + 1);

  if (pressedButtons.get(a))
  {
    pressedButtons.reset(a);
    dainput::send_action_terminated_event(a, 0);
  }
}


bool TouchInput::isButtonPressed(action_handle_t a)
{
  if ((get_action_type(a) & TYPEGRP__MASK) != TYPEGRP_DIGITAL)
    return false;
  return a != BAD_ACTION_HANDLE && a < pressedButtons.size() && pressedButtons.get(a);
}


void TouchInput::setStickValue(dainput::action_handle_t a, const Point2 &val)
{
  if (a == BAD_ACTION_HANDLE)
    return;
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_STICK);
  sticks[a] = val;
}


Point2 TouchInput::getStickValue(dainput::action_handle_t a)
{
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_STICK);
  eastl::hash_map<dainput::action_handle_t, Point2>::iterator it = sticks.find(a);
  if (it == sticks.end())
    return Point2(0, 0);
  return it->second;
}


void TouchInput::setAxisValue(dainput::action_handle_t a, const float &val)
{
  if (a == BAD_ACTION_HANDLE)
    return;
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_AXIS);
  axis[a] = val;
}


float TouchInput::getAxisValue(dainput::action_handle_t a)
{
  G_ASSERT((get_action_type(a) & TYPEGRP__MASK) == TYPEGRP_AXIS);
  eastl::hash_map<dainput::action_handle_t, float>::iterator it = axis.find(a);
  if (it == axis.end())
    return 0.0;
  return it->second;
}
