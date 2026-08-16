// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_lock.h>
#include <runtime/runtime.h>
#include <render/daFrameGraph/daFG.h>
#include <render/metronome.h>

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

    keepAlive = dafg::register_node("metronome_test_keepalive", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      registry.executionHas(dafg::SideEffects::External);
      return [] {};
    });

    dafg::metronome::update();
  }

  void executeGraph()
  {
    d3d::GpuAutoLock lock{};
    dafg::Runtime::get().runNodes();
  }

  ~TestRuntime()
  {
    keepAlive = {};
    dafg::Runtime::shutdown();
    shader_vars_mock::reset();
  }

  dafg::NodeHandle keepAlive;
};
