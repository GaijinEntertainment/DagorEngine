// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "chopWaterPhysics.h"
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_threadPool.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_adjpow2.h>
#include <math/dag_hlsl_floatx.h>
#include <fftWater/chop_water_distort_uv.hlsli>
#include <math/integer/dag_IPoint3.h>
#include <FFT_CPU_Simulation.h>

namespace convar
{
extern ConVarT<float, false> uv_distort_strength;
}

ChopWaterPhysics::~ChopWaterPhysics()
{
  threadpool::wait(this);
  cascades.clear();
  allocatedCascades = 0;
}

void ChopWaterPhysics::reset()
{
  threadpool::wait(this);

  lastTick = 0;
  updatingFifo = -1;

  currentJobIndex = -1;
  currentJobSize = 0;
  if (!cascades.empty())
  {
    for (int i = 0; i < MAX_FIFO; ++i)
      syncRun(i);
  }
}

bool ChopWaterPhysics::validateNextTimeTick(double time)
{
  int currentTick = ceil(time / tickRate);
  return currentTick > lastTick - MAX_FIFO;
}

void ChopWaterPhysics::initializeCascades()
{
  threadpool::wait(this);

  mem_set_0(fifoMaxZ);
  allFifoMaxZ.store(0);

  bool needRecreate = (cascades.empty() || allocatedCascades != numCascades);
  if (needRecreate)
  {
    cascades.clear();
    cascades.resize(numCascades);
    allocatedCascades = numCascades;
  }

  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    if (needRecreate)
    {
      for (int i = 0; i < cascades[cascadeNo].fifo.size(); ++i)
      {
        clear_and_resize(cascades[cascadeNo].fifo[i], WATER_CPU_HEIGHTMAP_RES * WATER_CPU_HEIGHTMAP_RES * 3 + 1);
        mem_set_0(cascades[cascadeNo].fifo[i]);
      }
      mem_set_0(cascades[cascadeNo].cascadeMaxZ);
    }
  }
}

ChopWaterPhysics::ChopWaterPhysics(ChopWaterGenerator &chop_gen, int num_cascades, const fft_water::WaterHeightmap *water_heightmap) :
  chopGen(chop_gen), waterHeightmap(water_heightmap), allocatedCascades(0)
{
  currentJobIndex = -1;
  currentJobSize = 0;
  numCascades = min(num_cascades, (int)fft_water::MAX_NUM_CASCADES);
  seaLevel = maxSeaLevel = 0.f;
  // tickRate = 1/3.0;//three times a second is more than enough
  tickRate = 0.375; // three times a second is more than enough, use 0.375
  totalNCount = num_cascades * WATER_CPU_HEIGHTMAP_RES;
  debug("totalNCount = %d", totalNCount);
  lastTick = 0;
  updatingFifo = -1;
  endUpdateRowsJobIdx = totalNCount;
  endFinalizeCascadesJobIdx = endUpdateRowsJobIdx + totalNCount * 3;
  totalJobSize = endFinalizeCascadesJobIdx + totalNCount;
  calcWaveHeight();
  reset(); //-V1053
}

bool ChopWaterPhysics::isAsyncRunning() const { return interlocked_acquire_load(this->done) == 0; }

void ChopWaterPhysics::wait() { threadpool::wait(this); }

