// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/computeShaderFallback/voltexRenderer.h>
#include <3d/dag_eventQueryHolder.h>

#include <shaders/dag_DynamicShaderHelper.h>
#include <daSkies2/daSkies.h>

#include "clouds2Common.h"

class CloudsField
{
public:
  bool getReadbackData(float &alt_start, float &alt_top) const;

  void init();
  void layersHeightsBarrier();
  void setParams(const DaSkies::CloudsSettingsParams &params);
  void invalidate();

  CloudsChangeFlags render();
  void renderCloudVolume(VolTexture *cloud_volume, float max_dist, const TMatrix &view_tm, const TMatrix4 &proj_tm);

private:
  void initCloudsVolumeRenderer();
  void doReadback();

  void closedCompressed();
  void initCompressed();

  void genFieldGeneral(VoltexRenderer &renderer, DynamicShaderHelper non_empty_fill, ManagedTex &voltex);

  void genFieldCompressed();
  void genField();

  void initDownsampledField();
  void renderDownsampledField();

  UniqueTex cloudsFieldVolCompressed;
  UniqueTexHolder cloudsFieldVolTemp;
  UniqueTexHolder cloudsFieldVol, cloudsDownSampledField;
  UniqueTexHolder layersPixelCount, layersHeights;

  eastl::unique_ptr<ComputeShaderElement> build_dacloud_volume_cs;
  eastl::unique_ptr<ComputeShaderElement> refineAltitudes;

  PostFxRenderer build_dacloud_volume_ps;
  PostFxRenderer refineAltitudesPs;

  VoltexRenderer genCloudsField, genCloudsFieldCmpr, downsampleCloudsField;
  DynamicShaderHelper genCloudLayersNonEmpty, genCloudLayersNonEmptyCmpr;

  bool frameValid = false;

  int resXZ = 512, resY = 64;
  int targetXZ = 512, targetY = 64;
  int downsampleRatio = 8;
  float averaging = 0.65;
  bool useCompression = false;

  struct
  {
    EventQueryHolder query;
    float layerAltStart = 0, layerAltTop = 0;
    bool valid = false;
  } readbackData;
};
