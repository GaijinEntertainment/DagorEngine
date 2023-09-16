#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#define __DEBUG_FILEPATH          "*"
#include <startup/dag_mainCon.inc.cpp>
#include <startup/dag_globalSettings.h>
#include <debug/dag_logSys.h>
#include <stdio.h>
#include <signal.h>
#include <vromfsPacker/vromfsPacker.h>

size_t dagormem_max_crt_pool_sz = 256 << 20;
size_t dagormem_first_pool_sz = 512 << 20;
size_t dagormem_next_pool_sz = 256 << 20;


static void __cdecl ctrl_break_handler(int) { quit_game(0); }


static void stderr_report_fatal_error(const char *, const char *msg, const char *call_stack)
{
  fprintf(stderr, "Fatal error: %s\n%s", msg, call_stack);
  quit_game(1, false);
}


static int err_count = 0;
static int log_callback(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  if (lev_tag == LOGLEVEL_ERR || lev_tag == LOGLEVEL_FATAL)
    err_count++;
  return 1;
}

int DagorWinMain(bool debugmode)
{
  dgs_report_fatal_error = stderr_report_fatal_error;
  debug_set_log_callback(&log_callback);

  setvbuf(stdout, NULL, _IOFBF, 8192);

  debug_enable_timestamps(false);
  if (!dgs_get_argv("dumpver"))
    printf("Virtual ROM file system (vromfs) pack builder v1.96\n%s\n",
      !::dgs_execute_quiet ? "Copyright (C) Gaijin Games KFT, 2023\n" : "");

  signal(SIGINT, ctrl_break_handler);

  int retCode = buildVromfs(nullptr, make_span_const((const char *const *)dgs_argv, dgs_argc));

  if (retCode != 0)
    return retCode;

  return err_count ? 13 : 0;
}

#if _TARGET_PC_LINUX
#include <debug/dag_hwExcept.h>

int DagorWinMain(int, bool debugmode)
{
  int retcode = -1;
  DAGOR_TRY { retcode = DagorWinMain(debugmode); }
  DAGOR_CATCH(DagorException e)
  {
#if DAGOR_EXCEPTIONS_ENABLED
    retcode = 13;
    logerr("exception: %s", e.excDesc);
    flush_debug_file();
#endif
  }
  return retcode;
}
int DagorWinMainInit(int, bool debugmode) { return true; }
#endif