void ChopWaterPhysics::increaseTime(double time)
{
  double nextTickF = time / tickRate;
  int currentTick = ceil(nextTickF); // this one is already potentially required for interpolation!

  // call reset for all physics if we are behind
  if (currentTick <= lastTick - MAX_FIFO)
  {
    logwarn("we are behind in nv water! current tick %d, last tick %d", currentTick, lastTick);
    reset();
    return;
  }

  if (cascades.empty())
  {
    lastTick = currentTick;
    return;
  }

  int nextTick = currentTick + 1; // future tick

  if (isAsyncRunning())
  {
    if (lastTick < currentTick)
    {
      logwarn("we had to wait for async task");
      threadpool::wait(this);
    }
    else
      return;
  }
  if (lastTick < currentTick - MAX_FIFO)
  {
    logwarn("we were at tick %d, and we need to %d", lastTick, currentTick);
    lastTick = currentTick - MAX_FIFO; // skip useless ticks
  }

  if (lastTick == currentTick - 1 && asyncUpdatingTick == currentTick && currentJobIndex >= 0)
  {
    // finalize unfinished task
    currentJobSize = totalJobSize - currentJobIndex;
    doJob();
  }

  while (lastTick < currentTick) // something went wrong, we step too far ahead!
    syncRun(lastTick + 1);

  if (nextTick <= lastTick)
    return;
  // todo: since we will run a task, there should be clear limitation, that only when simulation is finished, we can run new one
  G_ASSERTF(nextTick == lastTick + 1, "nextTick = %d lastTick=%d", nextTick, lastTick);

  double time_left = 2.0 - (nextTick - nextTickF); // from 0 to 1
  runAsyncTask(nextTick, time_left);
}

void ChopWaterPhysics::setCascadesSimulationTime(double time)
{
  if (cascades.empty())
    return;

  chopGen.begin_cpu_snapshot(time);
}

void ChopWaterPhysics::syncRun(int nextLastTick)
{
  threadpool::wait(this);
  currentJobIndex = -1;
  currentJobSize = 0;

  double futureTime = nextLastTick * tickRate;
  setCascadesSimulationTime(futureTime);
  asyncUpdatingTick = nextLastTick;
  currentJobIndex = 0;
  currentJobSize = totalJobSize;
  int64_t reft = ref_time_ticks();
  while (currentJobIndex >= 0)
  {
    int prevIdx = currentJobIndex;
    doJob();
    // if no progress happened (e.g. finalizeSnapshotCascade returned 0), force advance to next phase
    if (currentJobIndex == prevIdx)
    {
      if (currentJobIndex < endUpdateRowsJobIdx)
        currentJobIndex = endUpdateRowsJobIdx;
      else if (currentJobIndex < endFinalizeCascadesJobIdx)
        currentJobIndex = endFinalizeCascadesJobIdx;
      else
        break;
      currentJobSize = totalJobSize - currentJobIndex;
    }
  }
  // debug("sync to %d in %dus", nextLastTick, get_time_usec(reft));

  // single-threaded debug below (comment upper doJob, doJob and runAsyncTask in increaseTime)
  /*int64_t reft = ref_time_ticks();
  updateSnapshotRows(0, totalNCount);
  finalizeSnapshotCascade(0, 3*totalNCount);

  //starting from this moment, old firstTick is invalid
  int destFifo = nextLastTick%MAX_FIFO;

  getDisplaceData(destFifo, 0, totalNCount);
  int totalTex = get_time_usec(reft);

  lastTick = nextLastTick;//now, last tick is officially nextLastTick
  debug("sync to %d(%d) in %dus", nextLastTick, destFifo, get_time_usec(reft));*/
}

int ChopWaterPhysics::updateSnapshotRows(int start, int count)
{
  const int totalRows = totalNCount;
  int haveRows = chopGen.get_cpu_snapshot_progress_rows();
  int remainingRows = max(0, totalRows - haveRows);
  if (remainingRows <= 0 || count <= 0)
    return 0;

  int rowsBudget = min(max(1, count), remainingRows);
  int rowsProcessed = chopGen.update_cpu_snapshot_rows(rowsBudget);
  return rowsProcessed;
}

int ChopWaterPhysics::finalizeSnapshotCascade(int start, int count)
{
  // One job id per cascade for finalize_snapshot
  if (start >= WATER_CPU_CASCADES_COUNT || count <= 0)
    return 0;
  int cascadeIndex = clamp(start, 0, WATER_CPU_CASCADES_COUNT - 1);
  int destFifo = asyncUpdatingTick % MAX_FIFO;
  chopGen.finalize_snapshot_cascade(cascadeIndex, destFifo);
  return 1;
}

