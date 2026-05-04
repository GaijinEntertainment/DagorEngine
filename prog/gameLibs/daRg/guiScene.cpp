// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "guiScene.h"

#include <quirrel/frp/dag_frp.h>
#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_panelRenderer.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_joystick.h>

#include <generic/dag_tabUtils.h>
#include <generic/dag_relocatableFixedVector.h>
#include <debug/dag_traceInpDev.h>
#include <shaders/dag_shaderBlock.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>

#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiMouseIds.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiPointingData.h>
#include <drv/hid/dag_hiGlobals.h>

#include <quirrel/bindQuirrelEx/sqModulesDagor.h>

#include <gui/dag_visualLog.h>
#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <drv/hid/dag_hiXInputMappings.h>

#include "joystickAxisObservable.h"
#include "cursor.h"
#include "scriptUtil.h"
#include "scriptBinding.h"
#include "inputStack.h"
#include "hotkeys.h"
#include "profiler.h"
#include "timer.h"
#include "picAsyncLoad.h"
#include "gamepadCursor.h"
#include "yuvRenderer.h"
#include "textUtil.h"
#include "eventData.h"
#include "behaviorHelpers.h"
#include "panelRenderer.h"
#include "panelMath3d.h"
#include "elementRef.h"
#include "canvasDraw.h"
#include "dirPadNav.h"
#include "xmb.h"

#include "textLayout.h"
#include "dargDebugUtils.h"
#include "guiGlobals.h"
#include <daRg/robjWorldBlur.h>

#include <sqstdaux.h>

#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_plane3.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/optional.h>

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <ioSys/dag_dataBlock.h>

#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>
#include <quirrel/quirrel_json/jsoncpp.h>

#include "dasScripts.h"

#include <util/dag_convar.h>

#if _TARGET_C1 | _TARGET_C2

#endif

#if _TARGET_PC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetObject
#endif


#define ENABLE_SQ_MEM_STAT DAGOR_DBGLEVEL > 0

#if ENABLE_SQ_MEM_STAT
namespace sqmemtrace
{
extern unsigned mem_used, mem_used_max, mem_cur_ptrs, mem_max_ptrs;
}
#endif

static void enable_kb_layout_and_locks_tracking()
{
  // assuming that driver will forcefully notify client on locks/layout
  // state on enableTracking() call
  HumanInput::IGenKeyboard *kbd = ::global_cls_drv_kbd ? ::global_cls_drv_kbd->getDevice(0) : nullptr;
  if (kbd)
  {
    kbd->enableLayoutChangeTracking(true);
    kbd->enableLocksChangeTracking(true);
  }
}


namespace darg
{

CONSOLE_BOOL_VAL("darg", debug_cursor, false);

static constexpr int MAIN_SCREEN_ID = 999;

static constexpr unsigned MAX_JOY_AXIS_OBSERVABLES = 15;

using namespace sqfrp;


static const int rebuild_stacks_flags =
  ElementTree::RESULT_ELEMS_ADDED_OR_REMOVED | ElementTree::RESULT_INVALIDATE_RENDER_LIST | ElementTree::RESULT_INVALIDATE_INPUT_STACK;


static const Point2 out_of_screen_pos(-99, -99); // to be removed or reworked

#if DAGOR_DBGLEVEL > 0

#define THREAD_CHECK_COLLECT_CALL_STACK 0

class ApiThreadCheck
{
public:
  ApiThreadCheck(const GuiScene *scn) : scene(scn)
  {
    int curTid = ::get_current_thread_id();
    prevThread = scene->apiThreadId;
    if (prevThread != 0 && prevThread != curTid)
    {
#if THREAD_CHECK_COLLECT_CALL_STACK
      char stkText[4096];
      ::stackhlp_get_call_stack(stkText, sizeof(stkText), scene->threadCheckCallStack, 32);
      DAG_FATAL("daRg: Non-safe access from threads %d and %d (main thread = %d)\nFirst access from:\n%s", prevThread, curTid,
        int(::get_main_thread_id()), stkText);
#else
      G_ASSERTF(0, "daRg: Non-safe access from threads %d and %d (main thread = %d)", prevThread, curTid, int(::get_main_thread_id()));
#endif
    }
    scene->apiThreadId = curTid;

#if THREAD_CHECK_COLLECT_CALL_STACK
    ::stackhlp_fill_stack(scene->threadCheckCallStack, 32, 1);
#endif
  }

  ~ApiThreadCheck() { scene->apiThreadId = prevThread; }

