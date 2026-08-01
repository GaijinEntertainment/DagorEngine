//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <anim/dag_animDecl.h>

class GeomNodeTree;

namespace AnimV20
{

class IAnimCharPostController
{
public:
  virtual void onPostAnimBlend(GeomNodeTree & /*tree*/, vec3f /*world_translate*/) {}
  virtual bool overridesBlender() const { return false; }

  virtual void update(float dt, GeomNodeTree &tree, AnimcharBaseComponent *ac) = 0;
};

} // namespace AnimV20
