// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <EASTL/functional.h>

class DagorAsset;
class TMatrix;

class InvisibleFacesHighlight
{
public:
  enum class State
  {
    Analyzing, // value is the percent done
    Ready,     // value is the number of transparent triangles
  };
  using StateCb = eastl::function<void(State state, int value)>;

  void setStateCallback(StateCb cb) { stateCb = eastl::move(cb); }
  void setAsset(const DagorAsset *asset);
  bool isSupported() const { return supported; }

  void render(int lod, const TMatrix &wtm);

private:
  struct LodGeometry
  {
    Tab<Point3> verts;
    bool cached = false;
  };

  void retrieveGeometry(int lod);

  const DagorAsset *asset = nullptr;
  Tab<LodGeometry> lodCache;
  StateCb stateCb;
  bool supported = false;
  bool isRendInst = false;
};
