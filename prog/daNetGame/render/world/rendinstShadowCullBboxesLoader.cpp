// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rendinstShadowCullBboxesLoader.h"

#include "shadowsManager.h"
#include <util/dag_threadPool.h>
#include <perfMon/dag_statDrv.h>

void RiShadowCullBboxesLoaderJob::addTest(mat44f_cref cull_matrix, const ViewTransformData &transform)
{
  WinAutoLock lock(testsQueueMutex);
  waitingTestsToPerform.push_back({cull_matrix, transform});
}

void RiShadowCullBboxesLoaderJob::removeOutsideShadowRegions(const ToroidalStaticShadows &static_shadows)
{
  WinAutoLock lock(testsQueueMutex);
  TMatrix cascadeGlobTm = static_shadows.getLightViewProj(static_shadows.cascadesCount() - 1);
  mat44f invCascadeGlobTm, tmpTm;
  v_mat44_make_from_43cu(tmpTm, cascadeGlobTm.m[0]);
  v_mat44_inverse43_to44(invCascadeGlobTm, tmpTm);
  waitingTestsToPerform.erase(eastl::remove_if(waitingTestsToPerform.begin(), waitingTestsToPerform.end(),
                                [&invCascadeGlobTm](const ShadowsOcclusionTestData &regionData) {
                                  return !ToroidalStaticShadows::isRegionInsideCascade(invCascadeGlobTm, regionData.transform);
                                }),
    waitingTestsToPerform.end());
}

void RiShadowCullBboxesLoaderJob::start(const ToroidalStaticShadows &static_shadows)
{
  if (!interlocked_acquire_load(this->done) || waitingTestsToPerform.empty())
    return;
  removeOutsideShadowRegions(static_shadows);
  if (!waitingTestsToPerform.empty())
    threadpool::add(this);
}

void RiShadowCullBboxesLoaderJob::reset()
{
  threadpool::wait(this);
  WinAutoLock lock(testsQueueMutex);
  waitingTestsToPerform.clear();
}

void RiShadowCullBboxesLoaderJob::doJob()
{
  if (!shadowsManager.getVisibilityTestAvailableSpace())
    return;

  dag::Vector<ShadowsOcclusionTestData> currentTestsToPerform;
  {
    WinAutoLock lock(testsQueueMutex);
    currentTestsToPerform.swap(waitingTestsToPerform);
  }

  for (auto &testData : currentTestsToPerform)
  {
    int riLeftCount = shadowsManager.gatherAndTryAddRiForShadowsVisibilityTest(testData.cullMatrix, testData.transform);
    if (riLeftCount > 0)
      addTest(testData.cullMatrix, testData.transform);
  }
}
