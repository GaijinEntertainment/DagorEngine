// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "synced_command_store.h"

void drv3d_dx12::SyncedCommandStore::ExecutionSemaphoreSynced::reset()
{
  activePrimaryCommandsCount.store(0, std::memory_order_relaxed);
}

void drv3d_dx12::SyncedCommandStore::ExecutionSemaphoreSynced::onPushPrimaryCommand()
{
  int32_t expected = 0;
  while (!activePrimaryCommandsCount.compare_exchange_weak(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
  {
    expected = 0;
    ::wait(activePrimaryCommandsCount, 1, std::memory_order_acquire);
  }
}

void drv3d_dx12::SyncedCommandStore::ExecutionSemaphoreSynced::onConsumePrimaryCommand()
{
  activePrimaryCommandsCount.fetch_sub(1, std::memory_order_release);
  notify_one(activePrimaryCommandsCount);
  G_ASSERT(activePrimaryCommandsCount.load(std::memory_order_relaxed) >= 0);
}

#if !DX12_FIXED_EXECUTION_MODE
void drv3d_dx12::SyncedCommandStore::setExecutionMode(CommandExecutionMode mode)
{
  if (mode != CommandExecutionMode::CONCURRENT_SYNCED)
    return;

  syncExecutionCommands = true;
  executionSemaphoreSynced.reset();
}
#endif
