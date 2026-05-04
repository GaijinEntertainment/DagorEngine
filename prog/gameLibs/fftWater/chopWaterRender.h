// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_postFxRenderer.h>
#include <heightmap/heightmapRenderer.h>
#include <heightmap/heightmapCulling.h>
#include <generic/dag_smallTab.h>
#include "waterCommon.h"
#include "waterRenderCommon.h"
#include <3d/dag_resPtr.h>
#include <shaders/dag_overrideStateId.h>
#include <fftWater/fftWater.h>
#include <fftWater/chopWaterGen.h>

class ShaderMaterial;
class ShaderElement;
class BaseTexture;
typedef BaseTexture Texture;
class BBox2;
struct IWaterDecalsRenderHelper;
class ComputeShaderElement;

class ChopWaterRender
{
public:
  static constexpr int MAX_NUM_CASCADES = WATER_VISUAL_CASCADES_COUNT;

  ChopWaterRender(ChopWaterGenerator &chop_gen, int render_quality, int geom_quality, bool depth_renderer, bool ssr_renderer,
    const fft_water::WaterHeightmap *water_heightmap = nullptr);
  ~ChopWaterRender();

  void reset();
  void reinit();

  void setLevel(float water_level);
  float getMinLevel() const { return minWaterLevel; }
  float getMaxLevel() const { return maxWaterLevel; }
  void setMinMaxLevel(float min_water_level, float max_water_level);
  void calcWaveHeight(float &out_max_wave_height, float &out_significant_wave_height);

  void render(const Point3 &origin, TEXTUREID distanceTex, int geom_lod_quality, int survey_id, const Frustum &frustum,
    const Driver3dPerspective &persp, const WaterRenderCommon &waterRenderCommon, IWaterDecalsRenderHelper *decals_renderer = NULL,
    fft_water::RenderMode render_mode = fft_water::RenderMode::WATER_SHADER);

  void setWaterDim(int dim_bits);
  int getGeomQuality() const { return geomQuality; }
  bool isDepthRendererEnabled() const { return depthRendererEnabled; }
  bool isSSRRendererEnabled() const { return ssrRendererEnabled; }
  void setRenderQuad(const BBox2 &b);
  void setWakeHtTex(TEXTUREID wake_ht_tex);
  void setLod0AreaSize(float size) { lod0AreaRadius = size; }
  float getLod0AreaSize() { return lod0AreaRadius; }
  void setGridLod0AdditionalTesselation(float additional_tesselation) { lod0TesselationAdditional = additional_tesselation; }

  void setForceTessellation(bool force) { forceTessellation = force; }

  void getGridDataAtCamera(float &gridAlign, Point2 &gridOffset)
  {
    gridAlign = cameraPatchAlign;
    gridOffset = cameraPatchOffset;
  }

  bool isDetailWavesEnabled() const;

protected:
  bool depthRendererEnabled;
  bool ssrRendererEnabled;
  int renderQuality;
  int geomQuality;
  HeightmapRenderer chopWaterRenderer[fft_water::RenderMode::MAX];
  float waterCellSize;

  ChopWaterGenerator &chopGen;

  float waterLevel;
  float minWaterLevel, maxWaterLevel;
  float maxWaveHeight, significantWaveHeight;
  float maxWaveSize[MAX_NUM_CASCADES];
  int numCascades; // todo: support using arbitrary number of cascades for rendering if perf is bad

  float cameraPatchAlign = 1.0f;
  Point2 cameraPatchOffset = ZERO<Point2>();

  BBox2 *renderQuad;

  bool forceTessellation;

  float lod0TesselationAdditional;
  float lod0AreaRadius;
  float lastLodExtension;

  shaders::UniqueOverrideStateId overrideAlpha, overrideRGB;

  const fft_water::WaterHeightmap *waterHeightmap = nullptr;
};
