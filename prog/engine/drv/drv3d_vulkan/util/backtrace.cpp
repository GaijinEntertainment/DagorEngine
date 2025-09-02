// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "backtrace.h"
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>
#include <EASTL/hash_map.h>
#include <hash/mum_hash.h>

struct BackTraceEntry
{
  static constexpr uint32_t MAX_FRAMES = 64;
  static constexpr uint32_t MAX_FRAMES_TOTAL = MAX_FRAMES + stackhelp::ext::CallStackCaptureStore::call_stack_max_size;

  stackhelp::CallStackCaptureStore<MAX_FRAMES> stack;
  stackhelp::ext::CallStackCaptureStore extStack;

  String resolvedStack;

  uint64_t hash()
  {
    if (extStack.storeSize)
    {
      void *flatBuf[MAX_FRAMES_TOTAL];
      memcpy(&flatBuf[0], stack.store, stack.storeSize * sizeof(void *));
      memcpy(&flatBuf[stack.storeSize], extStack.store, extStack.storeSize * sizeof(void *));

      return mum_hash(flatBuf, sizeof(void *) * (stack.storeSize + extStack.storeSize), 0);
    }
    else
      return mum_hash(stack.store, sizeof(void *) * stack.storeSize, 0);
  }
};

namespace
{

eastl::hash_map<uint64_t, BackTraceEntry> entries;
WinCritSec rw_sync;

} // anonymous namespace

uint64_t backtrace::get_hash(uint32_t ignore_frames /*=0*/)
{
  BackTraceEntry entry;
  entry.stack.capture(2 + ignore_frames);
  entry.extStack.capture();
  uint64_t hash = entry.hash();

  WinAutoLock lock(rw_sync);
  const auto fv = entries.find(hash);
  if (fv == entries.end())
    entries[hash] = entry;
  else
  {
    // fast hash collision check
    G_ASSERT((fv->second.stack.storeSize == entry.stack.storeSize) && (fv->second.stack.store[0] == entry.stack.store[0]) &&
             (fv->second.extStack.storeSize == entry.extStack.storeSize) && (fv->second.extStack.store[0] == entry.extStack.store[0]));
  }

  return hash;
}

String backtrace::get_stack_by_hash(uint64_t caller_hash)
{
  WinAutoLock lock(rw_sync);

  if (!entries.count(caller_hash))
    return String("<caller not found>");

  BackTraceEntry &entry = entries[caller_hash];

  if (entry.resolvedStack.empty())
    entry.resolvedStack = stackhelp::ext::get_call_stack_str(entry.stack, entry.extStack);

  return entry.resolvedStack;
}

String backtrace::get_stack() { return get_stack_by_hash(get_hash()); }

void backtrace::cleanup()
{
  WinAutoLock lock(rw_sync);

  entries.clear();
}