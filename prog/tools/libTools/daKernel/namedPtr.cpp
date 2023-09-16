#include <libTools/util/daKernel.h>
#include <util/dag_oaHashNameMap.h>
#include <generic/dag_tab.h>

static OAHashNameMap<true> names;
static Tab<void *> ptrs(inimem);

void *dakernel::get_named_pointer(const char *name)
{
  int id = names.getNameId(name);
  if (id < 0)
    return NULL;
  return ptrs[id];
}

void dakernel::set_named_pointer(const char *name, void *p)
{
  int id = names.addNameId(name);
  if (id == names.nameCount() - 1)
    ptrs.push_back(NULL);
  G_ASSERT(ptrs.size() == names.nameCount());
  ptrs[id] = p;
}

void dakernel::reset_named_pointers()
{
  names.reset(false);
  clear_and_shrink(ptrs);
}
