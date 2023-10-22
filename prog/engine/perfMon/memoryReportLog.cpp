#include "memoryReportPrivate.h"

#include <gui/dag_visualLog.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

namespace memoryreport
{

class LogBackend : public IBackend
{
  void onReport(const Entry &entry) override
  {
    visuallog::logmsg(
      String(64, "memory allocated: %.2fMb; difference: %db", entry.memory_allocated / 1024.0f / 1024.0f, entry.allocation_diff),
      E3DCOLOR(255, 255, 0), LOGLEVEL_DEBUG);
  }

  void onStart() override { visuallog::logmsg("memory report started", E3DCOLOR(255, 255, 0), LOGLEVEL_DEBUG); }

  void onStop() override { visuallog::logmsg("memory report stopped", E3DCOLOR(255, 255, 0), LOGLEVEL_DEBUG); }
};

static LogBackend log_backend;

} // namespace memoryreport

REGISTER_MEMORYREPORT_BACKEND(log, memoryreport::log_backend)
