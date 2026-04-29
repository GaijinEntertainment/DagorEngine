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
#include "screen.h"
#include "mousePointer.h"
#include "sceneStatus.h"
#include "touchPointers.h"
#include "kbFocus.h"

#include <drv/hid/dag_hiDecl.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <gui/dag_stdGuiRender.h>
#include <EASTL/vector_set.h>
#include <EASTL/unique_ptr.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <osApiWrappers/dag_critSec.h>
#include <quirrel/frp/dag_frp.h>

class SqModules;
class TextureIDHolder;

namespace sqfrp
{
class IStateWatcher;
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
class Screen;
class DasScriptsData;


struct VrPointer
{
  Panel *activePanel = nullptr;
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
  GuiScene();
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
  virtual int onVrInputEvent(InputEvent event, int hand, int button_id, int prev_result = 0) override;

  virtual void setVrStickScroll(int hand, const Point2 &scroll) override;

  virtual void onKeyboardLayoutChanged(const char *layout) override;
  virtual void onKeyboardLocksChanged(unsigned locks) override;

  virtual Element *traceInputHit(ElementTree *etree, InputDevice device, Point2 pos) override;

  virtual void queueScriptHandler(BaseScriptHandler *h) override;
  virtual void callScriptHandlers(bool is_shutdown = false) override final;

  virtual void forceFrpUpdateDeferred() override final;

  virtual void inspectSceneTree(eastl::string &out, int max_depth, const char *filter_text, bool include_hidden, bool skip_non_visual,
    int max_elements) override;
  virtual void inspectElementsAtPos(eastl::string &out, int x, int y) override;
  virtual void findElementsByText(eastl::string &out, const char *text, bool case_sensitive, int max_results) override;

  virtual void setSceneInputActive(bool active) override;
  virtual void skipRender() override;

  virtual IWndProcComponent::RetCode process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result) override;

  void invalidateElement(Element *elem);
  void deferredRecalLayout(Element *elem);

  void rebuildInvalidatedParts();
  void recalcLayoutFromRoots(dag::Span<Element *> fixed_size_roots, dag::Span<Element *> size_roots, dag::Span<Element *> flow_roots);
  void onElementDetached(Element *elem);
  void validateAfterRebuild(Element *elem);

  void rebuildStacksAndNotify(Screen *screen, bool refresh_hotkeys_nav, bool update_global_hover);
  void rebuildAllScreensStacksAndNotify(bool refresh_hotkeys_nav, bool update_global_hover);
  void applyInteractivityChange(int prev_iflags, bool refresh_hotkeys_nav, bool update_global_hover);
  void refreshHotkeysNav();
  virtual void notifyInputConsumersCallback() override;

  static GuiScene *get_from_sqvm(HSQUIRRELVM vm);
  static GuiScene *get_from_elem(const Element *elem);

  virtual SqModules *getModuleMgr() const override { return moduleMgr.get(); }
  virtual StringKeys *getStringKeys() const override { return stringKeys; }
  virtual StdGuiRender::GuiContext *getGuiContext() const override { return guiContext.get(); }

  const Hotkeys &getHotkeys() const { return hotkeys; }
  virtual IEventList &getEvents() override { return *this; }

  void errorMessage(const char *msg);
  void errorMessageWithCb(const char *msg);

  int getFramesCount() const { return framesCount; }
  int getActsCount() const { return actsCount; }

  virtual bool hasInteractiveElements() const override;
  virtual bool hasInteractiveElements(int bhv_flags) const override;
  int calcInteractiveFlags() const;

  // for WT mouse mode management
  virtual bool hasActiveCursor() const override { return activeCursor != nullptr; }
  virtual bool getForcedCursorMode(bool &out_value) const override;

  virtual void activateProfiler(bool on) override;
  virtual Profiler *getProfiler() const override { return profiler.get(); }
  IGuiSceneCallback *getCb() const { return guiSceneCb; }

  CursorPosState &activePointerState();
  const CursorPosState &activePointerState() const;
  Point2 activePointerPos() const;
  void setMousePos(const Point2 &p, bool reset_target);
  void moveActiveCursorToElem(Element *elem, bool use_transform);
  void moveActiveCursorToElemBox(Element *elem, const BBox2 &bbox, bool jump);

  void updateGlobalHover();

  virtual bool sendEvent(const char *id, const Sqrat::Object &data) override;
  virtual void postEvent(const char *id, const Json::Value &data) override;
  void processPostedEvents();

  SceneConfig *getConfig() { return &config; }

  virtual void setSceneErrorRenderMode(SceneErrorRenderMode mode) override;
  virtual bool getErrorText(eastl::string &text) const override;

  Sqrat::Array getAllObservables() const;

  void addCursor(Cursor *c);
  void removeCursor(Cursor *c);

  void onPictureLoaded(const Picture *pic);
  virtual void setPictureDiscardAllowed(bool discardAllowed) override { pictureDiscardAllowed = discardAllowed; }

  HumanInput::IGenJoystick *getJoystick(int16_t ord_id = -1) const override;

