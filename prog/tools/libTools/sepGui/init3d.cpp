#include "init3d.h"
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <sepGui/wndGlobal.h>
#include <workCycle/dag_startupModules.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <3d/dag_drv3d.h>
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

  virtual void act() {}

  virtual void draw(int /*ticks*/) {}
  virtual void changeRenderSettings(bool & /*draw_scene*/, bool & /*clr_scene*/) {}
  virtual bool canActScene() { return true; }
  virtual bool canCloseNow()
  {
    debug_ctx("closing window");
    debug_flush(false);
    return true;
  }

  virtual void drawCopyrightMessage(int) {}
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
  ShowWindow((HWND)handle, 0);

  // perform dagor init
  win32_set_main_wnd(handle);

  dgs_load_settings_blk();

  DataBlock *global_settings_blk = const_cast<DataBlock *>(::dgs_get_settings());
  DataBlock b(String(260, "%s/../commonData/startup_editors.blk", sgg::get_exe_path_full()));

  global_settings_blk->addBlock("directx")->setInt("max_genmip_tex_sz", 4096);
  global_settings_blk->addBlock("directx")->setBool("immutable_textures",
    b.getBlockByNameEx("directx")->getBool("immutable_textures", false));
  global_settings_blk->addBlock("directx")->setInt("inline_vb_size",
    b.getBlockByNameEx("directx")->getInt("inline_vb_size", 32 << 20));
  global_settings_blk->addBlock("directx")->setInt("inline_ib_size", b.getBlockByNameEx("directx")->getInt("inline_ib_size", 2 << 20));
  if (int bones = b.getBlockByNameEx("directx")->getInt("bonesCount", 0))
    global_settings_blk->addBlock("directx")->setInt("bonesCount", bones);

  global_settings_blk->addBlock("video")->setBool("windowed_ext", true);
  global_settings_blk->addBlock("video")->setStr("mode", "windowed");
  global_settings_blk->addBlock("video")->setInt("min_target_size", b.getBlockByNameEx("video")->getInt("min_target_size", 0));
  global_settings_blk->addBlock("video")->setStr("instancing", b.getBlockByNameEx("video")->getStr("instancing", "auto"));
  global_settings_blk->addBlock("video")->setStr("driver",
    drv_name ? drv_name : b.getBlockByNameEx("video")->getStr("driver", "auto"));
  global_settings_blk->addBlock("physx")->setBool("disable_hardware", true);
  if (const DataBlock *b2 = b.getBlockByName("stub3d"))
    global_settings_blk->addNewBlock(b2, "stub3d");

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

  dgs_set_window_mode(WindowMode::WINDOWED);

  if (!d3d::init_video(win32_get_instance(), NULL, NULL, 0, handle, handle, 0, "", NULL))
    return false;

  ShowWindow((HWND)handle, 0); // should not be needed unless d3d::init_video() forces WS_VISIBLE

  ::dagor_common_startup();
  ::startup_game(RESTART_ALL);

  ::dagor_gui_manager = new De3GuiMgrDrawFps;

  // clear backbuffer once and forever
  int targetW, targetH;
  d3d::set_render_target();
  d3d::get_target_size(targetW, targetH);
  d3d::setview(0, 0, targetW, targetH, 0, 1);
  d3d::clearview(CLEAR_TARGET, E3DCOLOR(10, 10, 64, 0), 0, 0);

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
