#include "backtrace.h"
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_critSec.h>
#include <util/dag_string.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <hash/mum_hash.h>

struct BackTraceEntry
{
  static constexpr uint32_t MAX_FRAMES = 32;

  void *stackBuffer[MAX_FRAMES];
  uint32_t frames;
  String resolvedStack;

  uint64_t hash() { return mum_hash(stackBuffer, sizeof(void *) * frames, 0); }
};

namespace
{

eastl::hash_map<uint64_t, BackTraceEntry> entries;
WinCritSec rw_sync;

} // anonymous namespace

uint64_t backtrace::get_hash(uint32_t ignore_frames /*=0*/)
{
  BackTraceEntry entry;
  entry.frames = stackhlp_fill_stack(entry.stackBuffer, BackTraceEntry::MAX_FRAMES, 2 + ignore_frames);
  uint64_t hash = entry.hash();

  WinAutoLock lock(rw_sync);
  const auto fv = entries.find(hash);
  if (fv == entries.end())
    entries[hash] = entry;
  else
  {
    // fast hash collision check
    G_ASSERT((fv->second.frames == entry.frames) && (fv->second.stackBuffer[0] == entry.stackBuffer[0]));
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
    entry.resolvedStack = stackhlp_get_call_stack_str(entry.stackBuffer, entry.frames);

  return entry.resolvedStack;
}

String backtrace::get_stack() { return get_stack_by_hash(get_hash()); }

void backtrace::cleanup()
{
  WinAutoLock lock(rw_sync);

  entries.clear();
}