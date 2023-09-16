//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <astronomy/astronomy.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <math/random/dag_random.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>

static inline float get_local_time_of_day_exact(const char *env_time_str, float range_f, float latitude, int year, int month, int day)
{
  const DataBlock *timesBlk = ::dgs_get_game_params()->getBlockByNameEx("timesOfDay");
  Point2 timeRange = timesBlk->getPoint2(String(env_time_str).toLower().str(), Point2(0.4f, 0.4f));
  return calculate_local_time_of_day(latitude, year, month, day, lerp(timeRange.x, timeRange.y, range_f));
}
static inline float get_local_time_of_day_random(const char *env_time_str, int seed, float latitude, int year, int month, int day)
{
  return get_local_time_of_day_exact(env_time_str, _frnd(seed), latitude, year, month, day);
}
