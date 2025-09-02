// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include <drv/hid/dag_hiPointing.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>
#include <perfMon/dag_statDrv.h>

#include <util/dag_bitArray.h>
#include <util/dag_delayedAction.h>
#include <memory/dag_framemem.h>
#include <math/dag_mathUtils.h>

#include "guiScene.h"
#include "animation.h"
#include "scriptUtil.h"
#include "textUtil.h"
#include "profiler.h"

#include "dargDebugUtils.h"
#include <util/dag_convar.h>

#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>


namespace darg
{

CONSOLE_FLOAT_VAL("darg", kinetic_vel_viscosity, 0.25f);


static bool is_same_tbl_component(const Element *elem, const Component &new_comp)
{
  return !elem->hasScriptBuilder() && new_comp.scriptBuilder.IsNull() && new_comp.scriptDesc.IsEqual(elem->props.scriptDesc);
}


static void enable_kb_ime(bool on)
{
  run_action_on_main_thread([on]() {
    if (auto kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : nullptr)
      kbd->enableIme(on);
  });
}

ElementTree::ElementTree(GuiScene *gui_scene, Screen *sc) : guiScene(gui_scene), screen(sc)
{
  elemAllocator.init(sizeof(Element), 32);
  xmbAllocator.init(sizeof(XmbData), 32);
}


ElementTree::~ElementTree() { clear(); }


static bool elem_children_compare_order(Element *a, Element *b)
{
  SQInteger ordA = a->props.scriptDesc.RawGetSlotValue<SQInteger>(a->csk->sortOrder, 0);
  SQInteger ordB = b->props.scriptDesc.RawGetSlotValue<SQInteger>(b->csk->sortOrder, 0);
  return ordA < ordB;
}


static Element *resolve_xmb_parent(Element *elem)
{
  for (Element *xmbParent = elem->parent; xmbParent; xmbParent = xmbParent->parent)
    if (xmbParent->xmb)
      return xmbParent;
  return nullptr;
}


int ElementTree::releaseChild(Element *elem, Element *child)
{
  G_ASSERT_RETURN(child, 0);
  int outFlags = RESULT_ELEMS_ADDED_OR_REMOVED;
  if (child->hasHierFadeOutAnims() && (child->hasFlags(Element::F_WAIT_FOR_CHILDREN_FADEOUT) || child->hasFadeOutAnims()))
  {
    outFlags |= detachElement(child);
    elem->fadeOutChildren.push_back(child);
    child->startFadeOutAnims();
    animated.insert(child);
  }
  else
    outFlags |= deleteElement(child);
  return outFlags;
}


static void add_child_sorted(Element *elem, Element *child)
{
  auto &tab = elem->children;
  ptrdiff_t pos = 0;
  if (!tab.empty())
  {
    auto it = eastl::upper_bound(tab.begin(), tab.end(), child, elem_children_compare_order);
    pos = eastl::distance(tab.begin(), it);
    G_ASSERT(pos >= 0 && pos <= tab.size());
  }
  tab.insert(tab.begin() + pos, child);
}


static bool attach_detach_handlers_differ(const Sqrat::Table &a, const Sqrat::Table b, const StringKeys *csk)
{
  // Note: this is not 100% reliable, this will skip recreated closures
  // that are the same function capturing different variables
  // In certain cases it is needed to use explcit 'key' in components

  Sqrat::Object attachA = a.RawGetSlot(csk->onAttach);
  Sqrat::Object attachB = b.RawGetSlot(csk->onAttach);
  if (attachA.IsNull() != attachB.IsNull())
    return true;

  Sqrat::Object detachA = a.RawGetSlot(csk->onDetach);
  Sqrat::Object detachB = b.RawGetSlot(csk->onDetach);
  if (detachA.IsNull() != detachB.IsNull())
    return true;

  if (attachA.GetType() == OT_CLOSURE && attachB.GetType() == OT_CLOSURE)
  {

    if (_closure(attachA.GetObject())->_function != _closure(attachB.GetObject())->_function)
      return true;
  }

  if (detachA.GetType() == OT_CLOSURE && detachB.GetType() == OT_CLOSURE)
  {
    if (_closure(detachA.GetObject())->_function != _closure(detachB.GetObject())->_function)
      return true;
  }

  return false;
}


static bool match_elem_with_new_comp(const Element *elem, const Component &comp, bool is_gap, const StringKeys *csk)
{
#define MATCH true // highlight return values in code to ease reading

  if (elem->rendObjType != comp.rendObjType)
    return false;

  bool compHasBuilder = !comp.scriptBuilder.IsNull();
  if (elem->hasScriptBuilder() != compHasBuilder)
    return false;

  if (elem->props.scriptDesc.RawGetSlot(csk->xmbNode).IsNull() != comp.scriptDesc.RawGetSlot(csk->xmbNode).IsNull())
    return false;

  if (elem->hasFlags(Element::F_GAP) != is_gap)
    return false;

  if (attach_detach_handlers_differ(elem->props.scriptDesc, comp.scriptDesc, csk))
    return false;

  if (!comp.uniqueKey.IsNull() && !elem->props.uniqueKey.IsNull() && elem->props.uniqueKey.IsEqual(comp.uniqueKey))
    return MATCH;

  if (is_same_tbl_component(elem, comp))
    return MATCH;

  if (comp.uniqueKey.IsNull() && elem->props.uniqueKey.IsNull() && elem->behaviors.size() == comp.behaviors.size() &&
      memcmp(elem->behaviors.data(), comp.behaviors.data(), comp.behaviors.size() * sizeof(Behavior *)) == 0)
    return MATCH;

  return false;

#undef MATCH
}


void ElementTree::reuseUnchangedDynamicChildren(dag::Vector<Element *> &elem_children,
  dag::Vector<Sqrat::Object, framemem_allocator> &comp_children, Tab<Element *> &new_children_src)
{
  G_ASSERTF(comp_children.size() == new_children_src.size(), "Children array size mismatch: %d vs %d", int(comp_children.size()),
    int(new_children_src.size()));

  if (elem_children.empty())
    return;

  for (int icComp = 0, ncComp = comp_children.size(); icComp < ncComp; ++icComp)
  {
    Sqrat::Object &childCompObj = comp_children[icComp];
    if (childCompObj.GetType() != OT_CLOSURE)
      continue;

    for (int icElem = 0, ncElem = elem_children.size(); icElem < ncElem; ++icElem)
    {
      Element *childElem = elem_children[icElem];
      if (!childElem) // already reused and cleared
        continue;
      if (childElem->props.scriptBuilder.IsNull()) // not a dynamic
        continue;

      if (childElem->props.scriptBuilder.IsEqual(childCompObj))
      {
        new_children_src[icComp] = childElem;
        elem_children[icElem] = nullptr;
        break;
      }
    }
  }
}


void ElementTree::collectChildrenToReuse(dag::Vector<Element *> &elem_children,
  const dag::Vector<Component, framemem_allocator> &comp_children, Tab<Element *> &new_children_src)
{
  G_ASSERTF(comp_children.size() == new_children_src.size(), "Children array size mismatch: %d vs %d", int(comp_children.size()),
    int(new_children_src.size()));

  if (elem_children.empty())
    return;

  const StringKeys *csk = guiScene->getStringKeys();

  for (int icComp = 0, ncComp = comp_children.size(); icComp < ncComp; ++icComp)
  {
    const Component &childComp = comp_children[icComp];
    if (childComp.scriptDesc.IsNull())
      continue;
    if (new_children_src[icComp]) // already assigned
      continue;

    for (int icElem = 0, ncElem = elem_children.size(); icElem < ncElem; ++icElem)
    {
      Element *childElem = elem_children[icElem];
      if (!childElem) // already reused and cleared
        continue;

      if (match_elem_with_new_comp(childElem, childComp, false, csk))
      {
        new_children_src[icComp] = childElem;
        elem_children[icElem] = nullptr;
        break;
      }
    }
  }
}

bool ElementTree::does_element_affect_layout(const Element *elem) { return elem->hasFlags(Element::F_DOES_AFFECT_LAYOUT); }

bool ElementTree::does_component_affect_layout(const darg::Component &comp, const StringKeys *csk)
{
  return comp.rendObjType != ROBJ_NONE || !comp.scriptDesc.RawGetSlot(csk->children).IsNull() ||
         !comp.scriptDesc.RawGetSlot(csk->size).IsNull();
}


static void add_gaps(const Sqrat::Table &script_desc, dag::Vector<Element *> &cur_elem_children, const StringKeys *csk,
  dag::Vector<Sqrat::Object, framemem_allocator> &children_objs, Component::TmpVector &children_comps,
  Tab<Element *> &children_source_elems, Bitarray &gaps_mask, const Sqrat::Object &parent_builder)
{
  G_ASSERTF(children_objs.size() == children_comps.size(), "Children array size mismatch (1): %d vs %d", int(children_objs.size()),
    int(children_comps.size()));
  G_ASSERTF(children_objs.size() == children_source_elems.size(), "Children array size mismatch (2): %d vs %d",
    int(children_objs.size()), int(children_source_elems.size()));

  Sqrat::Object gapObj = script_desc.RawGetSlot(csk->gap);
  SQObjectType gapType = gapObj.GetType();
  if (gapType != OT_CLOSURE && gapType != OT_TABLE && gapType != OT_CLASS)
    return;

  Component tmpComp;

  gaps_mask.resize(children_objs.size() * 2);

  // TODO: match unchanged dynamic gaps also just like non-gap components

  bool needGap = false;
  for (int iChild = 0, nChildren = children_objs.size(); iChild < nChildren; ++iChild)
  {
    Element *matchedSrcElem = children_source_elems[iChild];
    darg::Component &candidateComp = children_comps[iChild];
    // insert gaps only if element is renderable or have children
    bool haveValidElem = (matchedSrcElem && ElementTree::does_element_affect_layout(matchedSrcElem)) ||
                         (!candidateComp.scriptDesc.IsNull() && ElementTree::does_component_affect_layout(candidateComp, csk));

    if (needGap && haveValidElem)
    {
      if (Component::build_component(tmpComp, gapObj, csk, parent_builder))
      {
        Element *source = nullptr;
        for (int i = 0, n = cur_elem_children.size(); i < n; ++i)
        {
          if (!cur_elem_children[i])
            continue; // already reused and cleared

          if (match_elem_with_new_comp(cur_elem_children[i], tmpComp, true, csk))
          {
            source = cur_elem_children[i];
            cur_elem_children[i] = nullptr;
            break;
          }
        }

        children_objs.insert(children_objs.begin() + iChild, gapObj);
        children_comps.insert(children_comps.begin() + iChild, tmpComp);
        insert_items(children_source_elems, iChild, 1, &source);
        gaps_mask.set(iChild);

        ++iChild; // handle indices change on insert
        ++nChildren;
      }

      needGap = false;
    }
    if (haveValidElem)
      needGap = true;
  }
}


Element *ElementTree::rebuild(HSQUIRRELVM vm, Element *existing, const Component &comp, Element *parent,
  const Sqrat::Object &parent_builder, int call_depth, int &out_flags)
{
  G_ASSERT(!existing || existing->etree == this);
  if (comp.scriptDesc.IsNull())
  {
    darg_immediate_error(vm, String(0, "%s: rebuild failed: null comp", __FUNCTION__));
    out_flags |= deleteElement(existing);
    return nullptr;
  }

  // no need to rebuild non-dynamic child element if no data was changed
  bool isSameDescTable = existing && is_same_tbl_component(existing, comp);
  if (isSameDescTable)
  {
    if (!existing->watch.empty())
    {
      Sqrat::Object traceKey = guiScene->getStringKeys()->watch;
      Sqrat::Object::iterator it;
      if (comp.scriptDesc.Next(it))
        traceKey = Sqrat::Object(it.getKey(), vm);
      darg_assert_trace_var("Unexpected watches in a component defined by table - probably incorrectly mutating the same table",
        comp.scriptDesc, traceKey);
    }

    return existing;
  }

  if (!existing)
    out_flags |= RESULT_ELEMS_ADDED_OR_REMOVED;

  StringKeys *csk = guiScene->getStringKeys();

  const int inputBehaviorFlags = Behavior::F_HANDLE_JOYSTICK | Behavior::F_HANDLE_KEYBOARD | Behavior::F_HANDLE_MOUSE |
                                 Behavior::F_HANDLE_TOUCH | Behavior::F_CAN_HANDLE_CLICKS | Behavior::F_FOCUS_ON_CLICK;

  bool wasHidden = existing && existing->isHidden();
  bool hadTransitions = existing && !existing->transitions.empty();
  bool hadBehaviors = existing && !existing->behaviors.empty();
  bool hadInputBehaviors = existing && existing->hasBehaviors(inputBehaviorFlags);
  bool wasXmbNode = existing && existing->xmb;
  auto prevZOrder = existing ? existing->zOrder : Element::undefined_z_order;

  Element *elem = existing ? existing : allocateElement(vm, parent, csk);

  elem->setup(comp, guiScene, existing ? SM_REBUILD_UPDATE : SM_INITIAL);

  if (elem->isHidden() != wasHidden)
    out_flags |= RESULT_ELEMS_ADDED_OR_REMOVED;
  if (!existing && !elem->hotkeyCombos.empty())
    out_flags |= RESULT_HOTKEYS_STACK_MODIFIED;
  if (elem->zOrder != prevZOrder)
    out_flags |= RESULT_INVALIDATE_RENDER_LIST;

  dag::Vector<Sqrat::Object, framemem_allocator> childrenDescObjs;
  Component::TmpVector childrenComps;
  Tab<Element *> newChildrenSrc(framemem_ptr());
  Bitarray gapsMask(framemem_ptr());

  const Sqrat::Object &childrensParentBuilder = comp.scriptBuilder.IsNull() ? parent_builder : comp.scriptBuilder;

  comp.readChildrenObjects(childrensParentBuilder, csk, childrenDescObjs);

  newChildrenSrc.resize(childrenDescObjs.size());
  mem_set_0(newChildrenSrc);

  reuseUnchangedDynamicChildren(elem->children, childrenDescObjs, newChildrenSrc);

  childrenComps.resize(childrenDescObjs.size()); // size should be synchronized with childrenDescObjs and newChildrenSrc
  for (size_t iChild = 0, nChildren = childrenDescObjs.size(); iChild < nChildren; ++iChild)
  {
    const Sqrat::Object &o = childrenDescObjs[iChild];
    bool reusedDynamic = newChildrenSrc[iChild] != nullptr;
    if (!o.IsNull() && !reusedDynamic)
      Component::build_component(childrenComps[iChild], o, csk, childrensParentBuilder);
  }
  collectChildrenToReuse(elem->children, childrenComps, newChildrenSrc);

  if (elem->layout.flowType != FLOW_PARENT_RELATIVE)
  {
    add_gaps(comp.scriptDesc, elem->children, csk, childrenDescObjs, childrenComps, newChildrenSrc, gapsMask, parent_builder);
  }

  bool haveCompGaps = gapsMask.size() != 0;

  for (Element *child : elem->children)
  {
    if (child) // left unmatched
      out_flags |= releaseChild(elem, child);
  }
  elem->children.clear();
  elem->children.reserve(childrenComps.size());

  if (removeExpiredFadeOutChildren(elem))
    out_flags |= RESULT_ELEMS_ADDED_OR_REMOVED;

  G_ASSERT(newChildrenSrc.size() == childrenComps.size());
  bool sortChildren = comp.scriptDesc.RawGetSlotValue(csk->sortChildren, false);
  for (int i = 0, n = newChildrenSrc.size(); i < n; ++i)
  {
    Element *srcElem = newChildrenSrc[i];
    if (srcElem && !srcElem->props.scriptBuilder.IsNull() && srcElem->props.scriptBuilder.IsEqual(childrenDescObjs[i]))
    {
      elem->children.push_back(srcElem); // reuse unchanged dynamic
      continue;
    }

    G_ASSERT(!srcElem || srcElem->hasFlags(Element::F_GAP) == (haveCompGaps && gapsMask[i] != 0));

    Element *child = rebuild(vm, srcElem, childrenComps[i], elem, childrensParentBuilder, call_depth + 1, out_flags);
    if (child)
    {
      if (!sortChildren)
        elem->children.push_back(child);
      else
        add_child_sorted(elem, child);

      if (haveCompGaps && gapsMask[i])
        child->updFlags(Element::F_GAP, true);
    }
  }

  childrenComps.clear();
  childrenDescObjs.clear();

  if (!elem->animations.empty())
    animated.insert(elem);
  if (hadTransitions != !elem->transitions.empty())
  {
    if (!elem->transitions.empty())
      withTransitions.insert(elem);
    else
      withTransitions.erase(elem);
  }

  if (!elem->behaviors.empty() && !hadBehaviors)
  {
    G_ASSERT(find_value_idx(withBehaviors, elem) < 0);
    withBehaviors.push_back(elem);
  }
  else if (elem->behaviors.empty() && hadBehaviors)
  {
    G_ASSERT(find_value_idx(withBehaviors, elem) >= 0);
    erase_item_by_value(withBehaviors, elem);
  }


  if (!existing)
  {
    elem->onAttach(guiScene);
  }

  bool isXmbNode = elem->xmb != nullptr;
  if (isXmbNode)
  {
    Element *xmbParent = resolve_xmb_parent(elem);
    if (xmbParent != elem->xmb->xmbParent)
      out_flags |= RESULT_NEED_XMB_REBUILD;
  }
  if (wasXmbNode != isXmbNode)
    out_flags |= RESULT_NEED_XMB_REBUILD;

  if (elem->hasBehaviors(inputBehaviorFlags) != hadInputBehaviors)
    out_flags |= RESULT_INVALIDATE_INPUT_STACK;

  elem->delayedCallElemStateHandler(); // force state synchronization
  if (existing && elem->watch.size() && call_depth > 0)
    guiScene->validateAfterRebuild(elem);

  return elem;
}


bool ElementTree::removeExpiredFadeOutChildren(Element *elem)
{
  bool removed = false;
  for (int i = elem->fadeOutChildren.size() - 1; i >= 0; --i)
  {
    Element *child = elem->fadeOutChildren[i];
    if (child->fadeOutFinished())
    {
      G_ASSERT(eastl::find(elem->fadeOutChildren.begin(), elem->fadeOutChildren.end(), child) != elem->fadeOutChildren.end());
      G_ASSERT(eastl::find(elem->children.begin(), elem->children.end(), child) == elem->children.end());
      deleteElement(child);
      elem->fadeOutChildren.erase(elem->fadeOutChildren.begin() + i);
      removed = true;
    }
  }

  for (int i = elem->children.size() - 1; i >= 0; --i)
  {
    Element *child = elem->children[i];
    G_ASSERT(child);
    // child may be detached during hierarchical detachElement() call
    if (child && child->isDetached() && child->fadeOutFinished())
    {
      G_ASSERT(eastl::find(elem->fadeOutChildren.begin(), elem->fadeOutChildren.end(), child) == elem->fadeOutChildren.end());
      G_ASSERT(eastl::find(elem->children.begin(), elem->children.end(), child) != elem->children.end());
      deleteElement(child);
      elem->children.erase(elem->children.begin() + i);
      removed = true;
    }
  }


  return removed;
}


void ElementTree::releaseXmbOnDetach(Element *elem)
{
  if (!elem->xmb)
    return;

  if (elem->xmb->xmbParent)
  {
    XmbData *pxmb = elem->xmb->xmbParent->xmb;
    G_ASSERTF(pxmb, "xmbParent = %p", elem->xmb->xmbParent);
    if (pxmb)
    {
      auto it = eastl::find(pxmb->xmbChildren.begin(), pxmb->xmbChildren.end(), elem);
      G_ASSERT(it != pxmb->xmbChildren.end());
      pxmb->xmbChildren.erase(it);
    }
  }

  freeXmbData(elem->xmb);
  elem->xmb = nullptr;
}


int ElementTree::detachElement(Element *elem)
{
  if (!elem || elem->isDetached())
    return 0;

  if (elem == xmbFocus)
    guiScene->setXmbFocus(nullptr);

  int resFlags = RESULT_ELEMS_ADDED_OR_REMOVED;
  if (!elem->hotkeyCombos.empty())
    resFlags |= RESULT_HOTKEYS_STACK_MODIFIED;

  SqStackChecker check(guiScene->getScriptVM());

  if (elem->behaviors.size())
  {
    // Assuming there are behaviors with different life times
    // Expecting short-lived ones are most probably located at the end of the vector
    bool found = false;
    for (int i = withBehaviors.size() - 1; i >= 0; --i)
    {
      if (withBehaviors[i] == elem)
      {
        erase_items(withBehaviors, i, 1);
        found = true;
        break;
      }
    }
    (void)found;
    G_ASSERT(found);
  }
  if (elem == kbFocus)
  {
    G_ASSERT(canHandleInput);
    if (kbFocus->hasBehaviors(Behavior::F_DISPLAY_IME))
      enable_kb_ime(false);
    kbFocus = nullptr;
    kbFocusCaptured = false;
  }
  if (elem == nextKbFocus)
    nextKbFocus = nullptr;
  auto itTopHover = topLevelHovers.find(elem);
  if (itTopHover != topLevelHovers.end())
  {
    elem->callHoverHandler(false);
    topLevelHovers.erase(itTopHover);
  }

  auto itOverscroll = overscroll.find(elem);
  if (itOverscroll != overscroll.end())
    overscroll.erase(itOverscroll);

  // Iterate backwards, but preserve order
  size_t fadeOutInsertPos = elem->fadeOutChildren.size();
  for (int i = elem->children.size() - 1; i >= 0; --i)
  {
    Element *child = elem->children[i];
    G_ASSERT_CONTINUE(child);

    int childRes = detachElement(child);
    if (child->hasHierFadeOutAnims())
    {
      if (childRes & RESULT_ELEMS_ADDED_OR_REMOVED)
      {
        elem->children.erase(elem->children.begin() + i);
        elem->fadeOutChildren.insert(elem->fadeOutChildren.begin() + fadeOutInsertPos, child);
        child->startFadeOutAnims();
        animated.insert(child);
      }
    }
    resFlags |= childRes;
  }

  kineticScroll.erase(elem);

  releaseXmbOnDetach(elem);

  elem->onDetach(guiScene);
  return resFlags;
}


int ElementTree::deleteElement(Element *elem)
{
  if (!elem)
    return 0;

  int resFlags = 0;

  if (elem->animations.size())
  {
    for (auto &anim : elem->animations)
      anim->skipOnDelete();
    // animated.erase(elem);
  }
  animated.erase(elem);
  animCleanupRoots.erase(elem);

  if (!elem->transitions.empty())
    withTransitions.erase(elem);

  for (Element *child : elem->children)
  {
    G_ASSERT_CONTINUE(child);
    resFlags |= deleteElement(child);
  }

  for (Element *child : elem->fadeOutChildren)
  {
    G_ASSERT_CONTINUE(child);
    resFlags |= deleteElement(child);
  }

  elem->children.clear();
  elem->fadeOutChildren.clear();

  elem->updateFadeOutCountersBeforeDetach();

  resFlags |= detachElement(elem) | RESULT_ELEMS_ADDED_OR_REMOVED;

  elem->onDelete();

  freeElement(elem);
  return resFlags;
}


int ElementTree::beforeRender(float dt)
{
  TIME_PROFILE(etree_beforeRender);
  return updateBehaviors(Behavior::STAGE_BEFORE_RENDER, dt);
}


void ElementTree::clear()
{
  if (root)
  {
    deleteElement(root);
    root = nullptr;
  }
  animated.clear();
  clear_and_shrink(withBehaviors);
  kbFocus = nullptr;
  kbFocusCaptured = false;
  topLevelHovers.clear();
  nextKbFocus = nullptr;
  nextKbFocusQueued = false;
  nextKbFocusNeedCapture = false;
  xmbFocus = nullptr;
  sceneStateFlags = 0;
}


int ElementTree::updateBehaviors(Behavior::UpdateStage stage, float dt)
{
  AutoProfileScope profile(guiScene->getProfiler(), stage == Behavior::STAGE_ACT ? M_BHV_UPDATE : M_BHV_BEFORE_RENDER);

  int resultFlags = 0;
  for (Element *elem : withBehaviors)
  {
    for (Behavior *bhv : elem->behaviors)
    {
      if (bhv->updateStages & stage)
      {
        int res = bhv->update(stage, elem, dt);
        resultFlags |= res;
      }
    }
  }
  return resultFlags;
}


void ElementTree::updateAnimations(float dt, bool *finished_any)
{
  TIME_PROFILE(etree_updateAnimations);

  int64_t dt_usec = int64_t(dt * 1000000);
  // DEBUG_CTX("Updating %d animated elems", animated.size());
  if (finished_any)
    *finished_any = false;
  for (Element *elem : animated)
  {
    for (auto &anim : elem->animations)
    {
      bool wasFinished = anim->isFinished();
      anim->update(dt_usec);
      if (!wasFinished && anim->isFinished() && finished_any)
        *finished_any = true;
    }
  }
}


void ElementTree::updateTransitions(float dt)
{
  int64_t dt_usec = int64_t(dt * 1000000);
  for (Element *elem : withTransitions)
  {
    for (auto &tr : elem->transitions)
      tr.update(dt_usec);
  }
}


void ElementTree::startKineticScroll(Element *elem, const Point2 &vel)
{
  float maxScrollX = elem->screenCoord.contentSize.x - elem->screenCoord.size.x;
  float maxScrollY = elem->screenCoord.contentSize.y - elem->screenCoord.size.y;

  elem->scrollVel = vel;

  if (maxScrollX <= 1.0f)
    elem->scrollVel.x = 0;
  if (maxScrollY <= 1.0f)
    elem->scrollVel.y = 0;

  if (lengthSq(vel) > 0)
    kineticScroll.insert(elem);
  else
    kineticScroll.erase(elem);
}


void ElementTree::stopKineticScroll(Element *elem)
{
  elem->scrollVel.zero();
  kineticScroll.erase(elem);
}


void ElementTree::updateKineticScroll(float dt)
{
  Tab<Element *> removed(framemem_ptr());

  for (Element *elem : kineticScroll)
  {
    Point2 prevOffs = elem->screenCoord.scrollOffs;
    elem->scroll(elem->scrollVel * dt);
    Point2 newOffs = elem->screenCoord.scrollOffs;

    if (lengthSq(prevOffs - newOffs) < 1.0f)
      removed.push_back(elem);
    else
    {
      float prevSpeed = length(elem->scrollVel);
      float newSpeed = approach(prevSpeed, 0.0f, dt, kinetic_vel_viscosity);
      if (newSpeed >= 1.0f)
        elem->scrollVel *= newSpeed / prevSpeed;
      else
        removed.push_back(elem);
    }
  }

  for (Element *elem : removed)
  {
    elem->scrollVel.zero();
    kineticScroll.erase(elem);
  }
}


void ElementTree::skipAllOneShotAnims()
{
  for (Element *elem : animated)
  {
    for (auto &anim : elem->animations)
    {
      if (!anim->desc.loop)
        anim->skip();
    }
  }
}


int ElementTree::cleanupAnimations()
{
  int result = 0;

  G_ASSERT(animCleanupRoots.empty());

  for (Element *elem : animated)
  {
    if (elem->isDetached() && elem->fadeOutFinished())
    {
      G_ASSERT(elem->parent);
      animCleanupRoots.insert(elem->parent);
    }
  }

  for (; !animCleanupRoots.empty();)
  {
    auto it = animCleanupRoots.begin();
    Element *e = *it;
    if (removeExpiredFadeOutChildren(e))
      result |= RESULT_ELEMS_ADDED_OR_REMOVED;
    else
    {
      G_ASSERTF(0, "Failed to remove animated children, roots count = %d, animated count = %d", int(animCleanupRoots.size()),
        int(animated.size()));
    }

    animCleanupRoots.erase(it);
  }

  return result;
}


int ElementTree::update(float dt)
{
  int animRes = cleanupAnimations();

  updateBehaviors(Behavior::STAGE_ACT, dt);
  updateOverscroll(dt);

  return animRes;
}


void ElementTree::startAnimation(const Sqrat::Object &trigger) { callAnimMethod(trigger, &Animation::start); }


void ElementTree::requestAnimStop(const Sqrat::Object &trigger) { callAnimMethod(trigger, &Animation::requestStop); }


void ElementTree::skipAnim(const Sqrat::Object &trigger) { callAnimMethod(trigger, &Animation::skip); }


void ElementTree::skipAnimDelay(const Sqrat::Object &trigger) { callAnimMethod(trigger, &Animation::skipDelay); }


void ElementTree::callAnimMethod(const Sqrat::Object &trigger, void (Animation::*func)())
{
  if (trigger.IsNull())
    return;

  for (Element *elem : animated)
    for (auto &animation : elem->animations)
      if (animation->desc.trigger.IsEqual(trigger))
        (animation.get()->*func)();
}


bool ElementTree::hasCapturedKbFocus() const { return kbFocus && kbFocusCaptured; }

void ElementTree::applyNextKbFocus()
{
  G_ASSERT(!guiScene->isRebuildingInvalidatedParts);
  if (nextKbFocusQueued)
  {
    setKbFocus(nextKbFocus, nextKbFocusNeedCapture);
    nextKbFocusQueued = false;
    nextKbFocus = nullptr;
    nextKbFocusNeedCapture = false;
  }
}


void ElementTree::setKbFocus(Element *elem, bool capture)
{
  if (!canHandleInput)
    return;

  if (kbFocus == elem)
  {
    kbFocusCaptured = capture;
    return;
  }

  if (guiScene->isRebuildingInvalidatedParts)
  {
    nextKbFocus = elem;
    nextKbFocusQueued = true;
    nextKbFocusNeedCapture = capture;
    return;
  }

  bool wasTextInput = kbFocus && is_text_input(kbFocus);
  bool hadIME = kbFocus && kbFocus->hasBehaviors(Behavior::F_DISPLAY_IME);

  if (kbFocus)
  {
    kbFocus->clearGroupStateFlags(Element::S_KB_FOCUS);

    Sqrat::Object scriptDesc = kbFocus->props.scriptDesc;
    Sqrat::Function onBlur(scriptDesc.GetVM(), scriptDesc, scriptDesc.RawGetSlot(kbFocus->csk->onBlur));
    if (!onBlur.IsNull())
      guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onBlur));
  }

  kbFocus = elem;
  kbFocusCaptured = capture;

  if (kbFocus)
  {
    kbFocus->setGroupStateFlags(Element::S_KB_FOCUS);

    Sqrat::Object scriptDesc = elem->props.scriptDesc;
    Sqrat::Function onFocus(scriptDesc.GetVM(), scriptDesc, scriptDesc.RawGetSlot(elem->csk->onFocus));
    if (!onFocus.IsNull())
      guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onFocus));

    // if (elem->hasFlags(Element::F_STICK_CURSOR))
    //   guiScene->moveMouseCursorTo(elem);
  }

  bool isTextInput = kbFocus && is_text_input(kbFocus);
  bool hasIME = kbFocus && kbFocus->hasBehaviors(Behavior::F_DISPLAY_IME);
  if (wasTextInput || isTextInput)
    guiScene->notifyInputConsumersCallback();

  if (hadIME != hasIME)
    enable_kb_ime(hasIME);
}


