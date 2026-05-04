// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fftWater/chopWaterGen.h>

#include <math/random/dag_random.h>
#include <math/dag_Point4.h>
#include <math/dag_hlsl_floatx.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_span.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <EASTL/sort.h>
#include <string.h>
#include <vecmath/dag_vecMath.h>
#include "cpuTexSampler.h"
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>

#define CHOP_WATER_NUM_WAVES_MAX  256
#define CHOP_WATER_MAX_WIND_SPEED 10

#define GLOBAL_VARS_LIST               \
  VAR(global_time_seconds)             \
  VAR(water_normal_detail)             \
  VAR(tex_foam_dissolve)               \
  VAR(tex_noise_white_64x64)           \
  VAR(hm_normal_prev_array)            \
  VAR(tex_w_array)                     \
  VAR(spectrum_domain_max_wave_height) \
  VAR(spectrum_domain_power)           \
  VAR(simulation_tile_size)            \
  VAR(choppiness)                      \
  VAR(storminess)                      \
  VAR(foam_tile_size)                  \
  VAR(foam_coverage)                   \
  VAR(foam_turbidity)                  \
  VAR(foam_sharpness)                  \
  VAR(foam_detail_amp_mul)             \
  VAR(foam_dissolve_uv_scale)          \
  VAR(chop_uv_distort_strength)        \
  VAR(chop_wind_speed)                 \
  VAR(num_waves)                       \
  VAR(num_detail_waves)                \
  VAR(detail_waves_amp)                \
  VAR(water_detail_wavelet_texture)    \
  VAR(wave_params_0)                   \
  VAR(wave_params_1)                   \
  VAR(wavelet_texture)                 \
  VAR(hm_wave_texture)                 \
  VAR(hm_wave_texture_tiled)           \
  VAR(hm_normal_prev_layer)            \
  VAR(detail_wave_params)              \
  VAR(wave_texture_tiled_pixel_size)   \
  VAR(size_in_meters)                  \
  VAR(inv_size_in_meters)              \
  VAR(size_in_meters_over_4)           \
  VAR(domain_max_wave_height)          \
  VAR(domain_significant_wave_height)  \
  VAR(significant_wave_height_inv)     \
  VAR(foam_retention)                  \
  VAR(wind_dir)                        \
  VAR(chopWaterDebug)                  \
  VAR(detail_waves_params)             \
  VAR(chop_uv_domain_scale)            \
  VAR(q_height_mul)                    \
  VAR(q_height_mul4)                   \
  VAR(q_power)                         \
  VAR(q_divisor)                       \
  VAR(scatterIntensityParams)          \
  VAR(last_cascade_nm_strength)        \
  VAR(q_height_mul_tex)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR
namespace convar
{
CONSOLE_INT_VAL("water", chop_debug, 0, 0, 2);
CONSOLE_BOOL_VAL("water", chop_wavegen_pause, false);

CONSOLE_FLOAT_VAL("chop", amplitude_scale, 1.0f);
CONSOLE_FLOAT_VAL("chop", wind_spread, 1.0f);
CONSOLE_FLOAT_VAL("chop", opposing_damping, 0.01f);
CONSOLE_FLOAT_VAL("chop", opposing_ratio, 0.0f);
CONSOLE_FLOAT_VAL("chop", choppiness, 0.4f);

CONSOLE_FLOAT_VAL("chop", foam_tile_size, 0.4f);
CONSOLE_FLOAT_VAL("chop", foam_coverage, 0.5f);
CONSOLE_FLOAT_VAL("chop", foam_turbidity, 0.6f);
CONSOLE_FLOAT_VAL("chop", foam_sharpness, 0.5f);
CONSOLE_FLOAT_VAL("chop", foam_retention, 0.05f);
CONSOLE_FLOAT_VAL("chop", low_foam_turbidity, 0.6f);
CONSOLE_FLOAT_VAL("chop", low_foam_sharpness, 0.5f);
CONSOLE_FLOAT_VAL("chop", low_foam_coverage, 0.5f);
CONSOLE_FLOAT_VAL("chop", foam_detail_amp_mul, 1.0f);
CONSOLE_FLOAT_VAL("chop", foam_dissolve_uv_scale, 2.0f);
CONSOLE_FLOAT_VAL("chop", q_height_mul_x, 0.2f);
CONSOLE_FLOAT_VAL("chop", q_height_mul_y, 1.0f);
CONSOLE_FLOAT_VAL("chop", q_height_mul_z, 2.0f);
CONSOLE_FLOAT_VAL("chop", q_height_mul_w, 2.0f);
CONSOLE_FLOAT_VAL("chop", q_height_mul4, 4.0f);
CONSOLE_FLOAT_VAL("chop", q_height_mul_tex, 0.2f);
CONSOLE_FLOAT_VAL("chop", q_power, 9.4f);
CONSOLE_FLOAT_VAL("chop", q_sig_wave_height_mul, 13.23f);
CONSOLE_FLOAT_VAL("chop", last_cascade_nm_strength, 1.0f);

CONSOLE_FLOAT_VAL("chop", uv_distort_strength, 0.25f);

CONSOLE_FLOAT_VAL("chop", wave_prev_last_cascade_amplitude_mul, 1.0f);
CONSOLE_FLOAT_VAL("chop", wave_last_cascade_amplitude_mul, 7.0f);
CONSOLE_FLOAT_VAL("chop", detail_waves_amplitude, 0.22f);

CONSOLE_FLOAT_VAL("chop", detail_waves_mask_scale, 2.13f);
CONSOLE_FLOAT_VAL("chop", detail_waves_distort_str, 0.52f);
CONSOLE_FLOAT_VAL("chop", detail_waves_detail_bias, 0.26f);
CONSOLE_FLOAT_VAL("chop", detail_waves_detail_strength, 0.55f);

CONSOLE_FLOAT_VAL("chop", scatter_intensity, 1.8f);
CONSOLE_FLOAT_VAL("chop", scatter_intensity_pow, 1.0f);
CONSOLE_FLOAT_VAL("chop", scatter_intensity_skylight_mul, 1.0f);
} // namespace convar

struct WaveParams
{
  uint32_t num_waves;
  Point4 wave_params_0[CHOP_WATER_NUM_WAVES_MAX];
  Point4 wave_params_1[CHOP_WATER_NUM_WAVES_MAX];
};

constexpr float WATER_G = 9.81f;

static void CalculatePhillipsWave(const Point2 &wave_dir_norm, const Point2 &wind_dir_norm, float wave_length, float wind_speed,
  float phillips_constant, float damping_factor, float &result_wave_amp, float &result_wave_speed)
{
  float k = 2.0f * M_PI / wave_length;
  float L = (wind_speed * wind_speed) / WATER_G;

  float k_dot_w = dot(wave_dir_norm, wind_dir_norm);
  float k_dot_w_sq = k_dot_w * k_dot_w;
  float exp_term = exp(-1.0f / (k * L) / (k * L));
  float pk = phillips_constant * exp_term * k_dot_w_sq / (k * k * k * k);
  if (k_dot_w < 0.0f)
  {
    pk *= damping_factor;
  }
  result_wave_amp = sqrt(max(pk, 0.0f));
  result_wave_speed = sqrt(WATER_G / k);
}

Point2 DirFromAngle(float angle)
{
  float sx = sin(angle);
  float cx = cos(angle);
  return Point2(sx, cx);
}

ChopWaterGenerator::ChopWaterGenerator()
{
  m_spectrum_cache.emplace_back();
  Update(0.0f); // to calc maxWaveHeight, significantWaveHeight
}