int ChopWaterPhysics::getDisplaceData(int destFifo, int start, int count)
{
  if (cascades.empty())
    return 0;
  if (start == 0)
    for (int i = 0; i < numCascades; ++i)
      cascades[i].cascadeMaxZ[destFifo] = 0.f;

  int totalComputed = 0;
  const int N = WATER_CPU_HEIGHTMAP_RES;
  const int domainsUsed = min(numCascades, (int)WATER_CPU_CASCADES_COUNT);
  const int totalRowsPerStage = domainsUsed * N;
  float allFifoMaxZLocal = allFifoMaxZ.load();

  SmallTab<uint16_t, MidmemAlloc> tmpLine;
  tmpLine.resize(N * 3);

  int localStart = start;
  int remaining = count;
  while (remaining > 0)
  {
    int domain = (localStart / N) % domainsUsed;
    int row = localStart % N;
    int can = min(remaining, N - row);
    for (int r = 0; r < can; ++r)
    {
      float maxZRow = 0.0f;
      bool rowExported =
        chopGen.export_displacement_row_u16(domain, N, row + r, tmpLine.data(), seaLevel, maxWaveSize[domain], destFifo, maxZRow);
      if (!rowExported)
      { // shouldn't happen since we streamline jobs
        G_ASSERT(false);
        break;
      }

      uint16_t *dst = cascades[domain].fifo[destFifo].data() + (row + r) * (N * 3);
      memcpy(dst, tmpLine.data(), sizeof(uint16_t) * N * 3);
      cascades[domain].cascadeMaxZ[destFifo] = max(cascades[domain].cascadeMaxZ[destFifo], maxZRow);
      ++totalComputed;
      ++localStart;
    }
    remaining -= can;
  }

  if (start + count == totalRowsPerStage)
  {
    fifoMaxZ[destFifo] = 0.f;
    for (int i = 0; i < numCascades; ++i)
      fifoMaxZ[destFifo] += cascades[i].cascadeMaxZ[destFifo];
    allFifoMaxZLocal = 0.f;
    for (int fifo = 0; fifo < MAX_FIFO; fifo++)
      allFifoMaxZLocal = max(allFifoMaxZLocal, fifoMaxZ[fifo]);
    allFifoMaxZ.store(allFifoMaxZLocal);
  }
  return totalComputed;
}

