//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaders.h>
#include <poisson-disk-generator/PoissonGenerator.h>

//
// NOTE: Resulting 2d points are padded to be aligned to float4 values!
//
inline bool generate_poission_points(UniqueBufHolder &buf, uint32_t seed, int num_samples, const char *name)
{
  if (!VariableMap::isVariablePresent(get_shader_variable_id(name, true)))
    return false;

  int poissonIters = 1;

  std::vector<PoissonGenerator::Point> points;
  for (; points.size() != num_samples && poissonIters <= 100; ++seed, ++poissonIters)
  {
    PoissonGenerator::DefaultPRNG PRNG(seed);
    points = PoissonGenerator::generatePoissonPoints(num_samples, PRNG);
  }

  if (points.size() == num_samples)
    debug("%d poisson points has been found in %d iterations.", num_samples, poissonIters);
  else
  {
    debug("%d poisson points has not been found in 100 iterations. Distribution may slightly be biased!", num_samples);
    points.resize(num_samples);
  }

  buf.close();
  buf = dag::buffers::create_persistent_cb(points.size(), name);

  auto pointsGPU = lock_sbuffer<Point4>(buf.getBuf(), 0, points.size(), VBLOCK_DISCARD | VBLOCK_WRITEONLY);
  G_ASSERT_RETURN(pointsGPU, false);

  for (size_t index = 0; index < points.size(); index++)
  {
    pointsGPU[index].x = points[index].x;
    pointsGPU[index].y = points[index].y;
  }

  buf.setVar();

  return true;
}
