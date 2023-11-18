#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_drv3d.h>
#include <perfMon/dag_graphStat.h>
#include <debug/dag_log.h>

#include "drv_returnAddrStore.h"


template <typename T, bool /*REPORT_ERRORS*/ = true>
class GenericBufferErrorHandler
{
public:
  static void errorDiscardingStageMemory(const char *name)
  {
    logerr("%s: Error while discarding staging memory of buffer '%s'", T::driverName(), name);
  }
  static void errorAllocatingTemporaryMemory(const char *name)
  {
    logerr("%s: Error while allocating temporary memory for buffer '%s'", T::driverName(), name);
  }
  static void errorUnknownStagingMemoryType(const char *name)
  {
    logerr("%s: Buffer '%s' has insufficient information to select proper staging memory type, "
           "falling back to read/write",
      T::driverName(), name);
  }
  static void errorLockWithoutPrevoiusUnlock(const char *name)
  {
    logerr("%s: Buffer '%s' unlocked without previous successful lock", T::driverName(), name);
  }
  static void errorLockOffsetIsTooLarge(const char *name, uint32_t offset, uint32_t size)
  {
    logerr("%s: Buffer '%s' lock with offset of %u is larger than buffer size %u", T::driverName(), name, offset, size);
  }
  static void errorLockSizeIsTooLarge(const char *name, uint32_t offset, uint32_t range, uint32_t size)
  {
    logerr("%s: Buffer '%s' lock with offset of %u and size of %u (%u) is larger than buffer size "
           "%u",
      T::driverName(), name, offset, range, offset + range, size);
  }
  static void errorLockWithInvalidLockingFlags(const char *name, uint32_t flags)
  {
    logerr("%s: Buffer '%s' locked with invalid combination of locking flags 0x%X", T::driverName(), name, flags);
  }
  static void errorDiscardingBuffer(const char *name) { logerr("%s: Error while discarding buffer '%s'", T::driverName(), name); }
  static void errorUnlockWithoutPreviousLock(const char *name)
  {
    logerr("%s: Buffer '%s' unlocked without previous lock", T::driverName(), name);
  }
  static void errorUnexpectedUseCase(const char *name) { logerr("%s: Unexpected use case for buffer '%s'", T::driverName(), name); }
  static void errorInvalidStructSizeForRawView(const char *name, uint32_t struct_size)
  {
    logerr("%s: Buffer '%s' uses invalid struct size of %u for buffer with RAW views", T::driverName(), name, struct_size);
  }
  static void errorAllocationOfBufferFailed(const char *name, uint32_t size, uint32_t struct_size, uint32_t cflags)
  {
    logerr("%s: Allocation of buffer '%s' failed, requested size '%u' bytes with a structure size "
           "of '%u' bytes and creation flags of '0x%08X'",
      T::driverName(), name, size, struct_size, cflags);
  }
  static void errorStructuredIndirect(const char *name)
  {
    logerr("indirect buffer can't be structured one in DX11, check <%s>", name);
  }

  static void errorDiscardWithoutPointer(const char *name)
  {
    logerr("%s: Discarded buffer '%s' without providing output pointer", name);
  }

  static void errorFormatUsedWithInvalidUsageFlags(const char *name)
  {
    logerr("%s: Can't create buffer '%s' with texture format which is Structured, Vertex, Index or "
           "Raw",
      T::driverName(), name);
  }
};

template <typename T>
class GenericBufferErrorHandler<T, false>
{
public:
  static void errorDiscardingStageMemory(const char *) {}
  static void errorAllocatingTemporaryMemory(const char *) {}
  static void errorUnknownStagingMemoryType(const char *) {}
  static void errorLockWithoutPrevoiusUnlock(const char *) {}
  static void errorLockOffsetIsTooLarge(const char *, uint32_t, uint32_t) {}
  static void errorLockSizeIsTooLarge(const char *, uint32_t, uint32_t, uint32_t) {}
  static void errorLockWithInvalidLockingFlags(const char *, uint32_t) {}
  static void errorDiscardingBuffer(const char *) {}
  static void errorUnlockWithoutPreviousLock(const char *) {}
  static void errorUnexpectedUseCase(const char *) {}
  static void errorInvalidStructSizeForRawView(const char *, uint32_t) {}
  static void errorAllocationOfBufferFailed(const char *, uint32_t, uint32_t, uint32_t) {}
  static void errorStructuredIndirect(const char *) {}
  static void errorDiscardWithoutPointer(const char *) {}
  static void errorFormatUsedWithInvalidUsageFlags(const char *) {}
};

template <typename T, bool /*REPORT_WARNINGS*/ = true>
class GenericBufferWarningHandler
{
public:
  static void warnPerformanceBlockingBufferReadBack(const char *name)
  {
    logwarn("%s: Performance: Blocking read back of buffer '%s'", T::driverName(), name);
  }
  static void warnPerformanceTemporaryMemoryForReadBack(const char *name)
  {
    logwarn("%s: Performance: Using temporary memory as staging memory for read back of buffer "
            "'%s'",
      T::driverName(), name);
  }
  static void warnPerformanceNotConsumedAsyncReadBack(const char *name)
  {
    logwarn("%s: Performance: Previous async read back was not consumed '%s'", T::driverName(), name);
  }
  static void warnPerformanceReadBackOfUnchangedResource(const char *name)
  {
    logwarn("%s: Performance: Repeated read back of unchanged buffer '%s'", T::driverName(), name);
  }
  static void warnPerformanceHostCopyToBufferWithTemporary(const char *name)
  {
    logwarn("%s: Performance: Host copy of buffer '%s' uploaded with temporary memory", T::driverName(), name);
  }
  static void warnBadStructureSize(const char *name, uint32_t stuct_size)
  {
    logwarn("%s: Performance: The structure size of %u of buffer '%s' has a hardware unfriendly "
            "size and probably results in degraded performance. For some platforms it might "
            "requires the shader compiler to restructure the structure type to avoid layout "
            "violations and on other platforms it results in wasted memory as the memory manager "
            "has to insert extra padding to align the buffer properly.",
      T::driverName(), stuct_size, name);
  }
  static void warnUsageAsyncWithoutReadBackRequested(const char *name)
  {
    logwarn("%s: Usage: Requested async access to buffer '%s' but without read request, read "
            "locking flag missing?",
      T::driverName(), name);
  }
};

