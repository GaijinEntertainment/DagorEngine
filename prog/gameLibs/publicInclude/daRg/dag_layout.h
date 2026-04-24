//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <cstddef>
#include <math/dag_Point2.h>
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


struct Offsets
{
  float t = 0, r = 0, b = 0, l = 0;

  float *ptr() { return &t; }
  const float *ptr() const { return &t; }
  Point2 lt() const { return Point2(l, t); }
  Point2 rb() const { return Point2(r, b); }
};
static_assert(offsetof(Offsets, l) == 3 * sizeof(float), "Offsets fields must be contiguous");


struct LayoutExtended
{
  Offsets margin;
  Offsets padding;
  SizeSpec minSize[2];
  SizeSpec maxSize[2];
};


struct Layout
{
  SizeSpec pos[2];
  SizeSpec size[2];
  LayoutExtended *ext;
  float gap;
  LayoutFlow flowType;
  ElemAlign hAlign : 4;
  ElemAlign vAlign : 4;
  ElemAlign hPlacement : 4;
  ElemAlign vPlacement : 4;

  Layout();
  ~Layout();

  Layout(const Layout &) = delete;
  Layout &operator=(const Layout &) = delete;

  // Null-safe read accessors (return defaults when ext is null)
  const Offsets &margin() const { return ext ? ext->margin : zeroOffsets; }
  const Offsets &padding() const { return ext ? ext->padding : zeroOffsets; }
  const SizeSpec &minSize(int axis) const { return ext ? ext->minSize[axis] : defaultSizeSpec; }
  const SizeSpec &maxSize(int axis) const { return ext ? ext->maxSize[axis] : defaultSizeSpec; }

  // Mutable ext accessors (allocate on first use)
  LayoutExtended *ensureExt();
  Offsets &marginMut() { return ensureExt()->margin; }
  Offsets &paddingMut() { return ensureExt()->padding; }
  SizeSpec &minSizeMut(int axis) { return ensureExt()->minSize[axis]; }
  SizeSpec &maxSizeMut(int axis) { return ensureExt()->maxSize[axis]; }

  bool readPos(const Sqrat::Object &pos_obj, const char **err_msg);
  bool readSize(const Sqrat::Object &size_obj, const char **err_msg);

  void read(const Element *elem, const Properties &props, const StringKeys *csk);

  static bool read_size(const Sqrat::Object &size_obj, SizeSpec res[2], const char **err_msg);
  static bool size_spec_from_array(const Sqrat::Array &arr, SizeSpec res[2], const char **err_msg);
  static bool size_spec_from_obj(const Sqrat::Object &obj, SizeSpec &res, const char **err_msg);

private:
  static const Offsets zeroOffsets;
  static const SizeSpec defaultSizeSpec;
};

static_assert(int(_NUM_ALIGNMENTS) <= 16, "ElemAlign must fit in 4 bits");

} // namespace darg
