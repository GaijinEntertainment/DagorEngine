//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daRg/dag_inputIds.h>
#include <daRg/dag_guiConstants.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>
#include <humanInput/dag_hiPointingData.h>
#include <math/dag_TMatrix.h>
#include <math/dag_e3dColor.h>
#include <math/dag_frustum.h>
#include <EASTL/vector.h>
#include <EASTL/optional.h>
#include <3d/dag_resId.h>


typedef struct SQVM *HSQUIRRELVM;
class SqModules;
class String;


namespace HumanInput
{
class IGenJoystick;
class IGenPointing;
struct ButtonBits;
} // namespace HumanInput

namespace StdGuiRender
{
class GuiContext;
}


namespace darg
{

class StringKeys;
class Profiler;
class IEventList;
class Element;
class BaseScriptHandler;
struct IGuiScene;

enum SceneErrorRenderMode
{
  SEI_WARNING_ICON,
  SEI_WARNING_SINGLE_STRING,
  SEI_WARNING_MESSAGE,
  SEI_STOP_AND_SHOW_FULL_INFO,

#if _TARGET_PC
#if DAGOR_DBGLEVEL > 0
  SEI_DEFAULT = SEI_WARNING_MESSAGE
#else
  SEI_DEFAULT = SEI_WARNING_SINGLE_STRING
#endif
#else // console
#if DAGOR_DBGLEVEL > 0
  SEI_DEFAULT = SEI_WARNING_MESSAGE
#else
  SEI_DEFAULT = SEI_WARNING_ICON // just red box at right top of screen, to pass TRC & TCR
#endif
#endif
};


struct HotkeyButton
{
  InputDevice devId;
  unsigned int btnId;
  bool ckNot;

  HotkeyButton() {} //-V730
  HotkeyButton(InputDevice d, unsigned int b) : devId(d), btnId(b), ckNot(false) {}

  bool operator==(const HotkeyButton &hb) const { return devId == hb.devId && btnId == hb.btnId && ckNot == hb.ckNot; }
  bool operator!=(const HotkeyButton &hb) const { return devId != hb.devId || btnId != hb.btnId || ckNot != hb.ckNot; }
  bool operator<(const HotkeyButton &hb) const
  {
    if (ckNot != hb.ckNot)
      return ckNot;
    if (devId < hb.devId)
      return true;
    if (devId > hb.devId)
      return false;
    return btnId < hb.btnId;
  }
};


struct IGuiSceneCallback
{
  enum InteractiveFlags
  {
    IF_MOUSE = 0x01,
    IF_KEYBOARD = 0x02,
    IF_GAMEPAD_STICKS = 0x04,
    IF_DIRPAD_NAV = 0x08,
    IF_GAMEPAD_AS_MOUSE = 0x10
  };

  virtual void onScriptError(darg::IGuiScene * /*scene*/, const char * /*err_msg*/) {}
  virtual void onToggleInteractive(int /*interactive_flags IF_* */) {}
  virtual void onDismissError() {}
  virtual void onShutdownScript(HSQUIRRELVM) {}
  virtual bool useInputConsumersCallback() { return false; }
  virtual void onInputConsumersChange(dag::ConstSpan<HotkeyButton> /*device_buttons*/, dag::ConstSpan<String> /*event_hotkeys*/,
    bool /*consume_text_input*/)
  {}
};


struct IGuiScene
{
public:
  virtual ~IGuiScene() {}
  virtual void setCallback(IGuiSceneCallback *cb) = 0;

  // Need this to perform additional binding
  virtual HSQUIRRELVM getScriptVM() const = 0;
  virtual SqModules *getModuleMgr() const = 0;
  virtual StdGuiRender::GuiContext *getGuiContext() const = 0;

  virtual void runScriptScene(const char *fn) = 0;
  virtual void reloadScript(const char *fn) = 0;

