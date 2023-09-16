#include <debug/dag_fatal.h>
#include <generic/dag_tab.h>
#include <util/limBufWriter.h>
#include <memory/dag_mem.h>
#include <startup/dag_globalSettings.h>

#if _TARGET_PC_LINUX // econom mode
void _core_fatal_context_push(const char *) {}
void _core_fatal_context_pop() {}
void _core_fatal_set_context_stack_depth(int) {}

#else
static int fatal_ctx_depth = 16;
static int dag_fill_fatal_context(char *buf, int sz, bool terse);

enum
{
  MAX_RECLEN = 256,
  MAX_RECNUM = 16
};
static thread_local char fatal_ctx_buf[MAX_RECLEN * MAX_RECNUM];
static thread_local int fatal_ctx_buf_cnt = 0;

void _core_fatal_context_push(const char *str)
{
  if (fatal_ctx_buf_cnt >= fatal_ctx_depth)
    return;

  char *fc_buf = fatal_ctx_buf;
  int cnt = fatal_ctx_buf_cnt;
  strncpy(fc_buf + cnt * MAX_RECLEN, str, MAX_RECLEN - 1);
  fc_buf[cnt * MAX_RECLEN + MAX_RECLEN - 1] = 0;
  fatal_ctx_buf_cnt = cnt + 1;
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
  const char *fc_buf = fatal_ctx_buf;
  lbw.aprintf("%s", fc_buf);
  for (int i = 1; i < fc_cnt; i++)
    lbw.aprintf("\n%s", fc_buf + i * MAX_RECLEN);
  if (!terse)
    lbw.append("\n~~~~~~~~~~~~~~~~~~~\n");
  int len = sz - lbw.getBufLeft();
  lbw.done();
  return len;
}
#endif

#define EXPORT_PULL dll_pull_baseutil_fatalCtx
#include <supp/exportPull.h>
