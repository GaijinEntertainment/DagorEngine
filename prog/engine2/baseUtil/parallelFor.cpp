#include <util/dag_parallelFor.h>
#include <util/dag_parallelForInline.h>

void threadpool::parallel_for(uint32_t begin, uint32_t end, uint32_t quant,
  eastl::fixed_function<sizeof(void *) * 2, void(uint32_t tbegin, uint32_t tend, uint32_t thread_id)> cb, uint32_t add_jobs_count,
  JobPriority prio, bool wake,
  uint32_t dapDescId) // wide load expects high priority
{
  parallel_for_inline(begin, end, quant, eastl::move(cb), add_jobs_count, prio, wake, dapDescId);
}

#define EXPORT_PULL dll_pull_baseutil_parallel_for
#include <supp/exportPull.h>
