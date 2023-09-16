#include "waterCommon.h"
#include <math/dag_mathUtils.h>

#define USE_FACET_SIZE_FROM_PRESET 1

NVWaveWorks_FFT_CPU_Simulation::Params simulation_cascade_params(const NVWaveWorks_FFT_CPU_Simulation::Params &p,
  const fft_water::SimulationParams &simulation, int cascade_no, int cascade_num, float cascade_window_length,
  float cascade_facet_size)
{
#if USE_FACET_SIZE_FROM_PRESET
  cascade_facet_size = simulation.facetSize;
#endif
  // It is important to keep windowIn, windowOut (number of harmonics) same for each cascade!
  // so the constants below should be equal too
  // To keep consistency between players with the different quaility settings
  // and physics as well
  const int targetNumCascades = fft_water::DEFAULT_NUM_CASCADES;
  // Use a constant value for the base resolution so that periods and simulation results were not dependent too much
  // on a chosen quality for fft resolution
  const float fftResTarget = float(1 << TARGET_FFT_RESOLUTION);
  const float fftRes = fftResTarget;

  const bool is_least_detailed_cascade_level = cascade_no == 0;
  const bool is_most_detailed_cascade_level = cascade_no >= NUM_PHYS_CASCADES && cascade_no == (cascade_num - 1);
  // Compuate a divisor for the cascade period to keep a constant amount of details
  const float periodStart1 = p.fft_period;
  const float periodEnd1 = cascade_facet_size * fftResTarget;
  const float periodK1 = max(powf(safediv(periodStart1, periodEnd1), safediv(1.0f, targetNumCascades - 1)), 0.001f);
  const float periodStart2 = periodStart1 / powf(periodK1, fft_water::MIN_NUM_CASCADES - 2);
  const float periodEnd2 = cascade_facet_size * fftRes;
  const float periodK2 =
    max(powf(safediv(periodStart2, periodEnd2), 1.0f / max(cascade_num - fft_water::MIN_NUM_CASCADES + 1, 1)), 0.001f);
  const float periodK = cascade_no < fft_water::MIN_NUM_CASCADES - 1 ? periodK1 : periodK2;
  const float periodStart = cascade_no < fft_water::MIN_NUM_CASCADES - 1 ? periodStart1 : periodStart2;
  const int cascadeIndex = cascade_no < fft_water::MIN_NUM_CASCADES - 1 ? cascade_no : cascade_no - fft_water::MIN_NUM_CASCADES + 2;
  const float windowFFT = (cascade_no < fft_water::MIN_NUM_CASCADES - 1 ? fftResTarget : fftRes) * 0.5f * sqrtf(2.0f);
  const float windowFFTIn = (cascade_no < fft_water::MIN_NUM_CASCADES ? fftResTarget : fftRes) * 0.5f * sqrtf(2.0f);

  const float minWindowIn = (windowFFTIn * cascade_window_length) / periodK;
  const float maxWindowOut = windowFFT * cascade_window_length;

  NVWaveWorks_FFT_CPU_Simulation::Params params = p;
  params.fft_period = periodStart / powf(periodK, cascadeIndex);

  if (is_least_detailed_cascade_level)
    params.window_in = 0;
  else
    params.window_in = minWindowIn - 0.00013f; // to avoid integer numbers, affects tiling
  if (is_most_detailed_cascade_level)
    params.window_out = windowFFT;
  else
    params.window_out = maxWindowOut - 0.00013f; // to avoid integer numbers, affects tiling

  params.wave_amplitude = simulation.amplitude[cascade_no];
  params.choppy_scale = simulation.choppiness;
  params.wind_dependency = simulation.windDependency;
  params.wind_alignment = simulation.windAlignment;
  G_STATIC_ASSERT((int)fft_water::Spectrum::LAST == 1);
  params.spectrum = (int)simulation.spectrum;
  return params;
}

void set_cascade_params(NVWaveWorks_FFT_CPU_Simulation &fft, const NVWaveWorks_FFT_CPU_Simulation::Params &params, bool all_res)
{
  bool reallocate = false;
  fft.reinit(params, all_res, reallocate);
}
