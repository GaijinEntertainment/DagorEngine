// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sender.h"

#include "configuration.h"
#include "files.h"
#include "log.h"
#include "stats.h"
#include "ui.h"
#include "upload.h"

#include <string>

#if _TARGET_PC_WIN
#include <windows.h>
#include <osApiWrappers/dag_unicode.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#endif


namespace breakpad
{

int spawn(const std::vector<std::string> &parent)
{
#if _TARGET_PC_WIN
  std::string cmd;
  for (const auto &a : parent)
    cmd += a + " ";

  log << "Restarting parent: " << cmd << std::endl;
  stats::count("spawn", "start");
  wchar_t buf[2048] = {L'\0'};
  std::wstring wcmd = utf8_to_wcs(cmd.c_str(), buf, 2048);
  if (wcmd.empty())
  {
    log << "Could not spawn process: error in commandline" << std::endl;
    stats::count("spawn", "fail");
    return 1;
  }

  std::wstring wexe = utf8_to_wcs(parent[0].c_str(), buf, 2048);
  std::wstring wcwd = wexe.substr(0, wexe.rfind(L'\\'));

  STARTUPINFOW si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));

  bool spawned = ::CreateProcessW(wexe.c_str(), (wchar_t *)wcmd.c_str(), NULL, NULL, FALSE, 0, NULL, wcwd.c_str(), &si, &pi);
  stats::count("spawn", spawned ? "success" : "fail");
  return spawned ? 0 : 1;
#else
  if (pid_t pid = fork())
  {
    bool spawned = waitpid(pid, NULL, WNOHANG) >= 0;
    stats::count("spawn", spawned ? "success" : "fail");
    return spawned ? 0 : 1;
  }
  else
  {
    const size_t argc = parent.size();
    char **argv = new char *[argc + 1];
    for (int i = 0; i < argc; ++i)
      argv[i] = (char *)parent[i].c_str();
    argv[argc] = NULL;

    log << "Restarting parent: ";
    for (int i = 0; i < argc; ++i)
      log << (argv[i] ?: "NULL") << " ";
    log << std::endl;
    if (-1 == execv(argv[0], argv))
      log << "Could not spawn " << argv[0] << " errno " << errno << std::endl;
  }
  return 0;
#endif
}

int process_report(int argc, char **argv)
{
  log << "Started as ";
  for (int i = 0; i < argc; ++i)
    log << argv[i] << " ";
  log << std::endl << "Logging to " << log.getPath() << std::endl;

  Configuration cfg(argc, argv);
  log << "Configuration:\n" << cfg << std::endl;
  if (!cfg.isValid())
    return 1;

  stats::init(cfg);
  stats::count("report", "start");

  ui::Response r = cfg.silent ? ui::Response::Send : ui::query(cfg);
  if (r != ui::Response::Send)
  {
    stats::count("report", "decline");
    log << "User refused to submit report" << std::endl;
    return (r == ui::Response::Restart) ? spawn(cfg.parent) : 0;
  }
  cfg.addComment("Feedback requests", cfg.allowEmail ? "yes" : "no");

  files::prepare(cfg);
  log << "User agreed to send data. Updated config:\n" << cfg << std::endl;

  std::string finalResponse;
  bool success = false;
  for (const auto &url : cfg.urls)
  {
    std::string response;
    if (upload(cfg, url, response))
    {
      log << "Report to " << url << " submitted successfully: " << response << std::endl;
      finalResponse += (finalResponse.empty() ? "" : ", ") + response;
      if (!success)
      {
        success = true;
        finalResponse = response;
      }
    }
    else
    {
      log << "Could not submit report to " << url << ": " << response << std::endl;
      if (!success && finalResponse.empty())
        finalResponse += response.substr(0, response.find('\n'));
    }
  }

  stats::count("report", success ? "success" : "fail");

  if (success)
    files::postprocess(cfg, finalResponse.substr(0, finalResponse.find(',')));
  r = cfg.silent ? ui::Response::Close : ui::notify(cfg, finalResponse, success);
  return (r == ui::Response::Restart) ? spawn(cfg.parent) : 0;
}

} // namespace breakpad
