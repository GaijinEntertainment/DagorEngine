// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_lock.h>
#include <runtime/runtime.h>
#include <render/daFrameGraph/daFG.h>

struct TestRuntime
{
  TestRuntime() { dafg::Runtime::startup(); }

  void executeGraph()
  {
    d3d::GpuAutoLock lock{};
    dafg::Runtime::get().runNodes();
  }

  ~TestRuntime() { dafg::Runtime::shutdown(); }
};