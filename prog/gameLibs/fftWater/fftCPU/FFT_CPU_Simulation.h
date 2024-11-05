// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define MAX_FFT_RESOLUTION_GAUSS 9 // for gauss

typedef float complex[2];

void init_nv_wave_works();  // global init
void close_nv_wave_works(); // global close

namespace cpu_types
{

typedef struct
{
  float x;
  float y;
} float2;

typedef struct
{
  float x;
  float y;
  float z;
  float w;
} float4;

typedef struct
{
  unsigned short x;
  unsigned short y;
  unsigned short z;
  unsigned short w;
} half4;

}; // namespace cpu_types

static const int SPECTRUM_PHILLIPS = 0;
static const int SPECTRUM_UNIFIED_DIRECTIONAL = 1;

class NVWaveWorks_FFT_CPU_Simulation
{
public:
  struct Params
  {
    int fft_resolution_bits;
    float fft_period;
    float wind_dir_x, wind_dir_y; // normalized
    float wave_amplitude;
    float wind_speed;
    float wind_dependency;
    float wind_alignment;
    float small_wave_fraction;
    float choppy_scale;
    int spectrum;

    // Window params for setting up this cascade's spectrum, measured in pixels from DC
    float window_in;
    float window_out;

    Params() :
      fft_resolution_bits(0),
      fft_period(0),
      wind_dir_x(0),
      wind_dir_y(0),
      wave_amplitude(0),
      wind_speed(0),
      wind_dependency(0),
      wind_alignment(0),
      small_wave_fraction(0),
      choppy_scale(0),
      window_in(0),
      window_out(0),
      spectrum(SPECTRUM_UNIFIED_DIRECTIONAL)
    {}
    bool operator!=(const Params &params)
    {
      return wave_amplitude != params.wave_amplitude || wind_speed != params.wind_speed || wind_dir_x != params.wind_dir_x ||
             wind_dir_y != params.wind_dir_y || wind_dependency != params.wind_dependency || wind_alignment != params.wind_alignment ||
             small_wave_fraction != params.small_wave_fraction || window_in != params.window_in || window_out != params.window_out;
    }
    bool operator==(const Params &p) { return !(*this != p); }
  };

  NVWaveWorks_FFT_CPU_Simulation();
  void setParams(const NVWaveWorks_FFT_CPU_Simulation::Params &params) { m_params = params; }
  const NVWaveWorks_FFT_CPU_Simulation::Params &getParams() const { return m_params; }
  void setSmallWaveFraction(float smallWaveFraction)
  {
    m_H0UpdateRequired = true;
    m_params.small_wave_fraction = smallWaveFraction;
  }

  ~NVWaveWorks_FFT_CPU_Simulation();

  // Simulation primitives
  void UpdateH0(int row);
  void UpdateHt(int row);
  void UpdateHtC(int row);
  void ComputeFFT(int index);
  void getHalfData(int row, cpu_types::half4 *compressed);
  void getFloatData(int row, cpu_types::float4 *uncompressed);
  // unaligned data, 3*short. store as (data-minHt)/maxScaleHt
  // compressed data should be of a size N*N*3+1(!)
  void getUint16Data(int row, float minHt, float maxScaleHt, unsigned short *compressed, float &out_max_z);

  // Mandatory NVWaveWorks_FFT_Simulation interface
  bool init();
  bool reinit(const NVWaveWorks_FFT_CPU_Simulation::Params &params, bool all_res, bool &reallocate);

  bool IsH0UpdateRequired() const { return m_H0UpdateRequired; }
  void SetH0UpdateNotRequired() { m_H0UpdateRequired = false; }

  void setTime(double dSimTime) { m_doubletime = dSimTime; }


  bool allocateAllResources();
  void releaseAllResources();
  const complex *getHtData(int row = 0) const { return m_fftCPU_io_buffer + (row << m_params.fft_resolution_bits); }
  const cpu_types::float2 *getH0Data(int row = 0) const { return m_h0_data + row * ((1 << m_params.fft_resolution_bits) + 2); }
  const float *getOmegaData(int row = 0) const { return m_omega_data + (row << m_params.fft_resolution_bits); }
  bool initGaussAndOmega();

private:
  Params m_params;

  float *m_omega_data;
  cpu_types::float2 *m_h0_data;
  float *m_sqrt_table; // pre-computed coefficient for speed-up computation of update spectrum

  // in-out buffer for FFTCPU, it holds 3 FFT images sequentially
  complex *m_fftCPU_io_buffer;


  double m_doubletime;

  bool m_H0UpdateRequired;
};

float get_spectrum_rms_sqr(const NVWaveWorks_FFT_CPU_Simulation::Params &params);
void calc_wave_height(const NVWaveWorks_FFT_CPU_Simulation::Params *fft, int num_cascades, float &out_significant_wave_height,
  float &out_max_wave_height, float *out_max_wave_size);
const cpu_types::float2 *get_global_gauss_data(int &gauss_resolution, int &stride);