template <typename T>
class GenericBufferWarningHandler<T, false>
{
public:
  static void warnPerformanceBlockingBufferReadBack(const char *) {}
  static void warnPerformanceTemporaryMemoryForReadBack(const char *) {}
  static void warnPerformanceNotConsumedAsyncReadBack(const char *) {}
  static void warnPerformanceReadBackOfUnchangedResource(const char *) {}
  static void warnPerformanceHostCopyToBufferWithTemporary(const char *) {}
  static void warnBadStructureSize(const char *, uint32_t) {}
  static void warnUsageAsyncWithoutReadBackRequested(const char *) {}
};

// GenericBufferMemoryArchitecture for NUMA systems
// This implements local copy and staging memory systems for Sbuffer implementation for systems
// where CPU and GPU memory is in a possible NUMA configuration.
template <typename T, bool /*IS_UMA*/ = false>
class GenericBufferMemoryArchitecture
{
  eastl::unique_ptr<uint8_t[]> localBuffer;
  typename T::StagingMemoryType stagingMemory = T::getNullStagingMemory();

protected:
  bool allocateHostCopyMemory(uint32_t buf_flags, uint32_t size)
  {
    if (!(buf_flags & SBCF_DYNAMIC) &&
        ((buf_flags & SBCF_BIND_MASK) == SBCF_BIND_VERTEX || (buf_flags & SBCF_BIND_MASK) == SBCF_BIND_INDEX))
    {
      localBuffer = eastl::make_unique<uint8_t[]>(size);
      return true;
    }
    return false;
  }
  void freeHostCopyMemory() { localBuffer.reset(); }
  bool hasHostCopy() const { return static_cast<bool>(localBuffer); }
  uint8_t *getHostCopyPointer(uint32_t offset) { return localBuffer.get() + offset; }

  uint8_t *getStagingMemoryPointer(uint32_t offset) { return T::getMemoryPointer(stagingMemory, offset); }
  template <typename R, bool E>
  bool allocateStagingMemory(uint32_t buf_flags, uint32_t size, const char *name, GenericBufferErrorHandler<R, E> &reporter)
  {
    G_UNUSED(reporter); // for whatever reason VC throws a warning that reporter is unreferenced, which is obviously incorrect
    if (T::isValidMemory(stagingMemory))
      T::freeMemory(stagingMemory);

    if (hasHostCopy())
      return false;

    if (0 == (buf_flags & (SBCF_DYNAMIC | SBCF_USAGE_READ_BACK)))
      return false;

    if (0 != (buf_flags & (SBCF_CPU_ACCESS_READ | SBCF_USAGE_READ_BACK)))
    {
      if (0 != (buf_flags & SBCF_CPU_ACCESS_WRITE))
      {
        stagingMemory = T::allocateReadWriteStagingMemory(size);
      }
      else
      {
        stagingMemory = T::allocateReadOnlyStagingMemory(size);
      }
    }
    else if (0 != (buf_flags & SBCF_CPU_ACCESS_WRITE))
    {
      stagingMemory = T::allocateWriteOnlyStagingMemory(size);
    }
    else
    {
      reporter.errorUnknownStagingMemoryType(name);
      stagingMemory = T::allocateReadWriteStagingMemory(size);
    }
    return true;
  }
  template <typename R, bool E>
  bool discardStagingMemory(uint32_t buf_flags, uint32_t size, const char *name, GenericBufferErrorHandler<R, E> &reporter)
  {
    G_UNUSED(reporter); // for whatever reason VC throws a warning that reporter is unreferenced, which is obviously incorrect
    if (!T::isValidMemory(stagingMemory))
      return false;

    if (0 != (buf_flags & (SBCF_CPU_ACCESS_READ | SBCF_USAGE_READ_BACK)))
    {
      if (0 != (buf_flags & SBCF_CPU_ACCESS_WRITE))
      {
        stagingMemory = T::discardReadWriteStagingMemory(stagingMemory, size);
      }
      else
      {
        stagingMemory = T::discardReadOnlyStagingMemory(stagingMemory, size);
      }
    }
    else if (0 != (buf_flags & SBCF_CPU_ACCESS_WRITE))
    {
      stagingMemory = T::discardWriteOnlyStagingMemory(stagingMemory, size);
    }
    else
    {
      reporter.errorUnknownStagingMemoryType(name);
      stagingMemory = T::discardReadWriteStagingMemory(stagingMemory, size);
    }
    return true;
  }
  void freeStagingMemory()
  {
    if (T::isValidMemory(stagingMemory))
    {
      T::freeMemory(stagingMemory);
      stagingMemory = T::getNullStagingMemory();
    }
  }
  bool hasStagingMemory() const { return T::isValidMemory(stagingMemory); }
  void resetStagingMemory() { stagingMemory = T::getNullStagingMemory(); }
  void uploadStagingMemoryToBuffer(uint32_t stage_offset, typename T::BufferReferenceType buffer, uint32_t buffer_offset,
    uint32_t size)
  {
    T::uploadBuffer(stagingMemory, buffer, stage_offset, buffer_offset, size);
  }
  void readBackBufferToStagingMemory(typename T::BufferReferenceType buffer, uint32_t buffer_offset, uint32_t stage_offset,
    uint32_t size)
  {
    T::readBackBuffer(buffer, stagingMemory, buffer_offset, stage_offset, size);
  }
  void blockingReadBackBufferToStagingMemory(typename T::BufferReferenceType buffer, uint32_t buffer_offset, uint32_t stage_offset,
    uint32_t size)
  {
    T::blockingReadBackBuffer(buffer, stagingMemory, buffer_offset, stage_offset, size);
  }
  void invalidateStagingMemory(uint32_t offset, uint32_t size) { T::invalidateMemory(stagingMemory, offset, size); }
};

