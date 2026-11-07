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
    float texelSize; // the render thread rewrites the rtss settings
  };

  dag::Vector<ShadowsOcclusionTestData> waitingTestsToPerform;
  WinCritSec testsQueueMutex;

  ShadowsManager &shadowsManager;

public:
  RiShadowCullBboxesLoaderJob(ShadowsManager &shadows_manager) : shadowsManager(shadows_manager) {}
  ~RiShadowCullBboxesLoaderJob() { reset(); }

  void addTest(mat44f_cref cull_matrix, float texel_size);
  void start();
  void reset();

  const char *getJobName(bool &) const override { return DAPROFILER_STRING("ri_shadow_cull_bboxes_loader"); }
  virtual void doJob() override;
};
