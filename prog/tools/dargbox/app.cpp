// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main.h"
#include "scriptBindings.h"
#include "vr.h"
#include "vrInput.h"
#include "gamelib/sound.h"
#include "gamelib/input.h"
#include <vr/vrGuiSurface.h>

#include <daRg/dag_guiScene.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_joystick.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_panelRenderer.h>
#include <sqModules/sqModules.h>
#include <quirrel/sqConsole/sqConsole.h>
#include <squirrel/memtrace.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <bindQuirrelEx/bindQuirrelEx.h>
#include <sound/dag_genAudio.h>

#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdblob.h>

#include <startup/dag_globalSettings.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_lock.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <debug/dag_logSys.h>
#include <startup/dag_restart.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoyData.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <gui/dag_stdGuiRender.h>
#include <debug/dag_debug.h>
#include <gui/dag_visualLog.h>
#include <memory/dag_dbgMem.h>
#include <libTools/util/fileUtils.h>
#include <workCycle/dag_wcHooks.h>
#include <stdio.h>

#include <shaders/dag_shaderBlock.h>
#include <util/dag_simpleString.h>
#include <stdlib.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_frustum.h>
#include <image/dag_texPixel.h>
#include <debug/dag_log.h>
#include <3d/dag_picMgr.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_perfTimer2.h>
#include <perfMon/dag_daProfilerSettings.h>
#include <render/debugTexOverlay.h>

#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <webui/sqDebuggerPlugin.h>
#include <quirrel/sqDebugger/sqDebugger.h>
#include <quirrel/sqDebugger/scriptProfiler.h>
#if HAS_MATCHING_MODULE
#include <quirrel/matchingModule/matchingModule.h>
#endif
#include <quirrel/nestdb/nestdb.h>

#include <daRg/soundSystem/uiSoundSystem.h>

#include <gui/dag_visConsole.h>
#include <visualConsole/dag_visualConsole.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <forceFeedback/forceFeedback.h>
#include <ioEventsPoll/ioEventsPoll.h>

#include <daScript/daScript.h>

#if _TARGET_PC || _TARGET_ANDROID || _TARGET_C3
#include <startup/dag_inpDevClsDrv.h>
#include <math/dag_bounds2.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiKeyboard.h>
#endif

#include <drv/hid/dag_hiJoystick.h>

#include <folders/folders.h>

// Error log:
//  0 - disabled
//  1 - fatal                          messages
//  2 - fatal + error                  messages
//  3 - fatal + error + warning        messages
//  4 - fatal + error + warning + note messages
#define LOG_LEVEL 3

bool scripts_used = true;
int requested_exit_code = -1;
int cur_update = 0;
int limit_updates = 0;
bool benchmark = false;
bool do_render = true;
bool use_system_cursor = false;

static int globalFrameBlockId = -1;

static DataBlock blkFx;
static console::IVisualConsoleDriver *visualConsoleDriver = NULL;


static eastl::unique_ptr<ioevents::IOEventsPoll> io_events_poll;

static darg::IGuiScene *darg_scene = NULL;
static darg::JoystickHandler *joystick_handler = NULL;

darg::IGuiScene *get_ui_scene() { return darg_scene; }

ioevents::IOEventsDispatcher *get_io_events_poll()
{
  if (!io_events_poll)
    io_events_poll = ioevents::create_poll();
  return io_events_poll.get();
}

void os_debug_break() { debug("das: script break"); }


das::Context *get_clone_context(das::Context *ctx, uint32_t category) { return new das::Context(*ctx, category); }


das::Context *get_context(int stack_size) { return new das::Context(stack_size); }

das::smart_ptr<das::FileAccess> get_file_access(char *pak)
{
  if (pak)
    return das::make_smart<das::FsFileAccess>(pak, das::make_smart<das::FsFileAccess>());
  return das::make_smart<das::FsFileAccess>();
}


PULL_CONSOLE_PROC(darg_console_handler)

static struct GuiSceneCb : public darg::IGuiSceneCallback
{
  void onScriptError(darg::IGuiScene * /*scene*/, const char * /*err_msg*/) override {}
  void onDismissError() override {}
  void onShutdownScript(HSQUIRRELVM vm) override
  {
    destroy_script_console_processor(vm);
    unbind_dargbox_script_api(vm);

#if DAGOR_DBGLEVEL > 0
    dag_sq_debuggers.shutdownDebugger(0);
#endif
  }
} gui_scene_cb;


