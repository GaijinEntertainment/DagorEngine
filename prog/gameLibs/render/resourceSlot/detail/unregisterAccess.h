#pragma once

#include <detail/storage.h>

namespace resource_slot::detail
{

void unregister_access(resource_slot::detail::Storage &storage, resource_slot::detail::NodeId node_id, unsigned generation);

}