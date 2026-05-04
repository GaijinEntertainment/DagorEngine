// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <util/dag_globDef.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_carray.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include "getDisplacement.h"
#include "waterCommon.h"
#include <fftWater/fftWater.h>
#include <fftWater/chopWaterGen.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <dag/dag_vector.h>

class ChopWaterPhysics : public cpujobs::IJob
{
protected:
  static constexpr int MAX_FIFO = 5;
  struct Cascade
  {
    carray<SmallTab<uint16_t, MidmemAlloc>, MAX_FIFO> fifo;
    carray<float, MAX_FIFO> cascadeMaxZ;
  };
  static constexpr int MAX_CASCADES = WATER_CPU_CASCADES_COUNT;
  dag::Vector<Cascade> cascades;
  const fft_water::WaterHeightmap *waterHeightmap;
  carray<float, MAX_FIFO> fifoMaxZ = {0};
  dag::AtomicFloat<float> allFifoMaxZ = 0.f;
  int numCascades;
  int allocatedCascades;

  double tickRate;
  volatile int lastTick;     // fifo is initialized from [lastTick - MAX_FIFO + 1; lastTick]
  volatile int updatingFifo; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that corresponding fifo is
                             // not valid
  volatile int asyncUpdatingTick; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that corresponding fifo
                                  // is not valid
  volatile int currentJobIndex, currentJobSize; // updatingFifo is either -1 (nothing updating) or [0; MAX_FIFO-1) - meaning that
                                                // corresponding fifo is not valid
  float seaLevel, maxSeaLevel, maxWaveHeight, significantWaveHeight;
  carray<float, MAX_CASCADES> maxWaveSize;
  int totalNCount;
  int max_num_successive_steps; // we limit ourselves on #of successive steps
  int max_num_binary_steps;     // we limit ourselves on #of binary search steps
  float renderGridAlign = 1.0f;
  Point2 renderGridOffset = ZERO<Point2>();

  int endUpdateRowsJobIdx, endFinalizeCascadesJobIdx, totalJobSize;

  ChopWaterGenerator &chopGen;

public:
  const char *getJobName(bool &) const override { return "water_phys_chop"; }
  virtual void doJob(); // from IJob

  void calcWaveHeight();
  void reset();
  bool validateNextTimeTick(double time);

  ChopWaterPhysics(ChopWaterGenerator &chop_gen, int num_cascades, const fft_water::WaterHeightmap *water_heightmap);

  ~ChopWaterPhysics();

  void syncRun(int nextLastTick);

  void runAsyncTask(int nextLastTick, float tasks);
  void setCascadesSimulationTime(double time);

  int updateSnapshotRows(int start, int count);
  int finalizeSnapshotCascade(int start, int count);

  int getDisplaceData(int destFifo, int start, int count);

  bool isAsyncRunning() const;
  void wait();
  void increaseTime(double time);

  // duplicate of WaterNVPhysics::getAbsFifoIndex
  void getAbsFifoIndex(double time, int &fifo1, int &fifo2, float &fifo2Part) const // lerp(fifo1, fifo2, fifo2Part);
  {
    double fifoT = time / tickRate, fifoFloored = floor(fifoT);
    fifo1 = fifoT;

    int currentLastTick = interlocked_relaxed_load(lastTick); // save locally, so there are no racing conditions
    int firstTick = currentLastTick - MAX_FIFO + 1;
    fifo2 = (fifo1 + 1);

    fifo2Part = fifoT - fifoFloored;
    if (fifo1 < firstTick) // -V1051
    {
      fifo2 = fifo1 = firstTick;
      fifo2Part = 0;
    }
    if (fifo1 >= currentLastTick)
    {
      fifo2 = fifo1 = currentLastTick;
      fifo2Part = 0.f;
    }
    int localUpdatingFifo = interlocked_relaxed_load(updatingFifo); // save locally, so there are no racing conditions
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
    if (cascades.empty() || cascade >= numCascades || fifo1 < 0 || fifo2 < 0)
      disp1 = disp2 = v_zero();
    else
      getDisplacementsBilinearChop(disp1, disp2, xz, fifo1, fifo2);
  }

  void getDisplacements(vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const
  {
    disp1 = disp2 = v_zero();
    if (cascades.empty() || fifo1 < 0 || fifo2 < 0)
      return;

    getDisplacementsBilinearChop(disp1, disp2, xz, fifo1, fifo2);
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
  int intersectSegment(double time, const Point3 &vStart, const Point3 &vEnd, float &fResult); // duplicate of
                                                                                               // WaterNVPhysics::intersectSegment
  int getHeightAboveWater(double time, const Point3 &in_point, float &result, Point3 *displacement = NULL,
    bool matchRenderGrid = false); // duplicate of WaterNVPhysics::getHeightAboveWater

protected:
  void getDisplacementsBilinearChop(vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const;

  int intersectRayWithOcean(double time, Point3 &result, float &T, const Point3 &position, const Point3 &direction,
    Point3 *out_displacement = nullptr, bool matchRenderGrid = false); // duplicate of WaterNVPhysics::intersectRayWithOcean
  void initializeCascades();
};
