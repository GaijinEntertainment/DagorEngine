// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsCounter.h>
#include <render/lights/lightsPartition.h>
#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_perfTimer.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_assert.h>
#include <util/dag_globDef.h>
#include <math/dag_hlsl_floatx.h>
#include <render/lights/renderLights.hlsli>

LightsCounter::LightsCounter(OmniLightsManager *omni_lights_manager, SpotLightsManager *spot_lights_manager) :
  omniLightsManager(omni_lights_manager), spotLightsManager(spot_lights_manager)
{}

LightsCounter::~LightsCounter()
{
  if (threadpool::get_num_workers() == 0)
    return;
  for (auto &job : jobs)
    threadpool::wait(&job);
}

void LightsCounter::countVisibleLights(const LightsVisibilityChecker &visibility_checker, uint32_t omni_lights_count,
  uint32_t spot_lights_count, size_t first_bucket, size_t last_bucket)
{
  G_ASSERT(first_bucket <= last_bucket);

  const auto lastBucketForOmniStep = min(last_bucket, BUCKETS_COUNT_FOR_OMNI_LIGHTS - 1);
  const auto firstBucketForSpotStep = max(first_bucket, BUCKETS_COUNT_FOR_OMNI_LIGHTS);

  if (first_bucket <= lastBucketForOmniStep)
    countVisibleLightsImpl<LightType::Omni>(visibility_checker, omni_lights_count, first_bucket, lastBucketForOmniStep);

  if (firstBucketForSpotStep <= last_bucket)
    countVisibleLightsImpl<LightType::Spot>(visibility_checker, spot_lights_count, firstBucketForSpotStep, last_bucket);
}

template <LightType Type>
void LightsCounter::countVisibleLightsImpl(const LightsVisibilityChecker &visibility_checker, uint32_t lights_count,
  size_t first_bucket, size_t last_bucket)
{
  static_assert(Type == LightType::Omni || Type == LightType::Spot, "LightsCounter::countVisibleLightsImpl - invalid LightType");
  G_ASSERT(first_bucket <= last_bucket);

  if constexpr (Type == LightType::Omni)
  {
    G_ASSERT(last_bucket < BUCKETS_COUNT_FOR_OMNI_LIGHTS);
  }
  else
  {
    G_ASSERT(first_bucket >= BUCKETS_COUNT_FOR_OMNI_LIGHTS);
  }

  for (size_t bucketIndex = first_bucket; bucketIndex <= last_bucket; ++bucketIndex)
  {
    Bucket bucket{};
    uint32_t firstLightIdInBucket = static_cast<uint32_t>(bucketIndex * BUCKET_SIZE);
    if constexpr (Type == LightType::Spot)
    {
      firstLightIdInBucket -= static_cast<uint32_t>(BUCKETS_COUNT_FOR_OMNI_LIGHTS * BUCKET_SIZE);
    }
    const uint32_t lightsEndInBucket = min<uint32_t>(lights_count, firstLightIdInBucket + static_cast<uint32_t>(BUCKET_SIZE));

    for (uint32_t lightId = firstLightIdInBucket; lightId < lightsEndInBucket; ++lightId)
    {
      switch (visibility_checker.test(Type, lightId))
      {
        case LightsVisibilityChecker::TestResult::Clustered:
          ++bucket.visibleCounts.clustered;
          ++bucket.lastAllocatedLightsCount;
          break;
        case LightsVisibilityChecker::TestResult::Far:
          ++bucket.visibleCounts.far;
          ++bucket.lastAllocatedLightsCount;
          break;
        case LightsVisibilityChecker::TestResult::Invisible:
        {
          bool isLightAllocated = false;
          if constexpr (Type == LightType::Omni)
            isLightAllocated = omniLightsManager->isLightValid(lightId);
          else
            isLightAllocated = spotLightsManager->isLightValid(lightId);

          if (isLightAllocated)
            ++bucket.lastAllocatedLightsCount;
          break;
        }
        default:
          G_ASSERT_FAIL("LightsCounter::countVisibleLightsImpl - unknown test result for light of type '%d' and id '%d'",
            static_cast<uint32_t>(Type), lightId);
          break;
      }
    }

    bucket.estimationsAllocatedLightsCount = buckets[bucketIndex].estimationsAllocatedLightsCount;
    buckets[bucketIndex] = bucket;
  }
}

