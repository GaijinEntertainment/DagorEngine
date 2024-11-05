// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <render/gpuVisibilityTest.h>

class ShadowsManager;

class RiShadowCullBboxesLoaderJob final : public cpujobs::IJob
{
  struct ShadowsOcclusionTestData
  {
    mat44f cullMatrix;
    ViewTransformData transform;
    int riCount;
  };

  dag::Vector<ShadowsOcclusionTestData> waitingTestsToPerform;
  WinCritSec testsQueueMutex;

  ShadowsManager &shadowsManager;

public:
  RiShadowCullBboxesLoaderJob(ShadowsManager &shadows_manager) : shadowsManager(shadows_manager) {}
  ~RiShadowCullBboxesLoaderJob() { reset(); }

  void addTest(mat44f_cref cull_matrix, const ViewTransformData &transform, int ri_count = -1);
  void start();
  void reset();

  virtual void doJob() override;
};