void ChopWaterGenerator::LazyInit()
{
  ps_waves_heightmap.init("chop_waves_gen_heightmap", true);
  ps_waves_heightmap_tiling.init("chop_waves_heightmap_tiling", true);
  ps_waves_gen_normal.init("chop_waves_gen_normal", true);
  ps_detail_waves_heightmap.init("chop_detail_waves_gen", true);

  int flagsNoLowLevel = SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC | SBCF_MISC_STRUCTURED | SBCF_FRAMEMEM;

  String bufferName1;
  bufferName1.printf(0, "wave_params_0_buf");
  waveParams0_buf.set(d3d::create_sbuffer(sizeof(Point4), CHOP_WATER_NUM_WAVES_MAX, flagsNoLowLevel, 0, bufferName1, RESTAG_WATER),
    bufferName1);

  bufferName1.printf(0, "wave_params_1_buf");
  waveParams1_buf.set(d3d::create_sbuffer(sizeof(Point4), CHOP_WATER_NUM_WAVES_MAX, flagsNoLowLevel, 0, bufferName1, RESTAG_WATER),
    bufferName1);

  bufferName1.printf(0, "detail_wave_params_buf");
  detailWaveParams_buf.set(
    d3d::create_sbuffer(sizeof(Point4), CHOP_WATER_NUM_WAVES_MAX, flagsNoLowLevel, 0, bufferName1, RESTAG_WATER), bufferName1);

  const int w = WATER_WAVE_SPECTRUM_RES;
  const int wFinalNormalFoam = WATER_WAVE_SPECTRUM_RES;

  m_height_map = dag::create_tex(nullptr, w, w, TEXCF_RTARGET | TEXFMT_R16F, 1, "water_wave_spectrum_height_map", RESTAG_WATER);
  m_height_map_tiled =
    dag::create_tex(nullptr, w, w, TEXCF_RTARGET | TEXFMT_R16F, 1, "water_wave_spectrum_height_map_tiled", RESTAG_WATER);

  G_ASSERT(!m_spectrum_cache.empty());
  WaterWaveSpectrum &wave_spectrum = m_spectrum_cache.front();

  String name;
  name.printf(0, "water_wave_spectrum[rt_array_0]");
  wave_spectrum.rt_array_0 = dag::create_array_tex(wFinalNormalFoam, wFinalNormalFoam, WATER_VISUAL_CASCADES_COUNT,
    TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS | TEXCF_CLEAR_ON_CREATE, 0, name.c_str(), RESTAG_WATER),
  name.c_str();
  name.printf(0, "water_wave_spectrum[rt_array_1]");
  wave_spectrum.rt_array_1 = dag::create_array_tex(wFinalNormalFoam, wFinalNormalFoam, WATER_VISUAL_CASCADES_COUNT,
    TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS | TEXCF_CLEAR_ON_CREATE, 0, name.c_str(), RESTAG_WATER),
  name.c_str();

  lazyInited = true;
}

ChopWaterGenerator::~ChopWaterGenerator(void)
{
  if (m_wavelet_cpu_tex)
    m_wavelet_cpu_tex.close();
  if (m_height_map)
    m_height_map.close();
  if (m_height_map_tiled)
    m_height_map_tiled.close();
  for (auto &spectrum : m_spectrum_cache)
  {
    if (spectrum.rt_array_0)
      spectrum.rt_array_0.close();
    if (spectrum.rt_array_1)
      spectrum.rt_array_1.close();
  }
}

void ChopWaterGenerator::setWaveletTextures(TEXTUREID chopWaterDetailCombined, TEXTUREID foamDissolveTex, TEXTUREID whiteNoise64Tex,
  TEXTUREID detailWaveletTexture)
{
  ShaderGlobal::set_texture(water_normal_detailVarId, chopWaterDetailCombined);
  ShaderGlobal::set_texture(tex_foam_dissolveVarId, foamDissolveTex);
  ShaderGlobal::set_texture(tex_noise_white_64x64VarId, whiteNoise64Tex);
  ShaderGlobal::set_texture(water_detail_wavelet_textureVarId, detailWaveletTexture);

  GenerateWaveletCPU();
  LazyInit();
}

void ChopWaterGenerator::setWind(float speed, const Point2 &wind_dir)
{
  const bool windSpeedChanged = speed != genProps.wind_speed;
  windDir = wind_dir;
  windDir.x = -windDir.x; // to match FFT generator logic
  genProps.wind_speed = speed;

  if (windSpeedChanged) // update waves params so getMaxWaveHeight, getSignificantWaveHeight become correct immediately
    GenerateWaves(genProps, windDir, &m_spectrum_cache[0], genProps.wind_speed);
}

void ChopWaterGenerator::Update(float water_time, bool detail_waves_enabled)
{
  genProps.amplitude_scale = convar::amplitude_scale;
  genProps.wind_spread = convar::wind_spread;
  genProps.opposing_damping = convar::opposing_damping;
  genProps.opposing_ratio = convar::opposing_ratio;
  genProps.choppiness = convar::choppiness;

  genProps.foam_tile_size = convar::foam_tile_size;
  genProps.foam_coverage = convar::foam_coverage;
  genProps.foam_turbidity = convar::foam_turbidity;
  genProps.foam_sharpness = convar::foam_sharpness;
  genProps.foam_retention = convar::foam_retention;
  genProps.low_foam_turbidity = convar::low_foam_turbidity;
  genProps.low_foam_sharpness = convar::low_foam_sharpness;
  genProps.low_foam_coverage = convar::low_foam_coverage;
  genProps.foam_dissolve_scale = convar::foam_dissolve_uv_scale;
  genProps.foam_detail_amp_mul = convar::foam_detail_amp_mul;

  genProps.micro_foam_h0 = convar::q_height_mul_x;
  genProps.micro_foam_h1 = convar::q_height_mul_y;
  genProps.micro_foam_h2 = convar::q_height_mul_z;
  genProps.micro_foam_h3 = convar::q_height_mul_w;
  genProps.micro_foam_h4 = convar::q_height_mul4;
  genProps.micro_foam_htex = convar::q_height_mul_tex;
  genProps.micro_foam_q_power = convar::q_power;
  genProps.micro_foam_q_sig_wave_mul = convar::q_sig_wave_height_mul;

  genProps.detail_prev_last_cascade_mul = convar::wave_prev_last_cascade_amplitude_mul;
  genProps.detail_last_cascade_mul = convar::wave_last_cascade_amplitude_mul;
  genProps.detail_waves_mul = convar::detail_waves_amplitude;
  genProps.detail_mask_strength = convar::detail_waves_detail_strength;
  genProps.detail_mask_bias = convar::detail_waves_detail_bias;

  WaterWaveSpectrum *wave_spectrum = &m_spectrum_cache[0];

  {
    TIME_D3D_PROFILE(chop_water_waves);
    // generate max wind wave spectrum
    GenerateWaves(genProps, windDir, &max_wind_wave_spectrum, CHOP_WATER_MAX_WIND_SPEED);

    // generate waves
    GenerateWaves(genProps, windDir, wave_spectrum, genProps.wind_speed);
    if (detail_waves_enabled)
      GenerateDetailWaves(genProps, wave_spectrum, water_time);
  }

  // calculate domains power
  wave_spectrum->domains[0].power = wave_spectrum->domains[0].max_wave_height / max_wind_wave_spectrum.domains[0].max_wave_height;
  wave_spectrum->domains[1].power = wave_spectrum->domains[1].max_wave_height / max_wind_wave_spectrum.domains[1].max_wave_height;
  wave_spectrum->domains[2].power = wave_spectrum->domains[2].max_wave_height / max_wind_wave_spectrum.domains[2].max_wave_height;
  wave_spectrum->domains[3].power = wave_spectrum->domains[3].max_wave_height / max_wind_wave_spectrum.domains[3].max_wave_height;
  wave_spectrum->domains[4].power = wave_spectrum->domains[4].max_wave_height / max_wind_wave_spectrum.domains[4].max_wave_height;

  if (!lazyInited)
  {
    return;
  }
  G_ASSERTF(m_wavelet_0 != BAD_TEXTUREID, "ChopWater: m_wavelet_0 is not set");
  // render domains
  ShaderGlobal::set_float(global_time_secondsVarId, water_time);
  ShaderGlobal::set_float(last_cascade_nm_strengthVarId, convar::last_cascade_nm_strength);

  ArrayTexture *rt_arr_curr = pingPongIndex % 2 ? wave_spectrum->rt_array_0.getArrayTex() : wave_spectrum->rt_array_1.getArrayTex();
  ArrayTexture *rt_arr_prev = pingPongIndex % 2 ? wave_spectrum->rt_array_1.getArrayTex() : wave_spectrum->rt_array_0.getArrayTex();

  ShaderGlobal::set_texture(hm_normal_prev_arrayVarId,
    pingPongIndex % 2 ? wave_spectrum->rt_array_1.getTexId() : wave_spectrum->rt_array_0.getTexId());

  {
    TIME_D3D_PROFILE(chop_water_domains);
    RenderDomain(genProps, windDir, wave_spectrum->domains[0], rt_arr_curr, m_wavelet_0, 0);
    RenderDomain(genProps, windDir, wave_spectrum->domains[1], rt_arr_curr, m_wavelet_0, 1);
    RenderDomain(genProps, windDir, wave_spectrum->domains[2], rt_arr_curr, m_wavelet_0, 2);
    RenderDomain(genProps, windDir, wave_spectrum->domains[3], rt_arr_curr, m_wavelet_0, 3);
    if (detail_waves_enabled)
      RenderDomain(genProps, windDir, wave_spectrum->domains[4], rt_arr_curr, m_wavelet_0, 4);
  }

  {
    TIME_D3D_PROFILE(chop_water_mips);
    // generate mip chain
    rt_arr_curr->generateMips();
  }

  // set variables for water rendering (which happens in waterRender.cpp: WaterNVRender::render)
  TEXTUREID rt_array_id = pingPongIndex % 2 ? wave_spectrum->rt_array_0.getTexId() : wave_spectrum->rt_array_1.getTexId();
  ShaderGlobal::set_texture(tex_w_arrayVarId, rt_array_id);

  Point4 spectrum_domain_max_wave_height = Point4(wave_spectrum->domains[0].max_wave_height, wave_spectrum->domains[1].max_wave_height,
    wave_spectrum->domains[2].max_wave_height, wave_spectrum->domains[3].max_wave_height);
  Point4 spectrum_domain_power = Point4(wave_spectrum->domains[0].power, wave_spectrum->domains[1].power,
    wave_spectrum->domains[2].power, wave_spectrum->domains[3].power);

  ShaderGlobal::set_float4(spectrum_domain_max_wave_heightVarId, spectrum_domain_max_wave_height);
  ShaderGlobal::set_float4(spectrum_domain_powerVarId, spectrum_domain_power);

  ShaderGlobal::set_float(simulation_tile_sizeVarId, genProps.domain_size);
  ShaderGlobal::set_float(chop_uv_domain_scaleVarId, genProps.domain_size / 1000.0f);
  float q_height_mul_sum =
    genProps.micro_foam_h0 + genProps.micro_foam_h1 + genProps.micro_foam_h2 + genProps.micro_foam_h3 + genProps.micro_foam_h4;
  float q_divisor_val = q_height_mul_sum / (wave_spectrum->significant_wave_height * genProps.micro_foam_q_sig_wave_mul);
  ShaderGlobal::set_float4(q_height_mulVarId,
    Color4(genProps.micro_foam_h0, genProps.micro_foam_h1, genProps.micro_foam_h2, genProps.micro_foam_h3));
  ShaderGlobal::set_float(q_height_mul4VarId, genProps.micro_foam_h4);
  ShaderGlobal::set_float(q_powerVarId, genProps.micro_foam_q_power);
  ShaderGlobal::set_float(q_divisorVarId, q_divisor_val);
  ShaderGlobal::set_float(q_height_mul_texVarId, genProps.micro_foam_htex);
  ShaderGlobal::set_float(choppinessVarId, genProps.choppiness);
  ShaderGlobal::set_float(storminessVarId, wave_spectrum->storminess);
  ShaderGlobal::set_float(significant_wave_height_invVarId, 1.0f / max(0.01f, wave_spectrum->significant_wave_height));
  ShaderGlobal::set_float4(scatterIntensityParamsVarId,
    Color4(convar::scatter_intensity, convar::scatter_intensity_skylight_mul, convar::scatter_intensity_pow, 0));

  ShaderGlobal::set_float(foam_tile_sizeVarId, genProps.foam_tile_size);
  ShaderGlobal::set_float(foam_dissolve_uv_scaleVarId, genProps.foam_dissolve_scale * genProps.domain_size / 1000.0f);
  if (detail_waves_enabled)
  {
    ShaderGlobal::set_float(foam_coverageVarId, genProps.foam_coverage);
    ShaderGlobal::set_float(foam_turbidityVarId, genProps.foam_turbidity);
    ShaderGlobal::set_float(foam_sharpnessVarId, genProps.foam_sharpness);
  }
  else
  {
    ShaderGlobal::set_float(foam_coverageVarId, convar::low_foam_coverage);
    ShaderGlobal::set_float(foam_turbidityVarId, convar::low_foam_turbidity);
    ShaderGlobal::set_float(foam_sharpnessVarId, convar::low_foam_sharpness);
  }
  ShaderGlobal::set_float(foam_detail_amp_mulVarId, genProps.foam_detail_amp_mul);
  ShaderGlobal::set_float(chop_wind_speedVarId, genProps.wind_speed);

  ShaderGlobal::set_float(chop_uv_distort_strengthVarId, convar::uv_distort_strength);
  ShaderGlobal::set_float(chopWaterDebugVarId, float(convar::chop_debug));

  ShaderGlobal::set_float4(detail_waves_paramsVarId, Color4(convar::detail_waves_mask_scale, convar::detail_waves_distort_str,
                                                       convar::detail_waves_detail_strength, convar::detail_waves_detail_bias));

  ++pingPongIndex;
}

