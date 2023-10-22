#include <debug/dag_hwExcept.h>
#include <debug/dag_except.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <osApiWrappers/dag_miscApi.h>

int DagorHwException::setHandler(const char * /*thread*/) { return -1; }
void DagorHwException::removeHandler(int /*id*/) {}
void DagorHwException::reportException(DagorException &caught_except, bool terminate, const char * /*add_dump_fname*/)
{
  out_debug_str_fmt("reportException: code=%d", caught_except.excCode);
  out_debug_str(caught_except.excDesc);
  //__debugbreak();
  if (terminate)
    terminate_process(2);
}
void DagorHwException::cleanup() {}
