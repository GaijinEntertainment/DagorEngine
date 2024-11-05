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
#include <3d/dag_resPtr.h>
#include <shaders/dag_overrideStateId.h>
#include <fftWater/fftWater.h>


class NVWaveWorks_FFT_CPU_Simulation;
class ShaderMaterial;
class ShaderElement;
class BaseTexture;
typedef BaseTexture Texture;
class WaterNVPhysics;
class BBox2;
struct IWaterDecalsRenderHelper;
class GPGPUData;
class CSGPUData;
class ComputeShaderElement;

// FFT
class WaterNVRender
{
public:
  static constexpr int MAX_NUM_CASCADES = fft_water::MAX_NUM_CASCADES;

  typedef fft_water::FoamParams FoamParams;

  const FoamParams &getFoamParams() const { return foamParams; }
  void setFoamParams(const FoamParams &foam_params) { foamParams = foam_params; }
  void setAnisotropy(int aniso, float mip_bias);
  int getAnisotropy() const { return anisotropy; };
  float getMipBias() const { return mipBias; };
  void setSmallWaveFraction(float smallWaveFraction);
  const fft_water::SimulationParams &getSimulationParams() { return fftSimulationParams; }
  void setSimulationParams(const fft_water::SimulationParams &params) { fftSimulationParams = params; }

  float getCascadeWindowLength() const { return cascadeWindowLength; }
  void setCascadeWindowLength(float value) { cascadeWindowLength = value; }

  float getCascadeFacetSize() const { return cascadeFacetSize; }
  void setCascadeFacetSize(float value) { cascadeFacetSize = value; }

#if DAGOR_DBGLEVEL > 0
  void enableGraphicFeature(int feature, bool enable);
  void getCascadePeriod(int cascade_no, float &out_period, float &out_window_in, float &out_window_out);
#endif

  WaterNVRender(const NVWaveWorks_FFT_CPU_Simulation::Params &p, const fft_water::SimulationParams &simulation, int quality,
    int geom_quality, bool depth_renderer, bool ssr_renderer, bool one_to_four_cascades, int num_cascades, float cascade_window_length,
    float cascade_facet_size, const fft_water::WaterHeightmap *water_heightmap);
  ~WaterNVRender();

  void reset();
  uint32_t getFormatForFoam();
  void initFoam();
  void closeFoam();
  void enableTurbulenceFoam(bool value) { turbulenceFoamEnabled = value && renderQuality >= fft_water::RENDER_GOOD; }
  void reinit(const Point2 &wind_dir, float wind_speed, float period);

  void setLevel(float water_level);
  const Point2 &getWaveDisplacementDistance() const { return waveDisplacementDist; }
  void setWaveDisplacementDistance(const Point2 &value);
  void calcWaveHeight(float &out_max_wave_height, float &out_significant_wave_height);
  void initRefraction(int w, int h);
  void closeRefraction();

  void simulateAllAt(double time); // SYNC!
  void updateTexturesAll();        // SYNC!
  void calculateGradients();
  void calculateCascadesRoughness();

  void prepareRefraction(Texture *scene_target_tex);

  void render(const Point3 &origin, TEXTUREID distanceTex, int geom_lod_quality, int survey_id, const Frustum &frustum,
    const Driver3dPerspective &persp, IWaterDecalsRenderHelper *decals_renderer = NULL,
    fft_water::RenderMode render_mode = fft_water::RenderMode::WATER_SHADER);

  void performGPUFFT();
  void closeGPU();
  bool initGPU();

  void closeDisplacementCPU();
  void initDisplacementCPU();
  void setVertexSamplers(int water_vertex_samplers); // for quality
  int setWaterCell(float water_cell_size, bool auto_set_samplers_cnt);
  void setWaterDim(int dim_bits);
  int getQuality() const { return renderQuality; }
  int getGeomQuality() const { return geomQuality; }
  bool getOneToFourCascades() const { return oneToFourCascades; }
  int getFFTResolutionBits() const { return fft[0].getParams().fft_resolution_bits; }
  bool isDepthRendererEnabled() const { return depthRendererEnabled; }
  bool isSSRRendererEnabled() const { return ssrRendererEnabled; }
  void setRenderQuad(const BBox2 &b);
  void setWakeHtTex(TEXTUREID wake_ht_tex);
  void setLod0AreaSize(float size) { lod0AreaRadius = size; }
  float getLod0AreaSize() { return lod0AreaRadius; }
  void setGridLod0AdditionalTesselation(float additional_tesselation) { lod0TesselationAdditional = additional_tesselation; }

  void setForceTessellation(bool force) { forceTessellation = force; }