const fft_water::ChopWaterProps &ChopWaterGenerator::getProps() const { return genProps; }

void ChopWaterGenerator::setProps(const fft_water::ChopWaterProps &props)
{
  using namespace convar;

  const float oldWindSpeed = genProps.wind_speed;

  genProps = props;
  genProps.wind_speed = oldWindSpeed; // wind_speed is changed only through setWind API (consistency with rest of fftWater module)

  amplitude_scale = genProps.amplitude_scale;
  wind_spread = genProps.wind_spread;
  opposing_damping = genProps.opposing_damping;
  opposing_ratio = genProps.opposing_ratio;
  choppiness = genProps.choppiness;

  foam_tile_size = genProps.foam_tile_size;
  foam_coverage = genProps.foam_coverage;
  foam_turbidity = genProps.foam_turbidity;
  foam_sharpness = genProps.foam_sharpness;
  foam_retention = genProps.foam_retention; //-V1001
  low_foam_turbidity = genProps.low_foam_turbidity;
  low_foam_sharpness = genProps.low_foam_sharpness;
  low_foam_coverage = genProps.low_foam_coverage;
  foam_dissolve_uv_scale = genProps.foam_dissolve_scale; //-V1001
  foam_detail_amp_mul = genProps.foam_detail_amp_mul;    //-V1001

  q_height_mul_x = genProps.micro_foam_h0;                    //-V1001
  q_height_mul_y = genProps.micro_foam_h1;                    //-V1001
  q_height_mul_z = genProps.micro_foam_h2;                    //-V1001
  q_height_mul_w = genProps.micro_foam_h3;                    //-V1001
  q_height_mul4 = genProps.micro_foam_h4;                     //-V1001
  q_height_mul_tex = genProps.micro_foam_htex;                //-V1001
  q_power = genProps.micro_foam_q_power;                      //-V1001
  q_sig_wave_height_mul = genProps.micro_foam_q_sig_wave_mul; //-V1001

  wave_prev_last_cascade_amplitude_mul = genProps.detail_prev_last_cascade_mul; //-V1001
  wave_last_cascade_amplitude_mul = genProps.detail_last_cascade_mul;           //-V1001
  last_cascade_nm_strength = genProps.detail_last_cascade_nm;                   //-V1001
  detail_waves_amplitude = genProps.detail_waves_mul;                           //-V1001
  detail_waves_detail_strength = genProps.detail_mask_strength;                 //-V1001
  detail_waves_detail_bias = genProps.detail_mask_bias;                         //-V1001

  scatter_intensity = genProps.scatter_intensity;                           //-V1001
  scatter_intensity_pow = genProps.scatter_intensity_pow;                   //-V1001
  scatter_intensity_skylight_mul = genProps.scatter_intensity_skylight_mul; //-V1001
}

float ChopWaterGenerator::getMaxWaveHeight() const
{
  G_ASSERT(!m_spectrum_cache.empty());

  float maxWaveHeight = m_spectrum_cache.front().max_wave_height;
  G_ASSERT(maxWaveHeight > 0.0f);

  return maxWaveHeight;
}

float ChopWaterGenerator::getMaxWaveHeightCascade(int cascade) const
{
  G_ASSERT(!m_spectrum_cache.empty());
  G_ASSERT_RETURN(cascade < WATER_VISUAL_CASCADES_COUNT, 0.0f);

  float maxAmp = m_spectrum_cache.front().domains[cascade].max_wave_height;
  G_ASSERT(maxAmp > 0.0f);
  return maxAmp;
}

float ChopWaterGenerator::getSignificantWaveHeight() const
{
  G_ASSERT(!m_spectrum_cache.empty());

  float significantWaveHeight = m_spectrum_cache.front().significant_wave_height;
  G_ASSERT(significantWaveHeight > 0.0f);

  return significantWaveHeight;
}

