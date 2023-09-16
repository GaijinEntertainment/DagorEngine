//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <EASTL/vector.h>
#include <dag/dag_vector.h>

#include <daRg/dag_guiTypes.h>
#include <daRg/dag_layout.h>
#include <daRg/dag_animation.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_inputIds.h>

#include <quirrel/frp/dag_frp.h>

#include <math/dag_e3dColor.h>
#include <math/dag_bounds2.h>

#include <sqrat.h>

#if DAGOR_DBGLEVEL > 0
#define DARG_USE_DBGCOLOR 1
#else
#define DARG_USE_DBGCOLOR 0
#endif

struct GuiVertexTransform;
namespace StdGuiRender
{
class GuiContext;
}


namespace sqfrp
{
class BaseObservable;
}

namespace darg
{

class Component;
class RenderObject;
class RendObjParams;
class Animation;
class Behavior;
class Properties;
class Transform;
class Style;
class StringKeys;
class RenderList;
class InputStack;
class HotkeyCombo;
class Hotkeys;
class EventList;
class GuiScene;
class ElemGroup;
class ScrollHandler;
class Transition;
class ElementTree;
class ElementRef;


struct ElemStacks
{
  RenderList *rlist = nullptr;
  InputStack *input = nullptr;
  InputStack *cursors = nullptr;
  InputStack *eventHandlers = nullptr;
};

struct ElemStackCounters
{
  int hierOrderRender = 0;
  int hierOrderInput = 0;
  int hierOrderCursor = 0;
  int hierOrderEvtH = 0;
};


struct XmbData
{
  Sqrat::Table nodeDesc;
  Element *xmbParent = nullptr;
  dag::Vector<Element *> xmbChildren;

  bool calcCanFocus();

  ~XmbData();
};

bool should_use_outside_area(InputDevice device);

class Element : public sqfrp::IStateWatcher
{
public:
  enum StateFlags
  {
    S_KB_FOCUS = 0x0001,
    S_HOVER = 0x0002,
    S_TOP_HOVER = 0x0004,
    S_DRAG = 0x0008,

    S_MOUSE_ACTIVE = 0x0010,
    S_KBD_ACTIVE = 0x0020,
    S_HOTKEY_ACTIVE = 0x0040,
    S_TOUCH_ACTIVE = 0x0080,
    S_JOYSTICK_ACTIVE = 0x0100,
    S_VR_ACTIVE = 0x2000,
    S_ACTIVE = S_MOUSE_ACTIVE | S_KBD_ACTIVE | S_HOTKEY_ACTIVE | S_TOUCH_ACTIVE | S_JOYSTICK_ACTIVE | S_VR_ACTIVE,
  };

  enum Flags
  {
    F_FLOWING = 0x0001,
    F_SUBPIXEL = 0x0002,
    F_HAS_SCRIPT_BUILDER = 0x0004,
    F_CLIP_CHILDREN = 0x0008,
    F_SIZE_CALCULATED = 0x0010,
    F_HAS_EVENT_HANDLERS = 0x0020,
    F_DISABLE_INPUT = 0x0040,
    F_STOP_MOUSE = 0x0080,
    F_STOP_HOVER = 0x0100,
    F_STOP_HOTKEYS = 0x0200,
    F_STICK_CURSOR = 0x0400,
    F_JOYSTICK_SCROLL = 0x0800,
    F_SKIP_DIRPAD_NAV = 0x1000,
    F_INPUT_PASSIVE = 0x2000,
    F_IGNORE_EARLY_CLIP = 0x4000,
    F_SCRIPT_ATTACH_DONE = 0x8000,
    F_WAIT_FOR_CHILDREN_FADEOUT = 0x10000,
    F_HIDDEN = 0x20000,
    F_DETACHED = 0x40000,
    F_SCREEN_BOX_CLIPPED_OUT = 0x80000, //< calculated on render
    F_GAP = 0x100000,
    F_LAYOUT_RECALC_PENDING_FIXED_SIZE = 0x200000,
    F_LAYOUT_RECALC_PENDING_SIZE = 0x400000,
    F_LAYOUT_RECALC_PENDING_FLOW = 0x800000,
    F_DOES_AFFECT_LAYOUT = 0x1000000,
    F_ZORDER_ON_TOP = 0x2000000,
    F_HAS_CURSOR = 0x4000000,
  };

public:
  Element(HSQUIRRELVM vm, ElementTree *etree, Element *parent, const StringKeys *csk);
  Element(const Element &) = delete;
  ~Element();

