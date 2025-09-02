// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>
#include <detail/storage.h>
#include <detail/unregisterAccess.h>

static das::mutex resSlotClearMutex;
static das::hash_set<const das::Context *> resSlotClearContexts;

void mark_context_for_clear_resource_slot(const das::Context *context)
{
  G_ASSERT_RETURN(context != nullptr, );

  das::lock_guard<das::mutex> lock{resSlotClearMutex};
  resSlotClearContexts.insert(context);
}

void bind_dascript::clear_resource_slot(const das::Context *context)
{
  G_ASSERT_RETURN(context != nullptr, );

  das::lock_guard<das::mutex> lock{resSlotClearMutex};

  auto it = resSlotClearContexts.find(context);
  if (it != resSlotClearContexts.end())
  {
    resSlotClearContexts.erase(it);
  }
  else
  {
    // Context was not marked for clearing, nothing to do
    return;
  }

  for (auto &record : resource_slot::detail::storage_list)
  {
    resource_slot::detail::Storage &storage = record.second;
    for (resource_slot::detail::NodeDeclaration &decl : storage.registeredNodes)
    {
      if (decl.id != resource_slot::detail::NodeId::Invalid && decl.context == context)
      {
        resource_slot::detail::unregister_access(storage, decl.id, decl.generation);
      }
    }
  }
}