// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_PC_WIN
#include <windows.h>
#endif

#include <util/dag_stdint.h>
#include <util/dag_compilerDefs.h>

class String;

#define INLINE_VB_SIZE (1024 << 10)
#define INLINE_IB_SIZE (1024 << 10)

#define FALLBACK_SCREEN_WIDTH  1280
#define FALLBACK_SCREEN_HEIGHT 720

bool get_settings_resolution(int &width, int &height, bool &is_retina, int def_width, int def_height, bool &out_is_auto);
void select_best_dt(float cpu_dt, float gpu_dt, float &best_dt, bool &use_gpu_dt);

#if _TARGET_APPLE
bool get_settings_use_retina();
#endif

#if _TARGET_PC_WIN
void get_current_main_window_rect(int &out_def_left, int &out_def_top, int &out_def_width, int &out_def_height);
void get_current_display_screen_mode(int &out_def_left, int &out_def_top, int &out_def_width, int &out_def_height);

struct RenderWindowSettings
{
  int resolutionX;
  int resolutionY;
  int clientWidth;
  int clientHeight;
  float aspect;
  int winRectLeft;
  int winRectRight;
  int winRectTop;
  int winRectBottom;
  uint32_t winStyle;
};

void get_render_window_settings(RenderWindowSettings &p, class Driver3dInitCallback *cb = nullptr);

struct RenderWindowParams
{
  void *hinst;
  const char *wcname;
  int ncmdshow;
  void *hwnd;
  void *rwnd;
  void *icon;
  const char *title;
  void *mainProc;
};

bool set_render_window_params(RenderWindowParams &p, const RenderWindowSettings &s);

int set_display_device_mode(bool inwin, int res_x, int res_y, int scr_bpp, DEVMODE &out_devm);

bool get_monitor_info(const char *monitorName, String *friendlyMonitorName, int *uniqueIndex);
bool get_monitor_rect(const char *monitorName, RECT &monitorRect);

bool is_window_occluded(HWND window);

bool has_focus(HWND hWnd, UINT message, WPARAM wParam);

DAGOR_NOINLINE void paint_window(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, void *proc);

#endif

bool get_enable_hdr_from_settings(const char *name = nullptr);

/* Returns with the "video/monitor:t" value from config.
 nullptr result means default. */
const char *get_monitor_name_from_settings();
/* Returns with the displayName's or nullptr if it is "auto". */
const char *resolve_monitor_name(const char *displayName);

int drv_message_box(const char *utf8_text, const char *utf8_caption, int flags = 0);