static void init_webui(const DataBlock *debug_block)
{
  if (!debug_block)
    debug_block = dgs_get_settings()->getBlockByNameEx("debug");
  if (debug_block->getBool("http_server", true))
  {
    static webui::HttpPlugin test_http_plugins[] = {webui::profiler_http_plugin, webui::shader_http_plugin,
      webui::color_pipette_http_plugin, webui::colorpicker_http_plugin,
      {"sqdebug_darg", "squirrel debugger (daRg)", NULL, webui::on_sqdebug<0>}, NULL};
    // webui::plugin_lists[1] = aces_http_plugins;
    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = test_http_plugins;

    if (scripts_used)
      webui::plugin_lists[2] = webui::squirrel_http_plugins;

    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    webui::startup(&cfg);
  }
}

void run_autoexec(const char *pathname)
{
  if (!pathname || !*pathname)
  {
    debug("autoexec: not specified");
    return;
  }

  if (!::dd_file_exist(pathname))
    return;

  console::process_file(pathname);
}

static float deg_to_fov(float deg) { return 1.f / tanf(DEG_TO_RAD * 0.5f * deg); }

static void apply_camera_setup(const CameraSetup &camera_setup)
{
  G_ASSERT(!check_nan(camera_setup.transform));

// we assume it is orthonormalized already
#if DAGOR_DBGLEVEL > 0
  if (fabsf(lengthSq(camera_setup.transform.getcol(0)) - 1) > 1e-5f || fabsf(lengthSq(camera_setup.transform.getcol(1)) - 1) > 1e-5f ||
      fabsf(lengthSq(camera_setup.transform.getcol(2)) - 1) > 1e-5f ||
      fabsf(dot(camera_setup.transform.getcol(0), camera_setup.transform.getcol(1))) > 1e-6f ||
      fabsf(dot(camera_setup.transform.getcol(0), camera_setup.transform.getcol(2))) > 1e-6f ||
      fabsf(dot(camera_setup.transform.getcol(1), camera_setup.transform.getcol(2))) > 1e-6f)
  {
    logerr("view matrix should be orthonormalized %@ %@ %@", camera_setup.transform.getcol(0), camera_setup.transform.getcol(1),
      camera_setup.transform.getcol(2));
  }
#endif
  ::grs_cur_view.itm = camera_setup.transform;
  ::grs_cur_view.tm = orthonormalized_inverse(::grs_cur_view.itm);
  ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

  d3d::settm(TM_VIEW, ::grs_cur_view.tm);

  float fovInTan = deg_to_fov(camera_setup.fov);
  int w = 4, h = 3;
  d3d::get_screen_size(w, h);
  // we specify fov in horizontal axis, by 16:9 normalization
  // so we need to renormalize it to vert fov first and then
  // find our hor/vert fov
  // note that fov is inverse (as it's tangent) and thus we
  // inverse division too
  float verFov = fovInTan * 16.f / 9.f;
  float horFov = fovInTan;
  if (camera_setup.fovMode == EFM_HOR_PLUS)
    horFov = verFov * h / w;
  else if (camera_setup.fovMode == EFM_HYBRID)
  {
    if (float(w) / float(h) < 16.f / 9.f) // if it's less wide than 16:9 then we do ver+
      verFov = fovInTan * w / h;
    horFov = verFov * h / w;
  }
  d3d::setpersp(Driver3dPerspective(horFov, verFov, camera_setup.znear, camera_setup.zfar, 0, 0));
  d3d::setview(0, 0, w, h, 0, 1); //?
}


class UiGameScene : public DagorGameScene,
#if _TARGET_PC || _TARGET_ANDROID || _TARGET_C3
                    public HumanInput::IGenKeyboardClient,
                    public HumanInput::IGenPointingClient,
