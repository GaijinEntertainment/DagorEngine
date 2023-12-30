#include <daRg/dag_element.h>
#include <daRg/dag_renderObject.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiGlobals.h>
#include <daRg/dag_uiSound.h>
#include <daRg/dag_picture.h>
#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_sceneRender.h>

#include <EASTL/algorithm.h>
#include <generic/dag_relocatableFixedVector.h>
#include <perfMon/dag_statDrv.h>
#include <memory/dag_framemem.h>
#include <math/random/dag_random.h>
#include <utf8/utf8.h>
#include <util/dag_convar.h>

#include <sqext.h>

#include "guiScene.h"
#include "elemGroup.h"
#include "animation.h"
#include "textUtil.h"
#include "scriptUtil.h"
#include "inputStack.h"
#include "hotkeys.h"
#include "scrollHandler.h"
#include "behaviors/bhvTextArea.h"
#include "stdRendObj.h"
#include "profiler.h"
#include "dargDebugUtils.h"
#include "eventData.h"
#include "behaviorHelpers.h"
#include "elementRef.h"


#define DEBUG_XMB_OVERLAY 0

namespace darg
{

using namespace sqfrp;

CONSOLE_BOOL_VAL("darg", button_margin_for_mouse, false);

bool XmbData::calcCanFocus()
{
  Sqrat::Object canFocus = nodeDesc.GetSlot("canFocus");
  if (canFocus.IsNull()) // allow by default
    return true;

  if (canFocus.GetType() == OT_CLOSURE)
  {
    HSQUIRRELVM vm = nodeDesc.GetVM();
    Sqrat::Function func(vm, Sqrat::Object(vm), canFocus);
    bool res = false;
    return (func.Evaluate(res) && res);
  }

  return canFocus.Cast<bool>();
}


XmbData::~XmbData()
{
  // This is a bit redundant, but needed for the case when xmb node is removed
  // from the element during rebuild. In this case, xmbParent points to the
  // Element with xmb==nullptr.
  // This is not a problem itself, since XMB nodes will be relinked, but triggers
  // an assert required for XMB consistency check.
  for (Element *child : xmbChildren)
    if (child->xmb)
      child->xmb->xmbParent = nullptr;
}


const int16_t Element::undefined_z_order = -1;


Element::Element(HSQUIRRELVM vm, ElementTree *etree_, Element *parent_, const StringKeys *csk_) :
  parent(parent_), etree(etree_), rendObjType(ROBJ_NONE), csk(csk_), props(vm)
{
  screenCoord.reset();

  G_ASSERT(!parent || etree_ == parent->etree);

  hierDepth = parent ? parent->hierDepth + 1 : 0;
}


Element::~Element()
{
  releaseWatch();
  releaseElemGroup();
  releaseScrollHandler();

  G_ASSERT(children.empty());
  G_ASSERT(fadeOutChildren.empty());

  animations.clear();
  hotkeyCombos.clear();

  if (auto ro = rendObj())
    eastl::destroy_at(ro);
  delete robjParams;
  delete transform;

  G_ASSERT(!ref || !ref->elem);
  G_ASSERT(!xmb);
}


HSQUIRRELVM Element::getVM() const { return props.storage.GetVM(); }


bool Element::isInMainTree() const { return etree->guiScene->getElementTree() == etree; }

bool Element::isMainTreeRoot() const { return !parent && isInMainTree(); }


bool Element::isAscendantOf(const Element *child) const
{
  G_ASSERT_RETURN(child, false);
  for (const Element *e = child->parent; e; e = e->parent)
    if (e == this)
      return true;
  return false;
}


static void call_attach_detach_handler(GuiScene *scene, Element *elem, bool attach)
{
  const Sqrat::Object *slot = attach ? &elem->csk->onAttach : &elem->csk->onDetach;

  const Sqrat::Object &funcName = attach ? elem->csk->onAttach : elem->csk->onDetach;
  Sqrat::Function func = elem->props.scriptDesc.RawGetFunction(funcName);

  if (!func.IsNull())
  {
    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(func, &nparams, &nfreevars));

    if (nparams < 1 || nparams > 2)
    {
      darg_assert_trace_var("Expected 1 param or no params for attach/detach", elem->props.scriptDesc, *slot);
      return;
    }

    if (nparams == 1)
    {
      auto handler = new ScriptHandlerSqFunc<>(func);
      if (!attach)
        handler->allowOnShutdown = true;
      scene->queueScriptHandler(handler);
    }
    else
    {
      // shoud be called with delay, but needs Element pointer to stay valid
      func(elem->getRef(func.GetVM()));
    }
  }
}


void Element::onAttach(GuiScene *)
{
  for (Behavior *bhv : behaviors)
    bhv->onAttach(this);
}


void Element::callScriptAttach(GuiScene *gui_scene)
{
  // if (hasFlags(F_SCRIPT_ATTACH_DONE))
  //   return;
  //^ More distant leaves could be updated, while this element was reused
  // Maybe we need anoter flag propagated to rebuld root

  if (!hasFlags(F_SCRIPT_ATTACH_DONE))
  {
    call_attach_detach_handler(gui_scene, this, true);
    updFlags(F_SCRIPT_ATTACH_DONE, true);
  }

  for (Element *child : children)
    child->callScriptAttach(gui_scene);
}


void Element::onDetach(GuiScene *gui_scene)
{
  if (isDetached())
    return;

  gui_scene->onElementDetached(this);
  releaseWatch();
  releaseScrollHandler();

  if (hasFlags(F_SCRIPT_ATTACH_DONE))
    call_attach_detach_handler(gui_scene, this, false);

  for (Behavior *bhv : behaviors)
    bhv->onDetach(this, Behavior::DETACH_FINAL);

  if (xmb)
  {
    etree->freeXmbData(xmb);
    xmb = nullptr;
  }

  flags |= F_DETACHED;

  playSound(csk->detach);

  if (ref)
    ref->elem = nullptr;
}


void Element::onDelete()
{
  for (Behavior *bhv : behaviors)
    bhv->onDelete(this);
}


void Element::setupBehaviorsList(const dag::Vector<Behavior *> &comp_behaviors)
{
  G_ASSERT(behaviors.empty());
  G_ASSERT(behaviorsSummaryFlags == 0);

  behaviors.resize(comp_behaviors.size());
  memcpy(behaviors.data(), comp_behaviors.data(), comp_behaviors.size() * sizeof(Behavior *));

  behaviorsSummaryFlags = 0;

  for (Behavior *bhv : behaviors)
    behaviorsSummaryFlags |= bhv->flags;
}


void Element::updateBehaviorsList(const dag::Vector<Behavior *> &comp_behaviors)
{
  G_ASSERT(!isDetached());

  if (behaviors.size() == comp_behaviors.size() &&
      memcmp(behaviors.data(), comp_behaviors.data(), behaviors.size() * sizeof(Behavior *)) == 0)
    return;

  for (Behavior *oldBhv : behaviors)
    if (eastl::find(comp_behaviors.begin(), comp_behaviors.end(), oldBhv) == comp_behaviors.end())
      oldBhv->onDetach(this, Behavior::DETACH_REBUILD);

  for (Behavior *newBhv : comp_behaviors)
  {
    bool isNew = eastl::find(behaviors.begin(), behaviors.end(), newBhv) == behaviors.end();
    if (isNew)
      newBhv->onAttach(this);
  }

  behaviorsSummaryFlags = 0;
  behaviors.resize(comp_behaviors.size());
  memcpy(behaviors.data(), comp_behaviors.data(), comp_behaviors.size() * sizeof(Behavior *));
  for (Behavior *bhv : behaviors)
    behaviorsSummaryFlags |= bhv->flags;
}


