//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>

class RainDroplets
{
public:
  RainDroplets();
  void init();
  void setRainRate(float rain_rate);
  void close();

protected:
  TEXTUREID puddleMapTexId, /*car_dropletsTexId,*/ envi_dropletsTexId;
  int rain_scale_var_id;
  bool inited;
};
