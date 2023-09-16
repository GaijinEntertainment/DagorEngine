//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_overrideStateId.h>
#include <util/dag_hash.h>
#include <string.h>

namespace shaders
{

struct StencilState
{
  uint8_t func : 4, fail : 4, zFail : 4, pass : 4; // zFunc:4, stencil* can be 3 bits each
  uint8_t readMask = 255, writeMask = 255;
  StencilState()
  {
    func = 1;                // CMPF_NEVER
    pass = zFail = fail = 1; // STNCLOP_KEEP
    readMask = writeMask = 255;
  }
  StencilState(uint8_t stencil_func, uint8_t stencil_fail, uint8_t z_fail, uint8_t stencil_z_pass, uint8_t write_mask,
    uint8_t read_mask)
  {
    set(stencil_func, stencil_fail, z_fail, stencil_z_pass, write_mask, read_mask);
  }

  void set(uint8_t stencil_func, uint8_t stencil_fail, uint8_t z_fail, uint8_t stencil_z_pass, uint8_t write_mask, uint8_t read_mask)
  {
    func = stencil_func;
    fail = stencil_fail;
    zFail = z_fail;
    pass = stencil_z_pass;
    writeMask = write_mask;
    readMask = read_mask;
    G_ASSERT(func && fail && zFail && pass);
  }
};

struct OverrideState
{
  enum StateBits
  {
    Z_TEST_DISABLE = 1 << 0,
    Z_WRITE_DISABLE = 1 << 1,
    Z_BOUNDS_ENABLED = 1 << 2,
    Z_CLAMP_ENABLED = 1 << 3,
    Z_FUNC = 1 << 4,
    Z_BIAS = 1 << 5,  // both biases
    STENCIL = 1 << 6, // all stencil
    BLEND_OP = 1 << 7,
    BLEND_OP_A = 1 << 8,
    BLEND_SRC_DEST = 1 << 9,
    BLEND_SRC_DEST_A = 1 << 10,
    CULL_NONE = 1 << 11, // superceeds flipcull
    FLIP_CULL = 1 << 12,
    FORCED_SAMPLE_COUNT = 1 << 13,
    CONSERVATIVE = 1 << 14,
    SCISSOR_ENABLED = 1 << 15,
    ALPHA_TO_COVERAGE = 1 << 16,
    Z_WRITE_ENABLE = 1 << 17,
  };
  uint32_t bits = 0;
  uint8_t zFunc; // zFunc can be 3 bits
  uint8_t forcedSampleCount;
  uint8_t blendOp = 0;  // 3 bits
  uint8_t blendOpA = 0; // 3 bits
  uint8_t sblend = 0;
  uint8_t dblend = 0;
  uint8_t sblenda = 0;
  uint8_t dblenda = 0;
  uint32_t colorWr = 0xFFFFFFFF; // always &=
  StencilState stencil;

  float zBias = 0, slopeZBias = 0;

  bool isOn(uint32_t mask) const { return (bits & mask) != 0; }
  void set(uint32_t mask) { bits |= mask; }
  void reset(uint32_t mask) { bits &= ~mask; }
  OverrideState()
  {
    memset(this, 0, sizeof(OverrideState));
    colorWr = 0xFFFFFFFF;
  }
  OverrideState(const OverrideState &a) { memcpy(this, &a, sizeof(*this)); }

  OverrideState &operator=(const OverrideState &a)
  {
    if (this != &a)
      memcpy(this, &a, sizeof(*this));
    return *this;
  }

  bool operator==(const OverrideState &s) const { return memcmp(this, &s, sizeof(OverrideState)) == 0; }
  bool operator!=(const OverrideState &s) const { return !(*this == s); }

  void validate()
  {
    if (!isOn(Z_BIAS))
      zBias = slopeZBias = 0.f;
    if (!isOn(FORCED_SAMPLE_COUNT))
      forcedSampleCount = 0;
    if (!isOn(Z_FUNC))
      zFunc = 0;
    if (!isOn(BLEND_OP))
      blendOp = 0;
    if (!isOn(STENCIL))
      memset(&stencil, 0, sizeof(stencil));
  }
};

void set_stencil_ref(uint8_t ref); // in the moment, we use this instead of absent d3d::set_stencil_ref. To be replaced

// void add(OverrideStateId);//add another override. Can potentially create implicit override, which is combination of currently set
// override and this one. void remove(OverrideStateId);//removes override.

}; // namespace shaders
