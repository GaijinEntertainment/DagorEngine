// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_hwBreakpoint.h>


namespace dag::hwbrk
{
void set(const void *, Size, Slot, When) {}
void remove(Slot) {}
void on_break(eastl::function<void(void *, Slot)>) {}
} // namespace dag::hwbrk

#define EXPORT_PULL dll_pull_kernel_dagorHwBreakpoint
#include <supp/exportPull.h>
