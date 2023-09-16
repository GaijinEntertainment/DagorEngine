#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_threadPool.h>
#include <perfMon/dag_statDrv.h>
#include "waterPhys.h"
#include <math/dag_adjpow2.h>
#include <fftWater/fftWater.h>
#include <math/integer/dag_IPoint3.h>

#define THRESHOLD_TO_SWITCH_OFF_WAVES 0.2

WaterNVPhysics::~WaterNVPhysics()
{
  threadpool::wait(this);
  if (cascades)
    delete[] cascades;
  cascades = 0;
}

void WaterNVPhysics::reset()
{
  threadpool::wait(this);

  lastTick = 0;
  updatingFifo = -1;

  currentJobIndex = -1;
  currentJobSize = 0;
  if (cascades)
  {
    for (int i = 0; i < MAX_FIFO; ++i)
      syncRun(i);
  }
}

bool WaterNVPhysics::validateNextTimeTick(double time)
{
  int currentTick = ceil(time / tickRate);
  return currentTick > lastTick - MAX_FIFO;
}

void WaterNVPhysics::setCascades(const NVWaveWorks_FFT_CPU_Simulation::Params &p)
{
  noActualWaves = false;
  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
    fftParams[cascadeNo] =
      simulation_cascade_params(p, fftSimulationParams, cascadeNo, numCascades, cascadeWindowLength, cascadeFacetSize);

  calcWaveHeight();
}

void WaterNVPhysics::initializeCascades()
{
  threadpool::wait(this);

  mem_set_0(fifoMaxZ);
  allFifoMaxZ = 0;
  if (noActualWaves)
  {
    delete[] cascades;
    cascades = nullptr;
    return;
  }

  if (!cascades)
    cascades = new Cascade[numCascades];

  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    set_cascade_params(cascades[cascadeNo].fft, fftParams[cascadeNo], true);
    const int fftRes = 1 << cascades[cascadeNo].fft.getParams().fft_resolution_bits;
    for (int i = 0; i < cascades[cascadeNo].fifo.size(); ++i)
    {
      clear_and_resize(cascades[cascadeNo].fifo[i], fftRes * fftRes * 3 + 1);
      mem_set_0(cascades[cascadeNo].fifo[i]);
    }
    mem_set_0(cascades[cascadeNo].cascadeMaxZ);
  }
}

WaterNVPhysics::WaterNVPhysics(const NVWaveWorks_FFT_CPU_Simulation::Params &p, const fft_water::SimulationParams &simulation,
  int num_cascades, float cascade_window_length, float cascade_facet_size, const fft_water::WaterHeightmap *water_heightmap) :
  cascades(NULL),
  fftSimulationParams(simulation),
  cascadeWindowLength(cascade_window_length),
  cascadeFacetSize(cascade_facet_size),
  waterHeightmap(water_heightmap)
{
  currentJobIndex = -1;
  currentJobSize = 0;
  noActualWaves = false;
  numCascades = min(num_cascades, (int)fft_water::MAX_NUM_CASCADES);
  int fftRes = 1 << p.fft_resolution_bits;
  seaLevel = maxSeaLevel = 0.f;
  // tickRate = 1/3.0;//three times a second is more than enough
  tickRate = 0.375; // three times a second is more than enough, use 0.375
  totalNCount = num_cascades * fftRes;
  debug("totalNCount = %d", totalNCount);
  lastTick = 0;
  updatingFifo = -1;
  setCascades(p);
  endUpdateHtJobIdx = totalNCount;
  endFFTJobIdx = endUpdateHtJobIdx + totalNCount * 3;
  totalJobSize = endFFTJobIdx + totalNCount;
  forceActualWaves = false;
  reset();
}

void WaterNVPhysics::reinit(const Point2 &wind_dir, float wind_speed, float period)
{
  NVWaveWorks_FFT_CPU_Simulation::Params params = fftParams[0];
  params.wind_dir_x = wind_dir.x;
  params.wind_dir_y = wind_dir.y;
  params.wind_speed = wind_speed;
  params.fft_period = period;
  setCascades(params);
}

bool WaterNVPhysics::isAsyncRunning() const { return interlocked_acquire_load(this->done) == 0; }

void WaterNVPhysics::increaseTime(double time)
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
  G_ASSERT((cascades == nullptr) == noActualWaves);

  if (!cascades)
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

void WaterNVPhysics::setCascadesSimulationTime(double time)
{
  if (!cascades)
    return;
  for (int i = 0; i < numCascades; ++i)
    cascades[i].fft.setTime(time);
}

void WaterNVPhysics::syncRun(int nextLastTick)
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
  doJob();
  debug("sync to %d in %dus", nextLastTick, get_time_usec(reft));

  /*int64_t reft = ref_time_ticks();
  updateH0();
  updateHt(0, totalNCount);
  computeFFT(0, 3*totalNCount);

  //starting from this moment, old firstTick is invalid
  int destFifo = nextLastTick%MAX_FIFO;

  getDisplaceData(destFifo, 0, totalNCount);
  int totalTex = get_time_usec(reft);

  lastTick = nextLastTick;//now, last tick is officially nextLastTick
  debug("sync to %d(%d) in %dus", nextLastTick, destFifo, get_time_usec(reft));*/
}

