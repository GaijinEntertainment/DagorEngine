#pragma once

#include <FFT_CPU_Simulation.h>
#include <fftWater/fftWater.h>

// FFT
const float kCascadeMinFacetTexels = 0.354f; // sqrt(2)/4, comes from 256/4, and considering that the fft area is limited by
                                             // the circe and it is a multiplier we get (256/4) / (256/2 * sqrt(2))
const float kCascadeFacetSize = 0.05f;

enum
{
  NUM_PHYS_CASCADES = 2,
  DEF_PHYS_FFT_RESOLUTION = 7,

  // If physics are important then render fft resolution should not be less than DEF_PHYS_FFT_RESOLUTION
  // Use lesser values only for visual water without physics or with very small waves (almost flat surface)
  // According to the recent tests and due to the reworked cascade alignemnt method, the fft render 6 is quite enough to be
  // in compilance with the fft phys 7 so the error remains small enough
  MIN_FFT_RESOLUTION = 6,
  MAX_FFT_RESOLUTION = MAX_FFT_RESOLUTION_GAUSS,
  TARGET_FFT_RESOLUTION = 8
};

NVWaveWorks_FFT_CPU_Simulation::Params simulation_cascade_params(const NVWaveWorks_FFT_CPU_Simulation::Params &p,
  const fft_water::SimulationParams &simulation, int cascade_no, int cascade_num, float cascade_window_length,
  float cascade_facet_size);
void set_cascade_params(NVWaveWorks_FFT_CPU_Simulation &fft, const NVWaveWorks_FFT_CPU_Simulation::Params &params, bool all_res);
