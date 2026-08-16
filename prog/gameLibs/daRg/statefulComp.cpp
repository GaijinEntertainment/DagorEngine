// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "statefulComp.h"

#include <daRg/dag_stringKeys.h>
#include <sqstdaux.h>

#include "component.h"
#include "guiScene.h"
#include "scriptUtil.h"
#include "dargDebugUtils.h"

#include <squirrel/sqpcheader.h>
#include <squirrel/sqvm.h>
#include <squirrel/sqstate.h>
#include <squirrel/sqstring.h>
#include <squirrel/sqfuncproto.h>
#include <squirrel/sqclosure.h>

namespace darg
{

using namespace sqfrp;

const char *const stateful_builder_lock_msg =
  "Creating observables or subscribing is not allowed in a stateful component builder; construct state in the component ctor";


static bool is_primitive_or_null(SQObjectType tp)
{
  return tp == OT_NULL || tp == OT_INTEGER || tp == OT_FLOAT || tp == OT_BOOL || tp == OT_STRING;
}


static WatchedHandle *try_get_observable(const Sqrat::Object &obj)
{
  if (obj.GetType() != OT_INSTANCE)
    return nullptr;
  return Sqrat::ClassType<WatchedHandle>::GetInstanceFromObj(obj.GetObject());
}


static void abandon(Sqrat::Object &obj) { sq_resetobject(&obj.GetObject()); }


void StatefulCompType::abandonScriptRefs()
{
  abandon(ctorFunc);
  abandon(keyFunc);
}


void StatefulCompDesc::abandonScriptRefs()
{
  abandon(typeRef);
  abandon(keyValue);
  for (Sqrat::Object &arg : args)
    abandon(arg);
}


// Sqrat gets a null VM only when the instance dies in the sq_close teardown walk,
// the one case where the script references must be abandoned, not released.
template <typename T>
static SQInteger release_bound_instance(HSQUIRRELVM vm, SQUserPointer ptr, SQInteger size)
{
  if (!vm)
    static_cast<T *>(ptr)->abandonScriptRefs();
  return Sqrat::ClassType<T>::ReleaseOwned(vm, ptr, size);
}


SQInteger StatefulCompType::script_ctor(HSQUIRRELVM vm)
{
  SQInteger top = sq_gettop(vm);
  if (top < 2 || top > 3)
    return sq_throwerror(vm, "Expected StatefulComp(ctor) or StatefulComp(ctor, key)");

  if (sq_gettype(vm, 2) != OT_CLOSURE)
    return sq_throwerror(vm, "StatefulComp ctor must be a script function");

  HSQOBJECT hCtor;
  sq_getstackobj(vm, 2, &hCtor);
  SQFunctionProto *ctorProto = _closure(hCtor)->_function;
  if (ctorProto->_varparams)
    return sq_throwerror(vm, "StatefulComp ctor must not be vararg");
  if (ctorProto->_ndefaultparams > 0)
    return sq_throwerror(vm, "StatefulComp ctor must not have default parameter values (arguments arrive as observables)");

  eastl::unique_ptr<StatefulCompType> self(new StatefulCompType());
  self->ctorFunc = Sqrat::Object(hCtor, vm);
  self->numArgs = ctorProto->_nparameters - 1; // skip 'this'

  if (top == 3 && sq_gettype(vm, 3) != OT_NULL)
  {
    if (sq_gettype(vm, 3) != OT_CLOSURE)
      return sq_throwerror(vm, "StatefulComp key must be a script function or null");

    HSQOBJECT hKey;
    sq_getstackobj(vm, 3, &hKey);
    SQFunctionProto *keyProto = _closure(hKey)->_function;
    if (keyProto->_varparams)
      return sq_throwerror(vm, "StatefulComp key function must not be vararg");
    if (keyProto->_ndefaultparams > 0)
      return sq_throwerror(vm, "StatefulComp key function must not have default parameter values");

    // Bind by name, so that reordering ctor parameters cannot silently
    // re-point the key.
    for (SQInt32 iKey = 1; iKey < keyProto->_nparameters; ++iKey)
    {
      const char *keyParamName = _stringval(keyProto->_parameters[iKey]);
      int argIdx = -1;
      for (SQInt32 iCtor = 1; iCtor < ctorProto->_nparameters; ++iCtor)
        if (strcmp(keyParamName, _stringval(ctorProto->_parameters[iCtor])) == 0)
        {
          argIdx = iCtor - 1;
          break;
        }
      if (argIdx < 0)
        return sqstd_throwerrorf(vm, "StatefulComp key parameter '%s' does not name a ctor parameter", keyParamName);
      self->keyArgIndices.push_back(argIdx);
    }
    self->keyFunc = Sqrat::Object(hKey, vm);
  }

  // After SetManagedInstance: it installs the plain ReleaseOwned hook itself.
  Sqrat::ClassType<StatefulCompType>::SetManagedInstance(vm, 1, self.release());
  sq_setreleasehook(vm, 1, &release_bound_instance<StatefulCompType>);
  return 0;
}


SQInteger StatefulCompType::call_mm(HSQUIRRELVM vm)
{
  // stack: 1 = type instance, 2 = call-site 'this', 3.. = arguments
  StatefulCompType *self = Sqrat::ClassType<StatefulCompType>::GetInstance(vm, 1);
  if (!self)
    return SQ_ERROR;

  int nArgs = int(sq_gettop(vm)) - 2;
  if (nArgs != self->numArgs)
  {
    String ctorName;
    get_closure_full_name(self->ctorFunc, ctorName);
    return sqstd_throwerrorf(vm, "%s expects exactly %d argument(s), got %d", ctorName.c_str(), self->numArgs, nArgs);
  }

  HSQOBJECT hType;
  sq_getstackobj(vm, 1, &hType);

  eastl::unique_ptr<StatefulCompDesc> desc(new StatefulCompDesc());
  desc->typeRef = Sqrat::Object(hType, vm);
  desc->type = self;
  desc->args.reserve(nArgs);
  for (int i = 0; i < nArgs; ++i)
  {
    HSQOBJECT hArg;
    sq_getstackobj(vm, 3 + i, &hArg);
    desc->args.push_back(Sqrat::Object(hArg, vm));
  }

  if (!self->keyFunc.IsNull())
  {
    // The key function sees values, not observables: unwrap them.
    sq_pushobject(vm, self->keyFunc.GetObject());
    sq_pushnull(vm);
    for (int k = 0, nk = int(self->keyArgIndices.size()); k < nk; ++k)
    {
      int argIdx = self->keyArgIndices[k];
      const Sqrat::Object &arg = desc->args[argIdx];
      if (WatchedHandle *h = try_get_observable(arg))
      {
        if (!h->graph || !h->graph->resolve(h->id))
        {
          sq_pop(vm, 2 + k);
          return sqstd_throwerrorf(vm, "StatefulComp key: argument %d is a released observable", argIdx + 1);
        }
        sq_pushobject(vm, h->graph->getValue(h->id).GetObject());
      }
      else
        sq_pushobject(vm, arg.GetObject());
    }
    if (SQ_FAILED(sq_call(vm, 1 + int(self->keyArgIndices.size()), SQTrue, SQTrue)))
    {
      sq_pop(vm, 1); // the closure
      return SQ_ERROR;
    }

    HSQOBJECT hKeyVal;
    sq_getstackobj(vm, -1, &hKeyVal);
    if (!is_primitive_or_null(sq_type(hKeyVal)))
    {
      sq_pop(vm, 2);
      return sqstd_throwerrorf(vm, "StatefulComp key must be a primitive value or null, got %s", sq_objtypestr(sq_type(hKeyVal)));
    }
    desc->keyValue = Sqrat::Object(hKeyVal, vm);
    sq_pop(vm, 2); // result + closure
  }

  auto *cd = Sqrat::ClassType<StatefulCompDesc>::getClassData(vm);
  G_ASSERT_RETURN(cd, sq_throwerror(vm, "StatefulCompDesc class is not registered"));
  sq_pushobject(vm, cd->classObj);
  if (SQ_FAILED(sq_createinstance(vm, -1)))
  {
    sq_pop(vm, 1);
    return sq_throwerror(vm, "Failed to create descriptor instance");
  }
  sq_remove(vm, -2);
  Sqrat::ClassType<StatefulCompDesc>::SetManagedInstance(vm, -1, desc.release());
  sq_setreleasehook(vm, -1, &release_bound_instance<StatefulCompDesc>);
  return 1;
}


StatefulCompDesc *try_get_stateful_desc(const Sqrat::Object &obj)
{
  if (obj.GetType() != OT_INSTANCE)
    return nullptr;
  return Sqrat::ClassType<StatefulCompDesc>::GetInstanceFromObj(obj.GetObject());
}


bool stateful_desc_matches_instance(const StatefulCompDesc *desc, const StatefulInstance *inst)
{
  if (desc->type != inst->type)
    return false;
  HSQUIRRELVM vm = desc->typeRef.GetVM();
  HSQOBJECT a = desc->keyValue.GetObject(), b = inst->keyValue.GetObject();
  return sq_obj_is_equal(vm, &a, &b);
}


// The node is owned by the script handle, so a ctor closure that keeps the
// cell past the instance only ends up with a cell nobody writes any more.
// Non-deferred + eager pull: reconcile writes must be seen in the same pass.
static Sqrat::Object create_arg_cell(ObservablesGraph *graph, const Sqrat::Object &initial, NodeId &out_id)
{
  HSQUIRRELVM vm = graph->vm;
  out_id = graph->createWatched(initial.GetObject());
  NodeSlot &s = graph->node(out_id);
  s.isDeferred = false;
  s.needImmediate = true;
  s.eagerPull = true;

  auto *cd = Sqrat::ClassType<WatchedHandle>::getClassData(vm);
  G_ASSERT_RETURN(cd, Sqrat::Object());
  SqStackChecker check(vm);
  sq_pushobject(vm, cd->classObj);
  if (SQ_FAILED(sq_createinstance(vm, -1)))
  {
    sq_pop(vm, 1);
    return Sqrat::Object();
  }
  sq_remove(vm, -2);
  Sqrat::ClassType<WatchedHandle>::SetManagedInstance(vm, -1, new WatchedHandle(out_id, graph));
  Sqrat::Var<Sqrat::Object> res(vm, -1);
  sq_pop(vm, 1);
  return res.value;
}


eastl::unique_ptr<StatefulInstance> stateful_mount(GuiScene *scene, StatefulCompDesc *desc, Component &out_comp)
{
  ObservablesGraph *graph = scene->frpGraph.get();
  HSQUIRRELVM vm = graph->vm;
  const StringKeys *csk = scene->getStringKeys();

  eastl::unique_ptr<StatefulInstance> inst(new StatefulInstance());
  inst->typeRef = desc->typeRef;
  inst->type = desc->type;
  inst->keyValue = desc->keyValue;
  inst->graph = graph;
  inst->ownerScope.sourcesImmediate = true;

  // Cells are created outside the owner scope, so that disposing the scope
  // cannot release them ahead of their readers.
  inst->argSlots.reserve(desc->args.size());
  for (const Sqrat::Object &arg : desc->args)
  {
    StatefulInstance::ArgSlot slot;
    if (WatchedHandle *h = try_get_observable(arg))
    {
      slot.observable = arg;
      slot.node = h->id;
      slot.pinned = true;
    }
    else
    {
      slot.observable = create_arg_cell(graph, arg, slot.node);
      if (slot.observable.IsNull())
      {
        darg_immediate_error(vm, "StatefulComp: failed to create argument cell");
        return nullptr;
      }
    }
    inst->argSlots.push_back(eastl::move(slot));
  }

  scene->getPerfStats().statefulCtorRuns++;

  Sqrat::Object ctorResult;
  {
    BuilderEvalGuard mutationDeny(vm);
    OwnerScopeGuard scopeGuard(graph, &inst->ownerScope);
    // The ctor is the place to create state, even when the mount is reached
    // from inside a locked builder (calc_comp_size).
    ConstructionLockGuard unlock(graph, nullptr);

    SqStackChecker check(vm);
    sq_pushobject(vm, inst->type->ctorFunc.GetObject());
    sq_pushnull(vm);
    for (const StatefulInstance::ArgSlot &slot : inst->argSlots)
      sq_pushobject(vm, slot.observable.GetObject());
    if (SQ_FAILED(sq_call(vm, 1 + SQInteger(inst->argSlots.size()), SQTrue, SQTrue)))
    {
      sq_pop(vm, 1); // the closure; the VM has already reported the error
      return nullptr;
    }
    Sqrat::Var<Sqrat::Object> res(vm, -1);
    ctorResult = res.value;
    sq_pop(vm, 2); // result + closure
  }

  String ctorName;
  SQObjectType resType = ctorResult.GetType();
  if (try_get_stateful_desc(ctorResult))
  {
    get_closure_full_name(inst->type->ctorFunc, ctorName);
    darg_immediate_error(vm,
      String(0, "%s: ctor returned a descriptor; return a builder closure or a description table", ctorName.c_str()));
    return nullptr;
  }
  if (resType != OT_CLOSURE && resType != OT_TABLE && resType != OT_CLASS)
  {
    get_closure_full_name(inst->type->ctorFunc, ctorName);
    darg_immediate_error(vm,
      String(0, "%s: ctor must return a builder closure or a description table, got %s", ctorName.c_str(), sq_objtypestr(resType)));
    return nullptr;
  }

  bool built;
  {
    ConstructionLockGuard lock(graph, stateful_builder_lock_msg);
    built = Component::build_component(out_comp, ctorResult, csk, ctorResult);
  }
  if (!built)
    return nullptr; // the error has been reported

  if (!out_comp.uniqueKey.IsNull())
  {
    get_closure_full_name(inst->type->ctorFunc, ctorName);
    darg_immediate_error(vm,
      String(0, "%s: a stateful component description must not set 'key'; identity comes from the StatefulComp key function",
        ctorName.c_str()));
    return nullptr;
  }

  return inst;
}


void stateful_update_args(GuiScene *scene, const StatefulCompDesc *desc, StatefulInstance *inst)
{
  ObservablesGraph *graph = inst->graph;
  HSQUIRRELVM vm = graph->vm;
  G_ASSERT_RETURN(desc->args.size() == inst->argSlots.size(), );

  for (int i = 0, n = int(desc->args.size()); i < n; ++i)
  {
    const Sqrat::Object &arg = desc->args[i];
    StatefulInstance::ArgSlot &slot = inst->argSlots[i];
    WatchedHandle *h = try_get_observable(arg);

    if (slot.pinned)
    {
      if (!h)
        darg_immediate_error(vm, String(0, "StatefulComp: argument %d was an observable at mount but is now a value", i + 1));
      else if (h->id != slot.node)
        darg_immediate_error(vm,
          String(0, "StatefulComp: argument %d is a different observable than at mount; pass the same one", i + 1));
    }
    else
    {
      if (h)
      {
        darg_immediate_error(vm, String(0, "StatefulComp: argument %d was a value at mount but is now an observable", i + 1));
        continue;
      }

#if DAGOR_DBGLEVEL > 0
      NodeSlot *watched = graph->resolve(slot.node);
      HSQOBJECT hArg = arg.GetObject();
      bool changed = watched && !sq_obj_is_equal(vm, &watched->value, &hArg);
#endif
      graph->setValue(slot.node, arg); // FRP ignores a write of an equal value
#if DAGOR_DBGLEVEL > 0
      // Nothing reads this observable reactively, so the new value cannot reach the
      // screen: most likely the ctor read it once with get(). Says nothing
      // about pinned observables, which are shared with the caller.
      if (changed && !slot.reportedDeadWrite && !graph->nodeHasConsumers(slot.node))
      {
        slot.reportedDeadWrite = true;
        String ctorName;
        get_closure_full_name(inst->type->ctorFunc, ctorName);
        darg_immediate_error(vm, String(0,
                                   "%s: the write to argument %d cannot reach the screen - nothing reads the observable reactively; "
                                   "derive a Computed from it or watch it instead of a one-time get() in the ctor",
                                   ctorName.c_str(), i + 1));
      }
#endif
    }
  }
  G_UNUSED(scene);
}


void StatefulInstance::unsubscribe()
{
  if (graph)
    graph->unsubscribeOwnerScope(ownerScope);
}


void StatefulInstance::dispose()
{
  if (!graph)
    return;
  graph->disposeOwnerScope(ownerScope);
  argSlots.clear();
  keyValue.Release();
  typeRef.Release();
  graph = nullptr;
}


void bind_stateful_comp(HSQUIRRELVM vm, Sqrat::Table &exports)
{
  ///@class daRg/StatefulComp
  Sqrat::Class<StatefulCompType, Sqrat::NoCopy<StatefulCompType>> typeClass(vm, "StatefulComp");
  typeClass //
    .SquirrelCtor(StatefulCompType::script_ctor, -2, ".c c|o")
    .SquirrelFunc("_call", StatefulCompType::call_mm, -2)
    /**/;

  ///@class daRg/StatefulCompDesc
  Sqrat::Class<StatefulCompDesc, Sqrat::NoConstructor<StatefulCompDesc>> descClass(vm, "StatefulCompDesc");

  exports.Bind("StatefulComp", typeClass);
}

} // namespace darg