  HSQUIRRELVM getVM() const;

  void setup(const Component &component, GuiScene *gui_scene, SetupMode setup_mode);

  void onAttach(GuiScene *gui_scene);
  void onDetach(GuiScene *gui_scene);
  void onDelete();
  void callScriptAttach(GuiScene *gui_scene);

  void render(StdGuiRender::GuiContext &ctx, RenderList *rend_list, bool hier_break) const;
  void postRender(StdGuiRender::GuiContext &ctx, RenderList *rend_list) const;

  void recalcLayout();
  void getSizeRoots(Element *&fixed_size_root, Element *&size_root, Element *&flow_root);
  void recalcScreenPositions();
  void recalcContentSize(int axis);
  void calcSizeConstraints(int axis, float *sz_min, float *sz_max) const;
  void calcFixedSizes();
  void calcConstrainedSizes(int axis);

  void putToSortedStacks(ElemStacks &stacks, ElemStackCounters &counters, int parent_z_order, bool parent_disable_input);
  void traceHit(const Point2 &p, InputStack *stack, int parent_z_order, int &hier_order);

  float calcParentW(float percent, bool use_min_max) const;
  float calcParentH(float percent, bool use_min_max) const;
  float calcWidthPercent(float percent) const;
  float calcHeightPercent(float percent) const;
  float calcFontHeightPercent(float mul) const;
  float sizeSpecToPixels(const SizeSpec &ss, int axis, bool use_min_max) const;

  bool isMainTreeRoot() const;
  bool isInMainTree() const;
  Element *getParent() const { return parent; }
  bool isAscendantOf(const Element *child) const;

  void scroll(const Point2 &delta, Point2 *overscroll = nullptr);
  void scrollTo(const Point2 &offs);
  Point2 getScrollRange(int axis) const;

  void setGroupStateFlags(int f);
  void clearGroupStateFlags(int f);
  // Note: user code should almost always set/clear flags for entire group
  void setElemStateFlags(int f);
  void clearElemStateFlags(int f);
  int getStateFlags() const { return stateFlags; }

  bool hasFlags(int f) const { return (flags & f) != 0; }
  void updFlags(int f, bool on);

  bool isHidden() const { return hasFlags(F_HIDDEN); }
  void setHidden(bool on) { updFlags(F_HIDDEN, on); }
  bool isDetached() const { return hasFlags(F_DETACHED); }
  bool bboxIsClippedOut() const { return hasFlags(F_SCREEN_BOX_CLIPPED_OUT); }

  Element *findChildByScriptCtor(const Sqrat::Object &ctor) const;

  bool hitTest(const Point2 &p) const;
  bool hitTest(float x, float y) const { return hitTest(Point2(x, y)); }
  bool hitTestWithTouchMargin(const Point2 &p, InputDevice device) const;
  bool hasBehaviors(int bhv_flags) const;
  bool hasScriptBuilder() const { return (flags & Element::F_HAS_SCRIPT_BUILDER) != 0; }

  bool hasFadeOutAnims() const;
  bool fadeOutFinished() const;
  void startFadeOutAnims();
  void updateFadeOutCountersBeforeDetach();
  bool hasHierFadeOutAnims() const { return hierNumFadeOutAnims > 0; }

  void onElemGroupRelease();
  void onScrollHandlerRelease();

  void delayedCallElemStateHandler();

  // IStateWatcher implementation
  virtual void onObservableRelease(sqfrp::BaseObservable *observable) override;
  virtual bool onSourceObservableChanged() override;
  virtual Sqrat::Object dbgGetWatcherScriptInstance() override;

  void callHoverHandler(bool hover_on);
  bool calcSoundPos(Point2 &res);
  void playSound(const Sqrat::Object &key);