#endif
                    public HumanInput::IGenJoystickClient
{
  IPoint2 lastCursorPos;
  UniqueTex panelCursor;

public:
  UiGameScene()
  {
    lastFtime = NULL;
    ftime = NULL;
    lastCursorPos = IPoint2(0, 0);


#if _TARGET_PC || _TARGET_ANDROID || _TARGET_C3
    if (global_cls_drv_pnt)
    {
      int scrW = StdGuiRender::real_screen_width();
      int scrH = StdGuiRender::real_screen_height();

      global_cls_drv_pnt->useDefClient(this);
      if (global_cls_drv_pnt->getDevice(0))
        global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, scrW, scrH);

      if (use_system_cursor)
        global_cls_drv_pnt->hideMouseCursor(false);

      if (global_cls_drv_kbd)
        global_cls_drv_kbd->useDefClient(this);
    }
#endif
    if (global_cls_drv_joy)
    {
      global_cls_drv_joy->useDefClient(this);
      global_cls_drv_joy->enableAutoDefaultJoystick(true);
    }
    HumanInput::stg_pnt.allowEmulatedLMB = false; // disable xbox gamepad click emulation
  }

  virtual ~UiGameScene()
  {
    closeDargScene();

#if _TARGET_PC
    if (global_cls_drv_pnt)
      global_cls_drv_pnt->useDefClient(NULL);
    if (global_cls_drv_kbd)
      global_cls_drv_kbd->useDefClient(NULL);
#endif
    if (global_cls_drv_joy)
      global_cls_drv_joy->useDefClient(NULL);

    if (lastFtime)
      delete (lastFtime);
    if (ftime)
      delete (ftime);

      // shutdownWebcache();
#if _TARGET_PC_WIN && !_TARGET_64BIT
    if (dgs_get_settings()->getBool("dumpMemLeaksAfterScene", false))
    {
      DEBUG_CTX("%s: dump leaks", __FUNCTION__);
      DagDbgMem::dump_leaks(true);
    }
#endif
  }


  void closeDargScene()
  {
    panelCursor.close();
    del_it(joystick_handler);
    del_it(darg_scene);
  }


  void loadScene()
  {
    closeDargScene();

    darg_scene = darg::create_gui_scene();
    darg_scene->setCallback(&gui_scene_cb);
    bindquirrel::apply_compiler_options_from_game_settings(darg_scene->getModuleMgr());

    joystick_handler = new darg::JoystickHandler();

    int w, h;
    d3d::get_render_target_size(w, h, nullptr);
    debugTexOverlay.setTargetSize(Point2(w, h));
    runScriptScene();

    int cursorSize = 64;
    eastl::unique_ptr<TexImage32> img(TexImage32::create(cursorSize, cursorSize));
    TexPixel32 *texels = img->getPixels();
    for (int y = 0; y < cursorSize; ++y)
      for (int x = 0; x < cursorSize; ++x, ++texels)
      {
        float d = (Point2(x, y) - Point2(cursorSize / 2, cursorSize / 2)).length();
        if (d > cursorSize * 0.4f || d < cursorSize * 0.2f)
          texels->u = 0;
        else
          texels->u = 0xFFFFFFFFU;
      }
    panelCursor = dag::create_tex(img.get(), cursorSize, cursorSize, TEXFMT_DEFAULT, 1, "panel_cursor");
    panelCursor.getTex2D()->texaddr(TEXADDR_BORDER);
    panelCursor.getTex2D()->texbordercolor(E3DCOLOR(0, 0, 0, 0));

    vr::apply_all_user_preferences();
  }


  void runScriptScene()
  {
#if _TARGET_C3

#endif
    HSQUIRRELVM vm = darg_scene->getScriptVM();
    SqModules *moduleMgr = darg_scene->getModuleMgr();

    sq_enabledebuginfo(vm, true);
    sq_enablevartrace(vm, true);


#if DAGOR_DBGLEVEL > 0
    sq_enabledebuginfo(vm, true);
    sq_enablevartrace(vm, true);
    dag_sq_debuggers.initDebugger(0, moduleMgr, "daRg Scripts");
    scriptprofile::register_profiler_module(vm, moduleMgr);

    const bool varTraceOn = sq_isvartracesupported();
#else
    const bool varTraceOn = false;
#endif

    Sqrat::Table(Sqrat::ConstTable(vm)).SetValue("VAR_TRACE_ENABLED", varTraceOn);

    darg::bind_lottie_animation(moduleMgr);

    bind_dargbox_script_api(moduleMgr);
    create_script_console_processor(moduleMgr, "sq.exec");

    const char *scriptFn = dgs_get_settings()->getStr("script", "scripts/script.nut");
    darg_scene->runScriptScene(scriptFn);
  }


  void reloadScripts(bool full_reinit)
  {
    if (full_reinit)
    {
      // hard reload
      loadScene();
      gamelib::sound::on_script_reload(true);
    }
    else
    {
      HSQUIRRELVM vm = darg_scene->getScriptVM();
      gamelib::sound::on_script_reload(false);
      sqeventbus::clear_on_reload(vm);
      bindquirrel::cleanup_dagor_system_file_handlers();
      bindquirrel::clear_workcycle_actions(vm);
      // soft reload
      const char *scriptFn = dgs_get_settings()->getStr("script", "scripts/script.nut");
      darg_scene->reloadScript(scriptFn);
    }
  }