//=================================================================================================
void ChopWaterGenerator::GenerateWaves(const fft_water::ChopWaterProps &water_props, const Point2 &wind_dir,
  WaterWaveSpectrum *wave_spectrum, float wind_speed)
{
  int rand_seed = 0;

  // ========================================
  // init wave spectrum
  // ========================================

  wave_spectrum->wind_dir = wind_dir;
  wave_spectrum->storminess = saturate(water_props.wind_speed / CHOP_WATER_MAX_WIND_SPEED);
  float size_in_meters = water_props.domain_size;

  for (uint32_t i = 0; i < WATER_VISUAL_CASCADES_COUNT; ++i)
  {
    WaterWaveSpectrumDomain &domain = wave_spectrum->domains[i];
    domain.size_in_meters = size_in_meters;
    domain.wave_length_min = size_in_meters / 32.0f;
    domain.wave_length_max = size_in_meters / 4.0f;
    domain.significant_wave_height = 0;
    domain.max_wave_height = 0;
    domain.power = 0;
    domain.waves.clear();
    domain.waves_collision.clear();

    size_in_meters *= 0.25f;
  }
  float wave_length_max = wave_spectrum->domains[0].wave_length_max;
  float wave_length_min = wave_spectrum->domains[WATER_VISUAL_CASCADES_COUNT - 1].wave_length_min;

  float kmin = 2.0f * M_PI / wave_length_max;
  float kmax = 2.0f * M_PI / wave_length_min;
  float log_kmin = log(kmin);
  float log_kmax = log(kmax);
  float wind_angle = atan2f(wind_dir.x, wind_dir.y);

  // ========================================
  // generate all waves
  // ========================================
  uint32_t num_waves_total = 128;
  Tab<WaterWave> all_waves(framemem_ptr());
  for (uint32_t i = 0; i < num_waves_total; ++i)
  {
    float t = (float)i / (float)(num_waves_total - 1);
    // float wave_length = wave_length_min * pow(wave_length_max / wave_length_min, t);
    float k = exp(lerp(log_kmin, log_kmax, t));
    float wave_length = 2.0f * M_PI / k;
    float wave_angle = 0;

    if (dagor_random::_rnd_float(rand_seed, 0.0f, 1.0f) < water_props.opposing_ratio) // opposing wave
      wave_angle = wind_angle + M_PI + dagor_random::_rnd_float(rand_seed, -water_props.wind_spread, water_props.wind_spread);
    else // aligned wave
      wave_angle = wind_angle + dagor_random::_rnd_float(rand_seed, -water_props.wind_spread, water_props.wind_spread);
    Point2 wave_dir = DirFromAngle(wave_angle);

    float wave_amp = 0;
    float wave_speed = 0;
    const float phillipsConstant = 0.2f;
    CalculatePhillipsWave(wave_dir, wind_dir, wave_length, wind_speed, phillipsConstant, water_props.opposing_damping, wave_amp,
      wave_speed);
    wave_amp = min(wave_amp, wave_length * 0.01f);
    wave_amp = max(wave_amp, wave_length * 0.0001f);

    if (convar::chop_wavegen_pause)
    {
      wave_speed = 0;
    }

    WaterWave wave;
    wave.dir_norm = wave_dir;
    Point3 tangent_norm = cross(Point3(wave_dir.x, wave_dir.y, 0.0), Point3(0.0f, 0.0f, 1.0f));
    wave.tangent_norm = Point2(tangent_norm.x, tangent_norm.y);
    wave.wave_length = wave_length;
    wave.amplitude = wave_amp * water_props.amplitude_scale;
    wave.speed = wave_speed;
    wave.phase = dagor_random::_rnd_float(rand_seed, 0.0f, 1.0f);
    all_waves.push_back(wave);
  }

  // ========================================
  // energy normalization
  // ========================================

  // estimate expected significant wave height(Hs) from wind speed
  float significant_wave_height = 0.21f * ((wind_speed * wind_speed) / WATER_G); // pierson–moskowitz
  // convert significant wave height(Hs) to target RMS amplitude
  // RMS = (Hs / 2) * 0.707 = half height × RMS factor
  float target_rms_amp = (significant_wave_height * 0.5f) * 0.707f;
  // calculate target total energy
  float target_total_energy = target_rms_amp * target_rms_amp;
  // compute total current energy
  float current_total_energy = 0.0f;
  for (auto &wave : all_waves)
  {
    current_total_energy += wave.amplitude * wave.amplitude;
  }

  // ========================================
  // sort waves by spectrum domains
  // ========================================
  for (auto &wave : all_waves)
    for (size_t domainIndex = 0; domainIndex < WATER_VISUAL_CASCADES_COUNT; ++domainIndex)
    {
      auto &domain = wave_spectrum->domains[domainIndex];
      const bool isLastDomain = (domainIndex == WATER_VISUAL_CASCADES_COUNT - 1);

      if (wave.wave_length >= domain.wave_length_min && wave.wave_length < domain.wave_length_max)
      {
        domain.waves.push_back(wave);
        if (isLastDomain)
          domain.waves.back().amplitude *= convar::wave_last_cascade_amplitude_mul;
        if (domainIndex == WATER_VISUAL_CASCADES_COUNT - 2)
          domain.waves.back().amplitude *= convar::wave_prev_last_cascade_amplitude_mul;
        break;
      }
    }

  // ========================================
  // calculate significant wave height(Hs)
  // ========================================
  float overall_m0 = 0.0f;
  for (auto &domain : wave_spectrum->domains)
  {
    float m0 = 0.0f;
    for (auto &wave : domain.waves)
    {
      m0 += 0.5f * wave.amplitude * wave.amplitude;
    }
    domain.significant_wave_height = 4.0f * sqrtf(m0);
    domain.max_wave_height = domain.significant_wave_height * 2.0f;
    overall_m0 += m0;
  }
  wave_spectrum->significant_wave_height = 4.0f * sqrtf(overall_m0);
  wave_spectrum->max_wave_height = wave_spectrum->significant_wave_height * 2.0f;
  G_ASSERT(wave_spectrum->significant_wave_height > 0.0f);


  // ========================================
  // build global list of collision waves
  // ========================================
  Tab<WaterWave> all_waves_collision(framemem_ptr());
  all_waves_collision = all_waves;
  eastl::sort(all_waves_collision.begin(), all_waves_collision.end(),
    [](const WaterWave &a, const WaterWave &b) { return a.amplitude > b.amplitude; });
  all_waves_collision.resize(WATER_CPU_SIGNIFICANT_WAVE_COUNT);

  // ========================================
  // sort collision waves by spectrum domains
  // ========================================
  for (auto &wave : all_waves_collision)
    for (auto &domain : wave_spectrum->domains)
    {
      if (wave.wave_length >= domain.wave_length_min && wave.wave_length < domain.wave_length_max)
      {
        domain.waves_collision.push_back(wave);
        break;
      }
    }
}

static Point2 rotate_uv_detail_chop(const Point2 &uv, float angle)
{
  Point2 sin_cos;
  sincos(angle, sin_cos.x, sin_cos.y);
  return float2(uv.x * sin_cos.y - uv.y * sin_cos.x, uv.x * sin_cos.x + uv.y * sin_cos.y);
}

void ChopWaterGenerator::GenerateDetailWaves(const fft_water::ChopWaterProps &water_props, WaterWaveSpectrum *wave_spectrum,
  float water_time)
{
  auto &detail_waves = wave_spectrum->domains[WATER_VISUAL_CASCADES_COUNT - 1].detail_waves;
  detail_waves.clear();

  const Point2 &wind_dir = wave_spectrum->wind_dir;

  uint32_t num_bands = 4;
  uint32_t num_dir = 5;

  constexpr float detailWavesSpeed = 0.3f;
  constexpr float detailWavesOpposingRatio = 0.5f;

  float band_amp = 1.0;
  for (uint32_t idx_band = 0; idx_band < num_bands; ++idx_band)
  {
    for (uint32_t idx_dir = 0; idx_dir < num_dir; ++idx_dir)
    {
      float dir_fr = (float)idx_dir / (float)num_dir;
      Point2 wave_dir = rotate_uv_detail_chop(Point2(0.0, 1.0), dir_fr * 2.0f * M_PI);

      float ww_0 = dot(-wave_dir, float2(wind_dir.x, -wind_dir.y));
      float ww_1 = saturate(ww_0);
      float ww_2 = ww_0 * 0.5f + 0.5f;

      float ww = 0;
      if (detailWavesOpposingRatio <= 0.5f)
        ww = lerp(ww_1, ww_2, detailWavesOpposingRatio / 0.5f);
      else
        ww = lerp(ww_2, 1.0f, (detailWavesOpposingRatio - 0.5f) / 0.5f);

      detail_waves.push_back(Point4(wave_dir.x * (water_time * band_amp * detailWavesSpeed + dir_fr),
        wave_dir.y * (water_time * band_amp * detailWavesSpeed + dir_fr), (float)idx_band, ww * band_amp));
    }
    band_amp *= 0.5;
  }
}

