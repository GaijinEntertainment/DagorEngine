// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

namespace PropPanel
{

class PropertyControlBase;

class TwoColorIndicatorControl
{
public:
  explicit TwoColorIndicatorControl(PropertyControlBase &control_holder);

  void setOldColor(E3DCOLOR value) { colorOld = value; }
  void setNewColor(E3DCOLOR value) { colorNew = value; }

  void updateImgui(int width, int height);

private:
  void draw(int width, int height);

  PropertyControlBase &controlHolder;
  E3DCOLOR colorOld, colorNew;
  Point2 viewOffset;
};

} // namespace PropPanel