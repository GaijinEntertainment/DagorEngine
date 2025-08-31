//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_bounds3.h>

class ILandmesh
{
public:
  static constexpr unsigned HUID = 0xB73CF653u; // ILandmesh

  virtual BBox3 getBBoxWithHMapWBBox() const = 0;
  virtual bool isLandmeshRenderingMode() const = 0;
};