void ChopWaterGenerator::RenderDomain(const fft_water::ChopWaterProps &water_props, const Point2 &wind_dir,
  WaterWaveSpectrumDomain &domain, ArrayTexture *rt_arr_curr, TEXTUREID wavelet, int tex_array_index)
{
  TIME_D3D_PROFILE(chop_water_gen_render_domain);
  uint32_t num_waves = (uint32_t)domain.waves.size();
  if (!num_waves)
  {
    return;
  }

  // update 'cp_wave_params'
  WaveParams wave_params;
  wave_params.num_waves = num_waves;
  for (uint32_t i = 0; i < num_waves; ++i)
  {
    WaterWave &wave = domain.waves[i];
    wave_params.wave_params_0[i] = Point4(wave.dir_norm.x, wave.dir_norm.y, wave.wave_length, wave.amplitude);
    wave_params.wave_params_1[i] = Point4(wave.speed, wave.phase, 0, 0);
  }

  waveParams0_buf.getBuf()->updateDataWithLock(0, num_waves * sizeof(Point4), &wave_params.wave_params_0[0], VBLOCK_DISCARD);
  waveParams1_buf.getBuf()->updateDataWithLock(0, num_waves * sizeof(Point4), &wave_params.wave_params_1[0], VBLOCK_DISCARD);

  ShaderGlobal::set_float(num_wavesVarId, float(num_waves));
  ShaderGlobal::set_buffer(wave_params_0VarId, waveParams0_buf.getId());
  ShaderGlobal::set_buffer(wave_params_1VarId, waveParams1_buf.getId());
  ShaderGlobal::set_texture(wavelet_textureVarId, wavelet);
  ShaderGlobal::set_float(wave_texture_tiled_pixel_sizeVarId, 1.0f / WATER_WAVE_SPECTRUM_RES);

  ShaderGlobal::set_float(size_in_metersVarId, domain.size_in_meters);
  ShaderGlobal::set_float(inv_size_in_metersVarId, 1.0f / domain.size_in_meters);
  ShaderGlobal::set_float(size_in_meters_over_4VarId, domain.size_in_meters / 4.0f);
  ShaderGlobal::set_float(domain_max_wave_heightVarId, domain.max_wave_height);
  ShaderGlobal::set_float(domain_significant_wave_heightVarId, domain.significant_wave_height);
  ShaderGlobal::set_float(foam_retentionVarId, pow(water_props.foam_retention, 0.01f) * 0.999f);
  ShaderGlobal::set_float4(wind_dirVarId, Color4(wind_dir.x, wind_dir.y, 0.f, convar::chop_debug));

  // height map
  d3d::set_render_target(m_height_map.getTex2D(), 0);
  if (tex_array_index < 4)
  {
    ps_waves_heightmap.render();
  }
  else // 5th domain + microwaves
  {
    uint32_t num_detail_waves = (uint32_t)domain.detail_waves.size();

    ShaderGlobal::set_float(num_detail_wavesVarId, float(num_detail_waves));
    ShaderGlobal::set_float(detail_waves_ampVarId, convar::detail_waves_amplitude);

    G_ASSERT(num_detail_waves <= CHOP_WATER_NUM_WAVES_MAX);
    if (num_detail_waves)
      detailWaveParams_buf.getBuf()->updateDataWithLock(0, num_detail_waves * sizeof(Point4), &domain.detail_waves[0], VBLOCK_DISCARD);

    ShaderGlobal::set_buffer(detail_wave_paramsVarId, detailWaveParams_buf.getId());

    ps_detail_waves_heightmap.render();
  }

  // tiling
  ShaderGlobal::set_texture(hm_wave_textureVarId, m_height_map.getTexId());
  d3d::set_render_target(m_height_map_tiled.getTex2D(), 0);
  ps_waves_heightmap_tiling.render();

  // normal map
  ShaderGlobal::set_texture(hm_wave_texture_tiledVarId, m_height_map_tiled.getTexId());
  ShaderGlobal::set_int(hm_normal_prev_layerVarId, tex_array_index);
  d3d::set_render_target(0, rt_arr_curr, tex_array_index, 0);
  ps_waves_gen_normal.render();
}

struct ReloadWaveletCPUTex : public BaseTexture::IReloadData
{
  ReloadWaveletCPUTex() {}

  virtual void reloadD3dRes(BaseTexture *t) { fillData(t); }
  virtual void destroySelf() { delete this; }

  static void fillData(BaseTexture *t)
  {
    const int res = WATER_WAVELET_RES;
    eastl::vector<float> waveletData;
    waveletData.resize(res * res);

    for (int y = 0; y < res; ++y)
      for (int x = 0; x < res; ++x)
      {
        Point2 texcoord(((float)x + 0.5f) / (float)res, ((float)y + 0.5f) / (float)res);
        const float phase_offset_freq = 2.0f;
        const float phase_offset_strength = 1.0f;
        float phase_offset = sinf(texcoord.x * phase_offset_freq * (float)M_PI) * 0.5f +
                             sinf(texcoord.x * (phase_offset_freq * 2.0f) * (float)M_PI) * 0.25f;
        float theta = texcoord.y * (float)M_PI * 2.0f + phase_offset * phase_offset_strength;
        waveletData[y * res + x] = sinf(theta);
      }

    uint8_t *dst = nullptr;
    int stride = 0;
    if (t->lockimg((void **)&dst, stride, 0, TEXLOCK_WRITE | TEXLOCK_DISCARD))
    {
      for (int y = 0; y < res; ++y)
      {
        Half *row = (Half *)(dst + y * stride);
        for (int x = 0; x < res; ++x)
        {
          row[x] = Half(waveletData[y * res + x]);
        }
      }
      t->unlockimg();
    }
  }
};


// NOTE:
// if you change wavelet - you need to actualize ChopWaterGenerator::update_cpu_snapshot_rows which is used for CPU sim (physics)
// wavelet is sampled in chop_water_gen_service.dshl (sample_wave)
void ChopWaterGenerator::GenerateWaveletCPU()
{
  const int res = WATER_WAVELET_RES;

  m_wavelet_cpu_tex = dag::create_tex(nullptr, res, res, TEXFMT_R16F, 1, "chop_wavelet_cpu", RESTAG_WATER);
  if (m_wavelet_cpu_tex.getTex2D())
  {
    ReloadWaveletCPUTex *rld = new ReloadWaveletCPUTex();
    if (!m_wavelet_cpu_tex.getTex2D()->setReloadCallback(rld))
      delete rld;
    ReloadWaveletCPUTex::fillData(m_wavelet_cpu_tex.getTex2D());
  }
  m_wavelet_0 = m_wavelet_cpu_tex.getTexId();
}

//=================================================================================================
namespace
{
static float SIN_TABLE_128[128];
static bool SIN_TABLE_128_INIT = false;
static inline void ensure_sin_table()
{
  if (!SIN_TABLE_128_INIT)
  {
    for (int i = 0; i < 128; ++i)
    {
      float a = (6.28318530718f * (float)i) * (1.0f / 128.0f);
      SIN_TABLE_128[i] = sinf(a);
    }
    SIN_TABLE_128_INIT = true;
  }
}

constexpr float sin_table_K = 128.0f / (2.0f * (float)M_PI);
static inline float sin_table_lookup(float angle)
{
  float t = angle * sin_table_K;
  int i = (int)t;
  i -= (i > t);                  // branchless floorf(t)
  return SIN_TABLE_128[i & 127]; // wrap every 2π via bitmask
}

static inline float cos_table_lookup(float angle)
{
  const float HALF_PI = (float)M_PI * 0.5f;
  return sin_table_lookup(angle + HALF_PI);
}
#ifdef CHOP_WATER_USE_SSE
// 4-wide table-based sine lookup
static inline vec4f sin_table_lookup_ps(vec4f angle)
{
  ensure_sin_table();
  const vec4f K = v_splats(sin_table_K);
  vec4f t = v_mul(angle, K);
  // i = trunc(t); floor = i - (i > t)
  vec4i i = v_cvt_trunci(t);
  vec4f ti = v_cvti_vec4f(i);
  vec4f gt = v_cmp_gt(ti, t);
  vec4i dec = v_andi(v_cast_vec4i(gt), v_splatsi(1));
  i = v_subi(i, dec);
  vec4i idx = v_andi(i, v_splatsi(127));

  alignas(16) int ii[4];
  v_sti(ii, idx);
  alignas(16) float vv[4];
  vv[0] = SIN_TABLE_128[ii[0]];
  vv[1] = SIN_TABLE_128[ii[1]];
  vv[2] = SIN_TABLE_128[ii[2]];
  vv[3] = SIN_TABLE_128[ii[3]];
  return v_ld(vv);
}
#endif
} // namespace