#if _TARGET_PC || _TARGET_ANDROID || _TARGET_C3
  virtual void attached(HumanInput::IGenPointing * /*pnt*/) {}
  virtual void detached(HumanInput::IGenPointing * /*pnt*/) {}

  void gmcMouseMove(HumanInput::IGenPointing *mouse, float /*dx*/, float /*dy*/) override
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    if (darg_scene)
    {
      bool processedByUi = darg_scene->onMouseEvent(darg::INP_EV_POINTER_MOVE, 0, rs.x, rs.y, rs.buttons);
      gamelib::input::on_mouse_move_event(darg_scene->getScriptVM(), mouse, Point2(rs.x, rs.y), processedByUi);
    }
  }

  void gmcMouseButtonDown(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    if (darg_scene)
    {
      bool processedByUi = darg_scene->onMouseEvent(darg::INP_EV_PRESS, btn_idx, rs.x, rs.y, rs.buttons);
      gamelib::input::on_mouse_button_event(darg_scene->getScriptVM(), mouse, btn_idx, true, Point2(rs.x, rs.y), processedByUi);
    }
  }

  void gmcMouseButtonUp(HumanInput::IGenPointing *mouse, int btn_idx) override
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    if (darg_scene)
    {
      bool processedByUi = darg_scene->onMouseEvent(darg::INP_EV_RELEASE, btn_idx, rs.x, rs.y, rs.buttons);
      gamelib::input::on_mouse_button_event(darg_scene->getScriptVM(), mouse, btn_idx, false, Point2(rs.x, rs.y), processedByUi);
    }
  }

  void gmcMouseWheel(HumanInput::IGenPointing *mouse, int scroll) override
  {
    const HumanInput::PointingRawState::Mouse &rs = mouse->getRawState().mouse;
    if (darg_scene)
    {
      bool processedByUi = darg_scene->onMouseEvent(darg::INP_EV_MOUSE_WHEEL, scroll, rs.x, rs.y, rs.buttons);
      gamelib::input::on_mouse_wheel_event(darg_scene->getScriptVM(), mouse, scroll, Point2(rs.x, rs.y), processedByUi);
    }
  }

  virtual void gmcTouchBegan(HumanInput::IGenPointing *pnt, int touch_idx, const HumanInput::PointingRawState::Touch &touch) override
  {
    if (darg_scene)
      darg_scene->onTouchEvent(darg::INP_EV_PRESS, pnt, touch_idx, touch);
  }

  virtual void gmcTouchMoved(HumanInput::IGenPointing *pnt, int touch_idx, const HumanInput::PointingRawState::Touch &touch) override
  {
    if (darg_scene)
      darg_scene->onTouchEvent(darg::INP_EV_POINTER_MOVE, pnt, touch_idx, touch);
  }

  virtual void gmcTouchEnded(HumanInput::IGenPointing *pnt, int touch_idx, const HumanInput::PointingRawState::Touch &touch) override
  {
    if (darg_scene)
      darg_scene->onTouchEvent(darg::INP_EV_RELEASE, pnt, touch_idx, touch);
  }

  virtual void attached(HumanInput::IGenKeyboard * /*kbd*/) {}
  virtual void detached(HumanInput::IGenKeyboard * /*kbd*/) {}

  virtual void gkcLayoutChanged(HumanInput::IGenKeyboard *, const char *layout) override
  {
    if (darg_scene)
      darg_scene->onKeyboardLayoutChanged(layout);
  }

  virtual void gkcLocksChanged(HumanInput::IGenKeyboard *, unsigned locks) override
  {
    if (darg_scene)
      darg_scene->onKeyboardLocksChanged(locks);
  }

  virtual void gkcButtonDown(HumanInput::IGenKeyboard *kbd, int btn_idx, bool repeat, wchar_t wc)
  {
    if (console::get_visual_driver())
    {
      console::get_visual_driver()->processKey(btn_idx, wc, true);
      if (console::get_visual_driver()->is_visible())
        return;
    }

    if (darg_scene)
    {
      using namespace HumanInput;
      bool processedByUi = false;
      const KeyboardRawState &rs = kbd->getRawState();
      if (btn_idx == DKEY_F7)
      {
        bool fullReinit = rs.shifts & rs.CTRL_BITS;
        reloadScripts(fullReinit);
        processedByUi = true;
      }
      else if (btn_idx == DKEY_P && (rs.shifts & rs.SHIFT_BITS) && (rs.shifts & rs.CTRL_BITS))
      {
        darg_scene->activateProfiler(!darg_scene->getProfiler());
        processedByUi = true;
      }
      else
      {
        if (darg_scene->onServiceKbdEvent(darg::INP_EV_PRESS, btn_idx, kbd->getRawState().shifts, repeat, wc) ||
            darg_scene->onKbdEvent(darg::INP_EV_PRESS, btn_idx, kbd->getRawState().shifts, repeat, wc))
          processedByUi = true;
      }

      gamelib::input::on_keyboard_event(darg_scene->getScriptVM(), kbd, btn_idx, true, repeat, wc, processedByUi);
    }
  }

  virtual void gkcButtonUp(HumanInput::IGenKeyboard *kbd, int btn_idx)
  {
    if (darg_scene)
    {
      bool processedByUi = darg_scene->onServiceKbdEvent(darg::INP_EV_RELEASE, btn_idx, kbd->getRawState().shifts, false) ||
                           darg_scene->onKbdEvent(darg::INP_EV_RELEASE, btn_idx, kbd->getRawState().shifts, false);

      gamelib::input::on_keyboard_event(darg_scene->getScriptVM(), kbd, btn_idx, false, false, 0, processedByUi);
    }
  }


  virtual void gkcMagicCtrlAltEnd() {}
  virtual void gkcMagicCtrlAltF() {}