void ChopWaterPhysics::doJob()
{
  if (currentJobIndex < 0)
    return;
  if (!currentJobSize)
    return;
  if (currentJobIndex < endUpdateRowsJobIdx)
  {
    int jobDone = updateSnapshotRows(currentJobIndex, currentJobSize);
    if (jobDone <= 0 && chopGen.is_cpu_snapshot_complete())
    {
      currentJobIndex = endUpdateRowsJobIdx;
      currentJobSize = totalJobSize - currentJobIndex;
      return;
    }
    interlocked_add(currentJobIndex, jobDone);
    interlocked_add(currentJobSize, -jobDone);
    return; // for chop finalize_snapshot_cascade is a fat task for single cascade, so we always do it in fresh job
  }
  if (currentJobIndex < endUpdateRowsJobIdx)
    return;
  if (currentJobIndex < endFinalizeCascadesJobIdx) // compute phase
  {
    int cascadeIndex = currentJobIndex - endUpdateRowsJobIdx;
    if (cascadeIndex >= WATER_CPU_CASCADES_COUNT)
    {
      currentJobIndex = endFinalizeCascadesJobIdx;
      currentJobSize = totalJobSize - currentJobIndex;
      return;
    }
    int jobDone = finalizeSnapshotCascade(cascadeIndex, currentJobSize);
    if (jobDone <= 0)
    {
      currentJobIndex = endFinalizeCascadesJobIdx;
      currentJobSize = totalJobSize - currentJobIndex;
      return;
    }
    else
    {
      interlocked_add(currentJobIndex, jobDone);
      interlocked_add(currentJobSize, -jobDone);
    }
    return; // always yield after compute phase step to avoid tight loops when jobDone==0
  }

  if (currentJobIndex < endFinalizeCascadesJobIdx) // average each fft computation is of the same size as all updateHT
    return;
  // starting from this moment, old firstTick is invalid

  int endDisplaceJobIdx = totalJobSize;
  if (currentJobIndex < endDisplaceJobIdx) // average each fft computation is of the same size as all updateHT
  {
    int destFifo = asyncUpdatingTick % MAX_FIFO;
    if (currentJobIndex == endFinalizeCascadesJobIdx)
      interlocked_release_store(updatingFifo, destFifo); // now, last tick is officially nextLastTick
    if (updatingFifo == -1)
    {
      interlocked_release_store(lastTick, asyncUpdatingTick); // now, last available tick is officially nextLastTick
      interlocked_release_store(updatingFifo, -1);            // now, last tick is officially nextLastTick*/
      currentJobIndex = -1;                                   // now, last tick is officially nextLastTick*/
      currentJobSize = 0;
      return;
    }
    int jobDone = getDisplaceData(interlocked_acquire_load(updatingFifo), currentJobIndex - endFinalizeCascadesJobIdx, currentJobSize);
    interlocked_add(currentJobIndex, +jobDone);
    interlocked_add(currentJobSize, -jobDone);
  }

  if (currentJobIndex == endDisplaceJobIdx) // average each fft computation is of the same size as all updateHT
  {
    interlocked_release_store(lastTick, asyncUpdatingTick); // now, last available tick is officially nextLastTick
    interlocked_release_store(updatingFifo, -1);            // now, last tick is officially nextLastTick*/
    currentJobIndex = -1;                                   // now, last tick is officially nextLastTick*/
    currentJobSize = 0;                                     // now, last tick is officially nextLastTick*/
  }
}

void ChopWaterPhysics::runAsyncTask(int nextLastTick, float tasks)
{
  threadpool::wait(this);
  if (currentJobIndex < 0)
  {
    // debug("startAsyncNextTick %d", nextLastTick);
    double futureTime = nextLastTick * tickRate;
    setCascadesSimulationTime(futureTime);
    asyncUpdatingTick = nextLastTick;
    currentJobIndex = 0;
    currentJobSize = 0;
  }
  // debug("portion %g", tasks);

  enum
  {
    MIN_JOB_SIZE_TO_START = 8,
    MIN_JOB_SIZE_TO_DO = 16,
    START_JOB_SIZE = MIN_JOB_SIZE_TO_DO * 2
  };

  int nextJobSize = currentJobIndex == 0 ? START_JOB_SIZE : int((totalJobSize - currentJobIndex) * tasks);
  nextJobSize = clamp(nextJobSize, (int)MIN_JOB_SIZE_TO_DO, totalJobSize - currentJobIndex);

  if (currentJobIndex < endUpdateRowsJobIdx)
  {
    // split update_chop_rows and finalize phase to separate jobs
    nextJobSize = min(nextJobSize, endUpdateRowsJobIdx - currentJobIndex);
  }
  else if (currentJobIndex < endFinalizeCascadesJobIdx)
  {
    // Compute phase (finalize) is 1 job per cascade
    nextJobSize = 1;
  }
  else
  {
    // Export (getDisplaceData) is fast in chop, do the entire export in one job
    nextJobSize = totalJobSize - currentJobIndex;
  }
  currentJobSize = nextJobSize;

  // int64_t reft = ref_time_ticks();
  // debug("portion of (%g)%d in %dusec", tasks, nextJobSize, get_time_usec(reft));
  threadpool::add(this);
}

