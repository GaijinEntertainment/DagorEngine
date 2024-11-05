// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceUsage.h"

#include <debug/dag_assert.h>

#include <EASTL/utility.h>
#include <EASTL/span.h>


namespace dabfg
{

void validate_usage(ResourceUsage usage)
{
  switch (usage.type)
  {
    case Usage::UNKNOWN:
      G_ASSERT(usage.access == Access::UNKNOWN);
      G_ASSERT(usage.stage == Stage::UNKNOWN);
      break;

    case Usage::COLOR_ATTACHMENT:
    case Usage::DEPTH_ATTACHMENT:
      G_ASSERT(usage.stage == Stage::POST_RASTER);
      G_ASSERT(usage.access != Access::UNKNOWN);
      break;

    case Usage::RESOLVE_ATTACHMENT:
      G_ASSERT(usage.stage == Stage::TRANSFER);
      G_ASSERT(usage.access == Access::READ_WRITE);
      break;

    case Usage::INPUT_ATTACHMENT:
      G_ASSERT(usage.stage == Stage::POST_RASTER);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;

    case Usage::BLIT:
    case Usage::COPY:
      G_ASSERT(usage.stage == Stage::TRANSFER);
      // RO or RW determines source or destination
      G_ASSERT(usage.access == Access::READ_ONLY || usage.access == Access::READ_WRITE);
      break;

    case Usage::SHADER_RESOURCE:
      G_ASSERT(usage.stage != Stage::TRANSFER);
      // RW or RO determines UAV or SRV
      G_ASSERT(usage.access == Access::READ_WRITE || usage.access == Access::READ_ONLY);
      break;

    case Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE:
      G_ASSERT((usage.stage & Stage::ALL_GRAPHICS) == usage.stage);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;

    case Usage::CONSTANT_BUFFER:
      G_ASSERT(usage.stage != Stage::TRANSFER);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;

    case Usage::INDEX_BUFFER:
    case Usage::VERTEX_BUFFER:
      G_ASSERT(usage.stage == Stage::PRE_RASTER);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;

    case Usage::INDIRECTION_BUFFER:
      G_ASSERT((usage.stage & Stage::ALL_INDIRECTION) == usage.stage);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;


    case Usage::VRS_RATE_TEXTURE:
      G_ASSERT(usage.stage == Stage::PRE_RASTER);
      G_ASSERT(usage.access == Access::READ_ONLY);
      break;
  }
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

// Stage and access are pre-determined for these, therefore if we got
// here, usage type before was different.
#define SHORT_CIRCUIT_AUTOSTAGE(TYPE, RESULT) \
  if (usage_after.type == Usage::TYPE)        \
    return RESULT;

  SHORT_CIRCUIT_AUTOSTAGE(VERTEX_BUFFER, RB_RO_VERTEX_BUFFER)
  SHORT_CIRCUIT_AUTOSTAGE(INDEX_BUFFER, RB_RO_INDEX_BUFFER)
  SHORT_CIRCUIT_AUTOSTAGE(INDIRECTION_BUFFER, RB_RO_INDIRECT_BUFFER)
  SHORT_CIRCUIT_AUTOSTAGE(VRS_RATE_TEXTURE, RB_RO_VARIABLE_RATE_SHADING_TEXTURE)

#undef SHORT_CIRCUIT_AUTOSTAGE

// Stage is always transfer for these, so barrier is needed either
// when usage type changes, or acces type is different.
#define SHORT_CIRCUIT_TRANSFER(TYPE, RES_SRC, RES_DST)                                                                    \
  if (usage_after.type == Usage::TYPE && (usage_before.type != Usage::TYPE || usage_before.access != usage_after.access)) \
  {                                                                                                                       \
    if (usage_after.access == Access::READ_ONLY)                                                                          \
      return RES_SRC;                                                                                                     \
    else                                                                                                                  \
      return RES_DST;                                                                                                     \
  }

  SHORT_CIRCUIT_TRANSFER(BLIT, RB_RO_BLIT_SOURCE, RB_RW_BLIT_DEST)
  SHORT_CIRCUIT_TRANSFER(COPY, RB_RO_COPY_SOURCE, RB_RW_COPY_DEST)

#undef SHORT_CIRCUIT_TRANSFER


  if (usage_after.type == Usage::SHADER_RESOURCE && usage_after.access == Access::READ_ONLY)
    return RB_RO_SRV | map_stage_flags_dest(usage_after.stage);

  if (usage_after.type == Usage::SHADER_RESOURCE && usage_after.access == Access::READ_WRITE)
    return RB_RW_UAV | map_stage_flags_dest(usage_after.stage);

  if (usage_after.type == Usage::CONSTANT_BUFFER)
    return RB_RO_CONSTANT_BUFFER | map_stage_flags_dest(usage_after.stage);

  if (usage_after.type == Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE)
    return RB_RO_CONSTANT_DEPTH_STENCIL_TARGET | map_stage_flags_dest(usage_after.stage);

  if ((usage_after.type == Usage::COLOR_ATTACHMENT || usage_after.type == Usage::DEPTH_ATTACHMENT)
      // No barrier needed when changing access
      && usage_before.type != usage_after.type)
    return RB_RW_RENDER_TARGET;

  G_ASSERTF(usage_after.type != Usage::INPUT_ATTACHMENT, "MSAA resolve attachments are not implemented yet.");
  G_ASSERTF(usage_after.type != Usage::RESOLVE_ATTACHMENT, "Subpass input attachments are not implemented yet.");

  return RB_NONE;
}

eastl::optional<ResourceActivationAction> get_activation_from_usage(History history, intermediate::ResourceUsage usage,
  ResourceType res_type)
{
  switch (history)
  {
    case History::No: return eastl::nullopt;
    case History::DiscardOnFirstFrame:
      switch (usage.type)
      {
        case Usage::COPY: return ResourceActivationAction::REWRITE_AS_COPY_DESTINATION;

        case Usage::BLIT:
        case Usage::COLOR_ATTACHMENT:
        case Usage::DEPTH_ATTACHMENT:
        case Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE:
        case Usage::RESOLVE_ATTACHMENT: return ResourceActivationAction::DISCARD_AS_RTV_DSV;

        case Usage::INPUT_ATTACHMENT:
          logerr("Attempted to deduce activation of a discarded texture used as an input "
                 "attachment! This is likely an application error!");
          return eastl::nullopt;

        case Usage::SHADER_RESOURCE:
          if (usage.access == Access::READ_WRITE)
            return ResourceActivationAction::DISCARD_AS_UAV;
          else if (res_type == ResourceType::Texture)
            return ResourceActivationAction::DISCARD_AS_RTV_DSV;
          else if (res_type == ResourceType::Buffer)
            return ResourceActivationAction::DISCARD_AS_UAV;
          G_ASSERTF(false, "Unhandled resource type! This should never happen!");
          return eastl::nullopt;

        case Usage::VERTEX_BUFFER:
        case Usage::INDEX_BUFFER:
        case Usage::INDIRECTION_BUFFER:
          logerr("Attempted to deduce activation of a discarded buffer used as a "
                 "geometry buffer (vertex/index/indirection)! This is likely an application error!");
          return eastl::nullopt;

        case Usage::VRS_RATE_TEXTURE:
          logerr("Attempted to deduce activation of a discarded texture used as a "
                 "VRS rate texture! This is likely an application error!");
          return eastl::nullopt;

        case Usage::CONSTANT_BUFFER:
          logerr("Attempted to deduce activation of a discarded resource used as a constant buffer! "
                 "This is likely an application error!");
          return ResourceActivationAction::DISCARD_AS_UAV;

        case Usage::UNKNOWN:
          logerr("Attempted to deduce activation of a discarded resource used as an UNKNOWN!"
                 " This may cause driver validation errors! Please, provide a usage.");
          return eastl::nullopt;
      }
      [[fallthrough]];

    case History::ClearZeroOnFirstFrame:
      switch (usage.type)
      {
        case Usage::COPY:
        case Usage::CONSTANT_BUFFER: return ResourceActivationAction::CLEAR_I_AS_UAV;

        case Usage::BLIT:
        case Usage::COLOR_ATTACHMENT:
        case Usage::DEPTH_ATTACHMENT:
        case Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE:
        case Usage::RESOLVE_ATTACHMENT:
        case Usage::INPUT_ATTACHMENT:
        case Usage::VRS_RATE_TEXTURE: return ResourceActivationAction::CLEAR_AS_RTV_DSV;

        case Usage::SHADER_RESOURCE:
          if (usage.access == Access::READ_WRITE)
            return ResourceActivationAction::CLEAR_I_AS_UAV;
          else if (res_type == ResourceType::Texture)
            return ResourceActivationAction::CLEAR_AS_RTV_DSV;
          else if (res_type == ResourceType::Buffer)
            return ResourceActivationAction::CLEAR_I_AS_UAV;
          G_ASSERTF(false, "Unhandled resource type! This should never happen!");
          return eastl::nullopt;

        case Usage::VERTEX_BUFFER:
        case Usage::INDEX_BUFFER:
        case Usage::INDIRECTION_BUFFER:
          logerr("Attempted to deduce activation of a cleared buffer used as a "
                 "geometry buffer (vertex/index/indirection)! This is likely an application error!");
          return ResourceActivationAction::CLEAR_I_AS_UAV;

        case Usage::UNKNOWN:
          logerr("Attempted to deduce activation of a cleared history resource used as an UNKNOWN!"
                 " This may cause driver validation errors! Please, provide a usage.");
          return eastl::nullopt;
      }
  }

  return {};
}

static uint32_t get_creation_flags_from_usage(ResourceUsage usage, ResourceType res_type)
{
  switch (usage.type)
  {
    case Usage::COPY:
    case Usage::BLIT:
      if (usage.access == Access::READ_WRITE)
        return TEXCF_UPDATE_DESTINATION;
      else
        break;
    case Usage::COLOR_ATTACHMENT:
    case Usage::DEPTH_ATTACHMENT:
    case Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE:
    case Usage::RESOLVE_ATTACHMENT:
    case Usage::INPUT_ATTACHMENT: return TEXCF_RTARGET;
    case Usage::SHADER_RESOURCE:
      if (res_type == ResourceType::Texture && usage.access == Access::READ_WRITE)
        return TEXCF_UNORDERED;
      else if (res_type == ResourceType::Texture)
        return TEXCF_RTARGET;
      else if (res_type == ResourceType::Buffer)
        return SBCF_BIND_UNORDERED;
      break;
    case Usage::VERTEX_BUFFER: return SBCF_BIND_VERTEX;
    case Usage::INDEX_BUFFER: return SBCF_BIND_INDEX;
    case Usage::INDIRECTION_BUFFER: return SBCF_MISC_DRAWINDIRECT;
    case Usage::VRS_RATE_TEXTURE: return TEXCF_VARIABLE_RATE;
    case Usage::CONSTANT_BUFFER: return SBCF_BIND_CONSTANT;
    case Usage::UNKNOWN: break;
  }
  return 0;
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type)
{
  auto newFlags = get_creation_flags_from_usage(usage, res_type);
  if (newFlags == TEXCF_UPDATE_DESTINATION && (flags & (TEXCF_RTARGET | TEXCF_UNORDERED)))
    newFlags = 0;

  flags |= newFlags;
}

} // namespace dabfg
