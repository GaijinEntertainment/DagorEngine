#include <memory/dag_physMem.h>
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_C3
#include <sys/mman.h>
#endif

#include <math/dag_mathBase.h>
#include <../drv/drv3d_commonCode/tabWithLock.h>
#include <util/dag_globDef.h>

using namespace dagor_phys_memory;
namespace dagor_phys_memory
{
struct MemBlock
{
  void *addr;
#if _TARGET_C1 | _TARGET_C2

#endif
  size_t size;
};

static uint32_t total_allocated_pages = 0; // protected by phys_map critsec
static uint32_t max_allocated_pages = 0;   // protected by phys_map critsec
static TabWithLock<MemBlock> phys_map(midmem_ptr());

void *alloc_phys_mem(size_t size, size_t alignment, uint32_t prot_flags, bool cpu_cached, bool log_failure)
{
  G_UNUSED(log_failure);
  G_ASSERT(size != 0);

#if _TARGET_ANDROID | _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C3
  (void)alignment;
  (void)prot_flags;
  (void)cpu_cached;
  return memalloc(size, midmem);

#else

  MemBlock blk;
  blk.addr = NULL;
  blk.size = size;

#if _TARGET_C1 | _TARGET_C2



#else // mac, linux
  G_UNREFERENCED(cpu_cached);
  G_UNREFERENCED(alignment);
  size = POW2_ALIGN(size, 4096);
  G_ASSERT((prot_flags & (PM_PROT_GPU_READ | PM_PROT_GPU_WRITE)) == 0);

  blk.addr = mmap(0, size, ((prot_flags & PM_PROT_CPU_READ) ? PROT_READ : 0) | ((prot_flags & PM_PROT_CPU_WRITE) ? PROT_WRITE : 0),
    MAP_PRIVATE | MAP_ANON, -1, 0);

  if (blk.addr == NULL)
    return NULL;
#endif
  phys_map.lock();
  phys_map.push_back(blk);
  total_allocated_pages += uint32_t(size / 4096);
  max_allocated_pages = max(max_allocated_pages, total_allocated_pages);
  phys_map.unlock();
  return blk.addr;
#endif
}

void free_phys_mem(void *ptr)
{
  if (ptr == NULL)
    return;
#if _TARGET_ANDROID | _TARGET_PC_WIN | _TARGET_XBOX | _TARGET_C3
  memfree(ptr, midmem);

#else

  phys_map.lock();
  for (int i = 0; i < phys_map.size(); ++i)
  {
    MemBlock &blk = phys_map[i];
    if (blk.addr == ptr)
    {
#if _TARGET_C1 | _TARGET_C2

#else
      munmap(blk.addr, blk.size);
#endif
      total_allocated_pages -= uint32_t(blk.size / 4096);
      erase_items_fast(phys_map, i, 1);
      phys_map.unlock();
      return;
    }
  }
  G_ASSERTF(0, "phys_release: non existent block &p", ptr);
  debug("phys_release: non existent block %p", ptr);
  phys_map.unlock();
#endif
}

size_t get_allocated_phys_mem() { return 4096UL * total_allocated_pages; }
size_t get_max_allocated_phys_mem() { return 4096UL * max_allocated_pages; }
void add_ext_allocated_phys_mem(int64_t size)
{
  size = (size + 4095) / 4096;
  phys_map.lock();
  total_allocated_pages += size;
  max_allocated_pages = max(max_allocated_pages, total_allocated_pages);
  phys_map.unlock();
}

} // namespace dagor_phys_memory
