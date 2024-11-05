// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
#include <sqrat.h>

class MinimapContext;

namespace darg
{
class Element;
}

class MinimapState
{
public:
  MinimapState(const Sqrat::Object &data);
  void calcItmFromView(TMatrix &itm, float vis_rad) const;
  void calcTmFromView(TMatrix &tm, float vis_rad) const;
  float getVisibleRadius() const;
  float getTargetVisibleRadius() const;
  float getMaxVisibleRadius() const;
  float setVisibleRadius(float r);
  void setTargetVisibleRadius(float r);
  void setInteractive(bool on);
  Point3 mapToWorld(float x, float y);
  Point2 worldToMap(const Point3 &world_pos);

  static MinimapState *get_from_element(const darg::Element *elem);

public:
  bool isHeroCentered = true;
  Point3 panWolrdPos = Point3(0, 0, 0);
  bool isSquare = false;
  bool isDragging = false;
  bool isViewUp = false;
  bool isInteractive = false;
  short prevMx = 0, prevMy = 0;
  MinimapContext *ctx = nullptr;

private:
  float baseVisibleRadius = 0;
  float currentVisibleRadius = 0;
  float targetVisibleRadius = 0;
};
