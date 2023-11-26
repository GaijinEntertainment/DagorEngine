//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/string.h>
#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_lockSbuffer.h>


/** Multidraw Context
 *
 * This class incapsulates logic of filling multidraw buffers.
 * It implements draw call id/per draw parameters passing for different platforms and manages buffers required for it.
 */
template <typename PerDrawDataT>
class MultidrawContext
{
  /**
   * @brief Name for context that is used as a buffer name and as a prefix for helper buffers.
   */
  eastl::string name;
  /**
   * @brief Buffer for draw calls arguments.
   */
  UniqueBuf multidrawArguments;
  /**
   * @brief Buffer for per draw parameters.
   */
  UniqueBufHolder perDrawArgsBuffer;
  /**
   * @brief Number of draw calls that can be stored in buffer.
   */
  uint32_t allocatedDrawcallsInBuffer = 0;

  /**
   * @brief Extended draw call arguments structure.
   *
   * This structure is used for platforms that pass draw call id/per draw parameters using per draw root constants.
   * Currently it is used only for DX12.
   */
  struct ExtendedDrawIndexedIndirectArgs
  {
    uint32_t drawcallId;
    DrawIndexedIndirectArgs args;
  };

  /**
   * @brief Checks if extended draw call arguments structure is used.
   */
  static bool usesExtendedMultiDrawStruct() { return d3d::get_driver_code().is(d3d::dx12); }

  /**
   * @brief Returns size of draw call arguments structure for a current driver.
   */
  static uint32_t bytesCountPerDrawcall()
  {
    const uint32_t multiDrawArgsStructSize =
      usesExtendedMultiDrawStruct() ? sizeof(ExtendedDrawIndexedIndirectArgs) : sizeof(DrawIndexedIndirectArgs);
    return multiDrawArgsStructSize;
  }

  /**
   * @brief Returns size of draw call arguments structure in dwords for a current driver.
   */
  static uint32_t dwordsCountPerDrawcall()
  {
    const uint32_t multiDrawArgsStructSize = bytesCountPerDrawcall();
    G_ASSERT(multiDrawArgsStructSize % sizeof(uint32_t) == 0);
    return multiDrawArgsStructSize / sizeof(uint32_t);
  }

  /**
   * @brief Returns name for per draw parameters buffer.
   */
  eastl::string getPerDrawArgsBufferName() const { return name + "_perDrawArgsBuffer"; }

  /**
   * @brief Checks if per draw parameter could be stored instead of draw id. In this case we don't need per draw parameters buffer.
   */
  static bool needPerDrawParamsBuffer() { return sizeof(PerDrawDataT) != sizeof(uint32_t); }

public:
  /**
   * @brief Default constructor.
   * @param name must be unique for each context.
   */
  MultidrawContext(const char *name) : name(name) {}
  /**
   * @brief Default move constructor.
   */
  MultidrawContext(MultidrawContext &&) = default;
  /**
   * @brief Default move assignment operator.
   */
  MultidrawContext &operator=(MultidrawContext &&) = default;
  /**
   * @brief Removed copy constructor since we use unique buffer holders.
   */
  MultidrawContext(MultidrawContext &) = delete;
  /**
   * @brief Removed copy assignment operator since we use unique buffer holders.
   */
  MultidrawContext &operator=(MultidrawContext &) = delete;

  /**
   * @brief Type for a callback that is called for each draw call to fill draw call parameters.
   * We don't allow to fill startInstanceLocation because it is used for draw call id on some platforms.
   */
  using MultidrawParametersSetter = eastl::fixed_function<sizeof(void *),
    void(uint32_t draw_index, uint32_t &index_count_per_instance, uint32_t &instance_count, uint32_t &start_index_location,
      int32_t &base_vertex_location, PerDrawDataT &per_draw_data)>;

