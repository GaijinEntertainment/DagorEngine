// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unordered_set.h>
#include <EASTL/vector_set.h>
#include <sqrat.h>
#include <daRg/dag_behavior.h>
#include <memory/dag_fixedBlockAllocator.h>
#include <memory/dag_framemem.h>
#include <math/dag_Point2.h>
#include <dag/dag_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>


namespace darg
{

class Element;
class Component;
class Animation;
class StringKeys;
class GuiScene;
class Panel;
class InputStack;
struct XmbData;


class ElementTree
{
public:
  enum
  {
    RESULT_ELEMS_ADDED_OR_REMOVED = 0x01,
    RESULT_HOTKEYS_STACK_MODIFIED = 0x02,
    RESULT_NEED_XMB_REBUILD = 0x04,
    RESULT_INVALIDATE_RENDER_LIST = 0x08,
    RESULT_INVALIDATE_INPUT_STACK = 0x10,
  };

  static constexpr int F_DRAG_ACTIVE = 0x0001;

public:
  ElementTree(GuiScene *gui_scene, Panel *panel);
  ~ElementTree();

  Element *rebuild(HSQUIRRELVM vm, Element *elem, const Component &comp, Element *parent, const Sqrat::Object &parent_builder,
    int call_depth, int &out_flags);
  int deleteElement(Element *elem);
  int detachElement(Element *elem);

  void clear();

  int beforeRender(float dt);
  int update(float dt);

  void startAnimation(const Sqrat::Object &trigger);
  void requestAnimStop(const Sqrat::Object &trigger);
  void skipAnim(const Sqrat::Object &trigger);
  void skipAnimDelay(const Sqrat::Object &trigger);
  void updateAnimations(float dt, bool *finished_any);
  void updateTransitions(float dt);
  void updateKineticScroll(float dt);
  void skipAllOneShotAnims();

  void setKbFocus(Element *elem, bool capture = false);
  void captureKbFocus(Element *elem) { setKbFocus(elem, true); }
  bool hasCapturedKbFocus() const;
  void applyNextKbFocus();

  void startKineticScroll(Element *elem, const Point2 &vel);
  void stopKineticScroll(Element *elem);

  void updateSceneStateFlags(int flags, bool op_set);
  void updateHover(InputStack &input_stack, size_t npointers, const Point2 *positions, SQInteger *stick_scroll_flags);
  bool doJoystickScroll(InputStack &input_stack, const Point2 &pointer_pos, const Point2 &delta);

  Element *findElementByKey(const Sqrat::Object &key);

  int deactivateInput(InputDevice device, int pointer_id);

  void setOverscroll(Element *elem, const Point2 &delta);
  void releaseOverscroll(Element *elem);
  void resetOverscroll(Element *elem);
  void updateOverscroll(float dt);

  static bool does_element_affect_layout(const Element *elem);
  static bool does_component_affect_layout(const darg::Component &comp, const StringKeys *csk);

private:
  int updateBehaviors(Behavior::UpdateStage stage, float dt);
  int cleanupAnimations();
  void callAnimMethod(const Sqrat::Object &trigger, void (Animation::*)());

  int releaseChild(Element *elem, Element *child);
  void reuseUnchangedDynamicChildren(dag::Vector<Element *> &elem_children,
    dag::Vector<Sqrat::Object, framemem_allocator> &comp_children, Tab<Element *> &new_children_src);
  void collectChildrenToReuse(dag::Vector<Element *> &elem_children, const dag::Vector<Component, framemem_allocator> &comp_children,
    Tab<Element *> &new_children_src);
  bool removeExpiredFadeOutChildren(Element *elem);

  void releaseXmbOnDetach(Element *elem);

public:
  Element *root = nullptr;
  Element *xmbFocus = nullptr;

  bool watchesAllowed = true;
  bool canHandleInput = true;

  typedef ska::flat_hash_set<Element *> ElementSet;
  ElementSet animated;
  ElementSet withTransitions;
  ElementSet kineticScroll;

  struct OverscrollState
  {
    bool active = false;
    Point2 delta = Point2(0, 0);
  };
  typedef ska::flat_hash_map<const Element *, OverscrollState> OverscrollMap;
  OverscrollMap overscroll;

  Tab<Element *> withBehaviors;

  Element *kbFocus = nullptr;
  bool kbFocusCaptured = false;
  eastl::vector_set<Element *> topLevelHovers;
  Element *nextKbFocus = nullptr;
  bool nextKbFocusQueued = false;
  bool nextKbFocusNeedCapture = false;

  GuiScene *guiScene = nullptr;
  Panel *panel = nullptr;

  int rebuildFlagsAccum = 0;
  int sceneStateFlags = 0;

private:
  eastl::vector_set<Element *> animCleanupRoots;

private:
  FixedBlockAllocator elemAllocator;

  Element *allocateElement(HSQUIRRELVM vm, Element *parent, const StringKeys *csk);
  void freeElement(Element *elem);

  FixedBlockAllocator xmbAllocator;

public:
  XmbData *allocateXmbData();
  void freeXmbData(XmbData *xmb);
};

} // namespace darg
