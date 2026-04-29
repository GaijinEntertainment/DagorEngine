// Copyright (C) Gaijin Games KFT.  All rights reserved.

// conditional rendering implementation
#include <drv/3d/dag_driver.h>
#include "globals.h"
#include "frontend.h"
#include "global_lock.h"
#include "query_pools.h"
#include "pipeline_state.h"
#include "device_context.h"
#include "query_pools.h"
#include "backend/cmd/query.h"

using namespace drv3d_vulkan;

bool d3d::begin_survey(int name)
{
  if (-1 == name)
    return false;

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
  CmdBeginSurvey cmd = Globals::surveys.start(name);
  Frontend::State::pipe.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({cmd.pool, cmd.index});
  Globals::ctx.dispatchCmdNoLock(cmd);
  return true;
}

void d3d::end_survey(int name)
{
  if (-1 == name)
    return;

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  OSSpinlockScopedLock frontLock(Globals::ctx.getFrontLock());
  Frontend::State::pipe.set<StateFieldGraphicsQueryState, GraphicsQueryState, FrontGraphicsState>({});
  Globals::ctx.dispatchCmdNoLock(Globals::surveys.end(name));
}

int d3d::create_predicate()
{
  if (Globals::cfg.has.conditionalRender)
    return Globals::surveys.add();
  return -1;
}

void d3d::free_predicate(int name)
{
  if (-1 != name)
    Globals::surveys.remove(name);
}

void d3d::begin_conditional_render(int name)
{
  if (-1 == name)
    return;

  D3D_CONTRACT_ASSERTF_RETURN(name >= 0, , "vlukan: driver received an invalid survey name '%d' in begin_conditional_rendering", name);

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  auto &pipeState = Frontend::State::pipe;
  pipeState.set<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
    Globals::surveys.startCondRender(static_cast<uint32_t>(name)));
}

void d3d::end_conditional_render(int name)
{
  if (-1 == name)
    return;

  D3D_CONTRACT_ASSERTF_RETURN(name >= 0, , "vlukan: driver received an invalid survey name '%d' in end_conditional_rendering", name);

  VERIFY_GLOBAL_LOCK_ACQUIRED();
  auto &pipeState = Frontend::State::pipe;
  pipeState.set<StateFieldGraphicsConditionalRenderingState, ConditionalRenderingState, FrontGraphicsState>(
    ConditionalRenderingState{});
}
