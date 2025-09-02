// Copyright (C) Gaijin Games KFT.  All rights reserved.

// global const buffer implementation
#include <drv/3d/dag_shaderConstants.h>
#include "globals.h"
#include "global_lock.h"
#include "frontend.h"
#include "global_const_buffer.h"

using namespace drv3d_vulkan;

int d3d::set_cs_constbuffer_size(int required_size)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  D3D_CONTRACT_ASSERTF(required_size >= 0, "Negative register count?");
  return Frontend::GCB.setComputeConstRegisterCount(required_size);
}

int d3d::set_vs_constbuffer_size(int required_size)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  D3D_CONTRACT_ASSERTF(required_size >= 0, "Negative register count?");
  return Frontend::GCB.setVertexConstRegisterCount(required_size);
}

bool d3d::set_const(unsigned stage, unsigned first, const void *data, unsigned count)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

  D3D_CONTRACT_ASSERT(stage < STAGE_MAX);
  Frontend::GCB.setConstRegisters(stage, first * SHADER_REGISTER_ELEMENTS,
    make_span((const uint32_t *)data, count * SHADER_REGISTER_ELEMENTS));
  return true;
}