void Element::setup(const Component &comp, GuiScene *gui_scene, SetupMode setup_mode)
{
  G_ASSERT(!comp.scriptDesc.IsNull());

  textSizeCache = Point2(-1, -1);

  bool initRendObj = (setup_mode == SM_INITIAL) || (setup_mode == SM_REBUILD_UPDATE && rendObjType != comp.rendObjType);
  if (initRendObj)
  {
    if (auto ro = rendObj())
      eastl::destroy_at(ro);
    del_it(robjParams);
    rendObjType = comp.rendObjType;
    if (!create_rendobj(rendObjType, rendObjStor))
      rendObjStor[0] = nullptr;
    robjParams = create_robj_params(rendObjType);
  }

  const Sqrat::Table &scriptDesc = comp.scriptDesc;
  Sqrat::Table prevDesc = props.scriptDesc;

  props.load(scriptDesc, comp.scriptBuilder, csk);
  discard_text_cache(robjParams);

  validateStaticText();

  layout.read(this, props, csk);
  if (isMainTreeRoot())
  {
    layout.size[0].mode = SizeSpec::PIXELS;
    layout.size[1].mode = SizeSpec::PIXELS;
    layout.size[0].value = StdGuiRender::screen_width();
    layout.size[1].value = StdGuiRender::screen_height();
  }

  if (setup_mode != SM_REALTIME_UPDATE)
  {
    setupScroll(scriptDesc);
    setupElemGroup(scriptDesc);

    Sqrat::Table xmbNode = scriptDesc.RawGetSlot(csk->xmbNode);
    if (xmbNode.IsNull() && xmb != nullptr)
    {
      etree->freeXmbData(xmb);
      xmb = nullptr;
    }
    else if (!xmbNode.IsNull() && xmb == nullptr)
      xmb = etree->allocateXmbData();
    if (xmb != nullptr)
    {
      xmbNode.SetInstance(csk->elem, this);
      xmb->nodeDesc = xmbNode;
    }
  }

  if (setup_mode == SM_INITIAL)
  {
    setupBehaviorsList(comp.behaviors);
    setupAnimations(scriptDesc);
  }
  else if (setup_mode == SM_REBUILD_UPDATE && !isDetached())
  {
    updateBehaviorsList(comp.behaviors);
  }

  if (setup_mode == SM_INITIAL || setup_mode == SM_REBUILD_UPDATE)
    setupWatch(scriptDesc);

  if (robjParams)
    robjParams->load(this);

  readTransform(scriptDesc);

  zOrder = scriptDesc.RawGetSlotValue<int16_t>(csk->zOrder, undefined_z_order);

  if (setup_mode != SM_REALTIME_UPDATE)
    setupTransitions(scriptDesc);

  for (eastl::unique_ptr<Animation> &anim : animations)
    anim->setup(setup_mode == SM_INITIAL);

  bool isFlowing = layout.flowType == FLOW_HORIZONTAL || layout.flowType == FLOW_VERTICAL || (parent && (parent->flags & F_FLOWING));
  bool isSubpixel = scriptDesc.RawGetSlotValue(csk->subPixel, (flags & F_SUBPIXEL) != 0);
  bool stickCursor = (!scriptDesc.RawGetSlot(csk->onClick).IsNull() || !scriptDesc.RawGetSlot(csk->onDoubleClick).IsNull()) &&
                     scriptDesc.RawGetSlotValue(csk->stickCursor, true);
  Sqrat::Object waitForChildrenFadeOut = scriptDesc.RawGetSlot(csk->waitForChildrenFadeOut);
  const bool hasCursor = scriptDesc.RawHasKey(csk->cursor) || !scriptDesc.RawGetSlot(csk->moveResizeCursors).IsNull();

  updFlags(F_FLOWING, isFlowing);
  updFlags(F_SUBPIXEL, isSubpixel);
  updFlags(F_HAS_SCRIPT_BUILDER, !comp.scriptBuilder.IsNull());
  updFlags(F_CLIP_CHILDREN, scriptDesc.RawGetSlotValue(csk->clipChildren, false));
  updFlags(F_HAS_EVENT_HANDLERS, scriptDesc.RawHasKey(csk->eventHandlers));
  updFlags(F_DISABLE_INPUT, scriptDesc.RawGetSlotValue(csk->disableInput, false));
  updFlags(F_STOP_MOUSE, scriptDesc.RawGetSlotValue(csk->stopMouse, false));
  updFlags(F_STOP_HOVER, scriptDesc.RawGetSlotValue(csk->stopHover, false));
  updFlags(F_STOP_HOTKEYS, scriptDesc.RawGetSlotValue(csk->stopHotkeys, false));
  updFlags(F_STICK_CURSOR, stickCursor);
  updFlags(F_JOYSTICK_SCROLL, scriptDesc.RawGetSlotValue(csk->joystickScroll, false));
  updFlags(F_SKIP_DIRPAD_NAV, scriptDesc.RawGetSlotValue(csk->skipDirPadNav, false));
  updFlags(F_INPUT_PASSIVE, scriptDesc.RawGetSlotValue(csk->inputPassive, false));
  updFlags(F_IGNORE_EARLY_CLIP, scriptDesc.RawGetSlotValue(csk->ignoreEarlyClip, false));
  if (waitForChildrenFadeOut.IsNull())
    updFlags(F_WAIT_FOR_CHILDREN_FADEOUT, parent && parent->hasFlags(F_WAIT_FOR_CHILDREN_FADEOUT));
  else
    updFlags(F_WAIT_FOR_CHILDREN_FADEOUT, waitForChildrenFadeOut.Cast<bool>());
  updFlags(F_HIDDEN, scriptDesc.RawGetSlotValue(csk->isHidden, false));
  updFlags(F_DOES_AFFECT_LAYOUT, ElementTree::does_component_affect_layout(comp, csk));
  updFlags(F_HAS_CURSOR, hasCursor);

  if (setup_mode == SM_INITIAL)
    readHotkeys(scriptDesc, scriptDesc.RawGetSlot(csk->hotkeys), gui_scene->getHotkeys());
  else if (setup_mode == SM_REBUILD_UPDATE && !isDetached())
  {
    Sqrat::Object prevHotkeys = prevDesc.RawGetSlot(csk->hotkeys);
    Sqrat::Object newHotkeys = scriptDesc.RawGetSlot(csk->hotkeys);
    if (!is_same_immutable_obj(prevHotkeys, newHotkeys))
      readHotkeys(scriptDesc, newHotkeys, gui_scene->getHotkeys());
  }

#if DARG_USE_DBGCOLOR
  if (debug_frames_mode == DFM_ELEM_UPDATE)
    dbgColor = E3DCOLOR(grnd() % 255, grnd() % 255, grnd() % 255);
#endif

  if (setup_mode != SM_REALTIME_UPDATE)
  {
    for (Behavior *bhv : behaviors)
      bhv->onElemSetup(this, setup_mode);
  }
}


void Element::delayedCallElemStateHandler()
{
  Sqrat::Object onElemState = props.scriptDesc.RawGetSlot(csk->onElemState);
  if (!onElemState.IsNull())
  {
    GuiScene *guiScene = GuiScene::get_from_elem(this);
    HSQUIRRELVM vm = onElemState.GetVM();
    Sqrat::Function f(vm, Sqrat::Object(vm), onElemState);
    guiScene->queueScriptHandler(new ScriptHandlerSqFunc<int>(f, stateFlags));
  }
}


void Element::updFlags(int f, bool on)
{
  if (on)
    flags |= f;
  else
    flags &= ~f;
}


void Element::setupScroll(const Sqrat::Table &desc_tbl)
{
  Sqrat::Object x = desc_tbl.RawGetSlot(csk->scrollOffsX);
  Sqrat::Object y = desc_tbl.RawGetSlot(csk->scrollOffsY);

  if (x.GetType() & SQOBJECT_NUMERIC)
    screenCoord.scrollOffs.x = x.Cast<float>();

  if (y.GetType() & SQOBJECT_NUMERIC)
    screenCoord.scrollOffs.y = y.Cast<float>();


  Sqrat::Object handlerObj = desc_tbl.RawGetSlot(csk->scrollHandler);

  if (handlerObj.GetType() != OT_INSTANCE)
  {
    if (handlerObj.GetType() != OT_NULL)
      darg_assert_trace_var(
        String(0, "Unexpected scrollHandler field type %s (%X)", sq_objtypestr(handlerObj.GetType()), handlerObj.GetType()), desc_tbl,
        csk->scrollHandler);
  }
  else
  {
    ScrollHandler *h = handlerObj.Cast<ScrollHandler *>();
    G_ASSERT(h);

    if (h != scrollHandler)
    {
      releaseScrollHandler();
      scrollHandler = h;
      if (h)
        h->subscribe(this);
    }
  }
}


void Element::releaseScrollHandler()
{
  if (scrollHandler)
  {
    scrollHandler->unsubscribe(this);
    scrollHandler = NULL;
  }
}


void Element::onScrollHandlerRelease() { scrollHandler = NULL; }


void Element::readTransform(const Sqrat::Table &desc)
{
  Sqrat::Table tmDesc = desc.RawGetSlot(csk->transform);
  if (tmDesc.IsNull())
  {
    del_it(transform);
    return;
  }

  if (!transform)
    transform = new Transform();

  Sqrat::Object objPivot = tmDesc.RawGetSlot(csk->pivot);
  if (!objPivot.IsNull())
    transform->pivot = script_get_point2(objPivot);

  Sqrat::Object objTranslate = tmDesc.RawGetSlot(csk->translate);
  if (!objTranslate.IsNull())
    transform->translate = script_get_point2(objTranslate);

  Sqrat::Object objScale = tmDesc.RawGetSlot(csk->scale);
  if (!objScale.IsNull())
    transform->scale = script_get_point2(objScale);

  transform->rotate = DegToRad(tmDesc.RawGetSlotValue(csk->rotate, 0.0f));
}


template <typename T>
static bool are_tabs_equal(const Tab<T> &a, const Tab<T> &b)
{
  if (a.size() != b.size())
    return false;
  for (int i = 0, n = a.size(); i < n; ++i)
    if (a[i] != b[i])
      return false;
  return true;
}


void Element::readHotkeys(const Sqrat::Table &desc, const Sqrat::Array &hotkeys_data, const Hotkeys &hotkeys)
{
  dag::Vector<eastl::unique_ptr<HotkeyCombo>> prevCombos;

  hotkeyCombos.swap(prevCombos);
  hotkeys.loadCombos(csk, desc, hotkeys_data, hotkeyCombos);

  // maintain state
  for (auto &prev : prevCombos)
  {
    if (prev->isPressed)
    {
      bool removed = true;
      for (auto &cur : hotkeyCombos)
      {
        if (are_tabs_equal(cur->buttons, prev->buttons) && cur->eventName == prev->eventName)
        {
          cur->isPressed = true; // prev->isPressed;
          removed = false;
          break;
        }
      }
      if (removed)
        clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);
    }
  }

  prevCombos.clear();
}


void Element::setupElemGroup(const Sqrat::Table &desc_tbl)
{
  Sqrat::Object groupObj = desc_tbl.RawGetSlot(csk->group);

  if (groupObj.GetType() != OT_INSTANCE)
  {
    if (groupObj.GetType() != OT_NULL)
      darg_assert_trace_var(String(0, "Unexpected group field type %s (%X)", sq_objtypestr(groupObj.GetType()), groupObj.GetType()),
        desc_tbl, csk->group);
  }
  else
  {
    ElemGroup *newGroup = groupObj.Cast<ElemGroup *>();
    G_ASSERT(newGroup);

    if (newGroup != group)
    {
      releaseElemGroup();
      group = newGroup;
      if (group)
        group->addChild(this);
    }
  }
}


void Element::releaseElemGroup()
{
  if (group)
  {
    group->removeChild(this);
    group = nullptr;
  }
}


void Element::onElemGroupRelease() { group = nullptr; }


void Element::releaseWatch()
{
  for (sqfrp::BaseObservable *w : watch)
    w->unsubscribeWatcher(this);
  watch.clear();
}


