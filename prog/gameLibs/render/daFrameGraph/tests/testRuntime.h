// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <runtime/runtime.h>
#include <render/daFrameGraph/daFG.h>

struct TestRuntime
{
  TestRuntime() { dafg::Runtime::startup(); }

  void executeGraph() { dafg::Runtime::get().runNodes(); }

  ~TestRuntime() { dafg::Runtime::shutdown(); }
};