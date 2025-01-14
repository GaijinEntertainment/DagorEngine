// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_guiScene.h>
#include <daRg/dag_events.h>

#include "component.h"
#include "sceneConfig.h"
#include "elementTree.h"
#include "renderList.h"
#include "yuvRenderer.h"
#include "inputStack.h"
#include "scriptBinding.h"
#include "gamepadCursor.h"
#include "hotkeys.h"
#include <daRg/dag_joystick.h>
#include "panel.h"
#include "pointerPosition.h"
#include "sceneStatus.h"
#include "touchPointers.h"

#include <drv/hid/dag_hiDecl.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <gui/dag_stdGuiRender.h>
#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <osApiWrappers/dag_critSec.h>

class SqModules;
class TextureIDHolder;

namespace sqfrp
{
class BaseObservable;
class ScriptValueObservable;
class IStateWatcher;
class ObservablesGraph;
} // namespace sqfrp


namespace darg
{

class Component;
class Element;
class RenderObject;
class JoystickAxisObservable;
class StringKeys;
class Cursor;
class Profiler;
class Timer;
class Panel;
struct DebugRenderBox;
class DasScriptsData;


struct VrPointer
{
  PanelData *activePanel = nullptr;
  Point2 stickScroll = Point2(0, 0);
};


class GuiScene final : public IGuiScene, public IEventList, public IWndProcComponent
{
public:
  enum
  {
    EVENT_BREAK = 0,
    EVENT_CONTINUE = 1
  };

public:
  GuiScene(StdGuiRender::GuiContext *gui_ctx);
  ~GuiScene();

  virtual void setCallback(IGuiSceneCallback *cb) override { guiSceneCb = cb; }
  virtual void initDasEnvironment(TGuiInitDas init_callback) override;
  virtual void shutdownDasEnvironment() override;

  // Need this to perform additional binding
  virtual HSQUIRRELVM getScriptVM() const override { return sqvm; }

  virtual void runScriptScene(const char *fn) override;
  virtual void reloadScript(const char *fn) override;

  virtual void update(float dt) override;
  virtual void mainThreadBeforeRender() override;
  virtual void renderThreadBeforeRender() override;
  virtual void buildRender() override;
  virtual void buildPanelRender(int panel_idx) override;
  virtual void flushRender() override;
  virtual void flushPanelRender() override;
  virtual void refreshGuiContextState() override;