template <typename Vec>
void Element::readWatches(const Sqrat::Table &desc_tbl, const Sqrat::Table &script_watches, Vec &out)
{
  G_ASSERT(out.empty());
  if (script_watches.GetType() == OT_ARRAY)
  {
    Sqrat::Array arr(script_watches);
    SQInteger len = arr.Length();

#if DAGOR_DBGLEVEL > 0
    static int numLenWarnings = 0;
    if (len > 100 && numLenWarnings < 5)
    {
      ++numLenWarnings;
      darg_assert_trace_var(String(0, "Watch array size %d is too big - most probably an error", int(len)), desc_tbl, csk->watch);
    }
#endif

    out.reserve(len);
    for (SQInteger i = 0; i < len; ++i)
    {
      BaseObservable *obs = try_cast_instance<BaseObservable>(arr.RawGetSlot(i), arr, i, "Reading 'watch' array");
      if (obs)
        out.push_back(obs);
    }
  }
  else
  {
    BaseObservable *obs = try_cast_instance<BaseObservable>(script_watches, desc_tbl, csk->watch, "Reading 'watch' field");
    if (obs)
      out.push_back(obs);
  }
}


void Element::setupWatch(const Sqrat::Table &desc_tbl)
{
  if (!etree->watchesAllowed)
    return;

  Sqrat::Object watchObj = desc_tbl.RawGetSlot(csk->watch);
  if (watchObj.IsNull() && watch.empty())
    return;

  dag::RelocatableFixedVector<BaseObservable *, 16, true, framemem_allocator> newWatches;
  if (!watchObj.IsNull())
    readWatches(desc_tbl, watchObj, newWatches);

  if (newWatches.size() == watch.size() && memcmp(newWatches.data(), watch.data(), watch.size() * sizeof(BaseObservable *)) == 0)
    return;

  if (props.scriptBuilder.IsNull() && !newWatches.empty())
  {
    darg_report_unrebuildable_element(this, "Component is not defined by function, but contains 'watch' field");
    newWatches.clear(); // prevent rebuild
  }

  for (sqfrp::BaseObservable *w : watch)
    w->unsubscribeWatcher(this);

  watch.assign(newWatches.begin(), newWatches.end());

  for (sqfrp::BaseObservable *w : watch)
    w->subscribeWatcher(this);
}


void Element::onObservableRelease(BaseObservable *observable)
{
  for (int i = watch.size() - 1; i >= 0; --i)
    if (watch[i] == observable)
      watch.erase(watch.begin() + i);
}


bool Element::onSourceObservableChanged()
{
  GuiScene::get_from_elem(this)->invalidateElement(this);
  return true;
}


Sqrat::Object Element::dbgGetWatcherScriptInstance()
{
  HSQUIRRELVM vm = props.scriptDesc.GetVM();
  return Sqrat::Object(getRef(vm), vm);
}


void Element::applyTransform(const GuiVertexTransform &prev_gvtm, GuiVertexTransform &res_gvtm) const
{
  res_gvtm.setViewTm(prev_gvtm.vtm);

  if (transform)
  {
    Color4 fontTex2rotCCSmS(0, 0, 0, 0);

    Point2 pivot = screenCoord.screenPos + Point2(transform->pivot.x * screenCoord.size.x, transform->pivot.y * screenCoord.size.y);

    Point2 translate = transform->getCurTranslate();
    Point2 scale = transform->getCurScale();
    float rotate = transform->getCurRotate();

    res_gvtm.setRotViewTm(fontTex2rotCCSmS, pivot.x + translate.x, pivot.y + translate.y, rotate, 0, true);
    res_gvtm.addViewTm(Point2(scale.x, 0), Point2(0, scale.y), -Point2(pivot.x * scale.x, pivot.y * scale.y) + pivot + translate);
  }
}


void Element::calcFullTransform(GuiVertexTransform &res_gvtm) const
{
  GuiVertexTransform baseTm;
  baseTm.resetViewTm();
  if (parent)
  {
    parent->calcFullTransform(baseTm);
    baseTm.addViewTm(Point2(1, 0), Point2(0, 1), -parent->screenCoord.scrollOffs);
  }
  applyTransform(baseTm, res_gvtm);
}


BBox2 Element::calcTransformedBbox() const
{
  GuiVertexTransform gvtm;
  calcFullTransform(gvtm);
  auto &sc = screenCoord;
  return scenerender::calcTransformedBbox(sc.screenPos, sc.screenPos + sc.size, gvtm);
}

BBox2 Element::calcTransformedBboxPadded() const
{
  GuiVertexTransform gvtm;
  calcFullTransform(gvtm);
  auto &sc = screenCoord;
  return scenerender::calcTransformedBbox(sc.screenPos + layout.padding.lt(), sc.screenPos + sc.size - layout.padding.rb(), gvtm);
}


bool Element::getNavAnchor(Point2 &val) const
{
  Sqrat::Array cursorNavAnchor = props.scriptDesc.RawGetSlot(csk->cursorNavAnchor);
  if (cursorNavAnchor.GetType() != OT_ARRAY)
    return false;

  SizeSpec sizeSpec[2];
  const char *errMsg = nullptr;
  if (!Layout::size_spec_from_array(cursorNavAnchor, sizeSpec, &errMsg))
    return false;

  val.x = sizeSpecToPixels(sizeSpec[0], 0, false);
  val.y = sizeSpecToPixels(sizeSpec[1], 1, false);
  return true;
}


bool Element::updateCtxTransform(StdGuiRender::GuiContext &ctx, GuiVertexTransform &prevGvtm, bool hier_break) const
{
  ElementTree::OverscrollMap::iterator itOverscroll = etree->overscroll.find(this);
  bool isOverscroll = itOverscroll != etree->overscroll.end();

  if (!transform && !hier_break && !isOverscroll)
    return false;

  GuiVertexTransform gvtm;
  if (hier_break)
    calcFullTransform(gvtm);
  else
    applyTransform(prevGvtm, gvtm);

  if (isOverscroll)
  {
    Color4 fontTex2rotCCSmS(0, 0, 0, 0);
    const ElementTree::OverscrollState &osState = itOverscroll->second;

    Point2 pivot = screenCoord.screenPos;
    Point2 translate = -osState.delta;
    Point2 scale(1, 1);

    gvtm.addViewTm(Point2(scale.x, 0), Point2(0, scale.y), -Point2(pivot.x * scale.x, pivot.y * scale.y) + pivot + translate);
  }

  ctx.setViewTm(gvtm.vtm);
  return true;
}


static const float opacity_transp_threshold = 0.9999f;


void Element::render(StdGuiRender::GuiContext &ctx, RenderList *rend_list, bool hier_break) const
{
  if (isHidden())
    return;

  float elemOpacity = props.getCurrentOpacity();
  if (elemOpacity < opacity_transp_threshold)
  {
    OpacityStackItem osi;
    osi.elem = this;
    osi.prevValue = rend_list->renderState.opacity;
    rend_list->opacityStack.push_back(osi);
    rend_list->renderState.opacity *= elemOpacity;
  }


  GuiVertexTransform prevGvtm;
  ctx.getViewTm(prevGvtm.vtm);

  bool transformSet = updateCtxTransform(ctx, prevGvtm, hier_break);
  if (transformSet)
    rend_list->renderState.transformActive = true;

  if (rendObj() && rend_list->renderState.opacity > 1e-5f)
  {
    ElemRenderData erd;
    erd.params = robjParams;
    erd.pos = screenCoord.screenPos;
    erd.size = screenCoord.size;
    rendObj()->renderCustom(ctx, this, &erd, rend_list->renderState);
  }


  if (screenCoord.scrollOffs.x != 0.0f || screenCoord.scrollOffs.y != 0.0f)
  {
    float vtm0[2][3];
    ctx.getViewTm(vtm0);

    GuiVertexTransform gvt;
    gvt.setViewTm(vtm0);

    gvt.addViewTm(Point2(1, 0), Point2(0, 1), -screenCoord.scrollOffs);

    ctx.setViewTm(gvt.vtm);
    transformSet = true;

    rend_list->renderState.transformActive = true;
  }


  if (transformSet)
  {
    TransformStackItem tsi;
    tsi.elem = this;
    tsi.gvtm = prevGvtm;
    rend_list->transformStack.push_back(tsi);
  }
}


#if DEBUG_XMB_OVERLAY

void Element::renderXmbOverlayDebug(StdGuiRender::GuiContext &ctx) const
{
  GuiScene *guiScene = GuiScene::get_from_elem(this);
  int focusInputStackDepth = -1, focusZOrder = -999;
  auto &stack = guiScene->getInputStack().stack;
  for (size_t iItem = 0, nItems = stack.size(); iItem < nItems; ++iItem)
  {
    const InputEntry &ie = stack[iItem];
    if (ie.elem == guiScene->getElementTree()->xmbFocus)
    {
      focusInputStackDepth = iItem;
      focusZOrder = ie.zOrder;
      break;
    }
  }

  if (hasFlags(F_STOP_MOUSE))
  {
    Point2 pos = screenCoord.screenPos;
    Point2 size = screenCoord.size;

    bool includeInCheck = false;
    int inputStackDepth = -1, zOrder = -999;
    for (size_t iItem = 0, nItems = stack.size(); iItem < nItems; ++iItem)
    {
      InputEntry &ie = stack[iItem];
      if (ie.elem == this)
      {
        inputStackDepth = iItem;

        if (ie.zOrder >= focusZOrder && (focusInputStackDepth >= 0 && iItem > focusInputStackDepth))
        {
          if (ie.elem->hasBehaviors(Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK) || ie.elem->hasFlags(Element::F_STOP_MOUSE))
            includeInCheck = true;
        }

        zOrder = ie.zOrder;
        break;
      }
    }


    if (includeInCheck)
      ctx.set_color(0, 30, 0, 30);
    else if (inputStackDepth >= 0)
      ctx.set_color(30, 15, 0, 30);
    else
      ctx.set_color(30, 0, 30, 30);
    ctx.render_box(pos, pos + size);

    if (includeInCheck)
      ctx.set_color(0, 130, 0, 130);
    else if (inputStackDepth >= 0)
      ctx.set_color(130, 50, 0, 130);
    else
      ctx.set_color(130, 0, 130, 130);
    ctx.render_frame(pos.x, pos.y, pos.x + size.x, pos.y + size.y, 4);

    ctx.set_font(0);
    BBox2 bbox = calcTransformedBbox();
    ctx.goto_xy(bbox.left() + 5, bbox.top() + 20);
    ctx.draw_str(String(0, "%.f, %.f | z = %d, depth = %d", bbox.left(), bbox.top(), zOrder, inputStackDepth));

    String strRB(0, "%.f, %.f", bbox.right(), bbox.bottom());
    StdGuiFontContext fctx;
    StdGuiRender::get_font_context(fctx, 0, 0);
    BBox2 strBox = StdGuiRender::get_str_bbox(strRB, strRB.length(), fctx);

    ctx.goto_xy(bbox.rightBottom() - strBox.width() - Point2(10, 4));
    ctx.draw_str(strRB, strRB.length());
  }

  if (this == guiScene->getElementTree()->xmbFocus)
  {
    ctx.set_font(0);
    BBox2 bbox = calcTransformedBbox();
    ctx.set_color(200, 200, 0, 200);
    ctx.goto_xy(bbox.left() + 5, bbox.top() + 20);
    ctx.draw_str(String(0, "z = %d, depth = %d", focusZOrder, focusInputStackDepth));
  }
}

