// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_lock.h>
#include <runtime/runtime.h>
#include <render/daFrameGraph/daFG.h>

namespace shader_vars_mock
{
void reset();
}

struct TestRuntime
{
  TestRuntime()
  {
    shader_vars_mock::reset();
    dafg::Runtime::startup();
  }

  void executeGraph()
  {
    d3d::GpuAutoLock lock{};
    dafg::Runtime::get().runNodes();
  }

  ~TestRuntime()
  {
    dafg::Runtime::shutdown();
    shader_vars_mock::reset();
  }
};
