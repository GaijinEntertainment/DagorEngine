//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_resId.h>

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