void ChopWaterPhysics::calcWaveHeight()
{
  calc_wave_height_chop(chopGen, numCascades, significantWaveHeight, maxWaveHeight, maxWaveSize.data());
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    float maxWave = maxWaveSize[cascadeNo];
    maxWaveSize[cascadeNo] = max(maxWave, 0.5f);
  }

  maxWaveHeight += 0.00001f;
  maxSeaLevel = seaLevel + maxWaveHeight;

  float maxRange = maxWaveHeight;
  if (waterHeightmap)
    maxRange += waterHeightmap->heightScale;
  max_num_successive_steps = clamp(int(maxRange * (16.f / 30.f)), 3, 16);
  max_num_binary_steps = clamp(int(maxRange * (16.f / 30.f) + 4.f), 4, 16);
  initializeCascades();
}

int ChopWaterPhysics::intersectRayWithOcean(double time, Point3 &result, float &T, const Point3 &in_position, const Point3 &direction,
  Point3 *out_displacement, bool matchRenderGrid)
{
  vec3f test_point_xz, old_test_point_xz, displacements;
  float test_point_ray_height;   // height above water
  float test_point_water_height; // height above water
  float t, outT = 0.f;           // distance traveled along the ray while tracing
  static constexpr int REFINE_STEPS = 4;
  const float t_threshold = 0.05f;                        // we stop successive tracing when we don't progress more than 5 cm each step
  constexpr float refinement_threshold_sqr = 0.1f * 0.1f; // we stop refinement step when we don't progress more than 10cm while doing
                                                          // refinement of current water altitude
  const bool verticalTrace = (fabsf(direction.x) + fabsf(direction.z) < 1e-5);
  const float t_multiplier = verticalTrace ? 1.0f : 1.8f / (fabs(direction.y) + 1.0f); // we increase step length at steep angles to
                                                                                       // speed up the tracing, but less than 2 to make
                                                                                       // sure the process converges and to add some
                                                                                       // safety to minimize chance of overshooting


  // checking if ray is outside of ocean surface volume . Checked in intersect segment
  // if ((in_position.y >= maxWaveHeight + seaLevel) && (in_position.y + T*direction.y >= maxWaveHeight + seaLevel))
  //  return 0;

  // getting to the top edge of volume where we can start
  // Do not extend the maxSeaLevel for frame to frame consistency.
  float allFifoMaxZLocal = allFifoMaxZ.load();
  float currentMaxSeaLevel = min(maxSeaLevel, seaLevel + allFifoMaxZLocal);
  if (waterHeightmap)
    currentMaxSeaLevel = waterHeightmap->heightMax + allFifoMaxZLocal;
  vec3f position = v_ldu(&in_position.x);
  const vec3f v_dir = v_ldu(&direction.x);
  if (in_position.y > currentMaxSeaLevel && direction.y < 0)
  {
    // todo: if maxWaveHeight is less than t_threshold, simply intersect with plane
    t = -(in_position.y - currentMaxSeaLevel) / direction.y;

    outT = t;
    position = v_madd(v_splats(t), v_dir, position);
  }

  int fifo1, fifo2;
  float fifo2Part;
  getFifoIndex(time, fifo1, fifo2, fifo2Part);
  vec3f v_fifo2Part = v_splats(fifo2Part);
  // tracing the ocean surface:
  // moving along the ray by distance defined by vertical distance form current test point, increased/decreased by safety multiplier
  // this process will converge despite our assumption on local flatness of the surface because curvature of the surface is smooth
  // NB: this process guarantees we don't shoot through wave tips
  int num_steps = 0;
  vec3f disp1, disp2;
  vec4f disp;
  const vec3f v_refinement_threshold_sqr = v_splats(refinement_threshold_sqr);
  for (; num_steps < max_num_successive_steps; ++num_steps)
  {
    displacements = v_zero();
    for (int k = 0; k < REFINE_STEPS; k++) // few refinement steps to converge at correct intersection point
    {
      // moving back sample points by the displacements read initially,
      // to get a guess on which undisturbed water surface point moved to the actual sample point
      // due to x,y motion of water surface, assuming the x,y disturbances are locally constant
      vec3f test_point_xz = v_sub(v_perm_xzxz(position), displacements);
      // todo: speed up refinement, if next point is exactly like previous point in integer coordinates (i.e. only lerp parameters had
      // changed).
      //  should be common
      getDisplacements(disp1, disp2, test_point_xz, fifo1, fifo2);
      displacements = v_madd(v_sub(disp2, disp1), v_fifo2Part, disp1);
      if (matchRenderGrid)
        disp = getRenderedHeight(v_extract_x(test_point_xz), v_extract_y(test_point_xz));
      else
        disp = getHeightmapDataBilinear(v_extract_x(test_point_xz), v_extract_y(test_point_xz));
      displacements = v_madd(v_splat_w(disp), displacements, disp);
      if (k)
      {
        vec4f tpdiffxz = v_and(v_sub(old_test_point_xz, test_point_xz), v_cast_vec4f(V_CI_MASK1100));
        if (v_test_vec_x_le(v_length3_sq_x(tpdiffxz), v_refinement_threshold_sqr))
          break;
      }
      old_test_point_xz = test_point_xz;
    }
    // getting t to travel along the ray
    t = t_multiplier * (v_extract_y(position) - v_extract_z(displacements));

    // traveling along the ray
    position = v_madd(v_splats(t), v_dir, position);
    outT += t;

    if (t < t_threshold || verticalTrace)
    {
      if (outT < 0.f || outT >= T)
        return 0;
      v_stu_p3(&result.x, position);
      T = outT;
      if (out_displacement)
      {
        vec3f disp = v_div(v_madd(v_sub(disp2, disp1), v_fifo2Part, disp1), v_splats((float)tickRate));
        v_stu_half(out_displacement, v_perm_xzxz(disp));
        out_displacement->z = v_extract_y(disp);
      }
      return 1;
    }
  }

  // if we're looking down and we did not hit water surface, doing binary search to make sure we hit water surface,
  // but there is risk of shooting through wave tips if we are tracing at extremely steep angles
  if (direction.y < -0.01f)
  {
    float startT = -0.1f * maxWaveHeight;
    vec3f positionBSStart = v_madd(v_dir, v_splats(startT), position); // part of wave back, just in case we missed something

    // getting to the bottom edge of volume where we can start
    float endT = -(v_extract_y(position) + maxWaveHeight + (waterHeightmap ? waterHeightmap->heightScale : 0.0f)) / direction.y;
    vec3f positionBSEnd = v_madd(v_splats(endT), v_dir, position);
    int i;
    for (i = 0; i < max_num_binary_steps; i++)
    {
      float nextT = (startT + endT) * 0.5f;
      position = v_mul(v_add(positionBSStart, positionBSEnd), V_C_HALF);

      for (int k = 0; k < REFINE_STEPS; k++)
      {
        test_point_xz = v_sub(v_perm_xzxz(position), displacements);
        getDisplacements(disp1, disp2, test_point_xz, fifo1, fifo2);
        displacements = v_madd(v_sub(disp2, disp1), v_fifo2Part, disp1);
        if (matchRenderGrid)
          disp = getRenderedHeight(v_extract_x(test_point_xz), v_extract_y(test_point_xz));
        else
          disp = getHeightmapDataBilinear(v_extract_x(test_point_xz), v_extract_y(test_point_xz));
        displacements = v_madd(v_splat_w(disp), displacements, disp);
        if (k)
        {
          vec4f tpdiffxz = v_and(v_sub(old_test_point_xz, test_point_xz), v_cast_vec4f(V_CI_MASK1100));
          if (v_test_vec_x_le(v_length3_sq_x(tpdiffxz), v_refinement_threshold_sqr))
            break;
        }
        old_test_point_xz = test_point_xz;
      }
      if (v_test_vec_x_gt(v_perm_yzwx(position), v_perm_zwxy(displacements))) // position.y > displacements.z
      {
        positionBSStart = position;
        startT = nextT;
      }
      else
      {
        positionBSEnd = position;
        endT = nextT;
      }
      if (endT - startT < t_threshold)
        break;
    }
    outT += (startT + endT) * 0.5f;
    if (outT < 0.f || outT >= T)
      return 0;
    position = v_mul(v_add(positionBSStart, positionBSEnd), V_C_HALF);
    v_stu_p3(&result.x, position);
    T = outT;
    if (out_displacement)
    {
      vec3f disp = v_div(v_madd(v_sub(disp2, disp1), v_fifo2Part, disp1), v_splats((float)tickRate));
      v_stu_half(out_displacement, v_perm_xzxz(disp));
      out_displacement->z = v_extract_y(disp);
    }
    return 1;
  }
  return 0;
}