#endif

  virtual void attached(HumanInput::IGenJoystick * /*joy*/) {}
  virtual void detached(HumanInput::IGenJoystick * /*joy*/) {}

  virtual void stateChanged(HumanInput::IGenJoystick *joy, int joy_ord_id)
  {
    if (global_cls_drv_joy->getDefaultJoystick() != joy)
      global_cls_drv_joy->setDefaultJoystick(joy);

    if (darg_scene && joystick_handler)
    {
      joystick_handler->onJoystickStateChanged(darg_scene, joy, joy_ord_id);
      gamelib::input::on_joystick_state_changed(darg_scene->getScriptVM(), joy, joy_ord_id);
    }
  }

  // DagorGameScene interface

  inline void reattachDevices(bool init_only)
  {
    // #if _TARGET_PC
    //     dagui::register_device_keyboard();
    //     dagui::register_device_mouse();
    // #endif
    //     dagui::register_device_joystick(::dgs_get_settings()->getBool("joystickRemap360", false));
    //
    //     if (!init_only)
    //       dagui::reset_hotkeys();
  }

  __forceinline void joystickAct()
  {
    if (!::global_cls_drv_joy->isDeviceConfigChanged())
      return;

    //    dagui::clear_devices();

    ::global_cls_drv_joy->refreshDeviceList();
    ::global_cls_drv_joy->enable(::global_cls_drv_joy->getDeviceCount() > 0);

    reattachDevices(false);
  }

  virtual void actScene()
  {
    if (io_events_poll)
      io_events_poll->poll();
#if HAS_MATCHING_MODULE
    bindquirrel::matching_module::update();
#endif
    webui::update();
    visuallog::act(::dagor_game_act_time);

    cpujobs::release_done_jobs();
    PictureManager::discard_unused_picture();

    if (::global_cls_drv_joy)
      joystickAct();

    if (darg_scene)
    {
      gamelib::input::process_pending_joystick_btn_stack(darg_scene->getScriptVM());

      d3d::GpuAutoLock lock;

      auto resolveEntityTm = [](uint32_t, const char *) { return TMatrix::IDENT; };

      auto vr_surface_intersect = [](const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &intersection) {
        if (vrgui::is_inited())
          return vrgui::intersect(pos, dir, point_in_gui, intersection);
        return false;
      };

      mat44f globtm;
      d3d::getglobtm(globtm);

      darg::IGuiScene::VrSceneData vrScene;
      vrScene.vrSpaceOrigin = TMatrix::IDENT;
      vrScene.camera = ::grs_cur_view.itm;
      vrScene.cameraFrustum = Frustum(globtm);
      vrScene.hands[0] = vr::get_grip_pose(0);
      vrScene.hands[1] = vr::get_grip_pose(1);
      vrScene.aims[0] = vr::get_aim_pose(0);
      vrScene.aims[1] = vr::get_aim_pose(1);
      vrScene.entityTmResolver = [](uint32_t, const char *) { return TMatrix::IDENT; };
      vrScene.vrSurfaceIntersector = vr_surface_intersect;
      darg_scene->updateSpatialElements(vrScene);
      darg_scene->refreshVrCursorProjections();

      joystick_handler->processPendingBtnStack(darg_scene);
      darg_scene->update(::dagor_game_act_time);
    }

#if _TARGET_PC
    // Exit
    if (HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_F10))
      requested_exit_code = 0;
