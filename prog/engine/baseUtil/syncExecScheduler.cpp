#include <generic/dag_execScheduler.h>
#include <osApiWrappers/dag_miscApi.h>

bool SyncExecutionScheduler::canProceed(const char * /*src_fname*/, int /*src_line*/) { return true; }
void SyncExecutionScheduler::wait(int msec)
{
  if (msec > 0)
    sleep_msec(msec);
}

#define EXPORT_PULL dll_pull_baseutil_syncExecScheduler
#include <supp/exportPull.h>
