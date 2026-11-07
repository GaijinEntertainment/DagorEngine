//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/shadersMetaData/dx11/compiled_shader_header.h>
#include <generic/dag_span.h>

namespace dx11
{

struct StageBytecodeRange
{
  uint32_t offset = 0;
  uint32_t size = 0; // size == 0 means the stage is absent
};

struct DecodedShader
{
  bool combined = false;
  uint8_t uavReadMask = 0;
  uint8_t uavWriteMask = 0;
  int32_t maxConstantRegUsed = -1;
  int32_t maxRtvUsed = -1;
  uint32_t hsTopology = 0; // D3D11_PRIMITIVE_TOPOLOGY, if none -- D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED == 0
  StageBytecodeRange vs, hs, ds, gs;
  uint32_t totalBytecodeSize = 0;
};

bool decode_metadata(dag::ConstSpan<uint8_t> metadata, DecodedShader &out);

inline uint32_t get_used_const_count(const auto &header) { return header.maxConstantRegUsed >= 0 ? header.maxConstantRegUsed + 1 : 0; }

} // namespace dx11
