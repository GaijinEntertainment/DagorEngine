// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/lights/lightsVisibilityChecker.h>
#include <render/lights/lightsPartition.hlsli>
#include <debug/dag_assert.h>
#include "smallLights.h"


LightsVisibilityChecker::LightsVisibilityChecker() = default;
LightsVisibilityChecker::LightsVisibilityChecker(TestParameters &&test_parameters, Config &&cfg) :
  testParams(eastl::move(test_parameters)), config(eastl::move(cfg))
{
  if (config.cacheResults)
  {
    testCache = eastl::make_unique<CacheType>();
  }
}

uint32_t LightsVisibilityChecker::getCacheLightIndex(LightType light_type, uint32_t light_id)
{
  uint32_t lightSlot = 0;
  switch (light_type)
  {
    case LightType::Omni: lightSlot = light_id; break;
    case LightType::Spot: lightSlot = light_id + OmniLightsManager::MAX_LIGHTS; break;
    default: G_ASSERT_FAIL("LightsVisibilityChecker::getCacheLightIndex - not supported light type"); break;
  }

  return lightSlot * BITS_PER_LIGHT;
}

bool LightsVisibilityChecker::tryGetCachedResult(LightType light_type, uint32_t light_id, TestResult &cached_result) const
{
  if (testCache)
  {
    const auto cacheIndex = getCacheLightIndex(light_type, light_id);
    const auto bit0 = static_cast<NumericResultType>(testCache->test(cacheIndex));
    const auto bit1 = static_cast<NumericResultType>(testCache->test(cacheIndex + 1));

    cached_result = static_cast<TestResult>(bit0 | (bit1 << static_cast<NumericResultType>(1)));
    return cached_result != TestResult::Unknown;
  }

  cached_result = TestResult::Unknown;
  return false;
}

void LightsVisibilityChecker::tryStoreResultToCache(LightType light_type, uint32_t light_id, TestResult actual_result) const
{
  if (!testCache)
    return;

  const auto cacheIndex = getCacheLightIndex(light_type, light_id);
  const auto value = static_cast<NumericResultType>(actual_result);
  testCache->set(cacheIndex, (value & static_cast<NumericResultType>(1)) != 0);
  testCache->set(cacheIndex + 1, (value & static_cast<NumericResultType>(2)) != 0);
}

void LightsVisibilityChecker::moveCacheData(LightsVisibilityChecker &&from, LightsVisibilityChecker &to, bool clear_cache)
{
  G_ASSERT(!(static_cast<bool>(from.testCache) ^ to.config.cacheResults));

  to.testCache = eastl::move(from.testCache);
  if (clear_cache && to.testCache)
    to.testCache->reset();
}

LightsVisibilityChecker::LightsVisibilityChecker(TestParameters &&test_parameters, LightsVisibilityChecker &&checker) :
  testParams(eastl::move(test_parameters)), config(eastl::move(checker.config))
{
  moveCacheData(eastl::move(checker), *this, true);
}

LightsVisibilityChecker::LightsVisibilityChecker(LightsVisibilityChecker &&checker) noexcept :
  testParams(eastl::move(checker.testParams)), config(eastl::move(checker.config))
{
  moveCacheData(eastl::move(checker), *this, false);
}

LightsVisibilityChecker &LightsVisibilityChecker::operator=(LightsVisibilityChecker &&checker) noexcept
{
  if (this != &checker)
  {
    testParams = eastl::move(checker.testParams);
    config = eastl::move(checker.config);
    moveCacheData(eastl::move(checker), *this, false);
  }

  return *this;
}

LightsVisibilityChecker::~LightsVisibilityChecker() = default;


LightsVisibilityChecker::TestResult LightsVisibilityChecker::test(LightType light_type, uint32_t light_id) const
{
  TestResult actualResult = TestResult::Unknown;
  if (tryGetCachedResult(light_type, light_id, actualResult))
  {
    G_ASSERT(actualResult != TestResult::Unknown);
    return actualResult;
  }

  switch (light_type)
  {
    case LightType::Omni: actualResult = testImpl<OmniLightsManager>(config.omniLightsManager, light_id); break;
    case LightType::Spot: actualResult = testImpl<SpotLightsManager>(config.spotLightsManager, light_id); break;
    default: G_ASSERT_FAIL("LightsVisibilityChecker::test - not supported light type"); break;
  }

  G_ASSERT(actualResult != TestResult::Unknown);
  tryStoreResultToCache(light_type, light_id, actualResult);
  return actualResult;
}