#else

void Element::renderXmbOverlayDebug(StdGuiRender::GuiContext &ctx) const { G_UNUSED(ctx); }

#endif

void Element::postRender(StdGuiRender::GuiContext &ctx, RenderList *rend_list) const
{
  G_ASSERT(!hasFlags(F_LAYOUT_RECALC_PENDING_FIXED_SIZE | F_LAYOUT_RECALC_PENDING_FLOW | F_LAYOUT_RECALC_PENDING_SIZE));
#if DARG_USE_DBGCOLOR
  if (debug_frames_mode != DFM_NONE)
  {
    if (!dbgColor) // Not inited?
      dbgColor = E3DCOLOR(grnd() % 255, grnd() % 255, grnd() % 255);
    Point2 pos = screenCoord.screenPos;
    Point2 size = screenCoord.size;
    ctx.set_color(dbgColor);
    ctx.render_frame(pos.x, pos.y, pos.x + size.x, pos.y + size.y, 2);
  }
#endif

#ifdef DARG_DEBUG_FOCUS
  GuiScene *guiScene = GuiScene::get_from_elem(this);
  if (this == guiScene->getElementTree()->kbFocus)
  {
    Point2 pos = screenCoord.screenPos;
    Point2 size = screenCoord.size;
    ctx.set_color(120, 0, 0, 120);
    ctx.render_frame(pos.x, pos.y, pos.x + size.x, pos.y + size.y, 2);
    if (size.x > 6 && size.y > 6)
      ctx.render_frame(pos.x + 3, pos.y + 3, pos.x + size.x - 3, pos.y + size.y - 3, 2);
  }
#endif

  renderXmbOverlayDebug(ctx);

  if (auto ro = rendObj())
    ro->postRender(ctx, this);

  if (rend_list->transformStack.size() && rend_list->transformStack.back().elem == this)
  {
    ctx.setViewTm(rend_list->transformStack.back().gvtm.vtm); //-V522
    rend_list->transformStack.pop_back();

    rend_list->renderState.transformActive = !rend_list->transformStack.empty();
  }

  if (rend_list->opacityStack.size() && rend_list->opacityStack.back().elem == this) //-V522
  {
    rend_list->renderState.opacity = rend_list->opacityStack.back().prevValue; //-V522
    rend_list->opacityStack.pop_back();
  }
}


bool Element::hasBehaviors(int bhv_flags) const { return (behaviorsSummaryFlags & bhv_flags) != 0; }


static bool should_put_to_render(const Element *elem)
{
  // if (elem->rendObjType != ROBJ_NONE)
  //   return true;
  // if (elem->transform)
  //   return true;
  // if (elem->animations.size() || elem->transitions.size())
  //   return true;
  // float elemOpacity = elem->props.getCurrentOpacity();
  // if (elemOpacity < opacity_transp_threshold)
  //   return true;
  // return false;

  //// ^ this breaks input processing because screen boxes are calculated on render

  G_UNUSED(elem);
  return true;
}


static bool are_hotkeys_input_transparent(const dag::Vector<eastl::unique_ptr<HotkeyCombo>> &hotkey_combos)
{
  for (const auto &hc : hotkey_combos)
    if (!hc->inputPassive)
      return false;
  return true;
}


int16_t Element::getZOrder(int parent_z_order) const
{
  if (hasFlags(F_ZORDER_ON_TOP))
    return INT16_MAX;

  return zOrder <= undefined_z_order ? parent_z_order : zOrder;
}


void Element::putToSortedStacks(ElemStacks &stacks, ElemStackCounters &counters, int parent_z_order, bool parent_disable_input)
{
  if (isHidden())
    return;

  const int order = getZOrder(parent_z_order);
  bool disableInput = parent_disable_input || (flags & F_DISABLE_INPUT);
  bool putToRender = should_put_to_render(this);

  if (putToRender)
  {
    RenderEntry re;
    re.cmd = RCMD_ELEM_RENDER;
    re.elem = this;
    re.hierOrder = ++counters.hierOrderRender;
    re.zOrder = order;
    stacks.rlist->push(re);
  }

  if (flags & F_CLIP_CHILDREN)
  {
    RenderEntry re;
    re.cmd = RCMD_SET_VIEWPORT;
    re.elem = this;
    re.hierOrder = ++counters.hierOrderRender;
    re.zOrder = order;
    stacks.rlist->push(re);
  }

  const int bhvFlagsInput =
    Behavior::F_FOCUS_ON_CLICK | Behavior::F_HANDLE_KEYBOARD | Behavior::F_HANDLE_MOUSE | Behavior::F_HANDLE_TOUCH;

  const int elemInputFlags = F_STOP_HOVER | F_STOP_MOUSE | F_JOYSTICK_SCROLL | F_STOP_HOTKEYS;

  if (stacks.input && !disableInput && !isDetached())
  {
    bool hasInputBhv = hasBehaviors(bhvFlagsInput);
    if (hasInputBhv || hotkeyCombos.size() || (flags & elemInputFlags) || xmb != nullptr)
    {
      InputEntry ie;
      ie.elem = this;
      ie.hierOrder = ++counters.hierOrderInput;
      ie.zOrder = order;
      ie.inputPassive = hasFlags(F_INPUT_PASSIVE) || (!hasInputBhv && are_hotkeys_input_transparent(hotkeyCombos));
      stacks.input->push(ie);
    }
  }

  if (stacks.cursors && !disableInput && !isDetached()) // cursor is a marker of interactive element, so no need for disableInput
  {
    if (hasFlags(F_HAS_CURSOR | F_JOYSTICK_SCROLL))
    {
      InputEntry ie;
      ie.elem = this;
      ie.hierOrder = ++counters.hierOrderCursor;
      ie.zOrder = order;
      stacks.cursors->push(ie);
    }
  }

  if (stacks.eventHandlers && (flags & F_HAS_EVENT_HANDLERS))
  {
    InputEntry ie;
    ie.elem = this;
    ie.hierOrder = ++counters.hierOrderEvtH;
    ie.zOrder = order;
    stacks.eventHandlers->push(ie);
  }

  for (Element *child : children)
    child->putToSortedStacks(stacks, counters, order, disableInput);

  for (Element *fadingChild : fadeOutChildren)
  {
    ElemStacks fadeStacks;
    fadeStacks.rlist = stacks.rlist;
    fadingChild->putToSortedStacks(fadeStacks, counters, order, /*disable input*/ true);
  }

  if (flags & F_CLIP_CHILDREN)
  {
    RenderEntry re;
    re.cmd = RCMD_RESTORE_VIEWPORT;
    re.elem = this;
    re.hierOrder = ++counters.hierOrderRender;
    re.zOrder = order;
    stacks.rlist->push(re);
  }

  if (putToRender)
  {
    RenderEntry re;
    re.cmd = RCMD_ELEM_POSTRENDER;
    re.elem = this;
    re.hierOrder = ++counters.hierOrderRender;
    re.zOrder = order;
    stacks.rlist->push(re);
  }
}


void Element::traceHit(const Point2 &p, InputStack *stack, int parent_z_order, int &hier_order)
{
  if (isHidden())
    return;

  const int order = getZOrder(parent_z_order);

  if (hitTest(p))
  {
    InputEntry ie;
    ie.elem = this;
    ie.hierOrder = ++hier_order;
    ie.zOrder = order;
    stack->push(ie);
  }

  for (Element *child : children)
    child->traceHit(p, stack, order, hier_order);
}


float Element::sizeSpecToPixels(const SizeSpec &ss, int axis, bool use_min_max) const
{
  switch (ss.mode)
  {
    case SizeSpec::PIXELS: return ss.value;

    case SizeSpec::CONTENT: return screenCoord.contentSize[axis];

    case SizeSpec::FLEX: return 0;

    case SizeSpec::FONT_H: return calcFontHeightPercent(ss.value);

    case SizeSpec::PARENT_W: return calcParentW(ss.value, use_min_max);

    case SizeSpec::PARENT_H: return calcParentH(ss.value, use_min_max);

    case SizeSpec::ELEM_SELF_W: return screenCoord.size.x * ss.value * 0.01f;

    case SizeSpec::ELEM_SELF_H: return screenCoord.size.y * ss.value * 0.01f;

    default: G_ASSERTF(0, "Unexpected SizeSpec mode %d", ss.mode);
  }
  return 0;
}


Point2 Element::calcParentRelPos()
{
  Point2 res;
  for (int axis = 0; axis < 2; ++axis)
    res[axis] = sizeSpecToPixels(layout.pos[axis], axis, false);

  return res;
}


