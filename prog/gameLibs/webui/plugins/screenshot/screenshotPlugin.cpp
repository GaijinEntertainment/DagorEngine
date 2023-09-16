#include <webui/httpserver.h>
#include <errno.h>

#include <memory/dag_framemem.h>
#include <generic/dag_tab.h>
#include <stdio.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_console.h>
#include <ioSys/dag_genIo.h>
#include <gui/dag_visConsole.h>
#include <3d/dag_drv3dCmd.h>
#if _TARGET_PC
#include <screenShotSystem/dag_screenShotSystem.h>
#endif
#include <image/dag_jpeg.h>
#include <3d/dag_drv3d.h>
#include <webui/helpers.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <stdlib.h>
#include "webui_internal.h"

using namespace webui;

#if _TARGET_PC
static bool save_jpeg32_shot_cb(const char *fn, TexPixel32 *im, int wd, int ht, int stride)
{
  return save_jpeg32((unsigned char *)im, wd, ht, stride, 90, *(YAMemSave *)fn);
}
#endif

static void jpeg_response(webui::SocketType conn, const YAMemSave &save)
{
  char buf[128];
  _snprintf(buf, sizeof(buf),
    "HTTP/1.1 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: image/jpeg\r\n"
    "Content-Length: %d\r\n"
    "\r\n",
    int(save.offset));
  buf[sizeof(buf) - 1] = 0;
  tcp_send_str(conn, buf);

  tcp_send(conn, save.mem, (int)save.offset);
}

static void make_screenshot(RequestInfo *params)
{
  YAMemSave save;
#if _TARGET_PC
  ScreenShotSystem::setWriteImageCB(save_jpeg32_shot_cb, "jpg");
  ScreenShotSystem::ScreenShot screen;
  if (ScreenShotSystem::makeScreenShot(screen))
  {
    if (ScreenShotSystem::saveScreenShotTo(screen, (char *)&save) && save.offset)
      return jpeg_response(params->conn, save);
  }
#else
  // do nothing
  (void)(params);
  (void)(save);
  (void)&jpeg_response;
#endif
  return html_response(params->conn, NULL, HTTP_SERVER_ERROR);
}

webui::HttpPlugin webui::screenshot_http_plugin = {"screenshot", "view screen shot", NULL, make_screenshot};
