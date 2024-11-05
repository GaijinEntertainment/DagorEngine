// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if DAGOR_DBGLEVEL > 0

#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>


static void add_local_profile_server_url()
{
  DataBlock *debugBlk = const_cast<DataBlock *>(::dgs_get_settings())->addBlock("debug");
  if (!debugBlk->blockExists("profile_server"))
  {
    DataBlock profileServerSettings;
    profileServerSettings.addBlock("servers")->addStr("url", "http://127.0.0.1:8080/json-rpc");
    debugBlk->addNewBlock(&profileServerSettings, "profile_server");
    debug("Added url override for the local profile server");
  }
  else
  {
    debug("Profile server settings override already exists in the debug block, keeping it intact");
  }
}


#if _TARGET_PC_WIN

#include <windows.h>
#include <shlwapi.h>
#include <strsafe.h>


void start_local_profile_server()
{
  PROCESS_INFORMATION p_info;
  STARTUPINFOW s_info;

  WCHAR pathBuf[MAX_PATH];
  memset(pathBuf, 0, sizeof(pathBuf));
  DWORD pathBufSize = countof(pathBuf);
  if (QueryFullProcessImageNameW(GetCurrentProcess(), 0, pathBuf, &pathBufSize) == 0)
  {
    logerr("Cannot determine the current module file path");
    return;
  }

  PathRemoveFileSpecW(pathBuf);
  PathAppendW(pathBuf, L"..\\..\\..\\tools\\profileServer");

  WCHAR profileServerDir[MAX_PATH];
  StringCbCopyW(profileServerDir, sizeof(profileServerDir) /* in bytes */, pathBuf);

  PathAppendW(pathBuf, L"profile-srv-dev.exe");

  memset(&s_info, 0, sizeof(s_info));
  memset(&p_info, 0, sizeof(p_info));
  s_info.cb = sizeof(s_info);

  if (CreateProcessW(NULL, pathBuf, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, profileServerDir, &s_info, &p_info))
  {
    debug("Started local profile server");
    CloseHandle(p_info.hThread);
    CloseHandle(p_info.hProcess);
    add_local_profile_server_url();
  }
  else
  {
    logerr("Failed to start local file server: %d", (int)GetLastError());
  }
}

#else // _TARGET_PC_WIN

void start_local_profile_server()
{
  debug("Starting local profile server is not supported on this platform, adding url override");
  add_local_profile_server_url();
}

#endif // _TARGET_PC_WIN

#endif // DAGOR_DBGLEVEL > 0
