// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/history.h>

#include <backend/intermediateRepresentation.h>
#include <frontend/internalRegistry.h>

#include <render/daFrameGraph/detail/access.h>
#include <render/daFrameGraph/detail/resourceType.h>

#include <drv/3d/dag_driver.h>
#include <EASTL/optional.h>


namespace dafg
{

enum class ValidateUsageResult
{
  Invalid,
  IncompatibleActivation,
  EmptyActivation,
  OK
};

ValidateUsageResult validate_usage(ResourceUsage usage, ResourceType res_type, History history);

ResourceBarrier barrier_for_transition(intermediate::ResourceUsage usage_before, intermediate::ResourceUsage usage_after);

eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, intermediate::ResourceUsage usage,
  ResourceType res_type);
__forceinline eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, ResourceUsage usage,
  ResourceType res_type)
{
  return get_activation_from_usage(history, intermediate::ResourceUsage{usage.type, usage.access, usage.stage}, res_type);
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type);
ResourceActivationAction get_history_activation(History history, ResourceActivationAction res_activaton, bool is_int = false);

} // namespace dafg