// GenericBufferMemoryArchitecture for UMA systems
// This implements local copy and staging memory systems for Sbuffer implementation for systems
// where CPU and GPU memory is a guaranteed UMA configuration.
// UMA allows a few optimizations in this case.
// Staging and local copy is not needed as the CPU can access the GPU memory and can do
// everything a staging or a local copy would be needed to implement certain use cases.
template <typename T>
class GenericBufferMemoryArchitecture<T, true>
{
protected:
  constexpr bool allocateHostCopyMemory(uint32_t, uint32_t) const { return false; }
  constexpr void freeHostCopyMemory() const {}
  constexpr bool hasHostCopy() const { return false; }
  constexpr uint8_t *getHostCopyPointer(uint32_t) const { return nullptr; }

  constexpr uint8_t *getStagingMemoryPointer(uint32_t) const { return nullptr; }
  template <typename T>
  constexpr bool allocateStagingMemory(uint32_t, uint32_t, const char *, T &) const
  {
    return false;
  }
  template <typename T>
  constexpr bool discardStagingMemory(uint32_t, uint32_t, const char *, T &) const
  {
    return false;
  }
  constexpr void freeStagingMemory() const {}
  constexpr bool hasStagingMemory() const { return false; }
  constexpr void resetStagingMemory() const {}
  constexpr void uploadStagingMemoryToBuffer(uint32_t, typename T::BufferReferenceType, uint32_t, uint32_t) const {}
  constexpr void readBackBufferToStagingMemory(typename T::BufferReferenceType, uint32_t, uint32_t, uint32_t) const {}
  constexpr void blockingReadBackBufferToStagingMemory(typename T::BufferReferenceType, uint32_t, uint32_t, uint32_t) const {}
  constexpr void invalidateStagingMemory(uint32_t, uint32_t) const {}
};

// GenericBufferReloadImplementation for systems with reload support.
template <bool /*HAS_RELOAD_SUPPORT*/ = true>
class GenericBufferReloadImplementation
{
  struct ReloadDataHandler
  {
    void operator()(Sbuffer::IReloadData *rd)
    {
      if (rd)
        rd->destroySelf();
    }
  };

  typedef eastl::unique_ptr<Sbuffer::IReloadData, ReloadDataHandler> ReloadDataPointer;

  ReloadDataPointer reloadInfo;

protected:
  bool updateReloadInfo(Sbuffer::IReloadData *rld)
  {
    reloadInfo.reset(rld);
    return true;
  }
  bool hasReloadInfo() const { return static_cast<bool>(reloadInfo); }
  bool executeReload(Sbuffer *self)
  {
    if (reloadInfo)
    {
      reloadInfo->reloadD3dRes(self);
      return true;
    }
    return false;
  }
};

// GenericBufferReloadImplementation for systems without reload support.
template <>
class GenericBufferReloadImplementation<false>
{
protected:
  bool updateReloadInfo(Sbuffer::IReloadData *) { return false; }
  bool hasReloadInfo() const { return false; }
  bool executeReload(Sbuffer *) { return false; }
};

