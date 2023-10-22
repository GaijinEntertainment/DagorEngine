#include "daProfilerInternal.h"
#include "daProfilePlatform.h"

namespace da_profiler
{
// to ensure out of line

DAGOR_NOINLINE
void ThreadStorage::freeEventChunks(uint64_t delete_older_than_that, uint64_t curTicks)
{
  nextFreeEventTick = curTicks + (cpu_frequency() >> 6); // each 1/64 second, i.e. 16msec.
  free_event_chunks(events, delete_older_than_that);
}

DAGOR_NOINLINE
void ThreadStorage::freeTagChunks(uint64_t delete_older_than_that, uint64_t curTicks)
{
  nextFreeTagTick = curTicks + (cpu_frequency() >> 6); // each 1/64 second, i.e. 16msec.
  free_event_chunks(stringTags, delete_older_than_that);
}

}; // namespace da_profiler