  const GuiScene *scene = nullptr;
  int prevThread = 0;
};

#else

class ApiThreadCheck
{
public:
  ApiThreadCheck(const GuiScene *) {}
};

#endif


IGuiScene *create_gui_scene()
{
  {
    using namespace HumanInput;
    G_STATIC_ASSERT(int(DIR_UP) == int(JOY_XINPUT_REAL_BTN_D_UP));
    G_STATIC_ASSERT(int(DIR_DOWN) == int(JOY_XINPUT_REAL_BTN_D_DOWN));
    G_STATIC_ASSERT(int(DIR_LEFT) == int(JOY_XINPUT_REAL_BTN_D_LEFT));
    G_STATIC_ASSERT(int(DIR_RIGHT) == int(JOY_XINPUT_REAL_BTN_D_RIGHT));
  }

  return new GuiScene();
}


GuiScene::GuiScene() : config(this), gamepadCursor(config), kbFocus(this)
{
  yuvRenderer.init();

  const int numQuads = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_gui_quads", 64 << 10);
  const int numExtraIndices = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_gui_extra", 1024);
  guiContext.reset(new StdGuiRender::GuiContext());
  G_VERIFY(guiContext->createBuffer(0, NULL, numQuads, numExtraIndices, "daRg.guibuf"));

  const char *dargProject = ::dgs_get_settings()->getStr("game_darg_project_script", nullptr);
  dasScriptsData.reset(new DasScriptsData(dargProject));

  add_wnd_proc_component(this);

  queryCurrentScreenResolution();

  config.buttonTouchMargin = min(deviceScreenSize.x, deviceScreenSize.y) * 0.025f;

  initScript();
  createNativeWatches();
}


GuiScene::~GuiScene()
{
  del_wnd_proc_component(this);

  clear(true);

  sceneScriptHandlers.releaseFunctions();
  // Native nodes must be destroyed before sq_close() in shutdownScript().
  // Order: clear() already called frpGraph->shutdown() which released script refs;
  // destroyNativeWatches() frees native-owned slots; then sq_close() destroys
  // remaining script handles (resolve -> null -> no-op) and asserts nodeCount()==0.
  destroyNativeWatches();
  shutdownScript();
}


void GuiScene::initDasEnvironment(TGuiInitDas init_callback) { dasScriptsData->initDasEnvironment(init_callback); }


void GuiScene::shutdownDasEnvironment() { dasScriptsData->shutdownDasEnvironment(); }


void GuiScene::createNativeWatches()
{
  sqfrp::ObservablesGraph *g = frpGraph.get();
  cursorPresent.create(g, Sqrat::Object(false, sqvm).GetObject());
  cursorOverStickScroll.create(g, Sqrat::Object(SQInteger(0), sqvm).GetObject());
  cursorOverClickable.create(g, Sqrat::Object(false, sqvm).GetObject());
  hoveredClickableInfo.create(g, Sqrat::Object(sqvm).GetObject());
  keyboardLayout.create(g, Sqrat::Object("", sqvm).GetObject());
  keyboardLocks.create(g, Sqrat::Object(SQInteger(0), sqvm).GetObject());
  isXmbModeOn.create(g, Sqrat::Object(false, sqvm).GetObject());
  updateCounter.create(g, Sqrat::Object(actsCount, sqvm).GetObject());

  for (auto &obs : joyAxisObservables)
  {
    HSQOBJECT initial;
    sq_resetobject(&initial);
    obs.id = frpGraph->createWatched(initial);
  }
}

void GuiScene::destroyNativeWatches()
{
  cursorPresent.destroy();
  cursorOverStickScroll.destroy();
  cursorOverClickable.destroy();
  hoveredClickableInfo.destroy();
  keyboardLayout.destroy();
  keyboardLocks.destroy();
  isXmbModeOn.destroy();
  updateCounter.destroy();

  for (auto &obs : joyAxisObservables)
    obs.graph = nullptr;
  joyAxisObservables.clear();
}

void GuiScene::initScript()
{
  G_ASSERT(!sqvm);
  sqvm = sq_open(1024);
  G_ASSERT(sqvm);

  moduleMgr.reset(new SqModules(sqvm, &sq_modules_dagor_file_access));
  frpGraph.reset(new ObservablesGraph(sqvm, "darg"));

  SqStackChecker stackCheck(sqvm);

  saveToSqVm();

  sq_setprintfunc(sqvm, script_print_func, script_err_print_func);
  sq_setcompilererrorhandler(sqvm, compile_error_handler);

  sq_newclosure(sqvm, runtime_error_handler, 0);
  sq_seterrorhandler(sqvm);
  stackCheck.check();

  G_ASSERT(!stringKeys);
  stringKeys = new StringKeys();
  stringKeys->init(sqvm);

  bindScriptClasses();

  canvasApi = Sqrat::Table(sqvm);
  RenderCanvasContext::bind_script_api(canvasApi);
}


void GuiScene::shutdownScript()
{
  if (guiSceneCb)
    guiSceneCb->onShutdownScript(sqvm);

  del_it(stringKeys);

  moduleMgr.reset();
  canvasApi.Release();

  if (sqvm)
  {
    sq_collectgarbage(sqvm);
    sq_close(sqvm);
    sqvm = nullptr;
  }

  G_ASSERT(frpGraph->nodeCount() == 0);
  frpGraph.reset();

  G_ASSERT(allCursors.empty());
}


void GuiScene::clearCursors(bool exiting)
{
  G_UNUSED(exiting);

  activeCursor = nullptr;

  config.defaultCursor.Release();
  forcedCursor.Release();

  for (;;)
  {
    size_t prevCount = allCursors.size();
    bool finished = true;
    for (Cursor *c : allCursors)
    {
      c->clear();
      if (allCursors.size() != prevCount)
      {
        finished = false;
        break;
      }
    }
    if (finished)
      break;
  }

  for (auto &itPanel : panels)
    for (PanelPointer &ptr : itPanel.second->pointers)
      ptr.cursor = nullptr;
}


void GuiScene::clear(bool exiting)
{
  G_ASSERT(!isClearing);

  TIME_PROFILE(GuiScene_clear);

  isClearing = true;

  if (guiSceneCb)
  {
    if (eastl::any_of(screens.begin(), screens.end(), [](const auto &p) { return !p.second->inputStack.stack.empty(); }))
      guiSceneCb->onToggleInteractive(0);

    if (guiSceneCb->useInputConsumersCallback())
      guiSceneCb->onInputConsumersChange(dag::ConstSpan<HotkeyButton>(), dag::ConstSpan<String>(), false);
  }

  // Need to call such script handlers as onDetach()
  callScriptHandlers(true);

  G_ASSERT(!lockTimersList);
  invalidatedElements.clear();
  pressedClickButtons.clear();

  kbFocus.reset();

  for (VrPointer &vrPtr : vrPointers)
    vrPtr = {};
  spatialInteractionState = {};
  panels.clear();
  screens.clear();
  focusedScreen = nullptr;
  G_ASSERT(!xmbFocus);
  xmbFocus = nullptr; // just in case
  G_ASSERT(keptXmbFocus.empty());
  keptXmbFocus.clear();

  clear_all_ptr_items_and_shrink(timers);
  clear_all_ptr_items_and_shrink(scriptHandlersQueue);

  if (!sceneScriptHandlers.onShutdown.IsNull())
    sceneScriptHandlers.onShutdown();

  clearCursors(exiting);
  mousePointer.onShutdown();

  touchPointers.reset();

  lastCameraTransform = TMatrix::IDENT;
  lastCameraFrustum = {};

  clear_all_ptr_items_and_shrink(timers);

  frpGraph->shutdown(exiting);

  status.reset();

  PicAsyncLoad::on_scene_shutdown(this);
  PictureManager::discard_unused_picture();
  needToDiscardPictures = false;

  sq_collectgarbage(sqvm);
  isClearing = false;

  darg_panel_renderer::clean_up();
}


Sqrat::Array GuiScene::getAllObservables() const
{
  Sqrat::Array res(sqvm, frpGraph->nodeCount());
  SQInteger idx = 0;
  frpGraph->forEachNode([&](sqfrp::NodeId id, const sqfrp::NodeSlot &) {
    Sqrat::Table item(sqvm);
    frpGraph->fillInfo(id, item);
    res.SetValue(idx, item);
    ++idx;
  });
  return res;
}


void GuiScene::setSceneErrorRenderMode(SceneErrorRenderMode mode)
{
  ApiThreadCheck atc(this);
  status.setSceneErrorRenderMode(mode);
}


bool GuiScene::getErrorText(eastl::string &text) const
{
  ApiThreadCheck atc(this);
  status.errorMsg.join(text);
  return !text.empty();
}


void GuiScene::forceFrpUpdateDeferred()
{
  ApiThreadCheck atc(this);

  if (status.isShowingFullErrorDetails)
    return;

  SqStackChecker stackCheck(sqvm);
  frpGraph->updateDeferred();
}


void GuiScene::repeatDirPadNav(float dt)
{
  if (lastDirPadNavBtn >= 0)
  {
    if (dirPadNavRepeatTimeout <= 0.0f)
    {
      dirPadNavRepeatTimeout = config.dirPadRepeatTime;
      dirPadNavigate(dirpadnav::gamepad_button_to_dir(lastDirPadNavBtn));
    }
    else
      dirPadNavRepeatTimeout -= dt;
  }
}


void GuiScene::updatePointers(float dt, bool &hover_dirty)
{
  // Apply deferred jumps (internal state only)
  if (isInputActive())
  {
    if (activePointingDevice == DEVID_MOUSE)
    {
      if (::dgs_app_active)
      {
        bool mouseJumped = mousePointer.hasNextFramePos();
        mousePointer.syncMouseCursorLocation();
        mousePointer.applyNextFramePos();
        if (mouseJumped)
        {
          mousePointer.markNeedsDriverSync();
          hover_dirty = true;
        }
      }
    }

    if (activePointingDevice == DEVID_JOYSTICK)
    {
      bool gpJumped = gamepadCursor.hasNextFramePos();
      gamepadCursor.applyNextFramePos();
      if (gpJumped)
        hover_dirty = true;
    }
  }

  // Device-specific movement
  updateGamepadCursor(dt, hover_dirty);
  updateVrStickScroll(dt);

  updateJoystickAxesObservables();
  if (activePointingDevice == DEVID_MOUSE && mousePointer.moveToTarget(dt))
    hover_dirty = true;
  if (activePointingDevice == DEVID_JOYSTICK && gamepadCursor.moveToTarget(dt))
    hover_dirty = true;

  // Sync mouse to OS driver (one-way out)
  if (activePointingDevice == DEVID_MOUSE)
    mousePointer.commitToDriver();
}


void GuiScene::updateGamepadCursor(float dt, bool &hover_dirty)
{
  if (!isInputActive())
    return;

  HumanInput::IGenJoystick *joy = getJoystick();
  bool haveCursor = activeCursor || (focusedScreen && focusedScreen->panel && focusedScreen->panel->hasActiveCursors());
  if (!haveCursor)
    return;

  if (config.gamepadCursorControl && ::dgs_app_active)
  {
    if (gamepadCursor.isStickActive(joy, focusedScreen))
    {
      // Switch device if needed - before computing movement
      if (activePointingDevice != DEVID_JOYSTICK)
      {
        gamepadCursor.syncPosFrom(mousePointer);
        activePointingDevice = DEVID_JOYSTICK;
      }

      if (gamepadCursor.updateCursorPos(joy, focusedScreen, lastCameraTransform, lastCameraFrustum, dt))
      {
        doSetXmbFocus(nullptr);
        hover_dirty = true;

        // Dispatch POINTER_MOVE directly with DEVID_JOYSTICK
        for (auto &[id, screen] : screens)
        {
          Point2 local;
          if (!screen->displayToLocal(gamepadCursor.pos, lastCameraTransform, lastCameraFrustum, local))
            local = out_of_screen_pos;
          onMouseEventInternal(screen.get(), INP_EV_POINTER_MOVE, DEVID_JOYSTICK, 0, local, 0, 0);
        }
      }
    }
  }

  auto scrollDelta = gamepadCursor.scroll(joy, dt);
  if (scrollDelta)
    doJoystickScroll(scrollDelta.value());
}


void GuiScene::updateVrStickScroll(float dt)
{
  if (isInputActive())
  {
    for (int hand = 0; hand < NUM_VR_POINTERS; ++hand)
    {
      const VrPointer &vrPtr = vrPointers[hand];
      if (vrPtr.stickScroll.lengthSq() > 1e-6f)
      {
        float scrollStep = 1000.0f * dt * StdGuiRender::screen_height() / 1080.0f;
        Point2 scrollDelta = vrPtr.stickScroll * scrollStep;
        if (vrPtr.activePanel)
        {
          Panel *panel = vrPtr.activePanel;
          if (panel->pointers[hand].isPresent)
          {
            if (panel->screen->etree.doJoystickScroll(panel->screen->inputStack, panel->pointers[hand].pos, scrollDelta))
            {
              panel->updateHover();
            }
          }
        }
        else
          doJoystickScroll(scrollDelta);
      }
    }
  }
}


void GuiScene::update(float dt)
{
  ApiThreadCheck atc(this);

  if (status.isShowingFullErrorDetails)
    return;

  AutoProfileScope profile(profiler, M_UPDATE_TOTAL);
  SqStackChecker stackCheck(sqvm);

  // need to do this before any element may change
  applyPendingHoverUpdate();

  applyDeferredRecalLayout();

  frpGraph->updateDeferred();

  actsCount++;
  updateCounter.setValue(Sqrat::Object(actsCount, sqvm));

  processPostedEvents();

  joystickHandler.processPendingBtnStack(this);

  repeatDirPadNav(dt);

  bool hoverDirty = false;
  updatePointers(dt, hoverDirty);

  bool cursorIsPresent;
  if (activePointingDevice == DEVID_JOYSTICK)
    cursorIsPresent = activeCursor != nullptr;
  else
    cursorIsPresent = !mousePointer.mouseWasRelativeOnLastUpdate && activeCursor != nullptr;
  cursorPresent.setValue(Sqrat::Object(cursorIsPresent, sqvm));

  if (hoverDirty)
    updateGlobalHover();
  updateTimers(dt);

  stackCheck.check();

  callScriptHandlers();

  rebuildInvalidatedParts();

  stackCheck.check();

  if (!status.isShowingFullErrorDetails && !sceneScriptHandlers.onUpdate.IsNull())
    sceneScriptHandlers.onUpdate.Execute(dt);

  stackCheck.check();

  // The same logic as in rebuildInvalidatedParts()

  int prevInteractiveFlags = calcInteractiveFlags();
  bool stacksRebuilt = false, hotkeysModified = false;

  for (auto &[id, screen] : screens)
  {
    int updRes = screen->etree.update(dt);
    if (updRes & rebuild_stacks_flags)
    {
      screen->rebuildStacks();
      stacksRebuilt = true;
    }
    if (updRes & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED)
      hotkeysModified = true;
    if (updRes & ElementTree::RESULT_NEED_XMB_REBUILD)
      screen->rebuildXmb();
  }

  if (stacksRebuilt || hotkeysModified)
    applyInteractivityChange(prevInteractiveFlags, hotkeysModified, /*update_global_hover*/ true);

  if (activeCursor)
    activeCursor->update(dt);

  for (auto &itPanel : panels)
    itPanel.second->update(dt);

  if (profiler.get())
    profiler->afterUpdate();

  for (auto &[id, sc] : screens)
    sc->updateDebugRenderBoxes(dt);
}


void GuiScene::refreshGuiContextState()
{
  ApiThreadCheck atc(this);

  guiContext->setTarget();
}


void GuiScene::mainThreadBeforeRender()
{
  ApiThreadCheck atc(this);

  framesCount++;

  if (HumanInput::IGenPointing *mouse = MousePointer::get_mouse())
    mouse->fetchLastState();
}


void GuiScene::renderProfileStats()
{
  const int fontId = 0;
  guiContext->set_font(fontId);
  float lineH = StdGuiRender::get_font_line_spacing(fontId);
  Point2 charCellSize = StdGuiRender::get_font_cell_size(fontId);

  const int numMetrics = NUM_PROFILER_METRICS;

#if ENABLE_SQ_MEM_STAT
#define N_MEM_METRICS 2
#else
#define N_MEM_METRICS 0
#endif


  const Point2 leftTop(20, 20);

  guiContext->reset_textures();
  guiContext->set_color(0, 0, 50, 100);
  guiContext->render_rect(leftTop.x, leftTop.y, leftTop.x + charCellSize.x * 80, leftTop.y + lineH * (numMetrics + 3 + N_MEM_METRICS));

  guiContext->set_color(255, 255, 255);
  String s;
  for (int i = 0; i < numMetrics; ++i)
  {
    ProfilerMetricId id = (ProfilerMetricId)i;
    guiContext->goto_xy(leftTop.x, leftTop.y + (i + 1) * lineH);

    float avg = 0, stdDev = 0, minVal = 0, maxVal = 0;
    if (profiler->getStats(id, avg, stdDev, minVal, maxVal))
      s.printf(64, "%s: %.02f ms [%.02f | %.02f] (std %.02f ms)", profiler_metric_names[i], avg, minVal, maxVal, stdDev);
    else
      s.printf(32, "%s: <no data>", profiler_metric_names[i]);
    guiContext->draw_str(s, s.length());
  }


  guiContext->goto_xy(leftTop.x, leftTop.y + (numMetrics + 2) * lineH);
  unsigned rlistSize = 0;
  for (auto &[id, sc] : screens)
    rlistSize += sc->renderList.list.size();
  s.printf(64, "Render list size = %u", rlistSize);
  guiContext->draw_str(s, s.length());

#if ENABLE_SQ_MEM_STAT
#define LINE_OFFS (numMetrics + 3)

  s.printf(64, "SQ mem use = %uK / max %uK", sqmemtrace::mem_used >> 10, sqmemtrace::mem_used_max >> 10);
  guiContext->goto_xy(leftTop.x, leftTop.y + (LINE_OFFS + 1) * lineH);
  guiContext->draw_str(s, s.length());

  s.printf(64, "SQ ptrs = %u / max %u", sqmemtrace::mem_cur_ptrs, sqmemtrace::mem_max_ptrs);
  guiContext->goto_xy(leftTop.x, leftTop.y + (LINE_OFFS + 2) * lineH);
  guiContext->draw_str(s, s.length());

#undef LINE_OFFS
#endif
}


void GuiScene::renderThreadBeforeRender()
{
  ApiThreadCheck atc(this);
  AutoProfileScope profileTotal(profiler, M_BEFORE_RENDER_TOTAL);

  for (auto &[id, sc] : screens)
  {
    G_UNUSED(id);
    G_ASSERT(sc->etree.rebuildFlagsAccum == 0);
  }

  yuvRenderer.resetOnFrameStart();

  int curTime = get_time_msec();
  float dt = (lastRenderTimestamp != 0) ? (curTime - lastRenderTimestamp) * 1e-3f : 0.0f;
  lastRenderTimestamp = curTime;

  if (status.isShowingFullErrorDetails)
    return;

  scrollToXmbFocus(dt);
  updateGlobalActiveCursor();

  int prevInteractiveFlags = calcInteractiveFlags();
  bool stacksRebuilt = false;

  {
    for (auto &[id, screen] : screens)
    {
      int brRes = 0;
      {
        AutoProfileScope profile(profiler, M_ETREE_BEFORE_RENDER);
        brRes = screen->etree.beforeRender(dt);
      }
      if (brRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
      {
        AutoProfileScope profile(profiler, M_RENDER_LIST_REBUILD);
        screen->rebuildStacks();
        stacksRebuilt = true;
      }
    }
    if (activeCursor)
    {
      int brResCursor = 0;
      {
        AutoProfileScope profile(profiler, M_ETREE_BEFORE_RENDER);
        brResCursor = activeCursor->etree.beforeRender(dt);
      }
      if (brResCursor & R_REBUILD_RENDER_AND_INPUT_LISTS)
      {
        AutoProfileScope profile(profiler, M_RENDER_LIST_REBUILD);
        activeCursor->rebuildStacks();
      }
    }
  }

  if (!panels.empty())
  {
    for (auto &itPanel : panels)
    {
      Panel *panel = itPanel.second.get();
      G_ASSERT(panel->screen->etree.rebuildFlagsAccum == 0);

      panel->updateActiveCursors();

      G_ASSERT(panel->screen->etree.rebuildFlagsAccum == 0);

      for (PanelPointer &ptr : panel->pointers)
      {
        if (ptr.cursor)
        {
          int pcbrRes = 0;
          {
            AutoProfileScope profile(profiler, M_ETREE_BEFORE_RENDER);
            pcbrRes = ptr.cursor->etree.beforeRender(dt);
          }
          if (pcbrRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
          {
            AutoProfileScope profile(profiler, M_RENDER_LIST_REBUILD);
            ptr.cursor->rebuildStacks();
          }
        }
      }
    }
  }

  bool hasFinishedAnims = false;
  if (dt > 0)
  {
    for (auto &[id, sc] : screens)
    {
      sc->etree.updateAnimations(dt, &hasFinishedAnims);
      sc->etree.updateTransitions(dt);
      sc->etree.updateKineticScroll(dt);
    }

    if (activeCursor)
    {
      activeCursor->etree.updateAnimations(dt, nullptr);
      activeCursor->etree.updateTransitions(dt);
    }

    for (auto &itPanel : panels)
    {
      Panel *panel = itPanel.second.get();
      for (PanelPointer &ptr : panel->pointers)
      {
        if (ptr.cursor)
        {
          ptr.cursor->etree.updateAnimations(dt, nullptr);
          ptr.cursor->etree.updateTransitions(dt);
        }
      }
    }
  }

  if (stacksRebuilt)
    applyInteractivityChange(prevInteractiveFlags, false, hasFinishedAnims);
  else if (hasFinishedAnims)
    updateGlobalHover();

  for (auto &[id, sc] : screens)
  {
    G_UNUSED(id);
    G_ASSERT(sc->etree.rebuildFlagsAccum == 0);
  }
}


void GuiScene::tryRestoreSavedXmbFocus()
{
  G_ASSERT(!dbgIsInTryRestoreSavedXmbFocus);
  dbgIsInTryRestoreSavedXmbFocus = true;

  for (int i = keptXmbFocus.size() - 1; i >= 0; --i)
  {
    Element *elem = keptXmbFocus[i];
    if (!elem->xmb) // just in case of some rebuild
      continue;

    if (xmb::is_xmb_node_input_covered(elem)) // nothing to do yet
      break;

    erase_items(keptXmbFocus, i, keptXmbFocus.size() - i);
    doSetXmbFocus(elem);
    break;
  }

  dbgIsInTryRestoreSavedXmbFocus = false;
}


void GuiScene::validateOverlaidXmbFocus()
{
  if (xmbFocus)
  {
    if (xmb::is_xmb_node_input_covered(xmbFocus))
    {
      G_ASSERT_RETURN(keptXmbFocus.size() < 100, );
      erase_item_by_value(keptXmbFocus, xmbFocus); // should not happen, but for safety add each element only once
      keptXmbFocus.push_back(xmbFocus);            // push to top
      doSetXmbFocus(nullptr);
    }
  }
  else
    tryRestoreSavedXmbFocus();
}


void GuiScene::buildRender()
{
  ApiThreadCheck atc(this);

  AutoProfileScope profileTotal(profiler, M_RENDER_TOTAL);

  guiContext->resetFrame();
  guiContext->start_render();

  if (status.isShowingFullErrorDetails)
    status.renderError(guiContext.get());
  else
  {
    {
      AutoProfileScope profileListRender(profiler, M_RENDER_LIST_RENDER);
      Screen *mainScreen = getGuiScreen(MAIN_SCREEN_ID);
      if (mainScreen)
      {
        G_ASSERT(mainScreen->isMainScreen);
        mainScreen->renderList.render(*guiContext);
        mainScreen->renderDebugInfo();
      }

      if (activeCursor)
        activeCursor->renderList.render(*guiContext);
      else if (isCursorForcefullyEnabled())
        renderInternalCursor();
    }

    if (status.isShowingError)
      status.renderError(guiContext.get());
  }

  G_ASSERT(!guiContext->restore_viewport());

  if (profiler)
  {
    profiler->afterRender();
    renderProfileStats();
  }


  if (debug_cursor)
  {
    String msg;
    guiContext->set_font(0);
    float yPos = 30;
    {
      Point2 rawPos(-999, -999);
      if (MousePointer::get_mouse())
      {
        const HumanInput::PointingRawState::Mouse &rs = MousePointer::get_mouse()->getRawState().mouse;
        rawPos = {rs.x, rs.y};
      }

      msg.printf(32, "mousePos = %.f, %.f, raw cursor = %.f, %.f", P2D(mousePointer.pos), rawPos.x, rawPos.y);
      auto bbox = StdGuiRender::get_str_bbox(msg, msg.length(), guiContext->curRenderFont);
      guiContext->goto_xy(guiContext->screen_width() - bbox.width().x - 10, yPos);
      guiContext->set_color(200, 200, 0);
      guiContext->draw_str(msg);
      yPos += ceilf(bbox.width().y * 1.25f);
    }
    {
      msg.printf(32, "gamepad cursor pos = %.f, %.f", P2D(gamepadCursor.pos));
      auto bbox = StdGuiRender::get_str_bbox(msg, msg.length(), guiContext->curRenderFont);
      guiContext->goto_xy(guiContext->screen_width() - bbox.width().x - 10, yPos);
      guiContext->set_color(200, 200, 0);
      guiContext->draw_str(msg);
      yPos += ceilf(bbox.width().y * 1.25f);
    }
    {
      msg.printf(32, "have activeCursor = %d, device = %d, translate = %.f, %.f", activeCursor != nullptr, activePointingDevice,
        activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform
          ? activeCursor->etree.root->transform->translate.x
          : -999.0f,
        activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform
          ? activeCursor->etree.root->transform->translate.y
          : -999.0f);
      auto bbox = StdGuiRender::get_str_bbox(msg, msg.length(), guiContext->curRenderFont);
      guiContext->goto_xy(guiContext->screen_width() - bbox.width().x - 10, yPos);
      guiContext->set_color(200, 200, 0);
      guiContext->draw_str(msg);
    }
  }

  if (debug_cursor)
  {
    mousePointer.debugRenderState(guiContext.get(), E3DCOLOR(255, 0, 200), E3DCOLOR(120, 120, 20, 40));
    gamepadCursor.debugRenderState(guiContext.get(), E3DCOLOR(0, 200, 255), E3DCOLOR(20, 120, 80, 40));
  }

  StdGuiRender::release();

  if (updateHoverRequestState == UpdateHoverRequestState::WaitingForRender)
    updateHoverRequestState = UpdateHoverRequestState::NeedToUpdate;
}


void GuiScene::renderInternalCursor()
{
  StdGuiRender::GuiContext &ctx = *guiContext;
  Point2 pos = activePointerPos();
  ctx.set_color(50, 100, 70, 100);
  float sz = ::min(deviceScreenSize.x, deviceScreenSize.y) * 0.01f;
  ctx.draw_line(pos.x - sz, pos.y, pos.x + sz, pos.y, 3);
  ctx.draw_line(pos.x, pos.y - sz, pos.x, pos.y + sz, 3);
}


void GuiScene::buildPanelRender(int panel_idx)
{
  ApiThreadCheck atc(this);

  // NOTE! This is not included in profiler metrics
  guiContext->resetFrame();
  guiContext->start_render();

  auto itPanel = panels.find(panel_idx);

  if (itPanel == panels.end())
  {
    // guiContext->set_color(20, 200, 50);
    // guiContext->render_box(0, 0, 2000, 2000);
  }
  else if (status.isShowingFullErrorDetails)
    status.renderError(guiContext.get());
  else
  {
    Panel *panel = itPanel->second.get();
    panel->screen->renderList.render(*guiContext);
    panel->screen->renderDebugInfo();

    for (size_t iPtr = 0; iPtr < Panel::MAX_POINTERS; ++iPtr)
    {
      const PanelPointer &ptr = panel->pointers[iPtr];
      if (ptr.cursor)
      {
        if (ptr.cursor->etree.root && ptr.cursor->etree.root->transform)
          ptr.cursor->etree.root->transform->translate = ptr.pos - ptr.cursor->hotspot;

        ptr.cursor->renderList.render(*guiContext);

        if (debug_cursor)
        {
          guiContext->set_font(0);
          guiContext->set_color(E3DCOLOR(100, 200, 100, 200));
          guiContext->goto_xy(ptr.pos + Point2(20, 20));
          String msg(0, "%d", iPtr);
          guiContext->draw_str_scaled(2, msg.c_str(), msg.length());
        }
      }
    }
  }

  G_ASSERT(!guiContext->restore_viewport());

  StdGuiRender::release();
}


void GuiScene::flushRenderImpl(FlushPart part)
{
  ApiThreadCheck atc(this);

  if (part == FlushPart::MainScene)
    blurWorld();

  guiContext->setTarget();
  StdGuiRender::acquire();
  guiContext->flushData();
  guiContext->end_render();

  if (needToDiscardPictures && pictureDiscardAllowed)
    needToDiscardPictures = !PictureManager::discard_unused_picture(/*force_lock*/ false);
}

void GuiScene::flushRender() { flushRenderImpl(FlushPart::MainScene); }

void GuiScene::flushPanelRender() { flushRenderImpl(FlushPart::Panel); }

void GuiScene::skipRender()
{
  ApiThreadCheck atc(this);

  lastRenderTimestamp = get_time_msec();

  if (needToDiscardPictures && pictureDiscardAllowed)
    needToDiscardPictures = !PictureManager::discard_unused_picture(/*force_lock*/ false);
}


void GuiScene::requestHoverUpdate()
{
  if (updateHoverRequestState == UpdateHoverRequestState::None)
    updateHoverRequestState = UpdateHoverRequestState::WaitingForRender;
}


void GuiScene::applyPendingHoverUpdate()
{
  if (updateHoverRequestState == UpdateHoverRequestState::NeedToUpdate)
  {
    updateGlobalHover();
    updateHoverRequestState = UpdateHoverRequestState::None;
  }
}

// removes roots from the array which have any parent with given flag
static void removeRootsWithParentFlag(dag::Vector<Element *> &roots, Element::Flags flag)
{
  roots.erase(eastl::remove_if(roots.begin(), roots.end(),
                [&](Element *e) {
                  for (Element *p = e->parent; p; p = p->parent)
                    if (p->hasFlags(flag))
                    {
                      e->updFlags(flag, false);
                      return true;
                    }
                  return false;
                }),
    roots.end());
}

void GuiScene::applyDeferredRecalLayout()
{
  TIME_PROFILE(applyDeferredRecalLayout);

  if (DAGOR_LIKELY(deferredRecalcLayout.empty()))
  {
    return;
  }

  // swap into temporary set just in case original set is changed somewhere down in the callstack
  eastl::vector_set<Element *> thisFrameUpdate;
  thisFrameUpdate.swap(deferredRecalcLayout);

  layoutRecalcFixedSizeRoots.reserve(thisFrameUpdate.size());
  layoutRecalcSizeRoots.reserve(thisFrameUpdate.size());
  layoutRecalcFlowRoots.reserve(thisFrameUpdate.size());

  for (Element *elem : thisFrameUpdate)
  {
    Element *fixedSizeRoot, *sizeRoot, *flowRoot;
    elem->getSizeRoots(fixedSizeRoot, sizeRoot, flowRoot);

    if (!fixedSizeRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE))
    {
      layoutRecalcFixedSizeRoots.push_back(fixedSizeRoot);
      fixedSizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE, true);
    }

    if (!sizeRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_SIZE))
    {
      layoutRecalcSizeRoots.push_back(sizeRoot);
      sizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_SIZE, true);
    }

    if (!flowRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_FLOW))
    {
      layoutRecalcFlowRoots.push_back(flowRoot);
      flowRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FLOW, true);
    }
  }

  removeRootsWithParentFlag(layoutRecalcFixedSizeRoots, Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE);
  removeRootsWithParentFlag(layoutRecalcSizeRoots, Element::F_LAYOUT_RECALC_PENDING_SIZE);
  removeRootsWithParentFlag(layoutRecalcFlowRoots, Element::F_LAYOUT_RECALC_PENDING_FLOW);

  recalcLayoutFromRoots(make_span(layoutRecalcFixedSizeRoots), make_span(layoutRecalcSizeRoots), make_span(layoutRecalcFlowRoots));

  layoutRecalcFixedSizeRoots.clear();
  layoutRecalcSizeRoots.clear();
  layoutRecalcFlowRoots.clear();
}

void GuiScene::updateGlobalHover()
{
  G_ASSERT(!dbgIsInUpdateHover);
  dbgIsInUpdateHover = true;

  dag::Vector<Point2, framemem_allocator> pointers;
  dag::Vector<SQInteger, framemem_allocator> stickScrollFlags;

  bool haveVisualPointer = false;

  // Panel hovers are updated independently via VR input events
  if (focusedScreen && focusedScreen->isMainScreen)
  {
    haveVisualPointer = activeCursor != nullptr || config.ignorePointerVisibilityForHovers;
    int numVisualPtrs = haveVisualPointer ? 1 : 0;

    const size_t nPtrs = numVisualPtrs + touchPointers.activePointers.size();
    pointers.resize(nPtrs);
    stickScrollFlags.resize(nPtrs);

    if (haveVisualPointer)
      pointers[0] = activePointerPos();
    for (size_t i = 0, n = touchPointers.activePointers.size(); i < n; ++i)
      pointers[i + numVisualPtrs] = touchPointers.activePointers[i].pos;

    focusedScreen->etree.updateHover(focusedScreen->inputStack, nPtrs, pointers.data(), stickScrollFlags.data());
  }
  else if (focusedScreen && focusedScreen->panel)
  {
    // Special processing for daRg screen space cursor interacting with panels
    // Allow it to work on panels, too
    Panel *panel = focusedScreen->panel;
    constexpr int ptrId = SpatialSceneData::AimOrigin::Internal;

    auto isPointInPanel = [](Panel *panel, Point2 pos) {
      IPoint2 canvasSize;
      G_ASSERT_RETURN(panel->getCanvasSize(canvasSize), false);
      return pos.x >= 0 && pos.y >= 0 && pos.x < canvasSize.x && pos.y < canvasSize.y;
    };

    Point2 localPos;
    if (focusedScreen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos) &&
        isPointInPanel(panel, localPos))
    {
      panel->setCursorState(ptrId, true, localPos);
      vrPointers[ptrId].activePanel = panel;
    }
    else
    {
      panel->setCursorState(ptrId, false);
      if (vrPointers[ptrId].activePanel == panel)
        vrPointers[ptrId].activePanel = nullptr;
    }
  }

  //
  // For now, support clickable info only for main sub-scene
  //

  Sqrat::Object clickableInfo(sqvm);

  bool willHandleClick = false;
  if (haveVisualPointer && focusedScreen && focusedScreen->isMainScreen)
  {
    for (Element *elem : focusedScreen->etree.topLevelHovers)
    {
      if (elem->hasBehaviors(Behavior::F_CAN_HANDLE_CLICKS))
      {
        for (Behavior *bhv : elem->behaviors)
        {
          if (bhv->willHandleClick(elem))
          {
            willHandleClick = true;
            clickableInfo = elem->props.scriptDesc.RawGetSlot(stringKeys->clickableInfo);
            break;
          }
        }
      }
    }
  }

  bool cursorWasOverStickScroll = cursorOverStickScroll.getValue().Cast<SQInteger>() != 0;
  bool cursorIsOverStickScroll = haveVisualPointer && stickScrollFlags[0] != 0;

  cursorOverStickScroll.setValue(Sqrat::Object(haveVisualPointer ? stickScrollFlags[0] : 0, sqvm));
  cursorOverClickable.setValue(Sqrat::Object(willHandleClick, sqvm));
  hoveredClickableInfo.setValue(clickableInfo);

  if (cursorWasOverStickScroll != cursorIsOverStickScroll)
    notifyInputConsumersCallback();

  validateOverlaidXmbFocus(); // TODO: Move it from here to after input stack rebuild

  dbgIsInUpdateHover = false;
}


