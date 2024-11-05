// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/util/progressInd.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <stdarg.h>

ConsoleProgressIndicator::~ConsoleProgressIndicator()
{
  printf("\n");
  fflush(stdout);
}

void ConsoleProgressIndicator::setActionDescFmt(const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (anum)
  {
#if _TARGET_STATIC_LIB
    DagorSafeArg::fprint_fmt(stdout, String(0, "%s%s\n", showPct ? "\r\t\t\r" : "", fmt), arg, anum);
#else
    String msg;
    msg.vprintf(0, String(0, "%s%s\n", showPct ? "\r\t\t\r" : "", fmt), arg, anum);
    if (fwrite(msg.data(), 1, msg.length(), stdout))
      clear_and_shrink(msg);
#endif
  }
  else
    printf("%s%s\n", showPct ? "\r\t\t\r" : "", fmt);
  lastPct = -1;
  fflush(stdout);
}

void ConsoleProgressIndicator::outputPct(int pct) { printf("\r%5d%%", pct); }


IGenericProgressIndicator *create_con_progressbar(bool show_pct) { return new ConsoleProgressIndicator(show_pct); }
