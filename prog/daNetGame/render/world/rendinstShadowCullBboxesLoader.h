// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_cpuJobs.h>
#include <osApiWrappers/dag_critSec.h>
#include <render/gpuVisibilityTest.h>

class ShadowsManager;
class ToroidalStaticShadows;

class RiShadowCullBboxesLoaderJob final : public cpujobs::IJob
{
  struct ShadowsOcclusionTestData
  {
    mat44f cullMatrix;
    ViewTransformData transform;
  };

  dag::Vector<ShadowsOcclusionTestData> waitingTestsToPerform;
  WinCritSec testsQueueMutex;

  ShadowsManager &shadowsManager;
  void removeOutsideShadowRegions(const ToroidalStaticShadows &static_shadows);

public:
  RiShadowCullBboxesLoaderJob(ShadowsManager &shadows_manager) : shadowsManager(shadows_manager) {}
  ~RiShadowCullBboxesLoaderJob() { reset(); }

  void addTest(mat44f_cref cull_matrix, const ViewTransformData &transform);
  void start(const ToroidalStaticShadows &static_shadows);
  void reset();

  const char *getJobName(bool &) const override { return "ri_shadow_cull_bboxes_loader"; }
  virtual void doJob() override;
};