void GuiScene::addCursor(Cursor *c) { allCursors.insert(c); }

void GuiScene::removeCursor(Cursor *c)
{
  allCursors.erase(c);
  if (c == activeCursor)
    activeCursor = nullptr;
  for (auto &itPanel : panels)
    itPanel.second->onRemoveCursor(c);
}


void GuiScene::updateGlobalActiveCursor()
{
  Cursor *prevCursor = activeCursor;
  activeCursor = nullptr;

  Point2 mousePos = activePointerPos();

  if (HumanInput::stg_pnt.emuTouchScreenWithMouse)
  {
    // Workaround for the cases when mouse is used to emulate touch.
    // In this situation no mouse event are sent, so pointerPosition is not very useful.
    // Use actual mouse position to make cursor the most sensible we can
    // However this is a hack and we better implement something explicit and straightforward
    // (some debug/develop cursors)
    // Also for daRg we ideally should not use mouse emulation at all - behavior
    // should process all the pointer devices in unified manner, so we may just switch to
    // mouse input for development on PC
    if (HumanInput::IGenPointing *mouse = MousePointer::get_mouse())
      mousePos.set(mouse->getRawState().mouse.x, mouse->getRawState().mouse.y);
  }

  bool haveFallbackCursor = config.useDefaultCursor || forcedCursor.GetType() == OT_INSTANCE;

  if (!isCursorForcefullyDisabled() && !haveActiveCursorOnPanels())
  {
    Screen *screen = getGuiScreen(MAIN_SCREEN_ID);
    bool matched = false;
    if (screen)
    {
      for (const InputEntry &ie : screen->cursorStack.stack)
      {
        if (ie.elem->hitTest(mousePos))
        {
          if (ie.elem->props.getCurrentCursor(stringKeys->cursor, &activeCursor))
          {
            matched = true;
            break;
          }

          if (haveFallbackCursor && ie.elem->hasFlags(Element::F_STOP_POINTING | Element::F_STOP_HOVER))
            break;
        }
      }
    }

    if (!matched)
    {
      if (forcedCursor.GetType() == OT_INSTANCE)
        activeCursor = forcedCursor.Cast<Cursor *>();
      else if (config.useDefaultCursor)
        activeCursor = config.defaultCursor.Cast<Cursor *>();
    }
  }

  if (activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform)
    activeCursor->etree.root->transform->translate = activePointerPos() - activeCursor->hotspot;

  if (prevCursor && prevCursor != activeCursor)
    prevCursor->etree.skipAllOneShotAnims();
}


CursorPosState &GuiScene::activePointerState()
{
  return activePointingDevice == DEVID_JOYSTICK ? static_cast<CursorPosState &>(gamepadCursor)
                                                : static_cast<CursorPosState &>(mousePointer);
}

const CursorPosState &GuiScene::activePointerState() const
{
  return activePointingDevice == DEVID_JOYSTICK ? static_cast<const CursorPosState &>(gamepadCursor)
                                                : static_cast<const CursorPosState &>(mousePointer);
}

Point2 GuiScene::activePointerPos() const { return activePointerState().pos; }


void GuiScene::setMousePos(const Point2 &p, bool reset_target) { mousePointer.setMousePos(p, reset_target); }


void GuiScene::requestActivePointerNextFramePos(const Point2 &p) { activePointerState().requestNextFramePos(p); }

void GuiScene::requestActivePointerTargetPos(const Point2 &p) { activePointerState().requestTargetPos(p); }


HumanInput::IGenJoystick *get_fallback_joystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


HumanInput::IGenJoystick *GuiScene::getJoystick(int16_t ord_id) const
{
  if (guiSceneCb && guiSceneCb->isJoystickManagedExternally())
    return guiSceneCb->getActiveJoystick(ord_id);
  return get_fallback_joystick();
}


void GuiScene::moveActiveCursorToElem(Element *elem, bool use_transform)
{
  BBox2 bbox;
  if (use_transform)
    bbox = elem->calcTransformedBbox();
  else
    bbox = BBox2(elem->screenCoord.screenPos, elem->screenCoord.screenPos + elem->screenCoord.size);
  moveActiveCursorToElemBox(elem, bbox, true);
}


void GuiScene::moveActiveCursorToElemBox(Element *elem, const BBox2 &bbox, bool jump)
{
  Point2 anchor;
  Point2 nextPos;
  if (elem->getNavAnchor(anchor))
    nextPos = min(max(floor(bbox.leftTop() + anchor), bbox.leftTop()), bbox.rightBottom());
  else
    nextPos = floor(bbox.center());

  Point2 nextPosGlobal = nextPos;

  Screen *screen = elem->etree->screen;
  bool convOk = screen->localToDisplay(nextPos, lastCameraTransform, lastCameraFrustum, nextPosGlobal);

  if (convOk)
  {
    if (jump)
      requestActivePointerNextFramePos(nextPosGlobal);
    else
      requestActivePointerTargetPos(nextPosGlobal);
  }
}


int GuiScene::handleMouseClick(Screen *screen, InputEvent event, InputDevice device, int btn_idx, Point2 pos, int accum_res)
{
  G_ASSERT_RETURN(screen, 0);

  const int pointerId = 0;

  bool buttonDown = (event == INP_EV_PRESS);

  int summaryRes = accum_res;

  for (const InputEntry &ie : screen->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_POINTING))
    {
      bool isHit = ie.elem->hitTest(pos);

      // set keyboard focus on click release
      if (isHit && !buttonDown && (btn_idx == 0 || btn_idx == 1) && ie.elem->hasBehaviors(Behavior::F_FOCUS_ON_CLICK) &&
          !kbFocus.hasCapturedFocus() && !(summaryRes & R_PROCESSED))
        kbFocus.setFocus(ie.elem);

      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_MOUSE)
          summaryRes |= bhv->pointingEvent(&screen->etree, ie.elem, device, event, pointerId, btn_idx, pos, summaryRes);
      }

      if (isHit && ie.elem->hasFlags(Element::F_STOP_POINTING))
        summaryRes |= R_PROCESSED | R_STOPPED;

      // release keyboard focus on click outside of element
      if (ie.elem == kbFocus.focus && !isHit && !kbFocus.hasCapturedFocus())
        kbFocus.setFocus(nullptr);
      // ^ if we allow clicking multiple screens, this logic to unfocus element should be revised
    }
  }

  return summaryRes;
}


int GuiScene::onMouseEvent(InputEvent event, int data, short mx, short my, int buttons, int prev_result)
{
  ApiThreadCheck atc(this);

  if (event == INP_EV_POINTER_MOVE)
  {
    auto mouse = MousePointer::get_mouse();
    if (mouse && mouse->getRelativeMovementMode())
      return prev_result;

    // isProgrammaticMove: setMousePos() on Windows triggers a synchronous
    // WM_MOUSEMOVE via mouse->setPosition().  If that reentrant event reached
    // this block it would call setMousePos() recursively.  Skip the switch
    // when the POINTER_MOVE originates from our own programmatic warp.
    if (activePointingDevice != DEVID_MOUSE && !mousePointer.isProgrammaticMove())
    {
      // Set device BEFORE setMousePos to prevent reentry from the same path.
      activePointingDevice = DEVID_MOUSE;
      // Warp OS cursor to where the gamepad cursor was - no visual jump
      mousePointer.setMousePos(gamepadCursor.pos, /*reset_target*/ true);
      // This event's (mx, my) is the stale pre-warp OS cursor position.
      // Discard it. The next real mouse move carries post-warp coords.
      updateGlobalHover();
      return prev_result;
    }

    if (!mousePointer.isProgrammaticMove())
      doSetXmbFocus(nullptr);

    mousePointer.onMouseMoveEvent(Point2(mx, my));

    updateGlobalHover();
  }

  int accumRes = prev_result;
  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()) && event != INP_EV_POINTER_MOVE)
      continue;

    Point2 local;
    if (!screen->displayToLocal(Point2(mx, my), lastCameraTransform, lastCameraFrustum, local))
      local = out_of_screen_pos;
    accumRes |= onMouseEventInternal(screen.get(), event, DEVID_MOUSE, data, local, buttons, accumRes);
  }

  callScriptHandlers(); // call handlers early for faster response

  if (accumRes & R_PROCESSED) // document could possibly be updated from handlers
    rebuildInvalidatedParts();

  return accumRes;
}


int GuiScene::onMouseEventInternal(Screen *screen, InputEvent event, InputDevice device, int data, Point2 pos, int buttons,
  int prev_result)
{
  G_UNUSED(buttons);
  DEBUG_TRACE_INPDEV("GuiScene::onMouseEvent(event=%d, device=%d, data=%d, pos=%.f, %.f, buttons=%d)", event, device, data, pos.x,
    pos.y, buttons);

  G_ASSERT_RETURN(screen, 0);

  const int pointerId = 0;

  if ((event == INP_EV_PRESS || event == INP_EV_RELEASE) &&
      (data == HumanInput::DBUTTON_SCROLLUP || data == HumanInput::DBUTTON_SCROLLDOWN))
  {
    // ignore 'click' events caused by wheel
    // instead process them as INP_EV_MOUSE_WHEEL messages
    return 0;
  }

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  // if (event == INP_EV_PRESS)
  //   visuallog::logmsg("---  INP_EV_PRESS  ---");
  // else if (event == INP_EV_RELEASE)
  //   visuallog::logmsg("---  INP_EV_RELEASE  ---");
  // else if (event == INP_EV_POINTER_MOVE)
  //   visuallog::logmsg("---  INP_EV_POINTER_MOVE  ---");

  int summaryRes = prev_result;

  if (event == INP_EV_PRESS || event == INP_EV_RELEASE)
  {
    if (updateHotkeys())
      summaryRes |= R_PROCESSED;

    summaryRes |= handleMouseClick(screen, event, device, data, pos, summaryRes);
  }
  else if (event == INP_EV_MOUSE_WHEEL)
  {
    Element *startHandler = nullptr;

    for (const InputEntry &ie : screen->inputStack.stack)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE) || ie.elem->hasFlags(Element::F_STOP_POINTING))
      {
        // If some element was initially hit, only its parents are allowed to process wheel events
        // We need to pass mouse wheel event through input stack to allow
        // scrolling containers that contain mouse-blocking children
        if (startHandler && !ie.elem->isAscendantOf(startHandler))
          continue;

        for (Behavior *bhv : ie.elem->behaviors)
        {
          if (bhv->flags & Behavior::F_HANDLE_MOUSE)
            summaryRes |= bhv->pointingEvent(&screen->etree, ie.elem, device, event, pointerId, data, pos, summaryRes);
        }

        if (startHandler == nullptr && ie.elem->hitTest(pos))
          startHandler = ie.elem;
      }
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    for (const InputEntry &ie : screen->inputStack.stack)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE))
      {
        for (Behavior *bhv : ie.elem->behaviors)
          summaryRes |= bhv->pointingEvent(&screen->etree, ie.elem, device, event, pointerId, data, pos, summaryRes);
      }
      if (ie.elem->hasFlags(Element::F_STOP_POINTING) && ie.elem->hitTest(pos))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    rebuildStacksAndNotify(screen, /*refresh_hotkeys_nav*/ false, /*update_global_hover*/ true);

  return summaryRes;
}


int GuiScene::onPanelMouseEvent(Panel *panel, int hand, InputEvent event, InputDevice device, int button_id, short mx, short my,
  int prev_result)
{
  // Mouse emulation for now
  DEBUG_TRACE_INPDEV("GuiScene::onPanelMouseEvent(hand=%d, event=%d, pos=%d, %d)", hand, event, mx, my);

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  if ((event == INP_EV_PRESS || event == INP_EV_RELEASE) &&
      (button_id == HumanInput::DBUTTON_SCROLLUP || button_id == HumanInput::DBUTTON_SCROLLDOWN))
  {
    // ignore 'click' events caused by wheel
    // instead process them as INP_EV_MOUSE_WHEEL messages
    return 0;
  }

  if (event == INP_EV_POINTER_MOVE && panel->pointers[hand].isPresent && (fabsf(panel->pointers[hand].pos.x - mx) < 1) &&
      (fabsf(panel->pointers[hand].pos.y - my) < 1))
  {
    return 0;
  }
  if (event == INP_EV_POINTER_MOVE)
    panel->pointers[hand].pos.set(mx, my);

  int summaryRes = prev_result;

  for (const InputEntry &ie : panel->screen->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE) || ie.elem->hasFlags(Element::F_STOP_POINTING))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_MOUSE)
          summaryRes |= bhv->pointingEvent(&panel->screen->etree, ie.elem, device, event, hand, button_id, Point2(mx, my), summaryRes);
      }
      if (ie.elem->hasFlags(Element::F_STOP_POINTING) && ie.elem->hitTest(mx, my))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (event == INP_EV_POINTER_MOVE)
    panel->updateHover();

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
  {
    // document could possibly be updated from handlers
    rebuildInvalidatedParts();
  }

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    rebuildStacksAndNotify(panel->screen, /*refresh_hotkeys_nav*/ false, /*update_global_hover*/ true);

  return summaryRes & R_PROCESSED;
}


int GuiScene::onPanelTouchEvent(Panel *panel, int hand, InputEvent event, short tx, short ty, int prev_result)
{
  ApiThreadCheck atc(this);

  DEBUG_TRACE_INPDEV("GuiScene::onPanelTouchEvent(event=%d, hand=%d, pos=%d, %d)", event, hand, tx, ty);

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  int summaryRes = prev_result;
  for (const InputEntry &ie : panel->screen->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_TOUCH) || ie.elem->hasFlags(Element::F_STOP_POINTING))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_TOUCH)
          summaryRes |= bhv->pointingEvent(&panel->screen->etree, ie.elem, DEVID_TOUCH, event, hand, 0, Point2(tx, ty), summaryRes);
      }

      if (ie.elem->hasFlags(Element::F_STOP_POINTING) && ie.elem->hitTest(Point2(tx, ty)))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (event == INP_EV_POINTER_MOVE)
  {
    doSetXmbFocus(nullptr);
    panel->updateHover();
  }

  panel->touchPointers.updateState(event, hand, Point2(tx, ty));

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
    rebuildInvalidatedParts();

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    rebuildStacksAndNotify(panel->screen, /*refresh_hotkeys_nav*/ false, /*update_global_hover*/ true);

  return summaryRes & R_PROCESSED;
}


int GuiScene::onTouchEvent(InputEvent event, HumanInput::IGenPointing *, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int prev_result)
{
  ApiThreadCheck atc(this);

  DEBUG_TRACE_INPDEV("GuiScene::onTouchEvent(event=%d, touch_idx=%d)", event, touch_idx);

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  // if (event == INP_EV_PRESS)
  //   visuallog::logmsg("---  INP_EV_PRESS  ---");
  // else if (event == INP_EV_RELEASE)
  //   visuallog::logmsg("---  INP_EV_RELEASE  ---");
  // else if (event == INP_EV_POINTER_MOVE)
  //   visuallog::logmsg("---  INP_EV_POINTER_MOVE  ---");

  auto screen = getFocusedScreen();
  if (!screen)
    return 0;

  Point2 pos(touch.x, touch.y);

  int summaryRes = prev_result;

  for (const InputEntry &ie : screen->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_TOUCH) || ie.elem->hasFlags(Element::F_STOP_POINTING))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_TOUCH)
          summaryRes |= bhv->pointingEvent(&screen->etree, ie.elem, DEVID_TOUCH, event, touch_idx, 0, pos, summaryRes);
      }

      if (ie.elem->hasFlags(Element::F_STOP_POINTING) && ie.elem->hitTest(pos))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (event == INP_EV_POINTER_MOVE)
    doSetXmbFocus(nullptr);

  touchPointers.updateState(event, touch_idx, pos);
  updateGlobalHover();

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
  {
    // document could possibly be updated from handlers
    rebuildInvalidatedParts();
  }

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    rebuildStacksAndNotify(screen, /*refresh_hotkeys_nav*/ false, /*update_global_hover*/ false);

  return summaryRes;
}


int GuiScene::onServiceKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc, int prev_result)
{
  G_UNUSED(repeat);
  G_UNUSED(wc);
  G_UNUSED(prev_result);

  DEBUG_TRACE_INPDEV("GuiScene::onServiceKbdEvent(event=%d, key_idx=%d, shifts=%d, wc=%d)", event, key_idx, shifts, wc);

  if (status.isShowingError && event == INP_EV_PRESS && key_idx == HumanInput::DKEY_BACK &&
      (shifts & HumanInput::KeyboardRawState::CTRL_BITS) != 0)
  {
    if ((shifts & HumanInput::KeyboardRawState::ALT_BITS) != 0)
    {
      status.dismissShownError();
      if (guiSceneCb)
        guiSceneCb->onDismissError();
      return R_PROCESSED;
    }
    else if ((shifts & HumanInput::KeyboardRawState::SHIFT_BITS) != 0)
    {
      status.toggleShowDetails();
      return R_PROCESSED;
    }
  }

  return 0;
}


int GuiScene::onKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc, int prev_result)
{
  ApiThreadCheck atc(this);

  G_UNUSED(shifts);
  DEBUG_TRACE_INPDEV("GuiScene::onKbdEvent(event=%d, key_idx=%d, shifts=%d, wc=%d)", event, key_idx, shifts, wc);

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  int resFlags = prev_result;

  eastl::vector_set<HotkeyButton>::iterator itClickBtn;

  if (config.kbCursorControl && event == INP_EV_RELEASE &&
      (itClickBtn = pressedClickButtons.find(HotkeyButton(DEVID_KEYBOARD, key_idx))) != pressedClickButtons.end())
  {
    for (auto &[id, screen] : screens)
    {
      if (!doesScreenAcceptInput(screen.get()))
        continue;

      Point2 localPos;
      if (!screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos))
        localPos = out_of_screen_pos;
      resFlags |= handleMouseClick(screen.get(), INP_EV_RELEASE, DEVID_KEYBOARD, 0, localPos, resFlags);
    }

    pressedClickButtons.erase(itClickBtn);
  }

  if (kbFocus.focus && kbFocus.focus->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
  {
    for (Behavior *bhv : kbFocus.focus->behaviors)
      if (bhv->flags & Behavior::F_HANDLE_KEYBOARD)
        resFlags |= bhv->kbdEvent(kbFocus.focus->etree, kbFocus.focus, event, key_idx, repeat, wc, resFlags);
  }

  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()))
      continue;

    if ((screen->inputStack.summaryBhvFlags & Behavior::F_HANDLE_KEYBOARD_GLOBAL) == Behavior::F_HANDLE_KEYBOARD_GLOBAL)
    {
      for (const InputEntry &ie : screen->inputStack.stack)
      {
        if (ie.elem != kbFocus.focus && ie.elem->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
        {
          for (Behavior *bhv : ie.elem->behaviors)
            if ((bhv->flags & Behavior::F_HANDLE_KEYBOARD_GLOBAL) == Behavior::F_HANDLE_KEYBOARD_GLOBAL)
              resFlags |= bhv->kbdEvent(&screen->etree, ie.elem, event, key_idx, repeat, wc, resFlags);
        }
      }
    }
  }

  if (!(resFlags & R_PROCESSED) && !repeat)
    if (updateHotkeys())
      resFlags |= R_PROCESSED;

  if (config.kbCursorControl && event == INP_EV_PRESS && !(resFlags & R_PROCESSED) && config.isClickButton(DEVID_KEYBOARD, key_idx))
  {
    for (auto &[id, screen] : screens)
    {
      if (!doesScreenAcceptInput(screen.get()))
        continue;

      Point2 localPos;
      if (!screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos))
        localPos = out_of_screen_pos;
      resFlags |= handleMouseClick(screen.get(), INP_EV_PRESS, DEVID_KEYBOARD, 0, localPos, resFlags);
    }

    pressedClickButtons.insert(HotkeyButton(DEVID_KEYBOARD, key_idx));
  }


  if (config.kbCursorControl && !(resFlags & R_PROCESSED) && event == INP_EV_PRESS)
  {
    using namespace HumanInput;
    if (key_idx == DKEY_LEFT)
      dirPadNavigate(DIR_LEFT);
    else if (key_idx == DKEY_RIGHT)
      dirPadNavigate(DIR_RIGHT);
    else if (key_idx == DKEY_UP)
      dirPadNavigate(DIR_UP);
    else if (key_idx == DKEY_DOWN)
      dirPadNavigate(DIR_DOWN);
  }

  callScriptHandlers();

  // if (resFlags & R_PROCESSED)
  //   rebuildInvalidatedParts();

  if (resFlags & R_REBUILD_RENDER_AND_INPUT_LISTS)
  {
    // TODO: not always need to update all screens
    rebuildAllScreensStacksAndNotify(/*refresh_hotkeys_nav*/ false, /*update_global_hover*/ true);
  }

  return resFlags;
}


