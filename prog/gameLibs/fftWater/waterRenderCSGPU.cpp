#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <3d/dag_lockSbuffer.h>
#include <perfMon/dag_statDrv.h>
#include <FFT_CPU_Simulation.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_computeShaders.h>
#include <math/dag_adjpow2.h>
// #include <math/dag_Point4.h>
// #include <math/dag_Point3.h>
// #include <math/dag_Point2.h>
#include <math/dag_hlsl_floatx.h>
#include "waterCSGPUData.h"

#if _TARGET_ANDROID
#define H0_COMPRESSION  0
#define FFT_COMPRESSION 0
#endif

#include "shaders/fft_water_defs.hlsli"
#if CS_MAX_FFT_RESOLUTION > (1 << MAX_FFT_RESOLUTION_GAUSS)
#error CS_MAX_FFT_RESOLUTION should not be bigger than gauss MAX_FFT_RESOLUTION
#endif
struct ConstantBuffer
{
  typedef uint32_t uint;
#include "shaders/fft_water_cbuffer.hlsli"
};

static void initConstantBuffer(ConstantBuffer &constant_buffer, const NVWaveWorks_FFT_CPU_Simulation::Params &m_params, int i);

void CSGPUData::Cascade::close()
{
  G_STATIC_ASSERT(sizeof(ConstantBuffer) == sizeof(CSGPUData::constantBuffer[0]));
#if !ALL_CASCADES_ARE_SAME_SIZE
  CascadeData::close();
#endif
};

void CSGPUData::close()
{
  del_it(update_h0);
  del_it(fftH);
  del_it(fftV);
  dispArray.close();
  gauss.close();
  for (int i = 0; i < cascades.size(); ++i)
  {
    cascades[i].close();
  }
  cascades.clear();
  asyncComputeFence = updateH0Fence = BAD_GPUFENCEHANDLE;
}

bool CSGPUData::init(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades)
{
  close();

  {
    auto driverDesc = d3d::get_driver_desc();
    if (driverDesc.shaderModel < 5.0_sm)
      return false;
  }

  h0UpdateRequired = true;

  static int csFFTResolutionVarId = ::get_shader_glob_var_id("cs_fft_resolution", true);
  ShaderGlobal::set_int(csFFTResolutionVarId, fft[0].getParams().fft_resolution_bits);

  const int N0 = 1 << fft[0].getParams().fft_resolution_bits;

#ifdef _TARGET_ANDROID
  update_h0 = new_compute_shader("update_h0_cs_android", true);
  fftH = new_compute_shader("fftH_cs_android", true);
  fftV = new_compute_shader("fftV_cs_android", true);
#else
  update_h0 = new_compute_shader("update_h0_cs", true);
  fftH = new_compute_shader("fftH_cs", true);
  fftV = new_compute_shader("fftV_cs", true);
#endif

  if (!update_h0 || !fftH || !fftV)
  {
    debug("fftwater: no compute shaders");
    close();
    return false;
  }

  cascades.resize(numCascades);

  if (!initDispArray(fft, false))
  {
    close();
    return false;
  }

  const int gauss_size = N0 * N0;
  gauss = dag::buffers::create_persistent_sr_structured(sizeof(float2), gauss_size, "fft_gauss");
  if (!gauss)
  {
    debug("fftwater: can't create gauss buffer");
    close();
    return false;
  }

  for (int i = 0; i < (ALL_CASCADES_ARE_SAME_SIZE ? 1 : cascades.size()); ++i)
  {
    const int N = 1 << fft[i].getParams().fft_resolution_bits;
    G_ASSERTF(N0 == N, "due to common gauss for all cascades cascades should have sizes (at least physics ones) %d(%d)!=%d", N, i,
      N0); // due to common gauss/omega for all cascades. can be changed
    const int h0_size = (N + 1) * (N + 1);
    const int htdt_size = (N / 2 + 1) * N;
    const int omega_size = (N / 2 + 1) * (N / 2 + 1);

    int data_count;
    CascadeData &cdata = getData(i, cascades.size(), data_count);

    cdata.h0 = dag::buffers::create_ua_sr_structured(sizeof(h0_type), data_count * h0_size, "fft_h0");
    cdata.ht = dag::buffers::create_ua_sr_structured(sizeof(ht_type), data_count * htdt_size, "fft_ht");
    cdata.dt = dag::buffers::create_ua_sr_structured(sizeof(dt_type), data_count * htdt_size, "fft_dt");
    cdata.omega = dag::buffers::create_persistent_sr_structured(sizeof(float), data_count * omega_size, "fft_omega");
    if (!cdata.h0 || !cdata.ht || !cdata.dt || !cdata.omega)
    {
      debug("fftwater: can't create sbuffers");
      close();
      return false;
    }
  }
  if (!driverReset(fft, numCascades))
  {
    debug("fftwater: can't fill sbuffers");
    close();
    return false;
  }
  return true;
}

