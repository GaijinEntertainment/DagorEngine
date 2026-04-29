//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_textureIDHolder.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_simpleString.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <generic/dag_smallTab.h>
#include <shaders/dag_postFxRenderer.h>
#include <resourcePool/resourcePool.h>
#include <3d/dag_sbufferIDHolder.h>
#include <osApiWrappers/dag_critSec.h>
#include <fftWater/fftWater.h>
#include <fftWater/chop_water_defines.hlsli>

#define WATER_PATCH_SIZE_VERTICES   9
#define WATER_PATCH_SIZE_QUADS      (WATER_PATCH_SIZE_VERTICES - 1)
#define WATER_PATCH_NUM_VERTICES    (WATER_PATCH_SIZE_VERTICES * WATER_PATCH_SIZE_VERTICES)
#define WATER_PATCH_NUM_QUADS       (WATER_PATCH_SIZE_QUADS * WATER_PATCH_SIZE_QUADS)
#define WATER_PATCH_NUM_TRIANGLES   (WATER_PATCH_NUM_QUADS * 2)
#define WATER_WAVELET_RES           16
#define WATER_WAVE_SPECTRUM_RES     256
#define WATER_VISUAL_CASCADES_COUNT 5

// source of physics/visual error
#define WATER_CPU_HEIGHTMAP_RES          128
#define WATER_CPU_CASCADES_COUNT         2
#define WATER_CPU_SIGNIFICANT_WAVE_COUNT 64

struct WaterWave
{
  Point2 dir_norm;
  Point2 tangent_norm;
  float wave_length;
  float amplitude;
  float speed;
  float phase;
};

struct WaterWaveSpectrumDomain
{
  float size_in_meters;
  float wave_length_min;
  float wave_length_max;
  float significant_wave_height;
  float max_wave_height;
  float power;
  Tab<WaterWave> waves;
  Tab<WaterWave> waves_collision;
  Tab<Point4> detail_waves; // only 5th domain
};

struct WaterWaveSpectrum
{
  Point2 wind_dir;
  float storminess = 0;

  float significant_wave_height = 0;
  float max_wave_height = 0;

  WaterWaveSpectrumDomain domains[WATER_VISUAL_CASCADES_COUNT] = {};

  UniqueTex rt_array_0;
  UniqueTex rt_array_1;
};

class ChopWaterGenerator
{
public:
  ChopWaterGenerator();
  ~ChopWaterGenerator();

  void setWaveletTextures(TEXTUREID chopWaterDetailCombined, TEXTUREID foamDissolveTex, TEXTUREID whiteNoise64Tex,
    TEXTUREID detailWaveletTexture);

  void Update(float water_time, bool detail_waves_enabled = false);
  void setWind(float speed, const Point2 &wind_dir);
  float getWindSpeed() const { return genProps.wind_speed; }

  const fft_water::ChopWaterProps &getProps() const;
  void setProps(const fft_water::ChopWaterProps &props);
  float getDomainSize() const { return genProps.domain_size; }

  float getMaxWaveHeight() const;
  float getMaxWaveHeightCascade(int cascade) const;
  float getSignificantWaveHeight() const;

  void begin_cpu_snapshot(double timestamp);
  int update_cpu_snapshot_rows(int rows_to_process);
  bool is_cpu_snapshot_complete() const;
  void finalize_snapshot_cascade(int cascadeIndex, int fifoIndex);
  int get_cpu_snapshot_progress_rows() const; // (0..WATER_CPU_CASCADES_COUNT*WATER_CPU_HEIGHTMAP_RES)
  bool export_displacement_row_u16(int domain, int dstWidth, int dstRow, uint16_t *dstPackedXYZ, float sea_level,
    float max_wave_height, int desiredFifoIndex, float &outMaxAbsZ) const;

private:
  void LazyInit();
  void precompute_cpu_wave_constants();

  void GenerateWaves(const fft_water::ChopWaterProps &water_props, const Point2 &wind_dir, WaterWaveSpectrum *wave_spectrum,
    float wind_speed);
  void GenerateDetailWaves(const fft_water::ChopWaterProps &water_props, WaterWaveSpectrum *wave_spectrum, float water_time);
  void RenderDomain(const fft_water::ChopWaterProps &water_props, const Point2 &wind_dir, WaterWaveSpectrumDomain &domain,
    ArrayTexture *rt_arr_curr, TEXTUREID wavelet, int tex_array_index);
  void GenerateWaveletCPU();

  struct CPUHeightmapSnapshot
  {
    double timestamp = 0.0;
    eastl::unique_ptr<float[]> domain_heightmap[WATER_CPU_CASCADES_COUNT];
    int fifoIndex = -1;
  };

  struct PendingCPUHeightmap
  {
    double timestamp = 0.0;
    eastl::unique_ptr<float[]> domain_heightmap[WATER_CPU_CASCADES_COUNT];
    int nextDomain = 0; // 0..WATER_CPU_CASCADES_COUNT
    int nextRow = 0;    // 0..WATER_CPU_HEIGHTMAP_RES
    bool active = false;
  };

  fft_water::ChopWaterProps genProps;
  Point2 windDir = Point2(1, 0);
  bool lazyInited = false;

  int pingPongIndex = 0;

  // shaders
  PostFxRenderer ps_waves_heightmap;
  PostFxRenderer ps_waves_heightmap_tiling;
  PostFxRenderer ps_waves_gen_normal;
  PostFxRenderer ps_detail_waves_heightmap;

  // buffers
  SbufferIDHolder waveParams0_buf;
  SbufferIDHolder waveParams1_buf;
  SbufferIDHolder detailWaveParams_buf;

  // textures
  TEXTUREID m_wavelet_0 = BAD_TEXTUREID;
  UniqueTex m_wavelet_cpu_tex;

  //
  UniqueTex m_height_map;
  UniqueTex m_height_map_tiled;

  WaterWaveSpectrum max_wind_wave_spectrum = {};
  Tab<WaterWaveSpectrum> m_spectrum_cache;

  Tab<CPUHeightmapSnapshot> m_cpu_snapshots;
  eastl::optional<PendingCPUHeightmap> m_pending_snapshot;
  int m_snapshot_write_index = -1; // -1 when empty

  struct CPUWavePrecomp
  {
    float scale = 1.0f;
    Point2 tangentStep = Point2(0.0f, 0.0f);
    Point2 tOffset = Point2(0.0f, 0.0f);
    float incThetaY = 0.0f;
    float sinInc1 = 0.0f;
    float cosInc1 = 1.0f;
    float sinInc2 = 0.0f;
    float cosInc2 = 1.0f;
    float amplitude = 0.0f;
    float speed = 0.0f;
    float phase = 0.0f;
    float dirNormY = 0.0f;
  };

  Tab<CPUWavePrecomp> m_cpu_wave_precomp[WATER_CPU_CASCADES_COUNT];
  float m_cpu_domain_size_in_meters[WATER_CPU_CASCADES_COUNT] = {};

  struct CPUWaveRowPhase
  {
    float s1 = 0.0f;
    float c1 = 1.0f;
    float s2 = 0.0f;
    float c2 = 1.0f;
  };

  // [domain][wave][y]
  Tab<Tab<CPUWaveRowPhase>> m_cpu_wave_row_phase[WATER_CPU_CASCADES_COUNT];
};
