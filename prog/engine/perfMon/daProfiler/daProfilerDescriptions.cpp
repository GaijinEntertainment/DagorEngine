// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "daProfilerDescriptions.h"

namespace da_profiler
{

// matches dag_daProfilerToken.h!
static const char *default_tokens[] = {
  "",
  "critsec",
  "mutex",
  "spinlock",
  "global_mutex",
  "rwlock_write",
  "rwlock_read",
  "tpool_wait",
  "rpc",
  "sleep",
  "Frame",
  "CPU Frame",
  "GPU",
  "GPU Frame",
  "Triangles",
  "Nested Frames",
};


void ProfilerDescriptions::initInternal()
{
  if (!empty())
    return;
  G_STATIC_ASSERT(TotalCount < 256); // fixedDescriptions are one byte
  G_STATIC_ASSERT(sizeof(default_tokens) / sizeof(const char *) == TotalCount);
  fixedDescriptions[0] = emplaceNoCheck(EventDescription{"", "", 0, 0}); // invalid description == 0
  for (int i = 1; i < DescCount; ++i)
    fixedDescriptions[i] = emplaceNoCheck(EventDescription{default_tokens[i], __FILE__, __LINE__, 0u | (IsWait << 24)});
  for (int i = DescCount; i < TotalCount; ++i)
    fixedDescriptions[i] = emplaceNoCheck(EventDescription{default_tokens[i], __FILE__, __LINE__, 0u});
}

}; // namespace da_profiler
