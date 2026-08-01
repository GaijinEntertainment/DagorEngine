// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/computeShaderFallback/voltexRenderer.h>
#include <3d/dag_eventQueryHolder.h>

#include <shaders/dag_DynamicShaderHelper.h>
#include <daSkies2/daSkies.h>
#include <render/nodeBasedShader.h>
#include <EASTL/unique_ptr.h>

#include "clouds2Common.h"

class DataBlock;

class CloudsField
{
public:
  bool getReadbackData(float &alt_start, float &alt_top) const;

  void init();
  void layersHeightsBarrier();
  void setParams(const DaSkies::CloudsSettingsParams &params);
  void invalidate();


  void initNBS(const String &root_graph);
  bool updateNBSShaders(const String &shader_name, const DataBlock &shader_blk, String &out_errors);
  void enableNBSOptionalGraph(const String &graph_name, bool enable);
  void closeNBS();
  bool isNBSActive() const;

  CloudsChangeFlags render();
  void renderCloudVolume(VolTexture *cloud_volume, TEXTUREID prev_cloud_volume, float max_dist, const TMatrix &view_tm,
    const TMatrix4 &proj_tm, const TMatrix4 &prev_glob_tm);

private:
  void initCloudsVolumeRenderer();
  void doReadback();

  void closedCompressed();
  void initCompressed();

  void genFieldGeneral(VoltexRenderer &renderer, DynamicShaderHelper non_empty_fill, ManagedTex &voltex);

  void copyFieldCompressed();
  void genFieldCompressed();
  void genField();
  void genFieldNBS();

  void initDownsampledField();
  void renderDownsampledField();

  UniqueTex cloudsFieldVolCompressed;
  UniqueTexWithShaderVar cloudsFieldVolTemp;
  UniqueTexWithShaderVar cloudsFieldVol, cloudsDownSampledField;
  UniqueTexWithShaderVar layersPixelCount, layersHeights;

  eastl::unique_ptr<ComputeShaderElement> build_dacloud_volume_cs;
  eastl::unique_ptr<ComputeShaderElement> refineAltitudes;

  eastl::unique_ptr<NodeBasedShader> nbsFieldCs;
  eastl::unique_ptr<NodeBasedShader> nbsFieldCompressedCs;
  bool useNBS = false;

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
  int frameNo = 0;

  struct
  {
    EventQueryHolder query;
    float layerAltStart = 0, layerAltTop = 0;
    bool valid = false;
  } readbackData;
};