size_t LightsCounter::findOptimalBucketConfiguration()
{
  if (optimalBucketConfigurationIndex.has_value())
    return optimalBucketConfigurationIndex.value();

  size_t result = 0;
  for (size_t confIndex = 1; confIndex < BUCKET_CONFIGURATIONS_COUNT; ++confIndex)
    if (bucketConfigurations[confIndex].executionTimesSum < bucketConfigurations[result].executionTimesSum)
      result = confIndex;

  optimalBucketConfigurationIndex = result;
  return result;
}

size_t LightsCounter::findBucketConfigurationToEstimate()
{
  size_t notEstimatedConfIndex = BUCKET_CONFIGURATIONS_COUNT;
  if (!optimalBucketConfigurationIndex.has_value())
  {
    for (size_t i = 0; i < BUCKET_CONFIGURATIONS_COUNT; ++i)
    {
      const size_t confIndex = (currentBucketConfigurationIndex + i) % BUCKET_CONFIGURATIONS_COUNT;
      if (bucketConfigurations[confIndex].requiredEstimations > 0)
      {
        notEstimatedConfIndex = confIndex;
        break;
      }
    }
  }
  return notEstimatedConfIndex;
}

void LightsCounter::update(const LightsVisibilityChecker &visibility_checker)
{
  TIME_D3D_PROFILE(lightsCounter_update);
  static_assert(LightsVisibilityChecker::isLightsCountThreadSafe(BUCKET_SIZE));
  static_assert(LightsVisibilityChecker::isLightsCountThreadSafe(OmniLightsManager::MAX_LIGHTS));
  static_assert(LightsVisibilityChecker::isLightsCountThreadSafe(SpotLightsManager::MAX_LIGHTS));

  const auto notEstimatedConfIndex = findBucketConfigurationToEstimate();

  const bool areAllConfigurationsEstimated = notEstimatedConfIndex == BUCKET_CONFIGURATIONS_COUNT;
  if (!areAllConfigurationsEstimated)
    currentBucketConfigurationIndex = notEstimatedConfIndex;
  else
    currentBucketConfigurationIndex = findOptimalBucketConfiguration();

  const uint32_t omniLightsCount = omniLightsManager->maxIndex() + 1;
  const uint32_t spotLightsCount = spotLightsManager->maxIndex() + 1;

  const uint64_t countStartTicks = profile_ref_ticks();
  const uint32_t lightsPerThread =
    static_cast<uint32_t>(BUCKET_SIZE << (BUCKET_CONFIGURATIONS_COUNT - 1 - currentBucketConfigurationIndex));

  // update can run in a threadpool job (LightsCullingJob in daNetGame), then this worker cannot take a count job
  const int freeWorkersCount = threadpool::get_num_workers() - (threadpool::get_current_worker_id() >= 0 ? 1 : 0);
  const size_t maxJobsCount = max(freeWorkersCount, 0);

  submittedJobIndices.clear();
  size_t nextJobSlot = 0;
  size_t currentThreadLastBucket = BUCKETS_COUNT_FOR_ALL_LIGHTS - 1;
  auto submitRange = [&](size_t first_bucket, size_t last_bucket) {
    if (first_bucket == 0)
    {
      currentThreadLastBucket = last_bucket;
      return;
    }

    while (nextJobSlot < BUCKETS_COUNT_FOR_ALL_LIGHTS && !interlocked_acquire_load(jobs[nextJobSlot].done))
      ++nextJobSlot;
    if (nextJobSlot == BUCKETS_COUNT_FOR_ALL_LIGHTS)
    {
      countVisibleLights(visibility_checker, omniLightsCount, spotLightsCount, first_bucket, last_bucket);
      return;
    }

    submittedJobIndices.push_back(static_cast<uint32_t>(nextJobSlot));
    CountJob &job = jobs[nextJobSlot++];
    job.counter = this;
    job.visibilityChecker = &visibility_checker;
    job.omniLightsCount = omniLightsCount;
    job.spotLightsCount = spotLightsCount;
    job.firstBucket = first_bucket;
    job.lastBucket = last_bucket;
    job.claimed.store(0, dag::memory_order_release);
    threadpool::add(&job, threadpool::PRIO_DEFAULT);
  };

  uint32_t remainingLightsCount = 0;
  for (const auto &b : buckets)
    remainingLightsCount += b.lastAllocatedLightsCount;

  size_t remainingRangesCount = maxJobsCount + 1;
  auto getRangeLightsBudget = [&]() {
    return max<uint32_t>(lightsPerThread, dag::divide_align_up(remainingLightsCount, remainingRangesCount));
  };

  size_t rangeFirstBucket = 0;
  size_t rangeBucketsCount = 0;
  uint32_t rangeLightsCount = 0;
  uint32_t rangeLightsBudget = getRangeLightsBudget();

  for (size_t i = 0; i < BUCKETS_COUNT_FOR_ALL_LIGHTS; ++i)
  {
    const auto newRangeLightsCount = rangeLightsCount + buckets[i].lastAllocatedLightsCount;
    if (newRangeLightsCount <= rangeLightsBudget || remainingRangesCount == 1)
    {
      rangeLightsCount = newRangeLightsCount;
      ++rangeBucketsCount;
      continue;
    }

    submitRange(rangeFirstBucket, rangeFirstBucket + rangeBucketsCount - 1);
    remainingLightsCount -= rangeLightsCount;
    --remainingRangesCount;

    rangeLightsCount = buckets[i].lastAllocatedLightsCount;
    rangeFirstBucket = i;
    rangeBucketsCount = 1;
    rangeLightsBudget = getRangeLightsBudget();
  }
  submitRange(rangeFirstBucket, rangeFirstBucket + rangeBucketsCount - 1);

  countVisibleLights(visibility_checker, omniLightsCount, spotLightsCount, 0, currentThreadLastBucket);

  for (uint32_t jobIndex : submittedJobIndices)
    jobs[jobIndex].handledByCurrentThread = jobs[jobIndex].tryRun();
  for (uint32_t jobIndex : submittedJobIndices)
    if (!jobs[jobIndex].handledByCurrentThread)
      threadpool::wait(&jobs[jobIndex]);
  const uint64_t countTicks = profile_ref_ticks() - countStartTicks;

  omniCounts = {};
  spotCounts = {};

  bool shouldResetBucketConfigurations = updatesSinceEstimationsEnd >= BUCKET_CONFIGURATIONS_REESTIMATION_PERIOD;

  uint32_t sumLastAllocatedLightsCount = 0;
  uint32_t sumEstimationsAllocatedLightsCount = 0;
  for (size_t i = 0; i < BUCKETS_COUNT_FOR_ALL_LIGHTS; ++i)
  {
    const auto &b = buckets[i];
    VisibleCounts &counts = i < BUCKETS_COUNT_FOR_OMNI_LIGHTS ? omniCounts : spotCounts;
    counts.clustered += b.visibleCounts.clustered;
    counts.far += b.visibleCounts.far;
    shouldResetBucketConfigurations |= isLightsCountChanged(b.lastAllocatedLightsCount, b.estimationsAllocatedLightsCount);
    sumLastAllocatedLightsCount += b.lastAllocatedLightsCount;
    sumEstimationsAllocatedLightsCount += b.estimationsAllocatedLightsCount;
  }
  shouldResetBucketConfigurations |= isLightsCountChanged(sumLastAllocatedLightsCount, sumEstimationsAllocatedLightsCount);

  handleClusteredOverflow(omniCounts, MAX_CLUSTERED_OMNI_LIGHTS);
  handleClusteredOverflow(spotCounts, MAX_CLUSTERED_SPOT_LIGHTS);

  if (areAllConfigurationsEstimated)
  {
    ++updatesSinceEstimationsEnd;

    auto &selectedConfiguration = bucketConfigurations[currentBucketConfigurationIndex];
    selectedConfiguration.selectedTimesSum += countTicks;
    if (++selectedConfiguration.selectedUpdatesCount >= BUCKET_CONFIGURATION_VALIDATION_UPDATES)
    {
      const uint64_t meanTime = selectedConfiguration.selectedTimesSum / selectedConfiguration.selectedUpdatesCount;
      shouldResetBucketConfigurations |= meanTime * 100 * BUCKET_CONFIGURATION_ESTIMATIONS >
                                         selectedConfiguration.executionTimesSum * BUCKET_CONFIGURATION_DEGRADATION_PERCENT;

      selectedConfiguration.selectedTimesSum = 0;
      selectedConfiguration.selectedUpdatesCount = 0;
    }
  }

  if (shouldResetBucketConfigurations)
    resetBucketConfigurations();
  else
  {
    auto &configuration = bucketConfigurations[currentBucketConfigurationIndex];
    if (configuration.requiredEstimations > 0)
    {
      --configuration.requiredEstimations;
      configuration.executionTimesSum += countTicks;
    }
  }
}