int GuiScene::onJoystickBtnEvent(HumanInput::IGenJoystick *joy, InputEvent event, int btn_idx, int device_number,
  const HumanInput::ButtonBits &buttons, int prev_result)
{
  ApiThreadCheck atc(this);

  // DEBUG_TRACE_INPDEV("GuiScene::onJoystickBtnEvent(event=%d, btn_idx=%d, dev_no=%d)", event, btn_idx, device_number);

  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  if (event == INP_EV_PRESS && activePointingDevice != DEVID_JOYSTICK)
  {
    gamepadCursor.syncPosFrom(mousePointer);
    activePointingDevice = DEVID_JOYSTICK;
  }

  int resFlags = prev_result;

  if (event == INP_EV_RELEASE && btn_idx == lastDirPadNavBtn)
    lastDirPadNavBtn = -1;

  eastl::vector_set<HotkeyButton>::iterator itClickBtn;
  if (event == INP_EV_RELEASE &&
      (itClickBtn = pressedClickButtons.find(HotkeyButton(DEVID_JOYSTICK, btn_idx))) != pressedClickButtons.end())
  {
    for (auto &[id, screen] : screens)
    {
      if (!doesScreenAcceptInput(screen.get()))
        continue;

      Point2 localPos;
      if (!screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos))
        localPos = out_of_screen_pos;
      resFlags |= handleMouseClick(screen.get(), INP_EV_RELEASE, DEVID_JOYSTICK, 0, localPos, resFlags);
    }
    pressedClickButtons.erase(itClickBtn);
  }

  if (focusedScreen && (focusedScreen->inputStack.summaryBhvFlags & Behavior::F_HANDLE_JOYSTICK))
  {
    for (const InputEntry &ie : focusedScreen->inputStack.stack)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_JOYSTICK))
      {
        for (Behavior *bhv : ie.elem->behaviors)
          if (bhv->flags & Behavior::F_HANDLE_JOYSTICK)
            resFlags |= bhv->joystickBtnEvent(&focusedScreen->etree, ie.elem, joy, event, btn_idx, device_number, buttons, resFlags);
      }
    }
  }

  if (::dgs_app_active && !(resFlags & R_PROCESSED))
    if (updateHotkeys())
      resFlags |= R_PROCESSED;

  if (::dgs_app_active && event == INP_EV_PRESS && !(resFlags & R_PROCESSED) && config.isClickButton(DEVID_JOYSTICK, btn_idx))
  {
    for (auto &[id, screen] : screens)
    {
      if (!doesScreenAcceptInput(screen.get()))
        continue;

      Point2 localPos;
      if (!screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos))
        localPos = out_of_screen_pos;
      resFlags |= handleMouseClick(screen.get(), INP_EV_PRESS, DEVID_JOYSTICK, 0, localPos, resFlags);
    }
    pressedClickButtons.insert(HotkeyButton(DEVID_JOYSTICK, btn_idx));
  }

  if (::dgs_app_active && (event == INP_EV_PRESS) && !(resFlags & R_PROCESSED))
  {
    using namespace dirpadnav;
    bool isDirPad = is_dir_pad_button(btn_idx);
    bool isStickNav = is_stick_dir_button(btn_idx);
    if (isDirPad || (!config.gamepadCursorControl && config.gamepadStickAsDirpad && isStickNav))
    {
      dirPadNavigate(gamepad_button_to_dir(btn_idx));

      float delay = ::max(config.dirPadRepeatDelay, config.dirPadRepeatTime);
      if (delay > 0)
      {
        lastDirPadNavBtn = btn_idx;
        dirPadNavRepeatTimeout = delay;
      }
    }
  }

  callScriptHandlers();

  if (resFlags & R_PROCESSED)
    rebuildInvalidatedParts();

  if (resFlags & R_REBUILD_RENDER_AND_INPUT_LISTS)
  {
    // TODO: not always need to update all screens
    rebuildAllScreensStacksAndNotify(/*refresh_hotkeys_nav*/ false, /*update_global_hover*/ true);
  }

  return resFlags;
}


void GuiScene::dirPadNavigate(Direction dir)
{
  using namespace HumanInput;
  using namespace dirpadnav;

  if (dir != DIR_RIGHT && dir != DIR_LEFT && dir != DIR_DOWN && dir != DIR_UP) //-V560
  {
    G_ASSERTF(0, "dir = %d", dir);
    return;
  }

  if (isXmbModeOn.getValue().Cast<bool>())
  {
    if (xmbNavigate(dir) || xmbFocus)
      return;
  }

  auto screen = getFocusedScreen();
  if (!screen)
    return;

  Point2 pointerLocalPos;
  bool convOk = screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, pointerLocalPos);
  if (!convOk)
    return;

  float minDistance = 1e9f;
  Element *target = nullptr;

  for (const InputEntry &ie : screen->inputStack.stack)
  {
    Element *elem = ie.elem;

    if (!elem->isDirPadNavigable())
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
      continue;

    if (dir_pad_new_pos_trace(screen->inputStack, bbox.center()) != elem)
      continue;

    float distance = calc_dir_nav_distance(pointerLocalPos, bbox, dir, true);

    if (distance < minDistance)
    {
      target = elem;
      minDistance = distance;
    }
  }

  if (target && target->xmb && (target->xmb->xmbParent || !target->xmb->xmbChildren.empty()))
    trySetXmbFocus(target);
  else
    doSetXmbFocus(nullptr);

  if (target && (target != xmbFocus))
    moveActiveCursorToElemBox(target, target->clippedScreenRect, false);
}


void GuiScene::trySetXmbFocus(Element *target)
{
  G_ASSERT(!isClearing);
  G_ASSERT(!target || target->etree);

  Element *newFocus = nullptr;
  if (DAGOR_UNLIKELY(target && target->isDetached()))
  {
    logwarn("daRg: Setting XMB focus to detached element");
  }
  else if (target && target->xmb)
  {
    if (target->xmb->calcCanFocus() && (target->etree->screen == focusedScreen))
      newFocus = target;
  }
  doSetXmbFocus(newFocus);
}


void GuiScene::doSetXmbFocus(Element *elem)
{
  if (xmbFocus == elem)
    return;

  if (DAGOR_UNLIKELY(elem && elem->etree->screen != focusedScreen))
  {
    G_ASSERT(!"Setting XMB focus to non-focused screen");
    xmbFocus = nullptr;
  }
  else if (DAGOR_UNLIKELY(elem && elem->isDetached()))
  {
    G_ASSERT(!"Setting XMB focus to detached element");
    xmbFocus = nullptr;
  }
  else if (DAGOR_UNLIKELY(elem && !elem->xmb))
  {
    G_ASSERT(!"Setting XMB focus to non-XMB element");
    xmbFocus = nullptr;
  }
  else
  {
    xmbFocus = elem;
  }

  isXmbModeOn.setValue(Sqrat::Object(xmbFocus != nullptr, sqvm));
}


bool GuiScene::xmbNavigate(Direction dir)
{
  G_ASSERT_RETURN(xmbFocus, false);
  G_ASSERT_RETURN(xmbFocus->xmb, false);
  G_ASSERT_RETURN(xmbFocus->etree->screen == focusedScreen, false);

  Element *xmbParent = xmbFocus->xmb->xmbParent;

  bool isLeaving = false;

  if (xmbParent)
  {
    G_ASSERT_RETURN(xmbParent->xmb, false);
    if (xmbParent->layout.flowType == FLOW_PARENT_RELATIVE || xmbParent->xmb->nodeDesc.RawGetSlotValue("screenSpaceNav", false))
    {
      Element *target = xmb::xmb_screen_space_navigate(xmbFocus, dir);
      if (target)
        doSetXmbFocus(target);
      else
        isLeaving = true;
    }
    else
    {
      Element *newFocus = xmb::xmb_grid_navigate(xmbFocus, dir);
      if (!newFocus)
        newFocus = xmb::xmb_siblings_navigate(xmbFocus, dir);

      if (newFocus)
        doSetXmbFocus(newFocus);
      else
        isLeaving = true;
    }
  }

  bool stopProcessing = true;
  if (isLeaving)
  {
    XmbCode xmbRes = XMB_CONTINUE;
    Sqrat::Function onLeave(xmbFocus->xmb->nodeDesc, "onLeave");
    if (!onLeave.IsNull())
    {
      auto res = onLeave.Eval<Sqrat::Object>(dir);
      if (res && res.value().GetType() == OT_INTEGER)
        xmbRes = res.value().Cast<XmbCode>();
    }
    stopProcessing = xmbRes == XMB_STOP;
    if (xmbRes == XMB_CONTINUE)
      doSetXmbFocus(nullptr);
  }

  return stopProcessing;
}


bool GuiScene::scrollIntoView(Element *elem, const BBox2 &viewport, const Point2 &max_abs_scroll, const Element *scroll_root)
{
  if (viewport.isempty() || viewport.width().lengthSq() < 1e-3f)
    return false;

  BBox2 bbox = elem->calcTransformedBbox();
  if (bbox.isempty())
    return false;

  Point2 delta(0, 0);

  if (bbox.right() > viewport.right())
    delta.x = ::min(bbox.right() - viewport.right(), max_abs_scroll.x);
  else if (bbox.left() < viewport.left())
    delta.x = ::max(bbox.left() - viewport.left(), -max_abs_scroll.x);

  if (bbox.bottom() > viewport.bottom())
    delta.y = ::min(bbox.bottom() - viewport.bottom(), max_abs_scroll.y);
  else if (bbox.top() < viewport.top())
    delta.y = ::max(bbox.top() - viewport.top(), -max_abs_scroll.y);

  if (delta.lengthSq() < 1e-3f)
    return false;

  return hierScroll(elem, delta, scroll_root);
}


bool GuiScene::scrollIntoViewCenter(Element *elem, const BBox2 &viewport, const Point2 &max_abs_scroll, const Element *scroll_root)
{
  if (viewport.isempty() || viewport.width().lengthSq() < 1e-3f)
    return false;

  BBox2 bbox = elem->calcTransformedBbox();
  if (bbox.isempty())
    return false;

  Point2 delta = bbox.center() - viewport.center();
  delta.x = ::clamp(delta.x, -max_abs_scroll.x, max_abs_scroll.x);
  delta.y = ::clamp(delta.y, -max_abs_scroll.y, max_abs_scroll.y);
  if (delta.lengthSq() < 1e-3f)
    return false;

  return hierScroll(elem, delta, scroll_root);
}


bool GuiScene::hierScroll(Element *elem, Point2 delta, const Element *scroll_root)
{
  Element *parent = elem->parent;
  if (!parent || elem == scroll_root)
    return false;

  bool scrolled = false;

  const bool allowAutoScroll =
    parent->scrollHandler || parent->hasBehaviors(Behavior::F_ALLOW_AUTO_SCROLL) || parent->hasFlags(Element::F_JOYSTICK_SCROLL);

  if (allowAutoScroll)
  {
    Point2 prevOffs = parent->screenCoord.scrollOffs;
    const ScreenCoord &psc = parent->screenCoord;

    parent->scrollTo(prevOffs + delta);
    Point2 actualScroll = psc.scrollOffs - prevOffs;

    if (delta.x > 0.0f)
      delta.x = ::max(0.0f, delta.x - actualScroll.x);
    else if (delta.x < 0.0f)
      delta.x = ::min(0.0f, delta.x - actualScroll.x);

    if (delta.y > 0.0f)
      delta.y = ::max(0.0f, delta.y - actualScroll.y);
    else if (delta.y < 0.0f)
      delta.y = ::min(0.0f, delta.y - actualScroll.y);

    if (lengthSq(psc.scrollOffs - prevOffs) > 1e-3f)
      scrolled = true;
  }

  if (delta.lengthSq() > 1e-3f)
    if (hierScroll(parent, delta, scroll_root))
      scrolled = true;

  return scrolled;
}

// dest is assumed to be initialized with default values
template <class T>
static void read_2_axis_props(T dest[2], Sqrat::Object src)
{
  if (src.GetType() == OT_ARRAY)
  {
    Sqrat::Array arr(src);
    SQInteger len = arr.Length();
    if (len > 0)
    {
      dest[0] = arr.RawGetSlotValue(SQInteger(0), dest[0]);
      dest[1] = (len > 1) ? arr.RawGetSlotValue(SQInteger(1), dest[0]) : dest[0]; // with single element use it for both axes
    }
  }
  else
  {
    HSQUIRRELVM vm = src.GetVM();
    sq_pushobject(vm, src);
    if (Sqrat::Var<T>::check_type(vm, -1))
      dest[0] = dest[1] = src.Cast<T>();
    sq_pop(vm, 1);
  }
}

void GuiScene::scrollToXmbFocus(float dt)
{
  if (!xmbFocus)
    return;

  G_ASSERT_RETURN(xmbFocus->xmb, );

  float scrollSpeed[2] = {1.0f, 1.0f};
  bool scrollToEdge[2] = {false, false};
  if (xmbFocus->xmb->xmbParent)
  {
    XmbData *pxmb = xmbFocus->xmb->xmbParent->xmb;
    G_ASSERT(pxmb);
    if (pxmb)
    {
      const Sqrat::Table &parentParams = pxmb->nodeDesc;
      read_2_axis_props(scrollSpeed, parentParams.RawGetSlot(stringKeys->scrollSpeed));
      read_2_axis_props(scrollToEdge, parentParams.RawGetSlot(stringKeys->scrollToEdge));
    }
  }

  Point2 maxScroll(dt * StdGuiRender::screen_height() * scrollSpeed[0], dt * StdGuiRender::screen_height() * scrollSpeed[1]);

  Element *scrollRoot = nullptr;
  BBox2 viewport = xmb::calc_xmb_viewport(xmbFocus, &scrollRoot);

  bool scrolled = false;

  if (scrollToEdge[0] == scrollToEdge[1])
  {
    // both directions simultaneously
    scrolled = scrollToEdge[0] ? scrollIntoView(xmbFocus, viewport, maxScroll, scrollRoot)
                               : scrollIntoViewCenter(xmbFocus, viewport, maxScroll, scrollRoot);
  }
  else
  {
    // 2 distinct operations
    Point2 separateMaxScroll[2] = {Point2(maxScroll[0], 0), Point2(0, maxScroll[1])};
    for (int axis = 0; axis < 2; ++axis)
    {
      bool res = scrollToEdge[axis] ? scrollIntoView(xmbFocus, viewport, separateMaxScroll[axis], scrollRoot)
                                    : scrollIntoViewCenter(xmbFocus, viewport, separateMaxScroll[axis], scrollRoot);
      if (res)
        scrolled = true;
    }
  }

  BBox2 bbox = xmbFocus->calcTransformedBbox();
  if (!scrolled)
  {
    if (!xmbFocus->clippedScreenRect.isempty())
      moveActiveCursorToElemBox(xmbFocus, bbox, false);
  }
  else
  {
    bool needCursorMove = false;
    Point2 targetCursorPos = activePointerPos();
    Point2 vpCenter = viewport.center();

    if ((bbox.right() < vpCenter.x && targetCursorPos.x > vpCenter.x) || (bbox.left() > vpCenter.x && targetCursorPos.x < vpCenter.x))
    {
      needCursorMove = true;
      targetCursorPos.x = vpCenter.x;
    }

    if ((bbox.bottom() < vpCenter.y && targetCursorPos.y > vpCenter.y) || (bbox.top() > vpCenter.y && targetCursorPos.y < vpCenter.y))
    {
      needCursorMove = true;
      targetCursorPos.y = vpCenter.y;
    }

    if (needCursorMove)
      requestActivePointerTargetPos(targetCursorPos);

    requestHoverUpdate();
  }
}


void GuiScene::doJoystickScroll(const Point2 &delta)
{
  auto screen = getFocusedScreen();

  if (!screen || !screen->etree.root)
    return;

  bool canScroll = activeCursor || (screen->panel && screen->panel->hasActiveCursors());
  if (!canScroll)
    return;

  Point2 localPos;
  bool transformValid = screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos);

  bool hasScrolled = transformValid && screen->etree.doJoystickScroll(screen->inputStack, localPos, delta);

  if (hasScrolled)
  {
    // turn off xmb mode if focus is outside of the viewport or moved away from center
    // to avoid it bouncing back
    if (xmbFocus)
      G_ASSERT(xmbFocus->xmb);
    if (xmbFocus && xmbFocus->xmb && !xmbFocus->transformedBbox.isempty() && xmbFocus->etree->screen == screen)
    {
      if (xmbFocus->clippedScreenRect.isempty())
        doSetXmbFocus(nullptr);
      else
      {
        bool scrollToEdge = false;

        if (xmbFocus->xmb->xmbParent)
        {
          XmbData *pxmb = xmbFocus->xmb->xmbParent->xmb;
          G_ASSERT(pxmb);
          if (pxmb)
            scrollToEdge = pxmb->nodeDesc.RawGetSlotValue(stringKeys->scrollToEdge, scrollToEdge);
        }

        if (scrollToEdge)
        {
          BBox2 viewport = xmb::calc_xmb_viewport(xmbFocus, nullptr);
          BBox2 elemBbox = xmbFocus->calcTransformedBboxPadded();
          if (elemBbox.top() < viewport.top() || elemBbox.bottom() > viewport.bottom() || elemBbox.left() < viewport.left() ||
              elemBbox.right() > viewport.right())
          {
            doSetXmbFocus(nullptr);
          }
        }
        else
          doSetXmbFocus(nullptr); // not bouncing back to center
      }
    }

    updateGlobalHover();
  }
}


void GuiScene::queueScriptHandler(BaseScriptHandler *h)
{
  ApiThreadCheck atc(this);

  scriptHandlersQueue.push_back(h);
}


void GuiScene::callScriptHandlers(bool is_shutdown)
{
  if (scriptHandlersQueue.empty())
    return;

  TIME_PROFILE(callScriptHandlers);

  dag::RelocatableFixedVector<BaseScriptHandler *, 8, true, framemem_allocator> queue; // TODO: use unique_ptr
  queue.assign(scriptHandlersQueue.begin(), scriptHandlersQueue.end());
  scriptHandlersQueue.clear();
  if (DAGOR_UNLIKELY(queue.size() > 64))
    scriptHandlersQueue.shrink_to_fit();

  for (BaseScriptHandler *handler : queue)
  {
    if (!is_shutdown || handler->allowOnShutdown)
    {
      DA_PROFILE_EVENT_DESC(handler->dapDescription);
      handler->call();
    }
  }

  clear_all_ptr_items(queue);
}


static bool is_hotkeys_subset(const HotkeyCombo *superset, const HotkeyCombo *subset, bool only_smaller)
{
  const Tab<HotkeyButton> &selBtns = superset->buttons;

  if (subset->buttons.size() > selBtns.size()) // larger subset
    return false;

  if (only_smaller && subset->buttons.size() == selBtns.size())
    return false;

  for (const HotkeyButton &button : subset->buttons)
    if (eastl::find(selBtns.begin(), selBtns.end(), button) == selBtns.end())
      return false;

  return true;
}


static bool is_hotkeys_subset(const HotkeyCombo *selected_combo, Element *candidate, bool only_smaller)
{
  G_ASSERT(!candidate->hotkeyCombos.empty());

  for (const auto &combo : candidate->hotkeyCombos)
  {
    if (combo.get() == selected_combo) // self
      continue;

    if (is_hotkeys_subset(selected_combo, combo.get(), only_smaller))
      return true;
  }

  return false;
}


