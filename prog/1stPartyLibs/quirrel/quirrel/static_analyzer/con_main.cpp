#include <util/dag_string.h>
#include <util/dag_globDef.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_stackHlp.h>

#define __UNLIMITED_BASE_PATH 1
#define __DEBUG_FILEPATH (::dgs_get_argv("debugfile") ? "debug" : NULL)
#include <startup/dag_mainCon.inc.cpp>

int sq3_sa_main(int argc, char ** argv);
extern const char * processing_file_name;

static bool fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  fprintf(stderr, "FATAL ERROR:\n%s:%d:\nDuring processing file '%s'\n%s\n%s", file, line, processing_file_name, msg, call_stack);
  fflush(stderr);
  exit(1);
  return false;
}

static void report_fatal(const char *title, const char *buf, const char *call_stack)
{
  fprintf(stderr, "FATAL ERROR:\n%s\nDuring processing file '%s'\n%s\n%s", title, processing_file_name, buf, call_stack);
  fflush(stderr);
  exit(1);
}

int DagorWinMain(bool /*debugmode*/)
{
  ::dgs_fatal_handler = fatal_handler;
  ::dgs_report_fatal_error = report_fatal;

  return sq3_sa_main(::dgs_argc, ::dgs_argv);
}
