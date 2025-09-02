//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/fixed_function.h>
#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/variant.h>

#include <3d/dag_resPtr.h>
#include <3d/dag_ringDynBuf.h>
#include <drv/3d/dag_draw.h>


// This is workaround for OOM on xbox
#define USE_STAGING_MULTIDRAW_BUF _TARGET_XBOXONE

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
#if USE_STAGING_MULTIDRAW_BUF
  /**
   * @brief Buffer for draw calls arguments.
   */
  UniqueBuf multidrawArguments;
  /**
   * @brief Staging buffer for multidrawArguments buffer.
   */
  RingDynamicSB multidrawArgsStagingBuffer;
#else
  /**
   * @brief Buffer for draw calls arguments.
   */
  RingDynamicSB multidrawArguments;
#endif
  /**
   * @brief Buffer for per draw parameters.
   */
  UniqueBufHolder perDrawArgsBuffer;
  /**
   * @brief Number of draw calls that can be stored in buffer.
   */
  uint32_t allocatedDrawcallsInBuffer = 0;

  /**
   * @brief Offset in buffer for next draw call.
   */
  uint32_t actualStart = 0;

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
#if _TARGET_PC_WIN
#define CONSTEXPR_EXT_MULTIDRAW
  static inline uint8_t bytesCountPerDrawcall = 0;
  static bool usesExtendedMultiDrawStruct()
  {
    if (DAGOR_UNLIKELY(!bytesCountPerDrawcall))
      bytesCountPerDrawcall =
        d3d::get_driver_code().is(d3d::dx12) ? sizeof(ExtendedDrawIndexedIndirectArgs) : sizeof(DrawIndexedIndirectArgs);
    static_assert(sizeof(ExtendedDrawIndexedIndirectArgs) != sizeof(DrawIndexedIndirectArgs));
    return bytesCountPerDrawcall == sizeof(ExtendedDrawIndexedIndirectArgs);
  }
#elif _TARGET_XBOX
#define CONSTEXPR_EXT_MULTIDRAW constexpr
  static constexpr uint8_t bytesCountPerDrawcall = sizeof(ExtendedDrawIndexedIndirectArgs);
  static constexpr bool usesExtendedMultiDrawStruct() { return true; } // DX12
#else
#define CONSTEXPR_EXT_MULTIDRAW constexpr
  static constexpr uint8_t bytesCountPerDrawcall = sizeof(DrawIndexedIndirectArgs);
  static constexpr bool usesExtendedMultiDrawStruct() { return false; }
#endif

  /**
   * @brief Returns name for per draw parameters buffer.
   */
  eastl::string getPerDrawArgsBufferName() const { return name + "_perDrawArgsBuffer"; }

#if USE_STAGING_MULTIDRAW_BUF
  /**
   * @brief Returns name for staging buffer.
   */
  eastl::string getMultidrawStagingBufferName() const { return name + "_staging"; }