void WaterNVPhysics::updateH0()
{
  if (!cascades)
    return;
  for (int i = 0; i < numCascades; ++i)
  {
    const int N = 1 << cascades[i].fft.getParams().fft_resolution_bits;
    if (cascades[i].fft.IsH0UpdateRequired())
    {
      for (int j = 0; j <= N; ++j)
        cascades[i].fft.UpdateH0(j);
      cascades[i].fft.SetH0UpdateNotRequired();
    }
  }
}

int WaterNVPhysics::updateHt(int start, int count)
{
  if (!cascades)
    return 0;
  int totalComputed = 0;
  for (int i = 0; i < numCascades; ++i)
  {
    const int N = 1 << cascades[i].fft.getParams().fft_resolution_bits;
    if (start >= N)
    {
      start -= N;
      continue;
    }
    int num = min(count, N - start);
    for (int j = start; j < start + num; ++j)
      cascades[i].fft.UpdateHt(j);
    totalComputed += num;
    if (totalComputed == count)
      break;
    start = 0;
    count -= num;
  }
  return totalComputed;
}

int WaterNVPhysics::computeFFT(int start, int count)
{
  if (!cascades)
    return 0;
  int totalComputed = 0;
  for (int i = 0; i < numCascades; ++i)
  {
    const int res = 1 << cascades[i].fft.getParams().fft_resolution_bits;
    const int N = 3 * res;
    if (start >= N)
    {
      start -= N;
      continue;
    }
    int num = min(count, N - start);
    // debug("cascade %d start=%d num=%d count=%d N=%d, ",i,start, num, count, N);
    G_ASSERT(start % res == 0);
    if (num < res)
      return totalComputed;
    for (int j = start / res; j < (start + num) / res; ++j)
    {
      cascades[i].fft.ComputeFFT(j);
      totalComputed += res;
    }
    if (totalComputed == count)
      break;
    start = 0;
    count -= num;
  }
  return totalComputed;
}

int WaterNVPhysics::getDisplaceData(int destFifo, int start, int count)
{
  if (!cascades)
    return 0;
  if (start == 0)
    for (int i = 0; i < numCascades; ++i)
      cascades[i].cascadeMaxZ[destFifo] = 0.f;

  int totalComputed = 0;
  for (int i = 0; i < numCascades; ++i)
  {
    const int N = 1 << cascades[i].fft.getParams().fft_resolution_bits;
    if (start >= N)
    {
      start -= N;
      continue;
    }
    int num = min(count, N - start);
    int stride = N * 3;
    float &cascadeMaxZ = cascades[i].cascadeMaxZ[destFifo];
    uint16_t *data = cascades[i].fifo[destFifo].data() + start * stride;
    for (int j = start; j < start + num; ++j, data += stride)
    {
      float maxZ;
      cascades[i].fft.getUint16Data(j, -maxWaveSize[i], maxWaveSize[i] * 2.f, data, maxZ);
      cascadeMaxZ = max(cascadeMaxZ, maxZ);
    }

    if (i == numCascades - 1 && start + num == N)
    {
      fifoMaxZ[destFifo] = 0.f;
      for (int i = 0; i < numCascades; ++i)
        fifoMaxZ[destFifo] += cascades[i].cascadeMaxZ[destFifo];
      allFifoMaxZ = 0.f;
      for (int fifo = 0; fifo < MAX_FIFO; fifo++)
        allFifoMaxZ = max(allFifoMaxZ, fifoMaxZ[fifo]);
    }

    totalComputed += num;
    if (totalComputed == count)
      break;
    start = 0;
    count -= num;
  }
  return totalComputed;
}