bool CSGPUData::initDispArray(const NVWaveWorks_FFT_CPU_Simulation *fft, bool r_target)
{
  TextureInfo ti;
  if (dispArray && dispArray.getArrayTex()->getinfo(ti) && (r_target == (ti.cflg & TEXCF_RTARGET ? true : false)))
    return true;

  const int N0 = 1 << fft[0].getParams().fft_resolution_bits;
  int numCascades = cascades.size();

  dispArray.close();
  dispArray = dag::create_array_tex(N0, N0, numCascades,
    TEXCF_UNORDERED | TEXFMT_A16B16G16R16F | (r_target ? TEXCF_RTARGET : 0) // for enabling cascades separately in debug mode
    ,
    1, "water3d_disp_cs_array");
  if (!dispArray)
    return false;

  G_ASSERT(dispArray);
  return true;
}

static inline float sqr(float a) { return a * a; }
static void initConstantBuffer(ConstantBuffer &constant_buffer, const NVWaveWorks_FFT_CPU_Simulation::Params &m_params, int arrayId)
{
  G_STATIC_ASSERT(sizeof(ConstantBuffer) < 128); // make sure allocated buffer is big enough

  const float twoPi = 6.283185307179586f;
  const float gravity = 9.810f;
  const float sqrtHalf = 0.707106781186f;

  const int m_resolution = 1 << m_params.fft_resolution_bits;

  // kStep = 2PI/L; norm = kStep * sqrt(1/2)
  float philNorm = 2 * PI / m_params.fft_period;

  constant_buffer.m_resolution = m_resolution;
  constant_buffer.m_resolution_bits = m_params.fft_resolution_bits;
  constant_buffer.m_resolution_bits_1 = m_params.fft_resolution_bits - 1;
  constant_buffer.m_resolution_plus_one = m_resolution + 1;
  constant_buffer.m_half_resolution = m_resolution / 2;
  constant_buffer.m_half_resolution_plus_one = m_resolution / 2 + 1;
  constant_buffer.m_resolution_plus_one_squared_minus_one = sqr(m_resolution + 1) - 1;

  constant_buffer.m_32_minus_log2_resolution = 32 - m_params.fft_resolution_bits;
  constant_buffer.m_window_in = m_params.window_in;
  constant_buffer.m_window_out = m_params.window_out;
  constant_buffer.m_wind_dir.x = m_params.wind_dir_x;
  constant_buffer.m_wind_dir.y = m_params.wind_dir_y;
  constant_buffer.m_frequency_scale = twoPi / m_params.fft_period;
  constant_buffer.m_linear_scale = philNorm * sqrtHalf * m_params.wave_amplitude;
  constant_buffer.m_wind_scale = m_params.wind_dependency;
  constant_buffer.m_wind_align = m_params.wind_alignment;
  constant_buffer.m_root_scale = m_params.wind_speed;
  constant_buffer.m_power_scale = m_params.small_wave_fraction;
  constant_buffer.m_choppy_scale = m_params.choppy_scale;
  constant_buffer.arrayId = arrayId; // i dont like it. it should be uav (render target)
}

static void updateConstantBuffer(ConstantBuffer &constant_buffer, double simTime) { constant_buffer.m_time = simTime; }

