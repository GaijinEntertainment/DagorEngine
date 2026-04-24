// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_guiConstants.h>

class Point2;
class BBox2;

namespace darg
{

class InputStack;
class Element;

namespace dirpadnav
{

Element *dir_pad_new_pos_trace(const InputStack &inputStack, const Point2 &pt);
float calc_dir_nav_distance(const Point2 &from, const BBox2 &bbox, Direction dir, bool allow_reverse);
Direction gamepad_button_to_dir(int btn_idx);
bool is_dir_pad_button(int btn_idx);
bool is_stick_dir_button(int btn_idx);

} // namespace dirpadnav

} // namespace darg
