//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <math/dag_bounds3.h>
#include <math/dag_plane3.h>

class ShaderMaterial;


namespace waterobjects
{
struct Obj
{
  BBox3 box;
  Plane3 plane;
  ShaderMaterial *mat;
  void *handle;
};

//! adds water object to global list associated with handle
void add(void *handle, ShaderMaterial *mat, const BBox3 &box, const Plane3 &plane);
//! delete all water objects with handle
void del(void *handle);
//! delete all water objects
void del_all();

//! returns all water objects
dag::Span<Obj> get_list();
} // namespace waterobjects