#if _TARGET_PC_MACOSX
    if (HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_LWIN) && HumanInput::raw_state_kbd.isKeyDown(HumanInput::DKEY_Q))
    {
      debug("request to close (Cmd+Q)");
      requested_exit_code = 0;
    }
#endif
#else
    // Exit
    if (HumanInput::raw_state_joy.buttons.get(5))
      requested_exit_code = 0;
#endif

    // updateWebcache();

    StdGuiRender::update_internals_per_act();

    // Quit Game
    if (requested_exit_code >= 0)
      quit_game(requested_exit_code);
  }

  virtual void drawScene()
  {
    cur_update++;
    if (benchmark && cur_update == limit_updates)
      quit_game(0);

    if (do_render)
    {
      ddsx::tex_pack2_perform_delayed_data_loading();
      windowClear();
      render();
      renderTrans();
    }

    debugTexOverlay.render();
  }

  virtual void beforeDrawScene(int /*realtime_elapsed_usec*/, float /*gametime_elapsed_sec*/)
  {
    PictureManager::per_frame_update();

    if (darg_scene)
      darg_scene->mainThreadBeforeRender();

    if (vr::is_configured())
    {
      CameraSetup cam;
      // if (!is_level_loading())
      //   cam = get_active_camera_setup();

      vr::acquire_frame_data(cam);
      vr::handle_controller_input();

      apply_camera_setup(cam);

      if (vr::is_enabled_for_this_frame())
        d3d::setpersp(vr::get_projection(StereoIndex::Bounding));
    }
  }

#define SUBST_TEX "onePixelTex"
  //  static void initWebcache()
  //  {
  //    cpujobs::init();
  //
  //    TexImage32 image[2];
  //    image[0].w = image[0].h = 1;
  //    *(E3DCOLOR*)(image+1) = E3DCOLOR(0,0,0,0); // transparent
  //    Texture *onePixelTex = d3d::create_tex(image, 1, 1, TEXCF_RGB|TEXCF_LOADONCE|TEXCF_SYSTEXCOPY, 1, SUBST_TEX);
  //    d3d_err(onePixelTex);
  //    register_managed_tex(SUBST_TEX, onePixelTex);
  //
  //    DataBlock cfg;
  //    cfg.setStr("mountPath", "webcache");
  //    cfg.setBool("noIndex", true);
  //    cfg.setStr("substTex", SUBST_TEX);
  //  }
  //  static void shutdownWebcache()
  //  {
  //    dagui::shutdown();
  //    cpujobs::term(false, 0);
  //
  //    release_managed_tex(tid);
  //  }
  //  static void updateWebcache() { cpujobs::release_done_jobs(); }

  virtual void sceneSelected(DagorGameScene * /*prev_scene*/)
  {
    reattachDevices(true);

    // initWebcache();

    DagDbgMem::next_generation();

    loadScene();
  }

  virtual void sceneDeselected(DagorGameScene * /*new_scene*/) {}

  // IRenderWorld interface
  virtual void render() {}

  virtual void renderTrans()
  {
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();

    if (vr::is_configured())
    {
      if (vr::is_enabled_for_this_frame())
      {
        vr::begin_frame();

        for (auto index : {StereoIndex::Left, StereoIndex::Right})
        {
          d3d::set_render_target(vr::get_frame_target(index), 0);
          d3d::clearview(CLEAR_TARGET, E3DCOLOR(30, 40, 50), 0, 0);
        }

        d3d::set_render_target(vr::get_gui_texture(), 0);
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);

        renderUi();

        vr::render_panels_and_gui_surface_if_needed(*darg_scene, globalFrameBlockId);

        vr::render_controller_poses();

        d3d::set_render_target();

        vr::finish_frame();
      }
    }
    else
    {
      TMatrix view;
      d3d::gettm(TM_VIEW, view);
      view = orthonormalized_inverse(view);

      ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
      darg_panel_renderer::render_panels_in_world(*darg_scene, view.getcol(3), darg_panel_renderer::RenderPass::Translucent);
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

      renderUi();
    }


    sleep_msec(dgs_get_settings()->getInt("perFrameSleep", 0));
  }

  virtual void renderIslDecals() {}
  virtual void renderToShadowMap() {}
  virtual void renderWaterReflection() {}
  virtual void renderWaterRefraction() {}

  DebugTexOverlay debugTexOverlay;

