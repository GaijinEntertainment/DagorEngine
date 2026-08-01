// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvBoundProps.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include <quirrel/frp/dag_frp.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>

#include "guiScene.h"
#include "elementTree.h"
#include "component.h"
#include "dargDebugUtils.h"


namespace darg
{

using sqfrp::NodeId;
using sqfrp::WatchedHandle;


BhvBoundProps bhv_bound_props;


// Static component tables are commonly frozen; bound slots are the
// behavior's controlled mutation, freezing still guards script-side writes.
class UnfreezeRAII
{
public:
  UnfreezeRAII(Sqrat::Object &o) : obj(o), wasImmutable(o.GetObject()._flags & SQOBJ_FLAG_IMMUTABLE)
  {
    if (wasImmutable)
      obj.UnfreezeSelf();
  }

  ~UnfreezeRAII()
  {
    if (wasImmutable)
      obj.FreezeSelf();
  }

  Sqrat::Object &obj;
  bool wasImmutable;
};


template <size_t N>
static bool key_in_list(const Sqrat::Object &key, const Sqrat::Object *const (&list)[N])
{
  for (const Sqrat::Object *k : list)
    if (key.IsEqual(*k))
      return true;
  return false;
}


static bool is_forbidden_key(const StringKeys *csk, const Sqrat::Object &key)
{
  const Sqrat::Object *const forbiddenKeys[] = {&csk->children, &csk->behavior, &csk->watch, &csk->rendObj, &csk->key};
  return key_in_list(key, forbiddenKeys);
}


struct BindingsScan
{
  dag::RelocatableFixedVector<NodeId, 8, true, framemem_allocator> nodes;
  bool layoutAffecting = false;
  bool valuesOutOfSync = false;
};


static void scan_bindings(Element *elem, bool report_errors, BindingsScan &out)
{
  const Sqrat::Object bindings = elem->props.scriptDesc.RawGetSlot(elem->csk->bindProps);
  if (bindings.GetType() != OT_TABLE)
  {
    if (report_errors && !bindings.IsNull())
      darg_assert_trace_var("bindProps must be a table of propName = observable", elem->props.scriptDesc, elem->csk->bindProps);
    return;
  }

  const StringKeys *csk = elem->csk;

  // paint-only keys: re-applying properties suffices, no layout recalc. Any
  // bound key not listed here conservatively triggers relayout, so a missing
  // entry costs a redundant layout pass, never stale layout.
  const Sqrat::Object *const paintOnlyKeys[] = {
    &csk->color,
    &csk->fillColor,
    &csk->borderColor,
    &csk->opacity,
    &csk->brightness,
    &csk->bgColor,
    &csk->fgColor,
    &csk->tint,
    &csk->picSaturate,
    &csk->fontFx,
    &csk->fontFxColor,
    &csk->fontFxFactor,
    &csk->fontFxOffsX,
    &csk->fontFxOffsY,
    &csk->fontTex,
    &csk->fontTexSu,
    &csk->fontTexSv,
    &csk->fontTexBov,
    &csk->textOverflowX,
    &csk->textOverflowY,
    &csk->ellipsis,
    &csk->imageHalign,
    &csk->imageValign,
    &csk->flipX,
    &csk->flipY,
    &csk->lineWidth,
  };

  const bool imageDrivesLayout = elem->props.getBool(csk->imageAffectsLayout, false, false) &&
                                 (elem->layout.size[0].mode == SizeSpec::CONTENT || elem->layout.size[1].mode == SizeSpec::CONTENT);

  sqfrp::ObservablesGraph *sceneGraph = GuiScene::get_from_elem(elem)->frpGraph.get();
  HSQUIRRELVM vm = elem->props.scriptDesc.GetVM();

  Sqrat::Table bindingsTbl(bindings);
  Sqrat::Object::iterator it;
  while (bindingsTbl.Next(it))
  {
    Sqrat::Object key(it.getKey(), vm);
    Sqrat::Object val(it.getValue(), vm);

    if (is_forbidden_key(csk, key))
    {
      if (report_errors)
        darg_assert_trace_var("structural property cannot be bound", bindingsTbl, key);
      continue;
    }

    HSQOBJECT hVal = val.GetObject();
    if (val.GetType() != OT_INSTANCE || !Sqrat::ClassType<WatchedHandle>::IsObjectOfClass(&hVal))
    {
      if (report_errors)
        darg_assert_trace_var("bindProps value must be an observable", bindingsTbl, key);
      continue;
    }

    WatchedHandle *handle = val.Cast<WatchedHandle *>();
    if (!handle)
      continue;
    if (handle->graph != sceneGraph)
    {
      if (report_errors)
        darg_assert_trace_var("bound observable belongs to another VM", bindingsTbl, key);
      continue;
    }

    bool seen = false;
    for (NodeId id : out.nodes)
      if (id == handle->id)
      {
        seen = true;
        break;
      }
    if (!seen)
      out.nodes.push_back(handle->id);

    bool paintOnly = key_in_list(key, paintOnlyKeys);
    if (!paintOnly && (key.IsEqual(csk->image) || key.IsEqual(csk->fallbackImage)))
      paintOnly = !imageDrivesLayout;
    out.layoutAffecting |= !paintOnly;

    if (!out.valuesOutOfSync)
    {
      Sqrat::Object cur = elem->props.scriptDesc.RawGetSlot(key);
      if (!cur.IsEqual(handle->getValue()))
        out.valuesOutOfSync = true;
    }
  }

  // escape hatch for a paint-only misclassification; no force-off
  out.layoutAffecting |= elem->props.getBool(csk->rtRecalcLayout, false, false);
}


BhvBoundProps::BhvBoundProps() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvBoundProps::onElemSetup(Element *elem, SetupMode setup_mode)
{
  // never called with SM_REALTIME_UPDATE, so applyForElem cannot re-enter
  BhvBoundPropsData *data = elem->props.storage.RawGetSlotValue<BhvBoundPropsData *>(elem->csk->boundPropsData, nullptr);
  if (!data)
  {
    data = new BhvBoundPropsData();
    elem->props.storage.SetValue(elem->csk->boundPropsData, data);
  }

  BindingsScan scan;
  scan_bindings(elem, /*report_errors*/ true, scan);
  data->layoutAffecting = scan.layoutAffecting;

  // temporary trees forbid side effects: no graph subscription, no deferred
  // work; onAttach still applies values so calc_comp_size measures live data
  if (elem->etree->isInternalTemporaryTree)
    return;

  if (scan.nodes.size() != data->nodes.size() ||
      (!data->nodes.empty() && memcmp(scan.nodes.data(), data->nodes.data(), data->nodes.size() * sizeof(NodeId)) != 0))
  {
    sqfrp::ObservablesGraph *graph = GuiScene::get_from_elem(elem)->frpGraph.get();
    for (NodeId id : data->nodes)
      graph->unsubscribeWatcher(id, data);
    data->nodes.assign(scan.nodes.begin(), scan.nodes.end());
    for (NodeId id : data->nodes)
      graph->subscribeWatcher(id, data);
  }

  // a rebuild replaces the desc with fresh builder output whose bound slots
  // hold static values, and onAttach does not fire for reused elements
  if (setup_mode == SM_REBUILD_UPDATE && scan.valuesOutOfSync)
    data->dirty = true;
}


void BhvBoundProps::onAttach(Element *elem)
{
  BhvBoundPropsData *data = elem->props.storage.RawGetSlotValue<BhvBoundPropsData *>(elem->csk->boundPropsData, nullptr);
  if (!data)
    return;

  // apply before the initial layout pass so it (and calc_comp_size
  // measurement) sees current observable values; pre-warmed descs skip it
  BindingsScan scan;
  scan_bindings(elem, /*report_errors*/ false, scan);
  if (scan.valuesOutOfSync)
    applyForElem(elem, data);
}


void BhvBoundProps::onDetach(Element *elem, DetachMode)
{
  BhvBoundPropsData *data = elem->props.storage.RawGetSlotValue<BhvBoundPropsData *>(elem->csk->boundPropsData, nullptr);
  if (!data)
    return;

  elem->props.storage.RawDeleteSlot(elem->csk->boundPropsData);
  if (!data->nodes.empty())
  {
    sqfrp::ObservablesGraph *graph = GuiScene::get_from_elem(elem)->frpGraph.get();
    for (NodeId id : data->nodes)
      graph->unsubscribeWatcher(id, data);
  }
  delete data;
}


int BhvBoundProps::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  BhvBoundPropsData *data = elem->props.storage.RawGetSlotValue<BhvBoundPropsData *>(elem->csk->boundPropsData, nullptr);
  if (!data || !data->dirty)
    return 0;
  data->dirty = false;
  return applyForElem(elem, data);
}


