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

#include <sqModules/sqModules.h>

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
#include "elementRef.h"
#include "canvasDraw.h"

#include "textLayout.h"
#include "dargDebugUtils.h"
#include "guiGlobals.h"
#include <daRg/robjWorldBlur.h>

#include <sqstdaux.h>

#include <util/dag_convar.h>

#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <math/dag_plane3.h>
#include <3d/dag_textureIDHolder.h>
#include <EASTL/optional.h>

#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <ioSys/dag_dataBlock.h>

#include <quirrel/quirrel_json/jsoncpp.h>

#include "dasScripts.h"


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

CONSOLE_BOOL_VAL("darg", debug_xmb, false);
CONSOLE_BOOL_VAL("darg", debug_dirpad_nav, false);
CONSOLE_BOOL_VAL("darg", debug_input_stack, false);

static constexpr unsigned MAX_JOY_AXIS_OBSERVABLES = 15;

using namespace sqfrp;


static const int rebuild_stacks_flags =
  ElementTree::RESULT_ELEMS_ADDED_OR_REMOVED | ElementTree::RESULT_INVALIDATE_RENDER_LIST | ElementTree::RESULT_INVALIDATE_INPUT_STACK;


struct DebugRenderBox
{
  BBox2 box;
  E3DCOLOR fillColor, borderColor;
  float lifeTime, timeLeft;
};


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


IGuiScene *create_gui_scene(StdGuiRender::GuiContext *ctx)
{
  {
    using namespace HumanInput;
    G_STATIC_ASSERT(int(DIR_UP) == int(JOY_XINPUT_REAL_BTN_D_UP));
    G_STATIC_ASSERT(int(DIR_DOWN) == int(JOY_XINPUT_REAL_BTN_D_DOWN));
    G_STATIC_ASSERT(int(DIR_LEFT) == int(JOY_XINPUT_REAL_BTN_D_LEFT));
    G_STATIC_ASSERT(int(DIR_RIGHT) == int(JOY_XINPUT_REAL_BTN_D_RIGHT));
  }

  return new GuiScene(ctx);
}


GuiScene::GuiScene(StdGuiRender::GuiContext *gui_ctx) : etree(this, nullptr), gamepadCursor(this), config(this)
{
  yuvRenderer.init();

  if (gui_ctx)
    guiContext = gui_ctx;
  else
  {
    ownGuiContext = true;
    const int numQuads = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_gui_quads", 64 << 10);
    const int numExtraIndices = ::dgs_get_game_params()->getBlockByNameEx("guiLimits")->getInt("drggs_gui_extra", 1024);
    guiContext = new StdGuiRender::GuiContext();
    G_VERIFY(guiContext->createBuffer(0, NULL, numQuads, numExtraIndices, "daRg.guibuf"));
  }

  dasScriptsData.reset(new DasScriptsData());

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
  destroyNativeWatches();
  shutdownScript();

  if (ownGuiContext)
    delete guiContext;
}


void GuiScene::initDasEnvironment(TGuiInitDas init_callback) { dasScriptsData->initDasEnvironment(init_callback); }


void GuiScene::shutdownDasEnvironment() { dasScriptsData->shutdownDasEnvironment(); }


static void create_scene_observable(const eastl::unique_ptr<sqfrp::ObservablesGraph> &frpGraph,
  eastl::unique_ptr<sqfrp::ScriptValueObservable> &ptr)
{
  ptr.reset(frpGraph->allocScriptValueObservable());
  ptr->generation = -1;
}

