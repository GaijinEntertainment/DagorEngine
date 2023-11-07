//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_mathBase.h>


class DataBlock;
class Point3;

class LeavesWindEffect
{
public:
  LeavesWindEffect();

  void initWind(const DataBlock &blk);

  void updateWind(real dt);

  void setShaderVars(const TMatrix &view_itm, real rocking_scale = 1, real rustling_scale = 1);

  static void setNoAnimShaderVars(const Point3 &colX, const Point3 &colY, const Point3 &colZ);


  static constexpr int WIND_GRP_NUM = 4;

protected:
  struct ScalarWindParams
  {
    real amp, pk, ik, dk;

    void init(const DataBlock &blk, real def_amp);
  };

  struct ScalarWindValue
  {
    real pos, target, vel;

    void init();
    void update(real dt, const ScalarWindParams &params, LeavesWindEffect &wind);
  };


  struct WindGroup
  {
    ScalarWindValue rocking, rustling;

    void init();
    void update(real dt, LeavesWindEffect &wind);

    void calcTm(Matrix3 &tm);
  };


  int windSeed;
  ScalarWindParams rocking, rustling;

  WindGroup windGrp[WIND_GRP_NUM];
};