//=================================================================================================
void ChopWaterGenerator::begin_cpu_snapshot(double timestamp)
{
  TIME_PROFILE(chop_begin_snapshot);
  if (!m_pending_snapshot.has_value())
    m_pending_snapshot.emplace();

  if (!m_pending_snapshot->active)
  {
    m_pending_snapshot->timestamp = timestamp;
    for (int d = 0; d < WATER_CPU_CASCADES_COUNT; ++d)
    {
      if (!m_pending_snapshot->domain_heightmap[d])
        m_pending_snapshot->domain_heightmap[d].reset(new float[WATER_CPU_HEIGHTMAP_RES * WATER_CPU_HEIGHTMAP_RES]);

      memset(m_pending_snapshot->domain_heightmap[d].get(), 0, sizeof(float) * WATER_CPU_HEIGHTMAP_RES * WATER_CPU_HEIGHTMAP_RES);
    }
    m_pending_snapshot->nextDomain = 0;
    m_pending_snapshot->nextRow = 0;
    m_pending_snapshot->active = true;

    precompute_cpu_wave_constants();
  }
}

// NOTE:
// This is CPU-approximation of sampling water wavelet many times to generate waves.
// See GenerateWaveletCPU and chop_water_gen_service.dshl (sample_wave)
// Important performance optimizations:
// - trigonometry: sin(a+b) = sin(a)cos(b)+cos(a)sin(b). faster than computing sin while traversing rows
// - table sin / cos. faster than computing sinf, cosf
// - SSE
// - don't use floorf
int ChopWaterGenerator::update_cpu_snapshot_rows(int rows_to_process)
{
  if (!m_pending_snapshot.has_value() || !m_pending_snapshot->active)
    return 0;
  static_assert(WATER_CPU_HEIGHTMAP_RES % 4 == 0);

  TIME_PROFILE(chop_rows_update);

  int processed = 0;
  const double ts = m_pending_snapshot->timestamp;

  const float invRes = 1.0f / (float)WATER_CPU_HEIGHTMAP_RES;
  const float TWO_PI = (float)M_PI * 2.0f;
  const float PHASE_OFFSET_STRENGTH = 1.0f;
  float ns1, ns2;
  for (; m_pending_snapshot->nextDomain < WATER_CPU_CASCADES_COUNT && processed < rows_to_process;)
  {
    const int d = m_pending_snapshot->nextDomain;
    float *out = m_pending_snapshot->domain_heightmap[d].get();

    const uint32_t wavesCount = (uint32_t)m_cpu_wave_precomp[d].size();
    // ensure precomp array is sized for this domain
    if ((int)m_cpu_wave_precomp[d].size() != (int)wavesCount)
      m_cpu_wave_precomp[d].resize(wavesCount);

    int yStart = m_pending_snapshot->nextRow;
    int rowsBudget = min(rows_to_process - processed, WATER_CPU_HEIGHTMAP_RES - yStart);
    int yEnd = yStart + rowsBudget;

    if (rowsBudget > 0)
    {
      for (uint32_t i = 0; i < wavesCount; ++i)
      {
        const CPUWavePrecomp &precomputedWave = m_cpu_wave_precomp[d][i];
        const float sinInc1 = precomputedWave.sinInc1;
        const float cosInc1 = precomputedWave.cosInc1;
        const float sinInc2 = precomputedWave.sinInc2;
        const float cosInc2 = precomputedWave.cosInc2;

        const float timeScale = 1.0f / m_cpu_domain_size_in_meters[d];
        const float timePhaseBase = (float)(ts * precomputedWave.speed) * timeScale + precomputedWave.phase;

        for (int y = yStart; y < yEnd; ++y)
        {
          const float texcoordY = ((float)y + 0.5f) * invRes;
          float *rowOut = out + y * WATER_CPU_HEIGHTMAP_RES;

          float baseY = texcoordY * precomputedWave.dirNormY + precomputedWave.tOffset.y + timePhaseBase;
          baseY *= precomputedWave.scale;

          // Wrap baseY * 32 into [0,1)
          const float y32 = baseY * 32.0f;
          int y32_i = (int)y32;
          y32_i -= (y32_i > y32);
          float thetaY = (y32 - (float)y32_i) * TWO_PI;

          // Use precomputed
          const CPUWaveRowPhase &rp = m_cpu_wave_row_phase[d][i][y];
          float s1 = rp.s1;
          float c1 = rp.c1;
          float s2 = rp.s2;
          float c2 = rp.c2;

          {
#ifdef CHOP_WATER_USE_SSE
            // Precompute sin(k*δ), cos(k*δ) for k=0..3 for both harmonics
            const float s1_d = sinInc1, c1_d = cosInc1;
            const float s1_2d = 2.0f * s1_d * c1_d;
            const float c1_2d = 2.0f * c1_d * c1_d - 1.0f;
            const float s1_3d = s1_2d * c1_d + c1_2d * s1_d;
            const float c1_3d = c1_2d * c1_d - s1_2d * s1_d;
            const float s1_4d = 2.0f * s1_2d * c1_2d;
            const float c1_4d = 2.0f * c1_2d * c1_2d - 1.0f;

            const float s2_d = sinInc2, c2_d = cosInc2;
            const float s2_2d = 2.0f * s2_d * c2_d;
            const float c2_2d = 2.0f * c2_d * c2_d - 1.0f;
            const float s2_3d = s2_2d * c2_d + c2_2d * s2_d;
            const float c2_3d = c2_2d * c2_d - s2_2d * s2_d;
            const float s2_4d = 2.0f * s2_2d * c2_2d;
            const float c2_4d = 2.0f * c2_2d * c2_2d - 1.0f;

            const vec4f sink1 = v_make_vec4f(0.0f, s1_d, s1_2d, s1_3d);
            const vec4f cosk1 = v_make_vec4f(1.0f, c1_d, c1_2d, c1_3d);
            const vec4f sink2 = v_make_vec4f(0.0f, s2_d, s2_2d, s2_3d);
            const vec4f cosk2 = v_make_vec4f(1.0f, c2_d, c2_2d, c2_3d);

            const vec4f idx0123 = v_make_vec4f(0.0f, 1.0f, 2.0f, 3.0f);
            const vec4f incTheta = v_splats(precomputedWave.incThetaY);
            const vec4f thetaStep = v_mul(incTheta, idx0123);
            const vec4f vAmp = v_splats(precomputedWave.amplitude);
            const vec4f vPosScale = v_splats(PHASE_OFFSET_STRENGTH);

            const vec4f half = v_splats(0.5f);
            const vec4f quart = v_splats(0.25f);

            const float incTheta4 = precomputedWave.incThetaY * 4.0f;

            uint32_t x = 0;
            for (; x < WATER_CPU_HEIGHTMAP_RES; x += 4)
            {
              vec4f s1Base = v_splats(s1);
              vec4f c1Base = v_splats(c1);
              vec4f s2Base = v_splats(s2);
              vec4f c2Base = v_splats(c2);

              vec4f s1v = v_add(v_mul(s1Base, cosk1), v_mul(c1Base, sink1));
              vec4f s2v = v_add(v_mul(s2Base, cosk2), v_mul(c2Base, sink2));

              vec4f phase_off = v_add(v_mul(s1v, half), v_mul(s2v, quart));
              vec4f theta4 = v_add(v_splats(thetaY), thetaStep);
              theta4 = v_add(theta4, v_mul(phase_off, vPosScale));

              vec4f val = sin_table_lookup_ps(theta4);
              val = v_mul(val, vAmp);

              vec4f dst = v_ldu(rowOut + x);
              dst = v_add(dst, val);
              v_stu(rowOut + x, dst);

              // advance base states by 4 steps
              float s1_old = s1;
              s1 = s1 * c1_4d + c1 * s1_4d;
              c1 = c1 * c1_4d - s1_old * s1_4d;

              float s2_old = s2;
              s2 = s2 * c2_4d + c2 * s2_4d;
              c2 = c2 * c2_4d - s2_old * s2_4d;

              thetaY += incTheta4;
            }
#else
            for (uint32_t x = 0; x < WATER_CPU_HEIGHTMAP_RES; ++x)
            {
              const float phase_offset = s1 * 0.5f + s2 * 0.25f;
              const float theta = thetaY + phase_offset * PHASE_OFFSET_STRENGTH;

              rowOut[x] += sin_table_lookup(theta) * precomputedWave.amplitude;

              // Advance recurrences
              ns1 = s1 * cosInc1 + c1 * sinInc1;
              c1 = c1 * cosInc1 - s1 * sinInc1;
              s1 = ns1;

              ns2 = s2 * cosInc2 + c2 * sinInc2;
              c2 = c2 * cosInc2 - s2 * sinInc2;
              s2 = ns2;
              thetaY += precomputedWave.incThetaY;
            }
#endif
          }
        }
      }

      processed += rowsBudget;
      m_pending_snapshot->nextRow = yEnd;
    }

    if (m_pending_snapshot->nextRow >= WATER_CPU_HEIGHTMAP_RES)
    {
      m_pending_snapshot->nextDomain++;
      m_pending_snapshot->nextRow = 0;
    }
  }

  return processed;
}