  // returns a combination of BehaviorResult flags
  virtual int onMouseEvent(InputEvent event, int data, short mx, short my, int buttons, int prev_result = 0) override;
  // returns a combination of BehaviorResult flags
  virtual int onTouchEvent(InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int prev_result = 0);
  // returns a combination of BehaviorResult flags
  virtual int onKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc = 0, int prev_result = 0) override;
  // returns a combination of BehaviorResult flags
  virtual int onServiceKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc = 0, int prev_result = 0) override;
  // returns a combination of BehaviorResult flags
  virtual int onJoystickBtnEvent(HumanInput::IGenJoystick *joy, InputEvent event, int key_idx, int dev_n,
    const HumanInput::ButtonBits &buttons, int prev_result = 0) override;
  // returns a combination of BehaviorResult flags
  virtual int onVrInputEvent(InputEvent event, int hand, int prev_result = 0) override;

  virtual void setVrStickScroll(int hand, const Point2 &scroll) override;

  virtual void onKeyboardLayoutChanged(const char *layout) override;
  virtual void onKeyboardLocksChanged(unsigned locks) override;

  virtual Element *traceInputHit(InputDevice device, Point2 pos) override;

  virtual void queueScriptHandler(BaseScriptHandler *h) override;
  virtual void callScriptHandlers(bool is_shutdown = false) override final;

  virtual void setKbFocus(Element *elem) override;

  virtual void setSceneInputActive(bool active) override;
  virtual void skipRender() override;

  virtual IWndProcComponent::RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override;

  void invalidateElement(Element *elem);

  void rebuildInvalidatedParts();
  void recalcLayoutFromRoots(dag::Span<Element *> fixed_size_roots, dag::Span<Element *> size_roots, dag::Span<Element *> flow_roots);
  void onElementDetached(Element *elem);
  void validateAfterRebuild(Element *elem);

  void rebuildElemStacks(bool refresh_hotkeys_nav);
  void refreshHotkeysNav();
  virtual void notifyInputConsumersCallback() override;

  static GuiScene *get_from_sqvm(HSQUIRRELVM vm);
  static GuiScene *get_from_elem(const Element *elem);

  virtual SqModules *getModuleMgr() const override { return moduleMgr.get(); }
  virtual StringKeys *getStringKeys() const override { return stringKeys; }
  virtual StdGuiRender::GuiContext *getGuiContext() const override { return guiContext; }

  const Hotkeys &getHotkeys() const { return hotkeys; }
  ElementTree *getElementTree() { return &etree; }
  virtual IEventList &getEvents() override { return *this; }

  void errorMessage(const char *msg);
  void errorMessageWithCb(const char *msg);

  int getFramesCount() const { return framesCount; }
  int getActsCount() const { return actsCount; }

  virtual bool hasInteractiveElements() override;
  virtual bool hasInteractiveElements(int bhv_flags) override;
  int calcInteractiveFlags();

  // for WT mouse mode management
  virtual bool hasActiveCursor() override { return activeCursor != nullptr; }
  virtual bool getForcedCursorMode(bool &out_value) override;

  virtual void activateProfiler(bool on) override;
  virtual Profiler *getProfiler() override { return profiler.get(); }
  IGuiSceneCallback *getCb() const { return guiSceneCb; }

  Point2 getMousePos() const { return pointerPos.mousePos; }
  void setMousePos(const Point2 &p, bool reset_target);
  void moveMouseCursorToElem(Element *elem, bool use_transform);
  void moveMouseCursorToElemBox(Element *elem, const BBox2 &bbox, bool jump);

  InputStack &getInputStack() { return inputStack; }

  void updateHover();

  virtual bool sendEvent(const char *id, const Sqrat::Object &data) override;
  virtual void postEvent(const char *id, const Json::Value &data) override;
  void processPostedEvents();

  SceneConfig *getConfig() { return &config; }

  virtual void setSceneErrorRenderMode(SceneErrorRenderMode mode) override;

  Sqrat::Array getAllObservables();

  void addCursor(Cursor *c);
  void removeCursor(Cursor *c);

  void onPictureLoaded(const Picture *pic);

  static HumanInput::IGenJoystick *getJoystick();

  bool useCircleAsActionButton() const;
  bool isInputActive() const { return isInputEnabledByHost && isInputEnabledByScript; }

  void setXmbFocus(Element *elem);
  bool trySetXmbFocus(Element *elem);
  BBox2 calcXmbViewport(Element *node, Element **nearest_xmb_viewport) const;

  virtual void ignoreDeviceCursorPos(bool on) override { shouldIgnoreDeviceCursorPos = on; }

  void doJoystickScroll(const Point2 &delta);
  Point2 getVrStickState(int hand);

  IPoint2 getDeviceScreenSize() const { return deviceScreenSize; }

  void renderPanelTo(int panel_idx, BaseTexture *dst) override;
  void updateSpatialElements(const VrSceneData &vr_scene) override;
  void refreshVrCursorProjections() override;
  bool hasAnyPanels() override;
  bool worldToPanelPixels(int panel_idx, const Point3 &world_target_pos, const Point3 &world_cam_pos, Point2 &out_panel_pos) override;

  const eastl::vector_map<int, eastl::unique_ptr<PanelData>> &getPanels() const { return panels; }


  const darg::PanelSpatialInfo *getPanelSpatialInfo(int id) const override
  {
    if (const auto &found = panels.find(id); found != panels.end())
      return &found->second->panel->spatialInfo;
    return nullptr;
  }


  bool haveActiveCursorOnPanels() const override;
  bool isAnyPanelPointedAtWithHand(int hand) const override { return spatialInteractionState.wasPanelHit(hand); }
  bool isAnyPanelTouchedWithHand(int hand) const override { return spatialInteractionState.isHandTouchingPanel[hand]; }

  void spawnDebugRenderBox(const BBox2 &box, E3DCOLOR fillColor, E3DCOLOR borederColor, float life_time);