  /**
   * @brief Fills multidraw buffers.
   * @param drawcalls_count number of draw calls.
   * @param params_setter function that sets draw call parameters.
   *
   * This method iterates over locked buffers content and calls params_setter for each draw call to fill only allowed parameters of
   * drawcall. If the buffers are too small, it recreates them.
   */
  void fillBuffers(uint32_t drawcalls_count, const MultidrawParametersSetter &params_setter)
  {
    if (allocatedDrawcallsInBuffer < drawcalls_count || !multidrawArguments.getBuf())
    {
      allocatedDrawcallsInBuffer = drawcalls_count;
      multidrawArguments.close();
      multidrawArguments =
        dag::create_sbuffer(sizeof(uint32_t), allocatedDrawcallsInBuffer * dwordsCountPerDrawcall(), SBCF_INDIRECT, 0, name.c_str());
      if (needPerDrawParamsBuffer())
      {
        perDrawArgsBuffer.close();
        perDrawArgsBuffer = dag::create_sbuffer(sizeof(PerDrawDataT), allocatedDrawcallsInBuffer,
          SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES | SBCF_DYNAMIC, 0, getPerDrawArgsBufferName().c_str());
      }
    }

    eastl::variant<eastl::monostate, LockedBuffer<DrawIndexedIndirectArgs>, LockedBuffer<ExtendedDrawIndexedIndirectArgs>>
      multidrawArgs;
    bool bufferLocked = true;
    if (usesExtendedMultiDrawStruct())
    {
      multidrawArgs = lock_sbuffer<ExtendedDrawIndexedIndirectArgs>(multidrawArguments.getBuf(), 0, drawcalls_count, VBLOCK_DISCARD);
      bufferLocked = (bool)eastl::get<2>(multidrawArgs);
    }
    else
    {
      multidrawArgs = lock_sbuffer<DrawIndexedIndirectArgs>(multidrawArguments.getBuf(), 0, drawcalls_count, VBLOCK_DISCARD);
      bufferLocked = (bool)eastl::get<1>(multidrawArgs);
    }
    if (!bufferLocked)
    {
      logerr("Buffer %s data wasn't updated.", multidrawArguments.getBuf()->getBufName());
      return;
    }

    eastl::optional<LockedBuffer<PerDrawDataT>> perDrawArgs;
    if (needPerDrawParamsBuffer())
    {
      perDrawArgs.value() = lock_sbuffer<PerDrawDataT>(perDrawArgsBuffer.getBuf(), 0, drawcalls_count, VBLOCK_DISCARD);
      if (!perDrawArgs)
      {
        logerr("Buffer %s data wasn't updated.", perDrawArgsBuffer.getBuf()->getBufName());
        return;
      }
    }
    for (uint32_t i = 0; i < drawcalls_count; ++i)
    {
      DrawIndexedIndirectArgs &drawCall =
        multidrawArgs.index() == 1 ? eastl::get<1>(multidrawArgs)[i] : eastl::get<2>(multidrawArgs)[i].args;
      uint32_t &drawcallId = multidrawArgs.index() == 1 ? drawCall.startInstanceLocation : eastl::get<2>(multidrawArgs)[i].drawcallId;
      PerDrawDataT &perDrawData = perDrawArgs ? perDrawArgs.value()[i] : reinterpret_cast<PerDrawDataT &>(drawcallId);
      params_setter(i, drawCall.indexCountPerInstance, drawCall.instanceCount, drawCall.startIndexLocation,
        drawCall.baseVertexLocation, perDrawData);
      if (perDrawArgs)
        drawcallId = i;
    }
  }
  /**
   * @brief Closes buffers.
   */
  void close()
  {
    multidrawArguments.close();
    perDrawArgsBuffer.close();
    allocatedDrawcallsInBuffer = 0;
  }
  /**
   * @brief Returns buffer for draw call arguments.
   */
  Sbuffer *getBuffer() const { return multidrawArguments.getBuf(); }

  /**
   * @brief Stride for draw call arguments buffer to use it in a draw call.
   */
  static uint32_t getStride() { return bytesCountPerDrawcall(); }
};
