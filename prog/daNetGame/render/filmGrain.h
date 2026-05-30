// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point4.h>

struct FilmGrainLutHolder
{
  static constexpr int SLICES_PER_STEP = 1;
  static constexpr int SETTINGS_PRIORITY = 0;
  static constexpr int EXTERNAL_PRIORITY = 1;
  static constexpr int GEN_PRIORITY = 2;

  int lutWH = 256;
  int lutD = 64;
  Point4 genParams = Point4(0, 0, 0, 0);

  bool rebuildRequested = false;

  UniqueTexWithShaderVar lut;
  eastl::unique_ptr<ComputeShaderElement> genCs;
  int genSlice = -1;

  void requestRebuild();
  void setSettings(const Point4 &value, int prio, int wh, int d, const Point4 &gen_params);
  void disable();
  void reinitFromSettings(int overrideWH, int overrideD, const Point4 &gen_params);
  bool generate(); // returns true when generation is complete (or not needed)
  void afterDeviceReset();

  static void enableExternalModifier(const Point4 &value);
  static void disableExternalModifier();
};