void GuiScene::createNativeWatches()
{
  create_scene_observable(frpGraph, cursorPresent);
  create_scene_observable(frpGraph, cursorOverStickScroll);
  create_scene_observable(frpGraph, cursorOverClickable);
  create_scene_observable(frpGraph, hoveredClickableInfo);
  create_scene_observable(frpGraph, keyboardLayout);
  create_scene_observable(frpGraph, keyboardLocks);
  create_scene_observable(frpGraph, isXmbModeOn);
  create_scene_observable(frpGraph, updateCounter);

  String errMsg;

  if (!cursorPresent->setValue(Sqrat::Object(false, sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!cursorOverStickScroll->setValue(Sqrat::Object(SQInteger(0), sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!cursorOverClickable->setValue(Sqrat::Object(false, sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!hoveredClickableInfo->setValue(Sqrat::Object(sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!keyboardLayout->setValue(Sqrat::Object("", sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!keyboardLocks->setValue(Sqrat::Object(SQInteger(0), sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!isXmbModeOn->setValue(Sqrat::Object(false, sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
  if (!updateCounter->setValue(Sqrat::Object(actsCount, sqvm), errMsg))
    darg_immediate_error(sqvm, errMsg);
}

void GuiScene::destroyNativeWatches()
{
  cursorPresent.reset();
  cursorOverStickScroll.reset();
  cursorOverClickable.reset();
  hoveredClickableInfo.reset();
  keyboardLayout.reset();
  keyboardLocks.reset();
  isXmbModeOn.reset();
  updateCounter.reset();

  joyAxisObservables.clear();
}

void GuiScene::initScript()
{
  G_ASSERT(!sqvm);
  sqvm = sq_open(1024);
  G_ASSERT(sqvm);

  moduleMgr.reset(new SqModules(sqvm));
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

  G_ASSERT(frpGraph->allObservables.empty());
  frpGraph.reset();

  G_ASSERT(allCursors.empty());
}


void GuiScene::clearCursors(bool exiting)
{
  G_UNUSED(exiting);

  config.defaultCursor.Release();

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
  activeCursor = nullptr;

  for (auto &itPanelData : panels)
    for (PanelPointer &ptr : itPanelData.second->panel->pointers)
      ptr.cursor = nullptr;
}


void GuiScene::clear(bool exiting)
{
  G_ASSERT(!isClearing);

  TIME_PROFILE(GuiScene_clear);

  isClearing = true;

  if (guiSceneCb)
  {
    if (!inputStack.stack.empty())
      guiSceneCb->onToggleInteractive(0);

    if (guiSceneCb->useInputConsumersCallback())
      guiSceneCb->onInputConsumersChange(dag::ConstSpan<HotkeyButton>(), dag::ConstSpan<String>(), false);
  }

  // Need to call such script handlers as onDetach()
  callScriptHandlers(true);

  G_ASSERT(!lockTimersList);
  invalidatedElements.clear();
  etree.clear();
  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  eventHandlersStack.clear();
  pressedClickButtons.clear();

  for (VrPointer &vrPtr : vrPointers)
    vrPtr = {};
  spatialInteractionState = {};
  panels.clear();

  clear_all_ptr_items_and_shrink(timers);
  clear_all_ptr_items_and_shrink(scriptHandlersQueue);

  if (!sceneScriptHandlers.onShutdown.IsNull())
    sceneScriptHandlers.onShutdown();

  clearCursors(exiting);
  pointerPos.onShutdown();

  touchPointers.reset();

  clear_all_ptr_items_and_shrink(timers);

  frpGraph->shutdown(exiting);

  status.reset();

  PicAsyncLoad::on_scene_shutdown(this);
  PictureManager::discard_unused_picture();
  needToDiscardPictures = false;

  sq_collectgarbage(sqvm);
  isClearing = false;

  darg_panel_renderer::clean_up();

  G_ASSERT(keptXmbFocus.empty());
}


Sqrat::Array GuiScene::getAllObservables()
{
  Sqrat::Array res(sqvm, frpGraph->allObservables.size());
  SQInteger idx = 0;
  for (auto obs : frpGraph->allObservables)
  {
    G_ASSERT_BREAK(idx < frpGraph->allObservables.size());
    Sqrat::Table item(sqvm);
    obs->fillInfo(item);
    res.SetValue(idx, item);
    ++idx;
  }
  return res;
}


void GuiScene::setSceneErrorRenderMode(SceneErrorRenderMode mode)
{
  ApiThreadCheck atc(this);
  status.setSceneErrorRenderMode(mode);
}


void GuiScene::moveMouseToTarget(float dt)
{
  if (pointerPos.moveMouseToTarget(dt))
    updateHover();
}


void GuiScene::update(float dt)
{
  ApiThreadCheck atc(this);

  if (status.sceneIsBroken)
    return;

  AutoProfileScope profile(profiler, M_UPDATE_TOTAL);
  SqStackChecker stackCheck(sqvm);
  String frpErrMsg;

  // need to do this before any element may change
  applyPendingHoverUpdate();

  if (!frpGraph->updateDeferred(frpErrMsg))
    logerr("FRP update deferred: %s", frpErrMsg.c_str());

  actsCount++;
  updateCounter->setValue(Sqrat::Object(actsCount, sqvm), frpErrMsg);

  processPostedEvents();

  joystickHandler.processPendingBtnStack(this);

  if (lastDirPadNavDir >= 0)
  {
    if (dirPadNavRepeatTimeout <= 0.0f)
    {
      dirPadNavRepeatTimeout = config.dirPadRepeatTime;
      dirPadNavigate(Direction(lastDirPadNavDir));
    }
    else
      dirPadNavRepeatTimeout -= dt;
  }

  bool prevNeedToMoveOnNextFrame = pointerPos.needToMoveOnNextFrame; //==
  if (isInputActive())
    pointerPos.update(dt);
  if (prevNeedToMoveOnNextFrame) //== This is workaround for XMB hover update on XBox - temporary, to be rewritten
    updateHover();

  cursorPresent->setValue(Sqrat::Object(!pointerPos.mouseWasRelativeOnLastUpdate && activeCursor != nullptr, sqvm), frpErrMsg);

  if (isInputActive())
  {
    if (etree.root && activeCursor && config.gamepadCursorControl)
      gamepadCursor.update(dt);

    for (int hand = 0; hand < NUM_VR_POINTERS; ++hand)
    {
      const VrPointer &vrPtr = vrPointers[hand];
      if (vrPtr.stickScroll.lengthSq() > 1e-6f)
      {
        float scrollStep = 1000.0f * dt * StdGuiRender::screen_height() / 1080.0f;
        Point2 scrollDelta = vrPtr.stickScroll * scrollStep;
        if (vrPtr.activePanel)
        {
          Panel *panel = vrPtr.activePanel->panel.get();
          if (panel && panel->pointers[hand].isPresent)
          {
            if (panel->etree.doJoystickScroll(panel->inputStack, panel->pointers[hand].pos, scrollDelta))
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

  updateJoystickAxesObservables();
  moveMouseToTarget(dt);
  updateTimers(dt);

  stackCheck.check();

  callScriptHandlers();

  rebuildInvalidatedParts();

  stackCheck.check();

  if (!status.sceneIsBroken && !sceneScriptHandlers.onUpdate.IsNull())
    sceneScriptHandlers.onUpdate.Execute(dt);

  stackCheck.check();

  int updRes = etree.update(dt);
  if (updRes & rebuild_stacks_flags)
  {
    rebuildElemStacks(updRes & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED);
    updateHover();
  }
  if (updRes & ElementTree::RESULT_NEED_XMB_REBUILD)
    rebuildXmb();

  if (activeCursor)
    activeCursor->update(dt);

  for (auto &itPanelData : panels)
    itPanelData.second->panel->update(dt);

  if (profiler.get())
    profiler->afterUpdate();

  updateDebugRenderBoxes(dt);
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

  if (HumanInput::IGenPointing *mouse = getMouse())
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
  s.printf(64, "Render list size = %d", (int)renderList.list.size());
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

  G_ASSERT(etree.rebuildFlagsAccum == 0);
  yuvRenderer.resetOnFrameStart();

  int curTime = get_time_msec();
  float dt = (lastRenderTimestamp != 0) ? (curTime - lastRenderTimestamp) * 1e-3f : 0.0f;
  lastRenderTimestamp = curTime;

  if (status.sceneIsBroken)
    return;

  scrollToXmbFocus(dt);
  updateActiveCursor();

  int brRes = 0, brResCursor = 0;
  {
    AutoProfileScope profile(profiler, M_ETREE_BEFORE_RENDER);
    brRes = etree.beforeRender(dt);
    if (activeCursor)
      brResCursor = activeCursor->etree.beforeRender(dt);
  }

  if ((brRes | brResCursor) & R_REBUILD_RENDER_AND_INPUT_LISTS)
  {
    AutoProfileScope profile(profiler, M_RENDER_LIST_REBUILD);
    if (brRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
      rebuildElemStacks(false);
    if ((brResCursor & R_REBUILD_RENDER_AND_INPUT_LISTS) && activeCursor)
      activeCursor->rebuildStacks();
  }

  if (!panels.empty())
  {
    for (auto &itPanelData : panels)
    {
      Panel *panel = itPanelData.second->panel.get();
      G_ASSERT(panel->etree.rebuildFlagsAccum == 0);

      panel->updateActiveCursors();

      G_ASSERT(panel->etree.rebuildFlagsAccum == 0);

      int panelBrRes = 0;
      {
        AutoProfileScope profile(profiler, M_ETREE_BEFORE_RENDER);
        panelBrRes = panel->etree.beforeRender(dt);
      }
      if (panelBrRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
      {
        AutoProfileScope profile(profiler, M_RENDER_LIST_REBUILD);
        panel->rebuildStacks();
      }
      G_ASSERT(panel->etree.rebuildFlagsAccum == 0);

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

  if (dt > 0)
  {
    bool hasFinishedAnims = false;
    etree.updateAnimations(dt, &hasFinishedAnims);
    etree.updateTransitions(dt);
    etree.updateKineticScroll(dt);
    if (activeCursor)
    {
      activeCursor->etree.updateAnimations(dt, nullptr);
      activeCursor->etree.updateTransitions(dt);
    }
    if (hasFinishedAnims)
      updateHover();

    for (auto &itPanelData : panels)
    {
      Panel *panel = itPanelData.second->panel.get();
      panel->etree.updateAnimations(dt, nullptr);
      panel->etree.updateTransitions(dt);
      panel->etree.updateKineticScroll(dt);
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

  G_ASSERT(etree.rebuildFlagsAccum == 0);
}


BBox2 GuiScene::calcXmbViewport(Element *node, Element **nearest_xmb_viewport) const
{
  BBox2 viewport(0, 0, StdGuiRender::screen_width(), StdGuiRender::screen_height());
  if (!node->xmb)
    return viewport;
  for (Element *e = node->xmb->xmbParent; e; e = e->parent)
  {
    bool isXmbViewport = e->xmb && e->xmb->nodeDesc.RawGetSlotValue(stringKeys->isViewport, false);

    if (isXmbViewport && nearest_xmb_viewport && !*nearest_xmb_viewport)
      *nearest_xmb_viewport = e;

    if (e->hasFlags(Element::F_CLIP_CHILDREN) || isXmbViewport)
    {
      BBox2 b = e->calcTransformedBboxPadded();
      viewport.lim[0] = ::max(viewport.lim[0], b.lim[0]);
      viewport.lim[1] = ::min(viewport.lim[1], b.lim[1]);
    }
  }
  return viewport;
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

    if (isXmbNodeInputCovered(elem)) // nothing to do yet
      break;

    erase_items(keptXmbFocus, i, keptXmbFocus.size() - i);
    setXmbFocus(elem);
    break;
  }

  dbgIsInTryRestoreSavedXmbFocus = false;
}


bool GuiScene::isXmbNodeInputCovered(Element *node) const
{
  int focusInputStackDepth = -1, focusZOrder = 0;
  for (size_t iItem = 0, nItems = inputStack.stack.size(); iItem < nItems; ++iItem)
  {
    const InputEntry &ie = inputStack.stack[iItem];
    if (ie.elem == node)
    {
      focusInputStackDepth = iItem;
      focusZOrder = ie.zOrder;
      break;
    }
  }

  if (focusInputStackDepth < 0)
    return false;

  BBox2 viewport = calcXmbViewport(node, nullptr);
  const float edgeThreshold = StdGuiRender::screen_height() * 0.01f;

  for (size_t iItem = 0; iItem < focusInputStackDepth; ++iItem)
  {
    const InputEntry &ie = inputStack.stack[iItem];
    if (ie.zOrder >= focusZOrder)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
      {
        BBox2 bbox = ie.elem->transformedBbox;
        if (bbox.isempty() || bbox.width().x < 0.1f || bbox.width().y < 0.1f)
          bbox = ie.elem->calcTransformedBbox();

        if (bbox.left() <= viewport.left() + edgeThreshold && bbox.top() <= viewport.top() + edgeThreshold &&
            bbox.right() >= viewport.right() - edgeThreshold && bbox.bottom() >= viewport.bottom() - edgeThreshold)
        {
          return true;
        }
      }
    }
  }
  return false;
}

void GuiScene::validateOverlaidXmbFocus()
{
  // visuallog::logmsg("validateOverlaidXmbFocus()");
  if (etree.xmbFocus)
  {
    if (isXmbNodeInputCovered(etree.xmbFocus))
    {
      G_ASSERT_RETURN(keptXmbFocus.size() < 100, );
      erase_item_by_value(keptXmbFocus, etree.xmbFocus); // should not happen, but for safety add each element only once
      keptXmbFocus.push_back(etree.xmbFocus);            // push to top
      setXmbFocus(nullptr);
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

  if (status.sceneIsBroken)
    status.renderError(guiContext);
  else
  {
    {
      AutoProfileScope profileListRender(profiler, M_RENDER_LIST_RENDER);

      renderList.render(*guiContext);
      if (activeCursor)
        activeCursor->renderList.render(*guiContext);
      else if (isCursorForcefullyEnabled())
        renderInternalCursor();
    }

    if (status.renderErrorFlag)
      status.renderError(guiContext);
  }

  G_ASSERT(!guiContext->restore_viewport());

  renderXmbDebug();
  renderDirPadNavDebug();
  renderInputStackDebug();
  drawDebugRenderBoxes();

  if (profiler)
  {
    profiler->afterRender();
    renderProfileStats();
  }

#if 0
  if (getMouse())
  {
    const HumanInput::PointingRawState::Mouse &rs = getMouse()->getRawState().mouse;
    String msg;
    guiContext->set_font(0);
    {
      msg.printf(32, "mousePos = %.f, %.f, raw cursor = %.f, %.f", P2D(pointerPos.mousePos), rs.x, rs.y);
      auto bbox = StdGuiRender::get_str_bbox(msg, msg.length(), guiContext->curRenderFont);
      guiContext->goto_xy(guiContext->screen_width()-bbox.width().x-10, 30);
      guiContext->set_color(200,200,0);
      guiContext->draw_str(msg);
    }
    {
      msg.printf(32, "have activeCursor = %d, translate = %.f, %.f",
        activeCursor!=nullptr,
        activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform
          ? activeCursor->etree.root->transform->translate.x : -999.0f,
        activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform
          ? activeCursor->etree.root->transform->translate.y : -999.0f
      );
      auto bbox = StdGuiRender::get_str_bbox(msg, msg.length(), guiContext->curRenderFont);
      guiContext->goto_xy(guiContext->screen_width()-bbox.width().x-10, 30+(bbox.width().y*1.25f));
      guiContext->set_color(200,200,0);
      guiContext->draw_str(msg);
    }
  }
#endif

  pointerPos.debugRender(guiContext);

  StdGuiRender::release();

  if (updateHoverRequestState == UpdateHoverRequestState::WaitingForRender)
    updateHoverRequestState = UpdateHoverRequestState::NeedToUpdate;
}


void GuiScene::renderXmbDebug()
{
  if (!debug_xmb)
    return;

  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->xmb)
    {
      BBox2 bbox = ie.elem->calcTransformedBbox();
      if (!bbox.isempty())
      {
        guiContext->set_color(160, 130, 120, 120);
        guiContext->render_frame(bbox.left() + 1, bbox.top() + 1, bbox.right() - 1, bbox.bottom() - 1, 2);
      }
    }
  }

  if (etree.xmbFocus)
  {
    BBox2 bbox = etree.xmbFocus->calcTransformedBbox();
    bool isInInputStack = false;
    for (const InputEntry &ie : inputStack.stack)
    {
      if (ie.elem == etree.xmbFocus)
      {
        isInInputStack = true;
        break;
      }
    }
    if (isInInputStack)
      guiContext->set_color(220, 200, 0, 180);
    else
      guiContext->set_color(240, 50, 0, 180);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);

    BBox2 viewport = calcXmbViewport(etree.xmbFocus, nullptr);
    guiContext->set_color(20, 200, 0, 180);
    guiContext->render_frame(viewport.left(), viewport.top(), viewport.right(), viewport.bottom(), 2);
  }
}


void GuiScene::renderDirPadNavDebug()
{
  if (!debug_dirpad_nav)
    return;

  static E3DCOLOR colors[] = {
    E3DCOLOR(120, 120, 0, 160),
    E3DCOLOR(120, 0, 120, 160),
    E3DCOLOR(0, 120, 120, 160),
    E3DCOLOR(20, 100, 160, 160),
    E3DCOLOR(160, 20, 100, 160),
    E3DCOLOR(100, 160, 20, 160),
  };

  for (const InputEntry &ie : inputStack.stack)
  {
    Element *elem = ie.elem;

    if (!elem->isDirPadNavigable())
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
      continue;

    const int nColors = sizeof(colors) / sizeof(colors[0]);
    guiContext->set_color(colors[elem->getHierDepth() % nColors]);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);
  }
}


void GuiScene::renderInputStackDebug()
{
  if (!debug_input_stack)
    return;

  for (const InputEntry &ie : inputStack.stack)
  {
    Element *elem = ie.elem;
    if (!elem->behaviorsSummaryFlags)
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty())
      continue;

    E3DCOLOR color(20, 20, 20, 200);
    if (elem->hasFlags(Element::F_INPUT_PASSIVE))
      color = E3DCOLOR(120, 160, 160, 200);
    else
    {
      if (elem->hasBehaviors(Behavior::F_HANDLE_MOUSE))
        color.r = 200;
      if (elem->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
        color.g = 200;
      if (!elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_HANDLE_KEYBOARD))
        color.b = 200;
    }

    guiContext->set_color(color);
    guiContext->render_frame(bbox.left(), bbox.top(), bbox.right(), bbox.bottom(), 2);
  }
}


void GuiScene::renderInternalCursor()
{
  StdGuiRender::GuiContext &ctx = *guiContext;
  Point2 pos = getMousePos();
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

  auto itPanelData = panels.find(panel_idx);

  if (itPanelData == panels.end())
  {
    // guiContext->set_color(20, 200, 50);
    // guiContext->render_box(0, 0, 2000, 2000);
  }
  else if (status.sceneIsBroken)
    status.renderError(guiContext);
  else
  {
    Panel *panel = itPanelData->second->panel.get();
    panel->renderList.render(*guiContext);
    for (PanelPointer &ptr : panel->pointers)
    {
      if (ptr.cursor)
      {
        if (ptr.cursor->etree.root && ptr.cursor->etree.root->transform)
          ptr.cursor->etree.root->transform->translate = ptr.pos;

        ptr.cursor->renderList.render(*guiContext);
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

  if (needToDiscardPictures)
    needToDiscardPictures = !PictureManager::discard_unused_picture(/*force_lock*/ false);
}

void GuiScene::flushRender() { flushRenderImpl(FlushPart::MainScene); }

void GuiScene::flushPanelRender() { flushRenderImpl(FlushPart::Panel); }

void GuiScene::skipRender()
{
  ApiThreadCheck atc(this);

  lastRenderTimestamp = get_time_msec();

  if (needToDiscardPictures)
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
    updateHover();
    updateHoverRequestState = UpdateHoverRequestState::None;
  }
}


void GuiScene::updateHover()
{
  G_ASSERT(!dbgIsInUpdateHover);
  dbgIsInUpdateHover = true;

  dag::Vector<Point2, framemem_allocator> pointers;
  dag::Vector<SQInteger, framemem_allocator> stickScrollFlags;

  bool haveVisualPointer = activeCursor != nullptr;
  int numVisualPtrs = haveVisualPointer ? 1 : 0;

  const size_t nPtrs = numVisualPtrs + touchPointers.activePointers.size();
  pointers.resize(nPtrs);
  stickScrollFlags.resize(nPtrs);

  if (haveVisualPointer)
    pointers[0] = pointerPos.mousePos;
  for (size_t i = 0, n = touchPointers.activePointers.size(); i < n; ++i)
    pointers[i + numVisualPtrs] = touchPointers.activePointers[i].pos;

  if (nPtrs)
    etree.updateHover(inputStack, nPtrs, &pointers[0], &stickScrollFlags[0]);
  else
    etree.updateHover(inputStack, 0, nullptr, nullptr); // prevent assert in [] operator

  Sqrat::Object clickableInfo(sqvm);

  bool willHandleClick = false;
  if (haveVisualPointer)
  {
    for (Element *elem : etree.topLevelHovers)
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

  bool cursorWasOverStickScroll = cursorOverStickScroll->getValueRef().Cast<SQInteger>() != 0;
  bool cursorIsOverStickScroll = haveVisualPointer && stickScrollFlags[0] != 0;

  String frpErrMsg;
  cursorOverStickScroll->setValue(Sqrat::Object(haveVisualPointer ? stickScrollFlags[0] : 0, sqvm), frpErrMsg);
  cursorOverClickable->setValue(Sqrat::Object(willHandleClick, sqvm), frpErrMsg);
  hoveredClickableInfo->setValue(clickableInfo, frpErrMsg);

  if (cursorWasOverStickScroll != cursorIsOverStickScroll)
    notifyInputConsumersCallback();

  validateOverlaidXmbFocus();

  dbgIsInUpdateHover = false;
}


void GuiScene::addCursor(Cursor *c) { allCursors.insert(c); }

void GuiScene::removeCursor(Cursor *c)
{
  allCursors.erase(c);
  if (c == activeCursor)
    activeCursor = nullptr;
  for (auto &itPanelData : panels)
    itPanelData.second->panel->onRemoveCursor(c);
}


void GuiScene::updateActiveCursor()
{
  Cursor *prevCursor = activeCursor;
  activeCursor = nullptr;

  Point2 mousePos = pointerPos.mousePos;

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
    if (HumanInput::IGenPointing *mouse = pointerPos.getMouse())
      mousePos.set(mouse->getRawState().mouse.x, mouse->getRawState().mouse.y);
  }

  if (!isCursorForcefullyDisabled())
  {
    bool matched = false;
    if (!haveActiveCursorOnPanels())
    {
      for (const InputEntry &ie : cursorStack.stack)
      {
        if (ie.elem->hitTest(mousePos))
        {
          if (ie.elem->props.getCurrentCursor(stringKeys->cursor, &activeCursor))
          {
            matched = true;
            break;
          }

          if (config.useDefaultCursor && ie.elem->hasFlags(Element::F_STOP_MOUSE | Element::F_STOP_HOVER))
            break;
        }
      }
    }

    if (!matched && config.useDefaultCursor)
      activeCursor = config.defaultCursor.Cast<Cursor *>();

    if (activeCursor && activeCursor->etree.root && activeCursor->etree.root->transform)
      activeCursor->etree.root->transform->translate = cursorPosition(activeCursor);
  }

  if (prevCursor && prevCursor != activeCursor)
    prevCursor->etree.skipAllOneShotAnims();
}


Point2 GuiScene::cursorPosition(Cursor *cursor) const
{
  Point2 result(pointerPos.mousePos);

  if (!shouldIgnoreDeviceCursorPos)
  {
    if (HumanInput::IGenPointing *mouse = getMouse())
    {
      result.x = mouse->getRawState().mouse.x;
      result.y = mouse->getRawState().mouse.y;
    }
  }

  result.x -= cursor->hotspot.x;
  result.y -= cursor->hotspot.y;

  return result;
}


void GuiScene::setMousePos(const Point2 &p, bool reset_target)
{
  Point2 prevMousePos = pointerPos.mousePos;
  pointerPos.setMousePos(p, reset_target);

#if _TARGET_XBOX

  // xbox mouse driver doesn't send MOUSE_MOVE events
  const float moveFactor = lengthSq(prevMousePos - pointerPos.mousePos);
  if (moveFactor > 0.01f)
    onMouseEvent(darg::INP_EV_POINTER_MOVE, 0, pointerPos.mousePos.x, pointerPos.mousePos.y, 0);

#else

  if (!getMouse()) // since we won't get into mouse move event handler
  {
    const float moveFactor = lengthSq(prevMousePos - pointerPos.mousePos);
    if (moveFactor > 0.5f)
      updateHover();
  }

#endif
}


HumanInput::IGenPointing *GuiScene::getMouse()
{
  HumanInput::IGenPointing *mouse = nullptr;
  if (::global_cls_drv_pnt)
    mouse = ::global_cls_drv_pnt->getDevice(0);
  return mouse;
}

HumanInput::IGenJoystick *GuiScene::getJoystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


void GuiScene::moveMouseCursorToElem(Element *elem, bool use_transform)
{
  BBox2 bbox;
  if (use_transform)
    bbox = elem->calcTransformedBbox();
  else
    bbox = BBox2(elem->screenCoord.screenPos, elem->screenCoord.screenPos + elem->screenCoord.size);
  moveMouseCursorToElemBox(elem, bbox, true);
}


void GuiScene::moveMouseCursorToElemBox(Element *elem, const BBox2 &bbox, bool jump)
{
  Point2 anchor;
  Point2 nextPos;
  if (elem->getNavAnchor(anchor))
    nextPos = min(max(floor(bbox.leftTop() + anchor), bbox.leftTop()), bbox.rightBottom());
  else
    nextPos = floor(bbox.center());
  if (jump)
    pointerPos.requestNextFramePos(nextPos);
  else
    pointerPos.requestTargetPos(nextPos);
}


int GuiScene::handleMouseClick(InputEvent event, InputDevice device, int btn_idx, short mx, short my, int buttons, int accum_res)
{
  if (!etree.root)
    return 0;

  const int pointerId = 0;

  bool buttonDown = (event == INP_EV_PRESS);

  int summaryRes = accum_res;

  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
    {
      bool isHit = ie.elem->hitTest(mx, my);

      // set keyboard focus on click release
      if (isHit && !buttonDown && (btn_idx == 0 || btn_idx == 1) && ie.elem->hasBehaviors(Behavior::F_FOCUS_ON_CLICK) &&
          !etree.hasCapturedKbFocus() && !(summaryRes & R_PROCESSED))
        etree.setKbFocus(ie.elem);

      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_MOUSE)
          summaryRes |= bhv->mouseEvent(&etree, ie.elem, device, event, pointerId, btn_idx, mx, my, buttons, summaryRes);
      }

      if (isHit && ie.elem->hasFlags(Element::F_STOP_MOUSE))
        summaryRes |= R_PROCESSED | R_STOPPED;

      // release keyboard focus on click outside of element
      if (ie.elem == etree.kbFocus && !isHit && !etree.hasCapturedKbFocus())
        etree.setKbFocus(nullptr);
    }
  }

  return summaryRes;
}


int GuiScene::onMouseEvent(InputEvent event, int data, short mx, short my, int buttons, int prev_result)
{
  ApiThreadCheck atc(this);

  return onMouseEventInternal(event, DEVID_MOUSE, data, mx, my, buttons, prev_result);
}


int GuiScene::onMouseEventInternal(InputEvent event, InputDevice device, int data, short mx, short my, int buttons, int prev_result)
{
  DEBUG_TRACE_INPDEV("GuiScene::onMouseEvent(event=%d, device=%d, data=%d, mx=%d, my=%d, buttons=%d)", event, device, data, mx, my,
    buttons);

  const int pointerId = 0;

  if ((event == INP_EV_PRESS || event == INP_EV_RELEASE) &&
      (data == HumanInput::DBUTTON_SCROLLUP || data == HumanInput::DBUTTON_SCROLLDOWN))
  {
    // ignore 'click' events caused by wheel
    // instead process them as INP_EV_MOUSE_WHEEL messages
    return 0;
  }

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  // if (event == INP_EV_PRESS)
  //   visuallog::logmsg("---  INP_EV_PRESS  ---");
  // else if (event == INP_EV_RELEASE)
  //   visuallog::logmsg("---  INP_EV_RELEASE  ---");
  // else if (event == INP_EV_POINTER_MOVE)
  //   visuallog::logmsg("---  INP_EV_POINTER_MOVE  ---");

  if (event == INP_EV_POINTER_MOVE && gamepadCursor.isMovingMouse)
    device = DEVID_JOYSTICK;

  if (event == INP_EV_POINTER_MOVE)
  {
    auto mouse = getMouse();
    if (mouse && mouse->getRelativeMovementMode())
      return 0;

    if (!pointerPos.isSettingMousePos)
      setXmbFocus(nullptr);

    pointerPos.onMouseMoveEvent(Point2(mx, my));

    updateHover();
  }

  int summaryRes = prev_result;

  if (etree.root)
  {
    if (event == INP_EV_PRESS || event == INP_EV_RELEASE)
    {
      if (updateHotkeys())
        summaryRes |= R_PROCESSED;

      summaryRes |= handleMouseClick(event, device, data, mx, my, buttons, summaryRes);
    }
    else if (event == INP_EV_MOUSE_WHEEL)
    {
      Element *startHandler = nullptr;

      for (const InputEntry &ie : inputStack.stack)
      {
        if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
        {
          // If some element was initially hit, only its parents are allowed to process wheel events
          // We need to pass mouse wheel event through input stack to allow
          // scrolling containers that contain mouse-blocking children
          if (startHandler && !ie.elem->isAscendantOf(startHandler))
            continue;

          for (Behavior *bhv : ie.elem->behaviors)
          {
            if (bhv->flags & Behavior::F_HANDLE_MOUSE)
              summaryRes |= bhv->mouseEvent(&etree, ie.elem, device, event, pointerId, data, mx, my, buttons, summaryRes);
          }

          if (startHandler == nullptr && ie.elem->hitTest(mx, my))
            startHandler = ie.elem;
        }
      }
    }
    else if (event == INP_EV_POINTER_MOVE)
    {
      for (const InputEntry &ie : inputStack.stack)
      {
        if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE))
        {
          for (Behavior *bhv : ie.elem->behaviors)
            summaryRes |= bhv->mouseEvent(&etree, ie.elem, device, event, pointerId, data, mx, my, buttons, summaryRes);
        }
        if (ie.elem->hasFlags(Element::F_STOP_MOUSE) && ie.elem->hitTest(mx, my))
          summaryRes |= R_PROCESSED | R_STOPPED;
      }
    }
  }

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
  {
    // document could possibly be updated from handlers
    rebuildInvalidatedParts();
  }

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
  {
    rebuildElemStacks(false);
    updateHover();
  }

  return summaryRes;
}


int GuiScene::onPanelMouseEvent(Panel *panel, int hand, InputEvent event, InputDevice device, int button_id, short mx, short my,
  int buttons, int prev_result)
{
  // Mouse emulation for now
  DEBUG_TRACE_INPDEV("GuiScene::onPanelMouseEvent(hand=%d, event=%d, pos=%d, %d)", hand, event, mx, my);

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  if (event == INP_EV_POINTER_MOVE && panel->pointers[hand].isPresent && (fabsf(panel->pointers[hand].pos.x - mx) < 1) &&
      (fabsf(panel->pointers[hand].pos.y - my) < 1))
  {
    return 0;
  }
  if (event == INP_EV_POINTER_MOVE)
    panel->pointers[hand].pos.set(mx, my);

  int summaryRes = prev_result;

  for (const InputEntry &ie : panel->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_MOUSE)
          summaryRes |= bhv->mouseEvent(&panel->etree, ie.elem, device, event, hand, button_id, mx, my, buttons, summaryRes);
      }
      if (ie.elem->hasFlags(Element::F_STOP_MOUSE) && ie.elem->hitTest(mx, my))
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
    panel->rebuildStacks();

  return summaryRes & R_PROCESSED;
}


static HumanInput::PointingRawState::Touch fill_touch_data(Panel *panel, int hand, short x, short y)
{
  static unsigned last_touch_ord_id = 0;
  // DaRg does not use extra-data yet... And I am hesitant to cache previous touches in full
  HumanInput::PointingRawState::Touch out;
  out.x = x;
  out.y = y;
  out.touchSrc = HumanInput::PointingRawState::Touch::TS_touchScreen;
  if (!panel->touchPointers.contains(hand))
    out.touchOrdId = ++last_touch_ord_id;

  return out;
}


int GuiScene::onPanelTouchEvent(Panel *panel, int hand, InputEvent event, short tx, short ty, int prev_result)
{
  ApiThreadCheck atc(this);

  DEBUG_TRACE_INPDEV("GuiScene::onPanelTouchEvent(event=%d, hand=%d, pos=%d, %d)", event, hand, tx, ty);

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  HumanInput::PointingRawState::Touch touch = fill_touch_data(panel, hand, tx, ty);

  int summaryRes = prev_result;
  for (const InputEntry &ie : panel->inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_TOUCH) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_TOUCH)
          summaryRes |= bhv->touchEvent(&etree, ie.elem, event, nullptr, hand, touch, summaryRes);
      }

      if (ie.elem->hasFlags(Element::F_STOP_MOUSE) && ie.elem->hitTest(Point2(touch.x, touch.y)))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (event == INP_EV_POINTER_MOVE)
  {
    setXmbFocus(nullptr);
    panel->updateHover();
  }

  panel->touchPointers.updateState(event, hand, touch);

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
    rebuildInvalidatedParts();

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    panel->rebuildStacks();

  return summaryRes & R_PROCESSED;
}


int GuiScene::onTouchEvent(InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
  const HumanInput::PointingRawState::Touch &touch, int prev_result)
{
  ApiThreadCheck atc(this);

  DEBUG_TRACE_INPDEV("GuiScene::onTouchEvent(event=%d, touch_idx=%d)", event, touch_idx);

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  // if (event == INP_EV_PRESS)
  //   visuallog::logmsg("---  INP_EV_PRESS  ---");
  // else if (event == INP_EV_RELEASE)
  //   visuallog::logmsg("---  INP_EV_RELEASE  ---");
  // else if (event == INP_EV_POINTER_MOVE)
  //   visuallog::logmsg("---  INP_EV_POINTER_MOVE  ---");

  int summaryRes = prev_result;

  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->hasBehaviors(Behavior::F_HANDLE_TOUCH) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
    {
      for (Behavior *bhv : ie.elem->behaviors)
      {
        if (bhv->flags & Behavior::F_HANDLE_TOUCH)
          summaryRes |= bhv->touchEvent(&etree, ie.elem, event, pnt, touch_idx, touch, summaryRes);
      }

      if (ie.elem->hasFlags(Element::F_STOP_MOUSE) && ie.elem->hitTest(Point2(touch.x, touch.y)))
        summaryRes |= R_PROCESSED | R_STOPPED;
    }
  }

  if (event == INP_EV_POINTER_MOVE)
    setXmbFocus(nullptr);

  touchPointers.updateState(event, touch_idx, touch);
  updateHover();

  callScriptHandlers();

  if (summaryRes & R_PROCESSED)
  {
    // document could possibly be updated from handlers
    rebuildInvalidatedParts();
  }

  if (summaryRes & R_REBUILD_RENDER_AND_INPUT_LISTS)
    rebuildElemStacks(false);

  return summaryRes;
}


int GuiScene::onServiceKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc, int prev_result)
{
  G_UNUSED(repeat);
  G_UNUSED(wc);
  G_UNUSED(prev_result);

  DEBUG_TRACE_INPDEV("GuiScene::onServiceKbdEvent(event=%d, key_idx=%d, shifts=%d, wc=%d)", event, key_idx, shifts, wc);

  if (status.renderErrorFlag && event == INP_EV_PRESS && key_idx == HumanInput::DKEY_BACK &&
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

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  int resFlags = prev_result;

  eastl::vector_set<HotkeyButton>::iterator itClickBtn;
  if (config.kbCursorControl && event == INP_EV_RELEASE &&
      (itClickBtn = pressedClickButtons.find(HotkeyButton(DEVID_KEYBOARD, key_idx))) != pressedClickButtons.end())
  {
    resFlags = handleMouseClick(INP_EV_RELEASE, DEVID_KEYBOARD, 0, pointerPos.mousePos.x, pointerPos.mousePos.y, 1, resFlags);
    pressedClickButtons.erase(itClickBtn);
  }

  Element *focused = etree.kbFocus;
  if (focused && focused->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
  {
    for (Behavior *bhv : focused->behaviors)
      if (bhv->flags & Behavior::F_HANDLE_KEYBOARD)
        resFlags |= bhv->kbdEvent(&etree, focused, event, key_idx, repeat, wc, resFlags);
  }

  if ((inputStack.summaryBhvFlags & Behavior::F_HANDLE_KEYBOARD_GLOBAL) == Behavior::F_HANDLE_KEYBOARD_GLOBAL)
  {
    for (const InputEntry &ie : inputStack.stack)
    {
      if (ie.elem != focused && ie.elem->hasBehaviors(Behavior::F_HANDLE_KEYBOARD))
      {
        for (Behavior *bhv : ie.elem->behaviors)
          if ((bhv->flags & Behavior::F_HANDLE_KEYBOARD_GLOBAL) == Behavior::F_HANDLE_KEYBOARD_GLOBAL)
            resFlags |= bhv->kbdEvent(&etree, ie.elem, event, key_idx, repeat, wc, resFlags);
      }
    }
  }

  if (!(resFlags & R_PROCESSED) && !repeat)
    if (updateHotkeys())
      resFlags |= R_PROCESSED;

  if (config.kbCursorControl && event == INP_EV_PRESS && !(resFlags & R_PROCESSED) && config.isClickButton(DEVID_KEYBOARD, key_idx))
  {
    resFlags |= handleMouseClick(INP_EV_PRESS, DEVID_KEYBOARD, 0, pointerPos.mousePos.x, pointerPos.mousePos.y, 1, resFlags);
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
    rebuildElemStacks(false);
    updateHover();
  }

  return resFlags;
}


int GuiScene::onJoystickBtnEvent(HumanInput::IGenJoystick *joy, InputEvent event, int btn_idx, int device_number,
  const HumanInput::ButtonBits &buttons, int prev_result)
{
  ApiThreadCheck atc(this);

  // DEBUG_TRACE_INPDEV("GuiScene::onJoystickBtnEvent(event=%d, btn_idx=%d, dev_no=%d)", event, btn_idx, device_number);

  if (status.sceneIsBroken || !isInputActive())
    return 0;

  int resFlags = prev_result;

  if (event == INP_EV_RELEASE && btn_idx == lastDirPadNavDir)
    lastDirPadNavDir = -1;

  eastl::vector_set<HotkeyButton>::iterator itClickBtn;
  if (event == INP_EV_RELEASE &&
      (itClickBtn = pressedClickButtons.find(HotkeyButton(DEVID_JOYSTICK, btn_idx))) != pressedClickButtons.end())
  {
    resFlags = handleMouseClick(INP_EV_RELEASE, DEVID_JOYSTICK, 0, pointerPos.mousePos.x, pointerPos.mousePos.y, 1, resFlags);
    pressedClickButtons.erase(itClickBtn);
  }

  if (inputStack.summaryBhvFlags & Behavior::F_HANDLE_JOYSTICK)
  {
    for (const InputEntry &ie : inputStack.stack)
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_JOYSTICK))
      {
        for (Behavior *bhv : ie.elem->behaviors)
          if (bhv->flags & Behavior::F_HANDLE_JOYSTICK)
            resFlags |= bhv->joystickBtnEvent(&etree, ie.elem, joy, event, btn_idx, device_number, buttons, resFlags);
      }
    }
  }

  if (::dgs_app_active && !(resFlags & R_PROCESSED))
    if (updateHotkeys())
      resFlags |= R_PROCESSED;

  if (::dgs_app_active && event == INP_EV_PRESS && !(resFlags & R_PROCESSED) && config.isClickButton(DEVID_JOYSTICK, btn_idx))
  {
    resFlags |= handleMouseClick(INP_EV_PRESS, DEVID_JOYSTICK, 0, pointerPos.mousePos.x, pointerPos.mousePos.y, 1, resFlags);
    pressedClickButtons.insert(HotkeyButton(DEVID_JOYSTICK, btn_idx));
  }

  if (::dgs_app_active && (event == INP_EV_PRESS) && !(resFlags & R_PROCESSED))
  {
    using namespace HumanInput;
    if (btn_idx == JOY_XINPUT_REAL_BTN_D_UP || btn_idx == JOY_XINPUT_REAL_BTN_D_DOWN || btn_idx == JOY_XINPUT_REAL_BTN_D_LEFT ||
        btn_idx == JOY_XINPUT_REAL_BTN_D_RIGHT)
    {
      dirPadNavigate(Direction(btn_idx));

      float delay = ::max(config.dirPadRepeatDelay, config.dirPadRepeatTime);
      if (delay > 0)
      {
        lastDirPadNavDir = btn_idx;
        dirPadNavRepeatTimeout = delay;
      }
    }
  }

  callScriptHandlers();

  if (resFlags & R_PROCESSED)
    rebuildInvalidatedParts();

  if (resFlags & R_REBUILD_RENDER_AND_INPUT_LISTS)
  {
    rebuildElemStacks(false);
    updateHover();
  }

  return resFlags;
}


static Element *dir_pad_new_pos_trace(const InputStack &inputStack, const Point2 &pt)
{
  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->hitTest(pt))
    {
      if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
        return ie.elem;
    }
  }
  return nullptr;
}

// return absolute value
static Point2 distance_to_box(const Point2 &p, const BBox2 &bbox)
{
  Point2 d(0, 0);

  if (bbox.left() > p.x)
    d.x = bbox.left() - p.x;
  else if (bbox.right() < p.x)
    d.x = p.x - bbox.right();

  if (bbox.top() > p.y)
    d.y = bbox.top() - p.y;
  else if (bbox.bottom() < p.y)
    d.y = p.y - bbox.bottom();

  return d;
}

static float weighted_nav_distance(float dx, float dy)
{
  return sqrtf(dx * dx + dy * dy);
  // return powf(dx*dx + dy*dy, 0.75f);
}


static float out_of_cone_penalty(float dist) { return dist * 5.0f + 10000.0f; }

static float other_side_penalty(float dist) { return dist * 20.0f + 50000.0f; }


static float calc_dir_nav_distance(const Point2 &from, const BBox2 &bbox, Direction dir, bool allow_reverse)
{
  const float anisotropyMul = 4.0f;
  const float coneAtanLim = 1.2f;

  float distance = 1e9f;
  ;
  Point2 delta = distance_to_box(from, bbox);
  if (dir == DIR_RIGHT)
  {
    float d = weighted_nav_distance(delta.x, delta.y * anisotropyMul);
    if (bbox.left() > from.x)
    {
      if (delta.y > delta.x * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else if (dir == DIR_LEFT)
  {
    float d = weighted_nav_distance(delta.x, delta.y * anisotropyMul);
    if (bbox.right() < from.x)
    {
      if (delta.y > delta.x * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else if (dir == DIR_DOWN)
  {
    float d = weighted_nav_distance(delta.x * anisotropyMul, delta.y);
    if (bbox.top() > from.y)
    {
      if (delta.x > delta.y * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  else
  {
    G_ASSERT(dir == DIR_UP);
    float d = weighted_nav_distance(delta.x * anisotropyMul, delta.y);
    if (bbox.bottom() < from.y)
    {
      if (delta.x > delta.y * coneAtanLim)
        distance = out_of_cone_penalty(d);
      else
        distance = d;
    }
    else if (allow_reverse)
      distance = other_side_penalty(d);
  }
  return distance;
}


void GuiScene::dirPadNavigate(Direction dir)
{
  using namespace HumanInput;

  if (dir != DIR_RIGHT && dir != DIR_LEFT && dir != DIR_DOWN && dir != DIR_UP)
    return;

  if (isXmbModeOn->getValueRef().Cast<bool>())
  {
    if (xmbNavigate(dir) || etree.xmbFocus)
      return;
  }

  float minDistance = 1e9f;
  Element *target = nullptr;

  for (const InputEntry &ie : inputStack.stack)
  {
    Element *elem = ie.elem;

    if (!elem->isDirPadNavigable())
      continue;

    const BBox2 &bbox = elem->clippedScreenRect;
    if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
      continue;

    if (dir_pad_new_pos_trace(inputStack, bbox.center()) != elem)
      continue;

    float distance = calc_dir_nav_distance(pointerPos.mousePos, bbox, dir, true);

    if (distance < minDistance)
    {
      target = elem;
      minDistance = distance;
    }
  }

  bool xmbFocusSet = false;
  if (target && target->xmb && (target->xmb->xmbParent || !target->xmb->xmbChildren.empty()))
    xmbFocusSet = trySetXmbFocus(target);
  else
    setXmbFocus(nullptr);

  if (target && !xmbFocusSet)
    moveMouseCursorToElemBox(target, target->clippedScreenRect, false);
}


bool GuiScene::trySetXmbFocus(Element *target)
{
  G_ASSERT(!isClearing);
  G_ASSERT(!target || target->etree);

  if (target && target->etree->panel)
    return false;

  Element *newFocus = nullptr;
  if (DAGOR_UNLIKELY(target && target->isDetached()))
  {
    logwarn("daRg: Setting XMB focus to detached element");
  }
  else if (target && target->xmb)
  {
    if (target->xmb->calcCanFocus())
      newFocus = target;
  }
  setXmbFocus(newFocus);
  return newFocus != nullptr;
}


void GuiScene::setXmbFocus(Element *elem)
{
  if (elem && elem->etree->panel)
    return;

  if (etree.xmbFocus == elem)
    return;

  if (DAGOR_UNLIKELY(elem && elem->isDetached()))
  {
    G_ASSERT(!"Setting XMB focus to detached element");
    etree.xmbFocus = nullptr;
  }
  else if (DAGOR_UNLIKELY(elem && !elem->xmb))
  {
    G_ASSERT(!"Setting XMB focus to non-XMB element");
    etree.xmbFocus = nullptr;
  }
  else
  {
    etree.xmbFocus = elem;
  }

  String errMsg;
  isXmbModeOn->setValue(Sqrat::Object(etree.xmbFocus != nullptr, sqvm), errMsg);
}


bool GuiScene::xmbGridNavigate(Direction dir)
{
  G_ASSERT_RETURN(etree.xmbFocus, false);
  G_ASSERT_RETURN(etree.xmbFocus->xmb, false);
  Element *xmbParent = etree.xmbFocus->xmb->xmbParent;
  G_ASSERT_RETURN(xmbParent, false);
  G_ASSERT_RETURN(xmbParent->xmb, false);
  Element *xmbGrandParent = xmbParent->xmb->xmbParent;
  if (!xmbGrandParent || !xmbGrandParent->xmb)
    return false;
  if (!xmbParent->xmb->nodeDesc.RawGetSlotValue("isGridLine", false))
    return false;
  G_ASSERT_RETURN(xmbParent->layout.flowType == FLOW_HORIZONTAL || xmbParent->layout.flowType == FLOW_VERTICAL, false);

  bool horiz = (dir == DIR_LEFT || dir == DIR_RIGHT);
  if ((xmbParent->layout.flowType == FLOW_HORIZONTAL) == horiz)
    return false;
  if ((xmbGrandParent->layout.flowType == FLOW_HORIZONTAL) != horiz)
    return false;

  auto curCellIt = eastl::find(xmbParent->xmb->xmbChildren.begin(), xmbParent->xmb->xmbChildren.end(), etree.xmbFocus);
  G_ASSERT_RETURN(curCellIt != xmbParent->xmb->xmbChildren.end(), false);
  ptrdiff_t curCellIdx = eastl::distance(xmbParent->xmb->xmbChildren.begin(), curCellIt);

  auto curLineIt = eastl::find(xmbGrandParent->xmb->xmbChildren.begin(), xmbGrandParent->xmb->xmbChildren.end(), xmbParent);
  G_ASSERT_RETURN(curLineIt != xmbGrandParent->xmb->xmbChildren.end(), false);
  ptrdiff_t curLineIdx = eastl::distance(xmbGrandParent->xmb->xmbChildren.begin(), curLineIt);

  int delta = (dir == DIR_LEFT || dir == DIR_UP) ? -1 : 1;

  int newLineIdx = curLineIdx + delta;
  bool wrap = xmbGrandParent->xmb->nodeDesc.RawGetSlotValue<bool>("wrap", true);
  if (wrap)
    newLineIdx = (newLineIdx + xmbGrandParent->xmb->xmbChildren.size()) % xmbGrandParent->xmb->xmbChildren.size();

  if (newLineIdx < 0 || newLineIdx >= xmbGrandParent->xmb->xmbChildren.size())
    return false;

  Element *newLine = xmbGrandParent->xmb->xmbChildren[newLineIdx];
  if (newLine->xmb->xmbChildren.empty())
    return false;
  if ((newLine->layout.flowType == FLOW_HORIZONTAL) == horiz)
    return false;

  Element *newCell = newLine->xmb->xmbChildren[::clamp(int(curCellIdx), 0, int(newLine->xmb->xmbChildren.size() - 1))];
  setXmbFocus(newCell);
  return true;
}


bool GuiScene::xmbNavigate(Direction dir)
{
  G_ASSERT_RETURN(etree.xmbFocus, false);

  bool horiz = (dir == DIR_LEFT || dir == DIR_RIGHT);
  int delta = (dir == DIR_LEFT || dir == DIR_UP) ? -1 : 1;

  G_UNUSED(horiz);
  Element *focus = etree.xmbFocus;
  G_ASSERT_RETURN(focus->xmb, false);
  Element *xmbParent = focus->xmb->xmbParent;

  bool isLeaving = false;

  if (xmbParent)
  {
    G_ASSERT_RETURN(xmbParent->xmb, false);
    if (xmbParent->layout.flowType == FLOW_PARENT_RELATIVE || xmbParent->xmb->nodeDesc.RawGetSlotValue("screenSpaceNav", false))
    {
      float minDistance = 1e9f;
      Element *target = nullptr;
      Point2 srcPos = focus->calcTransformedBbox().center();
      for (Element *elem : xmbParent->xmb->xmbChildren)
      {
        G_ASSERT_CONTINUE(elem->xmb);
        if (elem == focus)
          continue;
        if (!elem->xmb->calcCanFocus())
          continue;

        const BBox2 &bbox = elem->calcTransformedBbox();
        if (bbox.isempty() || bbox.width().x < 1 || bbox.width().y < 1)
          continue;

        float distance = calc_dir_nav_distance(srcPos, bbox, dir, false);
        if (distance < minDistance)
        {
          target = elem;
          minDistance = distance;
        }
      }

      if (target)
        setXmbFocus(target);
      else
        isLeaving = true;
    }
    else
    {
      int newIdx = -1;

      if (!xmbGridNavigate(dir))
      {
        if ((xmbParent->layout.flowType == FLOW_HORIZONTAL && horiz) || (xmbParent->layout.flowType == FLOW_VERTICAL && !horiz))
        {
          XmbData *parentXmbData = xmbParent->xmb;
          G_ASSERT(parentXmbData);
          auto prevIt = eastl::find(parentXmbData->xmbChildren.begin(), parentXmbData->xmbChildren.end(), etree.xmbFocus);
          G_ASSERT(prevIt != parentXmbData->xmbChildren.end());
          if (prevIt != parentXmbData->xmbChildren.end())
          {
            ptrdiff_t prevIdx = eastl::distance(parentXmbData->xmbChildren.begin(), prevIt);
            newIdx = prevIdx + delta;
            bool wrap = parentXmbData->nodeDesc.RawGetSlotValue<bool>("wrap", true);
            if (wrap)
              newIdx = (newIdx + parentXmbData->xmbChildren.size()) % parentXmbData->xmbChildren.size();
            else if (newIdx < 0 || newIdx >= parentXmbData->xmbChildren.size())
              newIdx = -1;
          }
        }

        if (newIdx >= 0)
        {
          Element *newFocus = xmbParent->xmb->xmbChildren[newIdx];
          setXmbFocus(newFocus);
        }
        else
          isLeaving = true;
      }
    }
  }

  bool stopProcessing = true;
  if (isLeaving)
  {
    XmbCode xmbRes = XMB_CONTINUE;
    Sqrat::Function onLeave(etree.xmbFocus->xmb->nodeDesc, "onLeave");
    if (!onLeave.IsNull())
    {
      Sqrat::Object res;
      if (onLeave.Evaluate(dir, res) && res.GetType() == OT_INTEGER)
        xmbRes = res.Cast<XmbCode>();
    }
    stopProcessing = xmbRes == XMB_STOP;
    if (xmbRes == XMB_CONTINUE)
      setXmbFocus(nullptr);
  }

  return stopProcessing;
}


void GuiScene::rebuildXmb()
{
  if (etree.root)
    rebuildXmb(etree.root, nullptr);
}


void GuiScene::rebuildXmb(Element *node, Element *xmb_parent)
{
  if (xmb_parent)
    G_ASSERT(xmb_parent->xmb);

  if (node->xmb)
  {
    if (xmb_parent)
      xmb_parent->xmb->xmbChildren.push_back(node);
    node->xmb->xmbParent = xmb_parent;
    node->xmb->xmbChildren.clear();
  }

  Element *nextXmbParent = node->xmb ? node : xmb_parent;
  for (Element *child : node->children)
    rebuildXmb(child, nextXmbParent);
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
  if (!etree.xmbFocus)
    return;

  G_ASSERT_RETURN(etree.xmbFocus->xmb, );

  float scrollSpeed[2] = {1.0f, 1.0f};
  bool scrollToEdge[2] = {false, false};
  if (etree.xmbFocus->xmb->xmbParent)
  {
    XmbData *pxmb = etree.xmbFocus->xmb->xmbParent->xmb;
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
  BBox2 viewport = calcXmbViewport(etree.xmbFocus, &scrollRoot);

  bool scrolled = false;

  if (scrollToEdge[0] == scrollToEdge[1])
  {
    // both directions simultaneously
    scrolled = scrollToEdge[0] ? scrollIntoView(etree.xmbFocus, viewport, maxScroll, scrollRoot)
                               : scrollIntoViewCenter(etree.xmbFocus, viewport, maxScroll, scrollRoot);
  }
  else
  {
    // 2 distinct operations
    Point2 separateMaxScroll[2] = {Point2(maxScroll[0], 0), Point2(0, maxScroll[1])};
    for (int axis = 0; axis < 2; ++axis)
    {
      bool res = scrollToEdge[axis] ? scrollIntoView(etree.xmbFocus, viewport, separateMaxScroll[axis], scrollRoot)
                                    : scrollIntoViewCenter(etree.xmbFocus, viewport, separateMaxScroll[axis], scrollRoot);
      if (res)
        scrolled = true;
    }
  }

  BBox2 bbox = etree.xmbFocus->calcTransformedBbox();
  if (!scrolled)
  {
    if (!etree.xmbFocus->clippedScreenRect.isempty())
      moveMouseCursorToElemBox(etree.xmbFocus, bbox, false);
  }
  else
  {
    bool needCursorMove = false;
    Point2 targetCursorPos = getMousePos();
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
      pointerPos.requestTargetPos(targetCursorPos);

    requestHoverUpdate();
  }
}


void GuiScene::doJoystickScroll(const Point2 &delta)
{
  if (!etree.root || !activeCursor)
    return;

  Point2 mousePos = getMousePos();

  if (etree.doJoystickScroll(inputStack, mousePos, delta))
  {
    // turn off xmb mode if focus is outside of the viewport or moved away from center
    // to avoid it bouncing back
    Element *xmbFocus = etree.xmbFocus;
    if (xmbFocus)
      G_ASSERT(xmbFocus->xmb);
    if (xmbFocus && xmbFocus->xmb && !xmbFocus->transformedBbox.isempty())
    {
      if (xmbFocus->clippedScreenRect.isempty())
        setXmbFocus(nullptr);
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
          BBox2 viewport = calcXmbViewport(xmbFocus, nullptr);
          BBox2 elemBbox = xmbFocus->calcTransformedBboxPadded();
          if (elemBbox.top() < viewport.top() || elemBbox.bottom() > viewport.bottom() || elemBbox.left() < viewport.left() ||
              elemBbox.right() > viewport.right())
          {
            setXmbFocus(nullptr);
          }
        }
        else
          setXmbFocus(nullptr); // not bouncing back to center
      }
    }

    updateHover();
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
  G_ASSERT(etree.rebuildFlagsAccum == 0);

  Tab<eastl::pair<HotkeyCombo *, Element *>> updatedCombos(framemem_ptr());
  size_t maxButtons = 0;

  for (const InputEntry &ie : inputStack.stack)
  {
    for (auto &combo : ie.elem->hotkeyCombos)
    {
      if (combo->updateOnCombo())
      {
        auto cmp = [&combo](eastl::pair<HotkeyCombo *, Element *> &item) { return is_hotkeys_subset(item.first, combo.get(), false); };
        bool isFirstUnique = eastl::find_if(updatedCombos.begin(), updatedCombos.end(), cmp) == updatedCombos.end();
        if (isFirstUnique)
        {
          updatedCombos.push_back(eastl::make_pair(combo.get(), ie.elem));
          maxButtons = ::max<size_t>(maxButtons, combo->buttons.size());
        }
      }
    }

    if (ie.elem->hasFlags(Element::F_STOP_HOTKEYS))
      break;
  }

  for (auto comboPair : updatedCombos)
  {
    HotkeyCombo *combo = comboPair.first;
    Element *comboElem = comboPair.second;

    if (combo->buttons.size() >= maxButtons)
    {
      if (combo->isPressed)
      {
        for (const InputEntry &ie : inputStack.stack)
          if (ie.elem != comboElem && !ie.elem->hotkeyCombos.empty() && is_hotkeys_subset(combo, ie.elem, true))
            ie.elem->clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);
      }

      combo->triggerOnCombo(this, comboElem);
    }
  }

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
  for (auto &itPanelData : panels)
  {
    if (handleInputEvent(itPanelData.second->panel->inputStack))
    {
      processed = true;
      break;
    }
  }

  processed = !processed && handleInputEvent(inputStack);

  // then check eventHandlers
  if (!processed)
  {
    sq_pushstring(sqvm, id, fullLen);
    HSQOBJECT hKey;
    sq_getstackobj(sqvm, -1, &hKey);
    Sqrat::Object key(hKey, sqvm);
    sq_pop(sqvm, 1);

    for (const InputEntry &ie : eventHandlersStack.stack)
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
      Sqrat::Object result;
      bool succeeded = handler.Evaluate(data, result);
      if (succeeded && (result.Cast<int>() != EVENT_CONTINUE))
        break;
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


void GuiScene::setKbFocus(Element *elem)
{
  ApiThreadCheck atc(this);
  etree.setKbFocus(elem);
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


void GuiScene::rebuildInvalidatedParts()
{
  G_ASSERT(etree.rebuildFlagsAccum == 0);
  G_ASSERT(!isClearing);

  if (invalidatedElements.empty())
  {
    etree.applyNextKbFocus();
    return;
  }

  TIME_PROFILE(rebuildInvalidatedParts);

  // Start all element trees rebuild

  etree.rebuildFlagsAccum = 0;

  for (Cursor *c : allCursors)
  {
    G_ASSERT(c->etree.rebuildFlagsAccum == 0);
    c->etree.rebuildFlagsAccum = 0;
  }

  for (auto &itPanelData : panels)
  {
    auto &panelData = itPanelData.second;
    G_ASSERT(panelData->panel->etree.rebuildFlagsAccum == 0);
    panelData->panel->etree.rebuildFlagsAccum = 0;
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
  for (; !invalidatedElements.empty() && !status.sceneIsBroken;)
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

  if (etree.rebuildFlagsAccum & rebuild_stacks_flags)
  {
    rebuildElemStacks(etree.rebuildFlagsAccum & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED);
    updateHover();
  }
  if (etree.rebuildFlagsAccum & ElementTree::RESULT_NEED_XMB_REBUILD)
    rebuildXmb();

  etree.rebuildFlagsAccum = 0;

  for (auto &itPanelData : panels)
  {
    Panel *panel = itPanelData.second->panel.get();
    if (panel->etree.rebuildFlagsAccum & rebuild_stacks_flags)
      panel->rebuildStacks();
    panel->etree.rebuildFlagsAccum = 0;
  }

  for (Cursor *c : allCursors)
  {
    if (c->etree.rebuildFlagsAccum & rebuild_stacks_flags)
      c->rebuildStacks();

    c->etree.rebuildFlagsAccum = 0;
  }

  etree.applyNextKbFocus();
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

  erase_item_by_value(keptXmbFocus, elem);
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
  if (elem_rebuilt->etree != &etree)
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
}


void GuiScene::runScriptScene(const char *fn)
{
  ApiThreadCheck atc(this);
  G_ASSERT(sqvm);

  clear(false);

  enable_kb_layout_and_locks_tracking();

  pointerPos.initMousePos();

  // G_ASSERTF(!moduleMgr->modules.size(), "Expected empty modules list on script run");
  if (!moduleMgr->modules.empty())
  {
    DEBUG_CTX("daRg: non-empty modules list on scene startup:");
    for (auto &module : moduleMgr->modules)
      DEBUG_CTX("  * %s", module.fn.c_str());
  }

  reloadScript(fn);
}


void GuiScene::reloadScript(const char *fn)
{
  TIME_PROFILE(GuiScene_reloadScript);

  ApiThreadCheck atc(this);

  clear(false);
  dasScriptsData->resetBeforeReload();
  frpGraph->generation++;
  frpGraph->setName(fn);

  // Ensure that string is always valid in this scope.
  // This may be too paranoid but in practice it happenned that the provided pointer to the string
  // was invalidated during script execution (in game settings datablock reload).
  String fnCopy(fn);

  Sqrat::Object rootDesc;
  String errMsg;
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
    etree.root = etree.rebuild(sqvm, etree.root, root, nullptr, root.scriptBuilder, 0, rebuildResult);
  }


  sq_collectgarbage(sqvm);

#if DAGOR_DBGLEVEL > 0
  logPrevReloadObservableLeftovers();
#endif

  if (etree.root == nullptr)
    darg_immediate_error(sqvm, String(0, "reloadScript failed: %s", fn));

  if (etree.root)
  {
    etree.root->recalcLayout();
    etree.root->callScriptAttach(this);
  }

  if (rebuildResult & rebuild_stacks_flags)
    rebuildElemStacks(rebuildResult & ElementTree::RESULT_HOTKEYS_STACK_MODIFIED);
  if (rebuildResult & ElementTree::RESULT_NEED_XMB_REBUILD)
    rebuildXmb();

  etree.rebuildFlagsAccum = 0;

  // ignore time spent for reloading, exclude it from all daRg timers used for behaviors and animations
  lastRenderTimestamp = get_time_msec();
}


void GuiScene::logPrevReloadObservableLeftovers()
{
  int numLeft = 0;
  String infoStr;
  for (BaseObservable *o : frpGraph->allObservables)
  {
    if (o->generation == frpGraph->generation - 1)
    {
      o->fillInfo(infoStr);
      debug("[FRP]: remaining observable: %s", infoStr.c_str());
      ++numLeft;
    }
  }
  debug("[FRP] %d observables left from reload step #%d", numLeft, frpGraph->generation - 1);
}


bool GuiScene::hasInteractiveElements()
{
  ApiThreadCheck atc(this);
  return hasInteractiveElements(Behavior::F_HANDLE_KEYBOARD | Behavior::F_HANDLE_MOUSE | Behavior::F_INTERNALLY_HANDLE_GAMEPAD_STICKS);
}

bool GuiScene::hasInteractiveElements(int bhv_flags)
{
  return (inputStack.summaryBhvFlags & bhv_flags) != 0 && !status.sceneIsBroken && isInputActive();
}

int GuiScene::calcInteractiveFlags()
{
  int iflags = 0;

  if ((hasInteractiveElements(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || isCursorForcefullyEnabled()) &&
      !isCursorForcefullyDisabled())
  {
    iflags |= IGuiSceneCallback::IF_MOUSE;
    if ((inputStack.summaryBhvFlags & Behavior::F_OVERRIDE_GAMEPAD_STICK) == 0)
      iflags |= IGuiSceneCallback::IF_GAMEPAD_AS_MOUSE;
  }
  if (hasInteractiveElements(Behavior::F_HANDLE_KEYBOARD))
    iflags |= IGuiSceneCallback::IF_KEYBOARD;
  if (hasInteractiveElements(Behavior::F_INTERNALLY_HANDLE_GAMEPAD_STICKS))
    iflags |= IGuiSceneCallback::IF_GAMEPAD_STICKS;
  if (inputStack.isDirPadNavigable)
    iflags |= IGuiSceneCallback::IF_DIRPAD_NAV;

  return iflags;
}

void GuiScene::rebuildElemStacks(bool refresh_hotkeys_nav)
{
  TIME_PROFILE(rebuildElemStacks);

  int prevInteractiveFlags = calcInteractiveFlags();

  renderList.clear();
  inputStack.clear();
  cursorStack.clear();
  eventHandlersStack.clear();

  if (etree.root)
  {
    ElemStacks stacks;
    stacks.rlist = &renderList;
    stacks.input = &inputStack;
    stacks.cursors = &cursorStack;
    stacks.eventHandlers = &eventHandlersStack;
    ElemStackCounters counters;

    etree.root->putToSortedStacks(stacks, counters, 0, false);
  }

  renderList.afterRebuild();

  // validateOverlaidXmbFocus();

  int curInteractiveFlags = calcInteractiveFlags();
  bool mouseFlagsChanged = (curInteractiveFlags & IGuiSceneCallback::IF_MOUSE) != (prevInteractiveFlags & IGuiSceneCallback::IF_MOUSE);
  bool dirPadNavChanged =
    (curInteractiveFlags & IGuiSceneCallback::IF_DIRPAD_NAV) != (prevInteractiveFlags & IGuiSceneCallback::IF_DIRPAD_NAV);

  if (refresh_hotkeys_nav)
  {
    TIME_PROFILE(refreshHotkeysNav);
    refreshHotkeysNav();
    notifyInputConsumersCallback();
  }
  else if (mouseFlagsChanged || dirPadNavChanged)
  {
    notifyInputConsumersCallback();
  }

  if (mouseFlagsChanged)
    updateHover();

  if (guiSceneCb && curInteractiveFlags != prevInteractiveFlags)
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
    pointerPos.onActivateSceneInput();

  notifyInputConsumersCallback();
}


void GuiScene::refreshHotkeysNav()
{
  if (sceneScriptHandlers.onHotkeysNav.IsNull())
    return;

  HumanInput::IGenJoystick *joy = getJoystick();
  HumanInput::IGenPointing *mouse = getMouse();
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

  for (const InputEntry &ie : inputStack.stack)
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
        item.SetValue(stringKeys->eventName, combo->eventName.c_str());
        item.SetValue(stringKeys->description, combo->description);
        item.SetValue(stringKeys->devId, nullObj);
        item.SetValue(stringKeys->btnId, nullObj);
        item.SetValue(stringKeys->btnName, nullObj);
        item.SetValue(stringKeys->buttons, nullObj);
        item.SetValue(stringKeys->action, Sqrat::Object(combo->handler.GetFunc(), sqvm));
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
          item.SetValue(stringKeys->devId, nullObj);
          item.SetValue(stringKeys->btnId, nullObj);
          item.SetValue(stringKeys->btnName, nullObj);
        }

        Sqrat::Array buttons(sqvm, combo->buttons.size());
        for (int iBtn = 0, nBtns = combo->buttons.size(); iBtn < nBtns; ++iBtn)
        {
          Sqrat::Table btnSq(sqvm);
          save_hotkey_button_to_sq_table(btnSq, combo->buttons[iBtn]);
          buttons.SetValue(SQInteger(iBtn), btnSq);
        }
        item.SetValue(stringKeys->buttons, buttons);
        item.SetValue(stringKeys->description, combo->description);
        item.SetValue(stringKeys->eventName, nullObj);
        item.SetValue(stringKeys->action, Sqrat::Object(combo->handler.GetFunc(), sqvm));

        res.Append(item);
      }
    }

    if (elem->hasFlags(Element::F_STOP_HOTKEYS))
      break;
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

  for (const InputEntry &ie : inputStack.stack)
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

  for (const InputEntry &ie : inputStack.stack)
  {
    if (ie.elem->hitTest(pointerPos.mousePos))
    {
      if (ie.elem->hasFlags(Element::F_JOYSTICK_SCROLL) && ie.elem->getAvailableScrolls() != 0)
      {
        buttons.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, config.joystickScrollAxisH));
        buttons.push_back(HotkeyButton(DEVID_JOYSTICK_AXIS, config.joystickScrollAxisV));
        break;
      }
      if (ie.elem->hasFlags(Element::F_STOP_HOVER | Element::F_STOP_MOUSE))
        break;
    }
  }

  if (inputStack.isDirPadNavigable)
  {
    buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_RIGHT));
    buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_LEFT));
    buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_DOWN));
    buttons.push_back(HotkeyButton(DEVID_JOYSTICK, JOY_XINPUT_REAL_BTN_D_UP));
  }

  bool consumeMouse = hasInteractiveElements(Behavior::F_HANDLE_MOUSE) || isCursorForcefullyEnabled();
  bool consumeTextInput = etree.kbFocus && is_text_input(etree.kbFocus);

  if (consumeMouse)
    append_items(buttons, config.clickButtons.size(), config.clickButtons.data());

  guiSceneCb->onInputConsumersChange(buttons, events, consumeTextInput);
}


void GuiScene::errorMessage(const char *msg)
{
  logerr("daRg: %s", msg);

  bool wasBroken = status.sceneIsBroken;

  status.appendErrorText(msg);

  if (!wasBroken && guiSceneCb)
    guiSceneCb->onToggleInteractive(0);
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

  (self->etree.*method)(trigger);
  // TODO: Optimize, trigger animation in a specific ElementTree only
  for (auto &itPanelData : self->panels)
    (itPanelData.second->panel->etree.*method)(trigger);

  return 0;
}

static Element *find_input_stack_elem_by_cb(InputStack &input_stack, Sqrat::Function &cb)
{
  bool res = false;
  for (InputEntry &ie : input_stack.stack)
    if (cb.Evaluate(ie.elem, ie.elem->props.scriptDesc, res) && res)
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
    self->etree.setKbFocus(nullptr, capture);
    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_INSTANCE && Sqrat::ClassType<ElementRef>::IsObjectOfClass(&obj))
  {
    ElementRef *ref = ElementRef::get_from_stack(vm, 2);
    if (!ref || !ref->elem)
      return sq_throwerror(vm, "Invalid ElementRef");
    self->etree.setKbFocus(ref->elem, capture);

    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
  {
    Sqrat::Function f(vm, Sqrat::Object(vm), obj);
    G_ASSERT(!f.IsNull());

    self->rebuildElemStacks(false);
    Element *elem = find_input_stack_elem_by_cb(self->inputStack, f);
    if (elem)
    {
      self->etree.setKbFocus(elem, capture);
      sq_pushbool(vm, SQTrue);
      return 1;
    }
  }
  else
  {
    self->rebuildElemStacks(false);
    Element *elem = find_input_stack_elem_by_key(vm, self->inputStack, obj);
    if (elem)
    {
      self->etree.setKbFocus(elem, capture);
      sq_pushbool(vm, SQTrue);
      return 1;
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
    self->moveMouseCursorToElem(ref->elem, useTransform);
    sq_pushbool(vm, SQTrue);
    return 1;
  }
  else if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
  {
    Sqrat::Function f(vm, Sqrat::Object(vm), obj);
    G_ASSERT(!f.IsNull());

    self->rebuildElemStacks(false);
    Element *elem = find_input_stack_elem_by_cb(self->inputStack, f);
    if (elem)
    {
      self->moveMouseCursorToElem(elem, useTransform);
      sq_pushbool(vm, SQTrue);
      return 1;
    }
  }
  else
  {
    self->rebuildElemStacks(false);
    Element *elem = find_input_stack_elem_by_key(vm, self->inputStack, obj);
    if (elem)
    {
      self->moveMouseCursorToElem(elem, useTransform);
      sq_pushbool(vm, SQTrue);
      return 1;
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
  PanelData *panelData = new PanelData(*scene, desc.value, idx);
  scene->panels[idx].reset(panelData);

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

  auto &panelData = it->second;
  for (int hand = 0; hand < NUM_VR_POINTERS; ++hand)
    if (panelData.get() == scene->vrPointers[hand].activePanel)
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
    etree.watchesAllowed = false;
    etree.canHandleInput = false;

    int rebuildResult = 0;
    etree.root = etree.rebuild(vm, etree.root, root, nullptr, root.scriptBuilder, 0, rebuildResult);
    G_ASSERT_RETURN(etree.root, sq_throwerror(vm, "Failed to build elements tree"));
    etree.root->recalcLayout();

    if (etree.root->children.size() != 1)
      return sq_throwerror(vm, String(32, "Enexpected root children count = %d", etree.root->children.size()));

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
  result.SetValue("x", self->getMousePos().x);
  result.SetValue("y", self->getMousePos().y);

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

  int relXPx = int(floor(self->getMousePos().x) - eref->elem->screenCoord.screenPos.x);
  int relYPx = int(floor(self->getMousePos().y) - eref->elem->screenCoord.screenPos.y);

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


static SQInteger get_comp_aabb_by_key(HSQUIRRELVM vm)
{
  HSQOBJECT keyObj;
  G_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, 2, &keyObj)));
  if (keyObj._type == OT_NULL)
    return 0;

  Sqrat::Object key(keyObj, vm);

  GuiScene *self = (GuiScene *)get_scene_from_sqvm(vm);

  Element *elem = self->getElementTree()->findElementByKey(key);
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

  for (RenderEntry &re : renderList.list)
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
      if (etree.root)
        etree.root->recalcLayout();
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
      pointerPos.onAppActivate();
      updateHover();
    }
    updateHotkeys(); // to compensate skipped joystick presses
  }
#if _TARGET_PC_WIN
  else if (msg == WM_WINDOWPOSCHANGED || msg == WM_DISPLAYCHANGE)
  {
    queryCurrentScreenResolution();
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
  String frpErrMsg;
  keyboardLayout->setValue(Sqrat::Object(layout, sqvm), frpErrMsg);
}


void GuiScene::onKeyboardLocksChanged(unsigned locks)
{
  ApiThreadCheck atc(this);
  String frpErrMsg;
  keyboardLocks->setValue(Sqrat::Object(locks, sqvm), frpErrMsg);
}


void GuiScene::blurWorld()
{
  if (renderList.worldBlurElems.empty())
    return;
  TIME_PROFILE(blurWorld);
  Tab<BBox2> blurBoxes(framemem_ptr());
  blurBoxes.reserve(renderList.worldBlurElems.size());
  {
    TIME_PROFILE_DEV(calcTransformedBbox);
    for (const Element *elem : renderList.worldBlurElems)
      blurBoxes.push_back(elem->calcTransformedBbox()); // may be optimized (probably using cached box from last frame)
  }
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


static constexpr float max_aim_distance = 8.f;
static constexpr float max_touch_distance = 0.02f;
static constexpr float new_touch_threshold = 0.25f * max_touch_distance;
struct PanelIntersection
{
  float distance = max_aim_distance + 1.f;
  Point2 uv;
};
using PossiblePanelIntersection = eastl::optional<PanelIntersection>;


static bool get_valid_uv_from_intersection_point(const TMatrix &panel_tm, const Point3 &panel_size, const Point3 &point,
  Point2 &out_uv)
{
  Point3 dir = point - panel_tm.getcol(3);
  Point2 pos{dir * normalize(panel_tm.getcol(0)), dir * normalize(panel_tm.getcol(1))};
  out_uv.set((pos.x + 0.5f * panel_size.x) / panel_size.x, 1.f - (pos.y + 0.5f * panel_size.y) / panel_size.y);
  return out_uv.x >= 0.f && out_uv.x < 1.f && out_uv.y >= 0.f && out_uv.y < 1.f;
}


static Point2 uv_to_panel_pixels(const PanelData &panel_data, const Point2 &uv)
{
  if (panel_data.canvas)
  {
    TextureInfo tinfo;
    panel_data.canvas->getTex()->getinfo(tinfo);
    return {uv.x * tinfo.w, uv.y * tinfo.h};
  }

  return Point2::ZERO;
}


static PossiblePanelIntersection cast_at_rectangle(const Panel &panel, const Point3 &src, const Point3 &target)
{
  const TMatrix &transform = panel.renderInfo.transform;
  Point3 planeP = transform.getcol(3);
  Point3 planeZ = transform.getcol(2);
  planeZ.normalize();

  Point2 uv;
  Plane3 plane(planeZ, planeP);
  Point3 isect = plane.calcLineIntersectionPoint(src, target);
  if (isect != src && get_valid_uv_from_intersection_point(transform, panel.spatialInfo.size, isect, uv))
    return PossiblePanelIntersection({(isect - src).length(), uv});

  return {};
}


static PossiblePanelIntersection project_at_rectangle(const Panel &panel, const Point3 &point, float max_dist)
{
  const TMatrix &panelTm = panel.renderInfo.transform;
  const Point3 panelOrigin = panelTm.getcol(3);
  const Point3 panelNormal = normalize(panelTm.getcol(2));
  Plane3 plane(panelNormal, panelOrigin);

  Point2 uv;
  Point3 projectedPoint = plane.project(point);
  float dist = -plane.distance(point); // FIXME: plane.distance(point) gives negative value... Wrong normals?
  if (dist > max_dist)
    return {};

  if (get_valid_uv_from_intersection_point(panelTm, panel.spatialInfo.size, projectedPoint, uv))
    return PossiblePanelIntersection({dist, uv});

  return {};
}


static PossiblePanelIntersection cast_at_hit_panel(const Panel &panel, const Point3 &world_from_pos, const Point3 &world_target_pos)
{
  switch (panel.spatialInfo.geometry)
  {
    case PanelGeometry::Rectangle: return cast_at_rectangle(panel, world_from_pos, world_target_pos);

    default: DAG_FATAL("Implement casting for this type of geometry!"); return {};
  }
}


static PossiblePanelIntersection cast_at_hit_panel(const Panel &panel, const TMatrix &aim)
{
  const Point3 worldPos = aim.getcol(3);
  const Point3 worldDir = normalize(aim.getcol(2));
  return cast_at_hit_panel(panel, worldPos, worldPos + worldDir * max_aim_distance);
}


static PossiblePanelIntersection project_at_hit_panel(const Panel &panel, const Point3 &point, float max_dist)
{
  G_ASSERTF_RETURN(panel.spatialInfo.geometry == PanelGeometry::Rectangle, {}, "Touching non-rectangular panels is not implemented");
  return project_at_rectangle(panel, point, max_dist);
}


bool GuiScene::worldToPanelPixels(int panel_idx, const Point3 &world_target_pos, const Point3 &world_cam_pos, Point2 &out_panel_pos)
{
  const auto &found = panels.find(panel_idx);
  if (found == panels.end())
    return false;

  const auto &panelData = found->second;
  if (!panelData->needRenderInWorld())
    return false;

  if (const Panel *panel = panelData->panel.get())
    if (const auto hit = cast_at_hit_panel(*panel, world_cam_pos, world_target_pos); hit.has_value())
    {
      out_panel_pos = uv_to_panel_pixels(*panelData, hit->uv);
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
  if (it == panels.end() || !it->second->panel)
    return;

  auto &panelData = it->second;
  panelData->panel->spatialInfo.renderRedirection = true;
  panelData->panel->updateSpatialInfoFromScript();
  ensurePanelBufferInitialized();
  int oldBuffer = getGuiContext()->currentBufferId;
  getGuiContext()->setBuffer(panel_render_buffer_index);
  panelData->updateTexture(*this, dst);
  getGuiContext()->setBuffer(oldBuffer);

  refreshGuiContextState();
}

void GuiScene::updateSpatialElements(const VrSceneData &vr_scene)
{
  ApiThreadCheck atc(this);

  int panelsToRender = 0;
  for (auto &itPanelData : panels)
  {
    auto &panelData = itPanelData.second;
    if (panelData->panel)
    {
      panelData->panel->updateSpatialInfoFromScript();
      if (panelData->needRenderInWorld())
        ++panelsToRender;
      else
        panelData->panel->renderInfo.isValid = false;
    }
  }

  if (panelsToRender == 0)
    return;

  ensurePanelBufferInitialized();
  int oldBuffer = getGuiContext()->currentBufferId;
  getGuiContext()->setBuffer(panel_render_buffer_index);

  //--- update render state -------------

  for (auto &itPanelData : panels)
  {
    auto &panelData = itPanelData.second;
    if (!panelData->needRenderInWorld())
      continue;

    panelData->updateTexture(*this, nullptr);

    Panel *panel = panelData->panel.get();

    panel->renderInfo.isValid = true;
    panel->renderInfo.texture = panelData->canvas->getId();
    panel->updateRenderInfoParamsFromScript();
    panel->renderInfo.resetPointer();

    darg_panel_renderer::panel_spatial_resolver(vr_scene.vrSpaceOrigin, vr_scene.camera, vr_scene.hands[0], vr_scene.hands[1],
      vr_scene.cameraFrustum, panel->spatialInfo, vr_scene.entityTmResolver, panel->renderInfo.transform);
  }

  getGuiContext()->setBuffer(oldBuffer);

  refreshGuiContextState();

  //--- process input: panel position has most-likely changed ---------------
  updateSpatialInteractionState(vr_scene);
}


void GuiScene::refreshVrCursorProjections()
{
  for (int hand : {0, 1})
    onVrInputEvent(INP_EV_POINTER_MOVE, hand);
}


bool GuiScene::hasAnyPanels()
{
  ApiThreadCheck atc(this);

  int actualPanels = 0;
  for (auto &itPanelData : panels)
    if (itPanelData.second->panel)
      ++actualPanels;

  return actualPanels > 0;
}


void GuiScene::updateSpatialInteractionState(const VrSceneData &vr_scene)
{
  float minHitDistance[NUM_VR_POINTERS] = {max_aim_distance + 1.f, max_aim_distance + 1.f};

  Point2 vrSurfaceHitUvs[NUM_VR_POINTERS];
  if (vr_scene.vrSurfaceIntersector)
    for (int hand : {0, 1})
    {
      const TMatrix &aim = vr_scene.aims[hand];
      const Point3 &pointerOrigin = aim.getcol(3);
      const Point3 &pointerDirection = aim.getcol(2);

      Point3 hit{0.f, 0.f, 0.f};
      if (vr_scene.vrSurfaceIntersector(pointerOrigin, pointerDirection, vrSurfaceHitUvs[hand], hit))
        minHitDistance[hand] = hit.length();
    }

  Point2 panelHitUvs[NUM_VR_POINTERS];
  int closestHitPanelIdxs[NUM_VR_POINTERS] = {-1, -1};
  for (const auto &itPanelData : panels)
  {
    const Panel *panel = itPanelData.second->panel.get();
    if (!panel->renderInfo.isValid)
      continue;

    if (!panel->spatialInfo.canBeTouched && !panel->spatialInfo.canBePointedAt)
      continue;

    for (int hand : {0, 1})
    {
      PossiblePanelIntersection hit;
      const PanelSpatialInfo &psi = panel->spatialInfo;
      float threshold = psi.canBeTouched ? min(max_touch_distance + 1.f, minHitDistance[hand]) : minHitDistance[hand];
      if (psi.canBeTouched)
      {
        bool panelCanBeTouchedWithThisHand = (psi.anchor == PanelAnchor::LeftHand && hand != 0) ||
                                             (psi.anchor == PanelAnchor::RightHand && hand != 1) ||
                                             psi.anchor == PanelAnchor::VRSpace || psi.anchor == PanelAnchor::Scene;
        if (panelCanBeTouchedWithThisHand)
          hit = project_at_hit_panel(*panel, vr_scene.indexFingertips[hand].getcol(3), max_touch_distance);
      }
      else if (psi.canBePointedAt)
        hit = cast_at_hit_panel(*panel, vr_scene.aims[hand]);

      if (hit.has_value() && hit->distance < threshold)
      {
        minHitDistance[hand] = hit->distance;
        closestHitPanelIdxs[hand] = itPanelData.first;
        panelHitUvs[hand] = hit->uv;
      }
    }
  }

  spatialInteractionState = {};
  for (int hand : {0, 1})
  {
    auto panelIt = panels.find(closestHitPanelIdxs[hand]);
    G_ASSERT(closestHitPanelIdxs[hand] < 0 || panelIt != panels.end());
    bool useTouchThresh = closestHitPanelIdxs[hand] >= 0 && panelIt->second->panel->spatialInfo.canBeTouched;
    if (minHitDistance[hand] < (useTouchThresh ? max_touch_distance : max_aim_distance))
    {
      auto &sis = spatialInteractionState;
      sis.hitDistances[hand] = minHitDistance[hand];
      sis.closestHitPanelIdxs[hand] = closestHitPanelIdxs[hand];
      if (sis.wasVrSurfaceHit(hand))
        sis.hitPos[hand] = vrSurfaceHitUvs[hand];
      else if (sis.wasPanelHit(hand))
      {
        int closestPanelIdx = sis.closestHitPanelIdxs[hand];
        auto closestPanelIt = panels.find(closestPanelIdx);
        G_ASSERT(closestPanelIt != panels.end());
        sis.hitPos[hand] = uv_to_panel_pixels(*closestPanelIt->second, panelHitUvs[hand]);
      }
    }
  }
}


int GuiScene::onVrInputEvent(InputEvent event, int hand, int prev_result)
{
  G_UNUSED(prev_result);

  ApiThreadCheck atc(this);

  G_ASSERT_RETURN(hand >= 0 && hand < NUM_VR_POINTERS, 0);

  int result = 0;
  if (status.sceneIsBroken || !isInputActive())
    return result;

  if (event == INP_EV_PRESS)
    vrActiveHand = hand;

  auto &sis = spatialInteractionState;
  const Point2 &cursorLocation = sis.hitPos[hand];

  if (hand == vrActiveHand && sis.wasVrSurfaceHit(vrActiveHand) &&
      inputStack.hitTest(cursorLocation, Behavior::F_HANDLE_MOUSE, Element::F_STOP_MOUSE))
    result = onMouseEventInternal(event, DEVID_VR, 0, cursorLocation.x, cursorLocation.y, 0, 0);

  VrPointer &vrPtr = vrPointers[hand];
  int closestPanelIdx = spatialInteractionState.closestHitPanelIdxs[hand];
  auto closestPanelIt = panels.find(closestPanelIdx);
  PanelData *closestPanelData = closestPanelIt != panels.end() ? closestPanelIt->second.get() : nullptr;
  if (vrPtr.activePanel != closestPanelData)
  {
    if (vrPtr.activePanel)
    {
      Panel *panel = vrPtr.activePanel->panel.get();
      if (const auto touch = panel->touchPointers.get(hand))
        onPanelTouchEvent(panel, hand, INP_EV_RELEASE, touch->pos.x, touch->pos.y);
      vrPtr.activePanel->setCursorStatus(hand, false);
      int resFlags = vrPtr.activePanel->panel->etree.deactivateInput(DEVID_VR, hand);
      if (resFlags & R_REBUILD_RENDER_AND_INPUT_LISTS)
        vrPtr.activePanel->panel->rebuildStacks();
    }
    vrPtr.activePanel = closestPanelData;
    if (vrPtr.activePanel)
      vrPtr.activePanel->setCursorStatus(hand, true);
  }

  if (closestPanelData)
  {
    Panel *panel = closestPanelData->panel.get();
    G_ASSERT(panel);
    // The closest hit panel can only be touched or pointed at (see how hit detection is performed).
    if (panel->spatialInfo.canBeTouched)
    { // If we get here, we have already detected a touch in this frame.
      const auto existingTouch = panel->touchPointers.get(hand);
      float curDist = sis.hitDistances[hand].value();
      bool canRegisterTouch = existingTouch || (curDist > 0.f && curDist < new_touch_threshold); // Avoid double activation
      bool touchNow = canRegisterTouch && panel->inputStack.hitTest(cursorLocation, Behavior::F_HANDLE_TOUCH, Element::F_STOP_MOUSE);
      sis.isHandTouchingPanel[hand] = touchNow;
      if (existingTouch && touchNow)
        result = onPanelTouchEvent(panel, hand, INP_EV_POINTER_MOVE, cursorLocation.x, cursorLocation.y);
      else if (touchNow)
        result = onPanelTouchEvent(panel, hand, INP_EV_PRESS, cursorLocation.x, cursorLocation.y);
      // else if (existingTouch) - Release is handled above, when we're no longer touching old activePanel, once.
    }
    else if (panel->inputStack.hitTest(cursorLocation, Behavior::F_HANDLE_MOUSE, Element::F_STOP_MOUSE))
      result = onPanelMouseEvent(panel, hand, event, DEVID_VR, 0, cursorLocation.x, cursorLocation.y, 0);
  }

  return result;
}


void GuiScene::setVrStickScroll(int hand, const Point2 &scroll)
{
  ApiThreadCheck atc(this);
  G_ASSERT_RETURN(hand >= 0 && hand < NUM_VR_POINTERS, );
  vrPointers[hand].stickScroll = scroll;
}


void GuiScene::spawnDebugRenderBox(const BBox2 &box, E3DCOLOR fillColor, E3DCOLOR borderColor, float life_time)
{
  debugRenderBoxes.push_back(DebugRenderBox{box, fillColor, borderColor, life_time, life_time});
}


void GuiScene::updateDebugRenderBoxes(float dt)
{
  for (int i = int(debugRenderBoxes.size()) - 1; i >= 0; --i)
  {
    debugRenderBoxes[i].timeLeft -= dt;
    if (debugRenderBoxes[i].timeLeft < 0)
      debugRenderBoxes.erase(debugRenderBoxes.begin() + i);
  }
}


void GuiScene::drawDebugRenderBoxes()
{
  for (DebugRenderBox &box : debugRenderBoxes)
  {
    float k = box.timeLeft / box.lifeTime;
    k = (1 - 2 * k);
    k = k * k;
    k = k * k * k;
    k = 1.0f - k; // apply easing
    k = clamp(k, 0.0f, 1.0f);

    guiContext->set_color(e3dcolor_lerp(0, box.fillColor, k));
    guiContext->render_box(box.box.left(), box.box.top(), box.box.right(), box.box.bottom());

    guiContext->set_color(e3dcolor_lerp(0, box.borderColor, k));
    guiContext->render_frame(box.box.left(), box.box.top(), box.box.right(), box.box.bottom(), 2);
  }
}


Point2 GuiScene::getVrStickState(int hand)
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


bool GuiScene::getForcedCursorMode(bool &out_value)
{
  if (forcedCursorMode.IsNull())
    return false;
  const HSQOBJECT &ho = forcedCursorMode.GetObject();
  out_value = sq_objtobool(&ho);
  return true;
}


bool GuiScene::isCursorForcefullyEnabled()
{
  bool val = false;
  return getForcedCursorMode(val) && val;
}

bool GuiScene::isCursorForcefullyDisabled()
{
  bool val = false;
  return getForcedCursorMode(val) && !val;
}


SQInteger GuiScene::force_cursor_active(HSQUIRRELVM vm)
{
  GuiScene *scene = GuiScene::get_from_sqvm(vm);
  Sqrat::Object newMode = Sqrat::Var<Sqrat::Object>(vm, 2).value;

  if (are_sq_obj_equal(vm, scene->forcedCursorMode, newMode))
    return 0;

  scene->forcedCursorMode = newMode;
  scene->updateActiveCursor();
  scene->applyInputActivityChange();

  return 0;
}


Element *GuiScene::traceInputHit(InputDevice device, Point2 pos) { return Element::trace_input_stack(this, inputStack, device, pos); }


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
    .Const("PANEL_RENDER_CAST_SHADOW", int(darg_panel_renderer::RenderFeatures::CastShadow))
    .Const("PANEL_RENDER_OPAQUE", int(darg_panel_renderer::RenderFeatures::Opaque))
    .Const("PANEL_RENDER_ALWAYS_ON_TOP", int(darg_panel_renderer::RenderFeatures::AlwaysOnTop)); //-V1071

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
    .SquirrelFunc("getCompAABBbyKey", get_comp_aabb_by_key, 2)
    .SquirrelFunc("setConfigProps", set_config_props, 2, ".t")
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
    .SquirrelFunc("forceCursorActive", &GuiScene::force_cursor_active, 2, "x b|o")
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
    ///@var clickPriority
    V(actionClickByBehavior)
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