// Public function available for exteral code
int ChopWaterPhysics::intersectSegment(double time, const Point3 &vStart, const Point3 &vEnd, float &fResult)
{
  // G_ASSERT(!check_nan(vStart));
  // G_ASSERT(!check_nan(vEnd));
  if (cascades.empty() && !waterHeightmap)
  {
    if ((vStart.y >= seaLevel) == (vEnd.y >= seaLevel)) // ray is below or above water
      return 0;
    if (fabsf(vEnd.y - vStart.y) < 1e-6f)
      fResult = 0.5f; // avoid division by small number, but don't miss intersection
    else
      fResult = (seaLevel - vStart.y) / (vEnd.y - vStart.y);
    fResult *= length(vEnd - vStart); // it returns meters!
    return 1;
  }
  Point3 direction = vEnd - vStart;
  Point3 result;
  // early out if start and end are both above possible water volume
  float allFifoMaxZLocal = allFifoMaxZ.load();
  float currentMaxSeaLevel = min(maxSeaLevel, seaLevel + allFifoMaxZLocal);
  if (waterHeightmap)
    currentMaxSeaLevel = waterHeightmap->heightMax + allFifoMaxZLocal;
  if ((vStart.y > currentMaxSeaLevel) && (vEnd.y > currentMaxSeaLevel))
    return 0;

  // finding intersection using ray casting against water volume
  float rayLength = length(direction);
  if (rayLength < 1e-6)
    return 0;
  float rayLength2 = rayLength;
  direction /= rayLength;
  if (intersectRayWithOcean(time, result, rayLength, vStart, direction))
  {
    // G_ASSERTF(rayLength < rayLength2 && rayLength>=0, "%.16f %.16f", rayLength, rayLength2);
    fResult = clamp(rayLength, 0.f, rayLength2);
    return 1;
  }
  return 0;
}

