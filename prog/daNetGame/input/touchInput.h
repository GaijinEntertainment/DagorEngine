// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daInput/input_api.h>
#include <math/dag_Point2.h>

#include <util/dag_bitArray.h>
#include <EASTL/hash_map.h>

struct TouchInput
{
public:
  TouchInput() = default;

  void pressButton(dainput::action_handle_t a);
  void releaseButton(dainput::action_handle_t a);
  bool isButtonPressed(dainput::action_handle_t a);

  void setStickValue(dainput::action_handle_t a, const Point2 &val);
  Point2 getStickValue(dainput::action_handle_t a);

  void setAxisValue(dainput::action_handle_t a, const float &val);
  float getAxisValue(dainput::action_handle_t a);

private:
  Bitarray pressedButtons;
  eastl::hash_map<dainput::action_handle_t, Point2> sticks; // TODO may be use AnalogStickAction ?
  eastl::hash_map<dainput::action_handle_t, float> axis;    // TODO may be use AnalogAxisAction ?
};


extern TouchInput touch_input;
