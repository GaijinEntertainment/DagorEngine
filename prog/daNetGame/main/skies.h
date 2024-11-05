// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;
class DPoint2;
struct DngSkies;
DngSkies *get_daskies();
void load_daskies(const DataBlock &blk,
  float time_of_day = -1.f,
  const char *weather_blk = nullptr,
  int year = 1941,
  int month = 6,
  int day = 21,
  float lat = 55,
  float lon = 37,
  int seed = -1);
void save_daskies(DataBlock &blk);
void before_render_daskies();
DPoint2 get_strata_clouds_origin();
DPoint2 get_clouds_origin();
float get_daskies_time();
void set_daskies_time(float time_of_day);
void set_strata_clouds_origin(DPoint2 pos);
void set_clouds_origin(DPoint2 pos);