protected:
  __int64 *lastFtime;
  __int64 *ftime;

  inline void windowClear()
  {
    static E3DCOLOR fcolor(212, 208, 200, 0);
    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL | CLEAR_TARGET, fcolor, 0, 0);
  }

  void renderUi()
  {
    if (darg_scene)
    {
      darg_scene->renderThreadBeforeRender();

      // main scene
      darg_scene->buildRender();
      darg_scene->flushRender();

      // sub-scene panel
      darg_scene->buildPanelRender(0);
      darg_scene->flushPanelRender();
    }

    StdGuiRender::ScopeStarterOptional strt;
#if _TARGET_PC && 0
    {
      StdGuiRender::set_font(0);
      StdGuiRender::set_color(160, 160, 0, 255);
      StdGuiRender::set_ablend(true);

      const HumanInput::PointingRawState::Mouse &rs = global_cls_drv_pnt->getDevice(0)->getRawState().mouse;
      StdGuiRender::goto_xy(StdGuiRender::screen_width() - 200, 60);

      char str[255];
      sprintf(str, "mouse: %d, %d", (int)rs.x, (int)rs.y);
      StdGuiRender::draw_str(str);
    }
#endif

    if (console::get_visual_driver())
    {
      console::get_visual_driver()->update();
      console::get_visual_driver()->render();
    }

    visuallog::draw(0);

    StdGuiRender::flush_data();
  }
};

class DagorDummyAudio : public IGenAudioSystem
{
public:
  virtual IGenSound *createSound(const char *) { return (IGenSound *)0x0BAD0000; }
  virtual void playSound(IGenSound *, int, float) {}
  virtual void releaseSound(IGenSound *) {}

  virtual IGenMusic *createMusic(const char *, bool) { return (IGenMusic *)0x0BAD0000; }
  virtual void setMusicVolume(IGenMusic *, float) {}
  virtual void playMusic(IGenMusic *, int, float) {}
  virtual void stopMusic(IGenMusic *) {}
  virtual void releaseMusic(IGenMusic *) {}
};
static DagorDummyAudio dummy_audio_default;

class UiSoundPlayer : public darg::IUiSoundPlayer
{
  virtual void play(const char *name, const Sqrat::Object &params, float volume, const Point2 *pos)
  {
    String msg;
    msg.printf(32, "Sound: %s", name);
    visuallog::logmsg(msg);
  }
  virtual IGenAudioSystem *getAudioSystem() { return &dummy_audio_default; }
};

static UiSoundPlayer dummy_ui_sound_player;

static UiGameScene *game_scene = NULL;

static class ShowTexConsole : public console::ICommandProcessor
{
  virtual bool processCommand(const char *argv[], int argc) override
  {
    if (argc < 1)
      return false;
    int found = 0;
    CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
    {
      if (game_scene)
      {
        String str = game_scene->debugTexOverlay.processConsoleCmd(argv, argc);
        if (!str.empty())
          console::print(str);
      }
    }
    return found;
  }

  void destroy() {}
} show_tex_console;