bool GuiScene::updateHotkeys()
{
  Tab<eastl::pair<HotkeyCombo *, Element *>> updatedCombos(framemem_ptr());
  Tab<eastl::pair<HotkeyCombo *, Element *>> releasedCombos(framemem_ptr());

  size_t maxButtons = 0;

  for (auto &[id, screen] : screens)
  {
    G_ASSERT(screen->etree.rebuildFlagsAccum == 0);

    if (!doesScreenAcceptInput(screen.get()))
      continue;

    bool isStopped = false;

    for (const InputEntry &ie : screen->inputStack.stack)
    {
      for (auto &combo : ie.elem->hotkeyCombos)
      {
        if (!isStopped)
        {
          if (combo->updateOnCombo(getJoystick()))
          {
            auto cmp = [&combo](eastl::pair<HotkeyCombo *, Element *> &item) {
              return is_hotkeys_subset(item.first, combo.get(), false);
            };
            bool isFirstUnique = eastl::find_if(updatedCombos.begin(), updatedCombos.end(), cmp) == updatedCombos.end();
            if (isFirstUnique)
            {
              updatedCombos.push_back(eastl::make_pair(combo.get(), ie.elem));
              maxButtons = ::max<size_t>(maxButtons, combo->buttons.size());
            }
          }
        }
        else
        {
          if (combo->forceReleaseButtons())
            releasedCombos.push_back(eastl::make_pair(combo.get(), ie.elem));
        }
      }

      if (ie.elem->hasFlags(Element::F_STOP_HOTKEYS))
        isStopped = true;
    }
  }

  for (auto &[combo, comboElem] : updatedCombos)
  {
    if (combo->buttons.size() >= maxButtons)
    {
      if (combo->isPressed)
      {
        for (const InputEntry &ie : comboElem->etree->screen->inputStack.stack)
          if (ie.elem != comboElem && !ie.elem->hotkeyCombos.empty() && is_hotkeys_subset(combo, ie.elem, true))
            ie.elem->clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);
      }

      combo->triggerOnCombo(this, comboElem);
    }
  }

  for (auto &[combo, comboElem] : releasedCombos)
    comboElem->clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);

  return !updatedCombos.empty();
}


bool GuiScene::sendEvent(const char *id, const Sqrat::Object &data)
{
  // first try hotkeys

  int fullLen = strlen(id);
  int len = fullLen;
  bool isPressed = true;
  if (fullLen > 4 && strcmp(id + fullLen - 4, ":end") == 0)
  {
    isPressed = false;
    len = fullLen - 4;
  }

  auto handleInputEvent = [this, id, len, isPressed, fullLen](const InputStack &input_stack) {
    bool processed = false;
    for (const InputEntry &ie : input_stack.stack)
    {
      if (ie.elem->hotkeyCombos.size())
      {
        if (ie.elem->isDetached())
        {
          LOGWARN_CTX("Detached element found in input stack");
          continue;
        }
        for (auto &combo : ie.elem->hotkeyCombos)
        {
          if (combo->updateOnEvent(this, ie.elem, id, len, isPressed))
          {
            processed = true;
            break;
          }

          if (!isPressed && combo->updateOnEvent(this, ie.elem, id, fullLen, true)) // handle explicit "<event>:end" separately
          {
            processed = true;
            break;
          }
        }
        if (processed)
          break;
      }
    }

    return processed;
  };

  bool processed = false;

  // Let's see if any panels we own accept the hotkey
  for (auto &[_, sc] : screens)
  {
    if (!doesScreenAcceptInput(sc.get()))
      continue;

    if (handleInputEvent(sc->inputStack))
    {
      processed = true;
      break;
    }
  }

  // then check eventHandlers
  if (!processed)
  {
    sq_pushstring(sqvm, id, fullLen);
    HSQOBJECT hKey;
    sq_getstackobj(sqvm, -1, &hKey);
    Sqrat::Object key(hKey, sqvm);
    sq_pop(sqvm, 1);

    for (auto &[_, sc] : screens)
    {
      if (!doesScreenAcceptInput(sc.get()))
        continue;

      for (const InputEntry &ie : sc->eventHandlersStack.stack)
      {
        if (ie.elem->isDetached())
        {
          LOGWARN_CTX("Detached element found in event handler stack");
          continue;
        }
        Sqrat::Table &desc = ie.elem->props.scriptDesc;
        Sqrat::Object funcObj = desc.RawGetSlot(stringKeys->eventHandlers).RawGetSlot(key);
        if (funcObj.IsNull())
          continue;

        processed = true;

        Sqrat::Function handler(sqvm, desc, funcObj);
        auto result = handler.Eval<Sqrat::Object>(data);
        if (result && (result.value().Cast<int>() != EVENT_CONTINUE))
          break;
      }
    }
  }

  callScriptHandlers();

  return processed;
}


void GuiScene::postEvent(const char *id, const Json::Value &data)
{
  WinAutoLock lock(postedEventsQueueCs);
  postedEventsQueue.push_back(eastl::make_pair(eastl::string(id), data));
}


void GuiScene::processPostedEvents()
{
  G_ASSERT(workingPostedEventsQueue.empty());

  {
    WinAutoLock lock(postedEventsQueueCs);
    workingPostedEventsQueue.swap(postedEventsQueue);
  }

  for (auto &it : workingPostedEventsQueue)
    sendEvent(it.first.c_str(), jsoncpp_to_quirrel(sqvm, it.second));

  workingPostedEventsQueue.clear();
}


void GuiScene::saveToSqVm() { sq_setsharedforeignptr(sqvm, this); }


IGuiScene *get_scene_from_sqvm(HSQUIRRELVM vm) { return GuiScene::get_from_sqvm(vm); }


GuiScene *GuiScene::get_from_sqvm(HSQUIRRELVM vm)
{
  G_ASSERT(vm);
  return (GuiScene *)sq_getsharedforeignptr(vm);
}


GuiScene *GuiScene::get_from_elem(const Element *elem) { return elem ? get_from_sqvm(elem->props.storage.GetVM()) : nullptr; }


static bool sort_invalidated_elems_cb(Element *a, Element *b)
{
  // put elements closer to root to end to efficiently pop from back during rebuild
  return b->getHierDepth() < a->getHierDepth();
}

void GuiScene::invalidateElement(Element *elem)
{
  if (isClearing)
  {
    debug("daRg: invalidating Element during scene cleanup");
    return;
  }

  if (isRebuildingInvalidatedParts && config.reportNestedWatchedUpdate)
  {
    LOGERR_CTX("Nested state update");

    if (SQ_SUCCEEDED(sqstd_formatcallstackstring(sqvm)))
    {
      const char *callstack = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(sqvm, -1, &callstack)));
      LOGERR_CTX(callstack);
      sq_pop(sqvm, 1);
    }
    else
      LOGERR_CTX("No call stack available");
  }

  auto itLower = eastl::lower_bound(invalidatedElements.begin(), invalidatedElements.end(), elem, sort_invalidated_elems_cb);
  auto itUpper = eastl::upper_bound(itLower, invalidatedElements.end(), elem, sort_invalidated_elems_cb);

  bool exists = false;
  for (auto it = itLower; it != itUpper; ++it)
  {
    if (*it == elem)
    {
      exists = true;
      break;
    }
  }
  if (!exists)
    invalidatedElements.insert(itUpper, elem);
}


void GuiScene::deferredRecalLayout(Element *elem) { deferredRecalcLayout.emplace(elem); }


void GuiScene::rebuildInvalidatedParts()
{
  G_ASSERT(!isClearing);

  if (invalidatedElements.empty())
  {
    kbFocus.applyNextFocus();
    return;
  }

  TIME_PROFILE(rebuildInvalidatedParts);

  // Start all element trees rebuild

  for (auto &[id, sc] : screens)
  {
    G_ASSERT(sc->etree.rebuildFlagsAccum == 0);
    sc->etree.rebuildFlagsAccum = 0;
  }

  for (Cursor *c : allCursors)
  {
    G_ASSERT(c->etree.rebuildFlagsAccum == 0);
    c->etree.rebuildFlagsAccum = 0;
  }

  isRebuildingInvalidatedParts = true;

  G_ASSERT(layoutRecalcFixedSizeRoots.empty());
  G_ASSERT(layoutRecalcSizeRoots.empty());
  G_ASSERT(layoutRecalcFlowRoots.empty());
  G_ASSERT(allRebuiltElems.empty());

  layoutRecalcFixedSizeRoots.reserve(invalidatedElements.size());
  layoutRecalcSizeRoots.reserve(invalidatedElements.size());
  layoutRecalcFlowRoots.reserve(invalidatedElements.size());
  allRebuiltElems.reserve(invalidatedElements.size());

  Component comp;

  // NOTE: the order of invalidated elements is important
  // The ones closer to root are in the end and should be rebuilt first
  // (See invalidateElement() and sort_invalidated_elems_cb() for details)
  for (; !invalidatedElements.empty() && !status.isShowingFullErrorDetails;)
  {
    G_ASSERT(!isClearing);

    Element *elem = invalidatedElements.back();
    invalidatedElements.pop_back();

    if (elem->props.scriptBuilder.IsNull())
    {
      darg_report_unrebuildable_element(elem, "Trying to rebuild component not defined by function");
    }
    else if (Component::build_component(comp, elem->props.scriptBuilder, stringKeys, elem->props.scriptBuilder))
    {
      int rebuildResult = 0;
      {
        TIME_PROFILE(etree_rebuild);
        Element *res = elem->etree->rebuild(sqvm, elem, comp, elem->parent, comp.scriptBuilder, 0, rebuildResult);
        G_UNUSED(res);
        G_ASSERT(res == elem);
        G_ASSERT(res);
      }

      Element *fixedSizeRoot, *sizeRoot, *flowRoot;
      elem->getSizeRoots(fixedSizeRoot, sizeRoot, flowRoot);

      if (!fixedSizeRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE))
      {
        layoutRecalcFixedSizeRoots.push_back(fixedSizeRoot);
        fixedSizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE, true);
      }

      if (!sizeRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_SIZE))
      {
        layoutRecalcSizeRoots.push_back(sizeRoot);
        sizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_SIZE, true);
      }

      if (!flowRoot->hasFlags(Element::F_LAYOUT_RECALC_PENDING_FLOW))
      {
        layoutRecalcFlowRoots.push_back(flowRoot);
        flowRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FLOW, true);
      }

      allRebuiltElems.push_back(elem);

      elem->etree->rebuildFlagsAccum |= rebuildResult;

      checkCursorHotspotUpdate(elem);
    }
    else
    {
      String funcName;
      get_closure_full_name(elem->props.scriptBuilder, funcName);
      String errMsg(0, "Failed to rebuild component %s", funcName.c_str());
      darg_immediate_error(sqvm, errMsg);
    }
  }

  {
    TIME_PROFILE(removeDuplicateRecalcRoots);
    removeRootsWithParentFlag(layoutRecalcFixedSizeRoots, Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE);
    removeRootsWithParentFlag(layoutRecalcSizeRoots, Element::F_LAYOUT_RECALC_PENDING_SIZE);
    removeRootsWithParentFlag(layoutRecalcFlowRoots, Element::F_LAYOUT_RECALC_PENDING_FLOW);
  }

  recalcLayoutFromRoots(make_span(layoutRecalcFixedSizeRoots), make_span(layoutRecalcSizeRoots), make_span(layoutRecalcFlowRoots));

  for (Element *e : allRebuiltElems)
  {
    e->callScriptAttach(this);
  }

  layoutRecalcFixedSizeRoots.clear();
  layoutRecalcSizeRoots.clear();
  layoutRecalcFlowRoots.clear();
  allRebuiltElems.clear();

  isRebuildingInvalidatedParts = false;

  // The same logic as in update()
  int prevInteractiveFlags = calcInteractiveFlags();
  bool stacksRebuilt = false, hotkeysModified = false;

  for (auto &[id, screen] : screens)
  {
    int updRes = screen->etree.rebuildFlagsAccum;
    if (updRes & rebuild_stacks_flags)
    {
      screen->rebuildStacks();
      stacksRebuilt = true;
    }
    if (updRes & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED)
      hotkeysModified = true;
    if (updRes & ElementTree::RESULT_NEED_XMB_REBUILD)
      screen->rebuildXmb();
    screen->etree.rebuildFlagsAccum = 0;
  }

  if (stacksRebuilt || hotkeysModified)
    applyInteractivityChange(prevInteractiveFlags, hotkeysModified, /*update_global_hover*/ true);

  for (Cursor *c : allCursors)
  {
    if (c->etree.rebuildFlagsAccum & rebuild_stacks_flags)
      c->rebuildStacks();

    c->etree.rebuildFlagsAccum = 0;
  }

  kbFocus.applyNextFocus();
}


void GuiScene::recalcLayoutFromRoots(dag::Span<Element *> fixed_size_roots, dag::Span<Element *> size_roots,
  dag::Span<Element *> flow_roots)
{
  AutoProfileScope profile(getProfiler(), M_RECALC_LAYOUT);
  TIME_PROFILE(elem_recalcLayoutFromRoots);

  for (Element *fixedSizeRoot : fixed_size_roots)
  {
    fixedSizeRoot->calcFixedSizes();
    fixedSizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FIXED_SIZE, false);
  }

  for (Element *sizeRoot : size_roots)
  {
    sizeRoot->calcConstrainedSizes(0);
    sizeRoot->calcConstrainedSizes(1);
    sizeRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_SIZE, false);
  }

  for (Element *flowRoot : flow_roots)
  {
    flowRoot->recalcScreenPositions();
    flowRoot->updFlags(Element::F_LAYOUT_RECALC_PENDING_FLOW, false);
  }
}


void GuiScene::onElementDetached(Element *elem)
{
  // possibly could exist in these lists, but let's see
  G_ASSERT(eastl::find(allRebuiltElems.begin(), allRebuiltElems.end(), elem) == allRebuiltElems.end());

  auto it = eastl::find(invalidatedElements.begin(), invalidatedElements.end(), elem);
  if (it != invalidatedElements.end())
    invalidatedElements.erase(it);

  if (elem == xmbFocus)
    doSetXmbFocus(nullptr);

  erase_item_by_value(keptXmbFocus, elem);

  deferredRecalcLayout.erase(elem);
}


void GuiScene::validateAfterRebuild(Element *elem)
{
  // possibly could exist in these lists, but let's see
  G_ASSERT(eastl::find(allRebuiltElems.begin(), allRebuiltElems.end(), elem) == allRebuiltElems.end());

  auto it = eastl::find(invalidatedElements.begin(), invalidatedElements.end(), elem);
  if (it != invalidatedElements.end())
    invalidatedElements.erase(it);
}


void GuiScene::checkCursorHotspotUpdate(Element *elem_rebuilt)
{
  for (Cursor *c : allCursors)
  {
    if (elem_rebuilt->etree == &c->etree)
    {
      G_ASSERT(c->etree.root && c->etree.root->children.size() == 1);
      if (c->etree.root && c->etree.root->children.size() && elem_rebuilt == c->etree.root->children[0])
        c->readHotspot();
      break;
    }
  }
}

static void ensure_no_modules_loaded_yet(SqModules *module_mgr)
{
  eastl::vector<eastl::string> loadedModules;
  module_mgr->getLoadedModules(loadedModules);

  // G_ASSERTF(!moduleMgr->modules.size(), "Expected empty modules list on script run");
  if (!loadedModules.empty())
  {
    DEBUG_CTX("daRg: non-empty modules list on scene startup:");
    for (auto &name : loadedModules)
      DEBUG_CTX("  * %s", name.c_str());
  }
}

void GuiScene::runScriptScene(const char *fn)
{
  ApiThreadCheck atc(this);
  G_ASSERT(sqvm);

  clear(false);

  enable_kb_layout_and_locks_tracking();

  mousePointer.initMousePos();

  ensure_no_modules_loaded_yet(moduleMgr.get());

  reloadScript(fn);
}


void GuiScene::reloadScript(const char *fn)
{
  TIME_PROFILE(GuiScene_reloadScript);

  ApiThreadCheck atc(this);

  clear(false);
  dasScriptsData->resetBeforeReload();
  frpGraph->setName(fn);

  G_ASSERT(screens.empty());
  Screen *screen = createGuiScreen(MAIN_SCREEN_ID);
  screen->isMainScreen = true;

  // Ensure that string is always valid in this scope.
  // This may be too paranoid but in practice it happenned that the provided pointer to the string
  // was invalidated during script execution (in game settings datablock reload).
  String fnCopy(fn);

  Sqrat::Object rootDesc;
  Sqrat::string errMsg;
  if (!moduleMgr->reloadModule(fnCopy.c_str(), true, SqModules::__main__, rootDesc, errMsg))
  {
    String msg;
    msg.printf(256, "Failed to load script module %s: %s", fnCopy.c_str(), errMsg.c_str());
    errorMessage(msg);
    // DAG_FATAL(msg);
    rootDesc = Sqrat::Table(sqvm);
  }

#if DAGOR_DBGLEVEL > 0
  // disabled for now because it is false-positive on persist-ed observables
  // frpGraph->checkLeaks();
#endif

  int rebuildResult = 0;

  {
    Component root;
    G_VERIFY(Component::build_component(root, rootDesc, stringKeys, rootDesc));
    screen->etree.root = screen->etree.rebuild(sqvm, screen->etree.root, root, nullptr, root.scriptBuilder, 0, rebuildResult);
  }

  sq_collectgarbage(sqvm);

#if DAGOR_DBGLEVEL > 0
  {
    // Log graph stats
    sqfrp::ObservablesGraph::Stats s = frpGraph->gatherGraphStats();
    DEBUG_CTX("[daRg][FRP] after %s reload and rebuild: "
              "Watched: %d (%d unused, %d not subscribed), Computed: %d (%d unused, %d not subscribed), "
              "%d subscribers, %d watchers",
      fn, s.watchedTotal, s.watchedUnused, s.watchedUnsubscribed, s.computedTotal, s.computedUnused, s.computedUnsubscribed,
      s.totalSubscribers, s.totalWatchers);
  }
#endif

  if (screen->etree.root == nullptr)
    darg_immediate_error(sqvm, String(0, "reloadScript failed: %s", fnCopy.c_str()));

  if (screen->etree.root)
  {
    screen->etree.root->recalcLayout();
    screen->etree.root->callScriptAttach(this);
  }

  if (rebuildResult & rebuild_stacks_flags)
    rebuildStacksAndNotify(screen, /*refresh_hotkeys_nav*/ rebuildResult & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED,
      /*update_global_hover*/ true);
  if (rebuildResult & ElementTree::RESULT_NEED_XMB_REBUILD)
    screen->rebuildXmb();

  screen->etree.rebuildFlagsAccum = 0;

  dasScriptsData->cleanupAfterReload();

  // ignore time spent for reloading, exclude it from all daRg timers used for behaviors and animations
  lastRenderTimestamp = get_time_msec();
}


bool GuiScene::hasInteractiveElements() const
{
  ApiThreadCheck atc(this);
  return hasInteractiveElements(Behavior::F_HANDLE_KEYBOARD | Behavior::F_HANDLE_MOUSE | Behavior::F_INTERNALLY_HANDLE_GAMEPAD_STICKS);
}


bool GuiScene::hasInteractiveElements(int bhv_flags) const
{
  if (status.isShowingFullErrorDetails || !isInputActive())
    return false;

  int res = 0;
  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()))
      continue;

    res |= screen->inputStack.summaryBhvFlags;
  }
  return (res & bhv_flags) != 0;
}


int GuiScene::calcInteractiveFlags() const
{
  if (status.isShowingFullErrorDetails || !isInputActive())
    return 0;

  int iflags = 0;

  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()))
      continue;

    const int bhvFlags = screen->inputStack.summaryBhvFlags;
    if (((bhvFlags & (Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK)) || isCursorForcefullyEnabled()) &&
        !isCursorForcefullyDisabled())
    {
      iflags |= IGuiSceneCallback::IF_MOUSE;
      if ((bhvFlags & Behavior::F_OVERRIDE_GAMEPAD_STICK) == 0)
        iflags |= IGuiSceneCallback::IF_GAMEPAD_AS_MOUSE;
    }
    if (bhvFlags & Behavior::F_HANDLE_KEYBOARD)
      iflags |= IGuiSceneCallback::IF_KEYBOARD;
    if (bhvFlags & Behavior::F_INTERNALLY_HANDLE_GAMEPAD_STICKS)
      iflags |= IGuiSceneCallback::IF_GAMEPAD_STICKS;
    if (screen.get() == focusedScreen && screen->inputStack.isDirPadNavigable)
      iflags |= IGuiSceneCallback::IF_DIRPAD_NAV;
  }

  return iflags;
}


