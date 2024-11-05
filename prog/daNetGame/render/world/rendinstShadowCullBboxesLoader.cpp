// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "rendinstShadowCullBboxesLoader.h"

#include "shadowsManager.h"
#include <util/dag_threadPool.h>
#include <perfMon/dag_statDrv.h>

void RiShadowCullBboxesLoaderJob::addTest(mat44f_cref cull_matrix, const ViewTransformData &transform, int ri_count)
{
  WinAutoLock lock(testsQueueMutex);
  waitingTestsToPerform.push_back({cull_matrix, transform, ri_count});
}

void RiShadowCullBboxesLoaderJob::start()
{
  if (!interlocked_acquire_load(this->done) || waitingTestsToPerform.empty())
    return;

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
  TIME_PROFILE(ri_shadow_cull_bboxes_loader);

  dag::Vector<ShadowsOcclusionTestData> currentTestsToPerform;
  {
    WinAutoLock lock(testsQueueMutex);
    currentTestsToPerform.swap(waitingTestsToPerform);
  }

  for (auto &testData : currentTestsToPerform)
  {
    int availableSpace = shadowsManager.getVisibilityTestAvailableSpace();
    if (testData.riCount <= availableSpace)
    {
      int riLeftCount = shadowsManager.gatherAndTryAddRiForShadowsVisibilityTest(testData.cullMatrix, testData.transform);
      if (riLeftCount > 0)
        addTest(testData.cullMatrix, testData.transform, riLeftCount);
    }
    else
      addTest(testData.cullMatrix, testData.transform, testData.riCount);
  }
}
