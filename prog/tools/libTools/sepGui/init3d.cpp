// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "init3d.h"
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <sepGui/wndGlobal.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_texMgr.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_restart.h>
#include <startup/dag_loadSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/setProgGlobals.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <util/dag_string.h>
#include <debug/dag_debug.h>

class De3GuiMgrDrawFps : public IGeneralGuiManager
{
public:
  virtual void beforeRender(int /*ticks_usec*/) {}

  virtual bool canCloseNow()
  {
    DEBUG_CTX("closing window");
    debug_flush(false);
    return true;
  }

  virtual void drawFps(float, float, float) {}
};


extern void default_crt_init_kernel_lib();
extern void default_crt_init_core_lib();
extern void messagebox_win_report_fatal_error(const char *title, const char *msg, const char *call_stack);

namespace workcycle_internal
{
intptr_t main_window_proc(void *, unsigned, uintptr_t, intptr_t);
}

static LRESULT FAR PASCAL d3dWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  switch (message)
  {
    // Prevent the window from being resized or becoming visible.
    // For example in case of device reset d3d::reset_device() would make it visible.
    case WM_WINDOWPOSCHANGING:
    {
      WINDOWPOS *windowPos = reinterpret_cast<WINDOWPOS *>(lParam);
      windowPos->x = 0;
      windowPos->y = 0;
      windowPos->cx = GetSystemMetrics(SM_CXSCREEN);
      windowPos->cy = GetSystemMetrics(SM_CYSCREEN);
      windowPos->flags = (windowPos->flags & ~SWP_SHOWWINDOW) | SWP_HIDEWINDOW;
      return 0;
    }

    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
      SetCursor(NULL);
      BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      return 1;
  }
  return workcycle_internal::main_window_proc(hWnd, message, wParam, lParam);
}

bool tools3d::inited = false;

bool tools3d::init(const char *drv_name, const DataBlock *blkTexStreaming)
{
  if (inited)
    return true;

  // Register a window class
  WNDCLASSW WndClass;
  memset(&WndClass, 0, sizeof(WndClass));
  WndClass.style = 0;
  WndClass.lpfnWndProc = d3dWindowProc;
  WndClass.cbClsExtra = 0;
  WndClass.cbWndExtra = 0;
  WndClass.hInstance = (HINSTANCE)win32_get_instance();
  WndClass.hIcon = NULL;
  WndClass.hCursor = NULL;
  WndClass.hbrBackground = NULL;
  WndClass.lpszMenuName = NULL;
  WndClass.lpszClassName = L"engine3d-srv";
  RegisterClassW(&WndClass);

  // create invisible window (sized as desktop)
  void *handle = CreateWindowExW(0, L"engine3d-srv", NULL, 0, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL,
    NULL, (HINSTANCE)win32_get_instance(), NULL);

  // perform dagor init
  win32_set_main_wnd(handle);

  dgs_load_settings_blk(true, String(260, "%s/../commonData/startup_editors.blk", sgg::get_exe_path_full()), nullptr);

  DataBlock *global_settings_blk = const_cast<DataBlock *>(::dgs_get_settings());

  // Some textures are re-created when the resolution of the view changes,
  // and they might not be used, so to avoid the "texture ... was destroyed
  // but was never used in rendering" messages and messages reporting about
  // the view size change itself, we disable logging those messages in tools.
  global_settings_blk->addBlock("debug")->setBool("view_resizing_related_logging_enabled", false);

  // Do not allow window occlusion checking because here (in tools3d::init) there is a desktop sized invisible window
  // created that always occludes daEditorX's main window.
  if (DataBlock *dx_blk = global_settings_blk->addBlock("directx"))
  {
    dx_blk->setBool("winOcclusionCheckEnabled", false);
    dx_blk->setInt("max_genmip_tex_sz", 4096);
    dx_blk->setBool("immutable_textures", dx_blk->getBool("immutable_textures", false));
    dx_blk->setInt("inline_vb_size", dx_blk->getInt("inline_vb_size", 32 << 20));
    dx_blk->setInt("inline_ib_size", dx_blk->getInt("inline_ib_size", 2 << 20));
  }

  if (DataBlock *video_blk = global_settings_blk->addBlock("video"))
  {
    video_blk->setBool("windowed_ext", true);
    video_blk->setStr("mode", "windowed");
    video_blk->setInt("min_target_size", video_blk->getInt("min_target_size", 0));
    video_blk->setStr("instancing", video_blk->getStr("instancing", "auto"));
    video_blk->setStr("driver", drv_name ? drv_name : video_blk->getStr("driver", "auto"));
  }

  global_settings_blk->addBlock("physx")->setBool("disable_hardware", true);

  ::dgs_limit_fps = true;
  ::dgs_dont_use_cpu_in_background = true;

  if (blkTexStreaming)
  {
    global_settings_blk->removeBlock("texStreaming");
    global_settings_blk->addNewBlock(blkTexStreaming, "texStreaming")->setInt("forceGpuMemMB", 1);
    init_managed_textures_streaming_support(-1);
  }

  if (!d3d::init_driver())
    DAG_FATAL("Error initializing 3D driver:\n%s", d3d::get_last_error());

  dgs_set_window_mode(WindowMode::WINDOWED_NO_BORDER);

  if (!d3d::init_video(win32_get_instance(), NULL, NULL, 0, handle, handle, 0, "", NULL))
    return false;

  ::dagor_common_startup();
  ::startup_game(RESTART_ALL);

  ::dagor_gui_manager = new De3GuiMgrDrawFps;

  // clear backbuffer once and forever
  {
    d3d::GpuAutoLock gpuLock;

    int targetW, targetH;
    d3d::set_render_target();
    d3d::get_target_size(targetW, targetH);
    d3d::setview(0, 0, targetW, targetH, 0, 1);
    d3d::clearview(CLEAR_TARGET, E3DCOLOR(10, 10, 64, 0), 0, 0);
  }

  enable_tex_mgr_mt(true, 64 << 10);
  inited = true;
  return true;
}

void tools3d::destroy()
{
  if (!inited)
    return;
  debug("+++ destroy 3d");
  d3d::window_destroyed(win32_get_main_wnd());

  del_it(::dagor_gui_manager);

  debug("destroy 3d renderer");
  ::shutdown_game(RESTART_ALL);
  debug("release 3d resource");
  d3d::release_driver();
  DestroyWindow((HWND)win32_get_main_wnd());
  inited = false;
}
