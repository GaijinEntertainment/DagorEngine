//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/string.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>
#include <generic/dag_span.h>

enum class FillOnGPU
{
  No,
  Yes
};

template <typename BaseArgsType>
class IndirectDrawcallsBuffer
{
  UniqueBuf buffer;
  uint32_t allocatedDrawcallsInBuffer = 0;
  eastl::string bufferName;

  static bool usesPackedDrawCallID() { return d3d::get_driver_code() == _MAKE4C('DX12'); }

  static bool packDrawCallIdAsOffset()
  {
    return d3d::get_driver_code() == _MAKE4C('PS4') || d3d::get_driver_code() == _MAKE4C('PS5') ||
           d3d::get_driver_code() == _MAKE4C('VULK');
  }

  static uint32_t dwordsCountPerDrawcall()
  {
    G_STATIC_ASSERT(sizeof(BaseArgsType) % sizeof(uint32_t) == 0);
    const uint32_t BASE_DWORDS_COUNT = sizeof(BaseArgsType) / sizeof(uint32_t);
    return usesPackedDrawCallID() ? BASE_DWORDS_COUNT + 1 : BASE_DWORDS_COUNT;
  }

  struct ExtendedDrawIndexedIndirectArgs
  {
    uint32_t drawcallId;
    BaseArgsType args;
  };

public:
  IndirectDrawcallsBuffer() = default;
  IndirectDrawcallsBuffer(eastl::string name) : bufferName(eastl::move(name)) {}
  IndirectDrawcallsBuffer(IndirectDrawcallsBuffer &&) = default;
  IndirectDrawcallsBuffer &operator=(IndirectDrawcallsBuffer &&) = default;
  IndirectDrawcallsBuffer(IndirectDrawcallsBuffer &) = delete;
  IndirectDrawcallsBuffer &operator=(IndirectDrawcallsBuffer &) = delete;

  void fillBuffer(dag::ConstSpan<BaseArgsType> drawcalls_data, uint32_t drawid_offset = 0, FillOnGPU fill_on_gpu = FillOnGPU::No)
  {
    if (allocatedDrawcallsInBuffer < drawcalls_data.size() || !buffer.getBuf())
    {
      allocatedDrawcallsInBuffer = drawcalls_data.size();
      buffer.close();
      buffer = dag::create_sbuffer(sizeof(uint32_t), allocatedDrawcallsInBuffer * dwordsCountPerDrawcall(),
        fill_on_gpu == FillOnGPU::No ? SBCF_INDIRECT : SBCF_UA_INDIRECT, 0, bufferName.c_str());
    }

    const uint32_t LOCK_FLAGS = VBLOCK_WRITEONLY;
    if (usesPackedDrawCallID())
    {
      if (auto data = lock_sbuffer<ExtendedDrawIndexedIndirectArgs>(buffer.getBuf(), 0, drawcalls_data.size(), LOCK_FLAGS))
      {
        for (uint32_t i = 0; i < drawcalls_data.size(); ++i)
        {
          data[i].drawcallId = i + drawid_offset;
          data[i].args = drawcalls_data[i];
        }
      }
      else
      {
        logerr("Buffer %s data wasn't updated.", bufferName.c_str());
      }
    }
    else if (packDrawCallIdAsOffset())
    {
      if (auto data = lock_sbuffer<BaseArgsType>(buffer.getBuf(), 0, drawcalls_data.size(), LOCK_FLAGS))
      {
        for (uint32_t i = 0; i < drawcalls_data.size(); ++i)
        {
          G_ASSERT(drawcalls_data[i].startInstanceLocation == 0);
          data[i] = drawcalls_data[i];
          data[i].startInstanceLocation = i + drawid_offset;
        }
      }
      else
      {
        logerr("Buffer %s data wasn't updated.", bufferName.c_str());
      }
    }
    else
      buffer->updateData(0, data_size(drawcalls_data), drawcalls_data.data(), LOCK_FLAGS);
  }

  void close()
  {
    buffer.close();
    allocatedDrawcallsInBuffer = 0;
  }

  Sbuffer *getBuffer() const { return buffer.getBuf(); }

  static uint32_t getStride() { return dwordsCountPerDrawcall() * sizeof(uint32_t); }

  static uint32_t getArgsOffset() { return usesPackedDrawCallID() ? sizeof(uint32_t) : 0; }
};