void Element::recalcScreenPositions()
{
  // screen coordinates
  if (parent)
    screenCoord.screenPos = screenCoord.relPos + parent->screenCoord.screenPos;
  else
    screenCoord.screenPos = screenCoord.relPos;

  if (!(flags & F_SUBPIXEL))
    screenCoord.screenPos = floor(screenCoord.screenPos);

  limitScrollOffset();

  for (Element *child : children)
    child->recalcScreenPositions();

  for (Behavior *bhv : behaviors)
    bhv->onRecalcLayout(this);

  afterRecalc();
}


void Element::calcFixedSizes()
{
  screenCoord.size.zero();
  screenCoord.contentSize.zero();

  // precalc fixed sizes.
  // when size is parent size, but parent is fixed size, this size is fixed too.
  for (int axis = 0; axis < 2; ++axis)
  {
    SizeSpec::Mode ssMode = layout.size[axis].mode;
    if (ssMode == SizeSpec::PIXELS || ssMode == SizeSpec::FONT_H || ssMode == SizeSpec::PARENT_W || ssMode == SizeSpec::PARENT_H)
      screenCoord.size[axis] = sizeSpecToPixels(layout.size[axis], axis, true);
    else if (ssMode == SizeSpec::FLEX && (!parent->isFlowAxis(axis) || parent->children.size() == 1))
      screenCoord.size[axis] = axis == 0 ? calcParentW(100, true) : calcParentH(100, true);
  }

  for (Element *child : children)
    child->calcFixedSizes();

  for (int axis = 0; axis < 2; ++axis)
  {
    screenCoord.contentSize[axis] = calcContentSizeByAxis(axis);
    SizeSpec::Mode ssMode = layout.size[axis].mode;
    if (ssMode == SizeSpec::CONTENT)
    {
      screenCoord.size[axis] = screenCoord.contentSize[axis];
      clampSizeToLimits(axis, screenCoord.size[axis]);
    }
    else if (ssMode == SizeSpec::FLEX && (parent->isFlowAxis(axis) && parent->children.size() != 1))
      screenCoord.size[axis] = sizeSpecToPixels(layout.minSize[axis], axis, false);
  }
}

void Element::calcSizeConstraints(int axis, float *sz_min, float *sz_max) const
{
  float szMin = sizeSpecToPixels(layout.minSize[axis], axis, false);
  float szMax = sizeSpecToPixels(layout.maxSize[axis], axis, false);

  if (szMin > 0 && szMax > 0 && szMax < szMin)
    szMax = szMin;

  *sz_min = szMin;
  *sz_max = szMax;
}

static float calc_flowing_spacing(int axis, const Element *prev_child, const Element *cur_child, float gap)
{
  return ::max(prev_child->layout.margin.rb()[axis], cur_child->layout.margin.lt()[axis]) + gap;
}

void Element::calcConstrainedSizes(int axis)
{
  if (children.size())
  {
    for (Element *child : children)
      if (child->layout.size[axis].mode == SizeSpec::CONTENT)
        child->calcConstrainedSizes(axis);

    if (layout.size[axis].mode == SizeSpec::CONTENT)
    {
      screenCoord.contentSize[axis] = calcContentSizeByAxis(axis);
      screenCoord.size[axis] = screenCoord.contentSize[axis];

      clampSizeToLimits(axis, screenCoord.size[axis]);
    }

    float freeSpace = screenCoord.size[axis];

    float flexWeightSum = 0;
    int numFlex = 0;
    Element *prevLayoutFlowChild = nullptr;

    for (Element *child : children)
    {
      const SizeSpec &ss = child->layout.size[axis];

      if (ss.mode == SizeSpec::PIXELS || ss.mode == SizeSpec::CONTENT || ss.mode == SizeSpec::FONT_H)
      {
        // size is already calculated
        if (isFlowAxis(axis))
          freeSpace -= child->screenCoord.size[axis];
      }
      else if (ss.mode == SizeSpec::PARENT_W || ss.mode == SizeSpec::PARENT_H)
      {
        child->screenCoord.size[axis] = child->sizeSpecToPixels(ss, axis, true);
        if (isFlowAxis(axis))
          freeSpace -= child->screenCoord.size[axis];
      }
      else if (ss.mode == SizeSpec::FLEX)
      {
        if (isFlowAxis(axis))
        {
          flexWeightSum += ss.value;
          ++numFlex;
        }
        else
          child->screenCoord.size[axis] = (axis == 0) ? child->calcParentW(100, true) : child->calcParentH(100, true);
      }
      else
        G_ASSERTF(0, "Unexpected size mode %d", ss.mode);

      if (isFlowAxis(axis) && ElementTree::does_element_affect_layout(child))
      {
        if (prevLayoutFlowChild)
          freeSpace -= calc_flowing_spacing(axis, prevLayoutFlowChild, child, layout.gap);
        else
          freeSpace -= ::max(layout.padding.lt()[axis], child->layout.margin.lt()[axis]);
        prevLayoutFlowChild = child;
      }
    }

    if (prevLayoutFlowChild)
      freeSpace -= ::max(layout.padding.rb()[axis], prevLayoutFlowChild->layout.margin.rb()[axis]);

    // apply flex
    if (numFlex && flexWeightSum > 0 && freeSpace > 0)
    {
      Tab<Element *> queue(framemem_ptr());
      queue.reserve(numFlex);
      for (Element *child : children)
      {
        SizeSpec::Mode sm = child->layout.size[axis].mode;
        if (sm == SizeSpec::FLEX)
          queue.push_back(child);
      }
      while (queue.size())
      {
        Element *exceeding = nullptr;
        for (int iq = 0, nq = queue.size(); iq < nq; ++iq)
        {
          Element *e = queue[iq];
          const SizeSpec &sizeSpec = e->layout.size[axis];
          float szPx = 0;
          if (sizeSpec.mode == SizeSpec::FLEX)
            szPx = floorf(freeSpace * sizeSpec.value / flexWeightSum + 0.5f);
          else
            G_ASSERT(0);

          float szMin, szMax;
          e->calcSizeConstraints(axis, &szMin, &szMax);

          if (szMax > 0 && szPx > szMax)
          {
            e->screenCoord.size[axis] = szMax;
            exceeding = e;
          }
          else if (szMin > 0 && szPx < szMin)
          {
            e->screenCoord.size[axis] = szMin;
            exceeding = e;
          }
          if (exceeding)
          {
            erase_items(queue, iq, 1);
            freeSpace -= e->screenCoord.size[axis];
            if (sizeSpec.mode == SizeSpec::FLEX)
            {
              flexWeightSum -= sizeSpec.value;
              --numFlex;
            }
            else
              G_ASSERT(0);

            break;
          }
        }
        if (exceeding)
          continue;

        float spaceLeft = freeSpace;
        for (int iq = 0, nq = queue.size(); iq < nq; ++iq)
        {
          Element *child = queue[iq];
          const SizeSpec &sizeSpec = child->layout.size[axis];

          if (iq == nq - 1)
          {
            child->screenCoord.size[axis] = spaceLeft;
          }
          else if (sizeSpec.mode == SizeSpec::FLEX)
          {
            float sz = floorf(freeSpace * sizeSpec.value / flexWeightSum + 0.5f);
            child->screenCoord.size[axis] = sz;
            spaceLeft -= sz;
          }
          else
            G_ASSERT(0);
        }
        break;
      }
    }

    for (Element *child : children)
      if (child->layout.size[axis].mode != SizeSpec::CONTENT)
        child->calcConstrainedSizes(axis);
  }

  if (children.empty() || layout.size[axis].mode != SizeSpec::CONTENT)
  {
    screenCoord.contentSize[axis] = calcContentSizeByAxis(axis);
    if (layout.size[axis].mode == SizeSpec::CONTENT)
      screenCoord.size[axis] = screenCoord.contentSize[axis];

    clampSizeToLimits(axis, screenCoord.size[axis]);
  }
  alignChildren(axis);
}


void Element::clampSizeToLimits(int axis, float &sz_px)
{
  float szMin, szMax;
  calcSizeConstraints(axis, &szMin, &szMax);
  if (szMin > 0 && sz_px < szMin)
    sz_px = szMin;
  else if (szMax > 0 && sz_px > szMax)
    sz_px = szMax;
}


void Element::alignChildren(int axis)
{
  if (children.empty())
    return;

  if (isFlowAxis(axis))
  {
    Element *firstLayoutChild = nullptr, *lastLayoutChild = nullptr;

    float offs = 0;

    for (Element *child : children)
    {
      if (ElementTree::does_element_affect_layout(child))
      {
        if (lastLayoutChild)
          offs += calc_flowing_spacing(axis, lastLayoutChild, child, layout.gap);
        if (!firstLayoutChild)
          firstLayoutChild = child;
        lastLayoutChild = child;
      }
      child->screenCoord.relPos[axis] = offs;
      offs += child->screenCoord.size[axis];
    }

    if (lastLayoutChild) // had any layout-affecting children
    {
      const ScreenCoord &scLast = lastLayoutChild->screenCoord;
      float joinedChildrenSize = scLast.relPos[axis] + scLast.size[axis];

      float commonOffs = 0;
      ElemAlign align = axis == 0 ? layout.hAlign : layout.vAlign;
      if (align == ALIGN_LEFT_OR_TOP)
        commonOffs = ::max(firstLayoutChild->layout.margin.lt()[axis], layout.padding.lt()[axis]);
      else if (align == ALIGN_CENTER)
        commonOffs = (screenCoord.size[axis] - joinedChildrenSize) / 2 + (layout.padding.lt()[axis] - layout.padding.rb()[axis]) / 2;
      else if (align == ALIGN_RIGHT_OR_BOTTOM)
        commonOffs =
          screenCoord.size[axis] - joinedChildrenSize - ::max(lastLayoutChild->layout.margin.rb()[axis], layout.padding.rb()[axis]);

      for (Element *child : children)
        child->screenCoord.relPos[axis] += commonOffs;
    }
  }
  else
  {
    for (Element *child : children)
    {
      ScreenCoord &chsc = child->screenCoord;

      ElemAlign place;
      if (axis == 0)
        place = (child->layout.hPlacement != PLACE_DEFAULT) ? child->layout.hPlacement : layout.hAlign;
      else
        place = (child->layout.vPlacement != PLACE_DEFAULT) ? child->layout.vPlacement : layout.vAlign;

      float relPos = 0;
      if (place == ALIGN_LEFT_OR_TOP)
        relPos = ::max(child->layout.margin.lt()[axis], layout.padding.lt()[axis]);
      else if (place == ALIGN_RIGHT_OR_BOTTOM)
        relPos = screenCoord.size[axis] - chsc.size[axis] - ::max(child->layout.margin.rb()[axis], layout.padding.rb()[axis]);
      else if (place == ALIGN_CENTER)
        relPos = (screenCoord.size[axis] - chsc.size[axis]) / 2 + (layout.padding.lt()[axis] - layout.padding.rb()[axis]) / 2;
      else
        G_ASSERTF(0, "Unexpected alignment %d", place);

      relPos += child->calcParentRelPos()[axis];

      chsc.relPos[axis] = relPos;
    }
  }
}


