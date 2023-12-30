#pragma once
#include <daSkies2/daScattering.h>
#include <math/dag_TMatrix4.h>
#include <3d/dag_resPtr.h>

struct PreparedSkies
{
  uint32_t scatteringGeneration = 0;
  Point3 preparedScatteringOrigin = {-1000000, -1000000, 1000000};
  Point3 preparedScatteringPrimaryLight = {0, 0, 0}, preparedScatteringSecondaryLight = {0, 0, 0};
  float transmittanceAltitude = -1000000;
  float goodQualityDist = 80000.f; // depends on zfr
  float fullDist = 300000.f;       // depends on where we need it. For example, clouds.
  float rangeScale = 1.0f, minRange = 100.f;
  float froxelsMaxDist = 32000.f; // current
  float skiesLastTcZ = 1;
  Color4 precomputedTcDist = {1. / 80000, 0, 0, 0};

  UniqueTex preparedLoss;

  carray<UniqueTex, 2> scatteringVolume;            //-V730_NOINIT
  carray<UniqueTex, 2> skiesLutTex, skiesLutMieTex; //-V730_NOINIT
  uint32_t frame = 0;
  uint32_t lastSkiesPrepareFrame = 0;
  uint32_t resetGen = 0;
  bool skiesLutRestartTemporal = false;
  bool panoramic = false;

  Color4 skiesParams = {0, 0, 0, -1};
  Color4 prevSkiesParams = {0, 0, 0, -1};
  DPoint3 prevWorldPos = {0, 0, 0};
  TMatrix4 prevGlobTm = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  Point4 prevViewVecLT = {0, 0, 0, 0}, prevViewVecRT = {0, 0, 0, 0}, prevViewVecLB = {0, 0, 0, 0}, prevViewVecRB = {0, 0, 0, 0};
};