bool LightsCounter::CountJob::tryRun()
{
  if (claimed.exchange(1, dag::memory_order_acq_rel) != 0)
    return false;
  counter->countVisibleLights(*visibilityChecker, omniLightsCount, spotLightsCount, firstBucket, lastBucket);
  return true;
}

void LightsCounter::CountJob::doJob() { tryRun(); }

void LightsCounter::handleClusteredOverflow(VisibleCounts &counts, uint32_t max_clustered_count)
{
  if (counts.clustered > max_clustered_count)
  {
    counts.far += counts.clustered - max_clustered_count;
    counts.clustered = max_clustered_count;
  }
  counts.far = min<uint32_t>(counts.far, MAX_VISIBLE_FAR_LIGHTS);
}

bool LightsCounter::isLightsCountChanged(uint32_t last_count, uint32_t estimations_count)
{
  const int32_t delta = static_cast<int32_t>(last_count) - static_cast<int32_t>(estimations_count);
  return abs(delta) > BUCKET_LIGHTS_COUNT_THRESHOLD;
}

void LightsCounter::resetBucketConfigurations()
{
  for (auto &bc : bucketConfigurations)
    bc = {};

  for (auto &b : buckets)
    b.estimationsAllocatedLightsCount = b.lastAllocatedLightsCount;

  updatesSinceEstimationsEnd = 0;
  optimalBucketConfigurationIndex.reset();
}

void LightsCounter::reset()
{
  omniCounts = VisibleCounts{};
  spotCounts = VisibleCounts{};
  mem_set_0(buckets);
  resetBucketConfigurations();
  currentBucketConfigurationIndex = 0;
}

const LightsCounter::VisibleCounts &LightsCounter::getCounts(LightType light_type) const
{
  switch (light_type)
  {
    case LightType::Omni: return omniCounts;
    case LightType::Spot: return spotCounts;
    default: G_ASSERT_FAIL("LightsCounter::getCounts - not supported light type"); return omniCounts;
  }
}

uint32_t LightsCounter::getClusteredLightsCount(LightType light_type) const { return getCounts(light_type).clustered; }

uint32_t LightsCounter::getFarLightsCount(LightType light_type) const { return getCounts(light_type).far; }