void ChopWaterGenerator::precompute_cpu_wave_constants()
{
  ensure_sin_table();

  // (!) do not use m_spectrum_cache in any other async function (i.e. update_cpu_snapshot_rows, finalize_snapshot_cascade)
  WaterWaveSpectrum &spectrum = m_spectrum_cache[0];
  const float invRes = 1.0f / (float)WATER_CPU_HEIGHTMAP_RES;
  const float TWO_PI = (float)M_PI * 2.0f;
  const float F1 = 16.0f * (float)M_PI;
  const float F2 = 32.0f * (float)M_PI;

  for (int d = 0; d < WATER_CPU_CASCADES_COUNT; ++d)
  {
    const WaterWaveSpectrumDomain &domain = spectrum.domains[d];
    const uint32_t wavesCount = (uint32_t)domain.waves_collision.size();
    m_cpu_wave_precomp[d].resize(wavesCount);
    m_cpu_wave_row_phase[d].resize(wavesCount);
    m_cpu_domain_size_in_meters[d] = domain.size_in_meters;
    for (uint32_t i = 0; i < wavesCount; ++i)
    {
      const WaterWave &w = domain.waves_collision[i];
      CPUWavePrecomp pre;
      pre.scale = (domain.size_in_meters / 32.0f) / w.wave_length;
      pre.tangentStep = w.tangent_norm * (invRes * pre.scale);
      pre.tOffset = (0.5f * invRes) * w.tangent_norm;
      pre.incThetaY = pre.tangentStep.y * 32.0f * TWO_PI;
      const float inc1 = pre.tangentStep.x * F1;
      const float inc2 = pre.tangentStep.x * F2;
      pre.sinInc1 = sinf(inc1);
      pre.cosInc1 = cosf(inc1);
      pre.sinInc2 = sinf(inc2);
      pre.cosInc2 = cosf(inc2);

      pre.amplitude = w.amplitude;
      pre.speed = w.speed;
      pre.phase = w.phase;
      pre.dirNormY = w.dir_norm.y;
      m_cpu_wave_precomp[d][i] = pre;

      // Precompute starting phases per row (y)
      m_cpu_wave_row_phase[d][i].resize(WATER_CPU_HEIGHTMAP_RES);
      for (uint32_t y = 0; y < WATER_CPU_HEIGHTMAP_RES; ++y)
      {
        const float texcoordY = ((float)y + 0.5f) * invRes;
        float baseX = texcoordY * w.dir_norm.x + pre.tOffset.x;
        baseX *= pre.scale;
        CPUWaveRowPhase rp;
        rp.s1 = sin_table_lookup(baseX * F1);
        rp.c1 = cos_table_lookup(baseX * F1);
        rp.s2 = sin_table_lookup(baseX * F2);
        rp.c2 = cos_table_lookup(baseX * F2);
        m_cpu_wave_row_phase[d][i][y] = rp;
      }
    }
  }
}

bool ChopWaterGenerator::is_cpu_snapshot_complete() const
{
  return m_pending_snapshot.has_value() && m_pending_snapshot->active && m_pending_snapshot->nextDomain >= WATER_CPU_CASCADES_COUNT;
}

void ChopWaterGenerator::finalize_snapshot_cascade(int cascadeIndex, int fifoIndex)
{
  if (!m_pending_snapshot.has_value() || !m_pending_snapshot->active)
    return;
  if (cascadeIndex < 0 || cascadeIndex >= WATER_CPU_CASCADES_COUNT)
    return;
  TIME_PROFILE(chop_finalize_snapshot);
  // Ensure ring buffer is initialized
  if (m_snapshot_write_index < 0)
  {
    m_cpu_snapshots.resize(5);
    m_snapshot_write_index = 0;
  }

  CPUHeightmapSnapshot &snap = m_cpu_snapshots[m_snapshot_write_index];
  snap.fifoIndex = fifoIndex;

  // Process single cascade domain
  const int d = cascadeIndex;
  float *raw = m_pending_snapshot->domain_heightmap[d].get();
  if (!snap.domain_heightmap[d])
    snap.domain_heightmap[d].reset(new float[WATER_CPU_HEIGHTMAP_RES * WATER_CPU_HEIGHTMAP_RES]);
  float *raw_dst = snap.domain_heightmap[d].get();
  if (raw && raw_dst)
  {
    CPUTextureSampler2D src(raw, WATER_CPU_HEIGHTMAP_RES, WATER_CPU_HEIGHTMAP_RES);
    CPUTextureSampler2D dst(raw_dst, WATER_CPU_HEIGHTMAP_RES, WATER_CPU_HEIGHTMAP_RES);
    float vals[4];
    float invCpuHMapRes = 1.0f / (float)WATER_CPU_HEIGHTMAP_RES;
    for (int32_t y = 0; y < WATER_CPU_HEIGHTMAP_RES; ++y)
    {
#ifdef CHOP_WATER_USE_SSE
      for (int32_t x = 0; x < WATER_CPU_HEIGHTMAP_RES; x += 4)
      {
        float u0 = ((float)(x + 0) + 0.5f) * invCpuHMapRes;
        float u1 = ((float)(x + 1) + 0.5f) * invCpuHMapRes;
        float u2 = ((float)(x + 2) + 0.5f) * invCpuHMapRes;
        float u3 = ((float)(x + 3) + 0.5f) * invCpuHMapRes;
        float v = ((float)y + 0.5f) * invCpuHMapRes;
        src.SampleLinearWrapTiled4(u0, u1, u2, u3, v, CHOP_TILING_OVERLAP, vals);
        dst.WritePixel(x + 0, y, vals[0]);
        dst.WritePixel(x + 1, y, vals[1]);
        dst.WritePixel(x + 2, y, vals[2]);
        dst.WritePixel(x + 3, y, vals[3]);
      }
#else
      for (uint32_t x = 0; x < WATER_CPU_HEIGHTMAP_RES; ++x)
      {
        Point2 tc(((float)x + 0.5f) * invCpuHMapRes, ((float)y + 0.5f) * invCpuHMapRes);
        float v = src.SampleLinearWrapTiled(tc, CHOP_TILING_OVERLAP);
        dst.WritePixel(x, y, v);
      }
#endif
    }
  }

  if (cascadeIndex == WATER_CPU_CASCADES_COUNT - 1)
  {
    snap.timestamp = m_pending_snapshot->timestamp;
    m_snapshot_write_index = (m_snapshot_write_index + 1) % (int)m_cpu_snapshots.size();
    m_pending_snapshot->active = false;
  }
}

int ChopWaterGenerator::get_cpu_snapshot_progress_rows() const
{
  if (!m_pending_snapshot.has_value() || !m_pending_snapshot->active)
    return 0;
  return m_pending_snapshot->nextDomain * WATER_CPU_HEIGHTMAP_RES + m_pending_snapshot->nextRow;
}

bool ChopWaterGenerator::export_displacement_row_u16(int domain, int dstWidth, int dstRow, uint16_t *dstPackedXYZ, float sea_level,
  float max_wave_height, int desiredFifoIndex, float &outMaxAbsZ) const
{
  if (domain < 0 || domain >= WATER_CPU_CASCADES_COUNT)
    return false;
  TIME_PROFILE(chop_export_u16);
  float *hm = nullptr;
  if (!m_cpu_snapshots.empty())
  {
    // Search latest snapshot with matching fifoIndex (fallback to latest if none)
    int best = -1;
    if (m_snapshot_write_index >= 0)
      best = (m_snapshot_write_index + (int)m_cpu_snapshots.size() - 1) % (int)m_cpu_snapshots.size();
    for (int i = 0; i < (int)m_cpu_snapshots.size(); ++i)
    {
      int idx = ((m_snapshot_write_index >= 0 ? m_snapshot_write_index : 0) + (int)m_cpu_snapshots.size() - 1 - i) %
                (int)m_cpu_snapshots.size();
      const CPUHeightmapSnapshot &s = m_cpu_snapshots[idx];
      if (s.fifoIndex == desiredFifoIndex)
      {
        best = idx;
        break;
      }
    }
    if (best >= 0)
      hm = m_cpu_snapshots[best].domain_heightmap[domain].get();
  }
  if (!hm)
  {
    return false;
  }

  outMaxAbsZ = 0.0f;
  // Encode as u16: x,z displacement are zero for CPU path; y is height above sea level remapped into [0..65535] using max_wave_height
  // This matches expected uint16 displacement packing used by FFT path when only height is needed.
  for (int x = 0; x < dstWidth; ++x)
  {
    float h = hm[dstRow * WATER_CPU_HEIGHTMAP_RES + x];
    float y = h;   // already absolute height at snapshot; store relative to sea level
    float rel = y; // if needed, subtract sea_level externally
    outMaxAbsZ = max(outMaxAbsZ, fabsf(rel));
    uint16_t uy = (uint16_t)clamp(int((rel / (max_wave_height * 2.f) + 0.5f) * 65535.f), 0, 65535);
    // pack as (x:0, y:uy, z:0)
    uint16_t *p = &dstPackedXYZ[x * 3];
    p[0] = 0;
    p[1] = uy;
    p[2] = 0;
  }
  return true;
}

