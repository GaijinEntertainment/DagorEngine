//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_baseDef.h>
#include <util/dag_stdint.h>

namespace dx11
{

constexpr uint32_t SIMPLE_METADATA_SIZE = 12;

constexpr uint32_t COMBINED_SHADERS_IDENT = _MAKE4C('DX11');
constexpr uint32_t COMBINED_HEADER_OFFSET = SIMPLE_METADATA_SIZE;
constexpr uint32_t COMBINED_METADATA_SIZE = 32;

constexpr uint32_t HS_TOPOLOGY_SHIFT = 24;
constexpr uint32_t COMBINED_LEN_MASK = (1u << HS_TOPOLOGY_SHIFT) - 1;

struct SimpleHeader
{
  uint32_t bytecodeByteSize;
  int32_t maxConstantRegUsed; // -1 means none
  int8_t maxRtvUsed;          // PS only, -1 means none
  // bit i is set when the shader loads (read mask) or stores (write mask) u<i>, an atomic sets both; a clear bit is no
  // access or an unknown one
  uint8_t uavReadMask;
  uint8_t uavWriteMask;
  // 0 when the masks are valid; without UAV access the three mask bytes hold the sign extension of maxRtvUsed, so code
  // that reads bytes 8..11 as an int32 maxRtvUsed still gets the right value for every shader that has no UAV access
  uint8_t noUavMasks;
};

static_assert(sizeof(SimpleHeader) == SIMPLE_METADATA_SIZE);

inline uint8_t get_uav_read_mask(const SimpleHeader &header) { return header.noUavMasks ? 0 : header.uavReadMask; }
inline uint8_t get_uav_write_mask(const SimpleHeader &header) { return header.noUavMasks ? 0 : header.uavWriteMask; }

// sets the three mask bytes, maxRtvUsed must already be set
inline void set_uav_masks(SimpleHeader &header, uint8_t read_mask, uint8_t write_mask)
{
  if (read_mask | write_mask)
  {
    header.uavReadMask = read_mask;
    header.uavWriteMask = write_mask;
    header.noUavMasks = 0;
  }
  else
    header.uavReadMask = header.uavWriteMask = header.noUavMasks = header.maxRtvUsed < 0 ? 0xFF : 0;
}

struct CombinedHeader
{
  uint32_t magic;
  uint32_t vsLenDwords;
  uint32_t hsLenDwordsAndTopology;
  uint32_t dsLenDwords;
  uint32_t gsLenDwords;
};

static_assert(sizeof(SimpleHeader) + sizeof(CombinedHeader) == COMBINED_METADATA_SIZE);

} // namespace dx11