static inline float triangle_s(const Point2 &v1, const Point2 &v2) { return abs(v1.x * v2.y - v1.y * v2.x); } // actually s*2
static inline float barycentric_interpolate(Point2 A, Point2 B, Point2 C, Point2 P, Point3 heights, float align)
{
  float s = align * align;
  float s1 = triangle_s(B - P, C - P);
  float s2 = triangle_s(C - P, A - P);
  float s3 = s - s1 - s2;
  return (heights[0] * s1 + heights[1] * s2 + heights[2] * s3) / s;
}
vec4f ChopWaterPhysics::getRenderedHeight(float x, float z)
{
  float result[4] = {0.0f, 0.0f, seaLevel, 1.0f};
  if (!waterHeightmap)
    return v_ldu(result);
  int triangleX = floorf((x - renderGridOffset.x) / renderGridAlign);
  int triangleZ = floorf((z - renderGridOffset.y) / renderGridAlign);
  float px = floorf(x / renderGridAlign) * renderGridAlign;
  float pz = floorf(z / renderGridAlign) * renderGridAlign;
  float d = renderGridAlign;
  Point2 p(x, z);
  Point2 points[4] = {Point2(px, pz), Point2(px + d, pz), Point2(px, pz + d), Point2(px + d, pz + d)};
  float samples[4] = {
    0,
  };
  for (int i = 0; i < 4; i++)
    samples[i] = v_extract_z(getHeightmapDataBilinear(points[i].x, points[i].y));
  IPoint3 indexes[4] = {IPoint3(0, 1, 3), IPoint3(0, 2, 3), IPoint3(2, 0, 1), IPoint3(2, 3, 1)};
  int i = (abs(triangleX + triangleZ) % 2) * 2;
  Point2 diag = points[indexes[i].z] - points[indexes[i].x];
  Point2 dir = p - points[indexes[i].x];
  i += dir.x * diag.y - dir.y * diag.x < 0;
  result[2] = barycentric_interpolate(points[indexes[i].x], points[indexes[i].y], points[indexes[i].z], p,
    Point3(samples[indexes[i].x], samples[indexes[i].y], samples[indexes[i].z]), renderGridAlign);
  return v_ldu(result);
}

