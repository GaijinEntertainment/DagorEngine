// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <fftWater/fftWater.h>

class WaterFlowmapObstacles
{
public:
  struct GatherObstacleEventCtx
  {
    FFTWater &water;
    fft_water::WaterFlowmap &waterFlowmap;

    inline void addCircularObstacle(const BBox3 &box, const TMatrix &tm);
    inline void addRectangularObstacle(const BBox3 &box, const TMatrix &tm);
  };
};