template <typename T>
class GenericSbufferImplementation final : public GenericBufferMemoryArchitecture<T, T::IS_UMA>,
                                           public GenericBufferReloadImplementation<T::HAS_RELOAD_SUPPORT>,
                                           protected GenericBufferErrorHandler<T, T::REPORT_ERRORS>,
                                           protected GenericBufferWarningHandler<T, T::REPORT_WARNINGS>,
                                           public Sbuffer
{
  using BufferReloadImplementation = GenericBufferReloadImplementation<T::HAS_RELOAD_SUPPORT>;
  using BufferMemoryArchitecture = GenericBufferMemoryArchitecture<T, T::IS_UMA>;
  using BufferErrorHandler = GenericBufferErrorHandler<T, T::REPORT_ERRORS>;
  using BufferWarningHandler = GenericBufferWarningHandler<T, T::REPORT_WARNINGS>;

  static constexpr uint8_t STATUS_READ_BACK_IN_PROGRESS = 1u << 0;
  static constexpr uint8_t STATUS_DEVICE_HAS_WRITTEN_TO = 1u << 1;
  static constexpr uint8_t STATUS_IS_INITIALIZED = 1u << 2;
  static constexpr uint8_t STATUS_IS_STREAM_BUFFER = 1u << 3;

  uint32_t bufSize = 0;
  uint32_t bufFlags = 0;
  uint32_t lockOffset = 0;
  uint32_t lockSize = 0;
  uint16_t structSize = 0;
  uint8_t lastLockFlags = 0;
  uint8_t statusFlags = 0;
  typename T::BufferType buffer = T::getNullBuffer();
  typename T::TemporaryMemoryType temporaryMemory = T::getNullTemporaryMemory();
  typename T::ViewFormatType viewFormat = T::getDefaultViewFormat();

  static bool is_stream_buffer(uint32_t flags)
  {
    // TODO: have a proper flag and not this hack...
    constexpr uint32_t required_flags = SBCF_DYNAMIC | SBCF_FRAMEMEM;
    if (required_flags == (required_flags & flags))
    {
      return T::allowsStreamBuffer(flags);
    }
    return false;
  }

  static uint32_t estimateSize(uint32_t buffer_size, uint32_t structure_size, uint32_t discard_count, uint32_t cflags)
  {
    auto estimatedSize = (buffer_size + structure_size - 1);
    estimatedSize = estimatedSize - (estimatedSize % structure_size);
    return max<uint32_t>(estimatedSize * discard_count, T::minBufferSize(cflags));
  }

  void compose(const char *buf_name)
  {
    if (is_stream_buffer(bufFlags))
    {
      statusFlags |= STATUS_IS_STREAM_BUFFER;
      // stream buffers use temp memory exclusively
      return;
    }
    auto idc = getInitialDiscardCount();
    auto buf = T::createBuffer(bufSize, structSize, idc, getMemoryClass(), bufFlags, buf_name);

    if (!T::isValidBuffer(buf))
    {
      BufferErrorHandler::errorAllocationOfBufferFailed(buf_name, bufSize, structSize, bufFlags);
      return;
    }

    adopt(eastl::move(buf), buf_name);

    if (T::REPORT_MEMORY_SIZE_TO_MANAGER)
    {
      auto sz = T::getBufferSize(buffer);
      // update memory usage at the texture manager
      tql::on_buf_changed(true, tql::sizeInKb(sz));
    }
  }

  void adopt(typename T::BufferType &&buf, const char *buf_name)
  {
    buffer = eastl::move(buf);

    if (!T::isMapable(buffer))
    {
      BufferMemoryArchitecture::allocateStagingMemory(bufFlags, bufSize, buf_name, *this);
    }

    T::addGenericView(buffer, structSize, viewFormat);

    // views have to be done separately
    if ((SBCF_BIND_SHADER_RES | SBCF_BIND_UNORDERED) & bufFlags)
    {
      if (needsRawView())
        T::addRawResourceView(buffer);
      else if (needsStructuredView())
        T::addStructuredResourceView(buffer, structSize);
      else
        T::addShaderResourceView(buffer, viewFormat);
    }
    if (SBCF_BIND_UNORDERED & bufFlags)
    {
      if (needsRawView())
        T::addRawUnorderedView(buffer);
      else if (needsStructuredView())
        T::addStructuredUnorderedView(buffer, structSize);
      else
        T::addUnorderedAccessView(buffer, viewFormat);
    }

    if (bufFlags & SBCF_ZEROMEM)
    {
      unsigned char *data = nullptr;
      if (lock(0, bufSize, (void **)&data, VBLOCK_WRITEONLY) && data)
      {
        memset(data, 0, bufSize);
        unlock();
      }
      bufferIsInitialized();
    }
  }

  bool isInitialized() const { return 0 != (statusFlags & STATUS_IS_INITIALIZED); }

  void bufferIsInitialized() { statusFlags |= STATUS_IS_INITIALIZED; }

  bool requiresReadBack() const
  {
    if (T::TRACK_GPU_WRITES)
      return 0 != (statusFlags & STATUS_DEVICE_HAS_WRITTEN_TO);
    // Obvious case, using UAV to write to it
    if (0 != (bufFlags & SBCF_BIND_UNORDERED))
    {
      return true;
    }
    // Tricky case, where buffer might be copied to, this tries to mimic DX11 implementation behavior
    if (structSize)
    {
      return !isDynamicBuffer();
    }
    return false;
  }

  void immediateReadBack()
  {
    if (T::TRACK_GPU_WRITES)
      statusFlags &= ~STATUS_DEVICE_HAS_WRITTEN_TO;
  }

  void beginReadBackProgress()
  {
    if (T::TRACK_GPU_WRITES)
      statusFlags = (statusFlags & ~STATUS_DEVICE_HAS_WRITTEN_TO) | STATUS_READ_BACK_IN_PROGRESS;
    else
      statusFlags |= STATUS_READ_BACK_IN_PROGRESS;
  }

  void endReadBackProgress() { statusFlags &= ~(STATUS_READ_BACK_IN_PROGRESS); }

  bool isReadBackInProgress() const { return 0 != (statusFlags & STATUS_READ_BACK_IN_PROGRESS); }

  bool needsRawView() const { return 0 != (bufFlags & SBCF_MISC_ALLOW_RAW); }

  bool needsStructuredView() const { return 0 != (bufFlags & SBCF_MISC_STRUCTURED); }

  static bool bufferLockedForRead(uint32_t lock_flags) { return lock_flags & VBLOCK_READONLY; }
  static bool bufferLockedForWrite(uint32_t lock_flags) { return lock_flags & VBLOCK_WRITEONLY; }
  bool bufferLockDiscardRequested(uint32_t lock_flags) const
  {
    return (lock_flags & VBLOCK_DISCARD) ||
           // a write to a staging buffer is a implicit discard request
           (bufferLockedForWrite(lock_flags) && !bufferLockedForRead(lock_flags) && isStagingBuffer());
  }
  static bool bufferSyncUpdateRequested(uint32_t lock_flags) { return lock_flags & VBLOCK_NOOVERWRITE; }
  bool isDynamicBuffer() const { return 0 != (bufFlags & SBCF_DYNAMIC); }
  bool isDynamicOrCPUWriteable() const { return 0 != (bufFlags & (SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE)); }
  bool bufferGpuTimelineUpdate(uint32_t lock_flags) const
  {
    // when the buffer is dynamic and its locked for write only, the bufferSyncUpdateRequested() is
    // implied
    return (!bufferLockedForRead(lock_flags) && bufferLockedForWrite(lock_flags) && !bufferLockDiscardRequested(lock_flags) &&
            !bufferSyncUpdateRequested(lock_flags) && !isDynamicBuffer());
  }
  bool isStagingBuffer() const
  {
    // if it can not be used for anything and its read/write its a staging buffer (dx11 driver requires
    // r/w so we do it too).
    return (0 == (bufFlags & SBCF_BIND_MASK)) && (SBCF_CPU_ACCESS_WRITE | SBCF_CPU_ACCESS_READ) == (SBCF_CPU_ACCESS_MASK & bufFlags);
  }
  uint32_t getInitialDiscardCount() const
  {
    if (isDynamicBuffer())
    {
      if (bufFlags & SBCF_BIND_CONSTANT)
        return T::INITIAL_DISCARD_COUNT_DYNAMIC_CONST_BUFFER;
      return T::INITIAL_DISCARD_COUNT_DYNAMIC_BUFFER;
    }
    else if (isStagingBuffer())
    {
      // staging buffers treat write only locking as discard locking, so we need space for discarding
      return T::INITIAL_DISCARD_COUNT_STAGING_BUFFER;
    }
    return T::INITIAL_DISCARD_COUNT_DEFAULT;
  }
  typename T::MemoryClass getMemoryClass() const { return get_memory_class(bufFlags); }

  bool validate_buffer_properties(uint32_t flags, uint32_t format, const char *stat_name)
  {
    bool hadError = false;
    // there is no such limitation in DX12, but we now keep code compliant with DX11
    if (bool(flags & SBCF_MISC_STRUCTURED) && bool(flags & SBCF_MISC_DRAWINDIRECT))
    {
      BufferErrorHandler::errorStructuredIndirect(stat_name);
      hadError = true;
    }
    if (!(format == 0 || ((flags & (SBCF_BIND_VERTEX | SBCF_BIND_INDEX | SBCF_MISC_STRUCTURED | SBCF_MISC_ALLOW_RAW)) == 0)))
    {
      BufferErrorHandler::errorFormatUsedWithInvalidUsageFlags(stat_name);
      hadError = true;
    }
    return hadError;
  }

public:
  // Expose this helper so the driver may use it for other things (transient buffer create)
  static typename T::MemoryClass get_memory_class(uint32_t cflags)
  {
    if (T::IS_UMA)
    {
      // buffer that can be read should use memory with cache
      if (cflags & (SBCF_CPU_ACCESS_READ | SBCF_USAGE_READ_BACK))
        return T::SHARED_CACHED_MEMORY_CLASS;
    }

    if (cflags & (SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC))
    {
      if (T::INDIRECT_BUFFER_IS_SPECIAL)
      {
        // can't use readback or upload heap as source
        if (SBCF_MISC_DRAWINDIRECT & cflags)
          return T::SHARED_UNCACHED_INDIRECT_MEMORY_CLASS;
      }

      if (cflags & SBCF_BIND_CONSTANT)
      {
        if (cflags & SBCF_CPU_ACCESS_READ)
          return T::SHARED_CACHED_MEMORY_CLASS;
        else
          return T::FAST_SHARED_UNCACHED_MEMORY_CLASS;
      }
      else
      {
        if (cflags & SBCF_CPU_ACCESS_READ)
          return T::SHARED_CACHED_MEMORY_CLASS;
        else
          return T::SHARED_UNCACHED_MEMORY_CLASS;
      }
    }
    return T::PRIVATE_MEMORY_CLASS;
  }

  GenericSbufferImplementation(uint32_t struct_size, uint32_t element_count, uint32_t flags, uint32_t format_flags,
    const char *stat_name) :
    structSize(struct_size),
    bufSize(max<uint32_t>(struct_size, 1) * element_count),
    bufFlags(flags),
    viewFormat(T::viewFormatFromFormatFlags(format_flags))
  {
    setResName(stat_name);

    BufferMemoryArchitecture::allocateHostCopyMemory(bufFlags, bufSize);

    validate_buffer_properties(bufFlags, format_flags, stat_name);

    if (needsRawView())
    {
      if (structSize != 1 && structSize != 4)
        BufferErrorHandler::errorInvalidStructSizeForRawView(stat_name, structSize);
      structSize = 4;
      viewFormat = T::getRawViewFormat();
    }

    compose(stat_name);
  }

  GenericSbufferImplementation(typename T::BufferType &&buf, uint32_t struct_size, uint32_t element_count, uint32_t flags,
    uint32_t format_flags, const char *stat_name) :
    structSize(struct_size),
    bufSize(max<uint32_t>(struct_size, 1) * element_count),
    bufFlags(flags),
    viewFormat(T::viewFormatFromFormatFlags(format_flags))
  {
    setResName(stat_name);
    BufferMemoryArchitecture::allocateHostCopyMemory(bufFlags, bufSize);

    validate_buffer_properties(flags, format_flags, stat_name);

    if (needsRawView())
    {
      if (structSize != 1 && structSize != 4)
        BufferErrorHandler::errorInvalidStructSizeForRawView(stat_name, structSize);
      structSize = 4;
      viewFormat = T::getRawViewFormat();
    }

    adopt(eastl::move(buf), stat_name);
  }
  ~GenericSbufferImplementation()
  {
    BufferMemoryArchitecture::freeStagingMemory();

    if (T::isValidBuffer(buffer))
    {
      T::deleteBuffer(buffer, getResName());
      if (T::REPORT_MEMORY_SIZE_TO_MANAGER)
      {
        TEXQL_ON_BUF_RELEASE(this);
      }
    }
  }

  int ressize() const override { return bufSize; }
  int getFlags() const override { return bufFlags; }
  bool setReloadCallback(IReloadData *rd) override { return BufferReloadImplementation::updateReloadInfo(rd); }
  void destroy() override
  {
    STORE_RETURN_ADDRESS();
    T::onDestroyRequest(this);
  }

  int unlock() override
  {
    STORE_RETURN_ADDRESS();
    if (0 == lastLockFlags)
    {
      BufferErrorHandler::errorUnlockWithoutPreviousLock(getResName());
    }

    if (isStreamBuffer())
    {
      lastLockFlags = 0;
      // nothing to do
      return 1;
    }

    if (bufferLockedForWrite(lastLockFlags))
    {
      if (T::isValidMemory(temporaryMemory))
      {
        if (BufferMemoryArchitecture::hasStagingMemory())
        {
          BufferErrorHandler::errorUnexpectedUseCase(getResName());
        }

        T::updateBuffer(temporaryMemory, this, bufFlags, buffer, lockOffset);
      }
      else
      {
        if (BufferMemoryArchitecture::hasHostCopy())
        {
          if (bufferGpuTimelineUpdate(lastLockFlags) && isInitialized())
          {
            BufferErrorHandler::errorUnexpectedUseCase(getResName());
          }
          // Only copy from local copy to stage or buffer directly, later steps will do the copying
          // and flushing
          if (T::isMapable(buffer))
          {
            memcpy(T::getMappedPointer(buffer, lockOffset), BufferMemoryArchitecture::getHostCopyPointer(lockOffset), lockSize);
          }
          else if (BufferMemoryArchitecture::hasStagingMemory())
          {
            memcpy(BufferMemoryArchitecture::getStagingMemoryPointer(lockOffset),
              BufferMemoryArchitecture::getHostCopyPointer(lockOffset), lockSize);
          }
          else
          {
            // For one time initialization this is okay to do, but repeatedly, a different was may be a better fit.
            if (isInitialized())
            {
              BufferWarningHandler::warnPerformanceHostCopyToBufferWithTemporary(getResName());
            }
            auto buf = T::allocateTemporaryUploadMemory(lockSize);
            if (T::isValidMemory(buf))
            {
              memcpy(T::getMemoryPointer(buf, 0), BufferMemoryArchitecture::getHostCopyPointer(lockOffset), lockSize);
              T::uploadBuffer(buf, buffer, 0, lockOffset, lockSize);
              T::freeMemory(buf);
            }
            else
            {
              BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
            }
          }
        }

        if (BufferMemoryArchitecture::hasStagingMemory())
        {
          if (bufferGpuTimelineUpdate(lastLockFlags) && isInitialized())
          {
            BufferErrorHandler::errorUnexpectedUseCase(getResName());
          }
          BufferMemoryArchitecture::uploadStagingMemoryToBuffer(lockOffset, buffer, lockOffset, lockSize);
        }
        else if (T::isMapable(buffer))
        {
          if (bufferGpuTimelineUpdate(lastLockFlags) && isInitialized() && !T::IS_UMA)
          {
            BufferErrorHandler::errorUnexpectedUseCase(getResName());
          }
          T::flushMappedRange(buffer, lockOffset, lockSize);
        }
      }
    }

    if (T::isValidMemory(temporaryMemory))
    {
      T::freeMemory(temporaryMemory);
      temporaryMemory = T::getNullTemporaryMemory();
    }

    bufferIsInitialized();
    lastLockFlags = 0;
    return 1;
  }

  int lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags) override
  {
    STORE_RETURN_ADDRESS();
    if (0 != lastLockFlags)
    {
      BufferErrorHandler::errorLockWithoutPrevoiusUnlock(getResName());
    }
    if (ofs_bytes >= bufSize)
    {
      BufferErrorHandler::errorLockOffsetIsTooLarge(getResName(), ofs_bytes, bufSize);
    }
    if (ofs_bytes + size_bytes > bufSize)
    {
      BufferErrorHandler::errorLockSizeIsTooLarge(getResName(), ofs_bytes, size_bytes, bufSize);
      return 0;
    }
    if (T::HAS_STRICT_LOCKING_RULES && flags == 0 && !BufferMemoryArchitecture::hasHostCopy())
    {
      BufferErrorHandler::errorLockWithInvalidLockingFlags(getResName(), flags);
    }

    lastLockFlags = static_cast<uint16_t>(flags);

    lockOffset = ofs_bytes;
    lockSize = size_bytes ? size_bytes : (bufSize - ofs_bytes);

    if (isStreamBuffer())
    {
      if (bufferLockDiscardRequested(flags))
      {
        temporaryMemory = T::discardStreamMememory(this, bufSize, structSize, bufFlags, temporaryMemory, getResName());
      }
      if (!T::isValidMemory(temporaryMemory))
      {
        BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
        lastLockFlags = 0;
        return 0;
      }
      if (!ptr)
      {
        BufferErrorHandler::errorDiscardWithoutPointer(getResName());
        lastLockFlags = 0;
        return 0;
      }
      *ptr = T::getMemoryPointer(temporaryMemory, ofs_bytes);
      return 1;
    }

    if (bufferLockDiscardRequested(flags))
    {
      buffer = T::discardBuffer(this, buffer, bufSize, structSize, getMemoryClass(), viewFormat, bufFlags, getResName());

      if (!T::isValidBuffer(buffer))
      {
        // Caller has to be prepared that getName may return NULL
        BufferErrorHandler::errorDiscardingBuffer(getResName());
        return 0;
      }

      if (BufferMemoryArchitecture::discardStagingMemory(bufFlags, bufSize, getResName(), *this))
      {
        if (!BufferMemoryArchitecture::hasStagingMemory())
        {
          BufferErrorHandler::errorDiscardingStageMemory(getResName());
          return 0;
        }
      }
    }

    if (0 == ((VBLOCK_WRITEONLY | VBLOCK_READONLY) & lastLockFlags))
    {
      if (T::HAS_STRICT_LOCKING_RULES && (0 == ((VBLOCK_DISCARD | VBLOCK_NOOVERWRITE) & lastLockFlags)) &&
          !BufferMemoryArchitecture::hasHostCopy())
      {
        BufferErrorHandler::errorLockWithInvalidLockingFlags(getResName(), flags);
      }
      lastLockFlags |= VBLOCK_WRITEONLY;
    }

    if (bufferLockedForRead(flags))
    {
      void *basePointer = nullptr;
      if (requiresReadBack())
      {
        if (BufferMemoryArchitecture::hasStagingMemory())
        {
          if (!ptr)
          {
            BufferMemoryArchitecture::readBackBufferToStagingMemory(buffer, lockOffset, lockOffset, lockSize);
            beginReadBackProgress();
            lastLockFlags = 0;
            return 0;
          }

          if (!isReadBackInProgress())
          {
            BufferWarningHandler::warnPerformanceBlockingBufferReadBack(getResName());
            BufferMemoryArchitecture::blockingReadBackBufferToStagingMemory(buffer, lockOffset, lockOffset, lockSize);
            immediateReadBack();
          }
          else
          {
            endReadBackProgress();
          }

          BufferMemoryArchitecture::invalidateStagingMemory(lockOffset, lockSize);
          basePointer = BufferMemoryArchitecture::getStagingMemoryPointer(lockOffset);
        }
        else if (T::isMapable(buffer))
        {
          if (!ptr)
          {
            T::flushBuffer(buffer, lockOffset, lockSize);
            beginReadBackProgress();
            lastLockFlags = 0;
            return 0;
          }

          if (!isReadBackInProgress())
          {
            BufferWarningHandler::warnPerformanceBlockingBufferReadBack(getResName());
            T::blockingFlushBuffer(buffer, lockOffset, lockSize);
            immediateReadBack();
          }
          else
          {
            endReadBackProgress();
          }

          T::invalidateMappedRange(buffer, lockOffset, lockSize);
          basePointer = T::getMappedPointer(buffer, lockOffset);
        }
        else
        {
          BufferWarningHandler::warnPerformanceTemporaryMemoryForReadBack(getResName());
          if (!ptr)
          {
            if (!T::isValidMemory(temporaryMemory))
            {
              temporaryMemory = T::allocateReadWriteStagingMemory(lockSize);
            }
            else
            {
              BufferWarningHandler::warnPerformanceNotConsumedAsyncReadBack(getResName());
            }
            if (T::isValidMemory(temporaryMemory))
            {
              T::readBackBuffer(buffer, temporaryMemory, lockOffset, 0, lockSize);
              beginReadBackProgress();
            }
            else
            {
              BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
            }
            lastLockFlags = 0;
            return 0;
          }

          if (!isReadBackInProgress() || !T::isValidMemory(temporaryMemory))
          {
            if (T::isValidMemory(temporaryMemory))
              T::freeMemory(temporaryMemory);

            BufferWarningHandler::warnPerformanceBlockingBufferReadBack(getResName());
            temporaryMemory = T::allocateReadWriteStagingMemory(lockSize);
            if (T::isValidMemory(temporaryMemory))
            {
              T::blockingReadBackBuffer(buffer, temporaryMemory, lockOffset, 0, lockSize);
            }
            else
            {
              BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
            }
            immediateReadBack();
          }
          else
          {
            endReadBackProgress();
          }

          if (!T::isValidMemory(temporaryMemory))
          {
            // on error it was reported already
            lastLockFlags = 0;
            return 0;
          }

          T::invalidateMemory(temporaryMemory, 0, lockSize);
          basePointer = T::getMemoryPointer(temporaryMemory, 0);
        }

        if (BufferMemoryArchitecture::hasHostCopy())
        {
          memcpy(BufferMemoryArchitecture::getHostCopyPointer(lockOffset), basePointer, lockSize);
          basePointer = BufferMemoryArchitecture::getHostCopyPointer(lockOffset);
        }
      }
      else
      {
        if (BufferMemoryArchitecture::hasStagingMemory())
        {
          basePointer = BufferMemoryArchitecture::getStagingMemoryPointer(lockOffset);
        }
        else if (T::isMapable(buffer))
        {
          basePointer = T::getMappedPointer(buffer, lockOffset);
        }
        else if (BufferMemoryArchitecture::hasHostCopy())
        {
          basePointer = BufferMemoryArchitecture::getHostCopyPointer(lockOffset);
        }
        else if (!T::isValidMemory(temporaryMemory))
        {
          BufferWarningHandler::warnPerformanceTemporaryMemoryForReadBack(getResName());
          BufferWarningHandler::warnPerformanceReadBackOfUnchangedResource(getResName());
          // Have to read back again, temp memory is released after unlock
          if (!ptr)
          {
            temporaryMemory = T::allocateReadWriteStagingMemory(lockSize);
            if (T::isValidMemory(temporaryMemory))
            {
              T::readBackBuffer(buffer, temporaryMemory, lockOffset, 0, lockSize);
            }
            else
            {
              BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
            }
            beginReadBackProgress();
            lastLockFlags = 0;
            return 0;
          }

          if (!isReadBackInProgress() || !T::isValidMemory(temporaryMemory))
          {
            if (T::isValidMemory(temporaryMemory))
              T::freeMemory(temporaryMemory);

            BufferWarningHandler::warnPerformanceBlockingBufferReadBack(getResName());
            temporaryMemory = T::allocateReadWriteStagingMemory(lockSize);
            if (T::isValidMemory(temporaryMemory))
            {
              T::blockingReadBackBuffer(buffer, temporaryMemory, lockOffset, 0, lockSize);
            }
            else
            {
              BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
            }
            immediateReadBack();
          }
          else
          {
            endReadBackProgress();
          }

          if (!T::isValidMemory(temporaryMemory))
          {
            // on error it was reported already
            lastLockFlags = 0;
            return 0;
          }

          T::invalidateMemory(temporaryMemory, 0, lockSize);
          basePointer = T::getMemoryPointer(temporaryMemory, 0);
        }
      }

      G_ASSERT(basePointer);

      *ptr = basePointer;
      Stat3D::updateLockVIBuf();
      return 1;
    }

    if (!ptr)
    {
      BufferWarningHandler::warnUsageAsyncWithoutReadBackRequested(getResName());
      lastLockFlags = 0;
      return 0;
    }

    if (bufferGpuTimelineUpdate(flags) && isInitialized())
    {
      G_ASSERT(!T::isValidMemory(temporaryMemory));
      temporaryMemory = T::allocateTemporaryUploadMemory(lockSize);
      if (T::isValidMemory(temporaryMemory))
      {
        *ptr = T::getMemoryPointer(temporaryMemory, 0);
        Stat3D::updateLockVIBuf();
        return 1;
      }
      else
      {
        BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
        lastLockFlags = 0;
        return 0;
      }
    }

    if (BufferMemoryArchitecture::hasHostCopy())
    {
      *ptr = BufferMemoryArchitecture::getHostCopyPointer(lockOffset);
    }
    else if (BufferMemoryArchitecture::hasStagingMemory())
    {
      *ptr = BufferMemoryArchitecture::getStagingMemoryPointer(lockOffset);
    }
    else if (T::isMapable(buffer))
    {
      *ptr = T::getMappedPointer(buffer, lockOffset);
    }
    else
    {
      temporaryMemory = T::allocateTemporaryUploadMemory(lockSize);
      if (T::isValidMemory(temporaryMemory))
      {
        *ptr = T::getMemoryPointer(temporaryMemory, 0);
      }
      else
      {
        BufferErrorHandler::errorAllocatingTemporaryMemory(getResName());
        lastLockFlags = 0;
        return 0;
      }
    }

    Stat3D::updateLockVIBuf();
    return 1;
  }
  int getElementSize() const override { return structSize; }
  int getNumElements() const override { return structSize ? bufSize / structSize : 0; }
  bool copyTo(Sbuffer *dst) override
  {
    STORE_RETURN_ADDRESS();
    auto dest = (GenericSbufferImplementation *)dst;
    T::copyBuffer(this, bufFlags, buffer, temporaryMemory, isStreamBuffer(), dest, dest->bufFlags, dest->buffer, dest->temporaryMemory,
      dest->isStreamBuffer(), 0, 0, bufSize);
    dest->deviceWritesTo();
    return true;
  }
  bool copyTo(Sbuffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size_bytes) override
  {
    STORE_RETURN_ADDRESS();
    auto dest = (GenericSbufferImplementation *)dst;
    T::copyBuffer(this, bufFlags, buffer, temporaryMemory, isStreamBuffer(), dest, dest->bufFlags, dest->buffer, dest->temporaryMemory,
      dest->isStreamBuffer(), src_offset, dst_offset, size_bytes);
    dest->deviceWritesTo();
    return true;
  }
  bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lock_flags) override
  {
    STORE_RETURN_ADDRESS();
    if (ofs_bytes + size_bytes > bufSize)
    {
      BufferErrorHandler::errorLockSizeIsTooLarge(getResName(), ofs_bytes, size_bytes, bufSize);
      return false;
    }

    G_ASSERT_RETURN(size_bytes, false);

    if (isStreamBuffer())
    {
      if (bufferLockDiscardRequested(lock_flags))
      {
        temporaryMemory = T::discardStreamMememory(this, bufSize, structSize, bufFlags, temporaryMemory, getResName());
      }
      memcpy(T::getMemoryPointer(temporaryMemory, ofs_bytes), src, size_bytes);
      return true;
    }

    // fastest path, mapable + nooverwrite, we can safely memcpy it over and we are done
    if (bufferSyncUpdateRequested(lock_flags) && T::isMapable(buffer) && !BufferMemoryArchitecture::hasHostCopy() &&
        !BufferMemoryArchitecture::hasStagingMemory())
    {
      memcpy(T::getMappedPointer(buffer, ofs_bytes), src, size_bytes);
      return true;
    }

    // on gpu timeline update we can always use fast path (note this is only fast path for cpu,
    // on gpu timeline it requires a gpu copy so its slow path there).
    if (!bufferGpuTimelineUpdate(lock_flags))
    {
      if ((lock_flags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) || isDynamicOrCPUWriteable() ||
          BufferMemoryArchitecture::hasHostCopy() || !isInitialized())
      {
        // in those cases we need to use locking to keep everything consistent
        return updateDataWithLock(ofs_bytes, size_bytes, src, lock_flags);
      }
    }
    if (BufferMemoryArchitecture::hasHostCopy())
    {
      memcpy(BufferMemoryArchitecture::getHostCopyPointer(ofs_bytes), src, size_bytes);
    }
    // copies src into a temp buffer and later during GPU execution copies the temp buffer into this
    // buffer at the given offset this keeps relative ordering to other GPU commands, eg its done on
    // GPU time line.
    T::pushBufferUpdate(this, bufFlags, buffer, ofs_bytes, src, size_bytes);
    return true;
  }

  typename T::BufferConstReferenceType getDeviceBuffer() const { return buffer; }

  bool isStreamBuffer() const { return !T::isValidBuffer(buffer) && (0 != (STATUS_IS_STREAM_BUFFER & statusFlags)); }

  typename T::TemporaryMemoryType getStreamBuffer() const { return temporaryMemory; }

  template <typename C>
  void updateDeviceBuffer(C clb)
  {
    if (T::isValidBuffer(buffer))
    {
      clb(buffer);
    }
  }

  bool has32BitIndexType() const { return (bufFlags & SBCF_INDEX32) != 0; }

  void recreate()
  {
    statusFlags = 0;
    BufferMemoryArchitecture::resetStagingMemory();
    temporaryMemory = T::getNullTemporaryMemory();
    compose(getResName());
  }
  void restore()
  {
    if (!T::isValidBuffer(buffer))
      return;

    BufferReloadImplementation::executeReload(this);
  }
  void deviceWritesTo()
  {
    if (T::TRACK_GPU_WRITES)
      statusFlags |= STATUS_DEVICE_HAS_WRITTEN_TO;
  }

  bool hasSystemCopy() const { return BufferMemoryArchitecture::hasHostCopy(); }
};
