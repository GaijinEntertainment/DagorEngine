// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <webui/httpserver.h>
#include <errno.h>

#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <util/dag_lookup.h>
#include <stdio.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <ioSys/dag_genIo.h>
#include <gui/dag_visConsole.h>
#include <image/dag_jpeg.h>
#include <drv/3d/dag_driver.h>
#include <ioSys/dag_memIo.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <debug/dag_logSys.h>
#include <workCycle/dag_delayedAction.h>
#include <math/random/dag_random.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_stackHlp.h>
#include "webui_internal.h"

using namespace webui;

#if _TARGET_TVOS
#define SEND_LOG_TIMEOUT (10000)

namespace debug_internal
{
const char *get_cachedlog_filename_tvos(int);
}

template <size_t LogIndex>
static void show_lastlog(RequestInfo *params)
{
  debug_flush(false);

  char rl[4096];

  file_ptr_t ld = df_open(debug_internal::get_cachedlog_filename_tvos(LogIndex), DF_READ);
  if (!ld)
  {
    _snprintf(rl, sizeof(rl), "Error: can't open log '%s'", debug_internal::get_cachedlog_filename_tvos(LogIndex));
    rl[sizeof(rl) - 1] = 0;
    return html_response(params->conn, rl, HTTP_SERVER_ERROR);
  }

  int file_size = df_length(ld);

  if (df_seek_to(ld, 0))
    return html_response(params->conn, nullptr, HTTP_SERVER_ERROR);

  text_response(params->conn, nullptr, file_size);

  int64_t start_time = ref_time_ticks();
  while (file_size > 0)
  {
    int r = df_read(ld, rl, sizeof(rl));
    if (r < 0)
      break;

    tcp_send(params->conn, rl, r);

    file_size -= r;
  }

  df_close(ld);
}

webui::HttpPlugin webui::tvos_http_plugins[] = {
  {"lastlog_1", "show full debug log previous start (tvOS only)", nullptr, show_lastlog<1>},
  {"lastlog_2", "show full debug log previous one (tvOS only)", nullptr, show_lastlog<2>},
  {"lastlog_3", "show full debug log previous two start (tvOS only)", nullptr, show_lastlog<3>}, {nullptr}};

#endif
