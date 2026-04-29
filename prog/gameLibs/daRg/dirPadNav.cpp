// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dirPadNav.h"
#include "inputStack.h"
#include <daRg/dag_behavior.h>
#include <drv/hid/dag_hiXInputMappings.h>


namespace darg
{

namespace dirpadnav
{

Element *dir_pad_new_pos_trace(const InputStack &inputStack, const Point2 &pt)
{
  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->hitTest(pt))
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_POINTING))
        return ie.elem;
    }
  }
  return nullptr;
}

// return absolute value
static Point2 distance_to_box(const Point2 &p, const BBox2 &bbox)
{
  Point2 d(0, 0);

  if (bbox.left() > p.x)
    d.x = bbox.left() - p.x;
  else if (bbox.right() < p.x)
    d.x = p.x - bbox.right();

  if (bbox.top() > p.y)
    d.y = bbox.top() - p.y;
  else if (bbox.bottom() < p.y)
    d.y = p.y - bbox.bottom();

  return d;
}

static float weighted_nav_distance(float dx, float dy)
{
  return sqrtf(dx * dx + dy * dy);
  // return powf(dx*dx + dy*dy, 0.75f);
}


static float out_of_cone_penalty(float dist) { return dist * 5.0f + 10000.0f; }

static float other_side_penalty(float dist) { return dist * 20.0f + 50000.0f; }


float calc_dir_nav_distance(const Point2 &from, const BBox2 &bbox, Direction dir, bool allow_reverse)
{
  const float anisotropyMul = 4.0f;
  const float coneAtanLim = 1.2f;

  float distance = 1e9f;
  ;
  Point2 delta = distance_to_box(from, bbox);
  if (dir == DIR_RIGHT)
  {
    float d = weighted_nav_distance(delta.x, delta.y * anisotropyMul);
    if (bbox.left() > from.x)
    {
      if (delta.y > delta.x * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else if (dir == DIR_LEFT)
  {
    float d = weighted_nav_distance(delta.x, delta.y * anisotropyMul);
    if (bbox.right() < from.x)
    {
      if (delta.y > delta.x * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else if (dir == DIR_DOWN)
  {
    float d = weighted_nav_distance(delta.x * anisotropyMul, delta.y);
    if (bbox.top() > from.y)
    {
      if (delta.x > delta.y * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else
  {
    G_ASSERT(dir == DIR_UP);
    float d = weighted_nav_distance(delta.x * anisotropyMul, delta.y);
    if (bbox.bottom() < from.y)
    {
      if (delta.x > delta.y * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  return distance;
}


bool is_dir_pad_button(int btn_idx)
{
  using namespace HumanInput;

  return btn_idx == JOY_XINPUT_REAL_BTN_D_UP || btn_idx == JOY_XINPUT_REAL_BTN_D_DOWN || btn_idx == JOY_XINPUT_REAL_BTN_D_LEFT ||
         btn_idx == JOY_XINPUT_REAL_BTN_D_RIGHT;
}


bool is_stick_dir_button(int btn_idx)
{
  using namespace HumanInput;

  return btn_idx == JOY_XINPUT_REAL_BTN_L_THUMB_UP || btn_idx == JOY_XINPUT_REAL_BTN_L_THUMB_DOWN ||
         btn_idx == JOY_XINPUT_REAL_BTN_L_THUMB_LEFT || btn_idx == JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT;
}


Direction gamepad_button_to_dir(int btn_idx)
{
  using namespace HumanInput;

  bool isDirPad = is_dir_pad_button(btn_idx);
  bool isStickNav = is_stick_dir_button(btn_idx);
  G_ASSERT_RETURN(isDirPad || isStickNav, Direction(-1)); //-V1016

  static constexpr Direction stickDirs[] = {DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN};
  return Direction(isDirPad ? btn_idx : stickDirs[btn_idx - JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT]);
}

} // namespace dirpadnav

} // namespace darg
