// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <detail/storage.h>

resource_slot::detail::StorageList resource_slot::detail::storage_list;
resource_slot::detail::ResSlotNameMap<resource_slot::detail::SlotId> resource_slot::detail::slot_map;
resource_slot::detail::ResSlotNameMap<resource_slot::detail::ResourceId> resource_slot::detail::resource_map;