void GuiScene::rebuildStacksAndNotify(Screen *screen, bool refresh_hotkeys_nav, bool update_global_hover)
{
  TIME_PROFILE(rebuildStacksAndNotify);

  int prevInteractiveFlags = calcInteractiveFlags();
  screen->rebuildStacks();
  applyInteractivityChange(prevInteractiveFlags, refresh_hotkeys_nav, update_global_hover);
}


void GuiScene::rebuildAllScreensStacksAndNotify(bool refresh_hotkeys_nav, bool update_global_hover)
{
  TIME_PROFILE(rebuildAllScreensStacksAndNotify);

  int prevInteractiveFlags = calcInteractiveFlags();
  for (auto &[id, screen] : screens)
    screen->rebuildStacks();
  applyInteractivityChange(prevInteractiveFlags, refresh_hotkeys_nav, update_global_hover);
}


void GuiScene::applyInteractivityChange(int prev_iflags, bool refresh_hotkeys_nav, bool update_global_hover)
{
  TIME_PROFILE(applyInteractivityChange);
  int curInteractiveFlags = calcInteractiveFlags();
  bool mouseFlagsChanged = (curInteractiveFlags & IGuiSceneCallback::IF_MOUSE) != (prev_iflags & IGuiSceneCallback::IF_MOUSE);
  bool dirPadNavChanged = (curInteractiveFlags & IGuiSceneCallback::IF_DIRPAD_NAV) != (prev_iflags & IGuiSceneCallback::IF_DIRPAD_NAV);

  if (refresh_hotkeys_nav)
  {
    TIME_PROFILE(refreshHotkeysNav);
    refreshHotkeysNav();
  }

  if (refresh_hotkeys_nav || mouseFlagsChanged || dirPadNavChanged)
    notifyInputConsumersCallback();

  if (mouseFlagsChanged || update_global_hover)
    updateGlobalHover();

  if (guiSceneCb && curInteractiveFlags != prev_iflags)
    guiSceneCb->onToggleInteractive(curInteractiveFlags);
}


void GuiScene::setSceneInputActive(bool active)
{
  ApiThreadCheck atc(this);

  bool inputWasActive = isInputActive();

  isInputEnabledByHost = active;

  if (isInputActive() != inputWasActive)
    applyInputActivityChange();
}


void GuiScene::scriptSetSceneInputActive(bool active)
{
  ApiThreadCheck atc(this);

  bool inputWasActive = isInputActive();

  isInputEnabledByScript = active;

  if (isInputActive() != inputWasActive)
    applyInputActivityChange();
}


void GuiScene::applyInputActivityChange()
{
  if (guiSceneCb)
    guiSceneCb->onToggleInteractive(calcInteractiveFlags());

  if (isInputActive())
    mousePointer.onActivateSceneInput();
  else
  {
    for (auto &[id, screen] : screens)
      screen->etree.deactivateAllInput();
    pressedClickButtons.clear();
  }

  notifyInputConsumersCallback();
}

bool GuiScene::doesScreenAcceptInput(const Screen *screen) const
{
  // TODO: the logic is to be reviewed
  if (!screen->etree.canHandleInput)
    return false;
  if (config.allScreensAcceptInput)
    return true;
  return screen->isMainScreen || screen == focusedScreen;
}


void GuiScene::refreshHotkeysNav()
{
  if (sceneScriptHandlers.onHotkeysNav.IsNull())
    return;

  HumanInput::IGenJoystick *joy = getJoystick();
  HumanInput::IGenPointing *mouse = MousePointer::get_mouse();
  HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : nullptr;

  eastl::vector_set<int> collected;
  Sqrat::Array res(sqvm);
  Sqrat::Object nullObj;

  StringKeys *csk = stringKeys;
  Hotkeys &hotkeys_ = hotkeys;
  auto save_hotkey_button_to_sq_table = [csk, &hotkeys_, joy, mouse, kbd](Sqrat::Table &tbl, const HotkeyButton &btn) {
    tbl.SetValue(csk->devId, btn.devId);
    tbl.SetValue(csk->btnId, btn.btnId);
    if (btn.devId == DEVID_JOYSTICK)
    {
      const char *btnName = hotkeys_.getButtonName(HotkeyButton(DEVID_JOYSTICK, btn.btnId));
      if (!btnName)
        btnName = joy ? joy->getButtonName(btn.btnId) : nullptr;
      tbl.SetValue(csk->btnName, btnName ? btnName : String(8, "?J:%d", btn.btnId).c_str());
    }
    else if (btn.devId == DEVID_KEYBOARD)
    {
      const char *btnName = kbd ? kbd->getKeyName(btn.btnId) : nullptr;
      tbl.SetValue(csk->btnName, btnName ? btnName : String(8, "?K:%d", btn.btnId).c_str());
    }
    else if (btn.devId == DEVID_MOUSE)
    {
      const char *btnName = mouse ? mouse->getBtnName(btn.btnId) : nullptr;
      tbl.SetValue(csk->btnName, btnName ? btnName : String(8, "?M:%d", btn.btnId).c_str());
    }
    else
      G_ASSERTF(0, "Unexpected device id %d", btn.devId);
  };

  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()))
      continue;

    for (const InputEntry &ie : screen->inputStack.stack)
    {
      Element *elem = ie.elem;
      if (elem->isDetached())
      {
        LOGWARN_CTX("Detached element found in input stack");
        continue;
      }

      for (const auto &combo : elem->hotkeyCombos)
      {
        if (!combo->eventName.empty())
        {
          Sqrat::Table item(sqvm);
          item.SetValue(csk->eventName, combo->eventName.c_str());
          item.SetValue(csk->description, combo->description);
          item.SetValue(csk->devId, nullObj);
          item.SetValue(csk->btnId, nullObj);
          item.SetValue(csk->btnName, nullObj);
          item.SetValue(csk->buttons, nullObj);
          item.SetValue(csk->action, Sqrat::Object(combo->handler.GetFunc(), sqvm));
          res.Append(item);
        }
        else if (combo->buttons.size())
        {
          if (combo->buttons.size() == 1)
          {
            HotkeyButton &btn = combo->buttons[0];
            int uniqueKey = (btn.devId << 24) | (btn.btnId & 0xFFFFFF);
            if (collected.find(uniqueKey) != collected.end())
              continue;
            collected.insert(uniqueKey);
            // No check for multi-button shortcuts yet
            // Pass them all to the script for all positions in the input stack
          }

          Sqrat::Table item(sqvm);
          if (combo->buttons.size() == 1)
            save_hotkey_button_to_sq_table(item, combo->buttons[0]);
          else
          {
            item.SetValue(csk->devId, nullObj);
            item.SetValue(csk->btnId, nullObj);
            item.SetValue(csk->btnName, nullObj);
          }

          Sqrat::Array buttons(sqvm, combo->buttons.size());
          for (int iBtn = 0, nBtns = combo->buttons.size(); iBtn < nBtns; ++iBtn)
          {
            Sqrat::Table btnSq(sqvm);
            save_hotkey_button_to_sq_table(btnSq, combo->buttons[iBtn]);
            buttons.SetValue(SQInteger(iBtn), btnSq);
          }
          item.SetValue(csk->buttons, buttons);
          item.SetValue(csk->description, combo->description);
          item.SetValue(csk->eventName, nullObj);
          item.SetValue(csk->action, Sqrat::Object(combo->handler.GetFunc(), sqvm));

          res.Append(item);
        }
      }

      if (elem->hasFlags(Element::F_STOP_HOTKEYS))
        break;
    }
  }

  sceneScriptHandlers.onHotkeysNav(res);
}


void GuiScene::notifyInputConsumersCallback()
{
  ApiThreadCheck atc(this);

  using namespace HumanInput;

  if (!guiSceneCb || !guiSceneCb->useInputConsumersCallback())
    return;

  Tab<HotkeyButton> buttons(framemem_ptr());
  Tab<String> events(framemem_ptr());

  for (auto &[id, screen] : screens)
  {
    if (!doesScreenAcceptInput(screen.get()))
      continue;

    for (const InputEntry &ie : screen->inputStack.stack)
    {
      Element *elem = ie.elem;
      if (elem->isDetached())
      {
        LOGWARN_CTX("Detached element found in input stack");
        continue;
      }

      for (const auto &combo : elem->hotkeyCombos)
      {
        if (combo->ignoreConsumerCallback)
          continue;

        if (!combo->eventName.empty())
          events.push_back(combo->eventName);
        else
        {
          for (const HotkeyButton &btn : combo->buttons)
            buttons.push_back(btn);
        }
      }

      for (Behavior *bhv : elem->behaviors)
        bhv->collectConsumableButtons(elem, buttons);

      if (elem->hasFlags(Element::F_STOP_HOTKEYS))
        break;
    }

    Point2 localPos;
    if (screen->displayToLocal(activePointerPos(), lastCameraTransform, lastCameraFrustum, localPos))
    {
      for (const InputEntry &ie : screen->inputStack.stack)
      {
        if (ie.elem->hitTest(localPos))
        {
          if (ie.elem->hasFlags(Element::F_JOYSTICK_SCROLL) && ie.elem->getAvailableScrolls() != 0)
          {
            buttons.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, config.joystickScrollAxisH));
            buttons.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, config.joystickScrollAxisV));
            break;
          }
          if (ie.elem->hasFlags(Element::F_STOP_HOVER | Element::F_STOP_POINTING))
            break;
        }
      }
    }

    if (screen.get() == focusedScreen && screen->inputStack.isDirPadNavigable)
    {
      buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_RIGHT));
      buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_LEFT));
      buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_DOWN));
      buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_UP));
    }
  }

  bool consumeMouse = hasInteractiveElements(Behavior::F_HANDLE_MOUSE) || isCursorForcefullyEnabled();
  bool consumeTextInput = kbFocus.focus && is_text_input(kbFocus.focus);

  if (consumeMouse)
    append_items(buttons, config.clickButtons.size(), config.clickButtons.data());

  guiSceneCb->onInputConsumersChange(buttons, events, consumeTextInput);
}


void GuiScene::errorMessage(const char *msg)
{
  logerr("daRg: %s", msg);

  bool wasShowingErrDetails = status.isShowingFullErrorDetails;

  status.appendErrorText(msg);

  if (!wasShowingErrDetails && status.isShowingFullErrorDetails && guiSceneCb)
    guiSceneCb->onToggleInteractive(calcInteractiveFlags());
}

void GuiScene::errorMessageWithCb(const char *msg)
{
  errorMessage(msg);
  if (guiSceneCb)
    guiSceneCb->onScriptError(this, msg);
}

void GuiScene::activateProfiler(bool on)
{
  ApiThreadCheck atc(this);

  if (on)
  {
    if (!profiler.get())
      profiler.reset(new Profiler());
  }
  else
    profiler.reset();
}

template <void (ElementTree::*method)(const Sqrat::Object &)>
SQInteger GuiScene::call_anim_method(HSQUIRRELVM vm)
{
  HSQOBJECT triggerObj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &triggerObj)));

  if (sq_type(triggerObj) == OT_NULL)
    return 0;

  Sqrat::Object trigger(triggerObj, vm);
  GuiScene *self = get_from_sqvm(vm);

  for (auto &[id, sc] : self->screens)
    (sc->etree.*method)(trigger);

  return 0;
}


SQInteger GuiScene::anim_pause(HSQUIRRELVM vm)
{
  HSQOBJECT triggerObj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &triggerObj)));

  if (sq_type(triggerObj) == OT_NULL)
    return 0;

  SQBool setPause = true;
  if (sq_gettop(vm) >= 3)
    sq_tobool(vm, 3, &setPause);

  Sqrat::Object trigger(triggerObj, vm);
  GuiScene *self = get_from_sqvm(vm);

  for (auto &[id, sc] : self->screens)
    sc->etree.pauseAnimation(trigger, setPause);

  return 0;
}


static Element *find_input_stack_elem_by_cb(InputStack &input_stack, Sqrat::Function &cb)
{
  for (InputEntry &ie : input_stack.stack)
    if (cb.Eval<bool>(ie.elem, ie.elem->props.scriptDesc).value_or(false))
      return ie.elem;

  return nullptr;
}

static Element *find_input_stack_elem_by_key(HSQUIRRELVM vm, InputStack &input_stack, const HSQOBJECT &obj)
{
  for (InputEntry &ie : input_stack.stack)
  {
    Sqrat::Object elemKey = ie.elem->props.scriptDesc.RawGetSlot(ie.elem->csk->key);
    if (!elemKey.IsNull() && are_sq_obj_equal(vm, obj, elemKey.GetObject()))
      return ie.elem;
  }

  return nullptr;
}


SQInteger GuiScene::set_kb_focus_impl(HSQUIRRELVM vm, bool capture)
{
  HSQOBJECT obj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &obj)));
  SQObjectType tp = sq_type(obj);

  GuiScene *self = get_from_sqvm(vm);

  if (tp == OT_NULL)
  {
    self->kbFocus.setFocus(nullptr, capture);
    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_INSTANCE && Sqrat::ClassType<ElementRef>::IsObjectOfClass(&obj))
  {
    ElementRef *ref = ElementRef::get_from_stack(vm, 2);
    if (!ref || !ref->elem)
      return sq_throwerror(vm, "Invalid ElementRef");
    self->kbFocus.setFocus(ref->elem, capture);

    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
  {
    Sqrat::Function f(vm, Sqrat::Object(vm), obj);
    G_ASSERT(!f.IsNull());

    self->rebuildAllScreensStacksAndNotify(false, /*update_global_hover*/ false);
    for (auto &[id, screen] : self->screens)
    {
      if (!self->doesScreenAcceptInput(screen.get()))
        continue;
      if (Element *elem = find_input_stack_elem_by_cb(screen->inputStack, f))
      {
        self->kbFocus.setFocus(elem, capture);
        sq_pushbool(vm, SQTrue);
        return 1;
      }
    }
  }
  else
  {
    self->rebuildAllScreensStacksAndNotify(false, /*update_global_hover*/ false);
    for (auto &[id, screen] : self->screens)
    {
      if (!self->doesScreenAcceptInput(screen.get()))
        continue;
      if (Element *elem = find_input_stack_elem_by_key(vm, screen->inputStack, obj))
      {
        self->kbFocus.setFocus(elem, capture);
        sq_pushbool(vm, SQTrue);
        return 1;
      }
    }
  }

  sq_pushbool(vm, SQFalse);
  return 1;
}


SQInteger GuiScene::set_kb_focus(HSQUIRRELVM vm) { return set_kb_focus_impl(vm, false); }


SQInteger GuiScene::capture_kb_focus(HSQUIRRELVM vm) { return set_kb_focus_impl(vm, true); }


SQInteger GuiScene::move_mouse_cursor(HSQUIRRELVM vm)
{
  GuiScene *self = get_from_sqvm(vm);

  HSQOBJECT obj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &obj)));
  SQObjectType tp = sq_type(obj);

  SQBool useTransform = true;
  if (sq_gettop(vm) > 2)
    G_VERIFY(SQ_SUCCEEDED(sq_getbool(vm, 3, &useTransform)));

  if (tp == OT_NULL)
  {
    sq_pushbool(vm, SQFalse);
    return 1;
  }
  else if (tp == OT_INSTANCE && Sqrat::ClassType<ElementRef>::IsObjectOfClass(&obj))
  {
    ElementRef *ref = ElementRef::get_from_stack(vm, 2);
    if (!ref || !ref->elem)
      return sq_throwerror(vm, "Invalid ElementRef");
    self->moveActiveCursorToElem(ref->elem, useTransform);
    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
  {
    Sqrat::Function f(vm, Sqrat::Object(vm), obj);
    G_ASSERT(!f.IsNull());

    self->rebuildAllScreensStacksAndNotify(false, /*update_global_hover*/ false);
    for (auto &[id, screen] : self->screens)
    {
      if (!self->doesScreenAcceptInput(screen.get()))
        continue;
      if (Element *elem = find_input_stack_elem_by_cb(screen->inputStack, f))
      {
        self->moveActiveCursorToElem(elem, useTransform);
        sq_pushbool(vm, SQTrue);
        return 1;
      }
    }
  }
  else
  {
    self->rebuildAllScreensStacksAndNotify(false, /*update_global_hover*/ false);
    for (auto &[id, screen] : self->screens)
    {
      if (!self->doesScreenAcceptInput(screen.get()))
        continue;
      if (Element *elem = find_input_stack_elem_by_key(vm, screen->inputStack, obj))
      {
        self->moveActiveCursorToElem(elem, useTransform);
        sq_pushbool(vm, SQTrue);
        return 1;
      }
    }
  }

  sq_pushbool(vm, SQFalse);
  return 1;
}


SQInteger GuiScene::set_update_handler(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;
  Sqrat::Var<GuiScene *> self(vm, 1);
  Sqrat::Var<Sqrat::Object> funcObj(vm, 2);
  self.value->sceneScriptHandlers.onUpdate = Sqrat::Function(vm, Sqrat::Object(vm), funcObj.value);
  return 0;
}


SQInteger GuiScene::set_shutdown_handler(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;
  Sqrat::Var<GuiScene *> self(vm, 1);
  Sqrat::Var<Sqrat::Object> funcObj(vm, 2);
  self.value->sceneScriptHandlers.onShutdown = Sqrat::Function(vm, Sqrat::Object(vm), funcObj.value);
  return 0;
}


SQInteger GuiScene::set_hotkeys_nav_handler(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;
  Sqrat::Var<GuiScene *> self(vm, 1);
  Sqrat::Var<Sqrat::Object> funcObj(vm, 2);
  self.value->sceneScriptHandlers.onHotkeysNav = Sqrat::Function(vm, Sqrat::Object(vm), funcObj.value);
  return 0;
}


SQInteger GuiScene::add_panel(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;

  Sqrat::Var<GuiScene *> varSelf(vm, 1);
  GuiScene *scene = varSelf.value;

  if (scene->isRebuildingInvalidatedParts)
    return sq_throwerror(vm, "Can't add panels during scene rebuild (probably incorrect side-effect in component builder?)");


  SQInteger idx = -1;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &idx)));
  if (idx < 0)
    return sq_throwerror(vm, "Panel id must be non-negative");

  if (scene->panels.find(idx) != scene->panels.end())
    return sqstd_throwerrorf(vm, "Panel with id %d already exists", idx);

  Sqrat::Var<Sqrat::Object> desc(vm, 3);
  Panel *panel = new Panel(*scene, desc.value, idx);
  scene->panels[idx].reset(panel);

  return 0;
}


SQInteger GuiScene::remove_panel(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;

  Sqrat::Var<GuiScene *> varSelf(vm, 1);
  GuiScene *scene = varSelf.value;

  if (scene->isRebuildingInvalidatedParts)
    return sq_throwerror(vm, "Can't remove panels during scene rebuild (probably incorrect side-effect in component builder?)");

  SQInteger idx = -1;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &idx)));

  auto it = scene->panels.find(idx);
  if (it == scene->panels.end())
    return 0;

  auto &panel = it->second;
  for (int hand = 0; hand < NUM_VR_POINTERS; ++hand)
    if (panel.get() == scene->vrPointers[hand].activePanel)
      scene->vrPointers[hand].activePanel = nullptr;

  scene->spatialInteractionState.forgetPanel(idx);
  scene->panels.erase(idx);
  return 0;
}


SQInteger GuiScene::mark_panel_dirty(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;

  Sqrat::Var<GuiScene *> varSelf(vm, 1);
  GuiScene *scene = varSelf.value;
  SQInteger idx = -1;
  G_VERIFY(SQ_SUCCEEDED(sq_getinteger(vm, 2, &idx)));

  auto it = scene->panels.find(idx);
  if (it == scene->panels.end())
    return 0;

  it->second->isDirty = true;

  return 0;
}


