// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess() : nameSpace(dabfg::root()) { G_ASSERT(false); }

void bind_dascript::NodeHandleWithSlotsAccess_reset(::resource_slot::NodeHandleWithSlotsAccess &handle) { G_ASSERT(false); }

bool resource_slot::NodeHandleWithSlotsAccess::valid() const { G_ASSERT_RETURN(false, false); }

resource_slot::NodeHandleWithSlotsAccess::~NodeHandleWithSlotsAccess() { G_ASSERT(false); }

resource_slot::State::State() : nameSpace(dabfg::root()), nodeId(-1), order(0), size(0) { G_ASSERT(false); }

void bind_dascript::ActionList_create(::resource_slot::detail::ActionList &, const char *, const char *) { G_ASSERT(false); }

void bind_dascript::ActionList_update(::resource_slot::detail::ActionList &, const char *, const char *, int) { G_ASSERT(false); }

void bind_dascript::ActionList_read(::resource_slot::detail::ActionList &, const char *, int) { G_ASSERT(false); }

::resource_slot::NodeHandleWithSlotsAccess bind_dascript::prepare_access(const ::bind_dascript::ResSlotPrepareCallBack &,
  ::das::Context *, das::LineInfoArg *)
{
  G_ASSERT_RETURN(false, {});
}

void bind_dascript::register_access(resource_slot::NodeHandleWithSlotsAccess &, dabfg::NameSpaceNameId, const char *,
  resource_slot::detail::ActionList &, ::bind_dascript::ResSlotDeclarationCallBack, das::Context *)
{
  G_ASSERT(false);
}

const char *bind_dascript::resourceToReadFrom(const resource_slot::State &state, const char *slot_name)
{
  G_ASSERT_RETURN(false, nullptr);
}

const char *bind_dascript::resourceToCreateFor(const resource_slot::State &state, const char *slot_name)
{
  G_ASSERT_RETURN(false, nullptr);
}

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(resource_slot::NodeHandleWithSlotsAccess &&) :
  nameSpace(dabfg::root())
{
  G_ASSERT(false);
}

resource_slot::NodeHandleWithSlotsAccess &resource_slot::NodeHandleWithSlotsAccess::operator=(
  resource_slot::NodeHandleWithSlotsAccess &&)
{
  G_ASSERT_RETURN(false, *this);
}

void bind_dascript::clear_resource_slot(const das::Context *) {}