#if DAGOR_DBGLEVEL > 0
static void chopWaterImguiWindow()
{
  using namespace convar;
  int chopDebug = chop_debug;
  if (ImGui::SliderInt("chop_debug", &chopDebug, 0, 2))
    chop_debug = chopDebug;

  float prevLastCascadeMul = convar::wave_prev_last_cascade_amplitude_mul;
  if (ImGui::SliderFloat("prev_last_cascade_mul", &prevLastCascadeMul, 0.0f, 10.0f, "%.2f"))
    convar::wave_prev_last_cascade_amplitude_mul = prevLastCascadeMul;

  float lastCascadeMul = convar::wave_last_cascade_amplitude_mul;
  if (ImGui::SliderFloat("last_cascade_mul", &lastCascadeMul, 0.0f, 10.0f, "%.2f"))
    convar::wave_last_cascade_amplitude_mul = lastCascadeMul;

  float detaiWavesMul = convar::detail_waves_amplitude;
  if (ImGui::SliderFloat("detail_waves_mul", &detaiWavesMul, 0.0f, 2.0f, "%.2f"))
    convar::detail_waves_amplitude = detaiWavesMul;

  float lastCascadeNMStrength = convar::last_cascade_nm_strength;
  if (ImGui::SliderFloat("last_cascade_NM", &lastCascadeNMStrength, 0.5f, 5.0f, "%.2f"))
    convar::last_cascade_nm_strength = lastCascadeNMStrength;

  ImGui::Separator();
  ImGui::Text("Foam Params");

  float foamTileSize = convar::foam_tile_size;
  if (ImGui::SliderFloat("foam_tile_size", &foamTileSize, 0.1f, 10.0f, "%.2f"))
    convar::foam_tile_size = foamTileSize;

  float foamCoverage = convar::foam_coverage;
  if (ImGui::SliderFloat("foam_coverage", &foamCoverage, 0.0f, 1.0f, "%.2f"))
    convar::foam_coverage = foamCoverage;

  float foamTurbidity = convar::foam_turbidity;
  if (ImGui::SliderFloat("foam_turbidity", &foamTurbidity, 0.0f, 1.0f, "%.2f"))
    convar::foam_turbidity = foamTurbidity;

  float foamSharpness = convar::foam_sharpness;
  if (ImGui::SliderFloat("foam_sharpness", &foamSharpness, 0.0f, 1.0f, "%.2f"))
    convar::foam_sharpness = foamSharpness;

  float foamRetention = convar::foam_retention;
  if (ImGui::SliderFloat("foam_retention", &foamRetention, 0.0f, 1.0f, "%.3f"))
    convar::foam_retention = foamRetention;

  float foamDissolveUvScale = convar::foam_dissolve_uv_scale;
  if (ImGui::SliderFloat("foam_dissolve_uv_scale", &foamDissolveUvScale, 0.5f, 8.0f, "%.2f"))
    convar::foam_dissolve_uv_scale = foamDissolveUvScale;

  ImGui::Separator();
  ImGui::Text("Low Quality Foam");
  float lowFoamTurbidity = convar::low_foam_turbidity;
  if (ImGui::SliderFloat("low_foam_turbidity", &lowFoamTurbidity, 0.0f, 1.0f, "%.2f"))
    convar::low_foam_turbidity = lowFoamTurbidity;
  float lowFoamSharpness = convar::low_foam_sharpness;
  if (ImGui::SliderFloat("low_foam_sharpness", &lowFoamSharpness, 0.0f, 1.0f, "%.2f"))
    convar::low_foam_sharpness = lowFoamSharpness;
  float lowFoamCoverage = convar::low_foam_coverage;
  if (ImGui::SliderFloat("low_foam_coverage", &lowFoamCoverage, 0.0f, 1.0f, "%.2f"))
    convar::low_foam_coverage = lowFoamCoverage;

  ImGui::Text("Micro Foam");
  float qH0 = convar::q_height_mul_x;
  if (ImGui::SliderFloat("q_h0_mul", &qH0, 0.0f, 5.0f, "%.2f"))
    convar::q_height_mul_x = qH0;
  float qH1 = convar::q_height_mul_y;
  if (ImGui::SliderFloat("q_h1_mul", &qH1, 0.0f, 5.0f, "%.2f"))
    convar::q_height_mul_y = qH1;
  float qH2 = convar::q_height_mul_z;
  if (ImGui::SliderFloat("q_h2_mul", &qH2, 0.0f, 5.0f, "%.2f"))
    convar::q_height_mul_z = qH2;
  float qH3 = convar::q_height_mul_w;
  if (ImGui::SliderFloat("q_h3_mul", &qH3, 0.0f, 5.0f, "%.2f"))
    convar::q_height_mul_w = qH3;
  float qH4 = convar::q_height_mul4;
  if (ImGui::SliderFloat("q_h4_mul", &qH4, 0.0f, 10.0f, "%.2f"))
    convar::q_height_mul4 = qH4;
  float qHeightMulTex = convar::q_height_mul_tex;
  if (ImGui::SliderFloat("q_height_mul_tex", &qHeightMulTex, 0.0f, 1.0f, "%.2f"))
    convar::q_height_mul_tex = qHeightMulTex;
  float qPow = convar::q_power;
  if (ImGui::SliderFloat("q_power", &qPow, 1.0f, 50.0f, "%.1f"))
    convar::q_power = qPow;
  float qSigMul = convar::q_sig_wave_height_mul;
  if (ImGui::SliderFloat("q_sig_wave_height_mul", &qSigMul, 0.05f, 40.0f, "%.2f"))
    convar::q_sig_wave_height_mul = qSigMul;

  float foamDetailAmpMul = convar::foam_detail_amp_mul;
  if (ImGui::SliderFloat("foam_detail_amp_mul", &foamDetailAmpMul, 0.0f, 4.0f, "%.2f"))
    convar::foam_detail_amp_mul = foamDetailAmpMul;

  ImGui::Separator();
  ImGui::Text("Detail Waves Params");

  float maskScale = convar::detail_waves_mask_scale;
  if (ImGui::SliderFloat("mask_scale", &maskScale, 0.1f, 10.0f, "%.2f"))
    convar::detail_waves_mask_scale = maskScale;

  float distortStr = convar::detail_waves_distort_str;
  if (ImGui::SliderFloat("distort_str", &distortStr, 0.0f, 1.0f, "%.2f"))
    convar::detail_waves_distort_str = distortStr;

  float detailBias = convar::detail_waves_detail_bias;
  if (ImGui::SliderFloat("detail_bias", &detailBias, 0.0f, 0.5f, "%.2f"))
    convar::detail_waves_detail_bias = detailBias;

  float detailStrength = convar::detail_waves_detail_strength;
  if (ImGui::SliderFloat("detail_strength", &detailStrength, 0.0f, 1.0f, "%.2f"))
    convar::detail_waves_detail_strength = detailStrength;

  ImGui::Separator();
  ImGui::Text("Scattering");
  float scatter = scatter_intensity;
  if (ImGui::SliderFloat("Scatter Intensity", &scatter, 0.0f, 40.0f, "%.2f"))
    scatter_intensity = scatter;

  float scatter_pow = scatter_intensity_pow;
  if (ImGui::SliderFloat("Scatter Intensity Pow", &scatter_pow, 0.0f, 5.0f, "%.2f"))
    scatter_intensity_pow = scatter_pow;

  float skylightMul = scatter_intensity_skylight_mul;
  if (ImGui::SliderFloat("Scatter SkyLight Multiplier", &skylightMul, 0.0f, 10.0f, "%.2f"))
    scatter_intensity_skylight_mul = skylightMul;
}

REGISTER_IMGUI_WINDOW("Render", "Chop Water", chopWaterImguiWindow);
#endif
