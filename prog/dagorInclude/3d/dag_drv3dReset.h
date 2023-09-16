//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct IDrv3DResetCB
{
  virtual void beforeReset(bool full_reset) = 0;
  virtual void afterReset(bool full_reset) = 0;
  virtual void resetCounter() {}
  virtual void fullscreenStateRestored() {}
};

struct IDrv3DDeviceLostCB
{
  virtual void onDeviceLost() = 0;
};

extern IDrv3DDeviceLostCB *ext_drv3d_device_lost_handler;

void set_3d_device_reset_callback(IDrv3DResetCB *handler);
void set_3d_device_lost_callback(IDrv3DDeviceLostCB *handler);

//! returns true if drawing is allowed (device is not lost)
#if _TARGET_PC_WIN | _TARGET_ANDROID
bool check_and_restore_3d_device();
#else
inline bool check_and_restore_3d_device() { return true; }
#endif

void before_reset_3d_device(bool full_reset);
void after_reset_3d_device(bool full_reset);
void change_driver_reset_request(bool &out_apply_after_reset_device, bool mode_reset = true);
void zero_reset_3d_device_counter();

extern bool dagor_d3d_force_driver_reset;
extern bool dagor_d3d_force_driver_mode_reset;
extern bool dagor_d3d_notify_fullscreen_state_restored;

unsigned int get_d3d_reset_counter();      // returns current reset generation, never 0
unsigned int get_d3d_full_reset_counter(); // returns current 'full' reset generation, never 0

void report_device_reset(int resetCount, int lastResetTime);

struct D3dResetQueue
{
  typedef void (*reset_func)(bool full_reset);

  const D3dResetQueue *next = nullptr; // single-linked list
  reset_func func = nullptr;
  const char *name = nullptr; // name

  D3dResetQueue(const char *n, reset_func f, bool add_to_after_reset) : name(n), func(f)
  {
    if (add_to_after_reset)
    {
      next = tailAfterReset;
      tailAfterReset = this;
    }
    else
    {
      next = tailBeforeReset;
      tailBeforeReset = this;
    }
  }

  static D3dResetQueue *tailBeforeReset, *tailAfterReset;
  static void perform_reset_queue(const D3dResetQueue *q, bool full_reset);
  static void perform_before_reset(bool full_reset) { perform_reset_queue(tailBeforeReset, full_reset); }
  static void perform_after_reset(bool full_reset) { perform_reset_queue(tailAfterReset, full_reset); }
};

#define DAG_D3DRQ_CC0(a, b)               a##b
#define DAG_D3DRQ_CC1(a, b)               DAG_D3DRQ_CC0(a, b)
#define REGISTER_D3D_BEFORE_RESET_FUNC(F) static D3dResetQueue DAG_D3DRQ_CC1(AutoD3dResetFunc, __LINE__)(#F, &F, false)
#define REGISTER_D3D_AFTER_RESET_FUNC(F)  static D3dResetQueue DAG_D3DRQ_CC1(AutoD3dResetFunc, __LINE__)(#F, &F, true)
