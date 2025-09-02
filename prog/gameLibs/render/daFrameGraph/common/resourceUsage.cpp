// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceUsage.h"

#include <debug/dag_assert.h>

#include <EASTL/utility.h>
#include <EASTL/span.h>
#include <math/dag_bits.h>


namespace dafg
{

eastl::string to_string(Usage usage_flags)
{
  eastl::string result;

  if (usage_flags == Usage::UNKNOWN)
    return "UNKNOWN";

  auto mask = eastl::to_underlying(usage_flags);
  while (mask)
  {
    uint32_t bit_index = __bsf(mask);
    Usage usage = (Usage)(1u << bit_index);

    switch (usage)
    {
      case Usage::UNKNOWN: result += "UNKNOWN"; break;
      case Usage::COLOR_ATTACHMENT: result += "COLOR_ATTACHMENT"; break;
      case Usage::DEPTH_ATTACHMENT: result += "DEPTH_ATTACHMENT"; break;
      case Usage::RESOLVE_ATTACHMENT: result += "RESOLVE_ATTACHMENT"; break;
      case Usage::SHADER_RESOURCE: result += "SHADER_RESOURCE"; break;
      case Usage::CONSTANT_BUFFER: result += "CONSTANT_BUFFER"; break;
      case Usage::INDEX_BUFFER: result += "INDEX_BUFFER"; break;
      case Usage::VERTEX_BUFFER: result += "VERTEX_BUFFER"; break;
      case Usage::COPY: result += "COPY"; break;
      case Usage::BLIT: result += "BLIT"; break;
      case Usage::INDIRECTION_BUFFER: result += "INDIRECTION_BUFFER"; break;
      case Usage::VRS_RATE_TEXTURE: result += "VRS_RATE_TEXTURE"; break;
      case Usage::INPUT_ATTACHMENT: result += "INPUT_ATTACHMENT"; break;
      case Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE: result += "DEPTH_ATTACHMENT_AND_SHADER_RESOURCE"; break;
    }
    result += " | ";

    mask &= ~(1u << bit_index);
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

eastl::string to_string(Stage stage_flags)
{
  eastl::string result;

  if (stage_flags == Stage::UNKNOWN)
    return "UNKNOWN";

  auto mask = eastl::to_underlying(stage_flags);
  while (mask)
  {
    uint32_t bit_index = __bsf(mask);
    Stage stage = (Stage)(1u << bit_index);

    switch (stage)
    {
      case Stage::UNKNOWN: result += "UNKNOWN"; break;
      case Stage::PRE_RASTER: result += "PRE_RASTER"; break;
      case Stage::POST_RASTER: result += "POST_RASTER"; break;
      case Stage::COMPUTE: result += "COMPUTE"; break;
      case Stage::TRANSFER: result += "TRANSFER"; break;
      case Stage::RAYTRACE: result += "RAYTRACE"; break;
      case Stage::PS_OR_CS: result += "PS_OR_CS"; break;
      case Stage::ALL_GRAPHICS: result += "ALL_GRAPHICS"; break;
      case Stage::ALL_INDIRECTION: result += "ALL_INDIRECTION"; break;
    }
    result += " | ";

    mask &= ~(1u << bit_index);
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

const char *to_string(Access access)
{
  switch (access)
  {
    case Access::UNKNOWN: return "UNKNOWN";
    case Access::READ_ONLY: return "READ_ONLY";
    case Access::READ_WRITE: return "READ_WRITE";
  }

  return "Invalid";
}

const char *to_string(History history)
{
  switch (history)
  {
    case History::No: return "No";
    case History::ClearZeroOnFirstFrame: return "ClearZeroOnFirstFrame";
    case History::DiscardOnFirstFrame: return "DiscardOnFirstFrame";
  }

  return "Invalid";
}

static eastl::string access_conjunction_name(uint32_t access_conjunction)
{
  eastl::string result;

  while (access_conjunction)
  {
    uint32_t bit_index = __bsf(access_conjunction);
    Access access = (Access)(1u << bit_index);

    result += to_string(access);
    result += " | ";

    access_conjunction &= ~(1u << bit_index);
  }

  if (!result.empty())
    result.resize(result.size() - 3);

  return result;
}

static bool set_check_activation(History history, eastl::optional<ResourceActivationAction> &activation,
  eastl::optional<ResourceActivationAction> discard_activation, eastl::optional<ResourceActivationAction> clear_activation)
{
  // NOTE: If our usage type has several bits set for now we would like to make sure that they all use the same activation action
  eastl::optional<ResourceActivationAction> new_activation =
    (history == History::DiscardOnFirstFrame) ? discard_activation : clear_activation;

  if (new_activation != eastl::nullopt)
  {
    if (activation != eastl::nullopt)
      return activation == new_activation;
    else
      activation = new_activation;
  }

  return true;
}

ValidateUsageResult validate_usage(ResourceUsage usage, ResourceType res_type, History history)
{
  if (usage.type == Usage::UNKNOWN)
  {
    if (usage.stage != Stage::UNKNOWN)
    {
      G_ASSERTF(false, "Usage with UNKNOWN type has non-zero stage. Resource usage initialization may be wrong!");
      return ValidateUsageResult::Invalid;
    }

    if (history != History::No)
    {
      logerr("daFG: Resource requested a history action but no usage is provided!"
             " This may cause driver validation errors! Please, provide a usage.");
      return ValidateUsageResult::Invalid;
    }

    return ValidateUsageResult::OK;
  }

  uint32_t allowed_access = uint32_t(Access::READ_ONLY) | uint32_t(Access::READ_WRITE);
  Stage allowed_stage = Stage::PRE_RASTER | Stage::POST_RASTER | Stage::COMPUTE | Stage::TRANSFER | Stage::RAYTRACE;

  auto mask = eastl::to_underlying(usage.type);
  while (mask)
  {
    uint32_t bit_index = __bsf(mask);
    Usage type = (Usage)(1u << bit_index);
    switch (type)
    {
      case Usage::UNKNOWN: break;

      case Usage::COLOR_ATTACHMENT:
      case Usage::DEPTH_ATTACHMENT:
        allowed_stage &= Stage::POST_RASTER;
        allowed_access &= uint32_t(Access::READ_ONLY) | uint32_t(Access::READ_WRITE);
        break;

      case Usage::RESOLVE_ATTACHMENT:
        allowed_stage &= Stage::TRANSFER;
        allowed_access &= uint32_t(Access::READ_WRITE);
        break;

      case Usage::VRS_RATE_TEXTURE:
        // PVS was complaining that '&=' operation always sets a value of 'allowed_access' variable to zero but this is not true
        allowed_stage &= Stage::POST_RASTER;
        allowed_access &= uint32_t(Access::READ_ONLY); // -V753
        break;

      case Usage::BLIT:
      case Usage::COPY:
        allowed_stage &= Stage::TRANSFER;
        allowed_access &= uint32_t(Access::READ_ONLY) | uint32_t(Access::READ_WRITE);
        break;

      case Usage::SHADER_RESOURCE:
        allowed_stage &= ~Stage::TRANSFER;
        allowed_access &= uint32_t(Access::READ_ONLY) | uint32_t(Access::READ_WRITE);
        break;

      case Usage::CONSTANT_BUFFER:
        allowed_stage &= ~Stage::TRANSFER;
        allowed_access &= uint32_t(Access::READ_ONLY);
        break;

      case Usage::INDEX_BUFFER:
      case Usage::VERTEX_BUFFER:
        allowed_stage &= Stage::PRE_RASTER;
        allowed_access &= uint32_t(Access::READ_ONLY);
        break;

      case Usage::INDIRECTION_BUFFER:
        allowed_stage &= Stage::ALL_INDIRECTION;
        allowed_access &= uint32_t(Access::READ_ONLY);
        break;

      default: break;
    }

    mask &= ~(1u << bit_index);
  }

  if ((usage.type & Usage::INPUT_ATTACHMENT) == Usage::INPUT_ATTACHMENT)
    allowed_access &= uint32_t(Access::READ_ONLY);

  if ((usage.type & Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE) == Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
    allowed_access &= uint32_t(Access::READ_ONLY);

  bool valid_conjunctions = true;
  if ((uint32_t(usage.access) & allowed_access) == 0)
  {
    logerr("daFG: Conjunction of what the usages imply '%s' doesn't support provided access '%s'.",
      access_conjunction_name(allowed_access), to_string(usage.access));
    valid_conjunctions = false;
  }
  if ((usage.stage & allowed_stage) == Stage::UNKNOWN)
  {
    logerr("daFG: Conjunction of what the usages imply '%s' doesn't support provided stage '%s'.", to_string(allowed_stage),
      to_string(usage.stage));
    valid_conjunctions = false;
  }
  if (!valid_conjunctions)
  {
    logerr("daFG: Provided usage type was '%s'", to_string(usage.type));
    G_ASSERT(false);
    return ValidateUsageResult::Invalid;
  }


  // Check if all usages have the same activation function if any
  if (history == History::No)
    return ValidateUsageResult::OK;

  // Let's first check activation and then, if it's empty, complain to logger
  eastl::optional<ResourceActivationAction> activation = eastl::nullopt;
  bool compatible_activations = true;
  mask = eastl::to_underlying(usage.type);
  while (mask)
  {
    uint32_t bit_index = __bsf(mask);
    Usage type = (Usage)(1u << bit_index);
    switch (type)
    {
      case Usage::UNKNOWN: break;

      case Usage::COLOR_ATTACHMENT:
      case Usage::DEPTH_ATTACHMENT:
      case Usage::RESOLVE_ATTACHMENT:
      case Usage::BLIT:
        compatible_activations &= set_check_activation(history, activation, ResourceActivationAction::DISCARD_AS_RTV_DSV,
          ResourceActivationAction::CLEAR_AS_RTV_DSV);
        break;

      case Usage::VRS_RATE_TEXTURE:
        compatible_activations &=
          set_check_activation(history, activation, eastl::nullopt, ResourceActivationAction::CLEAR_AS_RTV_DSV);
        break;

      case Usage::SHADER_RESOURCE:
        if ((usage.access == Access::READ_WRITE) || (res_type == ResourceType::Buffer))
          compatible_activations &= set_check_activation(history, activation, ResourceActivationAction::DISCARD_AS_UAV,
            ResourceActivationAction::CLEAR_I_AS_UAV);
        else if (res_type == ResourceType::Texture)
          compatible_activations &= set_check_activation(history, activation, ResourceActivationAction::DISCARD_AS_RTV_DSV,
            ResourceActivationAction::CLEAR_AS_RTV_DSV);
        else
        {
          G_ASSERTF(false, "Unhandled resource type! This should never happen!");
          return ValidateUsageResult::Invalid;
        }
        break;

      case Usage::CONSTANT_BUFFER:
        compatible_activations &= set_check_activation(history, activation, ResourceActivationAction::DISCARD_AS_UAV,
          ResourceActivationAction::CLEAR_I_AS_UAV);
        break;

      case Usage::INDEX_BUFFER:
      case Usage::VERTEX_BUFFER:
      case Usage::INDIRECTION_BUFFER:
        compatible_activations &= set_check_activation(history, activation, eastl::nullopt, ResourceActivationAction::CLEAR_I_AS_UAV);
        break;

      case Usage::COPY:
        compatible_activations &= set_check_activation(history, activation, ResourceActivationAction::REWRITE_AS_COPY_DESTINATION,
          ResourceActivationAction::CLEAR_I_AS_UAV);
        break;

      default: break;
    }
    mask &= ~(1u << bit_index);
  }

  if (!compatible_activations)
  {
    logerr("daFG: Provided usage type was '%s', provided history was '%s'", to_string(usage.type), to_string(history));
    return ValidateUsageResult::IncompatibleActivation;
  }

  // If after going through every usage bit we still don't have activation, let's complain to logger
  if (activation == eastl::nullopt)
  {
    mask = eastl::to_underlying(usage.type);
    while (mask)
    {
      uint32_t bit_index = __bsf(mask);
      Usage type = (Usage)(1u << bit_index);
      switch (type)
      {
        case Usage::UNKNOWN: break;

        case Usage::COLOR_ATTACHMENT:
        case Usage::DEPTH_ATTACHMENT:
        case Usage::RESOLVE_ATTACHMENT:
        case Usage::BLIT:
        case Usage::COPY:
        case Usage::CONSTANT_BUFFER:
        case Usage::SHADER_RESOURCE:
        {
          G_ASSERTF(false, "These usages should have an activation.");
          return ValidateUsageResult::Invalid;
        }

        case Usage::INDEX_BUFFER:
        case Usage::VERTEX_BUFFER:
        case Usage::INDIRECTION_BUFFER:
          logerr("daFG: Attempted to deduce activation of a discarded buffer used as a "
                 "geometry buffer (vertex/index/indirection)! This is likely an application error!");
          break;

        case Usage::VRS_RATE_TEXTURE:
          logerr("daFG: Attempted to deduce activation of a discarded texture used as a "
                 "VRS rate texture! This is likely an application error!");
          break;

        default: break;
      }
      mask &= ~(1u << bit_index);
    }

    logerr("daFG: Provided usage type was '%s', provided history was '%s'", to_string(usage.type), to_string(history));
    return ValidateUsageResult::EmptyActivation;
  }

  return ValidateUsageResult::OK;
}

using StageMapEntry = eastl::pair<Stage, ResourceBarrier>;

static constexpr StageMapEntry USAGE_STAGE_TO_BARRIER_DEST_STAGE[]{
  {Stage::UNKNOWN, RB_NONE},
  {Stage::PRE_RASTER, RB_STAGE_VERTEX},
  {Stage::POST_RASTER, RB_STAGE_PIXEL},
  {Stage::COMPUTE, RB_STAGE_COMPUTE},
  {Stage::TRANSFER, RB_NONE},
  {Stage::RAYTRACE, RB_STAGE_RAYTRACE},
};

static ResourceBarrier map_stage_flags(eastl::span<StageMapEntry const> map, Stage stage)
{
  int result = RB_NONE;
  for (auto &[stageFlag, d3dStage] : map)
  {
    const bool flagPresent = stageFlag == (stage & stageFlag);
    result |= flagPresent ? d3dStage : RB_NONE;
  }
  return static_cast<ResourceBarrier>(result);
}

static ResourceBarrier map_stage_flags_dest(Stage stage) { return map_stage_flags(USAGE_STAGE_TO_BARRIER_DEST_STAGE, stage); }

ResourceBarrier barrier_for_transition(intermediate::ResourceUsage usage_before, intermediate::ResourceUsage usage_after)
{
  if (usage_before.access == usage_after.access &&
      usage_before.type == usage_after.type
      // NOTE: if we require less stages than previous usage,
      // no barrier is needed
      && stagesSubsetOf(usage_after.stage, usage_before.stage))
    return RB_NONE;

  ResourceBarrier resource_barrier = RB_NONE;

  if ((usage_after.type & Usage::VERTEX_BUFFER) != Usage::UNKNOWN)
    resource_barrier = resource_barrier | RB_RO_VERTEX_BUFFER;

  if ((usage_after.type & Usage::INDEX_BUFFER) != Usage::UNKNOWN)
    resource_barrier = resource_barrier | RB_RO_INDEX_BUFFER;

  if ((usage_after.type & Usage::INDIRECTION_BUFFER) != Usage::UNKNOWN)
    resource_barrier = resource_barrier | RB_RO_INDIRECT_BUFFER;

  if ((usage_after.type & Usage::VRS_RATE_TEXTURE) != Usage::UNKNOWN)
    resource_barrier = resource_barrier | RB_RO_VARIABLE_RATE_SHADING_TEXTURE;

  if (((usage_after.type & Usage::BLIT) != Usage::UNKNOWN) &&
      ((usage_before.type & Usage::BLIT) == Usage::UNKNOWN || usage_before.access != usage_after.access))
  {
    if (usage_after.access == Access::READ_ONLY)
      resource_barrier = resource_barrier | RB_RO_BLIT_SOURCE;
    else
      resource_barrier = resource_barrier | RB_RW_BLIT_DEST;
  }

  if (((usage_after.type & Usage::COPY) != Usage::UNKNOWN) &&
      ((usage_before.type & Usage::COPY) == Usage::UNKNOWN || usage_before.access != usage_after.access))
  {
    if (usage_after.access == Access::READ_ONLY)
      resource_barrier = resource_barrier | RB_RO_COPY_SOURCE;
    else
      resource_barrier = resource_barrier | RB_RW_COPY_DEST;
  }

  if (((usage_after.type & Usage::SHADER_RESOURCE) != Usage::UNKNOWN) && usage_after.access == Access::READ_ONLY)
    resource_barrier = resource_barrier | RB_RO_SRV | map_stage_flags_dest(usage_after.stage);

  if (((usage_after.type & Usage::SHADER_RESOURCE) != Usage::UNKNOWN) && usage_after.access == Access::READ_WRITE)
    resource_barrier = resource_barrier | RB_RW_UAV | map_stage_flags_dest(usage_after.stage);

  if ((usage_after.type & Usage::CONSTANT_BUFFER) != Usage::UNKNOWN)
    resource_barrier = resource_barrier | RB_RO_CONSTANT_BUFFER | map_stage_flags_dest(usage_after.stage);

  if ((((usage_after.type & Usage::COLOR_ATTACHMENT) != Usage::UNKNOWN) ||
        ((usage_after.type & Usage::DEPTH_ATTACHMENT) != Usage::UNKNOWN))
      // No barrier needed when changing access
      && usage_before.type != usage_after.type)
    resource_barrier = resource_barrier | RB_RW_RENDER_TARGET;

  G_ASSERTF(usage_after.type != Usage::RESOLVE_ATTACHMENT, "MSAA resolve attachments are not implemented yet.");

  return resource_barrier;
}

eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, intermediate::ResourceUsage usage,
  ResourceType res_type)
{
  if (usage.type == Usage::UNKNOWN)
  {
    return eastl::nullopt;
  }

  switch (history)
  {
    case History::No: return eastl::nullopt;

    case History::DiscardOnFirstFrame:
    {
      uint32_t mask = uint32_t(usage.type);
      while (mask)
      {
        uint32_t bit_index = __bsf(mask);
        Usage type = (Usage)(1u << bit_index);
        switch (type)
        {
          case Usage::UNKNOWN: break;

          case Usage::COPY: return ResourceActivationAction::REWRITE_AS_COPY_DESTINATION;

          case Usage::BLIT:
          case Usage::COLOR_ATTACHMENT:
          case Usage::DEPTH_ATTACHMENT:
          case Usage::RESOLVE_ATTACHMENT: return ResourceActivationAction::DISCARD_AS_RTV_DSV; break;

          case Usage::SHADER_RESOURCE:
            if (usage.access == Access::READ_WRITE)
              return ResourceActivationAction::DISCARD_AS_UAV;
            else if (res_type == ResourceType::Texture)
              return ResourceActivationAction::DISCARD_AS_RTV_DSV;
            else if (res_type == ResourceType::Buffer)
              return ResourceActivationAction::DISCARD_AS_UAV;
            break;

          case Usage::CONSTANT_BUFFER: return ResourceActivationAction::DISCARD_AS_UAV; break;

          case Usage::VRS_RATE_TEXTURE:
          case Usage::VERTEX_BUFFER:
          case Usage::INDEX_BUFFER:
          case Usage::INDIRECTION_BUFFER: break;

          default: break;
        }
        mask &= ~(1u << bit_index);
      }
    }
    break;

    case History::ClearZeroOnFirstFrame:
    {
      uint32_t mask = uint32_t(usage.type);
      while (mask)
      {
        uint32_t bit_index = __bsf(mask);
        Usage type = (Usage)(1u << bit_index);
        switch (type)
        {
          case Usage::UNKNOWN: break;

          case Usage::COPY:
          case Usage::CONSTANT_BUFFER:
          case Usage::VERTEX_BUFFER:
          case Usage::INDEX_BUFFER:
          case Usage::INDIRECTION_BUFFER: return ResourceActivationAction::CLEAR_I_AS_UAV;

          case Usage::BLIT:
          case Usage::COLOR_ATTACHMENT:
          case Usage::DEPTH_ATTACHMENT:
          case Usage::RESOLVE_ATTACHMENT:
          case Usage::VRS_RATE_TEXTURE: return ResourceActivationAction::CLEAR_AS_RTV_DSV;

          case Usage::SHADER_RESOURCE:
            if (usage.access == Access::READ_WRITE)
              return ResourceActivationAction::CLEAR_I_AS_UAV;
            else if (res_type == ResourceType::Texture)
              return ResourceActivationAction::CLEAR_AS_RTV_DSV;
            else if (res_type == ResourceType::Buffer)
              return ResourceActivationAction::CLEAR_I_AS_UAV;
            break;

          default: break;
        }
        mask &= ~(1u << bit_index);
      }
    }
    break;
  }

  return {};
}

ResourceActivationAction get_history_activation(History history, ResourceActivationAction res_activaton, bool is_int)
{
  switch (history)
  {
    case History::DiscardOnFirstFrame:
    {
      switch (res_activaton)
      {
        case ResourceActivationAction::REWRITE_AS_RTV_DSV:
        case ResourceActivationAction::DISCARD_AS_RTV_DSV:
        case ResourceActivationAction::CLEAR_AS_RTV_DSV: return ResourceActivationAction::DISCARD_AS_RTV_DSV;

        case ResourceActivationAction::REWRITE_AS_UAV:
        case ResourceActivationAction::DISCARD_AS_UAV:
        case ResourceActivationAction::CLEAR_I_AS_UAV: return ResourceActivationAction::DISCARD_AS_UAV;

        default: break;
      }
    }
    break;

    case History::ClearZeroOnFirstFrame:
    {
      switch (res_activaton)
      {
        case ResourceActivationAction::REWRITE_AS_RTV_DSV:
        case ResourceActivationAction::DISCARD_AS_RTV_DSV:
        case ResourceActivationAction::CLEAR_AS_RTV_DSV: return ResourceActivationAction::CLEAR_AS_RTV_DSV;

        case ResourceActivationAction::REWRITE_AS_UAV:
        case ResourceActivationAction::DISCARD_AS_UAV:
        case ResourceActivationAction::CLEAR_I_AS_UAV:
          return is_int ? ResourceActivationAction::CLEAR_I_AS_UAV : ResourceActivationAction::CLEAR_F_AS_UAV;

        default: break;
      }
    }
    break;
    default: break;
  }
  G_ASSERTF(false, "History is not set! This should never happen!");
  return {};
}

static uint32_t get_creation_flags_from_usage(ResourceUsage usage, ResourceType res_type)
{
  uint32_t flags = 0;

  uint32_t mask = uint32_t(usage.type);
  while (mask)
  {
    uint32_t bit_index = __bsf(mask);
    Usage type = (Usage)(1u << bit_index);
    switch (type)
    {
      case Usage::COPY:
      case Usage::BLIT:
        if (usage.access == Access::READ_WRITE)
          flags |= TEXCF_UPDATE_DESTINATION;
        break;
      case Usage::COLOR_ATTACHMENT:
      case Usage::DEPTH_ATTACHMENT:
      case Usage::RESOLVE_ATTACHMENT: flags |= TEXCF_RTARGET; break;
      case Usage::SHADER_RESOURCE:
        if (res_type == ResourceType::Texture && usage.access == Access::READ_WRITE)
          flags |= TEXCF_UNORDERED;
        else if (res_type == ResourceType::Buffer)
          flags |= SBCF_BIND_UNORDERED;
        break;
      case Usage::VERTEX_BUFFER: flags |= SBCF_BIND_VERTEX; break;
      case Usage::INDEX_BUFFER: flags |= SBCF_BIND_INDEX; break;
      case Usage::INDIRECTION_BUFFER: flags |= SBCF_MISC_DRAWINDIRECT; break;
      case Usage::VRS_RATE_TEXTURE: flags |= TEXCF_VARIABLE_RATE; break;
      case Usage::CONSTANT_BUFFER: flags |= SBCF_BIND_CONSTANT; break;
      case Usage::UNKNOWN: break;
      default: break;
    }

    mask &= ~(1u << bit_index);
  }

  return flags;
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type)
{
  auto newFlags = get_creation_flags_from_usage(usage, res_type);
  if (newFlags == TEXCF_UPDATE_DESTINATION && (flags & (TEXCF_RTARGET | TEXCF_UNORDERED)))
    newFlags = 0;

  flags |= newFlags;
}

} // namespace dafg
