// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// fields that related to execution state in general

#include "util/tracked_state.h"
#include "driver.h"

namespace drv3d_vulkan
{

struct ExecutionStateStorage;

struct StateFieldExecutionStageCloser : TrackedStateFieldBase<false, false>
{
  struct Transit
  {
    ActiveExecutionStage from;
    ActiveExecutionStage to;
  };

  Transit data;

  template <typename StorageType>
  void reset(StorageType &)
  {
    data.from = ActiveExecutionStage::FRAME_BEGIN;
    data.to = ActiveExecutionStage::FRAME_BEGIN;
  }

  void set(Transit value) { data = value; }
  bool diff(Transit) { return true; }

  VULKAN_TRACKED_STATE_FIELD_CB_DEFENITIONS();
};

struct StateFieldActiveExecutionStage :
  // compute-compute changes should set dirty bit
  TrackedStateFieldBase<false, false>
{
  ActiveExecutionStage oldData;
  ActiveExecutionStage data;

  void reset(ExecutionStateStorage &)
  {
    data = ActiveExecutionStage::FRAME_BEGIN;
    oldData = ActiveExecutionStage::FRAME_BEGIN;
  }

  void set(ActiveExecutionStage value) { data = value; }
  bool diff(ActiveExecutionStage value) { return data != value; }

  const ActiveExecutionStage &getValueRO() const { return data; }

  VULKAN_TRACKED_STATE_FIELD_CB_NON_CONST_DEFENITIONS();
};

} // namespace drv3d_vulkan