void WaterNVPhysics::doJob()
{
  TIME_PROFILE(water_phys);
  if (currentJobIndex < 0)
    return;
  if (!currentJobIndex)
    updateH0();
  if (!currentJobSize)
    return;
  if (currentJobIndex < endUpdateHtJobIdx)
  {
    int jobDone = updateHt(currentJobIndex, currentJobSize);
    interlocked_add(currentJobIndex, jobDone);
    interlocked_add(currentJobSize, -jobDone);
  }
  if (currentJobIndex < endUpdateHtJobIdx)
    return;
  if (currentJobIndex < endFFTJobIdx) // average each fft computation is of the same size as all updateHT
  {
    int jobDone = computeFFT((currentJobIndex - endUpdateHtJobIdx), currentJobSize);
    interlocked_add(currentJobIndex, jobDone);
    interlocked_add(currentJobSize, -jobDone);
  }

  if (currentJobIndex < endFFTJobIdx) // average each fft computation is of the same size as all updateHT
    return;
  // starting from this moment, old firstTick is invalid

  int endDisplaceJobIdx = totalJobSize;
  if (currentJobIndex < endDisplaceJobIdx) // average each fft computation is of the same size as all updateHT
  {
    int destFifo = asyncUpdatingTick % MAX_FIFO;
    if (currentJobIndex == endFFTJobIdx)
      interlocked_release_store(updatingFifo, destFifo); // now, last tick is officially nextLastTick
    int jobDone = getDisplaceData(interlocked_acquire_load(updatingFifo), currentJobIndex - endFFTJobIdx, currentJobSize);
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

void WaterNVPhysics::runAsyncTask(int nextLastTick, float tasks)
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

  int nextJobSize = currentJobIndex == 0 ? START_JOB_SIZE : (totalJobSize - currentJobIndex) * tasks;
  if (nextJobSize < MIN_JOB_SIZE_TO_START)
    return;
  nextJobSize = clamp(nextJobSize, (int)MIN_JOB_SIZE_TO_DO, totalJobSize - currentJobIndex);
  currentJobSize = nextJobSize;

  if (currentJobIndex >= endUpdateHtJobIdx && currentJobIndex < endFFTJobIdx &&
      (!cascades || currentJobSize < (1 << cascades[0].fft.getParams().fft_resolution_bits)))
    return; // won't do anything at all
  // int64_t reft = ref_time_ticks();
  // debug("portion of (%g)%d in %dusec", tasks, nextJobSize, get_time_usec(reft));
  threadpool::add(this);
}

void WaterNVPhysics::setForceActualWaves(bool enforce)
{
  if (forceActualWaves != enforce)
  {
    forceActualWaves = enforce;
    calcWaveHeight();
  }
}

void WaterNVPhysics::calcWaveHeight()
{
  const Point2 initialShoreDamp(3.0f, 6.0f);
  calc_wave_height(fftParams.data(), numCascades, significantWaveHeight, maxWaveHeight, maxWaveSize.data());
  shoreDamp =
    Point2(min(initialShoreDamp.x, significantWaveHeight), max(min(initialShoreDamp.y, significantWaveHeight * 2.0f), 1e-2f));
  shoreDamp = Point2(1.0 / (shoreDamp.y - shoreDamp.x), -shoreDamp.x / (shoreDamp.y - shoreDamp.x));

  for (int cascadeNo = 0; cascadeNo < numCascades; ++cascadeNo)
  {
    float maxWave = maxWaveSize[cascadeNo];
    maxWaveSize[cascadeNo] = max(maxWave, 0.5f);
    fftScaleOfs[cascadeNo] = v_make_vec4f(maxWave * (2. / 65535), maxWave * (2. / 65535), maxWave * (2. / 65535), -maxWave);
  }

  noActualWaves = !forceActualWaves && maxWaveHeight < THRESHOLD_TO_SWITCH_OFF_WAVES;
  debug("maxWaveHeight = %f", maxWaveHeight);
  if (noActualWaves)
  {
    maxWaveHeight = 0;
    significantWaveHeight = 0;
  }
  else
  {
    maxWaveHeight += 0.00001f;
    maxSeaLevel = seaLevel + maxWaveHeight;
  }
  float maxRange = maxWaveHeight;
  if (waterHeightmap)
    maxRange += waterHeightmap->heightScale;
  max_num_successive_steps = clamp(int(maxRange * (16.f / 30.f)), 3, 16);
  max_num_binary_steps = clamp(int(maxRange * (16.f / 30.f) + 4.f), 4, 16);
  initializeCascades();
  // max_num_successive_steps = 16;
  // max_num_binary_steps = 16;
  // debug("phys maxWaveHeight %g total_rms = %g, steps = %d %d", maxWaveHeight, total_rms, max_num_successive_steps,
  // max_num_binary_steps);
}

int WaterNVPhysics::intersectRayWithOcean(double time, Point3 &result, float &T, const Point3 &in_position, const Point3 &direction,
  Point3 *out_displacement, bool matchRenderGrid)
{
  G_ASSERT(cascades != nullptr && !noActualWaves);
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
  float currentMaxSeaLevel = min(maxSeaLevel, seaLevel + allFifoMaxZ); // Do not extend the maxSeaLevel for frame to frame consistency.
  if (waterHeightmap)
    currentMaxSeaLevel = waterHeightmap->heightMax + allFifoMaxZ;
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
int WaterNVPhysics::intersectSegment(double time, const Point3 &vStart, const Point3 &vEnd, float &fResult)
{
  // G_ASSERT(!check_nan(vStart));
  // G_ASSERT(!check_nan(vEnd));
  if (!cascades)
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
  float currentMaxSeaLevel = min(maxSeaLevel, seaLevel + allFifoMaxZ);
  if (waterHeightmap)
    currentMaxSeaLevel = waterHeightmap->heightMax + allFifoMaxZ;
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
vec4f WaterNVPhysics::getRenderedHeight(float x, float z)
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

int WaterNVPhysics::getHeightAboveWater(double time, const Point3 &point, float &result, Point3 *out_displacement,
  bool matchRenderGrid)
{
  if (!cascades)
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
  if (
    intersectRayWithOcean(time, resPos, t, Point3(point.x, originY, point.z), Point3(0, -1.0f, 0), out_displacement, matchRenderGrid))
  {
    result = point.y - resPos.y;
    return 1;
  }
  return 0;
}