int ChopWaterPhysics::getHeightAboveWater(double time, const Point3 &point, float &result, Point3 *out_displacement,
  bool matchRenderGrid)
{
  if (cascades.empty())
  {
    if (waterHeightmap)
    {
      vec3f disp;
      if (matchRenderGrid)
        disp = getRenderedHeight(point.x, point.z);
      else
        disp = getHeightmapDataBilinear(point.x, point.z);
      result = point.y - v_extract_z(disp);
    }
    else
      result = point.y - seaLevel;
    if (out_displacement)
      *out_displacement = Point3(0, 0, 0);
    return 1;
  }
  Point3 resPos;
  Point3 direction = Point3(0, -1.0f, 0);
  float t = maxWaveHeight * 2.0f + 0.01f;
  if (waterHeightmap)
    t += waterHeightmap->heightScale;
  float originY = maxSeaLevel;
  if (waterHeightmap)
    originY = waterHeightmap->heightMax + maxWaveHeight;
  if (intersectRayWithOcean(time, resPos, t, Point3::xVz(point, originY), Point3(0, -1.0f, 0), out_displacement, matchRenderGrid))
  {
    result = point.y - resPos.y;
    return 1;
  }
  return 0;
}

void ChopWaterPhysics::getDisplacementsBilinearChop(vec3f &disp1, vec3f &disp2, vec4f xz, int fifo1, int fifo2) const
{
  float wx = v_extract_x(xz);
  float wz = v_extract_y(xz);
  float domainSize = chopGen.getDomainSize();
  float u = wx / domainSize;
  float v = wz / domainSize;

  int bits = 0;
  while ((1 << bits) < WATER_CPU_HEIGHTMAP_RES)
    ++bits;

  vec4f identScaleOfs = v_make_vec4f(1.0f, 1.0f, 1.0f, 0.0f);

  float h1 = 0.0f, h2 = 0.0f;
  for (int d = 0; d < numCascades; ++d)
  {
    Point2 domainUV = CalcDistortedCascadeUV(d, Point2(u, v), convar::uv_distort_strength);

    DECL_ALIGN16(int, adr[4]);
    vec4f domainInV = v_make_vec4f(domainUV.x, domainUV.y, 0.0f, 0.0f);
    vec4f uvFrac = get_bilinear_adr(bits, 1.0f, domainInV, adr);

    vec4f s1 = get_displacement_u16_bilinear(adr, uvFrac, cascades[d].fifo[fifo1].data(), identScaleOfs);
    vec4f s2 = get_displacement_u16_bilinear(adr, uvFrac, cascades[d].fifo[fifo2].data(), identScaleOfs);

    float amp = maxWaveSize[d];
    float uy1 = v_extract_y(s1);
    float uy2 = v_extract_y(s2);
    float h_d1 = ((uy1 / 65535.0f) - 0.5f) * (2.0f * amp);
    float h_d2 = ((uy2 / 65535.0f) - 0.5f) * (2.0f * amp);
    h1 += h_d1;
    h2 += h_d2;
  }

  disp1 = v_make_vec3f(0.0f, 0.0f, h1);
  disp2 = v_make_vec3f(0.0f, 0.0f, h2);
}