// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "memoryReportPrivate.h"

#include <util/dag_string.h>
#include <debug/dag_debug.h>

namespace memoryreport
{

class LogBackend : public IBackend
{
  void onReport(const Entry &entry) override
  {
    debug("memory allocated: %.2fMb; difference: %db", entry.memory_allocated / 1024.0f / 1024.0f, entry.allocation_diff);
  }

  void onStart() override { debug("memory report started"); }

  void onStop() override { debug("memory report stopped"); }
};

static LogBackend log_backend;

} // namespace memoryreport

REGISTER_MEMORYREPORT_BACKEND(log, memoryreport::log_backend)
