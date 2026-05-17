// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define DX12_HAS_CALLSTACK_EXT (_TARGET_PC_WIN && (DAGOR_DBGLEVEL > 0))
#if DX12_HAS_CALLSTACK_EXT

#include "constants.h"
#include <osApiWrappers/dag_stackHlp.h>
#include <ioSys/dag_dataBlock.h>
#include <dag/dag_vector.h>
#include <EASTL/array.h>
#include <hash/wyhash.h>
#include <util/dag_globDef.h>
#include <generic/dag_objectPool.h>

namespace drv3d_dx12::debug::call_stack::ext
{

using Message = stackhelp::ext::CallStackCaptureStore *;
using Capture = stackhelp::ext::CallStackCaptureStore;

struct Generator
{
  void configure(const DataBlock *api_config)
  {
    if (!api_config)
      return;
    enabled = api_config->getBool("callStackExt", true);
#if !DX12_FIXED_EXECUTION_MODE
    enabled &= strcmp(api_config->getStr("executionMode", ""), "immediate") != 0;
#endif
  }

  Message captureExtMessage(uint32_t frame_index)
  {
    if (DAGOR_UNLIKELY(!enabled))
      return nullptr;

    uint32_t i = frame_index % FRAME_FRAME_BACKLOG_LENGTH;
    auto &container = containerArray[i];
    if (DAGOR_UNLIKELY(lastFrame != i))
    {
      lastFrame = i;
      generation = 0;
      currentMessage = nullptr;
      container.freeAll();
    }

    size_t gen = stackhelp::ext::get_extended_call_stack_generation();
    if (DAGOR_LIKELY(gen == generation))
      return currentMessage;
    generation = gen;

    currentMessage = container.allocate();
    currentMessage->capture();
    if (DAGOR_UNLIKELY(!currentMessage->resolver))
    {
      container.free(currentMessage);
      currentMessage = nullptr;
    }

    return currentMessage;
  }

private:
  static inline thread_local size_t generation = 0;
  static inline thread_local Message currentMessage = nullptr;

  static inline thread_local ObjectPool<Capture> containerArray[FRAME_FRAME_BACKLOG_LENGTH];
  static inline thread_local uint32_t lastFrame = 0;
  bool enabled = false;
};

struct Resolver
{
  const char *extMessage(Message message)
  {
    if (!message)
      return "";

    uint64_t hash = wyhash(message->store, sizeof(message->store[0]) * message->storeSize, 37);
    for (LruRecord &r : cache)
    {
      if (r.hash == hash)
      {
        r.touched = generation;
        return buf[&r - cache.begin()].data();
      }
    }

    LruRecord &r = getLruSlot();
    r.hash = hash;
    r.touched = generation;
    eastl::array<char, max_buffer_size> &b = buf[&r - cache.begin()];
    stackhelp::ext::ResolvedRecord rec = static_cast<stackhelp::ext::CallStackInfo>(*message)(b.data(), b.size());
    b[min<size_t>(rec.writtenChars, b.size() - 1)] = 0;
    return b.data();
  }

private:
  static constexpr int max_buffer_size = 256;
  static constexpr int cache_size = 8;
  struct LruRecord
  {
    uint64_t hash;
    uint32_t touched = -1;
  };
  uint32_t generation = 0;
  eastl::array<eastl::array<char, max_buffer_size>, cache_size> buf;
  eastl::array<LruRecord, cache_size> cache;

  LruRecord &getLruSlot()
  {
    // Remove lru record
    auto *lru = cache.begin();
    for (LruRecord &r : cache)
    {
      if (r.touched == -1)
        return r;
      if (r.touched < lru->touched)
        lru = &r;
    }
    return *lru;
  }
};

} // namespace drv3d_dx12::debug::call_stack::ext
#endif