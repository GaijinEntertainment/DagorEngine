// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cachesim.h>

#include <CacheSim/CacheSim.h>
#include <debug/dag_assert.h>
#include <debug/dag_log.h>

static enum class State { Uninitialized, Ready, Scheduled, Active } current_state = State::Uninitialized;

static void ensureReady()
{
  if (current_state == State::Uninitialized)
  {
    CacheSim::Init();
    current_state = State::Ready;
  }
}

void ScopedCacheSim::schedule()
{
  ensureReady();
  current_state = State::Scheduled;
}

ScopedCacheSim::ScopedCacheSim()
{
  ensureReady();

  G_ASSERTF(current_state != State::Active, "Only one capture is allowed at a time.");

  if (current_state == State::Scheduled)
  {
    CacheSim::ResetThreadCoreMappings();
    CacheSim::SetThreadCoreMapping(CacheSim::GetCurrentThreadId(), 0);
    CacheSim::StartCapture();
    current_state = State::Active;
  }
}

ScopedCacheSim::~ScopedCacheSim()
{
  if (current_state == State::Active)
  {
    CacheSim::EndCapture(true);
    current_state = State::Ready;
  }
}
