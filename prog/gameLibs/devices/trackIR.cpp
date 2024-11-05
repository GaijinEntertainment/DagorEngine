// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "devices/trackIR.h"

#include <windows.h>
#include "npClient.h"

#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <math/dag_mathBase.h>

namespace trackir
{
static HMODULE trackir_client_dll = (HMODULE)NULL;
}


#define DBGRESULT(msg, result) debug("TrackIR: " msg "(%d)", (int)result)
#define CALL_NP(func, ...)     (((func) && trackir::trackir_client_dll) ? ((*(func))(__VA_ARGS__)) : NP_ERR_DLL_NOT_FOUND)


namespace trackir
{
static bool exists = false;
static bool is_registered = false;
static bool is_started = false;
static bool active = false;

static const long DEV_ID = 1878;
static const long APP_ID_HIGH = 0;
static const long APP_ID_LOW = 0;

static const int MAX_ANGLE_VALUE = 16383;
static const int MAX_POS_VALUE = 16383;
static const int MAX_RAW_POS_VALUE = 256;
static const int MAX_RAW_SMOOTH_POS_VALUE = 32766;

static int frame_signature = 0;
static int stale_frames = 0;
static Tab<TRACKIRDATA> prev_frames;
static int prev_frames_head = 0;

static TrackData current_track_data;

static PF_NP_REGISTERWINDOWHANDLE NPRegisterWindowHandle = NULL;
static PF_NP_UNREGISTERWINDOWHANDLE NPUnregisterWindowHandle = NULL;
static PF_NP_REGISTERPROGRAMPROFILEID NPRegisterProgramProfileID = NULL;
static PF_NP_QUERYVERSION NPQueryVersion = NULL;
static PF_NP_REQUESTDATA NPRequestData = NULL;
static PF_NP_GETSIGNATURE NPGetSignature = NULL;
static PF_NP_GETDATA NPGetData = NULL;
static PF_NP_STARTDATATRANSMISSION NPStartDataTransmission = NULL;
static PF_NP_STOPDATATRANSMISSION NPStopDataTransmission = NULL;

static bool get_dll_location(String &dll_path)
{
  dll_path = "";
  HKEY pKey = NULL;
  if (
    ::RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\NaturalPoint\\NATURALPOINT\\NPClient Location", 0, KEY_READ, &pKey) != ERROR_SUCCESS)
  {
    debug("TrackIR: DLL Location key not present!");
    return false;
  }

  bool res = false;
  DWORD dwSize = 0;
  if (RegQueryValueEx(pKey, "Path", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS)
  {
    char *path = new char[dwSize + 1];
    if (RegQueryValueEx(pKey, "Path", NULL, NULL, (LPBYTE)path, &dwSize) == ERROR_SUCCESS)
    {
      dll_path = path;
      res = true;
    }
    else
    {
      debug("TrackIR: Error reading location key!");
      res = false;
    }
    delete[] path;
  }
  ::RegCloseKey(pKey);
  return res;
}

static void acquire_functions(HMODULE dll)
{
#define ACQUIRE_FUNC(type, name)                       \
  NP##name = (type)::GetProcAddress(dll, "NP_" #name); \
  G_ASSERT(NP##name)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4191)
#endif

  ACQUIRE_FUNC(PF_NP_GETSIGNATURE, GetSignature);
  ACQUIRE_FUNC(PF_NP_REGISTERWINDOWHANDLE, RegisterWindowHandle);
  ACQUIRE_FUNC(PF_NP_UNREGISTERWINDOWHANDLE, UnregisterWindowHandle);
  ACQUIRE_FUNC(PF_NP_REGISTERPROGRAMPROFILEID, RegisterProgramProfileID);
  ACQUIRE_FUNC(PF_NP_QUERYVERSION, QueryVersion);
  ACQUIRE_FUNC(PF_NP_REQUESTDATA, RequestData);
  ACQUIRE_FUNC(PF_NP_GETDATA, GetData);
  ACQUIRE_FUNC(PF_NP_STARTDATATRANSMISSION, StartDataTransmission);
  ACQUIRE_FUNC(PF_NP_STOPDATATRANSMISSION, StopDataTransmission);

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#undef ACQUIRE_FUNC
}

static bool try_register()
{
  NPRESULT result = CALL_NP(NPRegisterProgramProfileID, DEV_ID);
  return result == NP_OK;
}

static void free_library()
{
  if (trackir_client_dll)
  {
    ::FreeLibrary(trackir_client_dll);
    trackir_client_dll = NULL;
  }
}

template <typename F, typename T>
static inline NPRESULT safe_call_1(F func, T *param)
{
  NPRESULT result = NP_ERR_DEVICE_NOT_PRESENT;
#ifndef SEH_DISABLED
  __try
  {
    result = CALL_NP(func, param);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return NP_ERR_DEVICE_NOT_PRESENT;
  }
#else
  result = CALL_NP(func, param);
#endif
  return result;
}

void init(int smoothing)
{
  String dllPath;

  exists = false;
  is_registered = false;
  is_started = false;

  G_ASSERT(smoothing >= 0);
  prev_frames.resize(smoothing);
  mem_set_ff(prev_frames);
  prev_frames_head = 0;

  if (!get_dll_location(dllPath))
  {
    exists = false;
    return;
  }

#if _TARGET_64BIT
  dllPath += "\\NPClient64.dll";
#else
  dllPath += "\\NPClient.dll";
#endif
  trackir_client_dll = ::LoadLibrary(dllPath);
  if (!trackir_client_dll)
  {
    debug("TrackIR: Error loading DLL");
    return;
  }

  acquire_functions(trackir_client_dll);

  { // checking version
    unsigned short ClientVer;
    NPRESULT result = safe_call_1(NPQueryVersion, &ClientVer);
    if (result == NP_OK)
      debug("TrackIR: version %d.%02d", ClientVer >> 8, ClientVer & 0x00FF);
    else
    {
      DBGRESULT("TrackIR: Error querying trackIR software version.", result);
      return free_library();
    }
  }

  { // checking signature
    SIGNATUREDATA pSignature;
    SIGNATUREDATA verifySignature;
    strcpy(verifySignature.DllSignature,
      "precise head tracking\n put your head into the game\n now go look around\n\n Copyright EyeControl Technologies");
    strcpy(verifySignature.AppSignature,
      "hardware camera\n software processing data\n track user movement\n\n Copyright EyeControl Technologies");
    NPRESULT result = safe_call_1(NPGetSignature, &pSignature);
    if (result != NP_OK)
    {
      DBGRESULT("TrackIR: Error on trying to check signature", result);
      return free_library();
    }

    if ((strcmp(verifySignature.DllSignature, pSignature.DllSignature)) ||
        (strcmp(verifySignature.AppSignature, pSignature.AppSignature)))
    {
      debug("TrackIR: Signature mismatch.");
      return free_library();
    }
  }

  // now looks like we have trackIR device, setting it up
  exists = true;

  {
    HWND hwnd = (HWND)win32_get_main_wnd();
    NPRESULT result = CALL_NP(NPRegisterWindowHandle, hwnd);
    if (result != NP_OK)
      DBGRESULT("Error registering window handle.", result);
    is_registered = result == NP_OK;
  }

  // Choose the Axes that you want tracking data for
  unsigned int dataFields = 0;

  // Rotation Axes
  dataFields |= NPPITCH;
  dataFields |= NPYAW;
  dataFields |= NPROLL;

  // Translation Axes
  dataFields |= NPX;
  dataFields |= NPY;
  dataFields |= NPZ;

  CALL_NP(NPRequestData, dataFields);

  memset(&current_track_data, 0, sizeof(TrackData));

  is_registered &= try_register();
}

void shutdown()
{
  stop();
  CALL_NP(NPUnregisterWindowHandle);
  free_library();
}

bool is_installed() { return exists; }

bool is_enabled() { return exists && is_registered && is_started; }

bool is_active() { return is_enabled() && active; }


TrackData &get_data() { return current_track_data; }

void update()
{
  if (!is_enabled())
    return;

  // could try to register here
  TRACKIRDATA newTid;
  memset(&newTid, 0, sizeof(newTid)); // to workaround unitilized warning
  NPRESULT result = CALL_NP(NPGetData, &newTid);
  if (result == NP_OK)
  {
    newTid.dwNPIOData = 0;
    active = newTid.wNPStatus == NPSTATUS_REMOTEACTIVE;
    if (newTid.wNPStatus == NPSTATUS_REMOTEACTIVE)
    {
      if (frame_signature != newTid.wPFrameSignature)
      {
        // debug("Rotation : NPPitch = %04.02f, NPYaw = %04.02f, NPRoll = %04.02f \r\n"
        //          "Translation : NPX = %04.02f, NPY = %04.02f, NPZ = %04.02f \r\n"
        //          "Information NPStatus = %d, Frame = %d",
        //      newTid.fNPPitch, newTid.fNPYaw, newTid.fNPRoll,
        //      newTid.fNPX, newTid.fNPY, newTid.fNPZ, newTid.wNPStatus, newTid.wPFrameSignature);

        TRACKIRDATA tid = newTid;
        if (prev_frames.size() > 0)
        {
          int numValidFrames = 1;
          for (int prevFrameNo = 0; prevFrameNo < prev_frames.size(); prevFrameNo++)
          {
            TRACKIRDATA &prev = prev_frames[prevFrameNo];
            if (prev.wNPStatus == NPSTATUS_REMOTEACTIVE)
            {
              tid.fNPYaw += prev.fNPYaw;
              tid.fNPPitch += prev.fNPPitch;
              tid.fNPRoll += prev.fNPRoll;
              tid.fNPX += prev.fNPX;
              tid.fNPY += prev.fNPY;
              tid.fNPZ += prev.fNPZ;
              numValidFrames++;
            }
          }
          tid.fNPYaw /= numValidFrames;
          tid.fNPPitch /= numValidFrames;
          tid.fNPRoll /= numValidFrames;
          tid.fNPX /= numValidFrames;
          tid.fNPY /= numValidFrames;
          tid.fNPZ /= numValidFrames;
          prev_frames[prev_frames_head++] = newTid;
          prev_frames_head %= prev_frames.size();
        }


        current_track_data.relYaw = clamp(tid.fNPYaw / (float)MAX_ANGLE_VALUE, -1.f, 1.f);
        current_track_data.relPitch = clamp(tid.fNPPitch / (float)MAX_ANGLE_VALUE, -1.f, 1.f);
        current_track_data.relRoll = clamp(tid.fNPRoll / (float)MAX_ANGLE_VALUE, -1.f, 1.f);

        current_track_data.yaw = current_track_data.relYaw * PI;
        current_track_data.pitch = current_track_data.relPitch * PI;
        current_track_data.roll = current_track_data.relRoll * PI;

        current_track_data.relX = clamp(tid.fNPX / (float)MAX_POS_VALUE, -1.f, 1.f);
        current_track_data.relY = clamp(tid.fNPY / (float)MAX_POS_VALUE, -1.f, 1.f);
        current_track_data.relZ = clamp(tid.fNPZ / (float)MAX_POS_VALUE, -1.f, 1.f);

        current_track_data.deltaX = clamp(tid.fNPDeltaX / (float)MAX_RAW_POS_VALUE, -1.f, 1.f);
        current_track_data.deltaY = clamp(tid.fNPDeltaY / (float)MAX_RAW_POS_VALUE, -1.f, 1.f);
        current_track_data.deltaZ = clamp(tid.fNPDeltaZ / (float)MAX_RAW_POS_VALUE, -1.f, 1.f);

        current_track_data.smoothX = clamp(tid.fNPSmoothX / (float)MAX_RAW_SMOOTH_POS_VALUE, 0.f, 1.f);
        current_track_data.smoothY = clamp(tid.fNPSmoothY / (float)MAX_RAW_SMOOTH_POS_VALUE, 0.f, 1.f);
        current_track_data.smoothZ = clamp(tid.fNPSmoothZ / (float)MAX_RAW_SMOOTH_POS_VALUE, 0.f, 1.f);

        current_track_data.rawX = clamp(tid.fNPRawX / (float)MAX_RAW_POS_VALUE, 0.f, 1.f);
        current_track_data.rawY = clamp(tid.fNPRawY / (float)MAX_RAW_POS_VALUE, 0.f, 1.f);
        current_track_data.rawZ = clamp(tid.fNPRawZ / (float)MAX_RAW_POS_VALUE, 0.f, 1.f);

        frame_signature = tid.wPFrameSignature;
        stale_frames = 0;
      }
      else
      {
        if (stale_frames > 30)
        {
          // debug("No New Data. Paused or Not Tracking?", stale_frames);
        }
        else
        {
          stale_frames++;
          // debug("No New Data for %d frames", stale_frames);
        }
      }
    }
    else
    {
      // debug("User Disabled");
    }
  }
}

void start()
{
  if (!exists || !is_registered)
    return;

  if (!is_started)
  {
    NPRESULT res = CALL_NP(NPStartDataTransmission);
    if (res != NP_OK)
      debug("TrackIR: Error on NPStartDataTransmission");
    is_started = true;
    frame_signature = 0;
    stale_frames = 0;
  }
}

void stop()
{
  if (!exists || !is_registered)
    return;

  if (is_started)
  {
    NPRESULT res = CALL_NP(NPStopDataTransmission);
    if (res != NP_OK)
      debug("TrackIR: Error on NPStopDataTransmission");
    is_started = false;
  }
}

void toggle_debug_draw()
{ /* STUB */
}
void draw_debug(HudPrimitives *)
{ /* STUB */
}
} // namespace trackir
