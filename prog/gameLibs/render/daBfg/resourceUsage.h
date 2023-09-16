#pragma once

#include <render/daBfg/usage.h>
#include <render/daBfg/stage.h>
#include <render/daBfg/history.h>

#include <render/daBfg/detail/access.h>
#include <render/daBfg/detail/resourceType.h>

#include <3d/dag_drv3d.h>
#include <EASTL/optional.h>


namespace dabfg
{

struct ResourceUsage
{
  Access access = Access::UNKNOWN;
  Usage type = Usage::UNKNOWN;
  Stage stage = Stage::UNKNOWN;
};

void validate_usage(ResourceUsage usage);

ResourceBarrier barrier_for_transition(ResourceUsage usage_before, ResourceUsage usage_after);

eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, ResourceUsage usage, ResourceType res_type);

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type);

} // namespace dabfg