static class ScriptConsole : public console::ICommandProcessor
{
  virtual bool processCommand(const char *argv[], int argc) override
  {
    if (argc < 1)
      return false;
    int found = 0;

    CONSOLE_CHECK_NAME("sq_memtrace", "reset_all", 1, 1) { sqmemtrace::reset_all(); }
    CONSOLE_CHECK_NAME("sq_memtrace", "dump_all", 1, 2)
    {
      sqmemtrace::dump_all(argc > 1 ? console::to_int(argv[1]) : -1);
      flush_debug_file();
    }

    return found;
  }

  void destroy() {}
} script_console;


static void init_sound()
{
  //   const DataBlock &blk = *::dgs_get_settings()->getBlockByNameEx("sound");
  //   bool haveSound = blk.getBool("haveSound", true);

  // #if DAGOR_DBGLEVEL > 0
  //   if (dgs_get_argv("nosound"))
  //     haveSound = false;
  // #endif

  gamelib::sound::initialize();

  debug("Sound system inited");
}

static void shutdown_sound() { gamelib::sound::finalize(); }

static void das_init()
{
  das::daScriptEnvironment::ensure();
  das::daScriptEnvironment::bound->das_def_tab_size = 2; // our coding style requires indenting of 2

  NEED_MODULE(Module_BuiltIn);
  NEED_MODULE(Module_Math);
  NEED_MODULE(Module_Strings);
  NEED_MODULE(DagorMath);
  NEED_MODULE(DagorTexture3DModule);
  NEED_MODULE(DagorResPtr);
  NEED_MODULE(DagorShaders);
  NEED_MODULE(DagorDriver3DModule);
  NEED_MODULE(DagorStdGuiRender);
  NEED_MODULE(ModuleDarg);
  das::Module::Initialize();
}

static void das_shutdown() { das::Module::Shutdown(); }

void reload_scripts(bool full_reinit)
{
  if (game_scene)
    game_scene->reloadScripts(full_reinit);
}

static void sq_exit_func(int code) { requested_exit_code = code; }

void dargbox_app_init()
{
  init_webui(NULL);

  darg::register_std_rendobj_factories();
  darg::register_blur_stubs();
  darg::ui_sound_player = &dummy_ui_sound_player;
  force_feedback::rumble::init();

  bindquirrel::set_sq_exit_function_ptr(sq_exit_func);

  init_sound();
  gamelib::input::initialize();

  nestdb::init();

  das_init();

  //::dgs_limit_fps = true;
  blkFx = *dgs_get_settings()->getBlockByNameEx("effects");
  da_profiler::set_profiling_settings(*dgs_get_settings()->getBlockByNameEx("debug"));
  limit_updates = dgs_get_settings()->getBlockByNameEx("debug")->getInt("limit_updates", 0);
  benchmark = limit_updates > 0;
  do_render = dgs_get_settings()->getBlockByNameEx("debug")->getBool("do_render", true);
  use_system_cursor = dgs_get_settings()->getBool("use_system_cursor", false);

  visuallog::setMaxItems(dgs_get_settings()->getInt("visualLogItems", 32));

  folders::initialize("dargbox");

  game_scene = new UiGameScene();
  dagor_select_game_scene(game_scene);


  console::init();
  add_con_proc(&show_tex_console);
  add_con_proc(&script_console);

  visualConsoleDriver = new VisualConsoleDriver;
  console::set_visual_driver(visualConsoleDriver, NULL);

  int monoFontId = StdGuiRender::get_font_id("mono");
  if (monoFontId < 0)
  {
    if (::dgs_get_settings()->getStr("fonts", nullptr) == nullptr)
      logerr("font 'mono' not found");
    monoFontId = 0;
  }
  visualConsoleDriver->setFontId(monoFontId);

  globalFrameBlockId = ShaderGlobal::getBlockId("global_frame");

  run_autoexec("autoexec.txt");
}

void dargbox_app_shutdown()
{
  force_feedback::rumble::shutdown();
  dagor_select_game_scene(NULL);
  del_it(game_scene);
  io_events_poll.reset();

  nestdb::shutdown();

  webui::shutdown();

  console::set_visual_driver(NULL, NULL);
  del_it(visualConsoleDriver);

  PictureManager::release();
  ddsx::shutdown_tex_pack2_data();

  shutdown_sound();
  gamelib::input::finalize();
  vr::tear_down_gui_surface();

  das_shutdown();
}
