// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>

#if DAGOR_DBGLEVEL > 0
#include <osApiWrappers/dag_stackHlp.h>

void debug_dump_stack(const char *text, int skip_frames)
{
#if _TARGET_XBOX
  (void)(text);
  (void)(skip_frames);
#else
  void *stack[32];
  ::stackhlp_fill_stack(stack, 32, skip_frames + 1);

  char stk[4096];
  ::stackhlp_get_call_stack(stk, sizeof(stk), stack, 32);
  if (text)
    debug("%s: %s", text, stk);
  else
    debug("%s", stk);
#endif
}
#endif

#if DAGOR_DBGLEVEL < 1 && DAGOR_FORCE_LOGS
void debug_dump_stack(const char *, int) {}
#endif

#define EXPORT_PULL dll_pull_kernel_debugDumpStack
#include <supp/exportPull.h>