Element *ElementTree::findElementByKey(const Sqrat::Object &key) { return root ? root->findElementByKey(key) : nullptr; }


void ElementTree::updateSceneStateFlags(int flags, bool op_set)
{
  if (op_set)
    sceneStateFlags |= flags;
  else
    sceneStateFlags &= ~flags;
}


void ElementTree::updateHover(InputStack &input_stack, size_t npointers, const Point2 *positions, SQInteger *stick_scroll_flags)
{
  memset(stick_scroll_flags, 0, sizeof(*stick_scroll_flags) * npointers);
  dag::Vector<bool, framemem_allocator> gotNewTopLevelHover(npointers, false);

  using ElemSet = eastl::vector_set<Element *, eastl::less<Element *>, framemem_allocator>;
  using ElemGroupSet = eastl::vector_set<ElemGroup *, eastl::less<ElemGroup *>, framemem_allocator>;

  ElemSet newHovers;
  ElemGroupSet newHoverGroups;

  ElemSet prevTopLevelHoverElems, newTopLevelHoverElems;
  ElemGroupSet prevTopLevelHoverGroups, newTopLevelHoverGroups;

  for (Element *elem : topLevelHovers)
  {
    prevTopLevelHoverElems.insert(elem);
    if (elem->group)
      prevTopLevelHoverGroups.insert(elem->group);
  }
  topLevelHovers.clear();

  for (size_t hand = 0; hand < npointers; ++hand)
  {
    bool stopHover =
      (sceneStateFlags & F_DRAG_ACTIVE) || !(input_stack.summaryBhvFlags & (Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK));
    Element *scrollStopElem = nullptr;

    for (const InputEntry &ie : input_stack.stack)
    {
      const Point2 &pos = positions[hand];

      // hover state -----
      if (!stopHover && ie.elem->hitTest(pos))
      {
        if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE))
        {
          if (!gotNewTopLevelHover[hand])
          {
            gotNewTopLevelHover[hand] = true;
            newTopLevelHoverElems.insert(ie.elem);
            if (ie.elem->group)
              newTopLevelHoverGroups.insert(ie.elem->group);
          }
          newHovers.insert(ie.elem);
          if (ie.elem->group)
            newHoverGroups.insert(ie.elem->group);
        }

        stopHover = ie.elem->hasFlags(Element::F_STOP_HOVER | Element::F_STOP_MOUSE);
      }

      // stick scroll state -----
      // see scroll logic in doJoystickScroll()
      if (ie.elem->hitTest(pos))
      {
        if (ie.elem->hasFlags(Element::F_JOYSTICK_SCROLL) && (!scrollStopElem || ie.elem->isAscendantOf(scrollStopElem)))
          stick_scroll_flags[hand] |= ie.elem->getAvailableScrolls();

        if (!scrollStopElem && ie.elem->hasFlags(Element::F_STOP_MOUSE))
          scrollStopElem = ie.elem;
      }
    }
  }

  for (const InputEntry &ie : input_stack.stack)
  {
    if (newHovers.find(ie.elem) != newHovers.end() || (ie.elem->group && newHoverGroups.find(ie.elem->group) != newHoverGroups.end()))
      ie.elem->setGroupStateFlags(Element::S_HOVER);
    else
      ie.elem->clearGroupStateFlags(Element::S_HOVER);
  }

  for (Element *elem : prevTopLevelHoverElems)
  {
    if (newTopLevelHoverElems.find(elem) == newTopLevelHoverElems.end() &&
        (!elem->group || newTopLevelHoverGroups.find(elem->group) == newTopLevelHoverGroups.end()))
    {
      elem->clearGroupStateFlags(Element::S_TOP_HOVER);
      elem->callHoverHandler(false);
    }
  }

  for (Element *elem : newTopLevelHoverElems)
  {
    if (prevTopLevelHoverElems.find(elem) == prevTopLevelHoverElems.end() &&
        (!elem->group || prevTopLevelHoverGroups.find(elem->group) == prevTopLevelHoverGroups.end()))
    {
      elem->setGroupStateFlags(Element::S_TOP_HOVER);
      elem->callHoverHandler(true);
    }
  }

  topLevelHovers.insert(newTopLevelHoverElems.begin(), newTopLevelHoverElems.end());
}


