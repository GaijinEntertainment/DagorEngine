// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <fftWater/fftWater.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_string.h>
#include <util/dag_lookup.h>
#include <debug/dag_assert.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathUtils.h>

namespace fft_water
{
WavePreset::WavePreset() {}

G_STATIC_ASSERT(BEAUFORT_SCALES_NUM == 7);
G_STATIC_ASSERT((int)Spectrum::LAST == 1);

static const WavePresets def_waves_phillips = {
  // Beaufort 0
  WavePreset(10.0f, 0.3f, 0.0f, 0.15f,
    SimulationParams({0.01f, 0.01f, 0.01f, 0.01f, 0.01f}, 1.0f, 1.0f, 1.0f, 0.03f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f),
    ChopWaterProps{.domain_size = 300.0f,
      .wind_speed = 0.0f,
      .amplitude_scale = 0.8f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.0f,
      .foam_tile_size = 4.0f,
      .foam_coverage = 0.35f,
      .foam_turbidity = 0.6f,
      .foam_sharpness = 0.45f,
      .foam_retention = 0.14f,
      .foam_dissolve_scale = 2.2f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 1.0f,
      .low_foam_sharpness = 1.0f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.8f,
      .micro_foam_h2 = 0.9f,
      .micro_foam_h3 = 1.0f,
      .micro_foam_h4 = 1.5f,
      .micro_foam_htex = 0.05f,
      .micro_foam_q_power = 14.0f,
      .micro_foam_q_sig_wave_mul = 12.0f,
      .detail_last_cascade_mul = 1.0f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.02f,
      .detail_mask_strength = 0.8f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.0f,
      .scatter_intensity = 1.8f,
      .scatter_intensity_pow = 1.0f,
      .scatter_intensity_skylight_mul = 1.0f}),
  // Beaufort 1
  WavePreset(60.0f, 0.9f, 0.0f, 0.15f, SimulationParams({1.0f, 1.0f, 1.0f, 0.7f, 0.7f}, 1.0f, 0.9f, 1.5f, 0.05f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f),
    ChopWaterProps{.domain_size = 500.0f,
      .wind_speed = 1.0f,
      .amplitude_scale = 0.5f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.0f,
      .foam_tile_size = 4.0f,
      .foam_coverage = 0.3f,
      .foam_turbidity = 0.9f,
      .foam_sharpness = 0.9f,
      .foam_retention = 0.1f,
      .foam_dissolve_scale = 5.0f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 1.0f,
      .low_foam_sharpness = 1.0f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.2f,
      .micro_foam_h2 = 2.0f,
      .micro_foam_h3 = 2.0f,
      .micro_foam_h4 = 1.2f,
      .micro_foam_htex = 0.03f,
      .micro_foam_q_power = 20.0f,
      .micro_foam_q_sig_wave_mul = 10.0f,
      .detail_last_cascade_mul = 2.2f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.27f,
      .detail_mask_strength = 0.35f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.0f,
      .scatter_intensity = 2.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 2.0f}),
  // Beaufort 2
  WavePreset(250.0f, 2.5f, 0.0f, 0.03f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 2.0f, 0.08f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f),
    ChopWaterProps{.domain_size = 600.0f,
      .wind_speed = 2.0f,
      .amplitude_scale = 0.9f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.9f,
      .foam_sharpness = 0.65f,
      .foam_retention = 0.05f,
      .foam_dissolve_scale = 6.0f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 0.7f,
      .low_foam_sharpness = 0.7f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.4f,
      .micro_foam_h2 = 1.0f,
      .micro_foam_h3 = 0.4f,
      .micro_foam_h4 = 0.4f,
      .micro_foam_htex = 0.08f,
      .micro_foam_q_power = 16.0f,
      .micro_foam_q_sig_wave_mul = 3.0f,
      .detail_last_cascade_mul = 2.0f,
      .detail_last_cascade_nm = 1.1f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.06f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.6f,
      .scatter_intensity = 6.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 7.0f}),
  // Beaufort 3
  WavePreset(750.0f, 4.5f, 0.0f, 0.01f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.2f, 0.075f, 0.75f, 0.985f, 10.0f, 0.3f, 0.32f),
    ChopWaterProps{.domain_size = 750.0f,
      .wind_speed = 3.3f,
      .amplitude_scale = 0.8f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.2f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.6f,
      .foam_sharpness = 0.78f,
      .foam_retention = 0.08f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 2.5f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.9f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 1.0f,
      .micro_foam_h1 = 0.6f,
      .micro_foam_h2 = 1.5f,
      .micro_foam_h3 = 2.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.1f,
      .micro_foam_q_power = 10.0f,
      .micro_foam_q_sig_wave_mul = 4.0f,
      .detail_last_cascade_mul = 2.0f,
      .detail_last_cascade_nm = 1.2f,
      .detail_waves_mul = 0.25f,
      .detail_mask_strength = 0.08f,
      .detail_mask_bias = 0.25f,
      .detail_prev_last_cascade_mul = 1.3f,
      .scatter_intensity = 12.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 4
  WavePreset(750.0f, 6.7f, 0.0f, 0.01f, SimulationParams({1.0f, 1.0f, 1.0f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.07f, 0.75f, 0.985f, 10.0f, 0.55f, 0.6f),
    ChopWaterProps{.domain_size = 850.0f,
      .wind_speed = 4.0f,
      .amplitude_scale = 0.8f,
      .wind_spread = 0.8f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.35f,
      .foam_turbidity = 0.7f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.2f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 3.2f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.7f,
      .micro_foam_h1 = 0.4f,
      .micro_foam_h2 = 1.8f,
      .micro_foam_h3 = 3.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.08f,
      .micro_foam_q_power = 9.6f,
      .micro_foam_q_sig_wave_mul = 2.4f,
      .detail_last_cascade_mul = 2.5f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.15f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 10.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 5
  WavePreset(1000.0f, 9.4f, 0.0f, 0.004f,
    SimulationParams({1.0f, 1.0f, 0.75f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.065f, 0.75f, 0.985f, 10.0f, 0.65f, 0.7f),
    ChopWaterProps{.domain_size = 1000.0f,
      .wind_speed = 6.0f,
      .amplitude_scale = 0.7f,
      .wind_spread = 0.6f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 3.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.8f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.2f,
      .foam_dissolve_scale = 3.0f,
      .foam_detail_amp_mul = 4.0f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 1.0f,
      .micro_foam_h2 = 2.0f,
      .micro_foam_h3 = 3.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.1f,
      .micro_foam_q_power = 9.8f,
      .micro_foam_q_sig_wave_mul = 1.6f,
      .detail_last_cascade_mul = 5.0f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.3f,
      .detail_mask_strength = 0.15f,
      .detail_mask_bias = 0.15f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 11.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 6
  WavePreset(1150.0f, 12.3f, 0.0f, 0.003f,
    SimulationParams({1.0f, 1.0f, 0.75f, 0.5f, 0.5f}, 1.0f, 0.7f, 3.0f, 0.1f, Spectrum::PHILLIPS),
    FoamParams(0.25f, 0.05f, 0.75f, 0.985f, 10.0f, 0.7f, 0.75f),
    ChopWaterProps{.domain_size = 1000.0f,
      .wind_speed = 7.0f,
      .amplitude_scale = 0.7f,
      .wind_spread = 0.5f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 3.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.7f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.25f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 4.0f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.18f,
      .micro_foam_h1 = 0.3f,
      .micro_foam_h2 = 3.2f,
      .micro_foam_h3 = 3.2f,
      .micro_foam_h4 = 4.0f,
      .micro_foam_htex = 0.12f,
      .micro_foam_q_power = 9.6f,
      .micro_foam_q_sig_wave_mul = 1.2f,
      .detail_last_cascade_mul = 3.5f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.2f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 11.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f})};

static const WavePresets def_waves_unified_directional = {{// Beaufort 0
  WavePreset(10.0f, 0.3f, 0.0f, 0.0f,
    SimulationParams({0.01f, 0.01f, 0.01f, 0.01f, 0.01f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f),
    ChopWaterProps{.domain_size = 300.0f,
      .wind_speed = 0.0f,
      .amplitude_scale = 0.8f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.0f,
      .foam_tile_size = 4.0f,
      .foam_coverage = 0.35f,
      .foam_turbidity = 0.6f,
      .foam_sharpness = 0.45f,
      .foam_retention = 0.14f,
      .foam_dissolve_scale = 2.2f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 1.0f,
      .low_foam_sharpness = 1.0f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.8f,
      .micro_foam_h2 = 0.9f,
      .micro_foam_h3 = 1.0f,
      .micro_foam_h4 = 1.5f,
      .micro_foam_htex = 0.05f,
      .micro_foam_q_power = 14.0f,
      .micro_foam_q_sig_wave_mul = 12.0f,
      .detail_last_cascade_mul = 1.0f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.02f,
      .detail_mask_strength = 0.8f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.0f,
      .scatter_intensity = 1.8f,
      .scatter_intensity_pow = 1.0f,
      .scatter_intensity_skylight_mul = 1.0f}),
  // Beaufort 1
  WavePreset(60.0f, 0.9f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f),
    ChopWaterProps{.domain_size = 500.0f,
      .wind_speed = 1.0f,
      .amplitude_scale = 0.5f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.0f,
      .foam_tile_size = 4.0f,
      .foam_coverage = 0.3f,
      .foam_turbidity = 0.9f,
      .foam_sharpness = 0.9f,
      .foam_retention = 0.1f,
      .foam_dissolve_scale = 5.0f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 1.0f,
      .low_foam_sharpness = 1.0f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.2f,
      .micro_foam_h2 = 2.0f,
      .micro_foam_h3 = 2.0f,
      .micro_foam_h4 = 1.2f,
      .micro_foam_htex = 0.03f,
      .micro_foam_q_power = 20.0f,
      .micro_foam_q_sig_wave_mul = 10.0f,
      .detail_last_cascade_mul = 2.2f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.27f,
      .detail_mask_strength = 0.35f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.0f,
      .scatter_intensity = 2.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 2.0f}),
  // Beaufort 2
  WavePreset(250.0f, 2.5f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f),
    ChopWaterProps{.domain_size = 600.0f,
      .wind_speed = 2.0f,
      .amplitude_scale = 0.9f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.9f,
      .foam_sharpness = 0.65f,
      .foam_retention = 0.05f,
      .foam_dissolve_scale = 6.0f,
      .foam_detail_amp_mul = 1.0f,
      .low_foam_turbidity = 0.7f,
      .low_foam_sharpness = 0.7f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 0.4f,
      .micro_foam_h2 = 1.0f,
      .micro_foam_h3 = 0.4f,
      .micro_foam_h4 = 0.4f,
      .micro_foam_htex = 0.08f,
      .micro_foam_q_power = 16.0f,
      .micro_foam_q_sig_wave_mul = 3.0f,
      .detail_last_cascade_mul = 2.0f,
      .detail_last_cascade_nm = 1.1f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.06f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.6f,
      .scatter_intensity = 6.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 7.0f}),
  // Beaufort 3
  WavePreset(750.0f, 4.5f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.2f, 0.14f, 0.7f, 0.99f, 10.0f, 0.4f, 0.65f),
    ChopWaterProps{.domain_size = 750.0f,
      .wind_speed = 3.3f,
      .amplitude_scale = 0.8f,
      .wind_spread = 1.0f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.2f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.6f,
      .foam_sharpness = 0.78f,
      .foam_retention = 0.08f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 2.5f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.9f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 1.0f,
      .micro_foam_h1 = 0.6f,
      .micro_foam_h2 = 1.5f,
      .micro_foam_h3 = 2.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.1f,
      .micro_foam_q_power = 10.0f,
      .micro_foam_q_sig_wave_mul = 4.0f,
      .detail_last_cascade_mul = 2.0f,
      .detail_last_cascade_nm = 1.2f,
      .detail_waves_mul = 0.25f,
      .detail_mask_strength = 0.08f,
      .detail_mask_bias = 0.25f,
      .detail_prev_last_cascade_mul = 1.3f,
      .scatter_intensity = 12.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 4
  WavePreset(750.0f, 6.7f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.14f, 0.7f, 0.99f, 10.0f, 0.65f, 0.8f),
    ChopWaterProps{.domain_size = 850.0f,
      .wind_speed = 4.0f,
      .amplitude_scale = 0.8f,
      .wind_spread = 0.8f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 6.0f,
      .foam_coverage = 0.35f,
      .foam_turbidity = 0.7f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.2f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 3.2f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.7f,
      .micro_foam_h1 = 0.4f,
      .micro_foam_h2 = 1.8f,
      .micro_foam_h3 = 3.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.08f,
      .micro_foam_q_power = 9.6f,
      .micro_foam_q_sig_wave_mul = 2.4f,
      .detail_last_cascade_mul = 2.5f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.15f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 10.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 5
  WavePreset(1000.0f, 9.4f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.12f, 0.7f, 0.99f, 10.0f, 0.6f, 0.8f),
    ChopWaterProps{.domain_size = 1000.0f,
      .wind_speed = 6.0f,
      .amplitude_scale = 0.7f,
      .wind_spread = 0.6f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 3.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.8f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.2f,
      .foam_dissolve_scale = 3.0f,
      .foam_detail_amp_mul = 4.0f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.2f,
      .micro_foam_h1 = 1.0f,
      .micro_foam_h2 = 2.0f,
      .micro_foam_h3 = 3.0f,
      .micro_foam_h4 = 2.0f,
      .micro_foam_htex = 0.1f,
      .micro_foam_q_power = 9.8f,
      .micro_foam_q_sig_wave_mul = 1.6f,
      .detail_last_cascade_mul = 5.0f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.3f,
      .detail_mask_strength = 0.15f,
      .detail_mask_bias = 0.15f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 11.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f}),
  // Beaufort 6
  WavePreset(1150.0f, 12.3f, 0.0f, 0.0f,
    SimulationParams({1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 0.95f, 0.0f, 0.05f, Spectrum::UNIFIED_DIRECTIONAL),
    FoamParams(0.27f, 0.09f, 0.7f, 0.99f, 10.0f, 0.6f, 0.8f),
    ChopWaterProps{.domain_size = 1000.0f,
      .wind_speed = 7.0f,
      .amplitude_scale = 0.7f,
      .wind_spread = 0.5f,
      .opposing_damping = 0.01f,
      .opposing_ratio = 0.0f,
      .choppiness = 1.5f,
      .foam_tile_size = 3.0f,
      .foam_coverage = 0.4f,
      .foam_turbidity = 0.7f,
      .foam_sharpness = 0.7f,
      .foam_retention = 0.25f,
      .foam_dissolve_scale = 4.0f,
      .foam_detail_amp_mul = 4.0f,
      .low_foam_turbidity = 0.8f,
      .low_foam_sharpness = 0.8f,
      .low_foam_coverage = 0.3f,
      .micro_foam_h0 = 0.18f,
      .micro_foam_h1 = 0.3f,
      .micro_foam_h2 = 3.2f,
      .micro_foam_h3 = 3.2f,
      .micro_foam_h4 = 4.0f,
      .micro_foam_htex = 0.12f,
      .micro_foam_q_power = 9.6f,
      .micro_foam_q_sig_wave_mul = 1.2f,
      .detail_last_cascade_mul = 3.5f,
      .detail_last_cascade_nm = 1.0f,
      .detail_waves_mul = 0.2f,
      .detail_mask_strength = 0.2f,
      .detail_mask_bias = 0.2f,
      .detail_prev_last_cascade_mul = 1.4f,
      .scatter_intensity = 11.0f,
      .scatter_intensity_pow = 5.0f,
      .scatter_intensity_skylight_mul = 9.0f})}};

const WavePresets &defWaves(const Spectrum &spectrum)
{
  switch (spectrum)
  {
    case Spectrum::PHILLIPS: return def_waves_phillips;
    case Spectrum::UNIFIED_DIRECTIONAL: return def_waves_unified_directional;
  }
  return def_waves_phillips;
}

const WavePreset &defWaves(const Spectrum &spectrum, int def_preset_no) { return defWaves(spectrum)[def_preset_no]; }


void load_wave_preset(const DataBlock *presetBlk, WavePreset &preset, int def_preset_no)
{
  G_ASSERT(presetBlk);

  Spectrum spectrum = (Spectrum)lup(presetBlk->getStr("spectra", ""), spectrum_names, countof(spectrum_names), (int)Spectrum::DEFAULT);

  String blkName;
  preset.period = presetBlk->getReal("period", defWaves(spectrum, def_preset_no).period);
  preset.wind = presetBlk->getReal("wind", defWaves(spectrum, def_preset_no).wind);
  preset.roughness = presetBlk->getReal("roughness", defWaves(spectrum, def_preset_no).roughness);
  preset.smallWaveFraction = presetBlk->getReal("smallWaveFraction", defWaves(spectrum, def_preset_no).smallWaveFraction);

  preset.simulation.choppiness = presetBlk->getReal("choppiness", defWaves(spectrum, def_preset_no).simulation.choppiness);
  preset.simulation.facetSize = presetBlk->getReal("facetSize", defWaves(spectrum, def_preset_no).simulation.facetSize);
  preset.simulation.windDependency = presetBlk->getReal("windDependency", defWaves(spectrum, def_preset_no).simulation.windDependency);
  preset.simulation.windAlignment = presetBlk->getReal("windAlignment", defWaves(spectrum, def_preset_no).simulation.windAlignment);
  preset.simulation.spectrum = spectrum;

  for (int ampNo = 0; ampNo < preset.simulation.amplitude.size(); ++ampNo)
  {
    blkName.printf(0, "amplitude%d", ampNo);
    const DataBlock *ampBlk = presetBlk->getBlockByName(blkName);
    if (!ampBlk)
    {
      preset.simulation.amplitude[ampNo] = defWaves(spectrum, def_preset_no).simulation.amplitude[ampNo];
      continue;
    }
    preset.simulation.amplitude[ampNo] = ampBlk->getReal("amplitude", defWaves(spectrum, def_preset_no).simulation.amplitude[ampNo]);
  }

  preset.foam.generation_threshold =
    presetBlk->getReal("foam_generation_threshold", defWaves(spectrum, def_preset_no).foam.generation_threshold);
  preset.foam.generation_amount =
    presetBlk->getReal("foam_generation_amount", defWaves(spectrum, def_preset_no).foam.generation_amount);
  preset.foam.dissipation_speed =
    presetBlk->getReal("foam_dissipation_speed", defWaves(spectrum, def_preset_no).foam.dissipation_speed);
  preset.foam.falloff_speed = presetBlk->getReal("foam_falloff_speed", defWaves(spectrum, def_preset_no).foam.falloff_speed);
  preset.foam.hats_mul = presetBlk->getReal("foam_hats_mul", defWaves(spectrum, def_preset_no).foam.hats_mul);
  preset.foam.hats_threshold = presetBlk->getReal("foam_hats_threshold", defWaves(spectrum, def_preset_no).foam.hats_threshold);
  preset.foam.hats_folding = presetBlk->getReal("foam_hats_folding", defWaves(spectrum, def_preset_no).foam.hats_folding);
  preset.foam.surface_folding_foam_mul =
    presetBlk->getReal("surface_folding_foam_mul", defWaves(spectrum, def_preset_no).foam.surface_folding_foam_mul);
  preset.foam.surface_folding_foam_pow =
    presetBlk->getReal("surface_folding_foam_pow", defWaves(spectrum, def_preset_no).foam.surface_folding_foam_pow);

  // ChopWaterProps
  const ChopWaterProps &defChop = defWaves(spectrum, def_preset_no).chopWater;
  preset.chopWater.wind_speed = presetBlk->getReal("chop_wind_speed", defChop.wind_speed);
  preset.chopWater.amplitude_scale = presetBlk->getReal("chop_amplitude_scale", defChop.amplitude_scale);
  preset.chopWater.wind_spread = presetBlk->getReal("chop_wind_spread", defChop.wind_spread);
  preset.chopWater.opposing_damping = presetBlk->getReal("chop_opposing_damping", defChop.opposing_damping);
  preset.chopWater.opposing_ratio = presetBlk->getReal("chop_opposing_ratio", defChop.opposing_ratio);
  preset.chopWater.choppiness = presetBlk->getReal("chop_choppiness", defChop.choppiness);

  preset.chopWater.foam_tile_size = presetBlk->getReal("chop_foam_tile_size", defChop.foam_tile_size);
  preset.chopWater.foam_coverage = presetBlk->getReal("chop_foam_coverage", defChop.foam_coverage);
  preset.chopWater.foam_turbidity = presetBlk->getReal("chop_foam_turbidity", defChop.foam_turbidity);
  preset.chopWater.foam_sharpness = presetBlk->getReal("chop_foam_sharpness", defChop.foam_sharpness);
  preset.chopWater.foam_retention = presetBlk->getReal("chop_foam_retention", defChop.foam_retention);
  preset.chopWater.low_foam_turbidity = presetBlk->getReal("chop_low_foam_turbidity", defChop.low_foam_turbidity);
  preset.chopWater.low_foam_sharpness = presetBlk->getReal("chop_low_foam_sharpness", defChop.low_foam_sharpness);
  preset.chopWater.low_foam_coverage = presetBlk->getReal("chop_low_foam_coverage", defChop.low_foam_coverage);
  preset.chopWater.foam_dissolve_scale = presetBlk->getReal("chop_foam_dissolve_scale", defChop.foam_dissolve_scale);
  preset.chopWater.foam_detail_amp_mul = presetBlk->getReal("chop_foam_detail_amp_mul", defChop.foam_detail_amp_mul);

  preset.chopWater.micro_foam_h0 = presetBlk->getReal("chop_micro_foam_h0", defChop.micro_foam_h0);
  preset.chopWater.micro_foam_h1 = presetBlk->getReal("chop_micro_foam_h1", defChop.micro_foam_h1);
  preset.chopWater.micro_foam_h2 = presetBlk->getReal("chop_micro_foam_h2", defChop.micro_foam_h2);
  preset.chopWater.micro_foam_h3 = presetBlk->getReal("chop_micro_foam_h3", defChop.micro_foam_h3);
  preset.chopWater.micro_foam_h4 = presetBlk->getReal("chop_micro_foam_h4", defChop.micro_foam_h4);
  preset.chopWater.micro_foam_htex = presetBlk->getReal("chop_micro_foam_htex", defChop.micro_foam_htex);
  preset.chopWater.micro_foam_q_power = presetBlk->getReal("chop_micro_foam_q_power", defChop.micro_foam_q_power);
  preset.chopWater.micro_foam_q_sig_wave_mul = presetBlk->getReal("chop_micro_foam_q_sig_wave_mul", defChop.micro_foam_q_sig_wave_mul);

  preset.chopWater.detail_last_cascade_mul = presetBlk->getReal("chop_detail_last_cascade_mul", defChop.detail_last_cascade_mul);
  preset.chopWater.detail_last_cascade_nm = presetBlk->getReal("chop_detail_last_cascade_nm", defChop.detail_last_cascade_nm);
  preset.chopWater.detail_waves_mul = presetBlk->getReal("chop_detail_waves_mul", defChop.detail_waves_mul);
  preset.chopWater.detail_mask_strength = presetBlk->getReal("chop_detail_mask_strength", defChop.detail_mask_strength);
  preset.chopWater.detail_mask_bias = presetBlk->getReal("chop_detail_mask_bias", defChop.detail_mask_bias);

  preset.chopWater.scatter_intensity = presetBlk->getReal("scatter_intensity", defChop.scatter_intensity);
  preset.chopWater.scatter_intensity_pow = presetBlk->getReal("scatter_intensity_pow", defChop.scatter_intensity_pow);
  preset.chopWater.scatter_intensity_skylight_mul =
    presetBlk->getReal("scatter_intensity_skylight_mul", defChop.scatter_intensity_skylight_mul);
}

void save_wave_preset(DataBlock *presetBlk, WavePreset &preset)
{
  G_ASSERT(presetBlk);

  String blkName;
  presetBlk->setReal("period", preset.period);
  presetBlk->setReal("wind", preset.wind);
  presetBlk->setReal("roughness", preset.roughness);
  presetBlk->setReal("smallWaveFraction", preset.smallWaveFraction);

  presetBlk->setReal("choppiness", preset.simulation.choppiness);
  presetBlk->setReal("facetSize", preset.simulation.facetSize);
  presetBlk->setReal("windDependency", preset.simulation.windDependency);
  presetBlk->setReal("windAlignment", preset.simulation.windAlignment);
  presetBlk->setStr("spectra", spectrum_names[(int)preset.simulation.spectrum]);

  for (int ampNo = 0; ampNo < preset.simulation.amplitude.size(); ++ampNo)
  {
    blkName.printf(0, "amplitude%d", ampNo);
    DataBlock *ampBlk = presetBlk->addBlock(blkName);
    G_ASSERT(ampBlk);
    ampBlk->setReal("amplitude", preset.simulation.amplitude[ampNo]);
  }

  presetBlk->setReal("foam_generation_threshold", preset.foam.generation_threshold);
  presetBlk->setReal("foam_generation_amount", preset.foam.generation_amount);
  presetBlk->setReal("foam_dissipation_speed", preset.foam.dissipation_speed);
  presetBlk->setReal("foam_falloff_speed", preset.foam.falloff_speed);
  presetBlk->setReal("foam_hats_mul", preset.foam.hats_mul);
  presetBlk->setReal("foam_hats_threshold", preset.foam.hats_threshold);
  presetBlk->setReal("foam_hats_folding", preset.foam.hats_folding);
  presetBlk->setReal("surface_folding_foam_mul", preset.foam.surface_folding_foam_mul);
  presetBlk->setReal("surface_folding_foam_pow", preset.foam.surface_folding_foam_pow);

  // ChopWaterProps
  presetBlk->setReal("chop_wind_speed", preset.chopWater.wind_speed);
  presetBlk->setReal("chop_amplitude_scale", preset.chopWater.amplitude_scale);
  presetBlk->setReal("chop_wind_spread", preset.chopWater.wind_spread);
  presetBlk->setReal("chop_opposing_damping", preset.chopWater.opposing_damping);
  presetBlk->setReal("chop_opposing_ratio", preset.chopWater.opposing_ratio);
  presetBlk->setReal("chop_choppiness", preset.chopWater.choppiness);

  presetBlk->setReal("chop_foam_tile_size", preset.chopWater.foam_tile_size);
  presetBlk->setReal("chop_foam_coverage", preset.chopWater.foam_coverage);
  presetBlk->setReal("chop_foam_turbidity", preset.chopWater.foam_turbidity);
  presetBlk->setReal("chop_foam_sharpness", preset.chopWater.foam_sharpness);
  presetBlk->setReal("chop_foam_retention", preset.chopWater.foam_retention);
  presetBlk->setReal("chop_low_foam_turbidity", preset.chopWater.low_foam_turbidity);
  presetBlk->setReal("chop_low_foam_sharpness", preset.chopWater.low_foam_sharpness);
  presetBlk->setReal("chop_low_foam_coverage", preset.chopWater.low_foam_coverage);
  presetBlk->setReal("chop_foam_dissolve_scale", preset.chopWater.foam_dissolve_scale);
  presetBlk->setReal("chop_foam_detail_amp_mul", preset.chopWater.foam_detail_amp_mul);

  presetBlk->setReal("chop_micro_foam_h0", preset.chopWater.micro_foam_h0);
  presetBlk->setReal("chop_micro_foam_h1", preset.chopWater.micro_foam_h1);
  presetBlk->setReal("chop_micro_foam_h2", preset.chopWater.micro_foam_h2);
  presetBlk->setReal("chop_micro_foam_h3", preset.chopWater.micro_foam_h3);
  presetBlk->setReal("chop_micro_foam_h4", preset.chopWater.micro_foam_h4);
  presetBlk->setReal("chop_micro_foam_htex", preset.chopWater.micro_foam_htex);
  presetBlk->setReal("chop_micro_foam_q_power", preset.chopWater.micro_foam_q_power);
  presetBlk->setReal("chop_micro_foam_q_sig_wave_mul", preset.chopWater.micro_foam_q_sig_wave_mul);

  presetBlk->setReal("chop_detail_last_cascade_mul", preset.chopWater.detail_last_cascade_mul);
  presetBlk->setReal("chop_detail_last_cascade_nm", preset.chopWater.detail_last_cascade_nm);
  presetBlk->setReal("chop_detail_waves_mul", preset.chopWater.detail_waves_mul);
  presetBlk->setReal("chop_detail_mask_strength", preset.chopWater.detail_mask_strength);
  presetBlk->setReal("chop_detail_mask_bias", preset.chopWater.detail_mask_bias);
  presetBlk->setReal("chop_domain_size", preset.chopWater.domain_size);
  presetBlk->setReal("chop_detail_prev_last_cascade_mul", preset.chopWater.detail_prev_last_cascade_mul);

  presetBlk->setReal("scatter_intensity", preset.chopWater.scatter_intensity);
  presetBlk->setReal("scatter_intensity_pow", preset.chopWater.scatter_intensity_pow);
  presetBlk->setReal("scatter_intensity_skylight_mul", preset.chopWater.scatter_intensity_skylight_mul);
}

void load_wave_presets(const DataBlock &blk, WavePresets &out_waves)
{
  for (int presetNo = 0; presetNo < out_waves.presets.size(); ++presetNo)
  {
    fft_water::WavePreset &preset = out_waves[presetNo];

    String blkName = String(0, "preset%d", presetNo);
    const DataBlock *presetBlk = blk.getBlockByName(blkName);
    if (!presetBlk)
    {
      preset = defWaves(Spectrum::DEFAULT)[presetNo];
      continue;
    }

    load_wave_preset(presetBlk, preset, presetNo);
  }
}

void apply_wave_preset(FFTWater *water, const WavePreset &preset, const Point2 &wind_dir)
{
  set_period(water, preset.period);
  set_roughness(water, preset.roughness, 0);
  set_small_wave_fraction(water, preset.smallWaveFraction);
  set_simulation_params(water, preset.simulation);
  set_foam(water, preset.foam);
  set_chop_water_props(water, preset.chopWater);
  set_wind_speed(water, preset.wind, preset.chopWater.wind_speed, wind_dir);
  reset_physics(water);
}

void apply_wave_preset(FFTWater *water, const WavePreset &preset1, const WavePreset &preset2, float alpha, const Point2 &wind_dir)
{
  G_STATIC_ASSERT(MAX_NUM_CASCADES == 5);
  WavePreset preset(lerp(preset1.period, preset2.period, alpha), lerp(preset1.wind, preset2.wind, alpha),
    lerp(preset1.roughness, preset2.roughness, alpha), lerp(preset1.smallWaveFraction, preset2.smallWaveFraction, alpha),
    SimulationParams({lerp(preset1.simulation.amplitude[0], preset2.simulation.amplitude[0], alpha),
                       lerp(preset1.simulation.amplitude[1], preset2.simulation.amplitude[1], alpha),
                       lerp(preset1.simulation.amplitude[2], preset2.simulation.amplitude[2], alpha),
                       lerp(preset1.simulation.amplitude[3], preset2.simulation.amplitude[3], alpha),
                       lerp(preset1.simulation.amplitude[4], preset2.simulation.amplitude[4], alpha)},
      lerp(preset1.simulation.choppiness, preset2.simulation.choppiness, alpha),
      lerp(preset1.simulation.windDependency, preset2.simulation.windDependency, alpha),
      lerp(preset1.simulation.windAlignment, preset2.simulation.windAlignment, alpha),
      lerp(preset1.simulation.facetSize, preset2.simulation.facetSize, alpha), preset2.simulation.spectrum),
    FoamParams(lerp(preset1.foam.generation_threshold, preset2.foam.generation_threshold, alpha),
      lerp(preset1.foam.generation_amount, preset2.foam.generation_amount, alpha),
      lerp(preset1.foam.dissipation_speed, preset2.foam.dissipation_speed, alpha),
      lerp(preset1.foam.falloff_speed, preset2.foam.falloff_speed, alpha), lerp(preset1.foam.hats_mul, preset2.foam.hats_mul, alpha),
      lerp(preset1.foam.hats_threshold, preset2.foam.hats_threshold, alpha),
      lerp(preset1.foam.hats_folding, preset2.foam.hats_folding, alpha),
      lerp(preset1.foam.surface_folding_foam_mul, preset2.foam.surface_folding_foam_mul, alpha),
      lerp(preset1.foam.surface_folding_foam_pow, preset2.foam.surface_folding_foam_pow, alpha)));

  ChopWaterProps blended;
  blended.wind_speed = lerp(preset1.chopWater.wind_speed, preset2.chopWater.wind_speed, alpha);
  blended.amplitude_scale = lerp(preset1.chopWater.amplitude_scale, preset2.chopWater.amplitude_scale, alpha);
  blended.wind_spread = lerp(preset1.chopWater.wind_spread, preset2.chopWater.wind_spread, alpha);
  blended.opposing_damping = lerp(preset1.chopWater.opposing_damping, preset2.chopWater.opposing_damping, alpha);
  blended.opposing_ratio = lerp(preset1.chopWater.opposing_ratio, preset2.chopWater.opposing_ratio, alpha);
  blended.choppiness = lerp(preset1.chopWater.choppiness, preset2.chopWater.choppiness, alpha);

  blended.foam_tile_size = lerp(preset1.chopWater.foam_tile_size, preset2.chopWater.foam_tile_size, alpha);
  blended.foam_coverage = lerp(preset1.chopWater.foam_coverage, preset2.chopWater.foam_coverage, alpha);
  blended.foam_turbidity = lerp(preset1.chopWater.foam_turbidity, preset2.chopWater.foam_turbidity, alpha);
  blended.foam_sharpness = lerp(preset1.chopWater.foam_sharpness, preset2.chopWater.foam_sharpness, alpha);
  blended.foam_retention = lerp(preset1.chopWater.foam_retention, preset2.chopWater.foam_retention, alpha);
  blended.low_foam_turbidity = lerp(preset1.chopWater.low_foam_turbidity, preset2.chopWater.low_foam_turbidity, alpha);
  blended.low_foam_sharpness = lerp(preset1.chopWater.low_foam_sharpness, preset2.chopWater.low_foam_sharpness, alpha);
  blended.low_foam_coverage = lerp(preset1.chopWater.low_foam_coverage, preset2.chopWater.low_foam_coverage, alpha);
  blended.foam_dissolve_scale = lerp(preset1.chopWater.foam_dissolve_scale, preset2.chopWater.foam_dissolve_scale, alpha);
  blended.foam_detail_amp_mul = lerp(preset1.chopWater.foam_detail_amp_mul, preset2.chopWater.foam_detail_amp_mul, alpha);

  blended.micro_foam_h0 = lerp(preset1.chopWater.micro_foam_h0, preset2.chopWater.micro_foam_h0, alpha);
  blended.micro_foam_h1 = lerp(preset1.chopWater.micro_foam_h1, preset2.chopWater.micro_foam_h1, alpha);
  blended.micro_foam_h2 = lerp(preset1.chopWater.micro_foam_h2, preset2.chopWater.micro_foam_h2, alpha);
  blended.micro_foam_h3 = lerp(preset1.chopWater.micro_foam_h3, preset2.chopWater.micro_foam_h3, alpha);
  blended.micro_foam_h4 = lerp(preset1.chopWater.micro_foam_h4, preset2.chopWater.micro_foam_h4, alpha);
  blended.micro_foam_htex = lerp(preset1.chopWater.micro_foam_htex, preset2.chopWater.micro_foam_htex, alpha);
  blended.micro_foam_q_power = lerp(preset1.chopWater.micro_foam_q_power, preset2.chopWater.micro_foam_q_power, alpha);
  blended.micro_foam_q_sig_wave_mul =
    lerp(preset1.chopWater.micro_foam_q_sig_wave_mul, preset2.chopWater.micro_foam_q_sig_wave_mul, alpha);

  blended.detail_last_cascade_mul = lerp(preset1.chopWater.detail_last_cascade_mul, preset2.chopWater.detail_last_cascade_mul, alpha);
  blended.detail_last_cascade_nm = lerp(preset1.chopWater.detail_last_cascade_nm, preset2.chopWater.detail_last_cascade_nm, alpha);
  blended.detail_waves_mul = lerp(preset1.chopWater.detail_waves_mul, preset2.chopWater.detail_waves_mul, alpha);
  blended.detail_mask_strength = lerp(preset1.chopWater.detail_mask_strength, preset2.chopWater.detail_mask_strength, alpha);
  blended.detail_mask_bias = lerp(preset1.chopWater.detail_mask_bias, preset2.chopWater.detail_mask_bias, alpha);
  blended.domain_size = lerp(preset1.chopWater.domain_size, preset2.chopWater.domain_size, alpha);
  blended.detail_prev_last_cascade_mul =
    lerp(preset1.chopWater.detail_prev_last_cascade_mul, preset2.chopWater.detail_prev_last_cascade_mul, alpha);
  blended.scatter_intensity = lerp(preset1.chopWater.scatter_intensity, preset2.chopWater.scatter_intensity, alpha);
  blended.scatter_intensity_pow = lerp(preset1.chopWater.scatter_intensity_pow, preset2.chopWater.scatter_intensity_pow, alpha);
  blended.scatter_intensity_skylight_mul =
    lerp(preset1.chopWater.scatter_intensity_skylight_mul, preset2.chopWater.scatter_intensity_skylight_mul, alpha);
  preset.chopWater = blended;
  apply_wave_preset(water, preset, wind_dir);
}

void apply_wave_preset(FFTWater *water, const WavePresets &waves, float bf_scale, const Point2 &wind_dir)
{
  int preset1 = clamp<int>(bf_scale, 0, waves.presets.size() - 1);
  int preset2 = clamp<int>(bf_scale + 1, 0, waves.presets.size() - 1);
  float presetAlpha = bf_scale - preset1;

  apply_wave_preset(water, waves.presets[preset1], waves.presets[preset2], presetAlpha, wind_dir);
}

void apply_wave_preset(FFTWater *water, float bf_scale, const Point2 &wind_dir, Spectrum spectrum)
{
  apply_wave_preset(water, defWaves(spectrum), bf_scale, wind_dir);
}

void get_wave_preset(const FFTWater *water, WavePreset &out_preset)
{
  Point2 windDir;
  float cascadeR;
  out_preset.period = get_period(water);
  get_wind_speed(water, out_preset.wind, windDir);
  get_roughness(water, out_preset.roughness, cascadeR);
  out_preset.smallWaveFraction = get_small_wave_fraction(water);
  out_preset.simulation = get_simulation_params(water);
  out_preset.foam = get_foam(water);
  out_preset.chopWater = get_chop_water_props(water);
}

void set_wind(FFTWater *handle, float bf_scale, const Point2 &wind_dir)
{
  G_STATIC_ASSERT(BEAUFORT_SCALES_NUM == 7);
  const float BeaufortWindSpeed[BEAUFORT_SCALES_NUM] = {0.0f, 0.6f, 2.0f, 3.0f, 6.0f, 8.1f, 10.9};
  const float BeaufortWindSpeedChop[BEAUFORT_SCALES_NUM] = {0.0f, 1.0f, 2.0f, 3.5f, 4.0f, 6.0f, 7.0f};
  int speed = clamp<int>(bf_scale, 0, BEAUFORT_SCALES_NUM - 2);

  const float fftWindSpeed = lerp<float>(BeaufortWindSpeed[speed], BeaufortWindSpeed[speed + 1], saturate(bf_scale - speed));
  const float chopWindSpeed = lerp<float>(BeaufortWindSpeedChop[speed], BeaufortWindSpeedChop[speed + 1], saturate(bf_scale - speed));

  set_wind_speed(handle, fftWindSpeed, chopWindSpeed, wind_dir);
}
} // namespace fft_water