private:
  void blurWorld();

  void clear(bool exiting);
  void clearCursors(bool exiting);

  void initScript();
  void shutdownScript();
  void bindScriptClasses();
  void saveToSqVm();

  Point2 cursorPosition(Cursor *cursor) const;
  void updateActiveCursor();
  bool updateHotkeys();
  void doUpdateJoystickAxesObservables();
  void updateJoystickAxesObservables()
  {
    if (!joyAxisObservables.empty())
      doUpdateJoystickAxesObservables();
  }

  enum class FlushPart
  {
    MainScene,
    Panel
  };
  void flushRenderImpl(FlushPart part);

  void renderProfileStats();
  void renderXmbDebug();
  void renderDirPadNavDebug();
  void renderInputStackDebug();
  void renderInternalCursor();

  static SQInteger set_timer(HSQUIRRELVM vm, bool periodic, bool reuse);
  void clearTimer(const Sqrat::Object &id_);
  void updateTimers(float dt);

  static HumanInput::IGenPointing *getMouse();
  void dirPadNavigate(Direction dir);
  bool xmbNavigate(Direction dir); //< return true if event was processed and consumed
  bool xmbGridNavigate(Direction dir);
  void moveMouseToTarget(float dt);
  void checkCursorHotspotUpdate(Element *elem_rebuilt);
  bool scrollIntoView(Element *elem, const BBox2 &viewport, const Point2 &max_scroll, const Element *scroll_root);
  bool scrollIntoViewCenter(Element *elem, const BBox2 &viewport, const Point2 &max_scroll, const Element *scroll_root);
  bool hierScroll(Element *elem, Point2 delta, const Element *scroll_root);
  void scrollToXmbFocus(float dt);
  void validateOverlaidXmbFocus();
  void tryRestoreSavedXmbFocus();
  bool isXmbNodeInputCovered(Element *node) const;
  void rebuildXmb();
  void rebuildXmb(Element *node, Element *xmb_parent);

  int handleMouseClick(InputEvent event, InputDevice device, int btn_idx, short mx, short my, int buttons, int accum_res);
  int onMouseEventInternal(InputEvent event, InputDevice device, int data, short mx, short my, int buttons, int accum_res);
  int onPanelMouseEvent(Panel *panel, int hand, InputEvent event, InputDevice device, int button_id, short mx, short my, int buttons,
    int prev_result = 0);
  int onPanelTouchEvent(Panel *panel, int hand, InputEvent event, short tx, short ty, int prev_result = 0);

  template <void (ElementTree::*method)(const Sqrat::Object &)>
  static SQInteger call_anim_method(HSQUIRRELVM vm);

  static SQInteger set_kb_focus_impl(HSQUIRRELVM vm, bool capture);
  static SQInteger set_kb_focus(HSQUIRRELVM vm);
  static SQInteger capture_kb_focus(HSQUIRRELVM vm);
  static SQInteger set_update_handler(HSQUIRRELVM vm);
  static SQInteger set_shutdown_handler(HSQUIRRELVM vm);
  static SQInteger set_hotkeys_nav_handler(HSQUIRRELVM vm);
  static SQInteger move_mouse_cursor(HSQUIRRELVM vm);
  static SQInteger add_panel(HSQUIRRELVM vm);
  static SQInteger remove_panel(HSQUIRRELVM vm);
  static SQInteger mark_panel_dirty(HSQUIRRELVM vm);
  static SQInteger force_cursor_active(HSQUIRRELVM vm);

  sqfrp::ScriptValueObservable *getCursorPresentObservable() { return cursorPresent.get(); }
  sqfrp::ScriptValueObservable *getCursorOverScrollObservable() { return cursorOverStickScroll.get(); }
  sqfrp::ScriptValueObservable *getCursorOverClickableObservable() { return cursorOverClickable.get(); }
  sqfrp::ScriptValueObservable *getHoveredClickableInfoObservable() { return hoveredClickableInfo.get(); }
  JoystickAxisObservable *getJoystickAxis(int axis);
  sqfrp::ScriptValueObservable *getKeyboardLayoutObservable() { return keyboardLayout.get(); }
  sqfrp::ScriptValueObservable *getKeyboardLocksObservable() { return keyboardLocks.get(); }
  sqfrp::ScriptValueObservable *getIsXmbModeOn() { return isXmbModeOn.get(); }
  sqfrp::ScriptValueObservable *getUpdateCounterObservable() { return updateCounter.get(); }

  void createNativeWatches();
  void destroyNativeWatches();

  void logPrevReloadObservableLeftovers();

  void updateDebugRenderBoxes(float dt);
  void drawDebugRenderBoxes();

  void queryCurrentScreenResolution();

  void scriptSetSceneInputActive(bool active);
  void applyInputActivityChange();

  bool isCursorForcefullyEnabled();
  bool isCursorForcefullyDisabled();

  void requestHoverUpdate();
  void applyPendingHoverUpdate();

public:
  eastl::unique_ptr<sqfrp::ObservablesGraph> frpGraph;
  eastl::unique_ptr<sqfrp::ScriptValueObservable> isXmbModeOn;
  bool hotkeysStackModified = false;

  SceneConfig config;
  YuvRenderer yuvRenderer;

  bool needToDiscardPictures = false;
  bool isRebuildingInvalidatedParts = false;

  static constexpr int panel_render_buffer_index = 2;

  Sqrat::Table canvasApi;

  eastl::unique_ptr<DasScriptsData> dasScriptsData;

