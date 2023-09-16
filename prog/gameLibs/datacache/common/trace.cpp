#include "trace.h"

#if MAX_TRACE_LEVEL > 0
#include <debug/dag_log.h>
#include <debug/dag_logSys.h>
#include <stdio.h>
#include <stdarg.h>

namespace datacache
{

static int datacahe_trace_level = 0;

void set_trace_level(int lev) { datacahe_trace_level = lev; }

void _do_trace_impl(int lev, const char *fmt, ...)
{
  if (lev > datacahe_trace_level)
    return;
  va_list ap;
  va_start(ap, fmt);
  if (*get_log_filename())
    cvlogmessage(_MAKE4C('CACH'), fmt, ap);
  else
  {
    vprintf(fmt, ap);
    printf("\n");
  }
  va_end(ap);
}

} // namespace datacache

#endif