// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_fatal.h>
#include <generic/dag_tab.h>
#include <util/limBufWriter.h>
#include <memory/dag_mem.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/unique_ptr.h>

static int fatal_ctx_depth = 16;
static int dag_fill_fatal_context(char *buf, int sz, bool terse);

enum
{
  MAX_RECLEN = 256,
  MAX_RECNUM = 16
};
static thread_local eastl::unique_ptr<char[]> fatal_ctx_buf[MAX_RECNUM];
static thread_local int fatal_ctx_buf_cnt = 0;

void _core_fatal_context_push(const char *str)
{
  if (fatal_ctx_buf_cnt >= fatal_ctx_depth)
    return;

  auto &fc_buf = fatal_ctx_buf[fatal_ctx_buf_cnt++];
  if (!fc_buf)
    fc_buf = eastl::make_unique<char[]>(MAX_RECLEN);
  strncpy(fc_buf.get(), str, MAX_RECLEN - 1);
  fc_buf[MAX_RECLEN - 1] = '\0';
  dgs_fill_fatal_context = dag_fill_fatal_context;
}
void _core_fatal_context_pop()
{
  int cnt = fatal_ctx_buf_cnt;
  if (cnt < 1)
    return;

  fatal_ctx_buf_cnt = cnt - 1;
}
void _core_fatal_set_context_stack_depth(int depth) { fatal_ctx_depth = depth < MAX_RECNUM ? depth : MAX_RECNUM; }

static int dag_fill_fatal_context(char *buf, int sz, bool terse)
{
  int fc_cnt = fatal_ctx_buf_cnt;
  if (!fc_cnt)
  {
    if (buf && sz > 0)
      buf[0] = 0;
    return 0;
  }

  LimitedBufferWriter lbw(buf, sz);
  lbw.append(terse ? ", context:\n" : "\n---fatal context---\n");
  for (int i = 0; i < fc_cnt; i++)
    lbw.aprintf("%s%s", i ? "\n" : "", fatal_ctx_buf[i].get());
  if (!terse)
    lbw.append("\n~~~~~~~~~~~~~~~~~~~\n");
  int len = sz - lbw.getBufLeft();
  lbw.done();
  return len;
}

#define EXPORT_PULL dll_pull_baseutil_fatalCtx
#include <supp/exportPull.h>
