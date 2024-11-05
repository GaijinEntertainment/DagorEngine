//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_eventQueryHolder.h>
#include <util/dag_baseDef.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_carray.h>
#include <math/dag_color.h>
#include <shaders/dag_computeShaders.h>

class SphHarmCalc
{
public:
  enum EValueId
  {
    SH3_SH00 = 0,
    SH3_SH1M1 = 1,
    SH3_SH10 = 2,
    SH3_SH1P1 = 3,
    SH3_SH2M2 = 4,
    SH3_SH2M1 = 5,
    SH3_SH20 = 6,
    SH3_SH2P1 = 7,
    SH3_SH2P2 = 8,

    SH3_COUNT = 9
  };

  static constexpr int SPH_COUNT = 7;

  typedef carray<Color4, SH3_COUNT> values_t;
  typedef carray<Color4, SPH_COUNT> valuesOpt_t;

  enum EnviromentTextureType
  {
    CubeMap,
    Panorama
  };

  SphHarmCalc(float hdr_scale = 1.0f);
  void setHdrScale(float hdr_scale) { hdrScale = hdr_scale; }
  // if gamma is 1.0, than target is linear
  // if gamma is 2.2, than target data is first going to be mul by hdr_scale and then raised in 2.2 power
  void processCubeFace(Texture *texture, int face, int width, float gamma); // add harmonics from one side
  const values_t &getValues() const { return values; }
  values_t &changeValues() { return values; }

  const valuesOpt_t &getValuesOpt() const { return valuesOpt; }
  valuesOpt_t &changeValuesOpt() { return valuesOpt; }
  void finalize();
  void reset(); // reset harmonics
  bool isValid();

  bool processCubeData(CubeTexture *texture, float gamma, bool force_recalc);
  bool processPanoramaData(Texture *texture, float gamma, bool force_recalc);
  void processFaceData(uint8_t *buf, int face, int width, int stride, uint32_t fmt, float gamma);
  bool processFaceDataGpu(Texture *texture, int face, int width, int level_count, float gamma, bool force_recalc,
    EnviromentTextureType envi_type);
  inline void calcValues(const Color4 &color, const Point3 &norm, float domega); // requires finalize
  static Color3 calcFunc(dag::ConstSpan<Color4> values, const Point3 &norm);
  bool hasNans = false;

private:
  void invalidate();
  bool gpuFinalReduction(int width, bool force_recalc);
  void getValuesFromGpu(bool *should_retry = nullptr);

  bool valid;
  float hdrScale;
  values_t values;
  valuesOpt_t valuesOpt;
  int previousRunSize = -1;

  using SbufDeleter = DestroyDeleter<Sbuffer>;
  eastl::unique_ptr<ComputeShaderElement> spHarmReductor;
  eastl::unique_ptr<ComputeShaderElement> spHarmFinalReductor;
  eastl::unique_ptr<Sbuffer, SbufDeleter> harmonicsPreSum, harmonicsSum;
  EventQueryHolder valueComputeFence;
  bool inProgress = false;
  uint32_t attemptsToLock = 0;
};
