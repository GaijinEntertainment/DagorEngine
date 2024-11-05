// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <windowsx.h>

#include <dinput.h>
#include <process.h>
#include <synchapi.h>

#include <drv/hid/dag_hiDInput.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <generic/dag_initOnDemand.h>
#include <startup/dag_restart.h>
#include <debug/dag_debug.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#define DI_CREATE_TIMEOUT 2000 // 2 sec

static DWORD __stdcall di8_create_in_thread(void *arg)
{
  HINSTANCE hinst = (HINSTANCE)win32_get_instance();
  HRESULT hr = DirectInput8Create(hinst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)arg, NULL);
  static_assert(sizeof(HRESULT) <= sizeof(DWORD));
  return (DWORD)hr;
}

static HRESULT dinput_init()
{
  if (!HumanInput::dinput8)
  {
    static IDirectInput8 *di8tmp = nullptr;
    HANDLE hThread = (HANDLE)CreateThread(NULL, 0, di8_create_in_thread, &di8tmp, 0, NULL); //-V513
    HRESULT di_create_result = E_FAIL;
    if (DAGOR_UNLIKELY(WaitForSingleObject(hThread, DI_CREATE_TIMEOUT) == WAIT_TIMEOUT))
    {
      // To consider: add delayed action when it's actually completed?
      di_create_result = RPC_E_TIMEOUT;
      logwarn("DirectInput8Create timed out after waiting %d msec", DI_CREATE_TIMEOUT);
    }
    else
      GetExitCodeThread(hThread, (DWORD *)&di_create_result);
    CloseHandle(hThread);

    if (FAILED(di_create_result))
    {
      HumanInput::printHResult(__FILE__, __LINE__, "DirectInputCreateEx", di_create_result);
      return di_create_result;
    }

    HumanInput::dinput8 = di8tmp;
  }

  return DI_OK;
}

static void dinput_close()
{
  if (HumanInput::dinput8)
  {
    HumanInput::dinput8->Release();
    HumanInput::dinput8 = NULL;
  }
}

class DInput8RestartProc : public SRestartProc
{
public:
  virtual const char *procname() { return "DINPUT8"; }
  DInput8RestartProc() : SRestartProc(RESTART_INPUT | RESTART_VIDEO) {}

  void startup() { dinput_init(); }
  void shutdown() { dinput_close(); }
};
static InitOnDemand<DInput8RestartProc> dinput_rproc;


void HumanInput::startupDInput()
{
  // install only once
  if ((DInput8RestartProc *)dinput_rproc)
    return;

  // init and install restart proc
  dinput_rproc.demandInit();
  add_restart_proc(dinput_rproc);
}

void HumanInput::printHResult(const char *file, int ln, const char *prefix, int hr)
{
#if DAGOR_DBGLEVEL > 0
  LPVOID lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr,
    MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
  debug("%s,%d: %s=0x%X %s", file, ln, prefix, hr, lpMsgBuf);
  LocalFree(lpMsgBuf);
#else
  debug("%s,%d: %s=0x%X", file, ln, prefix, hr);
#endif
}
