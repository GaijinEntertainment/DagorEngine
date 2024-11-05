// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bindQuirrelEx/autoCleanup.h>
#include <debug/dag_assert.h>

namespace sq
{
static CleanupUnregRec *auto_binding_tail = NULL;

CleanupUnregRec::CleanupUnregRec(cleanup_unregfunc_cb_t cb) : unregfuncCb(cb), next(auto_binding_tail)
{
  G_ASSERT(unregfuncCb);
  auto_binding_tail = this;
}

void cleanup_unreg_native_api(HSQUIRRELVM vm)
{
  G_ASSERT(vm);
  for (auto *brr = auto_binding_tail; brr; brr = brr->next)
    brr->unregfuncCb(vm);
}

} // namespace sq
