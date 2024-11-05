//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driver.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_lockSbuffer.h>
#include <ioSys/dag_dataBlock.h>

#include <poisson-disk-generator/PoissonGenerator.h>

//
// NOTE: Resulting 2d points are padded to be aligned to float4 values!
//
inline UniqueBuf generate_poisson_points_buffer(uint32_t seed, int num_samples, const char *name, const Point2 &offset_mul,
  const Point2 &offset_add)
{
  int poissonIters = 1;

  std::vector<PoissonGenerator::Point> points;
  for (; points.size() != num_samples && poissonIters <= 100; ++seed, ++poissonIters)
  {
    PoissonGenerator::DefaultPRNG PRNG(seed);
    points = PoissonGenerator::generatePoissonPoints(num_samples, PRNG);
  }

  static bool loggingEnabled = dgs_get_settings()->getBlockByNameEx("debug")->getBool("view_resizing_related_logging_enabled", true);

  if (points.size() == num_samples)
  {
    if (loggingEnabled)
      debug("%d poisson points has been found in %d iterations.", num_samples, poissonIters);
  }
  else
  {
    if (loggingEnabled)
      debug("%d poisson points has not been found in 100 iterations. Distribution may slightly be biased!", num_samples);
    points.resize(num_samples);
  }

  auto buf = UniqueBuf(dag::buffers::create_persistent_cb(points.size(), name));

  if (auto pointsGPU = lock_sbuffer<Point4>(buf.getBuf(), 0, points.size(), VBLOCK_DISCARD | VBLOCK_WRITEONLY))
  {
    for (size_t index = 0; index < points.size(); index++)
    {
      pointsGPU[index].x = points[index].x * offset_mul.x + offset_add.x;
      pointsGPU[index].y = points[index].y * offset_mul.y + offset_add.y;
    }
  }

  return buf;
}

//
// NOTE: Resulting 2d points are padded to be aligned to float4 values!
//
inline bool generate_poission_points(UniqueBufHolder &buf, uint32_t seed, int num_samples, const char *name, const char *var_name,
  const Point2 &offset_mul, const Point2 &offset_add)
{
  if (!VariableMap::isVariablePresent(get_shader_variable_id(var_name, true)))
    return false;

  buf.close();
  buf = UniqueBufHolder(generate_poisson_points_buffer(seed, num_samples, name, offset_mul, offset_add), var_name);
  G_ASSERT_RETURN(buf, false);

  buf.setVar();

  return true;
}

inline bool generate_poission_points(UniqueBufHolder &buf, uint32_t seed, int num_samples, const char *name, const char *var_name)
{
  return generate_poission_points(buf, seed, num_samples, name, var_name, Point2::ONE, Point2::ZERO);
}