SQInteger GuiScene::set_timer(HSQUIRRELVM vm, bool periodic, bool reuse)
{
  if (!Sqrat::check_signature<GuiScene *>(vm))
    return SQ_ERROR;

  Sqrat::Var<GuiScene *> self(vm, 1);
  if (self.value->isClearing)
  {
    if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
    {
      const char *callstack = nullptr;
      G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &callstack)));
      LOGERR_CTX(callstack);
      sq_pop(vm, 1);
      G_ASSERT(!"setTimeout/setInterval() called during clearing scene, see log for script call stack");
    }
    else
      G_ASSERT(!"setTimeout/setInterval() called during clearing scene, no call stack available");

    return 0;
  }

  float dt;
  sq_getfloat(vm, 2, &dt);
  Sqrat::Var<Sqrat::Function> handler(vm, 3);
  Sqrat::Object id;
  if (sq_gettop(vm) > 3)
  {
    id = Sqrat::Var<Sqrat::Object>(vm, 4).value;
    if (id.IsNull())
      return sq_throwerror(vm, "Cannot use null as timer id");
  }
  else
    id = Sqrat::Object(handler.value.GetFunc(), vm);

  void *idFunc = nullptr;
  if (sq_isclosure(id.GetObject()))
    idFunc = (void *)id.GetObject()._unVal.pClosure->_function;
  else if (sq_isnativeclosure(id.GetObject()))
    idFunc = (void *)id.GetObject()._unVal.pNativeClosure->_function;

  Timer *existing = nullptr;
  for (Timer *t : self.value->timers)
  {
    if (are_sq_obj_equal(vm, t->id, id))
    {
      if (reuse)
      {
        existing = t;
        break;
      }
      else
        return sq_throwerror(vm, "Duplicate timer id");
    }
    else if (idFunc && id.GetType() == t->id.GetType() && sq_gettop(vm) < 4)
    {
      void *otherFunc;
      if (sq_isclosure(id.GetObject()))
        otherFunc = (void *)t->id.GetObject()._unVal.pClosure->_function;
      else
        otherFunc = (void *)t->id.GetObject()._unVal.pNativeClosure->_function;
      if (otherFunc == idFunc)
      {
        if (SQ_SUCCEEDED(sqstd_formatcallstackstring(vm)))
        {
          const char *callstack = nullptr;
          G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, -1, &callstack)));
          LOGERR_CTX("Timer id is a duplicate of existing closure, use explicit id if it's intended. Callstack:\n%s", callstack);
          sq_pop(vm, 1);
        }
        else
          LOGERR_CTX("Timer id is a duplicate of existing closure, use explicit id if it's intended. No callstack available.");
      }
    }
  }

  Timer *t = existing ? existing : new Timer(id);
  if (periodic)
    t->initPeriodic(dt, handler.value);
  else
    t->initOneShot(dt, handler.value);
  if (!existing)
    self.value->timers.push_back(t);
  sq_pushobject(vm, id);
  return 1;
}


void GuiScene::clearTimer(const Sqrat::Object &id_)
{
  G_ASSERT(!lockTimersList);
  const HSQOBJECT &id = id_.GetObject();
  HSQUIRRELVM vm = id_.GetVM();
  for (int i = timers.size() - 1; i >= 0; --i)
  {
    Timer *t = timers[i];
    if (are_sq_obj_equal(vm, t->id, id))
    {
      erase_items(timers, i, 1);
      delete t;
      break; // timer ids are unique
    }
  }
}


void GuiScene::updateTimers(float dt)
{
  G_ASSERT(!lockTimersList);

  lockTimersList = true;

  Tab<Sqrat::Function> cbQueue(framemem_ptr());

  for (int i = timers.size() - 1; i >= 0; --i)
    timers[i]->update(dt, cbQueue);

  for (int i = timers.size() - 1; i >= 0; --i)
  {
    if (timers[i]->isFinished())
    {
      Timer *t = timers[i];
      erase_items(timers, i, 1);
      delete t;
    }
  }

  lockTimersList = false;

  for (Sqrat::Function &f : cbQueue)
    f.Execute();
}


static SQInteger calc_comp_size(HSQUIRRELVM vm)
{
  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  if (!scene)
    return sq_throwerror(vm, "calc_comp_size: GuiScene is not created");

  Point2 outSize(0, 0);
  Sqrat::Var<Sqrat::Object> desc(vm, 2);
  if (desc.value.GetType() != OT_NULL)
  {
    const StringKeys *csk = scene->getStringKeys();

    Sqrat::Table rootDesc(vm);
    rootDesc.SetValue(csk->children, desc.value);


    Component root;
    if (!Component::build_component(root, rootDesc, csk, rootDesc))
      return sq_throwerror(vm, "Failed to build components tree");

    ElementTree etree(scene, nullptr);
    etree.isInternalTemporaryTree = true;
    etree.canHandleInput = false;

    int rebuildResult = 0;
    etree.root = etree.rebuild(vm, etree.root, root, nullptr, root.scriptBuilder, 0, rebuildResult);
    G_ASSERT_RETURN(etree.root, sq_throwerror(vm, "Failed to build elements tree"));
    etree.root->recalcLayout();

    if (etree.root->children.size() != 1)
      return sq_throwerror(vm, String(32, "Unexpected root children count = %d", etree.root->children.size()));

    Element *elem = etree.root->children[0];
    G_ASSERT_RETURN(elem, sq_throwerror(vm, "Empty root"));

    outSize = elem->screenCoord.size;
  }

  Sqrat::Array res(vm, 2);
  res.SetValue(SQInteger(0), outSize.x);
  res.SetValue(SQInteger(1), outSize.y);
  sq_pushobject(vm, res.GetObject());

  return 1;
}


static SQInteger get_mouse_cursor_pos(HSQUIRRELVM vm)
{
  GuiScene *self = (GuiScene *)get_scene_from_sqvm(vm);

  Sqrat::Table result(vm);
  result.SetValue("x", self->activePointerPos().x);
  result.SetValue("y", self->activePointerPos().y);

  if (sq_gettop(vm) == 1)
  {
    sq_pushobject(vm, result.GetObject());
    return 1;
  }

  if (!Sqrat::check_signature<ElementRef *>(vm, 2))
    return SQ_ERROR;

  ElementRef *eref = ElementRef::get_from_stack(vm, 2);
  if (!eref || !eref->elem)
  {
    sq_pushobject(vm, result.GetObject());
    return 1;
  }

  int relXPx = int(floor(self->activePointerPos().x) - eref->elem->screenCoord.screenPos.x);
  int relYPx = int(floor(self->activePointerPos().y) - eref->elem->screenCoord.screenPos.y);

  result.SetValue("relXPx", relXPx);
  result.SetValue("relYPx", relYPx);

  result.SetValue("relXPct", 100.f * relXPx / max(eref->elem->screenCoord.size.x, 1.f));
  result.SetValue("relYPct", 100.f * relYPx / max(eref->elem->screenCoord.size.y, 1.f));
  sq_pushobject(vm, result.GetObject());

  return 1;
}

static SQInteger resolve_button_id(HSQUIRRELVM vm)
{
  GuiScene *self = (GuiScene *)get_scene_from_sqvm(vm);
  const char *str = NULL;
  G_VERIFY(SQ_SUCCEEDED(sq_getstring(vm, 2, &str)));
  HotkeyButton hb = self->getHotkeys().resolveButton(str);
  sq_pushinteger(vm, (hb.devId << 8) | hb.btnId);
  return 1;
}

static SQInteger set_xmb_focus(HSQUIRRELVM vm)
{
  GuiScene *self = (GuiScene *)get_scene_from_sqvm(vm);
  Sqrat::Var<Sqrat::Table> node(vm, 2);
  SQObjectType ot = node.value.GetType();
  if (ot == OT_NULL)
    self->trySetXmbFocus(nullptr);
  else if (ot == OT_TABLE)
  {
    ElementRef *eref = ElementRef::cast_from_sqrat_obj(node.value.RawGetSlot(self->getStringKeys()->elem));
    self->trySetXmbFocus(eref ? eref->elem : nullptr);
  }
  else
    return sq_throwerror(vm, "XMB focus must be null or table");
  return 0;
}


SQInteger GuiScene::get_comp_aabb_by_key(HSQUIRRELVM vm)
{
  HSQOBJECT keyObj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &keyObj)));
  if (keyObj._type == OT_NULL)
    return 0;

  Sqrat::Object key(keyObj, vm);

  GuiScene *self = (GuiScene *)get_scene_from_sqvm(vm);

  Element *elem = nullptr;
  for (auto &[id, sc] : self->screens)
  {
    elem = sc->etree.findElementByKey(key);
    if (elem)
      break;
  }
  if (!elem)
    return 0;

  EventDataRect resRect;
  calc_elem_rect_for_event_handler(resRect, elem);
  Sqrat::Table res(vm);
  res.SetValue("l", resRect.l);
  res.SetValue("b", resRect.b);
  res.SetValue("r", resRect.r);
  res.SetValue("t", resRect.t);
  sq_pushobject(vm, res);
  return 1;
}


void GuiScene::onPictureLoaded(const Picture *pic)
{
  dag::Vector<Element *, framemem_allocator> collected;

  for (auto &[id, sc] : screens)
    for (RenderEntry &re : sc->renderList.list)
    {
      Element *elem = re.elem;
      if (elem->rendObjType != rendobj_image_id)
        continue;

      if (elem->layout.size[0].mode != SizeSpec::CONTENT && elem->layout.size[1].mode != SizeSpec::CONTENT)
        continue;

      const RobjParamsImage *robjParams = static_cast<const RobjParamsImage *>(elem->robjParams);
      if (robjParams->image != pic || !robjParams->imageAffectsLayout)
        continue;

      collected.push_back(elem);
      if (collected.size() > 32)
      {
        // fallback to full recalc
        collected.clear();
        if (sc->etree.root)
          sc->etree.root->recalcLayout();
        logerr("[daRg] Too many images [%s] sized to content", pic->getName());
        break;
      }
    }

  for (Element *elem : collected)
    elem->recalcLayout();
}


IWndProcComponent::RetCode GuiScene::process(void * /*hwnd*/, unsigned msg, uintptr_t wParam, intptr_t /*lParam*/,
  intptr_t & /*result*/)
{
  if (msg == GPCM_Activate)
  {
    if (uint16_t(wParam) != GPCMP1_Inactivate)
    {
      mousePointer.onAppActivate();
      updateGlobalHover();
    }
    updateHotkeys(); // to compensate skipped joystick presses
  }
#if _TARGET_PC_WIN
  else if (msg == WM_WINDOWPOSCHANGED || msg == WM_DISPLAYCHANGE)
  {
    queryCurrentScreenResolution();
  }
  else if (msg == WM_SYSCOMMAND)
  {
    if ((wParam & 0xFFF0) == SC_CLOSE)
    {
      kbFocus.sysCloseRequested = true;
    }
  }
#endif
  return PROCEED_OTHER_COMPONENTS;
}


void GuiScene::queryCurrentScreenResolution()
{
#if _TARGET_PC_WIN
  HWND hwnd = (HWND)win32_get_main_wnd();
  G_ASSERT(hwnd);

  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
  G_ASSERT(monitor);
  MONITORINFOEX minfo;
  memset(&minfo, 0, sizeof(minfo));
  minfo.cbSize = sizeof(MONITORINFOEX);
  if (GetMonitorInfo(monitor, &minfo) && minfo.rcMonitor.right > minfo.rcMonitor.left &&
      minfo.rcMonitor.bottom > minfo.rcMonitor.top) // sanity check
  {
    deviceScreenSize.x = minfo.rcMonitor.right - minfo.rcMonitor.left;
    deviceScreenSize.y = minfo.rcMonitor.bottom - minfo.rcMonitor.top;
  }
  else
  {
    G_ASSERT(!"Failed to retrieve monitor info");
    // fallback
    HDC desktopDc = GetDC(nullptr);
    deviceScreenSize = {GetDeviceCaps(desktopDc, HORZRES), GetDeviceCaps(desktopDc, VERTRES)};
    ReleaseDC(nullptr, desktopDc);
  }
#else
  deviceScreenSize = {int(StdGuiRender::screen_width()), int(StdGuiRender::screen_height())};
#endif
}


bool GuiScene::useCircleAsActionButton() const
{
#if _TARGET_C1 | _TARGET_C2

#else
  return false;
#endif
}


JoystickAxisObservable *GuiScene::getJoystickAxis(int axis)
{
  if (DAGOR_UNLIKELY(joyAxisObservables.empty())) // Create on demand
  {
    joyAxisObservables.reserve(MAX_JOY_AXIS_OBSERVABLES);
    for (int i = 0; i < MAX_JOY_AXIS_OBSERVABLES; ++i)
      joyAxisObservables.emplace_back(this);
  }
  return (unsigned)axis < joyAxisObservables.size() ? &joyAxisObservables[axis] : nullptr;
}

void GuiScene::doUpdateJoystickAxesObservables()
{
  dag::RelocatableFixedVector<int, MAX_JOY_AXIS_OBSERVABLES, false> joyAxisPosData;
  const HumanInput::IGenJoystick *joy = getJoystick();
  if (unsigned n = joy ? min((unsigned)joy->getAxisCount(), MAX_JOY_AXIS_OBSERVABLES) : 0u)
  {
    joyAxisPosData.resize(n);
    WinAutoLock lock(global_cls_drv_update_cs); // Protect against concurrent modifies by `dainput::poll` thread
    for (int i = 0; i < n; ++i)
      joyAxisPosData[i] = joy->getAxisPosRaw(i);
  }
  else // Still update everything with 0 in assumption that joystick was removed (and we need to reset all axes)
    joyAxisPosData.resize(joyAxisObservables.size());
  int j = 0;
  for (auto &v : joyAxisPosData)
    joyAxisObservables[j++].update(float(v) / HumanInput::JOY_XINPUT_MAX_AXIS_VAL);
}


void GuiScene::onKeyboardLayoutChanged(const char *layout)
{
  ApiThreadCheck atc(this);
  keyboardLayout.setValue(Sqrat::Object(layout, sqvm));
}


void GuiScene::onKeyboardLocksChanged(unsigned locks)
{
  ApiThreadCheck atc(this);
  keyboardLocks.setValue(Sqrat::Object(locks, sqvm));
}


void GuiScene::blurWorld()
{
  TIME_PROFILE(blurWorld);
  Tab<BBox2> blurBoxes(framemem_ptr());
  for (auto &[id, sc] : screens)
  {
    if (sc->renderList.worldBlurElems.empty())
      continue;
    blurBoxes.reserve(blurBoxes.size() + sc->renderList.worldBlurElems.size());
    {
      TIME_PROFILE_DEV(calcTransformedBbox);
      for (const Element *elem : sc->renderList.worldBlurElems)
        blurBoxes.push_back(elem->calcTransformedBbox()); // may be optimized (probably using cached box from last frame)
    }
  }
  if (blurBoxes.empty())
    return;
  do_ui_blur(blurBoxes);
}


bool GuiScene::haveActiveCursorOnPanels() const
{
  ApiThreadCheck atc(this);

  for (const VrPointer &vrPtr : vrPointers)
    if (vrPtr.activePanel)
      return true;
  return false;
}


bool GuiScene::worldToPanelPixels(int panel_idx, const Point3 &world_target_pos, const Point3 &world_cam_pos, Point2 &out_panel_pos)
{
  using namespace panelmath3d;

  const auto &found = panels.find(panel_idx);
  if (found == panels.end())
    return false;

  const auto &panel = found->second;
  if (!panel->needRenderInWorld())
    return false;

  if (const auto hit = cast_at_hit_panel(*panel, world_cam_pos, world_target_pos); hit.has_value())
  {
    out_panel_pos = uv_to_panel_pixels(*panel, hit->uv);
    return true;
  }

  return false;
}

void GuiScene::ensurePanelBufferInitialized()
{
  if (!panelBufferInitialized)
  {
    const int numQuads = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_panels_quads", 1000);
    const int numExtraIndices = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_panels_extra", 0);
    G_VERIFY(getGuiContext()->createBuffer(panel_render_buffer_index, NULL, numQuads, numExtraIndices, "dargpanels.guibuf"));
    panelBufferInitialized = true;
  }
}

void GuiScene::renderPanelTo(int panel_idx, BaseTexture *dst)
{
  ApiThreadCheck atc(this);

  auto it = panels.find(panel_idx);
  if (it == panels.end())
    return;

  auto &panel = it->second;
  panel->spatialInfo.renderRedirection = true;
  ensurePanelBufferInitialized();
  int oldBuffer = getGuiContext()->currentBufferId;
  getGuiContext()->setBuffer(panel_render_buffer_index);
  panel->updateTexture(*this, dst);
  getGuiContext()->setBuffer(oldBuffer);

  refreshGuiContextState();
}

void GuiScene::updateSpatialElements(const SpatialSceneData &spatial_scene)
{
  ApiThreadCheck atc(this);
  TIME_PROFILE(GuiScene_updateSpatialElements);

  // Store camera transform and frustum for later use
  lastCameraTransform = spatial_scene.camera;
  lastCameraFrustum = spatial_scene.cameraFrustum;

  dag::Vector<Panel *, framemem_allocator> panelsToRender;
  panelsToRender.reserve(panels.size());

  for (auto &itPanel : panels)
  {
    Panel *panel = itPanel.second.get();
    if (panel->needRenderInWorld()) // Ensures there is anchor/geo to resolve spatially
    {
      darg_panel_renderer::panel_spatial_resolver(spatial_scene, panel->spatialInfo, panel->renderInfo.transform);

      if (panel->spatialInfo.visible)
        panelsToRender.emplace_back(panel);
    }
    else
      panel->renderInfo.isValid = false;
  }

  if (panelsToRender.empty())
    return;

  TIME_PROFILE(GuiScene_drawSpatialElements);
  ensurePanelBufferInitialized();
  int oldBuffer = getGuiContext()->currentBufferId;
  getGuiContext()->setBuffer(panel_render_buffer_index);

  //--- update render state -------------

  for (Panel *panel : panelsToRender)
  {
    panel->updateTexture(*this, nullptr);
    panel->renderInfo.isValid = true;
    panel->renderInfo.texture = panel->canvas->getId();
  }

  getGuiContext()->setBuffer(oldBuffer);

  refreshGuiContextState();

  //--- process input: panel position has most-likely changed ---------------
  updateSpatialInteractionState(spatial_scene);
}


void GuiScene::refreshVrCursorProjections()
{
  for (int hand : {0, 1, 2})
    onVrInputEvent(INP_EV_POINTER_MOVE, hand, HumanInput::DBUTTON_LEFT);
}


bool GuiScene::hasAnyPanels() const
{
  ApiThreadCheck atc(this);

  return !panels.empty();
}