  BBox2 calcTransformedBbox() const;
  BBox2 calcTransformedBboxPadded() const;
  void calcFullTransform(GuiVertexTransform &res_gvtm) const;
  bool updateCtxTransform(StdGuiRender::GuiContext &ctx, GuiVertexTransform &prevGvtm, bool hier_break) const;

  bool isDirPadNavigable() const;
  bool getNavAnchor(Point2 &val) const;

  int getHierDepth() const { return hierDepth; }

  RenderObject *rendObj() const { return rendObjStor[0] ? (RenderObject *)&rendObjStor : nullptr; }
  Element *findElementByKey(const Sqrat::Object &key);

  int getAvailableScrolls() const;

  Sqrat::Object getRef(HSQUIRRELVM vm);
  Sqrat::Table getElementInfo(HSQUIRRELVM vm);
  Sqrat::Table getElementBBox(HSQUIRRELVM vm);

private:
  void setupAnimations(const Sqrat::Table &desc);
  void setupTransitions(const Sqrat::Table &desc);
  void addOrUpdateTransition(const Sqrat::Object &item, AnimProp prop_override = AP_INVALID);
  void readTransform(const Sqrat::Table &desc);
  void readHotkeys(const Sqrat::Table &desc, const Sqrat::Array &hotkeys_data, const Hotkeys &hotkeys);

  void setupElemGroup(const Sqrat::Table &desc_tbl);
  void releaseElemGroup();

  void setupWatch(const Sqrat::Table &desc_tbl);
  void releaseWatch();
  template <typename Vec>
  void readWatches(const Sqrat::Table &desc_tbl, const Sqrat::Table &script_watches, Vec &out);

  void setupBehaviorsList(const dag::Vector<Behavior *> &comp_behaviors);
  void updateBehaviorsList(const dag::Vector<Behavior *> &comp_behaviors);

  void setupScroll(const Sqrat::Table &desc_tbl);
  void releaseScrollHandler();
  void limitScrollOffset();

  Point2 calcParentRelPos();
  void alignChildren(int axis);
  float calcContentSizeByAxis(int axis);
  bool isFlowAxis(int axis) const;

  void playStateChangeSound(int prev_flags, int cur_flags);

  void afterRecalc();

  void validateStaticText();
  void clampSizeToLimits(int axis, float &sz_px);

  void applyTransform(const GuiVertexTransform &prev_gvtm, GuiVertexTransform &res_gvtm) const;
  void renderXmbOverlayDebug(StdGuiRender::GuiContext &ctx) const;

  int16_t getZOrder(int parent_z_order) const;

public:
  ElementTree *etree;
  const StringKeys *csk;

  int rendObjType;
  void *rendObjStor[1] = {nullptr};
  RendObjParams *robjParams = nullptr;
  Layout layout;
  ScreenCoord screenCoord;
  BBox2 clippedScreenRect; // calculated on render, affected by transform
  BBox2 transformedBbox;   // calculated on render

  int behaviorsSummaryFlags = 0;

  Element *parent = nullptr;
  dag::Vector<Element *> children;
  dag::Vector<Element *> fadeOutChildren;
  dag::Vector<eastl::unique_ptr<Animation>> animations;
  dag::Vector<Behavior *> behaviors;
  Properties props;
  dag::Vector<Transition> transitions;

  dag::Vector<sqfrp::BaseObservable *> watch;

  Transform *transform = nullptr;

  dag::Vector<eastl::unique_ptr<HotkeyCombo>> hotkeyCombos;

  ElemGroup *group = nullptr;
  ScrollHandler *scrollHandler = nullptr;

  XmbData *xmb = nullptr;

  Point2 textSizeCache = Point2(-1, -1);

  Point2 scrollVel = Point2(0, 0);
  ElementRef *ref = nullptr;

private:
  int flags = 0;
  int stateFlags = 0;

  int16_t zOrder = undefined_z_order;
  int16_t hierDepth = 0;
  int hierNumFadeOutAnims = 0;

#if DARG_USE_DBGCOLOR
  mutable E3DCOLOR dbgColor = 0u;
#endif

  static const int16_t undefined_z_order;

  friend class ElementTree;
};


} // namespace darg