CSGPUData::CascadeData &CSGPUData::getData(int idx, int num_cascades, int &groups_z)
{
#if ALL_CASCADES_ARE_SAME_SIZE
  (void)idx;
  groups_z = num_cascades;
  CascadeData &cdata = data;
#else
  (void)num_cascades;
  groups_z = 1;
  CascadeData &cdata = cascades[idx];
#endif
  return cdata;
}

void CSGPUData::setConstantBuffer(int idx, int groups_z)
{
  if (idx >= 0)
    d3d::set_cb0_data(STAGE_CS, constantBuffer[idx], groups_z * sizeof(constantBuffer[0]) / 16);
  else
    d3d::release_cb0_data(STAGE_CS);
}

bool CSGPUData::fillOmega(int numCascades, const NVWaveWorks_FFT_CPU_Simulation *fft)
{
  float *omegaDest;
#if ALL_CASCADES_ARE_SAME_SIZE
  if (data.omega->lock(0, 0, (void **)&omegaDest, VBLOCK_WRITEONLY))
#endif
  {
    for (int ci = 0; ci < numCascades; ++ci)
    {
#if !ALL_CASCADES_ARE_SAME_SIZE
      if (!cascades[ci].omega->lock(0, 0, (void **)&omegaDest, VBLOCK_WRITEONLY))
        return false;
#endif
      const int m_resolution = 1 << fft[ci].getParams().fft_resolution_bits;
      const int m_half_resolution_plus_one = m_resolution / 2 + 1;

      for (int i = 0; i < m_half_resolution_plus_one; ++i)
        memcpy(omegaDest + i * m_half_resolution_plus_one, fft[ci].getOmegaData(i), m_half_resolution_plus_one * sizeof(float));
      omegaDest += m_half_resolution_plus_one * m_half_resolution_plus_one;
#if !ALL_CASCADES_ARE_SAME_SIZE
      cascades[ci].omega->unlock();
#endif
    }
#if ALL_CASCADES_ARE_SAME_SIZE
    data.omega->unlock();
#endif
    return true;
  }
  return false;
}

bool CSGPUData::driverReset(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades)
{
  bool ret = true;
  for (int ci = 0; ci < numCascades; ++ci)
  {
    initConstantBuffer(*(ConstantBuffer *)constantBuffer[ci], fft[ci].getParams(), ci);
    ((ConstantBuffer *)constantBuffer[ci])->m_time = 0;
  }
  // fill gauss, omega
  const int m_resolution = 1 << fft[0].getParams().fft_resolution_bits;
  const int m_half_resolution_plus_one = m_resolution / 2 + 1;

  int gauss_resolution, gauss_stride;
  const cpu_types::float2 *in_gauss = get_global_gauss_data(gauss_resolution, gauss_stride);

  // copy actually used gauss window around center of max resolution buffer
  // note that we need to generate full resolution to maintain pseudo-randomness
  if (auto gaussData = lock_sbuffer<cpu_types::float2>(gauss.getBuf(), 0, m_resolution * m_resolution, VBLOCK_WRITEONLY))
  {
    const cpu_types::float2 *gauss_src = in_gauss + (gauss_resolution - m_resolution) / 2 * (1 + gauss_stride);
    for (int i = 0; i < m_resolution; ++i)
      gaussData.updateDataRange(i * m_resolution, gauss_src + i * gauss_stride, m_resolution);
  }
  else
  {
    G_ASSERTF(0, "gauss buffer %d not locked");
    logerr("gauss buffer %d not locked");
    ret = false;
  }

  if (ret)
  {
    if (fillOmega(numCascades, fft))
    {
      for (int ci = 0; ci < numCascades; ++ci)
        cascades[ci].omegaPeriod = fft[ci].getParams().fft_period;
    }
    else
    {
      G_ASSERTF(0, "omega buffer %d not locked", 0);
      logerr("omega buffer %d not locked", 0);
      for (int ci = 0; ci < numCascades; ++ci)
        cascades[ci].omegaPeriod = 0;
      ret = false;
    }
  }

  h0UpdateRequired = true;

  // updateH0 uses the freshly filled gauss buffer as input on async compute pipeline. Async pipeline needs to
  // wait for gfx pipeline in this case, probably because GenericBuffer::unlock() uses CopyResource on the gfx
  // context.
  updateH0Fence = d3d::insert_fence(GpuPipeline::GRAPHICS);

  return ret;
}

