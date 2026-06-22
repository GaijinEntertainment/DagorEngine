// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_resPtr.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_Point4.h>

class FilmGrainLutHolder
{
public:
  static void enable_external_modifier(const Point4 &value);
  static void disable_external_modifier();

  FilmGrainLutHolder();
  FilmGrainLutHolder(const FilmGrainLutHolder &) = delete;
  FilmGrainLutHolder &operator=(const FilmGrainLutHolder &) = delete;
  FilmGrainLutHolder(FilmGrainLutHolder &&) = delete;
  FilmGrainLutHolder &operator=(FilmGrainLutHolder &&) = delete;
  ~FilmGrainLutHolder();


  void setLutSettings(int wh, int d, const Point4 &gen_params);
  void reinitFromSettings(bool preset_allows);
  void setParamsFromSettings(const Point4 &value);
  void resetParamsFromSettings();
  void setExternalParams(const Point4 &value);
  void resetExternalParams();
  bool generate(); // returns true when generation is complete (or not needed)
  void afterDeviceReset();

private:
  static constexpr int SLICES_PER_STEP = 1;
  static constexpr int SETTINGS_PRIORITY = 0;
  static constexpr int EXTERNAL_PRIORITY = 1;
  static constexpr int GEN_PRIORITY = 2;

  UniqueTexWithShaderVar lut;
  eastl::unique_ptr<ComputeShaderElement> genCs;

  int lutWH = 256;
  int lutD = 64;
  Point4 genParams = Point4(0, 0, 0, 0);

  bool enabledFromSettings = false;
  bool enabledFromExternalModifier = false;

  int genSlice = -1;
  bool rebuildRequested = false;

  [[nodiscard]] bool isLutNeeded() const { return enabledFromSettings || enabledFromExternalModifier; }
  void requestRebuild();
  void resetLut();
  void setFilmGrainReady(bool is_ready);
};
