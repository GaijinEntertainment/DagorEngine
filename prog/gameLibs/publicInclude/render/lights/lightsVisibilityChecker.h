//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/bitset.h>
#include <EASTL/type_traits.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <render/lights/omniLightsManager.h>
#include <render/lights/spotLightsManager.h>
#include <render/lights/lightsBase.h>

class LightsVisibilityChecker final
{

public:
  struct TestParameters final
  {
    Frustum frustum{};
    Occlusion *occlusion = nullptr;
    vec4f znearPlane = v_zero();
    float markSmallLightsAsFarLimit = 0.0f;
    vec3f cameraPos = v_zero();
    OmniLightMaskType omniRequireAnyMask = OmniLightMaskType::OMNI_LIGHT_MASK_NONE;
    SpotLightMaskType spotRequireAnyMask = SpotLightMaskType::SPOT_LIGHT_MASK_NONE;
    float cutoffDistSq = 0.0f;
  };

  struct Config final
  {
    OmniLightsManager *omniLightsManager = nullptr;
    SpotLightsManager *spotLightsManager = nullptr;
    bool cacheResults = false;
  };

  LightsVisibilityChecker();
  LightsVisibilityChecker(TestParameters &&test_parameters, Config &&cfg);
  LightsVisibilityChecker(TestParameters &&test_parameters, LightsVisibilityChecker &&checker);
  ~LightsVisibilityChecker();

  LightsVisibilityChecker(LightsVisibilityChecker &&checker) noexcept;
  LightsVisibilityChecker &operator=(LightsVisibilityChecker &&checker) noexcept;

  LightsVisibilityChecker(const LightsVisibilityChecker &) = delete;
  LightsVisibilityChecker &operator=(const LightsVisibilityChecker &) = delete;

  enum class TestResult : uint8_t
  {
    Unknown = 0,
    Invisible = 1,
    Far = 2,
    Clustered = 3,
  };

  TestResult test(LightType light_type, uint32_t light_id) const;
  bool testVisibility(LightType light_type, uint32_t light_id) const;
  const TestParameters &getTestParams() const;

private:
  static constexpr size_t BITS_PER_LIGHT = 2;
  static constexpr size_t CACHE_BIT_SIZE = (OmniLightsManager::MAX_LIGHTS + SpotLightsManager::MAX_LIGHTS) * BITS_PER_LIGHT;

  using NumericResultType = eastl::underlying_type_t<TestResult>;
  using CacheType = eastl::bitset<CACHE_BIT_SIZE>;

  template <typename LightsManager>
  TestResult testImpl(const LightsManager *lights_manager, uint32_t light_id) const;

  static void moveCacheData(LightsVisibilityChecker &&from, LightsVisibilityChecker &to, bool clear_cache);

  static uint32_t getCacheLightIndex(LightType light_type, uint32_t light_id);

  bool tryGetCachedResult(LightType light_type, uint32_t light_id, TestResult &cached_result) const;
  void tryStoreResultToCache(LightType light_type, uint32_t light_id, TestResult actual_result) const;

  TestParameters testParams;
  Config config;
  eastl::unique_ptr<CacheType> testCache;
};