bool Element::isFlowAxis(int axis) const
{
  if (layout.flowType == FLOW_PARENT_RELATIVE)
    return false;
  return axis == (layout.flowType == FLOW_HORIZONTAL ? 0 : 1);
}


void Element::recalcContentSize(int axis) { screenCoord.contentSize[axis] = calcContentSizeByAxis(axis); }


float Element::calcContentSizeByAxis(int axis)
{
  float contentSize = 0;

  if (isFlowAxis(axis))
  {
    const Element *prevLayoutChild = nullptr;
    for (const Element *child : children)
    {
      if (!ElementTree::does_element_affect_layout(child))
        continue;

      contentSize += child->screenCoord.size[axis];

      if (prevLayoutChild)
        contentSize += calc_flowing_spacing(axis, prevLayoutChild, child, layout.gap);
      else
        contentSize += ::max(layout.padding.lt()[axis], child->layout.margin.lt()[axis]);

      prevLayoutChild = child;
    }

    if (prevLayoutChild)
      contentSize += (::max(layout.padding.rb()[axis], prevLayoutChild->layout.margin.rb()[axis]));
  }
  else
  {
    for (Element *child : children)
    {
      float childSize = child->screenCoord.size[axis] + ::max(layout.padding.lt()[axis], child->layout.margin.lt()[axis]) +
                        ::max(layout.padding.rb()[axis], child->layout.margin.rb()[axis]);

      if (child->layout.minSize[axis].mode == SizeSpec::CONTENT)
      {
        float childContentSize = child->screenCoord.contentSize[axis] +
                                 ::max(layout.padding.lt()[axis], child->layout.margin.lt()[axis]) +
                                 ::max(layout.padding.rb()[axis], child->layout.margin.rb()[axis]);
        childSize = ::max(childSize, childContentSize);
      }

      contentSize = ::max(contentSize, childSize);
    }
  }

  if (rendObjType == rendobj_text_id)
  {
    RobjParamsText *rparams = (RobjParamsText *)robjParams;

    if (textSizeCache.x < 0)
    {
      if (!rparams->passChar)
        textSizeCache =
          calc_text_size(props.text, props.text.length(), rparams->fontId, rparams->spacing, rparams->monoWidth, rparams->fontHt);
      else
      {
        int chars = utf8::distance(props.text.c_str(), props.text.c_str() + props.text.length());
        Tab<wchar_t> wtext(framemem_ptr());
        wtext.resize(chars);
        for (int i = 0; i < chars; i++)
          wtext[i] = rparams->passChar;
        textSizeCache = calc_text_size_u(wtext.data(), chars, rparams->fontId, rparams->spacing, rparams->monoWidth, rparams->fontHt);
      }
    }

    Point2 size = textSizeCache;
    size.x += 1; // for caret
    float axisSz = size[axis] + layout.padding.lt()[axis] + layout.padding.rb()[axis];

    contentSize = ::max(contentSize, axisSz);
  }
  if (rendObjType == rendobj_inscription_id)
  {
    RobjParamsInscription *params = (RobjParamsInscription *)robjParams;
    if (params)
    {
      BBox2 box;
      if (params->inscription)
        box = StdGuiRender::get_inscription_size(params->inscription);
      float w = box.isempty() ? 0 : box.width().x;
      Point2 size(w, params->fullHeight);
      contentSize = ::max(contentSize, size[axis] + layout.padding.lt()[axis] + layout.padding.rb()[axis]);
    }
  }
  else if (rendObjType == rendobj_textarea_id)
  {
    Point2 textSize;
    BhvTextArea::recalc_content(this, axis, screenCoord.size, textSize);
    contentSize = ::max(contentSize, textSize[axis]);
  }
  else if (rendObjType == rendobj_image_id)
  {
    const RobjParamsImage *rp = static_cast<const RobjParamsImage *>(robjParams);
    if (rp && rp->image && rp->imageAffectsLayout && layout.size[axis].mode == SizeSpec::CONTENT)
    {
      Point2 picSz = PictureManager::get_picture_pix_size(rp->image->getPic().pic);
      if (picSz.x > 0 && picSz.y > 0)
      {
        int otherAxis = 1 - axis;
        if (layout.size[otherAxis].mode == SizeSpec::PIXELS) // keep aspect ratio
        {
          float sz = floorf(layout.size[otherAxis].value * picSz[axis] / picSz[otherAxis] + 0.5f);
          contentSize = ::max(contentSize, sz);
        }
        else if (layout.size[otherAxis].mode == SizeSpec::CONTENT)
        {
          float sz = picSz[axis];
          contentSize = ::max(contentSize, sz);
        }
      }
    }
  }

  return contentSize;
}


float Element::calcParentW(float percent, bool use_min_max) const
{
  if (!parent)
    return 0;

  float available = parent->screenCoord.size[0] - ::max(layout.margin.left(), parent->layout.padding.left()) -
                    ::max(layout.margin.right(), parent->layout.padding.right());

  if (parent->layout.flowType == FLOW_HORIZONTAL)
    available -= parent->layout.gap * (parent->children.size() - 1);

  float res = floorf(::max(0.0f, available) * percent * 0.01f + 0.5f);

  if (use_min_max)
  {
    float minSize = sizeSpecToPixels(layout.minSize[0], 0, false);
    float maxSize = sizeSpecToPixels(layout.maxSize[0], 0, false);
    if (maxSize > 0.0f)
      res = ::min(res, maxSize);
    if (minSize > 0.0f)
      res = ::max(res, minSize);
  }

  return res;
}


float Element::calcParentH(float percent, bool use_min_max) const
{
  if (!parent)
    return 0;

  float available = parent->screenCoord.size[1] - ::max(layout.margin.top(), parent->layout.padding.top()) -
                    ::max(layout.margin.bottom(), parent->layout.padding.bottom());

  if (parent->layout.flowType == FLOW_VERTICAL)
    available -= parent->layout.gap * (parent->children.size() - 1);

  float res = floorf(::max(0.0f, available) * percent * 0.01f + 0.5f);

  if (use_min_max)
  {
    float minSize = sizeSpecToPixels(layout.minSize[1], 1, false);
    float maxSize = sizeSpecToPixels(layout.maxSize[1], 1, false);
    if (maxSize > 0.0f)
      res = ::min(res, maxSize);
    if (minSize > 0.0f)
      res = ::max(res, minSize);
  }

  return res;
}


float Element::calcWidthPercent(float percent) const { return ceilf(screenCoord.size.x * percent * 0.01f); }


float Element::calcHeightPercent(float percent) const { return ceilf(screenCoord.size.y * percent * 0.01f); }


float Element::calcFontHeightPercent(float mul) const
{
  float lineSpacing = StdGuiRender::get_font_line_spacing(props.getFontId(), props.getFontSize());
  return ceilf(lineSpacing * mul * 0.01f);
}


void Element::setupAnimations(const Sqrat::Table &desc)
{
  Sqrat::Array arr = desc.RawGetSlot(csk->animations);
  if (arr.IsNull())
    return;

  if (arr.GetType() != OT_ARRAY)
  {
    darg_assert_trace_var(String(0, "Invalid animations type %X (%s), array expected", arr.GetType(), sq_objtypestr(arr.GetType())),
      desc, csk->animations);
    return;
  }

  G_ASSERT(animations.empty());
  G_ASSERT(hierNumFadeOutAnims == 0);

  AnimDesc ad;

  int nDesc = arr.Length();
  animations.reserve(nDesc);
  for (int iDesc = 0; iDesc < nDesc; ++iDesc)
  {
    Sqrat::Object itemDesc = arr.RawGetSlot(iDesc);
    if (itemDesc.GetType() == OT_NULL)
      continue;
    if (itemDesc.GetType() != OT_TABLE)
    {
      darg_assert_trace_var(
        String(0, "Invalid animation type %X (%s), table expected", itemDesc.GetType(), sq_objtypestr(itemDesc.GetType())), itemDesc,
        iDesc);
      continue;
    }

    ad.reset();
    if (!ad.load(itemDesc, csk))
      continue;

    Animation *anim = create_anim(ad, this);
    if (anim)
      animations.emplace_back(anim);

    if (ad.playFadeOut)
      ++hierNumFadeOutAnims;
  }

  if (hierNumFadeOutAnims)
  {
    for (Element *e = parent; e; e = e->parent)
      e->hierNumFadeOutAnims += hierNumFadeOutAnims;
  }
}