private:
  StdGuiRender::GuiContext *guiContext = nullptr;
  bool ownGuiContext = false;

  ElementTree etree;
  RenderList renderList;
  InputStack inputStack;
  InputStack cursorStack;
  InputStack eventHandlersStack;

  GamepadCursor gamepadCursor;

  PointerPosition pointerPos;

  eastl::vector_set<HotkeyButton> pressedClickButtons;

  HSQUIRRELVM sqvm = nullptr;

  eastl::unique_ptr<SqModules> moduleMgr;

  GuiSceneScriptHandlers sceneScriptHandlers;
  StringKeys *stringKeys = nullptr;

  Cursor *activeCursor = nullptr;
  Sqrat::Object forcedCursorMode; // null|false|true
  eastl::unique_ptr<sqfrp::ScriptValueObservable> cursorPresent, cursorOverStickScroll, cursorOverClickable;
  eastl::unique_ptr<sqfrp::ScriptValueObservable> hoveredClickableInfo;
  eastl::unique_ptr<sqfrp::ScriptValueObservable> keyboardLayout, keyboardLocks;
  eastl::unique_ptr<sqfrp::ScriptValueObservable> updateCounter;
  dag::Vector<JoystickAxisObservable> joyAxisObservables;
  eastl::vector_set<Cursor *> allCursors;

  dag::Vector<Element *> invalidatedElements;
  dag::Vector<Element *> layoutRecalcFixedSizeRoots;
  dag::Vector<Element *> layoutRecalcSizeRoots;
  dag::Vector<Element *> layoutRecalcFlowRoots;
  dag::Vector<Element *> allRebuiltElems;

  Tab<BaseScriptHandler *> scriptHandlersQueue;
  Tab<Element *> keptXmbFocus;

  bool isClearing = false;

  int framesCount = 0;
  int actsCount = 0;

  IGuiSceneCallback *guiSceneCb = nullptr;

  Hotkeys hotkeys;
  JoystickHandler joystickHandler;

  eastl::unique_ptr<Profiler> profiler;

  Tab<Timer *> timers;
  bool lockTimersList = false; // lock for deletion/modification while iterating

  int lastRenderTimestamp = 0;

  int lastDirPadNavDir = -1;
  float dirPadNavRepeatTimeout = 0.0f;

  SceneStatus status;

  bool isInputEnabledByHost = true;
  bool isInputEnabledByScript = true;

  bool shouldIgnoreDeviceCursorPos = false;

  IPoint2 deviceScreenSize = IPoint2(0, 0);

  friend class GamepadCursor;

  enum class UpdateHoverRequestState
  {
    None,
    WaitingForRender,
    NeedToUpdate
  };

  UpdateHoverRequestState updateHoverRequestState = UpdateHoverRequestState::None;

  eastl::vector_map<int, eastl::unique_ptr<PanelData>> panels;

  static constexpr int NUM_VR_POINTERS = 2;
  struct SpatialInteractionState
  {
    eastl::optional<float> hitDistances[NUM_VR_POINTERS];
    int closestHitPanelIdxs[NUM_VR_POINTERS] = {-1, -1};
    Point2 hitPos[NUM_VR_POINTERS] = {{-1.f, -1.f}, {-1.f, -1.f}};
    bool isHandTouchingPanel[NUM_VR_POINTERS] = {false, false};

    bool wasVrSurfaceHit(int hand) const { return hitDistances[hand].has_value() && closestHitPanelIdxs[hand] < 0; }
    bool wasPanelHit(int hand) const { return hitDistances[hand].has_value() && closestHitPanelIdxs[hand] >= 0; }
    void forgetPanel(int panel_idx)
    {
      for (int i = 0; i < NUM_VR_POINTERS; ++i)
        if (closestHitPanelIdxs[i] == panel_idx)
        {
          hitDistances[i].reset();
          hitPos[i].set(-1.f, -1.f);
          closestHitPanelIdxs[i] = -1;
        }
    }
  };
  SpatialInteractionState spatialInteractionState;
  void updateSpatialInteractionState(const VrSceneData &vr_scene);

  VrPointer vrPointers[NUM_VR_POINTERS];
  int vrActiveHand = 1; //< temporary for mapping to mouse

  TouchPointers touchPointers;

  bool panelBufferInitialized = false;
  void ensurePanelBufferInitialized();

  eastl::vector<DebugRenderBox> debugRenderBoxes;

  mutable volatile int apiThreadId = 0;
  mutable void *threadCheckCallStack[32];
  friend class ApiThreadCheck;

  bool dbgIsInUpdateHover = false;
  bool dbgIsInTryRestoreSavedXmbFocus = false;

  eastl::vector<eastl::pair<eastl::string, Json::Value>> postedEventsQueue, workingPostedEventsQueue;
  WinCritSec postedEventsQueueCs;
};

} // namespace darg
