// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/browser/webBrowser.h"
#include <folders/folders.h>
#include "main/version.h"

#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <webBrowserHelper/webBrowser.h>
#include <darg/dag_browser.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_logSys.h>
#include <osApiWrappers/dag_direct.h>

#include <bindQuirrelEx/autoBind.h>

#include "main/main.h"


namespace browserutils
{

void init_web_browser()
{
  if (webbrowser::get_helper()->canEnable() || !::dgs_get_settings()->getBool("enableEmbeddedBrowser", true))
    return;
  webbrowser::Configuration cfg;

  String exeDir = folders::get_game_dir();
  cfg.resourcesBaseDir = exeDir.str();
  cfg.purgeCookiesOnStartup = dgs_get_settings()->getBool("wipeEmbeddedBrowserCookies", false);

  String cacheDir = folders::get_temp_dir() + ::get_game_name();
  cacheDir += "-ebrwsr";
  cfg.cachePath = cacheDir.str();

  cfg.userAgent = String(64, "DaNetGame/%s", get_exe_version_str()).str();
  cfg.acceptLanguages = ::get_localized_text("current_lang", "");

  char fnameBuf[DAGOR_MAX_PATH];
  const char *logName = ::dd_get_fname_without_path_and_ext(fnameBuf, sizeof(fnameBuf), ::get_log_filename());
  String log(256, "%s/%s-browser.log", ::get_log_directory(), logName);
  cfg.logPath = log.str();

  short portsStart = 16030;
  short portsEnd = 16060;
  cfg.availablePorts.reserve(portsEnd - portsStart);
  for (short i = portsStart; i < portsEnd; i++)
    cfg.availablePorts.push_back() = i;

  webbrowser::get_helper()->init(cfg);
}


void shutdown_web_browser() { webbrowser::get_helper()->shutdown(); }

} // namespace browserutils