template <typename LightsManagerT>
LightsVisibilityChecker::TestResult LightsVisibilityChecker::testImpl(const LightsManagerT *lights_manager, uint32_t light_id) const
{
  G_ASSERT(lights_manager);

  if (!lights_manager->isLightValid(light_id))
    return TestResult::Invisible;

  typename LightsManagerT::MaskType requireAnyMask{};

  if constexpr (eastl::is_same_v<LightsManagerT, OmniLightsManager>)
  {
    requireAnyMask = testParams.omniRequireAnyMask;
  }
  else if constexpr (eastl::is_same_v<LightsManagerT, SpotLightsManager>)
  {
    requireAnyMask = testParams.spotRequireAnyMask;
  }

  if (requireAnyMask && !(requireAnyMask & lights_manager->getLightMask(light_id)))
    return TestResult::Invisible;


  vec4f lightPosRad = lights_manager->getBoundingSphere(light_id);
  vec3f rad = v_splat_w(lightPosRad);

  if (!testParams.frustum.testSphereB(lightPosRad, rad))
    return TestResult::Invisible;

  if (testParams.occlusion)
  {
    if constexpr (eastl::is_same_v<LightsManagerT, OmniLightsManager>)
    {
      if (testParams.occlusion->isOccludedSphere(lightPosRad, rad))
        return TestResult::Invisible;
    }
    else if constexpr (eastl::is_same_v<LightsManagerT, SpotLightsManager>)
    {
      if (testParams.occlusion->isOccludedBox(lights_manager->getBoundingBox(light_id)))
        return TestResult::Invisible;
    }
  }

  vec4f length_sq = v_length3_sq(v_sub(testParams.cameraPos, lightPosRad));
  vec3f cutoffDistSqV = testParams.cutoffDistSq > 0 ? v_splats(testParams.cutoffDistSq) : V_C_INF;
  if (v_test_vec_x_gt(length_sq, cutoffDistSqV))
    return TestResult::Invisible;


  vec3f radScaled = rad;
  if constexpr (eastl::is_same_v<LightsManagerT, OmniLightsManager>)
  {
    radScaled = v_mul_x(v_splats(OMNI_LIGHT_BOUND_SPHERE_RADIUS_SCALE), rad);
  }

  vec4f res = v_add_x(v_sub_x(v_dot3_x(lightPosRad, testParams.znearPlane), radScaled), v_splat_w(testParams.znearPlane));
  vec4f camInSphereVec = v_sub_x(length_sq, v_mul(rad, rad));

#if _TARGET_SIMD_SSE
  bool intersectsNear = _mm_movemask_ps(res) & 1;
  bool camInSphere = _mm_movemask_ps(camInSphereVec) & 1;
#else
  bool intersectsNear = v_test_vec_x_lt_0(res);
  bool camInSphere = v_test_vec_x_lt_0(camInSphereVec);
#endif

  const bool small = lights_manager->getShadowId(light_id) == INVALID_SHADOW_VOLUME_ID &&
                     is_viewed_small(lightPosRad, length_sq, testParams.markSmallLightsAsFarLimit);

  const bool isFar = (intersectsNear || small) && !camInSphere;
  return isFar ? TestResult::Far : TestResult::Clustered;
}

const LightsVisibilityChecker::TestParameters &LightsVisibilityChecker::getTestParams() const { return testParams; }

bool LightsVisibilityChecker::testVisibility(LightType light_type, uint32_t light_id) const
{
  const auto testResult = test(light_type, light_id);
  G_ASSERT_RETURN(testResult != TestResult::Unknown, false);
  return (testResult == TestResult::Clustered) || (testResult == TestResult::Far);
}
