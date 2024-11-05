// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>
#include <detail/storage.h>
#include <detail/unregisterAccess.h>

static das::mutex resSlotClearMutex;

void bind_dascript::clear_resource_slot(const das::Context *context)
{
  G_ASSERT_RETURN(context != nullptr, );

  das::lock_guard<das::mutex> lock{resSlotClearMutex};
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