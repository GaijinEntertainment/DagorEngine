// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "component.h"
#include <util/dag_string.h>
#include <daRg/dag_stringKeys.h>
#include <perfMon/dag_statDrv.h>
#include <sqext.h>

#include "scriptUtil.h"
#include "dargDebugUtils.h"


namespace darg
{


static bool is_valid_single_child_type(const Sqrat::Object &desc)
{
  SQObjectType tp = desc.GetType();
  if (tp == OT_TABLE || tp == OT_CLASS || tp == OT_CLOSURE)
    return true;
  return false;
}


void Component::reset()
{
  uniqueKey.Release();
  rendObjType = ROBJ_NONE;

  scriptDesc.Release();
  scriptBuilder.Release();

  behaviors.clear();
}


int Component::read_robj_type(const Sqrat::Table &desc, const StringKeys *csk)
{
  Sqrat::Object rendObjField = desc.RawGetSlot(csk->rendObj);
  if (rendObjField.IsNull())
    return ROBJ_NONE;

  if (rendObjField.GetType() & SQOBJECT_NUMERIC)
    return rendObjField.Cast<int>();

  darg_assert_trace_var("Expected numeric type or null", desc, csk->rendObj);
  return ROBJ_NONE;
}


void Component::read_behaviors(const Sqrat::Table &desc, const StringKeys *csk, bhv_list_t &behaviors)
{
  behaviors.clear();
  Sqrat::Object bhvField = desc.RawGetSlot(csk->behavior);
  SQObjectType bhvType = bhvField.GetType();
  if (bhvType == OT_INSTANCE)
  {
    Behavior *bhv = try_cast_instance<Behavior>(bhvField, desc, csk->behavior, "Reading behavior field");
    if (bhv)
      behaviors.push_back(bhv);
  }
  else if (bhvType == OT_ARRAY)
  {
    Sqrat::Array bhvArr(bhvField);
    SQInteger len = bhvArr.Length();

#if DAGOR_DBGLEVEL > 0
    static int numLenWarnings = 0;
    if (len > 30 && numLenWarnings < 5 /* && desc.GetType()!=OT_CLASS*/)
    {
      ++numLenWarnings;
      darg_assert_trace_var(String(0, "Behaviors array size %d is too big - most probably an error", int(len)), desc, csk->behavior);
    }
#endif

    behaviors.reserve(len);
    for (SQInteger i = 0; i < len; ++i)
    {
      Sqrat::Object objBhv = bhvArr.RawGetSlot(i);
      Behavior *bhv = try_cast_instance<Behavior>(objBhv, bhvArr, i, "Reading behaviors array");
      if (bhv)
        behaviors.push_back(bhv);
    }
  }
  else if (bhvType != OT_NULL)
  {
    darg_assert_trace_var(String(0, "Unexpected 'behavior' field type %s (%X)", sq_objtypestr(bhvType), bhvType), desc, csk->behavior);
  }
}


static void check_children_count_limit(int n_children, const Sqrat::Object &child_desc, const Sqrat::Object &parent_builder)
{
  G_UNUSED(n_children);
  G_UNUSED(child_desc);
  G_UNUSED(parent_builder);

#if DAGOR_DBGLEVEL > 0
  static int numLenWarnings = 0;
  if (n_children > 10000 && numLenWarnings < 5 /* && desc.GetType()!=OT_CLASS*/)
  {
    ++numLenWarnings;
    String closureName;
    get_closure_full_name(parent_builder, closureName);
    String errMsg(0, "Children array size %d is too big - most probably an error (ascendant component: %s)", n_children,
      closureName.c_str());
    darg_assert_trace_var(errMsg, child_desc, 0);
  }
#endif
}


static void report_invalid_child_desc_type(SQObjectType tp, const Sqrat::Array &arr, int idx, const Sqrat::Object &parent_builder)
{
  String closureName;
  get_closure_full_name(parent_builder, closureName);
  String errMsg(32, "Invalid component description type = %s (%X) (ascendant component: %s)", sq_objtypestr(tp), tp,
    closureName.c_str());
  darg_assert_trace_var(errMsg, arr, idx);
}


static void report_invalid_child_desc_type(SQObjectType tp, const Sqrat::Object &parent, const Sqrat::Object &key,
  const Sqrat::Object &parent_builder)
{
  String closureName;
  get_closure_full_name(parent_builder, closureName);
  String errMsg(32, "Invalid component description type = %s (%X) (ascendant component: %s)", sq_objtypestr(tp), tp,
    closureName.c_str());
  darg_assert_trace_var(errMsg, parent, key);
}

static bool check_children_slot_type(const Sqrat::Object &children_slot, const Sqrat::Object &scriptDesc, const StringKeys *csk,
  const Sqrat::Object &parent_builder)
{
  SQObjectType childrenType = children_slot.GetType();
  if (childrenType == OT_NULL || childrenType == OT_ARRAY || is_valid_single_child_type(children_slot))
    return true;

  String closureName;
  get_closure_full_name(parent_builder, closureName);
  String errMsg(32, "Invalid component description type = %s (%X), (nested) child of %s", sq_objtypestr(childrenType), childrenType,
    closureName.c_str());
  darg_assert_trace_var(errMsg, scriptDesc, csk->children);
  return false;
}


void Component::readChildrenObjects(const Sqrat::Object &parent_builder, const StringKeys *csk,
  dag::Vector<Sqrat::Object, framemem_allocator> &out_children) const
{
  out_children.clear();

  Sqrat::Object childrenObj = scriptDesc.RawGetSlot(csk->children);
  SQObjectType childrenType = childrenObj.GetType();

  if (!check_children_slot_type(childrenObj, scriptDesc, csk, parent_builder))
    return;

  if (childrenType == OT_ARRAY)
  {
    Sqrat::Array arr(childrenObj);
    int nCh = arr.Length();

    check_children_count_limit(nCh, childrenObj, parent_builder);

    out_children.reserve(nCh);

    for (int i = 0; i < nCh; ++i)
    {
      Sqrat::Object childObj = arr.RawGetSlot(i);
      SQObjectType tp = childObj.GetType();

      if (is_valid_single_child_type(childObj))
        out_children.push_back(childObj);
      else if (tp != OT_NULL)
        report_invalid_child_desc_type(tp, arr, i, parent_builder);
    }
  }
  else if (is_valid_single_child_type(childrenObj))
    out_children.push_back(childrenObj);
  else if (childrenType != OT_NULL)
    report_invalid_child_desc_type(childrenType, scriptDesc, csk->children, parent_builder);
}


void Component::check_if_desc_may_be_component(const Sqrat::Table &desc, const Sqrat::Object &nearest_builder, const StringKeys *csk)
{
#if DAGOR_DBGLEVEL > 0
  if (desc.GetType() == OT_NULL || desc.Length() == 0)
    return;

  const Sqrat::Object *expected_keys[] = {&csk->children, &csk->behavior, &csk->flow, &csk->rendObj, &csk->size, &csk->transform,
    &csk->watch, &csk->hotkeys, &csk->eventHandlers};

  bool mayBe = false;
  for (const Sqrat::Object *key : expected_keys)
  {
    if (desc.RawHasKey(*key))
    {
      mayBe = true;
      break;
    }
  }

  if (!mayBe)
  {
    String closureName;
    get_closure_full_name(nearest_builder, closureName);
    String errMsg(32, "Probably invalid component description, (nested) child of %s", closureName.c_str());
    Sqrat::Object traceKey = csk->children;
    Sqrat::Object::iterator it;
    if (desc.Next(it))
      traceKey = Sqrat::Object(it.getKey(), desc.GetVM());

    darg_assert_trace_var(errMsg, desc, traceKey);
  }

#else
  G_UNUSED(desc);
  G_UNUSED(nearest_builder);
  G_UNUSED(csk);
#endif
}


bool Component::resolve_description(const Sqrat::Object &desc, Sqrat::Table &desc_tbl, Sqrat::Object &builder)
{
  HSQUIRRELVM vm = desc.GetVM();

  SQObjectType descType = desc.GetType();
  switch (descType)
  {
    case OT_TABLE:
    case OT_CLASS:
    {
      desc_tbl = desc;
      return true;
    }
    case OT_CLOSURE:
    {
      builder = desc;
      Sqrat::Function func(vm, Sqrat::Object(vm), desc.GetObject());
      Sqrat::Object tbl;
      bool ok;
      {
        TIME_PROFILE_DEV(comp_gen_sq_call);
        ok = func.Evaluate(tbl);
#if DAGOR_DBGLEVEL > 0 && TIME_PROFILER_ENABLED
        String cfn(framemem_ptr());
        get_closure_full_name(desc, cfn);
        DA_PROFILE_TAG(comp_gen_sq_call, cfn.c_str());
#endif
      }
      if (ok)
      {
        SQObjectType resType = tbl.GetType();
        if (resType == OT_CLASS || resType == OT_TABLE)
        {
          desc_tbl = tbl;
          return true;
        }
        else if (resType == OT_NULL)
        {
          desc_tbl = Sqrat::Table(vm);
          return true;
        }
        else
        {
          String closureName;
          get_closure_full_name(desc, closureName);
          String errMsg(32, "Invalid component description type %s (%X) returned from function %s", sq_objtypestr(resType), resType,
            closureName.c_str());

          darg_immediate_error(vm, errMsg);
          return false;
        }
      }
      else
      {
        desc_tbl = Sqrat::Table(vm);
        return false;
      }
    }
    case OT_NULL: desc_tbl = Sqrat::Table(vm); return false;
    default:
      desc_tbl = Sqrat::Table(vm);
      String errMsg(0, "Invalid component description type = %s (%X)", sq_objtypestr(descType), descType);
      darg_immediate_error(vm, errMsg);
      return false;
  }
}


bool Component::build_component(Component &comp, const Sqrat::Object &desc, const StringKeys *csk, const Sqrat::Object &parent_builder)
{
  using namespace Sqrat;

  SQObjectType descType = desc.GetType();
  Table descTbl;
  Object builder;

  if (!resolve_description(desc, descTbl, builder) || descTbl.IsNull())
  {
    comp.reset();
    return false;
  }

  check_if_desc_may_be_component(descTbl, descType == OT_CLOSURE ? desc : parent_builder, csk);

  comp.scriptBuilder = builder;
  comp.scriptDesc = descTbl;
  comp.uniqueKey = descTbl.RawGetSlot(csk->key);
  comp.rendObjType = read_robj_type(descTbl, csk);
  read_behaviors(descTbl, csk, comp.behaviors);

  return true;
}


} // namespace darg