  void getRoughness(float &out_roughness_base, float &out_cascades_roughness_base)
  {
    out_roughness_base = roughnessBase;
    out_cascades_roughness_base = cascadesRoughnessBase;
  }
  void setRoughness(float roughness_base, float cascades_roughness_base)
  {
    if (roughnessBase != roughness_base || cascadesRoughnessBase != cascades_roughness_base)
    {
      roughnessBase = roughness_base;
      cascadesRoughnessBase = cascades_roughness_base;
      cascadesRoughnessInvalid = true;
    }
  }

  void getGridDataAtCamera(float &gridAlign, Point2 &gridOffset)
  {
    gridAlign = cameraPatchAlign;
    gridOffset = cameraPatchOffset;
  }

  float getShoreWaveThreshold() const { return shoreWaveThreshold; }
  void setShoreWaveThreshold(float value);

  bool isShoreEnabled() const;
  void shoreEnable(bool enable);
  void setRenderPass(int pass);

protected:
  struct SavedStates
  {
    TEXTUREID shoreDistanceFieldTexId = BAD_TEXTUREID;
  };
  void setCascades(const NVWaveWorks_FFT_CPU_Simulation::Params &p);
  SavedStates setStates(TEXTUREID distanceTex, int vs_samplers);
  void resetStates(const SavedStates &states, int vs_samplers);
  int getAutoDisplacementSamplers(float threshold);
  void applyWaterCell();
  void applyShoreEnabled();

  bool autoVsamplersAdjust;
  bool turbulenceFoamEnabled;
  bool depthRendererEnabled;
  bool ssrRendererEnabled;
  int renderQuality, geomQuality;
  int vertexDisplaceSamplers;
  double currentSimulatedTime;
  double lastFoamTime;
  HeightmapRenderer meshRenderer[fft_water::RenderMode::MAX];
  const fft_water::WaterHeightmap *waterHeightmap = nullptr;
  int waterHeightmapPatchesGridRes = 0;
  int waterHeightmapPatchesCount = 0;
  int waterHeightmapPatchesGridScale = 1;
  int waterHeightmapTessFactor = 3; // water_tess_factor of water heightmap shaders in common_assumes.blk
  eastl::vector<Point4> waterHeightmapPatchPositions;
  bool waterHeightmapUseTessellation = false;
  LodGridVertexData waterHeightmapVdata;
  float waterCellSize;
  int renderPass;

  UniqueTex dispCPU[MAX_NUM_CASCADES];

  UniqueTex gradient[MAX_NUM_CASCADES], gradientArray;

  bool clearNeeded;
  UniqueTex foamGradient;

  UniqueTex refraction;
  GPGPUData *gpGpu;
  CSGPUData *csGpu;

  UniqueTexHolder heightmapGridTex;
  UniqueTexHolder heightmapPagesTex;

  UniqueBufHolder heightmapBuf;
  eastl::unique_ptr<ShaderMaterial> heightmapShmat[fft_water::RenderMode::MAX];
  ShaderElement *heightmapShElem[fft_water::RenderMode::MAX] = {nullptr, nullptr, nullptr};

  float waterLevel, maxWaveHeight, significantWaveHeight;
  float maxWaveSize[MAX_NUM_CASCADES];
  Point2 waveDisplacementDist;
  NVWaveWorks_FFT_CPU_Simulation *fft;
  fft_water::SimulationParams fftSimulationParams;
  FoamParams foamParams;
  int numCascades;
  int numFftCascades;
  float cascadeWindowLength;
  float cascadeFacetSize;
  bool cascadesRoughnessInvalid;
  PostFxRenderer gradientRenderer, gradientFoamRenderer; // common
  PostFxRenderer normalsRenderer;
  eastl::unique_ptr<ComputeShaderElement> gradientCs, gradientFoamCs, gradientFoamCombineCs;
  eastl::unique_ptr<ComputeShaderElement> gradientMipCs;
  bool computeGradientsEnabled;
  bool oneToFourCascades = false;

  TEXTUREID wakeHtTex;

  float cameraPatchAlign = 1.0f;
  Point2 cameraPatchOffset = ZERO<Point2>();

  BBox2 *renderQuad;
  int anisotropy;
  float mipBias;

  bool forceTessellation;

  float lod0TesselationAdditional;
  float lod0AreaRadius;
  float lastLodExtension;

  float roughnessBase;
  float cascadesRoughnessBase;
  float cascadesRoughnessMipBias;
  float cascadesRoughnessDistBias;

  Point2 detailsWeightDist;
  Point4 detailsWeightMin;
  Point4 detailsWeightMax;
  Point2 shoreDamp;
  Point2 shoreWavesDist;
  bool shoreEnabled;
  float shoreWaveThreshold = 0.62f;
  shaders::UniqueOverrideStateId overrideAlpha, overrideRGB;
  shaders::UniqueOverrideStateId zClampStateId, zClampFlipStateId;
};
