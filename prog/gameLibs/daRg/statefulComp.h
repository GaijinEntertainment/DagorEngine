// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sqrat.h>
#include <quirrel/frp/dag_frp.h>
#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>

namespace darg
{

class Component;
class GuiScene;

// Created by StatefulComp(ctor, key), normally at module scope. Its reference
// is the component identity that matching uses.
class StatefulCompType
{
public:
  Sqrat::Object ctorFunc;
  Sqrat::Object keyFunc;          // null = unkeyed
  dag::Vector<int> keyArgIndices; // ctor args the key function reads, bound by name at declaration
  int numArgs = 0;

  void abandonScriptRefs();

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger call_mm(HSQUIRRELVM vm); // _call: (type, args...) -> descriptor
};


// Result of calling a type: inert, the ctor has not run yet. Legal only in a
// children slot, and discarded after reconciliation.
class StatefulCompDesc
{
public:
  Sqrat::Object typeRef; // keeps the type instance alive
  StatefulCompType *type = nullptr;
  dag::Vector<Sqrat::Object> args;
  Sqrat::Object keyValue; // primitive or null

  void abandonScriptRefs();
};


// Mounted state: argument slots plus an owner scope holding everything the
// ctor created. Lives exactly as long as its element.
class StatefulInstance
{
public:
  struct ArgSlot
  {
    Sqrat::Object observable; // created at mount for a value argument, or the pinned external observable
    sqfrp::NodeId node;
    bool pinned = false;
    bool reportedDeadWrite = false; // dev diagnostic fired once for this observable
  };

  Sqrat::Object typeRef;
  StatefulCompType *type = nullptr;
  Sqrat::Object keyValue;
  dag::Vector<ArgSlot> argSlots;
  sqfrp::OwnerScope ownerScope;
  sqfrp::ObservablesGraph *graph = nullptr;

  ~StatefulInstance() { dispose(); }
  // Called at detach: stop reacting at once, the values live until dispose.
  void unsubscribe();
  // Releases the owner scope before the obsevables: ctor-created nodes read them.
  void dispose();
};


// ---- reconciler interface ----

// Returns null for anything that is not a descriptor.
StatefulCompDesc *try_get_stateful_desc(const Sqrat::Object &obj);

bool stateful_desc_matches_instance(const StatefulCompDesc *desc, const StatefulInstance *inst);

// Runs the ctor and evaluates its first description into out_comp. Returns
// null on failure, with the error already reported.
eastl::unique_ptr<StatefulInstance> stateful_mount(GuiScene *scene, StatefulCompDesc *desc, Component &out_comp);

// Writes new argument values into a matched instance's cells.
void stateful_update_args(GuiScene *scene, const StatefulCompDesc *desc, StatefulInstance *inst);

extern const char *const stateful_builder_lock_msg;

void bind_stateful_comp(HSQUIRRELVM vm, Sqrat::Table &exports);

} // namespace darg
