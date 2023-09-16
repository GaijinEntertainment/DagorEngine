#include "devices/jetSeat.h"
#if _TARGET_PC_WIN

#ifndef INITGUID
#define INITGUID
#endif

#include <guiddef.h>
#include "IUberwoorf.h"


#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>

namespace jetseat
{
#if _TARGET_PC_WIN
static int status = EJS_NO_DEVICE;

static IUberwoorf *uberwoorfInterface = NULL;

struct SeatVibroState
{
  float amplitude;
  UINT handle;
  UINT zone;

  SeatVibroState(UINT zone_) : amplitude(0.f), handle(0), zone(zone_) {}

  bool create(UINT effectHandle)
  {
    UINT duration[2] = {50, 50};
    BYTE ampl[2] = {0, 0};
    UINT validation = UW_VALIDATION_NO_ERROR;
    HRESULT hValidationResult = uberwoorfInterface->uwValidateVibration(&validation, zone, 2, duration, ampl);
    HRESULT hr = uberwoorfInterface->uwSetVibration(effectHandle, zone, duration, ampl, 2, &handle);
    return !FAILED(hr);
  }

  void update(float dt, UINT effectHandle)
  {
    UINT duration[2] = {50, 50};
    BYTE ampl[2] = {0, 0};
    ampl[0] = clamp(int(amplitude * 255), 0, 255);
    ampl[1] = clamp(int(amplitude * 255), 0, 255);
    if (ampl[1] > 0)
      duration[1] = INFINITE;
    UINT validation = UW_VALIDATION_NO_ERROR;
    HRESULT hValidationResult = uberwoorfInterface->uwValidateVibration(&validation, zone, 2, duration, ampl);
    G_ASSERTF(!FAILED(hValidationResult) && validation == UW_VALIDATION_NO_ERROR, "Vibro validation error %d", validation);
    if (!FAILED(hValidationResult) && validation == UW_VALIDATION_NO_ERROR)
      uberwoorfInterface->uwUpdateVibration(effectHandle, handle, zone, duration, ampl, 2);
  }
};

struct SeatEffectState
{
  UINT handle;
  bool loaded;
  SeatVibroState leftSeat;
  SeatVibroState rightSeat;
  SeatVibroState leftWaist;
  SeatVibroState rightWaist;
  SeatVibroState leftBack;
  SeatVibroState rightBack;

  SeatEffectState() :
    handle(0),
    loaded(false),
    leftSeat(UW_SIT_LEFT),
    rightSeat(UW_SIT_RIGHT),
    leftWaist(UW_BACK_LOW_LEFT),
    rightWaist(UW_BACK_LOW_RIGHT),
    leftBack(UW_BACK_MID_LEFT),
    rightBack(UW_BACK_MID_RIGHT)
  {}

  bool create()
  {
    HRESULT hrEff = uberwoorfInterface->uwCreateEffect(&handle);
    if (FAILED(hrEff))
      return false;

    if (!leftSeat.create(handle) || !rightSeat.create(handle) || !leftWaist.create(handle) || !rightWaist.create(handle) ||
        !leftBack.create(handle) || !rightBack.create(handle))
      return false;

    uberwoorfInterface->uwStartEffect(handle, INFINITE);
    loaded = true;
    return true;
  }

  void destroy()
  {
    uberwoorfInterface->uwStopEffect(handle);
    uberwoorfInterface->uwDeleteEffect(handle);
    loaded = false;
  }

  void update(float dt)
  {
    if (!loaded)
      return;

    leftSeat.update(dt, handle);
    rightSeat.update(dt, handle);
    leftWaist.update(dt, handle);
    rightWaist.update(dt, handle);
    leftBack.update(dt, handle);
    rightBack.update(dt, handle);
  }

  void setVibro(float lSeat, float rSeat, float lWaist, float rWaist, float lBack, float rBack)
  {
    leftSeat.amplitude = clamp(lSeat, 0.f, 1.f);
    rightSeat.amplitude = clamp(rSeat, 0.f, 1.f);
    leftWaist.amplitude = clamp(lWaist, 0.f, 1.f);
    rightWaist.amplitude = clamp(rWaist, 0.f, 1.f);
    leftBack.amplitude = clamp(lBack, 0.f, 1.f);
    rightBack.amplitude = clamp(rBack, 0.f, 1.f);
  }
};

static SeatEffectState persistentEffect;

void init()
{
  // Init COM
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr))
  {
    debug("Cannot initialize COM interface");
    return;
  }

  // Create COM instance
  hr = CoCreateInstance(CLSID_Uberwoorf, NULL, CLSCTX_INPROC_SERVER, IID_IUberwoorf, (void **)&uberwoorfInterface);
  if (FAILED(hr) || !uberwoorfInterface)
  {
    debug("Cannot create COM instance");
    return;
  }

  // Open interface
  UINT openStatus = E_FAIL;
  HRESULT openRes = uberwoorfInterface->uwOpen(&openStatus);
  if (openRes != S_OK)
  {
    status = openStatus == UW_STATUS_IN_USE          ? EJS_ALREADY_IN_USE
             : openStatus == UW_STATUS_NOT_CONNECTED ? EJS_NOT_CONNECTED
                                                     : EJS_NO_DEVICE;
    debug("Cannot initialize jetseat, status = %d", status);
    return;
  }

  // Reset interface
  uberwoorfInterface->uwReset();

  // Create and start persistent effect
  if (!persistentEffect.create())
  {
    debug("Cannot create persistent effect");
    return;
  }

  // Done
  status = EJS_OK;
}

void shutdown()
{
  if (status != EJS_OK)
    return;

  if (uberwoorfInterface)
  {
    persistentEffect.destroy();
    uberwoorfInterface->uwReset();
    uberwoorfInterface->uwClose();
    uberwoorfInterface->Release();
    uberwoorfInterface = NULL;
    CoUninitialize();
  }
  status = EJS_NO_DEVICE;
}

void set_vibro(float leftSeat, float rightSeat, float leftWaist, float rightWaist, float leftBack, float rightBack)
{
  persistentEffect.setVibro(leftSeat, rightSeat, leftWaist, rightWaist, leftBack, rightBack);
}

void update(float dt) { persistentEffect.update(dt); }
bool is_enabled() { return status == EJS_OK; }
#else
inline void init() {}
inline void shutdown() {}

inline bool is_enabled() { return false; }
inline bool is_installed() { return false; }
inline int get_status() { return EJS_NO_DEVICE; }

inline void set_vibro_strength(float) {}
inline void set_vibro(float, float, float, float, float, float) {}

inline void update(float) {}
#endif
}; // namespace jetseat
#endif
