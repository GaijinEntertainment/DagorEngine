//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/entityId.h>
#include <daECS/core/internal/typesAndLimits.h> //for ecs::component_index_t
#include <EASTL/internal/function.h>
#include "connid.h"

namespace net
{
class IConnection;
enum class CompReplicationFilter : uint8_t
{
  SkipUntilFilterUpdate,      // not yet implemented. SkipUntilFilterEvent will always skip replication, not call filter, until
                              // update_component_filter_event() called
  SkipForConnection,          // SkipForConnection will never ever call this filter again (until entity is recreated).
  SkipNow,                    // skip replication just now, and poll filter again on next change.
  ReplicateUntilFilterUpdate, // not yet implemented. ReplicateUntilFilterEvent will always replicate to this replication, and not call
                              // filter, until update_component_filter_event() called
  ReplicateForConnection,     // ReplicateForConnection will never ever call this filter again (until entity is recreated).
  ReplicateNow,               // will replicate just now, and poll filter again on next change.
};

using component_replication_filter =
  eastl::function<CompReplicationFilter(ecs::EntityId, ConnectionId controlled_by, const IConnection *conn)>;

typedef uint8_t replicate_component_filter_index_t;
static constexpr replicate_component_filter_index_t replicate_everywhere_filter_id = 0;
static constexpr replicate_component_filter_index_t invalid_filter_id =
  eastl::numeric_limits<replicate_component_filter_index_t>::max();

// register new filter with name, will return error if such filter is already registered for different function
replicate_component_filter_index_t register_component_filter(const char *name, component_replication_filter filter);
void update_component_filter_events();                                      // retrigger poll filters for all filters
void update_component_filter_event(replicate_component_filter_index_t fid); // retrigger poll filters for specific filter

replicate_component_filter_index_t find_component_filter(const char *name); // will return replicate_everywhere_filter_id if missing
replicate_component_filter_index_t find_component_filter_for_component(ecs::component_t component); // will return invalid_filter_id if
                                                                                                    // missing

const char *get_component_filter_name(replicate_component_filter_index_t); // will return nullptr if missing

void reset_replicate_component_filters();
void set_component_filter_cidx(ecs::component_index_t cidx, replicate_component_filter_index_t filter);
void set_component_filter(ecs::component_t component, const char *filter_name);
void set_component_filter(ecs::component_t component, replicate_component_filter_index_t filter);

// for performance it is made inline
inline void update_component_filter_event(replicate_component_filter_index_t fid) // retrigger poll filters for specific filter
{
  extern uint32_t dirty_component_filter_mask;
  if (fid != 0)
    dirty_component_filter_mask |= 1 << (fid - 1);
}
} // namespace net
