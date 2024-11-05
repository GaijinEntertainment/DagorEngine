// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/usage.h>
#include <render/daBfg/stage.h>
#include <render/daBfg/history.h>

#include <backend/intermediateRepresentation.h>
#include <frontend/internalRegistry.h>

#include <render/daBfg/detail/access.h>
#include <render/daBfg/detail/resourceType.h>

#include <drv/3d/dag_driver.h>
#include <EASTL/optional.h>


namespace dabfg
{

void validate_usage(ResourceUsage usage);

ResourceBarrier barrier_for_transition(intermediate::ResourceUsage usage_before, intermediate::ResourceUsage usage_after);

eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, intermediate::ResourceUsage usage,
  ResourceType res_type);
__forceinline eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, ResourceUsage usage,
  ResourceType res_type)
{
  return get_activation_from_usage(history, intermediate::ResourceUsage{usage.access, usage.type, usage.stage}, res_type);
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type);

} // namespace dabfg