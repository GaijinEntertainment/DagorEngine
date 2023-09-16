#pragma once

#include <FFT_CPU_Simulation.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_globDef.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include "getDisplacement.h"
#include "waterCommon.h"
#include <fftWater/fftWater.h>

class WaterNVPhysics : public cpujobs::IJob
{
protected:
  static constexpr int MAX_FIFO = 5;
  struct Cascade
  {
    NVWaveWorks_FFT_CPU_Simulation fft;
    carray<SmallTab<uint16_t, MidmemAlloc>, MAX_FIFO> fifo;
    carray<float, MAX_FIFO> cascadeMaxZ;
  };
  carray<NVWaveWorks_FFT_CPU_Simulation::Params, fft_water::MAX_NUM_CASCADES> fftParams;
  static constexpr int MAX_CASCADES = fft_water::MAX_NUM_CASCADES;
  Cascade *cascades;
  const fft_water::WaterHeightmap *waterHeightmap;
  carray<float, MAX_FIFO> fifoMaxZ = {0};
  float allFifoMaxZ = 0.f;
  fft_water::SimulationParams fftSimulationParams;
  int numCascades;
  float cascadeWindowLength;
  float cascadeFacetSize;
  double tickRate;
  volatile int lastTick;     // fifo is initialized from [lastTick - MAX_FIFO + 1; lastTick]
  volatile int updatingFifo; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that corresponding fifo is
                             // not valid
  volatile int asyncUpdatingTick; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that corresponding fifo
                                  // is not valid
  volatile int currentJobIndex, currentJobSize; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that
                                                // corresponding fifo is not valid
  float seaLevel, maxSeaLevel, maxWaveHeight, significantWaveHeight;
  Point2 shoreDamp;
  carray<float, MAX_CASCADES> maxWaveSize;
  carray<vec4f, MAX_CASCADES> fftScaleOfs;
  int totalNCount;
  int max_num_successive_steps; // we limit ourselves on #of successive steps
  int max_num_binary_steps;     // we limit ourselves on #of binary search steps
  bool noActualWaves;           // not enough waves
  bool forceActualWaves;
  float renderGridAlign = 1.0f;
  Point2 renderGridOffset = ZERO<Point2>();

  int endUpdateHtJobIdx, endFFTJobIdx, totalJobSize; //

  void setCascades(const NVWaveWorks_FFT_CPU_Simulation::Params &p);

public:
  virtual void doJob(); // from IJob

  void setSmallWaveFraction(float smallWaveFraction)
  {
    if (!cascades)
      return;
    for (int i = 0; i < numCascades; ++i)
      cascades[i].fft.setSmallWaveFraction(smallWaveFraction);
  }
  const fft_water::SimulationParams &getSimulationParams() { return fftSimulationParams; }
  void setSimulationParams(const fft_water::SimulationParams &simulation) { fftSimulationParams = simulation; }

  float getCascadeWindowLength() const { return cascadeWindowLength; }
  void setCascadeWindowLength(float value) { cascadeWindowLength = value; }

  float getCascadeFacetSize() const { return cascadeFacetSize; }
  void setCascadeFacetSize(float value) { cascadeFacetSize = value; }

  void reinit(const Point2 &wind_dir, float wind_speed, float period);
  void reset();
  bool validateNextTimeTick(double time);

  WaterNVPhysics(const NVWaveWorks_FFT_CPU_Simulation::Params &p, const fft_water::SimulationParams &simulation, int num_cascades,
    float cascade_window_length, float cascade_facet_size, const fft_water::WaterHeightmap *water_heightmap);


  ~WaterNVPhysics();

  void syncRun(int nextLastTick);

  void runAsyncTask(int nextLastTick, float tasks);
  void setCascadesSimulationTime(double time);
  void updateH0();
  int computeFFT(int start, int count);
  int updateHt(int start, int count);
  int getDisplaceData(int destFifo, int start, int count);

  bool isAsyncRunning() const;
  void increaseTime(double time);


