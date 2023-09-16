#include <memoryProfiler/dag_memoryStat.h>

#if MEM_STAT_ENABLED
#include <util/dag_stdint.h>
#include <util/dag_globDef.h>
#include <util/dag_tabHlp.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>
#include <memory/dag_mem.h>
#include <memory/dag_memStat.h>
#include <math/dag_mathBase.h>
#include <stdlib.h>

namespace memstat
{

#define MEM_BYTES_SHOW_THRESHOLD 1024 // do not show stat with less than this memory (bytes)

struct AllocationInfo
{
  const char *name;
  MemoryInfo mem;
  uint32_t parentIdx;
  bool heap_ptr;
  bool pushed;
};

static Tab<AllocationInfo> allocations(midmem);
static MemoryInfo memory_allocation_total = {0};
static uint32_t memAllocationsStack[16];
static uint32_t curAStackIdx = -1;
static WinCritSec cs_mem_stat;

void begin_memory_allocation(const char *name, MemoryInfo &current, MemoryInfo &total, bool push, bool heap_ptr)
{
  WinAutoLock lock(cs_mem_stat);

  if (push)
  {
    int total_count = 0;
    const uint32_t parentIdx = curAStackIdx < countof(memAllocationsStack) ? memAllocationsStack[curAStackIdx] : -1;
    uint32_t pushIdx = -1;
    AllocationInfo *aa;
    for (uint32_t i = 0, sz = (uint32_t)allocations.size(); i < sz; ++i)
      if (strcmp(name, allocations[i].name) == 0)
      {
        if (parentIdx != -1 && parentIdx != i)
          allocations[i].parentIdx = parentIdx;
        pushIdx = i;
        goto _alloc_rec_already_exist;
      }
    pushIdx = (uint32_t)allocations.size();
    total_count = allocations.capacity();
    if (!allocations.capacity())
      allocations.reserve(512);
    aa = &allocations.push_back();
    if (!total_count && allocations.capacity())
    {
      aa->name = "";
      BEGIN_MEM(allocations_itself);
      END_MEM(allocations_itself);
      (aa + 1)->mem.dagorMemoryChunks = 1;
      (aa + 1)->mem.dagorMemoryBytes = (int)midmem->getSize(allocations.data());
    }
    memset(aa, 0, sizeof(*aa));
    aa->name = heap_ptr ? str_dup(name, strmem) : name;
    aa->parentIdx = parentIdx;
    aa->heap_ptr = heap_ptr;
  _alloc_rec_already_exist:
    if (curAStackIdx == -1 || curAStackIdx < (countof(memAllocationsStack) - 1))
    {
      memAllocationsStack[++curAStackIdx] = pushIdx;
      allocations[pushIdx].pushed = true;
    }
  }

  memset(&current, 0, sizeof(current));

  current.dagorMemoryBytes = (int)dagor_memory_stat::get_memory_allocated();
  current.dagorMemoryChunks = (int)dagor_memory_stat::get_memchunk_count();

#if MEM_STAT_ENABLED > 1
#endif // MEM_STAT_ENABLED > 1

  total = memory_allocation_total;
}

void end_memory_allocation(const char *name, const MemoryInfo &prev_current, const MemoryInfo &prev_total)
{
  WinAutoLock lock(cs_mem_stat);
  MemoryInfo new_current, new_total;
  begin_memory_allocation(name, new_current, new_total, false);

  for (uint32_t dataNo = 0; dataNo < (sizeof(MemoryInfo) / sizeof(uint32_t)); dataNo++)
  {
    ((int *)&memory_allocation_total)[dataNo] +=
      ((int *)&new_current)[dataNo] - ((int *)&prev_current)[dataNo] - (((int *)&new_total)[dataNo] - ((int *)&prev_total)[dataNo]);
  }

  for (uint32_t allocationNo = 0; allocationNo < allocations.size(); allocationNo++)
  {
    if (!strcmp(allocations[allocationNo].name, name))
    {
      for (uint32_t dataNo = 0; dataNo < (sizeof(MemoryInfo) / sizeof(uint32_t)); dataNo++)
      {
        ((int *)&allocations[allocationNo].mem)[dataNo] += ((int *)&new_current)[dataNo] - ((int *)&prev_current)[dataNo];
      }
      if (allocations[allocationNo].pushed)
      {
        --curAStackIdx;
        G_ASSERT(curAStackIdx == -1 || curAStackIdx < countof(memAllocationsStack));
      }
      return;
    }
  }

  G_ASSERTF(0, "unmatched END_MEM('%s') call?", name);
}

void clear_allocations_by(const char *str)
{
  for (int i = 0; i < allocations.size(); ++i)
    if (strstr(allocations[i].name, str))
      memset(&allocations[i].mem, 0, sizeof(MemoryInfo));
}

static int sort_by_mem(int *a, int *b) { return allocations[*b].mem.dagorMemoryBytes - allocations[*a].mem.dagorMemoryBytes; }

static int print_a_stat(int allocationNo, int &rec_level, char *buf)
{
  int my_size = 0;
  if ((uint32_t)allocationNo < (uint32_t)allocations.size())
  {
    const AllocationInfo &ai = allocations[allocationNo];
    if (memcmp(ZERO_PTR<MemoryInfo>(), &ai.mem, sizeof(MemoryInfo)) != 0 &&
        (ai.mem.xmemTex2dBytes || ai.mem.xmemD3DBytes || abs(ai.mem.dagorMemoryBytes) > MEM_BYTES_SHOW_THRESHOLD))
    {
      G_ASSERT(rec_level * 4 < 64);
      memset(buf, ' ', rec_level * 4);
      buf[rec_level * 4] = 0;
      debug("%s%5.1fMb, %5.1fMb, %5.1fMb - %s           (%d/%d, %d/%d, %d/%d)", buf, ai.mem.dagorMemoryBytes / 1024 / 1024.f,
        ai.mem.xmemTex2dBytes / 1024 / 1024.f, ai.mem.xmemD3DBytes / 1024 / 1024.f, ai.name, ai.mem.dagorMemoryBytes,
        ai.mem.dagorMemoryChunks, ai.mem.xmemTex2dBytes, ai.mem.xmemTex2dChunks, ai.mem.xmemD3DBytes, ai.mem.xmemD3DChunks);
    }
    my_size = ai.mem.dagorMemoryBytes;
  }
  else
    rec_level = 0;
  Tab<int> idxs(tmpmem);
  for (int i = 0; i < allocations.size(); ++i)
    if (allocations[i].parentIdx == (uint32_t)allocationNo)
      idxs.push_back(i);
  if (!idxs.size())
    return my_size;
  dag_qsort(idxs.data(), idxs.size(), sizeof(int), (cmp_func_t)&sort_by_mem);
  if ((uint32_t)allocationNo < (uint32_t)allocations.size())
    ++rec_level;
  int mem_sum = 0;
  for (int i = 0; i < idxs.size(); ++i)
  {
    int rl = rec_level;
    mem_sum += print_a_stat(idxs[i], rec_level, buf);
    rec_level = rl;
  }
  if ((my_size - mem_sum) >= (64 << 10))
  {
    G_ASSERT(rec_level * 4 < 64);
    memset(buf, ' ', rec_level * 4);
    buf[rec_level * 4] = 0;
    debug("%s%5.1fMb, %5.1fMb, %5.1fMb - %s           (%d/%d, %d/%d, %d/%d)", buf, (my_size - mem_sum) / 1024.f / 1024.f, 0.f, 0.f,
      "_dark_matter_", (my_size - mem_sum), 0, 0, 0, 0, 0);
  }
  return my_size;
}

void dump_statistics()
{
  const char *f = "Allocation statistics: dagormem, tex2d, xmemD3D";
  debug("%s", f);
  debug("-----------------------------------------------");
  debug("%.1fMb, %.1fMb, %.1fMb - %s           (%d/%d, %d/%d, %d/%d)", memory_allocation_total.dagorMemoryBytes / 1024.f / 1024.f,
    memory_allocation_total.xmemTex2dBytes / 1024.f / 1024.f, memory_allocation_total.xmemD3DBytes / 1024.f / 1024.f, "Total",
    memory_allocation_total.dagorMemoryBytes, memory_allocation_total.dagorMemoryChunks, memory_allocation_total.xmemTex2dBytes,
    memory_allocation_total.xmemTex2dChunks, memory_allocation_total.xmemD3DBytes, memory_allocation_total.xmemD3DChunks);

  char spaceBuf[64];
  int rec_level;
  print_a_stat(-1, rec_level, spaceBuf);

  debug("\n\n\n");
}

void shutdown()
{
  for (int i = 0; i < allocations.size(); ++i)
    if (allocations[i].heap_ptr)
      memfree((void *)allocations[i].name, strmem);
}

}; // namespace memstat

#endif // #if MEM_STAT_ENABLED
