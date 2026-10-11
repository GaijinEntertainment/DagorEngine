// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvVirtualList.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "dargDebugUtils.h"
#include "statefulComp.h"
#include "elementTree.h"

#include <util/dag_string.h>


namespace darg
{


BhvVirtualList bhv_virtual_list;

// Marks a contributed child that is not one of the items: a spacer, or a tail
// entry.
static constexpr int NOT_AN_ITEM = -1;

BhvVirtualList::BhvVirtualList() : Behavior(STAGE_ACT, 0) {}


static BhvVirtualListData *get_data(const Element *elem)
{
  return elem->props.storage.RawGetSlotValue<BhvVirtualListData *>(elem->csk->virtualListData, nullptr);
}


// A flow child that occupies the given size along the axis and nothing on the
// other. Stands in for an item the window was told about but could not build.
static Sqrat::Object make_size_spacer(HSQUIRRELVM vm, const StringKeys *csk, int axis, float sz)
{
  Sqrat::Array size(vm, 2);
  size.SetValue(SQInteger(0), axis == 0 ? sz : 0.0f);
  size.SetValue(SQInteger(1), axis == 0 ? 0.0f : sz);
  Sqrat::Table desc(vm);
  desc.SetValue(csk->size, size);
  return desc;
}


// Carries a skipped run as margin, not size: flow spacing is
// max(prev.margin.rb, next.margin.lt), so the run dominates the neighbouring
// item's own margin instead of adding to it.
static Sqrat::Object make_run_spacer(HSQUIRRELVM vm, const StringKeys *csk, int axis, float sz, bool leading)
{
  // Offsets are {t, r, b, l}, and the margin has to sit on the side facing the
  // items: below a leading run, above a trailing one.
  const SQInteger side = (axis == 0) ? (leading ? 1 : 3) : (leading ? 2 : 0);
  Sqrat::Array margin(vm, 4);
  for (SQInteger i = 0; i < 4; ++i)
    margin.SetValue(i, i == side ? sz : 0.0f);

  Sqrat::Array size(vm, 2);
  size.SetValue(SQInteger(0), 0.0f);
  size.SetValue(SQInteger(1), 0.0f);

  Sqrat::Table desc(vm);
  desc.SetValue(csk->size, size);
  desc.SetValue(csk->margin, margin);
  return desc;
}


// A null entry is legal, as in 'children', but the caller must then stand a
// spacer in for it. Anything else would reach rebuild() unable to name its slot.
static bool is_valid_desc(const Sqrat::Object &obj, const Sqrat::Array &arr, int idx)
{
  const SQObjectType tp = obj.GetType();
  if (tp == OT_NULL)
    return false;

  if (tp == OT_TABLE || tp == OT_CLASS || tp == OT_CLOSURE || (tp == OT_INSTANCE && try_get_stateful_desc(obj)))
    return true;

  darg_assert_trace_var(String(0, "Invalid component description type = %s (%X)", sq_objtypestr(tp), tp), arr, idx);
  return false;
}


// largest i with itemTop[i] <= pos, clamped to a valid item index
static int item_at(const Tab<float> &item_top, float pos)
{
  int lo = 0, hi = int(item_top.size()) - 1; // itemTop has n+1 entries
  while (lo + 1 < hi)
  {
    int mid = (lo + hi) / 2;
    if (item_top[mid] <= pos)
      lo = mid;
    else
      hi = mid;
  }
  return lo;
}


void BhvVirtualList::onElemSetup(Element *elem, SetupMode)
{
  BhvVirtualListData *data = get_data(elem);
  if (!data)
  {
    // must exist before contributeChildren(), which runs before onAttach()
    data = new BhvVirtualListData();
    elem->props.storage.SetValue(elem->csk->virtualListData, data);
  }

  const Properties &props = elem->props;
  data->axis = (elem->layout.flowType == FLOW_HORIZONTAL) ? 0 : 1;
  data->overscan = max(0, props.getInt<int>(elem->csk->virtualOverscan, 3));
  data->initialCount = max(1, props.getInt<int>(elem->csk->virtualInitialCount, 32));

  Sqrat::Object itemsObj = props.scriptDesc.RawGetSlot(elem->csk->virtualItems);
  if (!itemsObj.IsNull() && itemsObj.GetType() != OT_ARRAY)
    darg_assert_trace_var("virtualItems must be an array", props.scriptDesc, elem->csk->virtualItems);
  Sqrat::Object tailObj = props.scriptDesc.RawGetSlot(elem->csk->virtualTail);
  if (!tailObj.IsNull() && tailObj.GetType() != OT_ARRAY)
    darg_assert_trace_var("virtualTail must be an array", props.scriptDesc, elem->csk->virtualTail);

  // an empty array contributes no children and so shifts nothing - a builder
  // that emits one conditionally must not error on every rebuild
  Sqrat::Object childrenObj = props.scriptDesc.RawGetSlot(elem->csk->children);
  const bool haveChildren = childrenObj.GetType() == OT_ARRAY ? Sqrat::Array(childrenObj).Length() > 0 : !childrenObj.IsNull();
  if (haveChildren)
    darg_assert_trace_var("VirtualList uses 'virtualItems'; 'children' would shift item indices", props.scriptDesc,
      elem->csk->children);

  // A component gap inserts elements that shift the item indices; a numeric one
  // adds spacing a spacer cannot reproduce. Zero does neither.
  const Sqrat::Object gapObj = props.scriptDesc.RawGetSlot(elem->csk->gap);
  const SQObjectType gapType = gapObj.GetType();
  const bool gapInsertsElements = gapType == OT_CLOSURE || gapType == OT_TABLE || gapType == OT_CLASS;
  if (gapInsertsElements || elem->layout.gap != 0.0f)
    darg_assert_trace_var("VirtualList does not support 'gap'", props.scriptDesc, elem->csk->gap);
  // reorders the children without changing their count, so nothing downstream
  // can detect it
  if (props.getBool(elem->csk->sortChildren, false))
    darg_assert_trace_var("VirtualList does not support 'sortChildren'", props.scriptDesc, elem->csk->sortChildren);

  // FLOW_PARENT_RELATIVE places children independently and takes the max for
  // content size, so the window would overlap itself. Also where axis comes from.
  if (elem->layout.flowType == FLOW_PARENT_RELATIVE)
    darg_assert_trace_var("VirtualList needs a flow axis: set 'flow'", props.scriptDesc, elem->csk->flow);

  // alignChildren() offsets a centered or end-aligned flow by the total size of
  // the children it built, which for a window is not the size of the list
  const ElemAlign flowAlign = (data->axis == 0) ? elem->layout.hAlign : elem->layout.vAlign;
  if (flowAlign != ALIGN_LEFT_OR_TOP)
    darg_assert_trace_var("VirtualList must be aligned to the start of its flow axis", props.scriptDesc,
      data->axis == 0 ? elem->csk->halign : elem->csk->valign);

  const int n = (itemsObj.GetType() == OT_ARRAY) ? int(Sqrat::Array(itemsObj).Length()) : 0;

  Sqrat::Object heightsObj = props.scriptDesc.RawGetSlot(elem->csk->virtualItemHeights);
  const float uniformHeight = props.getFloat(elem->csk->virtualItemHeight, 0.0f);
  if (heightsObj.GetType() == OT_ARRAY && int(Sqrat::Array(heightsObj).Length()) != n)
  {
    darg_assert_trace_var("virtualItemHeights must have as many entries as virtualItems", props.scriptDesc,
      elem->csk->virtualItemHeights);
    heightsObj = Sqrat::Object();
  }
  if (n > 0 && heightsObj.GetType() != OT_ARRAY && uniformHeight <= 0.0f)
    darg_assert_trace_var("VirtualList needs virtualItemHeight or virtualItemHeights", props.scriptDesc, elem->csk->virtualItems);
  if (n > 0 && props.scriptBuilder.IsNull())
    darg_assert_trace_var("A VirtualList component must be defined by a function: moving the window rebuilds it", props.scriptDesc,
      elem->csk->virtualItems);

  data->itemTop.resize(n + 1);
  data->itemTop[0] = 0.0f;
  if (heightsObj.GetType() == OT_ARRAY)
  {
    Sqrat::Array heights(heightsObj);
    for (int i = 0; i < n; ++i)
      data->itemTop[i + 1] = data->itemTop[i] + max(0.0f, heights.RawGetSlotValue<float>(SQInteger(i), uniformHeight));
  }
  else
  {
    for (int i = 0; i < n; ++i)
      data->itemTop[i + 1] = data->itemTop[i] + max(0.0f, uniformHeight);
  }
}


void BhvVirtualList::onDetach(Element *elem, DetachMode)
{
  if (BhvVirtualListData *data = get_data(elem))
  {
    elem->props.storage.DeleteSlot(elem->csk->virtualListData);
    delete data;
  }
}


void BhvVirtualList::contributeChildren(Element *elem, dag::Vector<Sqrat::Object, framemem_allocator> &children)
{
  BhvVirtualListData *data = get_data(elem);
  if (!data)
    return;

  data->builtItems.clear();

  // anything already in here came from the script's own 'children', which setup
  // asserts against - the index mapping cannot account for it
  const bool ownAll = children.empty();

  HSQUIRRELVM vm = elem->props.scriptDesc.GetVM();
  const int axis = data->axis;

  auto addChild = [&](const Sqrat::Object &obj, int item_idx) {
    children.push_back(obj);
    data->builtItems.push_back(item_idx);
  };

  Sqrat::Object itemsObj = elem->props.scriptDesc.RawGetSlot(elem->csk->virtualItems);
  const int n = data->nItems();

  if (n > 0 && itemsObj.GetType() == OT_ARRAY)
  {
    if (data->count < 0) // first build: no layout yet, so seed a window
    {
      data->first = 0;
      data->count = min(n, data->initialCount);
    }
    // also catches a window computed for a longer item list than this one
    data->first = clamp(data->first, 0, max(0, n - 1));
    data->count = clamp(data->count, 1, n - data->first);

    const int end = data->first + data->count;
    const float before = data->itemTop[data->first];
    const float after = data->totalSize() - data->itemTop[end];

    if (before > 0.0f)
      addChild(make_run_spacer(vm, elem->csk, axis, before, true), NOT_AN_ITEM);

    Sqrat::Array items(itemsObj);
    data->builtItems.reserve(data->count + 2);
    for (int i = data->first; i < end; ++i)
    {
      Sqrat::Object obj = items.RawGetSlot(SQInteger(i));
      if (is_valid_desc(obj, items, i))
      {
        addChild(obj, i);
        continue;
      }
      // the item still owns the height itemTop counts for it, or everything
      // after it renders shifted up and the extent shrinks
      const float h = data->itemTop[i + 1] - data->itemTop[i];
      if (h > 0.0f)
        addChild(make_size_spacer(vm, elem->csk, axis, h), NOT_AN_ITEM);
    }

    if (after > 0.0f)
      addChild(make_run_spacer(vm, elem->csk, axis, after, false), NOT_AN_ITEM);
  }

  // always built, even with no items: they sit after every one, so their own size
  // covers the end of the content and their height need not be declared
  Sqrat::Object tailObj = elem->props.scriptDesc.RawGetSlot(elem->csk->virtualTail);
  if (tailObj.GetType() == OT_ARRAY)
  {
    Sqrat::Array tail(tailObj);
    for (SQInteger i = 0, tailLen = tail.Length(); i < tailLen; ++i)
    {
      Sqrat::Object obj = tail.RawGetSlot(i);
      if (is_valid_desc(obj, tail, int(i)))
        addChild(obj, NOT_AN_ITEM);
    }
  }

  data->contributed = ownAll ? int(children.size()) : -1;
}


// The window the scroll position calls for, or false when the built one still
// covers the viewport - the case overscan buys, so a scroll can move that many
// items before anything is rebuilt.
static bool wanted_window(const Element *elem, const BhvVirtualListData *data, int &out_first, int &out_count)
{
  const int n = data->nItems();
  const int axis = data->axis;

  // Item i renders at padding.lt + itemTop[i] - scrollOffs inside a clip box of
  // padding.lt .. size - padding.rb, so the padding cancels out of the offset
  // and comes off the viewport instead.
  const Offsets &pad = elem->layout.padding();
  const float padLead = (axis == 0) ? pad.l : pad.t;
  const float padTrail = (axis == 0) ? pad.r : pad.b;
  const float viewport = elem->screenCoord.size[axis] - padLead - padTrail;
  if (n <= 0 || viewport <= 0.0f)
    return false; // not laid out yet; keep the seeded window

  const float top = elem->screenCoord.scrollOffs[axis];
  const int firstVisible = item_at(data->itemTop, top);
  const int lastVisible = item_at(data->itemTop, top + viewport);

  if (data->count > 0 && firstVisible >= data->first && lastVisible < data->first + data->count)
    return false;

  out_first = clamp(firstVisible - data->overscan, 0, max(0, n - 1));
  const int last = clamp(lastVisible + data->overscan, out_first, n - 1);
  out_count = last - out_first + 1;
  return true;
}


// index of the direct child of 'root' that 'elem' descends from, or -1
static int child_branch_index(const Element *root, const Element *elem)
{
  for (const Element *e = elem; e; e = e->parent)
  {
    if (e->parent == root)
    {
      for (int i = 0, n = int(root->children.size()); i < n; ++i)
        if (root->children[i] == e)
          return i;
      return -1;
    }
  }
  return -1;
}


// resolve_description() turns a null builder result into an empty description,
// so an element is created - count still aligned, guard below silent - that lays
// out at nothing while itemTop keeps counting its height.
static void warn_inert_items(const Element *elem, BhvVirtualListData *data)
{
  const int nChildren = int(elem->children.size());
  for (int i = 0, n = min(int(data->builtItems.size()), nChildren); i < n; ++i)
  {
    const int itemIdx = data->builtItems[i];
    if (itemIdx == NOT_AN_ITEM || data->itemTop[itemIdx + 1] - data->itemTop[itemIdx] <= 0.0f)
      continue;
    if (!ElementTree::does_element_affect_layout(elem->children[i]))
    {
      data->warnedInertItem = true;
      darg_assert_trace_var("A VirtualList item declared a height but lays out at nothing: a builder must return a description, "
                            "not null",
        elem->props.scriptDesc, elem->csk->virtualItems);
      return;
    }
  }
}


int BhvVirtualList::update(UpdateStage, Element *elem, float /*dt*/)
{
  BhvVirtualListData *data = get_data(elem);
  if (!data)
    return 0;

  // before the early returns below: a list already drifted this way sits stable,
  // so the window never moves again
  if (!data->warnedInertItem)
    warn_inert_items(elem, data);

  int wantFirst = 0, wantCount = 0;
  if (!wanted_window(elem, data, wantFirst, wantCount))
    return 0;
  if (wantFirst == data->first && wantCount == data->count)
    return 0;

  GuiScene *guiScene = GuiScene::get_from_elem(elem);

  // KbFocus::onElementDetached() does not run the script onBlur handler, so a
  // field leaving the window must be released here or its edit is dropped
  // unapplied. A branch that is not an item is a spacer or tail, which stay.
  if (Element *focus = guiScene->kbFocus.focus)
  {
    const int branch = child_branch_index(elem, focus);
    if (branch >= 0)
    {
      if (data->contributed < 0 || int(elem->children.size()) != data->contributed)
      {
        // A failed stateful mount emits no element and no positional mapping
        // survives it. Release rather than guess: onBlur still applies the edit.
        guiScene->kbFocus.setFocus(nullptr);
      }
      else if (branch < int(data->builtItems.size()))
      {
        const int itemIdx = data->builtItems[branch];
        if (itemIdx != NOT_AN_ITEM && (itemIdx < wantFirst || itemIdx >= wantFirst + wantCount))
          guiScene->kbFocus.setFocus(nullptr);
      }
    }
  }

  data->first = wantFirst;
  data->count = wantCount;
  guiScene->invalidateElement(elem);
  return 0;
}


} // namespace darg