  void getAbsFifoIndex(double time, int &fifo1, int &fifo2, float &fifo2Part) const // lerp(fifo1, fifo2, fifo2Part);
  {
    double fifoT = time / tickRate, fifoFloored = floor(fifoT);
    fifo1 = fifoT;

    int currentLastTick = lastTick; // save locally, so there are no racing conditions
    int firstTick = currentLastTick - MAX_FIFO + 1;
    fifo2 = (fifo1 + 1);

    fifo2Part = fifoT - fifoFloored;
    if (fifo1 < firstTick)
    {
      fifo2 = fifo1 = firstTick;
      fifo2Part = 0;
    }
    if (fifo1 >= currentLastTick)
    {
      fifo2 = fifo1 = currentLastTick;
      fifo2Part = 0.f;
    }
    int localUpdatingFifo = interlocked_acquire_load(updatingFifo); // save locally, so there are no racing conditions
    if (localUpdatingFifo == (fifo1 % MAX_FIFO))
    {
      fifo1 = fifo2;
      fifo2Part = 0.f;
    }

    if (localUpdatingFifo == (fifo1 + 1) % MAX_FIFO)
    {
      fifo2Part = 0.f;
      fifo2 = fifo1;
    }
  }
  void fifoIndexToLocal(int &fifo1, int &fifo2) const // lerp(fifo1, fifo2, fifo2Part);
  {
    fifo1 %= MAX_FIFO;
    fifo2 %= MAX_FIFO;
  }
  void getFifoIndex(double time, int &fifo1, int &fifo2, float &fifo2Part) const // lerp(fifo1, fifo2, fifo2Part);
  {
    getAbsFifoIndex(time, fifo1, fifo2, fifo2Part);
    fifoIndexToLocal(fifo1, fifo2);
  }

  void setRenderParams(float gridAlign, Point2 gridOffset)
  {
    renderGridAlign = gridAlign;
    renderGridOffset = gridOffset;
  }

  void setHeightmap(const fft_water::WaterHeightmap *water_heightmap) { waterHeightmap = water_heightmap; }

  vec4f getRenderedHeight(float x, float z);

  vec4f getHeightmapDataBilinear(float x, float z)
  {
    float result[4] = {0.0f, 0.0f, seaLevel, 1.0f};
    if (waterHeightmap)
      waterHeightmap->getHeightmapDataBilinear(x, z, result[2]);
    return v_ldu(result);
  }

  void getDisplacementsBilinear(int cascade, vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const
  {
    if (cascade >= numCascades)
      disp1 = disp2 = v_zero();
    else
    {
      const int bits = cascades[cascade].fft.getParams().fft_resolution_bits;
      DECL_ALIGN16(int, adr[4]);
      vec4f uvFrac = get_bilinear_adr(bits, cascades[cascade].fft.getParams().fft_period, xz, adr);
      disp1 = get_displacement_u16_bilinear(adr, uvFrac, cascades[cascade].fifo[fifo1].data(), fftScaleOfs[cascade]);
      disp2 = get_displacement_u16_bilinear(adr, uvFrac, cascades[cascade].fifo[fifo2].data(), fftScaleOfs[cascade]);
    }
  }

  void getDisplacementsNearest(int cascade, vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const
  {
    if (cascade >= numCascades)
      disp1 = disp2 = v_zero();
    else
    {
      const int bits = cascades[cascade].fft.getParams().fft_resolution_bits;
      uint32_t adr = get_nearest_adr(bits, cascades[cascade].fft.getParams().fft_period, xz);
      disp1 = get_displacement_u16_nearest(adr, cascades[cascade].fifo[fifo1].data(), fftScaleOfs[cascade]);
      disp2 = get_displacement_u16_nearest(adr, cascades[cascade].fifo[fifo2].data(), fftScaleOfs[cascade]);
    }
  }
  void getDisplacements(vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const
  {
    disp1 = disp2 = v_zero();
    for (int i = 0; i < numCascades; ++i)
    {
      const int bits = cascades[i].fft.getParams().fft_resolution_bits;
      DECL_ALIGN16(int, adr[4]);
      vec4f uvFrac = get_bilinear_adr(bits, cascades[i].fft.getParams().fft_period, xz, adr);
      disp1 = v_add(disp1, get_displacement_u16_bilinear(adr, uvFrac, cascades[i].fifo[fifo1].data(), fftScaleOfs[i]));
      disp2 = v_add(disp2, get_displacement_u16_bilinear(adr, uvFrac, cascades[i].fifo[fifo2].data(), fftScaleOfs[i]));
    }
  }
  vec3f getDisplacement(vec4f xz, int fifo1, int fifo2, float fifo2Part) const
  {
    vec3f disp1, disp2;
    getDisplacements(disp1, disp2, xz, fifo1, fifo2);
    return v_madd(v_sub(disp2, disp1), v_splats(fifo2Part), disp1);
  }

  void setLevel(float water_level)
  {
    seaLevel = water_level;
    maxSeaLevel = seaLevel + maxWaveHeight;
  }
  float getMaxWaveHeight() const { return maxWaveHeight; }
  float getSignificantWaveHeight() const { return significantWaveHeight; }
  int intersectSegment(double time, const Point3 &vStart, const Point3 &vEnd, float &fResult);
  int getHeightAboveWater(double time, const Point3 &in_point, float &result, Point3 *displacement = NULL,
    bool matchRenderGrid = false);
  void setForceActualWaves(bool enforce);

protected:
  void calcWaveHeight();
  int intersectRayWithOcean(double time, Point3 &result, float &T, const Point3 &position, const Point3 &direction,
    Point3 *out_displacement = nullptr, bool matchRenderGrid = false);
  void initializeCascades();
};
