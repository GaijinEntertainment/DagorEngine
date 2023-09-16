#include <util/fnameMap.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_fastNameMapTS.h>

static FastNameMapTS<false> nm;

void dagor_fname_map_clear() { nm.reset(false); }

const char *dagor_fname_map_add_fn(const char *fn) { return nm.internName(fn); }

int dagor_fname_map_get_fn_id(const char *fn, bool allow_add) { return allow_add ? nm.addNameId(fn) : nm.getNameId(fn); }
const char *dagor_fname_map_resolve_id(int id) { return nm.getName(id); }

int dagor_fname_map_count(int *out_mem_allocated, int *out_mem_used)
{
  const int nameCount = nm.nameCountRelaxed();
  if (out_mem_allocated || out_mem_used)
  {
    size_t used, allocated;
    nm.memInfo(used, allocated);
  }
  return nameCount;
}

#define EXPORT_PULL dll_pull_baseutil_fnameMap
#include <supp/exportPull.h>