void CSGPUData::updateH0(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades)
{
  if (!h0UpdateRequired)
    return;
  if (!update_h0)
    return;
  for (int i = 0; i < numCascades; ++i)
    initConstantBuffer(*(ConstantBuffer *)constantBuffer[i], fft[i].getParams(), i);

  d3d::set_buffer(STAGE_CS, 0, gauss.getBuf());

  d3d::insert_wait_on_fence(updateH0Fence, GpuPipeline::ASYNC_COMPUTE);

  for (int i = 0; i < (ALL_CASCADES_ARE_SAME_SIZE ? 1 : cascades.size()); ++i)
  {
    int groupsZ;
    CascadeData &cdata = getData(i, numCascades, groupsZ);
    d3d::set_rwbuffer(STAGE_CS, 0, cdata.h0.getBuf());
    setConstantBuffer(i, groupsZ);
    const int N = 1 << fft[i].getParams().fft_resolution_bits;
    update_h0->dispatch(1, N, groupsZ, GpuPipeline::ASYNC_COMPUTE);
  }

  setConstantBuffer(-1, 0);
  d3d::set_rwbuffer(STAGE_CS, 0, 0);

  h0UpdateRequired = false;
}

void CSGPUData::perform(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades, double time)
{
  if (!update_h0)
    return;
  G_ASSERT(numCascades <= cascades.size());
  numCascades = min<int>(numCascades, cascades.size());

  TIME_D3D_PROFILE(csGPUFFT);
  updateH0(fft, numCascades);
  for (int i = 0; i < cascades.size(); ++i)
  {
    updateConstantBuffer(*(ConstantBuffer *)constantBuffer[i], time);
  }

  for (int i = 0; i < (ALL_CASCADES_ARE_SAME_SIZE ? 1 : cascades.size()); ++i)
  {
    int groupsZ;
    CascadeData &cdata = getData(i, numCascades, groupsZ);
    d3d::resource_barrier({cdata.h0.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::set_buffer(STAGE_CS, 0, cdata.h0.getBuf());
    d3d::set_buffer(STAGE_CS, 1, cdata.omega.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 0, cdata.ht.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 1, cdata.dt.getBuf());
    setConstantBuffer(i, groupsZ);
    const int N = 1 << fft[i].getParams().fft_resolution_bits;
    fftH->dispatch(1, N / 2 + 1, groupsZ, GpuPipeline::ASYNC_COMPUTE);
  }
  d3d::set_rwbuffer(STAGE_CS, 1, NULL);
  d3d::set_rwtex(STAGE_CS, 0, dispArray.getBaseTex(), 0, 0);

  for (int i = 0; i < (ALL_CASCADES_ARE_SAME_SIZE ? 1 : cascades.size()); ++i)
  {
    int groupsZ;
    CascadeData &cdata = getData(i, numCascades, groupsZ);
    d3d::resource_barrier({cdata.ht.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::resource_barrier({cdata.dt.getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
    d3d::set_buffer(STAGE_CS, 0, cdata.ht.getBuf());
    d3d::set_buffer(STAGE_CS, 1, cdata.dt.getBuf());
    setConstantBuffer(i, groupsZ);
    const int N = 1 << fft[i].getParams().fft_resolution_bits;
    fftV->dispatch(1, N, groupsZ, GpuPipeline::ASYNC_COMPUTE);
  }

  d3d::resource_barrier({dispArray.getBaseTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_VERTEX, 0, 0});
  asyncComputeFence = d3d::insert_fence(GpuPipeline::ASYNC_COMPUTE);

  setConstantBuffer(-1, 0);
  d3d::set_rwtex(STAGE_CS, 0, NULL, 0, 0);
  d3d::set_buffer(STAGE_CS, 0, 0);
  d3d::set_buffer(STAGE_CS, 1, 0);
}