  bool useCircleAsActionButton() const;
  bool isInputActive() const { return isInputEnabledByHost && isInputEnabledByScript; }

  void doSetXmbFocus(Element *elem);
  void trySetXmbFocus(Element *elem);

  Point2 getVrStickState(int hand) const;

  IPoint2 getDeviceScreenSize() const { return deviceScreenSize; }

  void renderPanelTo(int panel_idx, BaseTexture *dst) override;
  void updateSpatialElements(const SpatialSceneData &spatial_scene) override;
  void refreshVrCursorProjections() override;
  bool hasAnyPanels() const override;
  bool worldToPanelPixels(int panel_idx, const Point3 &world_target_pos, const Point3 &world_cam_pos, Point2 &out_panel_pos) override;

  const eastl::vector_map<int, eastl::unique_ptr<Panel>> &getPanels() const { return panels; }


  const darg::PanelSpatialInfo *getPanelSpatialInfo(int id) const override
  {
    if (const auto &found = panels.find(id); found != panels.end())
      return &found->second->spatialInfo;
    return nullptr;
  }


  bool haveActiveCursorOnPanels() const override;
  bool isAnyPanelPointedAtWithHand(int hand) const override { return spatialInteractionState.wasPanelHit(hand); }
  bool isAnyPanelTouchedWithHand(int hand) const override { return spatialInteractionState.isHandTouchingPanel[hand]; }

  Screen *createGuiScreen(int id);
  Screen *getGuiScreen(int id) const;
  int getGuiScreenId(Screen *screen) const;
  bool destroyGuiScreen(int id);
  bool destroyGuiScreen(Screen *screen);
  bool setFocusedScreenById(int id);
  bool setFocusedScreen(Screen *screen);
  Screen *getFocusedScreen() const { return focusedScreen; }
  static SQInteger sqGetFocusedScreen(HSQUIRRELVM vm);

private:
  void blurWorld();

  void clear(bool exiting);
  void clearCursors(bool exiting);

  void initScript();
  void shutdownScript();
  void bindScriptClasses();
  void saveToSqVm();

  void updateGlobalActiveCursor();
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
  void renderInternalCursor();

  static SQInteger set_timer(HSQUIRRELVM vm, bool periodic, bool reuse);
  void clearTimer(const Sqrat::Object &id_);
  void updateTimers(float dt);

  void dirPadNavigate(Direction dir);
  void repeatDirPadNav(float dt);
  bool xmbNavigate(Direction dir); //< return true if event was processed and consumed
  void checkCursorHotspotUpdate(Element *elem_rebuilt);
  void doJoystickScroll(const Point2 &delta);
  void updatePointers(float dt, bool &hover_dirty);
  void updateGamepadCursor(float dt, bool &hover_dirty);
  void requestActivePointerNextFramePos(const Point2 &p);
  void requestActivePointerTargetPos(const Point2 &p);
  void updateVrStickScroll(float dt);
  bool scrollIntoView(Element *elem, const BBox2 &viewport, const Point2 &max_scroll, const Element *scroll_root);
  bool scrollIntoViewCenter(Element *elem, const BBox2 &viewport, const Point2 &max_scroll, const Element *scroll_root);
  bool hierScroll(Element *elem, Point2 delta, const Element *scroll_root);
  void scrollToXmbFocus(float dt);
  void validateOverlaidXmbFocus();
  void tryRestoreSavedXmbFocus();

  int handleMouseClick(Screen *screen, InputEvent event, InputDevice device, int btn_idx, Point2 pos, int accum_res);
  int onMouseEventInternal(Screen *screen, InputEvent event, InputDevice device, int data, Point2 pos, int buttons, int accum_res);
  int onPanelMouseEvent(Panel *panel, int hand, InputEvent event, InputDevice device, int button_id, short mx, short my,
    int prev_result = 0);
  int onPanelTouchEvent(Panel *panel, int hand, InputEvent event, short tx, short ty, int prev_result = 0);

  template <void (ElementTree::*method)(const Sqrat::Object &)>
  static SQInteger call_anim_method(HSQUIRRELVM vm);
  static SQInteger anim_pause(HSQUIRRELVM vm);

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
  static SQInteger get_comp_aabb_by_key(HSQUIRRELVM vm);

  sqfrp::NativeWatched *getCursorPresentObservable() { return &cursorPresent; }
  sqfrp::NativeWatched *getCursorOverScrollObservable() { return &cursorOverStickScroll; }
  sqfrp::NativeWatched *getCursorOverClickableObservable() { return &cursorOverClickable; }
  sqfrp::NativeWatched *getHoveredClickableInfoObservable() { return &hoveredClickableInfo; }
  JoystickAxisObservable *getJoystickAxis(int axis);
  sqfrp::NativeWatched *getKeyboardLayoutObservable() { return &keyboardLayout; }
  sqfrp::NativeWatched *getKeyboardLocksObservable() { return &keyboardLocks; }
  sqfrp::NativeWatched *getIsXmbModeOn() { return &isXmbModeOn; }
  sqfrp::NativeWatched *getUpdateCounterObservable() { return &updateCounter; }

  void createNativeWatches();
  void destroyNativeWatches();

