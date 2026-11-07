//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/lights/lightsBase.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <generic/dag_align.h>
#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <math/dag_adjpow2.h>
#include <EASTL/optional.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_atomic_types.h>
#include <perfMon/dag_daProfilerString.h>

class LightsVisibilityChecker;

class LightsCounter final
{
public:
  LightsCounter(OmniLightsManager *omni_lights_manager, SpotLightsManager *spot_lights_manager);
  ~LightsCounter();

  void update(const LightsVisibilityChecker &visibility_checker);
  void reset();

  uint32_t getClusteredLightsCount(LightType light_type) const;
  uint32_t getFarLightsCount(LightType light_type) const;

private:
  static constexpr size_t BUCKET_SIZE = 256;
  static constexpr size_t BUCKETS_COUNT_FOR_OMNI_LIGHTS = dag::divide_align_up(OmniLightsManager::MAX_LIGHTS, BUCKET_SIZE);
  static constexpr size_t BUCKETS_COUNT_FOR_SPOT_LIGHTS = dag::divide_align_up(SpotLightsManager::MAX_LIGHTS, BUCKET_SIZE);
  static constexpr size_t BUCKETS_COUNT_FOR_ALL_LIGHTS = BUCKETS_COUNT_FOR_OMNI_LIGHTS + BUCKETS_COUNT_FOR_SPOT_LIGHTS;
  static constexpr size_t BUCKET_CONFIGURATIONS_COUNT = get_const_bigger_log2(BUCKETS_COUNT_FOR_ALL_LIGHTS) + 1;
  static constexpr size_t BUCKET_CONFIGURATION_ESTIMATIONS = 5;
  static constexpr int32_t BUCKET_LIGHTS_COUNT_THRESHOLD = 16;
  static constexpr uint32_t BUCKET_CONFIGURATIONS_REESTIMATION_PERIOD = 8192;
  static constexpr uint32_t BUCKET_CONFIGURATION_VALIDATION_UPDATES = 300;
  static constexpr uint64_t BUCKET_CONFIGURATION_DEGRADATION_PERCENT = 150;

  struct VisibleCounts final
  {
    uint32_t clustered = 0;
    uint32_t far = 0;
  };

  struct Bucket final
  {
    uint32_t estimationsAllocatedLightsCount = 0;
    uint32_t lastAllocatedLightsCount = 0;
    VisibleCounts visibleCounts{};
  };

  struct BucketConfiguration final
  {
    uint64_t executionTimesSum = 0;
    uint64_t selectedTimesSum = 0;
    uint32_t requiredEstimations = BUCKET_CONFIGURATION_ESTIMATIONS;
    uint32_t selectedUpdatesCount = 0;
  };

  struct CountJob final : public cpujobs::IJob
  {
    LightsCounter *counter = nullptr;
    const LightsVisibilityChecker *visibilityChecker = nullptr;
    uint32_t omniLightsCount = 0;
    uint32_t spotLightsCount = 0;
    size_t firstBucket = 0;
    size_t lastBucket = 0;
    dag::AtomicInteger<int> claimed = 0;
    bool handledByCurrentThread = false;

    bool tryRun();
    void doJob() override;
    const char *getJobName(bool &) const override { return DAPROFILER_STRING("lights_counter_job"); }
  };

  void countVisibleLights(const LightsVisibilityChecker &visibility_checker, uint32_t omni_lights_count, uint32_t spot_lights_count,
    size_t first_bucket, size_t last_bucket);

  template <LightType Type>
  void countVisibleLightsImpl(const LightsVisibilityChecker &visibility_checker, uint32_t lights_count, size_t first_bucket,
    size_t last_bucket);

  const VisibleCounts &getCounts(LightType light_type) const;
  static void handleClusteredOverflow(VisibleCounts &counts, uint32_t max_clustered_count);
  static bool isLightsCountChanged(uint32_t last_count, uint32_t estimations_count);
  void resetBucketConfigurations();
  size_t findOptimalBucketConfiguration();
  size_t findBucketConfigurationToEstimate();

  OmniLightsManager *omniLightsManager;
  SpotLightsManager *spotLightsManager;

  VisibleCounts omniCounts;
  VisibleCounts spotCounts;

  carray<Bucket, BUCKETS_COUNT_FOR_ALL_LIGHTS> buckets;
  carray<BucketConfiguration, BUCKET_CONFIGURATIONS_COUNT> bucketConfigurations;
  size_t currentBucketConfigurationIndex = 0;
  eastl::optional<size_t> optimalBucketConfigurationIndex;
  uint32_t updatesSinceEstimationsEnd = 0;

  carray<CountJob, BUCKETS_COUNT_FOR_ALL_LIGHTS> jobs;
  StaticTab<uint32_t, BUCKETS_COUNT_FOR_ALL_LIGHTS> submittedJobIndices;
};
