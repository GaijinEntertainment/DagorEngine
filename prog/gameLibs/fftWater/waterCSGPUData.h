#pragma once


#include <3d/dag_resPtr.h>
#include <generic/dag_staticTab.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_drv3dConsts.h>
#include <generic/dag_carray.h>

class NVWaveWorks_FFT_CPU_Simulation;
class ComputeShaderElement;
class BaseTexture;
typedef BaseTexture Texture;
#define ALL_CASCADES_ARE_SAME_SIZE 1

class CSGPUData
{
public:
  UniqueTex dispArray;
  GPUFENCEHANDLE asyncComputeFence, updateH0Fence;

  void close();
  bool init(const NVWaveWorks_FFT_CPU_Simulation *fft, int num);
  bool initDispArray(const NVWaveWorks_FFT_CPU_Simulation *fft, bool r_target);
  bool driverReset(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades);
  void perform(const NVWaveWorks_FFT_CPU_Simulation *fft, int num, double time);

  CSGPUData() :
    update_h0(0), fftH(0), fftV(0), asyncComputeFence(BAD_GPUFENCEHANDLE), updateH0Fence(BAD_GPUFENCEHANDLE), h0UpdateRequired(false)
  {}

  ~CSGPUData() { close(); }

private:
  void updateH0(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades);
  bool fillOmega(int ci, const NVWaveWorks_FFT_CPU_Simulation *fft);
  static constexpr int MAX_NUM_CASCADES = 5;
  struct CascadeData
  {
    UniqueBuf h0, ht, dt, omega;
  };

  inline CascadeData &getData(int idx, int num_cascades, int &groups_z);
  inline void setConstantBuffer(int idx, int groups_z);

  DECL_ALIGN16(float, constantBuffer[MAX_NUM_CASCADES][6 * 4]);

#if ALL_CASCADES_ARE_SAME_SIZE
  CascadeData data;
  struct Cascade //: public CascadeData
#else
  struct Cascade : public CascadeData
#endif
  {
    Cascade() : omegaPeriod(0) {}
    ~Cascade() { close(); }
    void close();
    float omegaPeriod;
  };

  StaticTab<Cascade, MAX_NUM_CASCADES> cascades;
  bool h0UpdateRequired;
  ComputeShaderElement *update_h0, *fftH, *fftV;
  UniqueBuf gauss;
};