void GuiScene::updateSpatialInteractionState(const SpatialSceneData &spatial_scene)
{
  TIME_PROFILE(GuiScene_updateSpatialInteractions);
  using namespace panelmath3d;

  using Aim = SpatialSceneData::AimOrigin;
  eastl::array<float, NUM_VR_POINTERS> minHitDistance;
  minHitDistance.fill(max_aim_distance + 1.f);

  Point2 vrSurfaceHitUvs[NUM_VR_POINTERS];
  if (spatial_scene.vrSurfaceIntersector)
    for (int hand : {Aim::LeftHand, Aim::RightHand})
    {
      const SpatialSceneData::Aim &aim = spatial_scene.aims[hand];
      Point3 hit{0.f, 0.f, 0.f};
      if (spatial_scene.vrSurfaceIntersector(aim.pos, aim.dir, vrSurfaceHitUvs[hand], hit))
        minHitDistance[hand] = hit.length();
    }

  Point2 panelHitUvs[NUM_VR_POINTERS];
  int closestHitPanelIdxs[NUM_VR_POINTERS] = {-1, -1, -1};
  for (const auto &itPanel : panels)
  {
    const Panel *panel = itPanel.second.get();
    if (!panel->renderInfo.isValid)
      continue;

    if (!panel->spatialInfo.canBeTouched && !panel->spatialInfo.canBePointedAt)
      continue;

    for (int src : {Aim::LeftHand, Aim::RightHand, Aim::Generic})
    {
      PossiblePanelIntersection hit;
      const PanelSpatialInfo &psi = panel->spatialInfo;
      float threshold = psi.canBeTouched ? min(max_touch_distance + 1.f, minHitDistance[src]) : minHitDistance[src];
      if (psi.canBeTouched && (src == Aim::LeftHand || src == Aim::RightHand))
      {
        bool panelCanBeTouchedWithThisHand = (psi.anchor == PanelAnchor::LeftHand && src != Aim::LeftHand) ||
                                             (psi.anchor == PanelAnchor::RightHand && src != Aim::RightHand) ||
                                             psi.anchor == PanelAnchor::VRSpace || psi.anchor == PanelAnchor::Scene;
        if (panelCanBeTouchedWithThisHand)
          hit = project_at_hit_panel(*panel, spatial_scene.indexFingertips[src].getcol(3), max_touch_distance);
      }
      else if (psi.canBePointedAt)
      {
        const auto &aim = spatial_scene.aims[src];
        hit = cast_at_hit_panel(*panel, aim.pos, aim.pos + normalize(aim.dir) * max_aim_distance);
      }

      if (hit.has_value() && hit->distance < threshold)
      {
        minHitDistance[src] = hit->distance;
        closestHitPanelIdxs[src] = itPanel.first;
        panelHitUvs[src] = hit->uv;
      }
    }
  }

  spatialInteractionState = {};
  for (int src : {Aim::LeftHand, Aim::RightHand, Aim::Generic})
  {
    auto panelIt = panels.find(closestHitPanelIdxs[src]);
    G_ASSERT(closestHitPanelIdxs[src] < 0 || panelIt != panels.end());
    bool useTouchThresh = closestHitPanelIdxs[src] >= 0 && panelIt->second->spatialInfo.canBeTouched;
    if (minHitDistance[src] < (useTouchThresh ? max_touch_distance : max_aim_distance))
    {
      auto &sis = spatialInteractionState;
      sis.hitDistances[src] = minHitDistance[src];
      sis.closestHitPanelIdxs[src] = closestHitPanelIdxs[src];
      if (sis.wasVrSurfaceHit(src))
        sis.hitPos[src] = vrSurfaceHitUvs[src];
      else if (sis.wasPanelHit(src))
      {
        int closestPanelIdx = sis.closestHitPanelIdxs[src];
        auto closestPanelIt = panels.find(closestPanelIdx);
        G_ASSERT(closestPanelIt != panels.end());
        sis.hitPos[src] = uv_to_panel_pixels(*closestPanelIt->second, panelHitUvs[src]);
      }
    }
  }
}


int GuiScene::onVrInputEvent(InputEvent event, int hand, int button_id, int prev_result)
{
  //
  // TODO: handle processing result flags properly
  //

  using namespace panelmath3d;

  G_UNUSED(prev_result);

  ApiThreadCheck atc(this);

  G_ASSERT_RETURN(hand >= 0 && hand < NUM_VR_POINTERS, 0);

  int result = prev_result;
  if (status.isShowingFullErrorDetails || !isInputActive())
    return result;

  VrPointer &vrPtr = vrPointers[hand];
  int closestPanelIdx = spatialInteractionState.closestHitPanelIdxs[hand];
  auto closestPanelIt = panels.find(closestPanelIdx);
  Panel *closestPanel = closestPanelIt != panels.end() ? closestPanelIt->second.get() : nullptr;
  if (vrPtr.activePanel != closestPanel)
  {
    if (vrPtr.activePanel)
    {
      if (const auto touch = vrPtr.activePanel->touchPointers.get(hand))
        onPanelTouchEvent(vrPtr.activePanel, hand, INP_EV_RELEASE, touch->pos.x, touch->pos.y);
      vrPtr.activePanel->setCursorState(hand, false);
      int resFlags = vrPtr.activePanel->screen->etree.deactivateInput(DEVID_VR, hand);
      if (resFlags & R_REBUILD_RENDER_AND_INPUT_LISTS)
        vrPtr.activePanel->rebuildStacks();
    }
    vrPtr.activePanel = closestPanel;
    if (vrPtr.activePanel)
      vrPtr.activePanel->setCursorState(hand, true);
  }

  auto &sis = spatialInteractionState;
  const Point2 &cursorLocation = sis.hitPos[hand];

  if (closestPanel)
  {
    Panel *panel = closestPanel;
    // The closest hit panel can only be touched or pointed at (see how hit detection is performed).
    if (panel->spatialInfo.canBeTouched)
    { // If we get here, we have already detected a touch in this frame.
      const auto existingTouch = panel->touchPointers.get(hand);
      float curDist = sis.hitDistances[hand].value();
      bool canRegisterTouch = existingTouch || (curDist > 0.f && curDist < new_touch_threshold); // Avoid double activation
      bool touchNow =
        canRegisterTouch && panel->screen->inputStack.hitTest(cursorLocation, Behavior::F_HANDLE_TOUCH, Element::F_STOP_POINTING);
      sis.isHandTouchingPanel[hand] = touchNow;
      if (existingTouch && touchNow)
        result = onPanelTouchEvent(panel, hand, INP_EV_POINTER_MOVE, cursorLocation.x, cursorLocation.y);
      else if (touchNow)
        result = onPanelTouchEvent(panel, hand, INP_EV_PRESS, cursorLocation.x, cursorLocation.y);
      // else if (existingTouch) - Release is handled above, when we're no longer touching old activePanel, once.
    }
    else if (panel->spatialInfo.canBePointedAt)
      result = onPanelMouseEvent(panel, hand, event, DEVID_VR, button_id, cursorLocation.x, cursorLocation.y);
  }

  callScriptHandlers(); // call handlers early for faster response

  return result;
}


void GuiScene::setVrStickScroll(int hand, const Point2 &scroll)
{
  ApiThreadCheck atc(this);
  G_ASSERT_RETURN(hand >= 0 && hand < NUM_VR_POINTERS, );
  vrPointers[hand].stickScroll = scroll;
}


Point2 GuiScene::getVrStickState(int hand) const
{
  G_ASSERT_RETURN(hand >= 0 && hand < NUM_VR_POINTERS, Point2(0, 0));
  return vrPointers[hand].stickScroll;
}


static SQInteger set_config_props(HSQUIRRELVM vm)
{
  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  Sqrat::ClassType<SceneConfig>::PushNativeInstance(vm, &scene->config);
  sq_pushnull(vm);
  while (SQ_SUCCEEDED(sq_next(vm, 2))) // iterate provided table
  {
    if (SQ_FAILED(sq_set(vm, 3))) // set to config instance
      return SQ_ERROR;
  }
  return 0;
}


bool GuiScene::getForcedCursorMode(bool &out_value) const
{
  if (forcedCursor.IsNull())
    return false;
  const HSQOBJECT &ho = forcedCursor.GetObject();
  if (sq_type(ho) == OT_INSTANCE)
    out_value = true;
  else
    out_value = sq_objtobool(&ho);
  return true;
}


bool GuiScene::isCursorForcefullyEnabled() const
{
  bool val = false;
  return getForcedCursorMode(val) && val;
}

bool GuiScene::isCursorForcefullyDisabled() const
{
  bool val = false;
  return getForcedCursorMode(val) && !val;
}


SQInteger GuiScene::force_cursor_active(HSQUIRRELVM vm)
{
  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  Sqrat::Object newVal = Sqrat::Var<Sqrat::Object>(vm, 2).value;

  if (newVal.GetType() == OT_INSTANCE && !Sqrat::ClassType<Cursor>::IsClassInstance(newVal))
    return sq_throwerror(vm, "Forced cursor must be: null (disable override), false, true, or Cursor instance");

  if (are_sq_obj_equal(vm, scene->forcedCursor, newVal))
    return 0;

  scene->forcedCursor = newVal;
  scene->updateGlobalActiveCursor();
  scene->applyInputActivityChange();

  return 0;
}


Element *GuiScene::traceInputHit(ElementTree *etree, InputDevice device, Point2 pos)
{
  return Element::trace_input_stack(this, etree->screen->inputStack, device, pos);
}


Screen *GuiScene::getGuiScreen(int id) const
{
  auto it = screens.find(id);
  if (it == screens.end())
    return nullptr;
  return it->second.get();
}

int GuiScene::getGuiScreenId(Screen *screen) const
{
  auto it = eastl::find_if(screens.begin(), screens.end(), [screen](auto &p) { return p.second.get() == screen; });
  if (it == screens.end())
    return -1;
  return it->first;
}

Screen *GuiScene::createGuiScreen(int id)
{
  if (auto sc = getGuiScreen(id))
  {
    logerr("Gui screen #%d already created", id);
    return sc;
  }

  auto sc = new Screen(this);
  screens[id].reset(sc);
  if (!focusedScreen)
    focusedScreen = sc;
  return sc;
}

bool GuiScene::destroyGuiScreen(int id)
{
  auto it = screens.find(id);
  if (it == screens.end())
    return false;
  if (focusedScreen == it->second.get())
    focusedScreen = nullptr;
  screens.erase(it);
  return true;
}

bool GuiScene::setFocusedScreen(Screen *screen)
{
  // also see applyInputActivityChange()
  if (focusedScreen == screen)
    return false;
  if (focusedScreen)
    focusedScreen->etree.deactivateAllInput();
  focusedScreen = screen;

  pressedClickButtons.clear();
  kbFocus.setFocus(nullptr);
  keptXmbFocus.clear();

  return true;
}

bool GuiScene::destroyGuiScreen(Screen *screen) { return destroyGuiScreen(getGuiScreenId(screen)); }

bool GuiScene::setFocusedScreenById(int id) { return setFocusedScreen(getGuiScreen(id)); }

SQInteger GuiScene::sqGetFocusedScreen(HSQUIRRELVM vm)
{
  // returns screen id or null if no focused screen
  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  Screen *s = scene->focusedScreen;
  auto it = eastl::find_if(scene->screens.begin(), scene->screens.end(), [s](const auto &p) { return p.second.get() == s; });

  if (it != scene->screens.end())
  {
    sq_pushinteger(vm, it->first);
    return 1;
  }
  else
    return 0;
}

void GuiScene::bindScriptClasses()
{
  G_ASSERT(sqvm);

  moduleMgr->registerMathLib();
  moduleMgr->registerStringLib();
  moduleMgr->registerIoStreamLib();

  sqfrp::bind_frp_classes(moduleMgr.get());

  Sqrat::Table dargModuleExports(sqvm);
  binding::bind_script_classes(moduleMgr.get(), dargModuleExports);

  ///@module daRg
  Sqrat::ConstTable(sqvm)
    .Const("KBD_BIT_CAPS_LOCK", 1)
    .Const("KBD_BIT_NUM_LOCK", 2)
    .Const("KBD_BIT_SCROLL_LOCK", 4)
    .Const("PANEL_ANCHOR_NONE", int(PanelAnchor::None))
    .Const("PANEL_ANCHOR_SCENE", int(PanelAnchor::Scene))
    .Const("PANEL_ANCHOR_VRSPACE", int(PanelAnchor::VRSpace))
    .Const("PANEL_ANCHOR_HEAD", int(PanelAnchor::Head))
    .Const("PANEL_ANCHOR_LEFTHAND", int(PanelAnchor::LeftHand))
    .Const("PANEL_ANCHOR_RIGHTHAND", int(PanelAnchor::RightHand))
    .Const("PANEL_ANCHOR_ENTITY", int(PanelAnchor::Entity))
    .Const("PANEL_GEOMETRY_NONE", int(PanelGeometry::None))
    .Const("PANEL_GEOMETRY_RECTANGLE", int(PanelGeometry::Rectangle))
    .Const("PANEL_RC_NONE", int(PanelRotationConstraint::None))
    .Const("PANEL_RC_FACE_LEFT_HAND", int(PanelRotationConstraint::FaceLeftHand))
    .Const("PANEL_RC_FACE_RIGHT_HAND", int(PanelRotationConstraint::FaceRightHand))
    .Const("PANEL_RC_FACE_HEAD", int(PanelRotationConstraint::FaceHead))
    .Const("PANEL_RC_FACE_HEAD_LOCK_Y", int(PanelRotationConstraint::FaceHeadLockY))
    .Const("PANEL_RC_FACE_ENTITY", int(PanelRotationConstraint::FaceEntity))
    .Const("PANEL_RC_FACE_HEAD_LOCK_PLAYSPACE_Y", int(PanelRotationConstraint::FaceHeadLockPlayspaceY))
    .Const("PANEL_RENDER_CAST_SHADOW", int(darg_panel_renderer::RenderFeatures::CastShadow))
    .Const("PANEL_RENDER_OPAQUE", int(darg_panel_renderer::RenderFeatures::Opaque))
    .Const("PANEL_RENDER_ALWAYS_ON_TOP", int(darg_panel_renderer::RenderFeatures::AlwaysOnTop)) //-V1071
    .Const("MAIN_SCREEN_ID", int(MAIN_SCREEN_ID));

  Sqrat::Class<GuiScene, Sqrat::NoConstructor<GuiScene>> guiSceneClass(sqvm, "GuiScene");
  ///@class daRg/GuiScene
  guiSceneClass //
    .SquirrelFunc("setUpdateHandler", set_update_handler, 2, "xc|o")
    .SquirrelFunc("setShutdownHandler", set_shutdown_handler, 2, "xc|o")
    .SquirrelFunc("setHotkeysNavHandler", set_hotkeys_nav_handler, 2, "xc|o")
    .SquirrelFunc("addPanel", add_panel, 3, "xic|t")
    .SquirrelFunc("removePanel", remove_panel, 2, "xi")
    .SquirrelFunc("mark_panel_dirty", mark_panel_dirty, 2, "xi")
    .SquirrelFunc(
      "setTimeout", [](HSQUIRRELVM vm) { return GuiScene::set_timer(vm, false, false); }, -3, "xnc")
    .SquirrelFunc(
      "resetTimeout", [](HSQUIRRELVM vm) { return GuiScene::set_timer(vm, false, true); }, -3, "xnc")
    .SquirrelFunc(
      "setInterval", [](HSQUIRRELVM vm) { return GuiScene::set_timer(vm, true, false); }, -3, "xnc")
    .SquirrelFunc("setXmbFocus", set_xmb_focus, -2, "xo|t")
    .SquirrelFunc("getCompAABBbyKey", &GuiScene::get_comp_aabb_by_key, 2)
    .SquirrelFunc("setConfigProps", set_config_props, 2, ".t")
    .Func("setFocusedScreen", &GuiScene::setFocusedScreenById)
    .SquirrelFunc("getFocusedScreen", &GuiScene::sqGetFocusedScreen, 1, "x")
    .Func("haveActiveCursorOnPanels", &GuiScene::haveActiveCursorOnPanels)
    .Func("clearTimer", &GuiScene::clearTimer)
    .Prop("config", &GuiScene::getConfig)
    .Func("getAllObservables", &GuiScene::getAllObservables)
    .Prop("cursorPresent", &GuiScene::getCursorPresentObservable)
    .Prop("cursorOverStickScroll", &GuiScene::getCursorOverScrollObservable)
    .Prop("cursorOverClickable", &GuiScene::getCursorOverClickableObservable)
    .Prop("hoveredClickableInfo", &GuiScene::getHoveredClickableInfoObservable)
    .Prop("keyboardLayout", &GuiScene::getKeyboardLayoutObservable)
    .Prop("keyboardLocks", &GuiScene::getKeyboardLocksObservable)
    .Prop("updateCounter", &GuiScene::getUpdateCounterObservable)
    .Prop("circleButtonAsAction", &GuiScene::useCircleAsActionButton)
    .Prop("xmbMode", &GuiScene::getIsXmbModeOn)
    .Func("getJoystickAxis", &GuiScene::getJoystickAxis)
    .Func("enableInput", &GuiScene::scriptSetSceneInputActive)
    .SquirrelFunc("forceCursorActive", &GuiScene::force_cursor_active, 2, "x b|o|x")
    /**/;

#define V(x) .Var(#x, &SceneConfig::x)
  ///@class daRg/SceneConfig
  Sqrat::Class<SceneConfig, Sqrat::NoConstructor<SceneConfig>> sqSceneConfig(sqvm, "SceneConfig");
  sqSceneConfig
    ///@var defaultFont
    .SquirrelProp("defaultFont", SceneConfig::getDefaultFontId, SceneConfig::setDefaultFontId)
    ///@var defaultFontSize
    .SquirrelProp("defaultFontSize", SceneConfig::getDefaultFontSize, SceneConfig::setDefaultFontSize)
    ///@var kbCursorControl
    V(kbCursorControl)
    ///@var gamepadCursorControl
    V(gamepadCursorControl)
    ///@var gamepadStickAsDirpad
    V(gamepadStickAsDirpad)
    ///@var ignorePointerVisibilityForHovers
    V(ignorePointerVisibilityForHovers)
    ///@var gamepadCursorSpeed
    V(gamepadCursorSpeed)
    ///@var gamepadCursorHoverMaxTime
    V(gamepadCursorHoverMaxTime)
    ///@var gamepadCursorAxisV
    V(gamepadCursorAxisV)
    ///@var gamepadCursorAxisH
    V(gamepadCursorAxisH)
    ///@var gamepadCursorHoverMinMul
    V(gamepadCursorHoverMinMul)
    ///@var gamepadCursorHoverMaxMul
    V(gamepadCursorHoverMaxMul)
    ///@var gamepadCursorDeadZone
    V(gamepadCursorDeadZone)
    ///@var gamepadCursorNonLin
    V(gamepadCursorNonLin)
    ///@var reportNestedWatchedUpdate
    V(reportNestedWatchedUpdate)
    ///@var joystickScrollAxisH
    V(joystickScrollAxisH)
    ///@var joystickScrollAxisV
    V(joystickScrollAxisV)
    ///@var clickRumbleEnabled
    V(clickRumbleEnabled)
    ///@var clickRumbleLoFreq
    V(clickRumbleLoFreq)
    ///@var clickRumbleHiFreq
    V(clickRumbleHiFreq)
    ///@var clickRumbleDuration
    V(clickRumbleDuration)
    ///@var dirPadRepeatDelay
    V(dirPadRepeatDelay)
    ///@var dirPadRepeatTime
    V(dirPadRepeatTime)
    ///@var gamepadCursorHoverMinMul
    V(gamepadCursorHoverMinMul)
    ///@var useDefaultCursor
    V(useDefaultCursor)
    ///@var defaultCursor
    V(defaultCursor)
    ///@var actionClickByBehavior
    V(actionClickByBehavior)
    ///@var allScreensAcceptInput
    V(allScreensAcceptInput)
    ///@var moveClickThreshold
    V(moveClickThreshold)
    .Prop("defSceneBgColor", &SceneConfig::getDefSceneBgColor, &SceneConfig::setDefSceneBgColor)
    .Prop("defTextColor", &SceneConfig::getDefTextColor, &SceneConfig::setDefTextColor)
    .SquirrelFunc("setClickButtons", &SceneConfig::setClickButtons, 2, "xa")
    .SquirrelFunc("getClickButtons", &SceneConfig::getClickButtons, 1, "x")
    /**/;
#undef V

  ///@module daRg
  dargModuleExports //
    .SquirrelFunc("anim_start", GuiScene::call_anim_method<&ElementTree::startAnimation>, 2)
    .SquirrelFunc("anim_request_stop", GuiScene::call_anim_method<&ElementTree::requestAnimStop>, 2)
    .SquirrelFunc("anim_skip", GuiScene::call_anim_method<&ElementTree::skipAnim>, 2)
    .SquirrelFunc("anim_skip_delay", GuiScene::call_anim_method<&ElementTree::skipAnimDelay>, 2)
    .SquirrelFunc("anim_pause", GuiScene::anim_pause, -2, "..b")
    .SquirrelFunc("set_kb_focus", set_kb_focus, 2)
    .SquirrelFunc("capture_kb_focus", capture_kb_focus, 2)
    .SquirrelFunc("calc_comp_size", calc_comp_size, 2, ".o|t|c|x|y")
    .SquirrelFunc("move_mouse_cursor", GuiScene::move_mouse_cursor, -2, "..b")
    .SquirrelFunc("get_mouse_cursor_pos", get_mouse_cursor_pos, -1, ".x")
    .SquirrelFunc("resolve_button_id", resolve_button_id, 2, ".s")
    .SetValue("gui_scene", this)
    ///@ftype GuiScene
    ///@fvalue instance of GuiScene
    /**/;

  moduleMgr->addNativeModule("daRg", dargModuleExports);
}


} // namespace darg
