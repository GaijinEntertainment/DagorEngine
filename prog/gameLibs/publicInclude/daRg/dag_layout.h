//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <daRg/dag_guiConstants.h>

#include <sqrat.h>


namespace darg
{

class StringKeys;
class Style;
class Element;
class Properties;


struct SizeSpec
{
  enum Mode
  {
    PIXELS,
    FLEX,
    CONTENT,
    FONT_H,
    PARENT_W,
    PARENT_H,
    ELEM_SELF_W,
    ELEM_SELF_H
  } mode;
  float value;

  void reset(Mode mode_ = PIXELS)
  {
    mode = mode_;
    value = 0;
  }

  SizeSpec() { reset(); }
};


class Offsets : public Point4
{
public:
  Offsets() : Point4(0, 0, 0, 0) {}
  Offsets(real ax, real ay, real az, real aw) : Point4(ax, ay, az, aw) {}

  real top() const { return x; }
  real right() const { return y; }
  real bottom() const { return z; }
  real left() const { return w; }

  Point2 lt() const { return Point2(left(), top()); }
  Point2 rb() const { return Point2(right(), bottom()); }
};


struct Layout
{
  SizeSpec pos[2];
  SizeSpec size[2];
  LayoutFlow flowType;
  ElemAlign hAlign : 4;
  ElemAlign vAlign : 4;
  ElemAlign hPlacement : 4;
  ElemAlign vPlacement : 4;
  Offsets margin;
  Offsets padding;
  float gap;
  SizeSpec minSize[2];
  SizeSpec maxSize[2];

public:
  Layout();

  bool readPos(const Sqrat::Object &pos_obj, const char **err_msg);
  bool readSize(const Sqrat::Object &size_obj, const char **err_msg);

  void read(const Element *elem, const Properties &props, const StringKeys *csk);
  void invalidateSize();
  void invalidatePos();

  static bool size_spec_from_array(const Sqrat::Array &arr, SizeSpec res[2], const char **err_msg);
  static bool size_spec_from_obj(const Sqrat::Object &obj, SizeSpec &res, const char **err_msg);
};

} // namespace darg
