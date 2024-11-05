// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>

namespace PropPanel
{

class PropertyControlBase;

class PaletteCellControl
{
public:
  PaletteCellControl(PropertyControlBase &control_holder, int w, int h);

  void updateImgui(E3DCOLOR color, const char *name, bool selected);

private:
  PropertyControlBase &controlHolder;
  int width;
  int height;
};

} // namespace PropPanel