#endif

  /**
   * @brief Checks if per draw parameter could be stored instead of draw id. In this case we don't need per draw parameters buffer.
   */
  static constexpr bool needPerDrawParamsBuffer() { return sizeof(PerDrawDataT) != sizeof(uint32_t); }

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
   * @brief Executor for multidraw calls.
   *
   * This class is used to pass multidraw buffers to a draw call. It could be constructed only by MultidrawContext.
   * It is used to hide multidraw buffers from a user.
   */
  class MultidrawRenderExecutor
  {
    friend class MultidrawContext;
    /**
     * @brief Pointer to a context that is used for rendering.
     */
    const MultidrawContext *context;
    /**
     * @brief Default constructors and operator= are deleted.
     */
    MultidrawRenderExecutor() = delete;
    MultidrawRenderExecutor(MultidrawRenderExecutor &&) = delete;
    MultidrawRenderExecutor &operator=(MultidrawRenderExecutor &&) = delete;
    MultidrawRenderExecutor(MultidrawRenderExecutor &) = delete;
    MultidrawRenderExecutor &operator=(MultidrawRenderExecutor &) = delete;

  public:
    /**
     * @brief Constructor.
     * @param context reference to a context that is used for rendering. If context is a nullptr, render method does nothing.
     */
    MultidrawRenderExecutor(const MultidrawContext *context) : context(context) {}
    /**
     * @brief Renders draw calls.
     * @param primitive_type type of primitive.
     * @param first_drawcall index of first draw call in the buffer.
     * @param drawcalls_count number of draw calls to execute.
     *
     * This method renders draw calls using multidraw indirect buffers.
     */
    void render(uint32_t primitive_type, uint32_t first_drawcall, uint32_t drawcalls_count) const
    {
      if (!context)
        return;
#if USE_STAGING_MULTIDRAW_BUF
      d3d::multi_draw_indexed_indirect(primitive_type, context->multidrawArguments.getBuf(), drawcalls_count,
        context->bytesCountPerDrawcall, first_drawcall * context->bytesCountPerDrawcall);
#else
      d3d::multi_draw_indexed_indirect(primitive_type, context->multidrawArguments.getRenderBuf(), drawcalls_count,
        context->bytesCountPerDrawcall, (first_drawcall + context->actualStart) * context->bytesCountPerDrawcall);
#endif
    }
  };

  /**
   * @brief Fills multidraw buffers.
   * @param drawcalls_count number of draw calls.
   * @param set_cb function that sets draw call parameters.
   * @return executor that could be used to render draw calls.
   *
   * This method iterates over locked buffers content and calls params_setter for each draw call to fill only allowed parameters of
   * drawcall. If the buffers are too small, it recreates them.
   */
  template <typename T>
  MultidrawRenderExecutor fillBuffers(uint32_t drawcalls_count, const T &set_cb)
  {
    CONSTEXPR_EXT_MULTIDRAW bool extMultiDraw = usesExtendedMultiDrawStruct();

#if USE_STAGING_MULTIDRAW_BUF
    RingDynamicSB &multidrawArgsBuf = multidrawArgsStagingBuffer;
#else
    RingDynamicSB &multidrawArgsBuf = multidrawArguments;
#endif
    if (allocatedDrawcallsInBuffer < drawcalls_count || !multidrawArgsBuf.getRenderBuf())
    {
      const uint32_t CHUNK_SIZE = 2048;
      allocatedDrawcallsInBuffer = (drawcalls_count + CHUNK_SIZE - 1) / CHUNK_SIZE * CHUNK_SIZE;
#if USE_STAGING_MULTIDRAW_BUF
      multidrawArguments.close();
      multidrawArguments = dag::create_sbuffer(sizeof(uint32_t), bytesCountPerDrawcall * allocatedDrawcallsInBuffer / sizeof(uint32_t),
        SBCF_INDIRECT, 0, name.c_str());
      multidrawArgsStagingBuffer.close();
      multidrawArgsStagingBuffer.init(allocatedDrawcallsInBuffer, bytesCountPerDrawcall, sizeof(uint32_t), SBCF_DYNAMIC, 0,
        getMultidrawStagingBufferName().c_str());
#else
      multidrawArguments.close();
      multidrawArguments.init(allocatedDrawcallsInBuffer, bytesCountPerDrawcall, sizeof(uint32_t), SBCF_INDIRECT | SBCF_DYNAMIC, 0,
        name.c_str());
#endif
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
    if CONSTEXPR_EXT_MULTIDRAW (extMultiDraw)
    {
      multidrawArgs = multidrawArgsBuf.lockBufferAs<ExtendedDrawIndexedIndirectArgs>(drawcalls_count, actualStart);
      bufferLocked = (bool)eastl::get<2>(multidrawArgs);
    }
    else
    {
      multidrawArgs = multidrawArgsBuf.lockBufferAs<DrawIndexedIndirectArgs>(drawcalls_count, actualStart);
      bufferLocked = (bool)eastl::get<1>(multidrawArgs);
    }
    if (!bufferLocked)
    {
      logerr("Buffer %s data wasn't updated.", multidrawArgsBuf.getRenderBuf()->getBufName());
      return MultidrawRenderExecutor(nullptr);
    }

    eastl::optional<LockedBuffer<PerDrawDataT>> perDrawArgs;
    if constexpr (needPerDrawParamsBuffer())
    {
      perDrawArgs.emplace(perDrawArgsBuffer.getBuf(), 0, drawcalls_count, VBLOCK_DISCARD);
      if (!perDrawArgs)
      {
        logerr("Buffer %s data wasn't updated.", perDrawArgsBuffer.getBuf()->getBufName());
        return MultidrawRenderExecutor(nullptr);
      }
    }
    for (uint32_t i = 0; i < drawcalls_count; ++i)
    {
      auto &dc = extMultiDraw ? eastl::get<2>(multidrawArgs)[i].args : eastl::get<1>(multidrawArgs)[i];
      dc.startInstanceLocation = 0;
      uint32_t &drawcallId = extMultiDraw ? eastl::get<2>(multidrawArgs)[i].drawcallId : dc.startInstanceLocation;
      PerDrawDataT &perDrawData = needPerDrawParamsBuffer() ? perDrawArgs.value()[i] : reinterpret_cast<PerDrawDataT &>(drawcallId);
      set_cb(i, dc.indexCountPerInstance, dc.instanceCount, dc.startIndexLocation, dc.baseVertexLocation, perDrawData);
      if constexpr (needPerDrawParamsBuffer())
        drawcallId = i;
    }

#if USE_STAGING_MULTIDRAW_BUF
    extMultiDraw ? eastl::get<2>(multidrawArgs).close() : eastl::get<1>(multidrawArgs).close();
    multidrawArgsStagingBuffer.getRenderBuf()->copyTo(multidrawArguments.getBuf(), 0, actualStart * bytesCountPerDrawcall,
      drawcalls_count * bytesCountPerDrawcall);
#endif

    return MultidrawRenderExecutor(this);
  }
  /**
   * @brief Closes buffers.
   */
  void close()
  {
    multidrawArguments.close();
#if USE_STAGING_MULTIDRAW_BUF
    multidrawArgsStagingBuffer.close();
#endif
    perDrawArgsBuffer.close();
    allocatedDrawcallsInBuffer = 0;
  }
};

#undef CONSTEXPR_EXT_MULTIDRAW