int BhvBoundProps::applyForElem(Element *elem, BhvBoundPropsData *data)
{
  Sqrat::Table src = elem->props.scriptDesc;
  Sqrat::Object bindings = src.RawGetSlot(elem->csk->bindProps);
  if (bindings.GetType() != OT_TABLE)
    return 0;

  HSQUIRRELVM vm = src.GetVM();
  // Element::zOrder is private; setup copies it verbatim from the desc, so
  // detect the change at the desc level, around the bound writes
  const SQInteger prevZOrder = src.RawGetSlotValue<SQInteger>(elem->csk->zOrder, INT16_MIN);
  bool wroteAny = false;
  {
    UnfreezeRAII unfreeze(elem->props.scriptDesc);
    Sqrat::Table bindingsTbl(bindings);
    Sqrat::Object::iterator it;
    while (bindingsTbl.Next(it))
    {
      Sqrat::Object key(it.getKey(), vm);
      Sqrat::Object val(it.getValue(), vm);

      HSQOBJECT hVal = val.GetObject();
      if (val.GetType() != OT_INSTANCE || !Sqrat::ClassType<WatchedHandle>::IsObjectOfClass(&hVal))
        continue;
      if (is_forbidden_key(elem->csk, key))
        continue;

      WatchedHandle *handle = val.Cast<WatchedHandle *>();
      if (!handle)
        continue;

      src.SetValue(key, handle->getValue());
      wroteAny = true;
    }
  }

  if (!wroteAny)
    return 0;

  GuiScene *guiScene = GuiScene::get_from_elem(elem);

  Component tmpComp;
  tmpComp.rendObjType = elem->rendObjType;
  tmpComp.scriptDesc = src;
  tmpComp.scriptBuilder = elem->props.scriptBuilder;
  tmpComp.behaviors = elem->behaviors;

  const bool wasHidden = elem->isHidden();
  elem->setup(tmpComp, guiScene, SM_REALTIME_UPDATE);

  if (data->layoutAffecting)
    elem->recalcLayout();

  const SQInteger newZOrder = src.RawGetSlotValue<SQInteger>(elem->csk->zOrder, INT16_MIN);

  // the render list is z-sorted only at stack rebuild
  return (elem->isHidden() != wasHidden || newZOrder != prevZOrder) ? R_REBUILD_RENDER_AND_INPUT_LISTS : 0;
}


} // namespace darg