  void queryCurrentScreenResolution();

  void scriptSetSceneInputActive(bool active);
  void applyInputActivityChange();

  bool isCursorForcefullyEnabled() const;
  bool isCursorForcefullyDisabled() const;

  bool doesScreenAcceptInput(const Screen *screen) const;

  void requestHoverUpdate();
  void applyPendingHoverUpdate();
  void applyDeferredRecalLayout();

public:
  eastl::unique_ptr<sqfrp::ObservablesGraph> frpGraph;
  sqfrp::NativeWatched isXmbModeOn;
  bool hotkeysStackModified = false;

  SceneConfig config;
  YuvRenderer yuvRenderer;

  bool needToDiscardPictures = false;
  bool isRebuildingInvalidatedParts = false;

  static constexpr int panel_render_buffer_index = 2;

  Sqrat::Table canvasApi;

  eastl::unique_ptr<DasScriptsData> dasScriptsData;

  KbFocus kbFocus;
  Element *xmbFocus = nullptr;

  TMatrix lastCameraTransform = TMatrix::IDENT;
  Frustum lastCameraFrustum;

  const GamepadCursor &getGamepadCursor() const { return gamepadCursor; }
  const Cursor *getActiveCursor() const { return activeCursor; }

private:
  eastl::unique_ptr<StdGuiRender::GuiContext> guiContext;

  struct ScreenDeleter
  {
    void operator()(Screen *s) const { delete s; }
  };

  eastl::vector_map<int, eastl::unique_ptr<Screen, ScreenDeleter>> screens;
  Screen *focusedScreen = nullptr;

  // Devices for controlling the cursor (seamlessly switched by activity)
  MousePointer mousePointer;
  GamepadCursor gamepadCursor;
  InputDevice activePointingDevice = DEVID_MOUSE;

  eastl::vector_set<HotkeyButton> pressedClickButtons;

  HSQUIRRELVM sqvm = nullptr;

  eastl::unique_ptr<SqModules> moduleMgr;

  GuiSceneScriptHandlers sceneScriptHandlers;
  StringKeys *stringKeys = nullptr;

  Cursor *activeCursor = nullptr;
  Sqrat::Object forcedCursor; // null|false|true|Cursor(instance)
  sqfrp::NativeWatched cursorPresent;
  sqfrp::NativeWatched cursorOverStickScroll;
  sqfrp::NativeWatched cursorOverClickable;
  sqfrp::NativeWatched hoveredClickableInfo;
  sqfrp::NativeWatched keyboardLayout;
  sqfrp::NativeWatched keyboardLocks;
  sqfrp::NativeWatched updateCounter;
  dag::Vector<JoystickAxisObservable> joyAxisObservables;
  eastl::vector_set<Cursor *> allCursors;

  dag::Vector<Element *> invalidatedElements;
  dag::Vector<Element *> layoutRecalcFixedSizeRoots;
  dag::Vector<Element *> layoutRecalcSizeRoots;
  dag::Vector<Element *> layoutRecalcFlowRoots;
  dag::Vector<Element *> allRebuiltElems;
  eastl::vector_set<Element *> deferredRecalcLayout;

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

  int lastDirPadNavBtn = -1;
  float dirPadNavRepeatTimeout = 0.0f;

  SceneStatus status;

  bool isInputEnabledByHost = true;
  bool isInputEnabledByScript = true;

  IPoint2 deviceScreenSize = IPoint2(0, 0);

  enum class UpdateHoverRequestState
  {
    None,
    WaitingForRender,
    NeedToUpdate
  };

  UpdateHoverRequestState updateHoverRequestState = UpdateHoverRequestState::None;

  eastl::vector_map<int, eastl::unique_ptr<Panel>> panels;

  static constexpr int NUM_VR_POINTERS = SpatialSceneData::AimOrigin::Total;
  struct SpatialInteractionState
  {
    eastl::optional<float> hitDistances[NUM_VR_POINTERS];
    int closestHitPanelIdxs[NUM_VR_POINTERS] = {-1, -1, -1};
    Point2 hitPos[NUM_VR_POINTERS] = {{-1.f, -1.f}, {-1.f, -1.f}, {-1.f, -1.f}};
    bool isHandTouchingPanel[NUM_VR_POINTERS] = {false, false, false};

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
  void updateSpatialInteractionState(const SpatialSceneData &vr_scene);

  VrPointer vrPointers[NUM_VR_POINTERS];

  TouchPointers touchPointers;

  bool panelBufferInitialized = false;
  void ensurePanelBufferInitialized();

  mutable volatile int apiThreadId = 0;
  mutable void *threadCheckCallStack[32];
  friend class ApiThreadCheck;

  bool dbgIsInUpdateHover = false;
  bool dbgIsInTryRestoreSavedXmbFocus = false;

  eastl::vector<eastl::pair<eastl::string, Json::Value>> postedEventsQueue, workingPostedEventsQueue;
  WinCritSec postedEventsQueueCs;

  bool pictureDiscardAllowed = true;

  friend class Screen; // for localToDisplay/displayToLocal, to be removed later
};

} // namespace darg