  virtual void update(float dt) = 0;
  virtual void mainThreadBeforeRender() = 0;
  virtual void renderThreadBeforeRender() = 0;
  virtual void buildRender() = 0;
  virtual void buildPanelRender(int panel_idx) = 0;
  virtual void flushRender() = 0;
  virtual void refreshGuiContextState() = 0;

  // returns a combination of BehaviorResult flags
  virtual int onMouseEvent(InputEvent event, int data, short mx, short my, int buttons, int prev_result = 0) = 0;
  // returns a combination of BehaviorResult flags
  virtual int onTouchEvent(InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int prev_result = 0) = 0;
  // returns a combination of BehaviorResult flags
  virtual int onKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc = 0, int prev_result = 0) = 0;
  // returns a combination of BehaviorResult flags
  // Used for internal global hotkeys, call this before onKbdEvent
  virtual int onServiceKbdEvent(InputEvent event, int key_idx, int shifts, bool repeat, wchar_t wc = 0, int prev_result = 0) = 0;
  // returns a combination of BehaviorResult flags
  virtual int onJoystickBtnEvent(HumanInput::IGenJoystick *joy, InputEvent event, int key_idx, int dev_n,
    const HumanInput::ButtonBits &buttons, int prev_result = 0) = 0;

  virtual int onVrInputEvent(InputEvent event, int hand, int prev_result = 0) = 0;

  virtual void setVrStickScroll(int hand, const Point2 &scroll) = 0;

  virtual void queueScriptHandler(BaseScriptHandler *h) = 0;

  virtual void setKbFocus(Element *elem) = 0;

  virtual void onKeyboardLayoutChanged(const char *layout) = 0;
  virtual void onKeyboardLocksChanged(unsigned locks) = 0;

  virtual StringKeys *getStringKeys() const = 0;

  virtual bool hasInteractiveElements() = 0;
  virtual bool hasInteractiveElements(int bhv_flags) = 0;
  virtual bool hasActiveCursor() = 0; //< for WT mouse mode management

  virtual void activateProfiler(bool on) = 0;
  virtual Profiler *getProfiler() = 0;
  virtual IEventList &getEvents() = 0;
  virtual void setSceneErrorRenderMode(SceneErrorRenderMode mode) = 0;

  virtual void setSceneInputActive(bool active) = 0;
  virtual void notifyInputConsumersCallback() = 0;
  virtual void skipRender() = 0;

  // workaround for VR
  virtual void ignoreDeviceCursorPos(bool on) = 0;

  virtual bool haveActiveCursorOnPanels() const = 0;
  virtual eastl::optional<float> isAnyPanelPointedAtWithHand(int hand) const = 0;
  virtual bool isAnyPanelTouchedWithHand(int /*hand*/) const { return false; }
  typedef bool (*vr_surface_intersect)(const Point3 &pos, const Point3 &dir, Point2 &point_in_gui, Point3 &hit_pos);
  using EntityTransformResolver = TMatrix (*)(uint32_t, const char *);
  struct VrSceneData
  {
    TMatrix vrSpaceOrigin = TMatrix::IDENT;
    TMatrix camera = TMatrix::IDENT;
    Frustum cameraFrustum;

    TMatrix hands[2] = {TMatrix::IDENT, TMatrix::IDENT};
    TMatrix aims[2] = {TMatrix::IDENT, TMatrix::IDENT};
    TMatrix indexFingertips[2] = {TMatrix::IDENT, TMatrix::IDENT};

    vr_surface_intersect vrSurfaceIntersector = nullptr;
    EntityTransformResolver entityTmResolver = nullptr;
  };
  virtual void updateSpatialElements(const VrSceneData &vr_scene) = 0;
  virtual bool hasAnyPanels() = 0;
};


IGuiScene *create_gui_scene(StdGuiRender::GuiContext *ctx = nullptr);
IGuiScene *get_scene_from_sqvm(HSQUIRRELVM vm);

} // namespace darg