void Element::setupTransitions(const Sqrat::Table &desc)
{
  Sqrat::Object tCfg = desc.RawGetSlot(csk->transitions);

  if (tCfg.IsNull())
    return;

  SQObjectType cfgType = tCfg.GetType();
  if (cfgType == OT_ARRAY)
  {
    Sqrat::Array arr = tCfg;

    SQInteger arrSize = arr.Length();

    for (int iExisting = transitions.size() - 1; iExisting >= 0; --iExisting)
    {
      bool keep = false;
      for (SQInteger iNew = 0; iNew < arrSize; ++iNew)
      {
        Sqrat::Table item = arr.RawGetSlot(iNew);
        AnimProp prop = item.RawGetSlotValue(csk->prop, AP_INVALID);

        if (prop == transitions[iExisting].prop)
        {
          keep = true;
          break;
        }
      }

      if (!keep)
        transitions.erase(transitions.begin() + iExisting);
    }

    for (SQInteger iNew = 0; iNew < arrSize; ++iNew)
      addOrUpdateTransition(arr.RawGetSlot(iNew));
  }
  else if (cfgType == OT_INSTANCE)
  {
    AllTransitionsConfig *allTrCfg = tCfg.Cast<AllTransitionsConfig *>();
    if (!allTrCfg)
      darg_assert_trace_var("Invalid class for transitions item", desc, csk->transitions);
    else
    {
      G_ASSERT_RETURN(robjParams, );
      float *fVal;
      E3DCOLOR *colorVal;
      Point2 *p2Val;
      for (int prop = FIRST_ANIM_PROP; prop < NUM_ANIM_PROPS; ++prop)
      {
        AnimProp aprop = (AnimProp)prop;
        if (aprop == AP_OPACITY || (this->transform && (prop == AP_TRANSLATE || prop == AP_ROTATE || prop == AP_SCALE)) ||
            robjParams->getAnimFloat(aprop, &fVal) || robjParams->getAnimColor(aprop, &colorVal) ||
            robjParams->getAnimPoint2(aprop, &p2Val))
        {
          addOrUpdateTransition(allTrCfg->cfg, aprop);
        }
      }
    }
  }
  else
  {
    darg_assert_trace_var(String(32, "Unexpected transitions field type %s (%X)", sq_objtypestr(cfgType), cfgType), desc,
      csk->transitions);
  }
}


void Element::addOrUpdateTransition(const Sqrat::Object &item, AnimProp prop_override)
{
  AnimProp prop = prop_override != AP_INVALID ? prop_override : item.RawGetSlotValue(csk->prop, AP_INVALID);

  auto it = eastl::find_if(transitions.begin(), transitions.end(), [prop](Transition &tr) { return tr.prop == prop; });
  if (it == transitions.end())
    it = &transitions.emplace_back(prop, this);

  it->setup(item);
}


void Element::scroll(const Point2 &delta, Point2 *overscroll)
{
  Point2 prevScroll = screenCoord.scrollOffs;

  scrollTo(screenCoord.scrollOffs + delta);

  if (overscroll)
  {
    Point2 actualDelta = screenCoord.scrollOffs - prevScroll;
    Point2 over = delta - actualDelta;

    if (getScrollRange(0).length() < 1)
      over.x = 0;
    if (getScrollRange(1).length() < 1)
      over.y = 0;

    *overscroll = over;
  }
}


void Element::scrollTo(const Point2 &offs)
{
  screenCoord.scrollOffs = offs;
  limitScrollOffset();
}


Point2 Element::getScrollRange(int axis) const
{
  float range = max(0.0f, screenCoord.contentSize[axis] - screenCoord.size[axis]);
  if ((axis == 0 && layout.hAlign == ALIGN_LEFT_OR_TOP) || (axis == 1 && layout.vAlign == ALIGN_LEFT_OR_TOP))
    return Point2(0, range);
  if ((axis == 0 && layout.hAlign == ALIGN_RIGHT_OR_BOTTOM) || (axis == 1 && layout.vAlign == ALIGN_RIGHT_OR_BOTTOM))
    return Point2(-range, 0);
  else
  {
    float half = floorf(range / 2);
    return Point2(-half, range - half);
  }
}


void Element::limitScrollOffset()
{
  Point2 rangeX = getScrollRange(0);
  Point2 rangeY = getScrollRange(1);

  screenCoord.scrollOffs.x = clamp(screenCoord.scrollOffs.x, rangeX[0], rangeX[1]);
  screenCoord.scrollOffs.y = clamp(screenCoord.scrollOffs.y, rangeY[0], rangeY[1]);
}


void Element::setElemStateFlags(int f)
{
  G_ASSERTF(f != S_ACTIVE, "S_????_ACTIVE flags should be set/cleared independently");

  int prevSf = stateFlags;
  stateFlags |= f;

  if (stateFlags != prevSf)
  {
    delayedCallElemStateHandler();
    playStateChangeSound(prevSf, stateFlags);
  }
}


void Element::clearElemStateFlags(int f)
{
  G_ASSERTF(f != S_ACTIVE, "S_????_ACTIVE flags should be set/cleared independently");

  int prevSf = stateFlags;
  stateFlags &= ~f;

  if (stateFlags != prevSf)
  {
    delayedCallElemStateHandler();
    playStateChangeSound(prevSf, stateFlags);
  }
}


void Element::setGroupStateFlags(int f)
{
  G_ASSERTF(f != S_ACTIVE, "S_????_ACTIVE flags should be set/cleared independently");

  if (group)
    group->setStateFlags(f);
  else
    setElemStateFlags(f);
}


void Element::clearGroupStateFlags(int f)
{
  G_ASSERTF(f != S_ACTIVE, "S_????_ACTIVE flags should be set/cleared independently");

  if (group)
    group->clearStateFlags(f);
  else
    clearElemStateFlags(f);
}


void Element::playStateChangeSound(int prev_flags, int cur_flags)
{
  const Sqrat::Object *key = nullptr;

  if ((cur_flags & S_ACTIVE) && !(prev_flags & S_ACTIVE))
    key = &csk->active;
  else if ((cur_flags & S_DRAG) && !(prev_flags & S_DRAG))
    key = &csk->drag;
  else if ((cur_flags & S_TOP_HOVER) && !(prev_flags & S_TOP_HOVER))
    key = &csk->hover;

  Sqrat::string sndName;
  Sqrat::Object params;
  float vol;

  if (key && props.getSound(csk, *key, sndName, params, vol))
  {
    Point2 sndPos;
    bool posOk = calcSoundPos(sndPos);

    ui_sound_player->play(sndName.c_str(), params, vol, posOk ? &sndPos : nullptr);
  }
}


void Element::playSound(const Sqrat::Object &key)
{
  Sqrat::string sndName;
  Sqrat::Object params;
  float vol;
  if (props.getSound(csk, key, sndName, params, vol))
  {
    Point2 sndPos;
    bool posOk = calcSoundPos(sndPos);

    ui_sound_player->play(sndName.c_str(), params, vol, posOk ? &sndPos : nullptr);
  }
}


bool Element::calcSoundPos(Point2 &res)
{
  if (!(flags & F_SIZE_CALCULATED))
    return false;

  // use full screen size for now - probably it is always ok
  Point2 center = screenCoord.screenPos + screenCoord.size * 0.5;

  res.x = center.x / StdGuiRender::screen_width() * 2 - 1;
  res.y = center.y / StdGuiRender::screen_height() * 2 - 1;
  return true;
}


bool Element::hitTest(const Point2 &p) const
{
  //  const Point2 &lt = screenCoord.screenPos;
  //  Point2 rb = lt + screenCoord.size;
  //  return (p.x >= lt.x && p.y >= lt.y && p.x <= rb.x && p.y <= rb.y);
  return !bboxIsClippedOut() && (clippedScreenRect & p);
}

static float dist_to_box(const BBox2 &box, const Point2 &p)
{
  float dx = 0, dy = 0;

  if (p.x < box.left())
    dx = p.x - box.left();
  else if (p.x > box.right())
    dx = p.x - box.right();

  if (p.y < box.top())
    dy = p.y - box.top();
  else if (p.y > box.bottom())
    dy = p.y - box.bottom();
  return sqrtf(dx * dx + dy * dy);
}

bool should_use_outside_area(InputDevice device) { return device == DEVID_TOUCH || button_margin_for_mouse; }

bool Element::hitTestWithTouchMargin(const Point2 &p, InputDevice device) const
{
  if (bboxIsClippedOut())
    return false;

  if (hitTest(p))
    return true;

  if (!should_use_outside_area(device))
    return false;

  if (etree->guiScene->config.buttonTouchMargin <= 0)
    return false;

  if (props.getBool(csk->eventPassThrough, false))
  {
    BBox2 box = clippedScreenRect;
    box.inflate(etree->guiScene->config.buttonTouchMargin);
    return box & p;
  }

  InputStack &stack = etree->guiScene->getInputStack();

  const Element *nearestButton = nullptr;
  float minDist = etree->guiScene->config.buttonTouchMargin;

  for (InputStack::Stack::iterator it = stack.stack.begin(); it != stack.stack.end(); ++it)
  {
    const Element *e = it->elem;

    if (e->hasBehaviors(Behavior::F_HANDLE_TOUCH | (button_margin_for_mouse ? Behavior::F_HANDLE_MOUSE : 0)) &&
        !e->bboxIsClippedOut() && !e->props.getBool(csk->eventPassThrough, false))
    {
      bool usesMargin = false;
      for (auto &bhv : e->behaviors)
      {
        usesMargin = usesMargin || (bhv->flags & Behavior::Flags::F_USES_TOUCH_MARGIN);
      }
      if (usesMargin)
      {
        float distance = dist_to_box(e->clippedScreenRect, p);
        if (distance < minDist)
        {
          minDist = distance;
          nearestButton = e;
        }
      }
    }

    if (e->hasFlags(Element::F_STOP_MOUSE))
    {
      BBox2 box = e->clippedScreenRect;
      box.inflate(etree->guiScene->config.buttonTouchMargin);
      if (box & p)
        break;
    }
  }

  return nearestButton == this;
}

bool Element::hasFadeOutAnims() const
{
  for (const auto &anim : animations)
    if (anim->desc.playFadeOut)
      return true;
  return false;
}


bool Element::fadeOutFinished() const
{
  G_ASSERT(hierNumFadeOutAnims >= 0);
  if (hierNumFadeOutAnims == 0)
    return true;

  int nFadeOutFinished = 0;
  for (const auto &anim : animations)
  {
    if (anim->desc.playFadeOut)
    {
      if (!anim->isFinished())
        return false;
      ++nFadeOutFinished;
    }
  }
  G_ASSERT(nFadeOutFinished <= hierNumFadeOutAnims);
  return nFadeOutFinished == hierNumFadeOutAnims;
}