bool ElementTree::doJoystickScroll(InputStack &input_stack, const Point2 &pointer_pos, const Point2 &delta)
{
  Element *scrollStopElem = nullptr;

  for (const InputEntry &ie : input_stack.stack)
  {
    if (ie.elem->hitTest(pointer_pos))
    {
      if (ie.elem->hasFlags(Element::F_JOYSTICK_SCROLL) && (!scrollStopElem || ie.elem->isAscendantOf(scrollStopElem)))
      {
        ie.elem->scroll(delta);
        return true;
      }

      if (!scrollStopElem && ie.elem->hasFlags(Element::F_STOP_MOUSE))
        scrollStopElem = ie.elem;
    }
  }

  return false;
}


int ElementTree::deactivateInput(InputDevice device, int pointer_id)
{
  int resAccum = 0;
  for (Element *elem : withBehaviors)
  {
    for (Behavior *bhv : elem->behaviors)
    {
      resAccum |= bhv->onDeactivateInput(elem, device, pointer_id);
    }
  }
  return resAccum;
}

Element *ElementTree::allocateElement(HSQUIRRELVM vm, Element *parent, const StringKeys *csk)
{
  Element *elem = (Element *)elemAllocator.allocateOneBlock();
  new (elem, _NEW_INPLACE) Element(vm, this, parent, csk);
  return elem;
}

