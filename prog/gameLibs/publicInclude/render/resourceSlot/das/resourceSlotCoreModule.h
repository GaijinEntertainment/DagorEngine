//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <render/resourceSlot/registerAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccessVector.h>
#include <render/daBfg/das/nameSpaceNameId.h>
#include <render/daBfg/das/nodeHandle.h>


namespace das
{

template <>
struct das_auto_cast_move<::resource_slot::NodeHandleWithSlotsAccess>
{
  inline static ::resource_slot::NodeHandleWithSlotsAccess cast(::resource_slot::NodeHandleWithSlotsAccess &expr)
  {
    return eastl::move(expr);
  }

  inline static ::resource_slot::NodeHandleWithSlotsAccess cast(::resource_slot::NodeHandleWithSlotsAccess &&expr)
  {
    return eastl::move(expr);
  }
};

} // namespace das

MAKE_TYPE_FACTORY(NodeHandleWithSlotsAccess, ::resource_slot::NodeHandleWithSlotsAccess)
DAS_BIND_VECTOR(NodeHandleWithSlotsAccessVector, ::resource_slot::NodeHandleWithSlotsAccessVector,
  ::resource_slot::NodeHandleWithSlotsAccess, "::resource_slot::NodeHandleWithSlotsAccessVector")
MAKE_TYPE_FACTORY(State, ::resource_slot::State)
MAKE_TYPE_FACTORY(SlotAction, ::resource_slot::detail::SlotAction)
MAKE_TYPE_FACTORY(ActionList, ::resource_slot::detail::ActionList)


namespace bind_dascript
{

void NodeHandleWithSlotsAccess_reset(::resource_slot::NodeHandleWithSlotsAccess &handle);

using ResSlotPrepareCallBack = das::TBlock<void, ::resource_slot::NodeHandleWithSlotsAccess &, ::resource_slot::detail::ActionList &>;
::resource_slot::NodeHandleWithSlotsAccess prepare_access(const ::bind_dascript::ResSlotPrepareCallBack &prepare_callback,
  ::das::Context *context, das::LineInfoArg *at);

using ResSlotDeclarationCallBack = das::TLambda<void, dabfg::NodeHandle &, ::resource_slot::State &>;
void register_access(::resource_slot::NodeHandleWithSlotsAccess &handle, dabfg::NameSpaceNameId ns, const char *name,
  ::resource_slot::detail::ActionList &action_list, ::bind_dascript::ResSlotDeclarationCallBack declaration_callback,
  das::Context *context);

void ActionList_create(::resource_slot::detail::ActionList &list, const char *slot_name, const char *resource_name);
void ActionList_update(::resource_slot::detail::ActionList &list, const char *slot_name, const char *resource_name, int priority);
void ActionList_read(::resource_slot::detail::ActionList &list, const char *slot_name, int priority);

const char *resourceToReadFrom(const resource_slot::State &state, const char *slot_name);
const char *resourceToCreateFor(const resource_slot::State &state, const char *slot_name);

void clear_resource_slot(const das::Context *context);

} // namespace bind_dascript