void Element::startFadeOutAnims()
{
  for (auto &anim : animations)
  {
    if (anim->desc.playFadeOut)
    {
      G_ASSERT(anim->isFinished());
      anim->start();
    }
  }

  for (Element *child : children)
    child->startFadeOutAnims();
}


void Element::updateFadeOutCountersBeforeDetach()
{
  if (this->hierNumFadeOutAnims)
  {
    for (Element *p = this->parent; p; p = p->parent)
    {
      G_ASSERTF(p->hierNumFadeOutAnims > 0, "parent hierNumFadeOutAnims = %d, own = %d", p->hierNumFadeOutAnims,
        this->hierNumFadeOutAnims);
      p->hierNumFadeOutAnims -= this->hierNumFadeOutAnims;
      G_ASSERTF(p->hierNumFadeOutAnims >= 0, "parent hierNumFadeOutAnims = %d, own = %d", p->hierNumFadeOutAnims,
        this->hierNumFadeOutAnims);
    }
  }
}


Element *Element::findChildByScriptCtor(const Sqrat::Object &ctor) const
{
  if (ctor.IsNull())
    return NULL;

  for (Element *child : children)
  {
    if (!child->props.scriptBuilder.IsNull())
    {
      if (ctor.IsEqual(child->props.scriptBuilder))
        return child;
    }
    else
    {
      if (ctor.IsEqual(props.scriptDesc))
        return child;
    }
  }

  return NULL;
}


void Element::recalcLayout()
{
  AutoProfileScope profile(GuiScene::get_from_elem(this)->getProfiler(), M_RECALC_LAYOUT);
  TIME_PROFILE(elem_recalcLayout);

  Element *fixedSizeRoot, *sizeRoot, *flowRoot;
  getSizeRoots(fixedSizeRoot, sizeRoot, flowRoot);

  etree->guiScene->recalcLayoutFromRoots(make_span(&fixedSizeRoot, 1), make_span(&sizeRoot, 1), make_span(&flowRoot, 1));
}


static bool isElementAffectedByChildren(const Element *e)
{
  return (e->layout.size[0].mode == SizeSpec::CONTENT || e->layout.size[1].mode == SizeSpec::CONTENT);
}


void Element::getSizeRoots(Element *&fixed_size_root, Element *&size_root, Element *&flow_root)
{
  TIME_PROFILE(elem_getSizeRoots);
  int flowRootDistance = 0, fixedSizeRootDistance = 0;

  Element *flowRoot = this;
  for (; flowRoot->parent && ((flowRoot->parent->flags & F_FLOWING) || flowRoot->layout.hPlacement != ALIGN_LEFT ||
                               flowRoot->layout.vPlacement != ALIGN_TOP);)
  {
    ++flowRootDistance;
    flowRoot = flowRoot->parent;
    if (!isElementAffectedByChildren(flowRoot))
      break;
  }

  Element *sizeRoot = (parent ? parent : this);
  Element *fixedSizeRoot = this;
  for (; sizeRoot->parent && isElementAffectedByChildren(sizeRoot);)
  {
    ++fixedSizeRootDistance;
    fixedSizeRoot = sizeRoot;
    sizeRoot = sizeRoot->parent;
  }

  // ensure that all Elements' sizes are caclculated before recalcScreenPositions() call on them
  if (fixedSizeRootDistance < flowRootDistance)
  {
    fixedSizeRoot = flowRoot;
    sizeRoot = fixedSizeRoot->parent ? fixedSizeRoot->parent : fixedSizeRoot;
  }

  fixed_size_root = fixedSizeRoot;
  size_root = sizeRoot;
  flow_root = flowRoot;
}


void Element::callHoverHandler(bool hover_on)
{
  SqStackChecker check(props.scriptDesc.GetVM());

  Sqrat::Object handlerObj = props.scriptDesc.RawGetSlot(csk->onHover);
  if (!handlerObj.IsNull())
  {
    HSQUIRRELVM vm = handlerObj.GetVM();
    Sqrat::Function f(vm, Sqrat::Object(vm), handlerObj);

    SQInteger nparams = 0, nfreevars = 0;
    G_VERIFY(get_closure_info(f, &nparams, &nfreevars));

    GuiScene *guiScene = GuiScene::get_from_elem(this);
    if (nparams == 3)
    {
      HoverEventData evt;
      calc_elem_rect_for_event_handler(evt.targetRect, this);
      guiScene->queueScriptHandler(new ScriptHandlerSqFunc<bool, HoverEventData>(f, hover_on, evt));
    }
    else
      guiScene->queueScriptHandler(new ScriptHandlerSqFunc<bool>(f, hover_on));
  }
}


void Element::afterRecalc()
{
  if (!(flags & F_SIZE_CALCULATED))
  {
    flags |= F_SIZE_CALCULATED;

    playSound(csk->attach); //< put here to ensure valid coordinates
  }
}


void Element::validateStaticText()
{
#if DAGOR_DBGLEVEL > 0
  if (rendObjType == rendobj_inscription_id)
  {
    Sqrat::Object lastKey = props.storage.RawGetSlot(csk->stextLastKey);
    bool needToCheck = props.getBool(csk->validateStaticText, true);
    if (!needToCheck)
      return;

    if (!lastKey.IsEqual(props.uniqueKey))
    {
      props.storage.DeleteSlot(csk->stextLastVal);
      props.storage.DeleteSlot(csk->stextChangeCount);
    }
    else
    {
      Sqrat::Object prevTextObj = props.storage.RawGetSlot(csk->stextLastVal);

      if (!prevTextObj.IsNull())
      {
        Sqrat::Var<const char *> prevTextVar = prevTextObj.GetVar<const char *>();
        if (props.text != prevTextVar.value)
        {
          int prevChangeCount = props.storage.RawGetSlotValue(csk->stextChangeCount, 0);
          props.storage.SetValue(csk->stextChangeCount, prevChangeCount + 1);
          if (prevChangeCount == 10)
            logerr_ctx("daRg: static text value changed %d times, last valid value = %s", prevChangeCount + 1, prevTextVar.value);
          if (prevChangeCount >= 10)
          {
            props.text = "#ERR";
            discard_text_cache(robjParams);
            props.scriptDesc.SetValue(csk->text, "#ERR");
          }
        }
      }
      props.storage.SetValue(csk->stextLastVal, props.text.c_str());
    }

    props.storage.SetValue(csk->stextLastKey, props.uniqueKey);
  }
#endif
}


bool Element::isDirPadNavigable() const
{
  if (!hasBehaviors(Behavior::F_HANDLE_KEYBOARD | Behavior::F_HANDLE_MOUSE | Behavior::F_FOCUS_ON_CLICK))
    return false;

  if (hasFlags(F_SKIP_DIRPAD_NAV))
    return false;

  return true;
}


Element *Element::findElementByKey(const Sqrat::Object &key)
{
  if (props.uniqueKey.IsEqual(key))
    return this;
  Element *res;
  for (Element *child : children)
  {
    res = child->findElementByKey(key);
    if (res)
      return res;
  }
  return nullptr;
}


int Element::getAvailableScrolls() const
{
  int scrollFlags = 0;
  if (screenCoord.contentSize.x > screenCoord.size.x)
    scrollFlags |= O_HORIZONTAL;
  if (screenCoord.contentSize.y > screenCoord.size.y)
    scrollFlags |= O_VERTICAL;
  return scrollFlags;
}


Sqrat::Object Element::getRef(HSQUIRRELVM vm)
{
  if (!ref)
  {
    ref = new ElementRef();
    ref->elem = this;
  }
  G_ASSERT(ref->scriptInstancesCount >= 0);
  return ref->createScriptInstance(vm);
}

Sqrat::Table Element::getElementBBox(HSQUIRRELVM vm)
{
  Sqrat::Table bbox(vm);
  bbox.SetValue("x", transformedBbox[0].x);
  bbox.SetValue("y", transformedBbox[0].y);
  bbox.SetValue("w", transformedBbox.width().x);
  bbox.SetValue("h", transformedBbox.width().y);
  return bbox;
}

static void addDebugElementDescription(HSQUIRRELVM vm, Element *element, const char *name, Sqrat::Array &table)
{
  if (element == nullptr)
    return;

  String builderFuncName;
  get_closure_full_name(element->props.scriptBuilder, builderFuncName);

  Sqrat::Table desc(vm);
  desc.SetValue("name", name);
  desc.SetValue("elem", element->getRef(vm));
  desc.SetValue("builderFuncName", builderFuncName.c_str());
  desc.SetValue("bbox", element->getElementBBox(vm));
  table.Append(desc);
}

Sqrat::Table Element::getElementInfo(HSQUIRRELVM vm)
{
  String builderFuncName;
  get_closure_full_name(props.scriptBuilder, builderFuncName);

  Sqrat::Table info(vm);
  info.SetValue("elem", getRef(vm));
  info.SetValue("componentDesc", props.scriptDesc);
  info.SetValue("builder", props.scriptBuilder);
  info.SetValue("builderFuncName", builderFuncName.c_str());
  info.SetValue("bbox", getElementBBox(vm));

  Element *fixedSizeRoot, *sizeRoot, *flowRoot;
  getSizeRoots(fixedSizeRoot, sizeRoot, flowRoot);

  {
    Sqrat::Array roots(vm);
    addDebugElementDescription(vm, fixedSizeRoot, "fixedSizeRoot", roots);
    addDebugElementDescription(vm, sizeRoot, "sizeRoot", roots);
    addDebugElementDescription(vm, flowRoot, "flowRoot", roots);
    addDebugElementDescription(vm, parent, "parent", roots);
    info.SetValue("roots", roots);
  }

  {
    // element's componentDesc has children tables but we need built element references too
    Sqrat::Array childrenDescs(vm);
    for (Element *e : children)
      addDebugElementDescription(vm, e, "child", childrenDescs);
    info.SetValue("children", childrenDescs);
  }

  return info;
}


} // namespace darg