void ElementTree::freeElement(Element *elem)
{
  if (elem)
  {
    elem->~Element();
    elemAllocator.freeOneBlock(elem);
  }
}

XmbData *ElementTree::allocateXmbData()
{
  XmbData *xmb = (XmbData *)xmbAllocator.allocateOneBlock();
  new (xmb, _NEW_INPLACE) XmbData();
  return xmb;
}

void ElementTree::freeXmbData(XmbData *xmb)
{
  if (xmb)
  {
    xmb->~XmbData();
    xmbAllocator.freeOneBlock(xmb);
  }
}


void ElementTree::setOverscroll(Element *elem, const Point2 &req_delta)
{
  if (lengthSq(req_delta) < 1.0f)
    return;

  OverscrollState &state = overscroll[elem];

  Point2 screenSize = guiScene->getDeviceScreenSize();
  const float fadeStep = ceilf(min(screenSize.x, screenSize.y) * 0.01f);

  Point2 delta = req_delta;

  // exponentially fade actual offset
  if (state.delta.x * delta.x > 0)
    delta.x *= expf(-fabsf(state.delta.x) / fadeStep);
  if (state.delta.y * delta.y > 0)
    delta.y *= expf(-fabsf(state.delta.y) / fadeStep);

  state.delta += delta;
  state.active = true;

  if (lengthSq(state.delta) < 1.0f)
    resetOverscroll(elem);
}


void ElementTree::releaseOverscroll(Element *elem)
{
  OverscrollMap::iterator it = overscroll.find(elem);
  if (it == overscroll.end())
    return;

  it->second.active = false;
}


void ElementTree::resetOverscroll(Element *elem)
{
  OverscrollMap::iterator it = overscroll.find(elem);
  if (it != overscroll.end())
    overscroll.erase(it);
}


void ElementTree::updateOverscroll(float dt)
{
  Point2 screenSize = guiScene->getDeviceScreenSize();
  float screenDim = min(screenSize.x, screenSize.y);

  for (OverscrollMap::iterator it = overscroll.begin(); it != overscroll.end();)
  {
    OverscrollState &state = it->second;
    if (state.active)
      ++it;
    else
    {
      float dist = length(state.delta);
      float speed = cvt(dist, 0.0f, screenDim * 0.1f, screenDim * 1.0f, screenDim * 0.2f);
      float maxMove = screenDim * speed * dt;

      if (dist < maxMove)
        state.delta.zero();
      else
      {
        Point2 dir = state.delta / dist;
        state.delta -= dir * maxMove;
      }

      if (lengthSq(state.delta) < 1.0f)
        it = overscroll.erase(it);
      else
        ++it;
    }
  }
}


} // namespace darg
