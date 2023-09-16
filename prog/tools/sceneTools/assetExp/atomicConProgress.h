#pragma once

#include <libTools/util/atomicPrintf.h>
#include <libTools/util/progressInd.h>

struct AtomicConsoleProgressIndicator : public ConsoleProgressIndicator
{
  AtomicConsoleProgressIndicator(bool pct) : ConsoleProgressIndicator(pct) {}
  void setActionDescFmt(const char *fmt, const DagorSafeArg *arg, int anum) override
  {
    AtomicPrintfMutex::inst.lock();
    ConsoleProgressIndicator::setActionDescFmt(fmt, arg, anum);
    AtomicPrintfMutex::inst.unlock(stdout, true);
  }
  void outputPct(int pct) override
  {
    AtomicPrintfMutex::inst.lock();
    ConsoleProgressIndicator::outputPct(pct);
    AtomicPrintfMutex::inst.unlock(stdout, true);
  }

  static IGenericProgressIndicator *create(bool pct) { return new AtomicConsoleProgressIndicator(pct); }
};
