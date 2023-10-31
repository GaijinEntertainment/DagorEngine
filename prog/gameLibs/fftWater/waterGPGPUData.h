#pragma once

#include <3d/dag_resPtr.h>
#include <generic/dag_carray.h>
#include <shaders/dag_postFxRenderer.h>

class NVWaveWorks_FFT_CPU_Simulation;
class ShaderMaterial;
class ShaderElement;
class BaseTexture;
typedef BaseTexture Texture;

class GPGPUData
{
public:
  static constexpr int MAX_NUM_CASCADES = 5;
  carray<UniqueTex, MAX_NUM_CASCADES> dispGPU;
  UniqueTex dispArray;
  void reset();
  void close();
  bool init(const NVWaveWorks_FFT_CPU_Simulation *fft, int num);
  void updateHt0WindowsVB(const NVWaveWorks_FFT_CPU_Simulation *fft, int num);
  void perform(const NVWaveWorks_FFT_CPU_Simulation *fft, int num, double time);

  GPGPUData() { reset(); }
  ~GPGPUData() { close(); }

private:
  void updateH0(const NVWaveWorks_FFT_CPU_Simulation *fft, int num);
  bool fillOmega(const NVWaveWorks_FFT_CPU_Simulation *fft, int num);
  void fillBuffers(const NVWaveWorks_FFT_CPU_Simulation *fft, int numCascades);

  UniqueTex butterfly;

  // this 3 is not magical constant. We need to process N cascades with 2complex numbers *3 (displacements) = 6 channels.
  // So, 6 is a bad number, cause for MRT we will need 2 cascades with different MRT (one - with 4 components, one - with 2
  // components), and that mean different bits. Although all modern HW support different bits per target, it still mean unoptimal path
  // (and there is old HW). Luckily, 6*2 is 12, which is 3 targets with 4 components. So, for any even number of cascades (2,4,6) - we
  // get optimal codepath by creating 3 textures of (fft_res*cascadesNo/2, fft_res) resolution. For odd number of cascades, it become
  // suboptimal, though. Can be specially optimized in shader
  UniqueTex htPing[3], htPong[3];

  UniqueTex ht0;
  UniqueTex gauss, omega;
  PostFxRenderer displacementsRenderer, htRenderer; // gpGPU
  ShaderMaterial *h0Mat;
  ShaderElement *h0Element;
  Vbuffer *ht0Vbuf;
  Ibuffer *ht0Ibuf;
  ShaderMaterial *fftVMat, *fftHMat;
  ShaderElement *fftHElement, *fftVElement;
  Vbuffer *fftVbuf;
  bool h0GPUUpdateRequired = true, omegaGPUUpdateRequired = true;
  bool buffersReady = false;
};
