// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>
#include "driver.h"
#include "device_memory.h"

namespace drv3d_vulkan
{
struct GlobalConstBuffer
{
  struct DirtyState
  {
    enum Bits
    {
      // has to be the last to validate against invalid stages
      COMPUTE_CONST_REGISTERS,
      PIXEL_CONST_REGISTERS,
      VERTEX_CONST_REGISTERS,
      // no raytrace const registers, intentionally not supported (yet?)

      COUNT,
      INVALID = COUNT
    };
    typedef eastl::bitset<COUNT> Type;
  };

  // returns set
  bool markDirty(DirtyState::Bits bit, bool set)
  {
    DirtyState::Type n;
    n.set(bit, set);
    dirtyState |= n;
    return set;
  }

  void onFrontFlush() { dirtyState = ~DirtyState::Type(0); }

  DirtyState::Type dirtyState = ~DirtyState::Type(0);

  // current sizes of the register space sections (eg what needs uploading)
  uint32_t registerSpaceSizes[STAGE_MAX] = {MIN_COMPUTE_CONST_REGISTERS, FRAGMENT_SHADER_REGISTERS, VERTEX_SHADER_MIN_REGISTERS};
  // one big chunk to provide memory for register backing
  uint32_t totalRegisterSpace[(MAX_COMPUTE_CONST_REGISTERS + VERTEX_SHADER_MAX_REGISTERS + FRAGMENT_SHADER_REGISTERS) *
                              SHADER_REGISTER_ELEMENTS] = {};

  uint32_t *getRegisterSectionStart(uint32_t stage)
  {
    G_STATIC_ASSERT(STAGE_CS == 0);
    G_STATIC_ASSERT(STAGE_PS == 1);
    G_STATIC_ASSERT(STAGE_VS == 2);
    // segmentation [0-4095:cs][4096-8191:vs][8192-8263:ps]
    uint32_t offset = ((stage & 2) << 11)    // turns 2 into 4096
                      + ((stage & 1) << 13); // turns 1 into 8192
    return &totalRegisterSpace[offset * SHADER_REGISTER_ELEMENTS];
  }

  bool registerMemoryUpdate(uint32_t stage, uint32_t offset, dag::ConstSpan<uint32_t> blob)
  {
    auto data = getRegisterSectionStart(stage) + offset;
    // search for first difference
    auto range = eastl::mismatch(blob.begin(), blob.end(), data);
    bool modified = range.first != blob.end();
    // copy over until we reach the end
    while (range.first != blob.end())
    {
      *range.second++ = *range.first++;
    }
    return modified;
  }

  uint32_t setComputeConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(nextPowerOfTwo(cnt), MIN_COMPUTE_CONST_REGISTERS, MAX_COMPUTE_CONST_REGISTERS);
    else
      cnt = MIN_COMPUTE_CONST_REGISTERS; // TODO update things to allow 0 (eg shader can tell how many it needs)
    markDirty(DirtyState::COMPUTE_CONST_REGISTERS, registerSpaceSizes[STAGE_CS] < cnt);
    registerSpaceSizes[STAGE_CS] = cnt;
    return cnt;
  }
  uint32_t setVertexConstRegisterCount(uint32_t cnt)
  {
    if (cnt)
      cnt = clamp<uint32_t>(nextPowerOfTwo(cnt), VERTEX_SHADER_MIN_REGISTERS, VERTEX_SHADER_MAX_REGISTERS);
    else
      cnt = VERTEX_SHADER_MIN_REGISTERS; // TODO update things to allow 0 (eg shader can tell how many it needs)
    markDirty(DirtyState::VERTEX_CONST_REGISTERS, registerSpaceSizes[STAGE_VS] < cnt);
    registerSpaceSizes[STAGE_VS] = cnt;
    return cnt;
  }
  void setConstRegisters(int stage, uint32_t offset, dag::ConstSpan<uint32_t> blob)
  {
    auto ds = static_cast<DirtyState::Bits>(DirtyState::COMPUTE_CONST_REGISTERS + stage);
    G_ASSERT(ds < DirtyState::INVALID);
    markDirty(ds, registerMemoryUpdate(stage, offset, blob));
  }

  template <typename ContextClass>
  void setGlobalCbToStage(ContextClass &ctx, uint32_t stage)
  {
    ctx.setConstRegisterBuffer(
      ctx.uploadToDeviceFrameMem(registerSpaceSizes[stage] * SHADER_REGISTER_SIZE, getRegisterSectionStart(stage)),
      (ShaderStage)stage);
  }

  template <typename ContextClass>
  void flushGraphics(ContextClass &ctx)
  {
    DirtyState::Type unchangedMask;
    // keep compute dirty flags
    // TODO: move those defs somewhere else
    unchangedMask.set(DirtyState::COMPUTE_CONST_REGISTERS);

    if (dirtyState.test(DirtyState::VERTEX_CONST_REGISTERS))
      setGlobalCbToStage(ctx, STAGE_VS);
    if (dirtyState.test(DirtyState::PIXEL_CONST_REGISTERS))
      setGlobalCbToStage(ctx, STAGE_PS);

    dirtyState &= unchangedMask;
  }

  template <typename ContextClass>
  void flushCompute(ContextClass &ctx)
  {
    DirtyState::Type computeMask;
    computeMask.set(DirtyState::COMPUTE_CONST_REGISTERS);

    if (dirtyState.test(DirtyState::COMPUTE_CONST_REGISTERS))
      setGlobalCbToStage(ctx, STAGE_CS);

    dirtyState &= ~computeMask;
  }
};
} // namespace drv3d_vulkan
