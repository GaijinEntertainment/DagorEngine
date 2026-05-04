//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

// 0 - off
// 1 - only heap stat
// 2 - heap & video stat (usable, but slower)
#define MEM_STAT_ENABLED \
  (DAGOR_DBGLEVEL > 0 && _TARGET_PC) // Disabled on consoles since it's has high perf overhead if called frequently

namespace memstat
{
#if MEM_STAT_ENABLED

struct MemoryInfo
{
  int64_t dagorMemoryBytes;
  int64_t dagorMemoryChunks;

  int64_t xmemTex2dBytes;
  int64_t xmemTex2dChunks;

  int64_t xmemD3DBytes;
  int64_t xmemD3DChunks;
};

void begin_memory_allocation(const char *name, MemoryInfo &current, MemoryInfo &total, bool push = true, bool heap_ptr = false);
void end_memory_allocation(const char *name, const MemoryInfo &prev_current, const MemoryInfo &prev_total);

#define BEGIN_MEM(name)                                           \
  memstat::MemoryInfo beginMemCurrent##name, beginMemTotal##name; \
  memstat::begin_memory_allocation(#name, beginMemCurrent##name, beginMemTotal##name)

#define END_MEM(name) memstat::end_memory_allocation(#name, beginMemCurrent##name, beginMemTotal##name)

#define SCOPE_MEM(text)                                                           \
  struct ScopeMem##text                                                           \
  {                                                                               \
    memstat::MemoryInfo current, total;                                           \
    ScopeMem##text() { memstat::begin_memory_allocation(#text, current, total); } \
    ~ScopeMem##text() { memstat::end_memory_allocation(#text, current, total); }  \
  } scope_mem_##text

void dump_statistics();
void shutdown();
void clear_allocations_by(const char *str_);

#else // #if !MEM_STAT_ENABLED

#define BEGIN_MEM(name) ((void)(0))
#define END_MEM(name)   ((void)(0))
#define SCOPE_MEM(name) ((void)(0))

inline void dump_statistics() {}
inline void shutdown() {}
inline void clear_allocations_by(const char *) {}

#endif // #if MEM_STAT_ENABLED

}; // namespace memstat
