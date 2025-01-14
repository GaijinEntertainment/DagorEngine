// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include "d3d12_utils.h"
#include "driver.h"
#include "format_store.h"
#include "image_global_subresource_id.h"
#include "pipeline.h"
#include "resource_manager/image.h"
#include "stateful_command_buffer.h"
#include "typed_bit_set.h"

#include <dag/dag_vector.h>
#include <debug/dag_log.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_heap.h>
#include <EASTL/bitvector.h>
#include <EASTL/optional.h>
#include <generic/dag_enumerate.h>


namespace dag
{
template <>
struct is_type_init_constructing<D3D12_RESOURCE_BARRIER>
{
  static constexpr bool value = false;
};
} // namespace dag

DAG_DECLARE_RELOCATABLE(D3D12_RESOURCE_BARRIER);

namespace drv3d_dx12
{
// Meta stage, stage values of this indicate that any stage be meant (used for resource activation)
inline constexpr uint32_t STAGE_ANY = ~uint32_t(0);

#if DX12_REPORT_TRANSITION_INFO
#define REPORT debug
#else
#define REPORT(...)
#endif

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
// D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE is not included, depends on device support
static const /*expr*/ D3D12_RESOURCE_STATES D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE =
  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE;
#else
static const /*expr*/ D3D12_RESOURCE_STATES D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE = D3D12_RESOURCE_STATE_COMMON;
#endif

// Only a limited set of state can auto promote textures
inline bool is_texture_auto_promote_state(D3D12_RESOURCE_STATES state)
{
  // Clang complains that the assignment is not a constant expression...
  const /*expr*/ uint32_t auto_promote_mask = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_DEST |
                                              D3D12_RESOURCE_STATE_COPY_SOURCE
#if !_TARGET_XBOXONE
                                              // Spec says nothing special about promote and decay, its a read only state, so we assume
                                              // we can promote and decay to and from it
                                              | D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE
#endif
    ;
  return static_cast<uint32_t>(state) == (static_cast<uint32_t>(state) & auto_promote_mask) &&
         static_cast<uint32_t>(state) != D3D12_RESOURCE_STATE_COMMON;
}
// on systems with auto promote, those are the same, but we need the distinction on systems without auto promote
inline bool is_valid_static_texture_state(D3D12_RESOURCE_STATES state)
{
  // Clang complains that the assignment is not a constant expression...
  const /*expr*/ uint32_t auto_promote_mask = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_DEST |
                                              D3D12_RESOURCE_STATE_COPY_SOURCE
#if !_TARGET_XBOXONE
                                              // Spec says nothing special about promote and decay, its a read only state, so we assume
                                              // we can promote and decay to and from it
                                              | D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE
#endif
    ;
  return static_cast<uint32_t>(state) == (static_cast<uint32_t>(state) & auto_promote_mask);
}
inline bool is_texture_auto_promote_merge_able_state(D3D12_RESOURCE_STATES state)
{
  // Clang complains that the assignment is not a constant expression...
  const /*expr*/ uint32_t auto_promote_mask =
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_COPY_SOURCE;
  return static_cast<uint32_t>(state) == (static_cast<uint32_t>(state) & auto_promote_mask);
}
inline bool has_write_state(D3D12_RESOURCE_STATES state)
{
  const /*expr*/ uint32_t any_write_mask =
    D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_DEPTH_WRITE |
    D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_RESOLVE_DEST |
    D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE | D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE | D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE;
  return 0 != (static_cast<uint32_t>(state) & any_write_mask);
}
inline bool has_read_state(D3D12_RESOURCE_STATES state)
{
  const /*expr*/ uint32_t any_read_mask =
    D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_DEPTH_READ |
    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
    D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
    D3D12_RESOURCE_STATE_PRESENT | D3D12_RESOURCE_STATE_PREDICATION | D3D12_RESOURCE_STATE_VIDEO_DECODE_READ |
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ | D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ
#if !_TARGET_XBOXONE
    | D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE
#endif
    ;
  // common == 0 so needs a extra check
  if (state == D3D12_RESOURCE_STATE_COMMON)
    return true;
  return 0 != (static_cast<uint32_t>(state) & any_read_mask);
}

inline bool has_valid_read_state_combination(D3D12_RESOURCE_STATES state)
{
  if (D3D12_RESOURCE_STATE_DEPTH_READ & state)
  {
    // depth read can not be combined with any read state
    const /*expr*/ uint32_t depth_stencil_read_mask =
      D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
      D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    return 0 == (static_cast<uint32_t>(state) & ~(depth_stencil_read_mask));
  }

  return true;
}

inline bool has_single_write_state(D3D12_RESOURCE_STATES state)
{
  const /*expr*/ D3D12_RESOURCE_STATES any_write_mask =
    D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_DEPTH_WRITE |
    D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_RESOLVE_DEST |
    D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE | D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE | D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE;
  state = state & any_write_mask;

#define CHECK_BIT(bit) \
  if (state & bit)     \
  {                    \
    if (state != bit)  \
    {                  \
      return false;    \
    }                  \
  }
  CHECK_BIT(D3D12_RESOURCE_STATE_RENDER_TARGET);
  CHECK_BIT(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  CHECK_BIT(D3D12_RESOURCE_STATE_DEPTH_WRITE);
  CHECK_BIT(D3D12_RESOURCE_STATE_STREAM_OUT);
  CHECK_BIT(D3D12_RESOURCE_STATE_COPY_DEST);
  CHECK_BIT(D3D12_RESOURCE_STATE_RESOLVE_DEST);
  CHECK_BIT(D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE);
  CHECK_BIT(D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE);
  CHECK_BIT(D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE);
#undef CHECK_BIT
  return true;
}

enum class ResourceStateValidationFailBits
{
  MULTPLE_WRITE_BITS_CONFLICT,
  READ_WRITE_BITS_CONFLICT,
  INVALID_READ_BITS_COMBINATION,
  UAV_ON_INCOMPATIBE_RESOURCE,
  RENDER_TARGET_ON_INCOMPATIBLE_RESOURCE,
  DEPTH_STENCIL_ON_INCOMPATIBLE_RESOURCE,
  SRV_ON_INCOMPATILBE_RESOURCE,

  COUNT
};

using ResourceStateValidationFailState = TypedBitSet<ResourceStateValidationFailBits>;

inline ResourceStateValidationFailState validate_resource_state_elements(D3D12_RESOURCE_STATES state, uint32_t resource_flags,
  bool is_buffer)
{
  ResourceStateValidationFailState errorBits{};

  if (has_write_state(state))
  {
    if (!has_single_write_state(state))
    {
      errorBits.set(ResourceStateValidationFailBits::MULTPLE_WRITE_BITS_CONFLICT);
    }

    if (has_read_state(state))
    {
      // On consoles the initial buffer state is copy src + copy dst. This is valid only on consoles.
      // D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE will represents this, on other platforms this will
      // be D3D12_RESOURCE_STATE_COMMON which will not return true on has_write_state, so no need for
      // ifdef ugliness.
      if (!is_buffer || (D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE != state))
      {
        errorBits.set(ResourceStateValidationFailBits::READ_WRITE_BITS_CONFLICT);
      }
    }
  }

  if (has_read_state(state))
  {
    if (!has_valid_read_state_combination(state))
    {
      errorBits.set(ResourceStateValidationFailBits::INVALID_READ_BITS_COMBINATION);
    }
  }

  // Seems that GDK has some issues reporting UAV access capabilities of resources
#if !_TARGET_XBOX
  if (0 == (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS & resource_flags))
  {
    if (D3D12_RESOURCE_STATE_UNORDERED_ACCESS & state)
    {
      errorBits.set(ResourceStateValidationFailBits::UAV_ON_INCOMPATIBE_RESOURCE);
    }
  }
#endif

  if (0 == (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & resource_flags))
  {
    if (D3D12_RESOURCE_STATE_RENDER_TARGET & state)
    {
      errorBits.set(ResourceStateValidationFailBits::RENDER_TARGET_ON_INCOMPATIBLE_RESOURCE);
    }
  }

  if (0 == (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL & resource_flags))
  {
    if (D3D12_RESOURCE_STATE_DEPTH_WRITE & state)
    {
      errorBits.set(ResourceStateValidationFailBits::DEPTH_STENCIL_ON_INCOMPATIBLE_RESOURCE);
    }
  }

  if (0 != (D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE & resource_flags))
  {
    if ((D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) & state)
    {
      errorBits.set(ResourceStateValidationFailBits::SRV_ON_INCOMPATILBE_RESOURCE);
    }
  }

  return errorBits;
}

inline bool validate_resource_state(D3D12_RESOURCE_STATES state, uint32_t resource_flags, bool is_buffer)
{
  auto errors = validate_resource_state_elements(state, resource_flags, is_buffer);

  if (errors.any())
  {
    if (errors.test(ResourceStateValidationFailBits::MULTPLE_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid resource state, multiple write bits set, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::READ_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid resource state, read and write bits set, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::INVALID_READ_BITS_COMBINATION))
    {
      D3D_ERROR("DX12: Invalid resource state, invalid combination of read bits set, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::UAV_ON_INCOMPATIBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid resource state, UAV state without proper resource flags, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::RENDER_TARGET_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid resource state, RTV state without proper resource flags, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::DEPTH_STENCIL_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid resource state, DSV state without proper resource flags, with 0x%08X", state);
    }
    if (errors.test(ResourceStateValidationFailBits::SRV_ON_INCOMPATILBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid resource state, SRV state without proper resource flags, with 0x%08X", state);
    }
  }

  return errors.none();
}

// Extensive transition barrier validation, validates the following rules:
// - If write state are set, no read states can be set
// - Can not combine multiple write states together
// - Not all read states can be combined together
// - UAV state only if resource supports it
// - RTV state only if resource supports it
// - DSV state only if resource supports it
// - SRV state only if resource supports it
inline bool validate_transition_barrier(const D3D12_RESOURCE_TRANSITION_BARRIER &barrier)
{
  auto desc = barrier.pResource->GetDesc();
  auto beforeErrors =
    validate_resource_state_elements(barrier.StateBefore, desc.Flags, D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension);
  auto afterErrors =
    validate_resource_state_elements(barrier.StateAfter, desc.Flags, D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension);

  bool subResourceIndexIsValid = true;
  if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
  {
    subResourceIndexIsValid = barrier.Subresource == 0;
  }
  else
  {
    if (D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES != barrier.Subresource)
    {
      FormatStore fmt = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ? FormatStore::fromDXGIDepthFormat(desc.Format)
                                                                               : FormatStore::fromDXGIFormat(desc.Format);

      auto mipMaps = MipMapCount::make(desc.MipLevels);
      auto arrayLayers = ArrayLayerCount::make((D3D12_RESOURCE_DIMENSION_TEXTURE3D != desc.Dimension) ? desc.DepthOrArraySize : 1);
      auto totalSubResCount = SubresourcePerFormatPlaneCount::make(mipMaps, arrayLayers) * fmt.getPlanes();
      subResourceIndexIsValid = SubresourceIndex::make(barrier.Subresource) < totalSubResCount;
    }
  }

  if (beforeErrors.any() || afterErrors.any() || (barrier.StateBefore == barrier.StateAfter) || !subResourceIndexIsValid)
  {
    logdbg("DX12: validate_transition_barrier found an error, reporting resource properties:");
    printDesc(desc);

    char resnameBuffer[MAX_OBJECT_NAME_LENGTH];
    get_resource_name(barrier.pResource, resnameBuffer);
    if (!subResourceIndexIsValid)
    {
      D3D_ERROR("DX12: Invalid transition barrier, subresource index %u is out of range, for <%s> "
                "(%s)",
        barrier.Subresource, resnameBuffer, to_string(desc.Dimension));
    }

    if (barrier.StateBefore == barrier.StateAfter)
    {
      D3D_ERROR("DX12: Invalid transition barrier, before and after state are identical, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }

    if (beforeErrors.test(ResourceStateValidationFailBits::MULTPLE_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid transition barrier, multiple write bits set, for <%s> (%s) %u with "
                "StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::READ_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid transition barrier, read and write bits set, for <%s> (%s) %u with "
                "StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::INVALID_READ_BITS_COMBINATION))
    {
      D3D_ERROR("DX12: Invalid transition barrier, invalid combination of read bits set, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::UAV_ON_INCOMPATIBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, UAV state without proper resource flags, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::RENDER_TARGET_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, RTV state without proper resource flags, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::DEPTH_STENCIL_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, DSV state without proper resource flags, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }
    if (beforeErrors.test(ResourceStateValidationFailBits::SRV_ON_INCOMPATILBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, SRV state without proper resource flags, for <%s> "
                "(%s) %u with StateBefore as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateBefore);
    }

    if (afterErrors.test(ResourceStateValidationFailBits::MULTPLE_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid transition barrier, multiple write bits set, for <%s> (%s) %u with "
                "StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::READ_WRITE_BITS_CONFLICT))
    {
      D3D_ERROR("DX12: Invalid transition barrier, read and write bits set, for <%s> (%s) %u with "
                "StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::INVALID_READ_BITS_COMBINATION))
    {
      D3D_ERROR("DX12: Invalid transition barrier, invalid combination of read bits set, for <%s> "
                "(%s) %u with StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::UAV_ON_INCOMPATIBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, UAV state without proper resource flags, for <%s> "
                "(%s) %u with StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::RENDER_TARGET_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, RTV state without proper resource flags, for <%s> "
                "(%s) %u with StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::DEPTH_STENCIL_ON_INCOMPATIBLE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, DSV state without proper resource flags, for <%s> "
                "(%s) %u with StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }
    if (afterErrors.test(ResourceStateValidationFailBits::SRV_ON_INCOMPATILBE_RESOURCE))
    {
      D3D_ERROR("DX12: Invalid transition barrier, SRV state without proper resource flags, for <%s> "
                "(%s) %u with StateAfter as 0x%08X",
        resnameBuffer, to_string(desc.Dimension), barrier.Subresource, barrier.StateAfter);
    }

    return false;
  }

  return true;
}

inline D3D12_RESOURCE_STATES translate_buffer_barrier_to_state(ResourceBarrier barrier)
{
  D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;

  if (RB_NONE != (RB_RW_UAV & barrier))
  {
    result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  if (RB_NONE != (RB_RW_COPY_DEST & barrier))
  {
    result |= D3D12_RESOURCE_STATE_COPY_DEST;
  }

  if (RB_NONE != (RB_RO_SRV & barrier))
  {
    // only pixel shader is special
    if (RB_NONE != (RB_STAGE_PIXEL & barrier))
    {
      result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    if (RB_NONE != ((RB_STAGE_VERTEX | RB_STAGE_COMPUTE | RB_STAGE_RAYTRACE) & barrier))
    {
      result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
  }

  if (RB_NONE != ((RB_RO_CONSTANT_BUFFER | RB_RO_VERTEX_BUFFER) & barrier))
  {
    result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  }

  if (RB_NONE != (RB_RO_INDEX_BUFFER & barrier))
  {
    result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
  }

  if (RB_NONE != (RB_RO_INDIRECT_BUFFER & barrier))
  {
    result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
  }

  if (RB_NONE != (RB_RO_COPY_SOURCE & barrier))
  {
    result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
  }

  if (RB_NONE != (RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE & barrier))
  {
    result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  }

  return result;
}

inline D3D12_RESOURCE_STATES translate_texture_barrier_to_state(ResourceBarrier barrier, bool is_ds)
{
  D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;

  if (RB_NONE != (RB_RW_RENDER_TARGET & barrier))
  {
    if (is_ds)
    {
      if (RB_NONE != (RB_RO_SRV & barrier))
      {
        result |= D3D12_RESOURCE_STATE_DEPTH_READ;
      }
      else
      {
        result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
      }
    }
    else
    {
      result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
  }

  if (RB_NONE != (RB_RW_UAV & barrier))
  {
    result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  if (RB_NONE != (RB_RW_COPY_DEST & barrier))
  {
    result |= D3D12_RESOURCE_STATE_COPY_DEST;
  }

  if (RB_NONE != (RB_RW_BLIT_DEST & barrier))
  {
    result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
  }

  if (RB_NONE != (RB_RO_SRV & barrier))
  {
    // only pixel shader is special
    if (RB_NONE != (RB_STAGE_PIXEL & barrier))
    {
      result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    if (RB_NONE != ((RB_STAGE_COMPUTE | RB_STAGE_RAYTRACE | RB_STAGE_VERTEX) & barrier))
    {
      result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
#if _TARGET_XBOX
    // When using depth stencil for sampling we _need_ to set the read bit or we may hang the GPU
    // This behavior was introduced in barrier rework of GDK 201101
    if (is_ds)
    {
      result |= D3D12_RESOURCE_STATE_DEPTH_READ;
    }
#endif
  }

  if (RB_NONE != (RB_RO_COPY_SOURCE & barrier))
  {
    result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
  }

  if (RB_NONE != (RB_RO_VARIABLE_RATE_SHADING_TEXTURE & barrier))
  {
#if _TARGET_XBOXONE
    DAG_FATAL("DX12: RB_RO_VARIABLE_RATE_SHADING_TEXTURE on XBOX ONE used!");
#else
    result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
#endif
  }

  if (RB_NONE != (RB_RO_BLIT_SOURCE & barrier))
  {
    result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  }

  return result;
}
inline char *make_resource_barrier_string_from_state(char *str, size_t len, D3D12_RESOURCE_STATES state, ResourceBarrier barrier_mask)
{
  // Quick append that inserts " | " should ofs not be 0, only appends src if
  // enough room is left for it plus a terminating null.
  // Does not append a null!
  auto quickAppened = [](auto buf, auto len, auto ofs, auto &src) //
  {
    if (ofs)
    {
      if ((len - ofs - 1) < 3)
        return ofs;

      buf[ofs++] = ' ';
      buf[ofs++] = '|';
      buf[ofs++] = ' ';
    }
    auto ln = countof(src);
    auto space = ln - 1;
    if ((len - ofs - 1) < space)
      return ofs;

    eastl::copy(src, src + space, buf + ofs);
    return ofs + space;
  };
  size_t strOffset = 0;

  if (D3D12_RESOURCE_STATE_UNORDERED_ACCESS == state)
  {
    strOffset = quickAppened(str, len, strOffset, "RB_RW_UAV");
  }
  else if (D3D12_RESOURCE_STATE_COPY_DEST == state)
  {
    strOffset = quickAppened(str, len, strOffset, "RB_RW_COPY_DEST");
  }
  else if (D3D12_RESOURCE_STATE_RENDER_TARGET == state)
  {
    if (RB_RW_BLIT_DEST & barrier_mask)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RW_BLIT_DEST");
    }
    else
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RW_RENDER_TARGET");
    }
  }
  else if (D3D12_RESOURCE_STATE_DEPTH_WRITE == state)
  {
    strOffset = quickAppened(str, len, strOffset, "RB_RW_DEPTH_STENCIL_TARGET");
  }
  else
  {
    if (D3D12_RESOURCE_STATE_DEPTH_READ & state)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RO_CONSTANT_DEPTH_STENCIL_TARGET");
    }

    if ((D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) & state)
    {
      if (((barrier_mask & RB_STAGE_ALL_SHADERS) != RB_NONE) || (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE & state))
      {
        strOffset = quickAppened(str, len, strOffset, "RB_RO_SRV");
      }
      if (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE & state)
      {
        strOffset = quickAppened(str, len, strOffset, "RB_STAGE_PIXEL");
      }
      if (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE & state)
      {
        if ((barrier_mask & (RB_STAGE_ALL_SHADERS | RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE)) != RB_NONE)
        {
          if (ResourceBarrier::RB_STAGE_VERTEX & barrier_mask)
          {
            strOffset = quickAppened(str, len, strOffset, "RB_STAGE_VERTEX");
          }
          if (ResourceBarrier::RB_STAGE_COMPUTE & barrier_mask)
          {
            strOffset = quickAppened(str, len, strOffset, "RB_STAGE_COMPUTE");
          }
          if (ResourceBarrier::RB_STAGE_RAYTRACE & barrier_mask)
          {
            strOffset = quickAppened(str, len, strOffset, "RB_STAGE_RAYTRACE");
          }
          if (RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE & barrier_mask)
          {
            strOffset = quickAppened(str, len, strOffset, "RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE");
          }
        }
        else
        {
          // In some places there is no enough information at hand to decide which stage we are needing
          strOffset = quickAppened(str, len, strOffset, "(RB_STAGE_VERTEX AND/OR RB_STAGE_COMPUTE)");
        }
      }
    }
    if (D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER & state)
    {
      if (barrier_mask != RB_NONE)
      {
        strOffset = quickAppened(str, len, strOffset, "RB_RO_CONSTANT_BUFFER");
        if (ResourceBarrier::RB_STAGE_VERTEX & barrier_mask)
        {
          strOffset = quickAppened(str, len, strOffset, "RB_STAGE_VERTEX");
        }
        if (ResourceBarrier::RB_STAGE_PIXEL & barrier_mask)
        {
          strOffset = quickAppened(str, len, strOffset, "RB_STAGE_PIXEL");
        }
        if (ResourceBarrier::RB_STAGE_COMPUTE & barrier_mask)
        {
          strOffset = quickAppened(str, len, strOffset, "RB_STAGE_COMPUTE");
        }
        if (ResourceBarrier::RB_STAGE_RAYTRACE & barrier_mask)
        {
          strOffset = quickAppened(str, len, strOffset, "RB_STAGE_RAYTRACE");
        }
      }
      else
      {
        strOffset = quickAppened(str, len, strOffset, "RB_RO_VERTEX_BUFFER");
      }
    }
    if (D3D12_RESOURCE_STATE_INDEX_BUFFER & state)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RO_INDEX_BUFFER");
    }
    if (D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT & state)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RO_INDIRECT_BUFFER");
    }
    if (D3D12_RESOURCE_STATE_COPY_SOURCE & state)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RO_COPY_SOURCE");
    }
#if !_TARGET_XBOXONE
    if (D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE & state)
    {
      strOffset = quickAppened(str, len, strOffset, "RB_RO_VARIABLE_RATE_SHADING_TEXTURE");
    }
#endif
  }
  str[strOffset] = '\0';
  return str;
}

namespace detail
{
template <uint32_t Mask, uint32_t Index = 31, bool BitSet = Mask & (1u << Index)>
class TopMostSetBitIndexFinder
{
public:
  static constexpr uint32_t value = Index;
};

template <uint32_t Mask, uint32_t Index>
class TopMostSetBitIndexFinder<Mask, Index, false>
{
public:
  static constexpr uint32_t value = TopMostSetBitIndexFinder<Mask, Index - 1>::value;
};

// Error, no set bits
template <uint32_t Index>
class TopMostSetBitIndexFinder<0, Index, false>;

template <uint32_t Mask, uint32_t Index = 31>
class CountBits
{
public:
  static constexpr uint32_t value = ((Mask >> Index) & 1) + CountBits<Mask, Index - 1>::value;
};

template <uint32_t Mask>
class CountBits<Mask, 0>
{
public:
  static constexpr uint32_t value = Mask & 1;
};
} // namespace detail

// Provides the extra state bits and interfaces for it
template <uint32_t DefaultState, uint32_t StatusBits>
class ResourceStateBaseBitsProvider
{
  // Bit not used yet by D3D12_RESOURCE_STATES
  static constexpr uint32_t unused_bit_31 = 1u << 31;
  // GDK uses some extra bits to allow control over de/compression of compressed formats.
  // To have them easily available for later expressions we have a copy of the masks here.
  static constexpr uint32_t gdk_preserve_compressed_color = 0x10000000;
  static constexpr uint32_t gdk_preserve_compressed_depth = 0x10000000;
  static constexpr uint32_t gdk_preserve_compreesed_stencil = 0x20000000;
  static constexpr uint32_t gdk_preserve_indirect_color_clear = 0x40000000;
  static constexpr uint32_t gdk_preserve_scattered_color_fmask = 0x02000000;
  static constexpr uint32_t gdk_preserve_dcc = 0x20000000;
  static constexpr uint32_t gdk_preserve_expanded_color = 0x04000000;
  static constexpr uint32_t gdk_preserve_expanded_depth = 0x04000000;
  static constexpr uint32_t gdk_preserve_expanded_stencil = 0x08000000;

protected:
  // Make all bits available for states we don't use (yet)
  static constexpr uint32_t AvailableBits =
    D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_VIDEO_DECODE_READ | D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE |
    D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ | D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE | D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ |
    D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE | unused_bit_31 | gdk_preserve_compressed_color | gdk_preserve_compressed_depth |
    gdk_preserve_compreesed_stencil | gdk_preserve_indirect_color_clear | gdk_preserve_scattered_color_fmask | gdk_preserve_dcc |
    gdk_preserve_expanded_color | gdk_preserve_expanded_depth | gdk_preserve_expanded_stencil;

  static constexpr uint32_t StateMask = ~AvailableBits;
  uint32_t value = DefaultState;

  void setValue(uint32_t v) { value = v; }

  uint32_t getValue() const { return value; }

public:
  D3D12_RESOURCE_STATES get() const
  {
    // Sanity check to ensure that we can provide the needed bits, it will also fail to allocate
    // bits down the line as TopMostSetBitIndexFinder will be used on a mask of 0.
    G_STATIC_ASSERT(detail::CountBits<AvailableBits>::value >= StatusBits);
    return static_cast<D3D12_RESOURCE_STATES>(value & StateMask);
  }

  // Returns true if the given value has any bit set that is a user bit.
  // This should be used to detect if a input uses bits we may use for internal state tracking.
  // If this is fired someone forgot something to update.
  static bool user_bit_check(uint32_t value) { return 0 == (value & AvailableBits); }

  // Will retain user bits
  void set(D3D12_RESOURCE_STATES new_value)
  {
    G_ASSERT(user_bit_check(new_value));
    value = new_value | (value & AvailableBits);
  }
  // Does not retain user bits
  void reset(D3D12_RESOURCE_STATES new_value) { value = new_value; }
  // Will reset the state value and all masked user bits, all unmasked bits will be keept.
  void resetMasked(D3D12_RESOURCE_STATES new_value, uint32_t mask) { value = new_value | (value & (AvailableBits & ~mask)); }
  // Will reset the state value, keeps all unmasked user bits and sets the bits of mask if set_bits is set, otherwise those bits will
  // not be set This is the same as a chain of: resetMasked(value, bit_mask); if (set_bits) addBits(bit_mask);
  void resetConditionalBits(D3D12_RESOURCE_STATES new_value, bool set_bits, uint32_t mask)
  {
    value = new_value | (value & (AvailableBits & ~mask)) | (set_bits ? (AvailableBits & mask) : 0);
  }
};

// Optimized implementation for 0 used state bits
template <uint32_t DefaultState>
class ResourceStateBaseBitsProvider<DefaultState, 0>
{
protected:
  uint32_t value = DefaultState;

  void setValue(uint32_t v) { value = v; }

  uint32_t getValue() const { return value; }

public:
  D3D12_RESOURCE_STATES get() const { return static_cast<D3D12_RESOURCE_STATES>(value); }
  // Will retain user bits
  void set(D3D12_RESOURCE_STATES new_value) { value = new_value; }
  // Does not retain user bits
  void reset(D3D12_RESOURCE_STATES new_value) { value = new_value; }
  // Will reset the state value and all masked user bits, all unmasked bits will be keept.
  void resetMasked(D3D12_RESOURCE_STATES new_value, uint32_t) { value = new_value; }
  // Will reset the state value, keeps all unmasked user bits and sets the bits of mask if set_bits is set, otherwise those bits will
  // not be set This is the same as a chain of: resetMasked(value, bit_mask); if (set_bits) addBits(bit_mask);
  void resetConditionalBits(D3D12_RESOURCE_STATES new_value, bool, uint32_t) { value = new_value; }
};

// Common base utility
template <uint32_t DefaultState, uint32_t StatusBits>
class ResourceStateBaseRoot : public ResourceStateBaseBitsProvider<DefaultState, StatusBits>
{
  using BaseType = ResourceStateBaseBitsProvider<DefaultState, StatusBits>;

protected:
  // For whatever reason VC16 fails at basic class hierarchy concepts when a base has a specialization...
  using BaseType::getValue;
  using BaseType::setValue;

public:
  bool allMatch(uint32_t mask) const { return mask == (getValue() & mask); }

  bool anyMatch(uint32_t mask) const { return 0 != (getValue() & mask); }

  void addBits(uint32_t mask) { setValue(getValue() | mask); }

  void subBits(uint32_t mask) { setValue(getValue() & ~mask); }

  void flipBits(uint32_t mask) { setValue(getValue() ^ mask); }
};

#if DX12_USE_AUTO_PROMOTE_AND_DECAY
template <uint32_t DefaultState>
class ResourceStateBase : public ResourceStateBaseRoot<DefaultState, 1>
{
  using BaseType = ResourceStateBaseRoot<DefaultState, 1>;

protected:
  static constexpr uint32_t AutoPromoteDisableBitIndex = detail::TopMostSetBitIndexFinder<BaseType::AvailableBits>::value;
  static constexpr uint32_t AutoPromoteDisableBit = 1u << AutoPromoteDisableBitIndex;
  static constexpr uint32_t AvailableBits = BaseType::AvailableBits ^ AutoPromoteDisableBit;

public:
  // For whatever reason VC16 fails at basic class hierarchy concepts when a base has a specialization...
  using BaseType::addBits;
  using BaseType::allMatch;
  using BaseType::anyMatch;
  using BaseType::flipBits;
  using BaseType::get;
  using BaseType::reset;
  using BaseType::resetConditionalBits;
  using BaseType::resetMasked;
  using BaseType::set;
  using BaseType::subBits;

  operator D3D12_RESOURCE_STATES() const { return get(); }

  void autoPromote(D3D12_RESOURCE_STATES state) { addBits(state); }

  bool canMerge(D3D12_RESOURCE_STATES state) const
  {
    // we can never merge transition to common
    return (D3D12_RESOURCE_STATE_COMMON != state) && (!has_write_state(state) && !has_write_state(get()));
  }

  void transition(D3D12_RESOURCE_STATES state)
  {
    // transitioning to common will enable auto promote again and any other transition will turn it off.
    resetConditionalBits(state, D3D12_RESOURCE_STATE_COMMON != state, AutoPromoteDisableBit);
  }

  void initialize(D3D12_RESOURCE_STATES state) { transition(state); }

  D3D12_RESOURCE_STATES makeMergedState(D3D12_RESOURCE_STATES state) const { return state | get(); }

  bool needsTransition(D3D12_RESOURCE_STATES state) const
  {
    return (state != get()) && ((state != (state & get())) || (D3D12_RESOURCE_STATE_COMMON == state));
  }

  bool isAutoPromoteDisabled() const { return anyMatch(AutoPromoteDisableBit); }
};

// Implements auto promote and decay rules for buffer states
class BufferResourceState : public ResourceStateBase<D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE>
{
public:
  bool canAutoPromote(D3D12_RESOURCE_STATES state) const
  {
    // common can always auto promote to anything
    if (D3D12_RESOURCE_STATE_COMMON == get())
    {
      return true;
    }
    if (isAutoPromoteDisabled())
    {
      return false;
    }
    // read to write or write to write can not be auto promoted
    if (has_write_state(get()))
    {
      return false;
    }
    // We can't auto promote to a common state from non common state
    if (D3D12_RESOURCE_STATE_COMMON == state)
    {
      return false;
    }
    // read to read is okay, but write to read is not
    return !has_write_state(state);
  }

  void decay() { reset(D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE); }
};

// Implements auto promote and decay rules for texture states
class TextureResourceState : public ResourceStateBase<D3D12_RESOURCE_STATE_COMMON>
{
public:
  bool canAutoPromote(D3D12_RESOURCE_STATES state) const
  {
    if (!is_texture_auto_promote_state(state))
    {
      return false;
    }
    // if current state is common, any auto promote state is valid
    if (D3D12_RESOURCE_STATE_COMMON == get())
    {
      return true;
    }
    if (isAutoPromoteDisabled())
    {
      return false;
    }
    // can not promote from write state to another write or read state
    if (has_write_state(get()))
    {
      return false;
    }
    // only valid way to promote to a write state is from common
    if (has_write_state(state))
    {
      return false;
    }
    if (is_texture_auto_promote_merge_able_state(get()))
    {
      return true;
    }
    return false;
  }

  void decay()
  {
    if (!isAutoPromoteDisabled())
    {
      reset(D3D12_RESOURCE_STATE_COMMON);
    }
  }
};
#else
template <uint32_t DefaultState>
class ResourceStateBase : public ResourceStateBaseRoot<DefaultState, 0>
{
  using BaseType = ResourceStateBaseRoot<DefaultState, 0>;

public:
  // For whatever reason VC16 fails at basic class hierarchy concepts when a base has a specialization...
  using BaseType::addBits;
  using BaseType::allMatch;
  using BaseType::anyMatch;
  using BaseType::flipBits;
  using BaseType::get;
  using BaseType::reset;
  using BaseType::resetConditionalBits;
  using BaseType::resetMasked;
  using BaseType::set;
  using BaseType::subBits;

  operator D3D12_RESOURCE_STATES() const { return get(); }

  void autoPromote(D3D12_RESOURCE_STATES) {}

  constexpr bool canAutoPromote(D3D12_RESOURCE_STATES) const { return false; }

  bool canMerge(D3D12_RESOURCE_STATES state) const
  {
    // we can never merge transition to common
    return (D3D12_RESOURCE_STATE_COMMON != state) && (!has_write_state(state) && !has_write_state(get()));
  }

  void transition(D3D12_RESOURCE_STATES state) { reset(state); }

  void initialize(D3D12_RESOURCE_STATES state) { transition(state); }

  D3D12_RESOURCE_STATES makeMergedState(D3D12_RESOURCE_STATES state) const { return state | get(); }

  constexpr bool isAutoPromoteDisabled() const { return true; }

  bool needsTransition(D3D12_RESOURCE_STATES state) const
  {
    return (state != get()) && ((state != (state & get())) || (D3D12_RESOURCE_STATE_COMMON == state));
  }

  void decay() {}
};

// No auto promote and decay so we can directly alias it
using BufferResourceState = ResourceStateBase<D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE>;
// No auto promote and decay so we can directly alias it
using TextureResourceState = ResourceStateBase<D3D12_RESOURCE_STATE_COMMON>;
#endif

template <typename T, typename I>
class ResourceStateSetBase
{
protected:
  dag::Vector<T> dataSet;

public:
  T &operator[](I id)
  {
    if (id.index() >= dataSet.size())
    {
      dataSet.resize(id.index() + 1);
    }
    return dataSet.data()[id.index()]; // it is safe due to check above
  }

  void initialize(I base, uint32_t count, D3D12_RESOURCE_STATES state)
  {
    auto rangeEnd = base + count;
    if (rangeEnd.index() >= dataSet.size())
    {
      dataSet.resize(rangeEnd.index());
    }

    for (auto at = begin(dataSet) + base.index(), ed = begin(dataSet) + rangeEnd.index(); at != ed; ++at)
    {
      at->initialize(state);
    }
  }

  void decay()
  {
    for (auto &s : dataSet)
    {
      s.decay();
    }
  }

  void import(const ResourceStateSetBase &other)
  {
    dataSet.resize(other.dataSet.size());
    eastl::copy(begin(other.dataSet), end(other.dataSet), begin(dataSet));
  }
};

using BufferResourceStateSet = ResourceStateSetBase<BufferResourceState, BufferGlobalId>;
using TextureResourceStateSet = ResourceStateSetBase<TextureResourceState, ImageGlobalSubresourceId>;

class InitialTextureResourceStateSet
{
  TextureResourceStateSet dataSet;

public:
  void update(const TextureResourceStateSet &from)
  {
    dataSet.import(from);
    dataSet.decay();
  }

  void initialize(ImageGlobalSubresourceId base, uint32_t count, D3D12_RESOURCE_STATES state)
  {
    dataSet.initialize(base, count, state);
  }

  D3D12_RESOURCE_STATES getState(ImageGlobalSubresourceId base, SubresourceIndex sub) { return dataSet[base + sub]; }
};

class InititalResourceStateSet : public InitialTextureResourceStateSet
{};

inline void print_barrier(const D3D12_RESOURCE_TRANSITION_BARRIER &transition)
{
  logdbg(".Transition.pResource = 0x%p", transition.pResource);
  logdbg(".Transition.Subresource = %u", transition.Subresource);
  logdbg(".Transition.StateBefore = 0x%X", (uint32_t)transition.StateBefore);
  logdbg(".Transition.StateAfter = 0x%X", (uint32_t)transition.StateAfter);
}

inline void print_barrier(const D3D12_RESOURCE_ALIASING_BARRIER &aliasing)
{
  logdbg(".Aliasing.pResourceBefore = 0x%p", aliasing.pResourceBefore);
  logdbg(".Aliasing.pResourceAfter = 0x%p", aliasing.pResourceAfter);
}

inline void print_barrier(const D3D12_RESOURCE_UAV_BARRIER &uav) { logdbg(".UAV.pResource = 0x%p", uav.pResource); }

inline void print_barrier(const D3D12_RESOURCE_BARRIER &barrier)
{
  logdbg(".Type = %u", (uint32_t)barrier.Type);
  logdbg(".Flags = 0x%X", (uint32_t)barrier.Flags);
  switch (barrier.Type)
  {
    case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION: print_barrier(barrier.Transition); break;
    case D3D12_RESOURCE_BARRIER_TYPE_ALIASING: print_barrier(barrier.Aliasing); break;
    case D3D12_RESOURCE_BARRIER_TYPE_UAV: print_barrier(barrier.UAV); break;
  }
}

class BarrierBatcher
{
  dag::Vector<D3D12_RESOURCE_BARRIER> dataSet;

public:
  void purgeAll() { dataSet.clear(); }

  size_t batchSize() const { return dataSet.size(); }

  void beginTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    D3D12_RESOURCE_BARRIER newBarrier;
    newBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    newBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;
    newBarrier.Transition.pResource = res;
    newBarrier.Transition.Subresource = sub_res.index();
    newBarrier.Transition.StateBefore = from;
    newBarrier.Transition.StateAfter = to;
    G_ASSERT(validate_transition_barrier(newBarrier.Transition));
    dataSet.push_back(newBarrier);
  }

  void endTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    D3D12_RESOURCE_BARRIER newBarrier;
    newBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    newBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
    newBarrier.Transition.pResource = res;
    newBarrier.Transition.Subresource = sub_res.index();
    newBarrier.Transition.StateBefore = from;
    newBarrier.Transition.StateAfter = to;
    G_ASSERT(validate_transition_barrier(newBarrier.Transition));
    dataSet.push_back(newBarrier);
  }

  void transition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    D3D12_RESOURCE_BARRIER newBarrier;
    newBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    newBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    newBarrier.Transition.pResource = res;
    newBarrier.Transition.Subresource = sub_res.index();
    newBarrier.Transition.StateBefore = from;
    newBarrier.Transition.StateAfter = to;
    G_ASSERT(validate_transition_barrier(newBarrier.Transition));
    dataSet.push_back(newBarrier);
  }

  bool tryEraseTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    auto ref = eastl::find_if(begin(dataSet), end(dataSet),
      [res, sub_res, from, to](const auto &barrier) //
      {
        if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION != barrier.Type)
        {
          return false;
        }
        if (res != barrier.Transition.pResource)
        {
          return false;
        }
        if (sub_res.index() != barrier.Transition.Subresource)
        {
          return false;
        }
        return from == barrier.Transition.StateBefore && to == barrier.Transition.StateAfter;
      });
    if (end(dataSet) == ref)
    {
      return false;
    }

    dataSet.erase(ref);
    return true;
  }

  bool tryUpdateTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    auto ref = eastl::find_if(rbegin(dataSet), rend(dataSet),
      [res, sub_res](const auto &barrier) //
      {
        if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION != barrier.Type)
        {
          return false;
        }
        return res == barrier.Transition.pResource && sub_res.index() == barrier.Transition.Subresource;
      });

    if (rend(dataSet) == ref)
    {
      return false;
    }

    if (DAGOR_UNLIKELY(ref->Transition.StateAfter != from))
    {
      [&] {
        for (auto [i, b] : enumerate(dataSet))
        {
          // Indicator to quickly identify the barrier in question and related barriers
          const char *postFix = "";
          if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION == b.Type)
          {
            if (&b == &*ref)
            {
              postFix = "!!";
            }
            else if (b.Transition.pResource == res)
            {
              if (b.Transition.Subresource == sub_res.index())
              {
                postFix = "++";
              }
              else
              {
                postFix = "--";
              }
            }
          }
          logdbg("Barrier[%u]:%s", i, postFix);
          print_barrier(b);
        }
        G_ASSERTF(ref->Transition.StateAfter == from, "Unexpected barrier found, see listing in debug log for more detail!");
      }();
      return false;
    }

    // patching may result in a barrier where before and after end up begin identical, in this case
    // we have to delete it
    if (ref->Transition.StateBefore == to)
    {
      dataSet.erase(ref);
      return true;
    }

    ref->Transition.StateAfter = to;
    G_ASSERT(validate_transition_barrier(ref->Transition));
    return true;
  }

  bool tryFixMissingEndTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES to)
  {
    auto ref = eastl::find_if(begin(dataSet), end(dataSet),
      [res, sub_res](const auto &barrier) //
      {
        if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION != barrier.Type)
        {
          return false;
        }
        if (D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY != barrier.Flags)
        {
          return false;
        }
        return res == barrier.Transition.pResource && sub_res.index() == barrier.Transition.Subresource;
      });
    if (end(dataSet) == ref)
    {
      return false;
    }

    ref->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    ref->Transition.StateAfter = to;
    return true;
  }

  bool tryFuseBeginEndTransition(ID3D12Resource *res, SubresourceIndex sub_res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
  {
    auto ref = eastl::find_if(begin(dataSet), end(dataSet),
      [res, sub_res](const auto &barrier) //
      {
        if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION != barrier.Type)
        {
          return false;
        }
        if (D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY != barrier.Flags)
        {
          return false;
        }
        return res == barrier.Transition.pResource && sub_res.index() == barrier.Transition.Subresource;
      });
    if (end(dataSet) == ref)
    {
      return false;
    }

    ref->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    G_UNUSED(from);
    G_UNUSED(to);
    G_ASSERT(ref->Transition.StateBefore == from);
    G_ASSERT(ref->Transition.StateAfter == to);
    return true;
  }

  bool hasTranistionFor(ID3D12Resource *res, SubresourceIndex sub_res)
  {
    auto ref = eastl::find_if(begin(dataSet), end(dataSet),
      [res, sub_res](const auto &barrier) //
      {
        if (D3D12_RESOURCE_BARRIER_TYPE_TRANSITION != barrier.Type)
        {
          return false;
        }
        return res == barrier.Transition.pResource && sub_res.index() == barrier.Transition.Subresource;
      });

    return end(dataSet) != ref;
  }

  void flushUAV(ID3D12Resource *res)
  {
    D3D12_RESOURCE_BARRIER newBarrier;
    newBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    newBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    newBarrier.UAV.pResource = res;
    dataSet.push_back(newBarrier);
  }

  void flushAllUAV() { flushUAV(nullptr); }

  void flushAlias(ID3D12Resource *from, ID3D12Resource *to)
  {
    D3D12_RESOURCE_BARRIER newBarrier;
    newBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
    newBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    newBarrier.Aliasing.pResourceBefore = from;
    newBarrier.Aliasing.pResourceAfter = to;
    dataSet.push_back(newBarrier);
  }

  void flushAliasAll() { flushAlias(nullptr, nullptr); }

  bool hasBarriers() const { return !dataSet.empty(); }

  template <typename T>
  void execute(T &cmd)
  {
    if (hasBarriers())
    {
      cmd.resourceBarrier(static_cast<UINT>(dataSet.size()), dataSet.data());
      dataSet.clear();
    }
  }
};

#if DX12_ALLOW_SPLIT_BARRIERS
class SplitTransitionTracker
{
  dag::Vector<eastl::pair<ImageGlobalSubresourceId, D3D12_RESOURCE_STATES>> dataSet;

public:
  bool beginTransition(ImageGlobalSubresourceId global_id, D3D12_RESOURCE_STATES state)
  {
    eastl::pair value(global_id, state);
    auto ref = eastl::lower_bound(begin(dataSet), end(dataSet), value,
      [](auto l, auto r) //
      { return l.first < r.first; });
    dataSet.insert(ref, value);
    return true;
  }

  eastl::optional<D3D12_RESOURCE_STATES> checkPendingTransition(ImageGlobalSubresourceId global_id) const
  {
    auto ref = eastl::lower_bound(begin(dataSet), end(dataSet), eastl::pair(global_id, D3D12_RESOURCE_STATE_COMMON),
      [](auto l, auto r) //
      { return l.first < r.first; });
    if (end(dataSet) == ref)
    {
      return eastl::nullopt;
    }
    if (ref->first != global_id)
    {
      return eastl::nullopt;
    }
    return ref->second;
  }

  eastl::optional<D3D12_RESOURCE_STATES> endTransition(ImageGlobalSubresourceId global_id)
  {
    auto ref = eastl::lower_bound(begin(dataSet), end(dataSet), eastl::pair(global_id, D3D12_RESOURCE_STATE_COMMON),
      [](auto l, auto r) //
      { return l.first < r.first; });
    if (end(dataSet) == ref)
    {
      return eastl::nullopt;
    }
    if (ref->first != global_id)
    {
      return eastl::nullopt;
    }
    auto value = ref->second;
    dataSet.erase(ref);
    return value;
  }

  eastl::optional<eastl::pair<ImageGlobalSubresourceId, D3D12_RESOURCE_STATES>> endTransitionInRange(
    ImageGlobalSubresourceId global_id, uint32_t count)
  {
    auto ref = eastl::lower_bound(begin(dataSet), end(dataSet), eastl::pair(global_id, D3D12_RESOURCE_STATE_COMMON),
      [](auto l, auto r) //
      { return l.first < r.first; });

    if (end(dataSet) == ref)
    {
      return eastl::nullopt;
    }

    if (ref->first >= global_id + count)
    {
      return eastl::nullopt;
    }

    auto value = *ref;
    dataSet.erase(ref);
    return value;
  }
};
#else
class SplitTransitionTracker
{
public:
  constexpr bool beginTransition(ImageGlobalSubresourceId, D3D12_RESOURCE_STATES) const { return false; }

  eastl::optional<D3D12_RESOURCE_STATES> checkPendingTransition(ImageGlobalSubresourceId) const { return eastl::nullopt; }

  eastl::optional<D3D12_RESOURCE_STATES> endTransition(ImageGlobalSubresourceId) { return eastl::nullopt; }

  eastl::optional<eastl::pair<ImageGlobalSubresourceId, D3D12_RESOURCE_STATES>> endTransitionInRange(ImageGlobalSubresourceId,
    uint32_t)
  {
    return eastl::nullopt;
  }
};
#endif

class UnorderedAccessTracker
{
  // list of resources that will be used after the next flush as uav resource
  // should one be in the incoming list and outgoing list then it needs a uav barrier
  dag::Vector<ID3D12Resource *> incomingUAVResources;
  // list of resources that where previously accessed as uav resource and need
  // a uav barrier before they can be accessed again as uav resource to avoid race over its data
  dag::Vector<ID3D12Resource *> outgoingUAVResources;
  dag::Vector<ID3D12Resource *> userUavResources;
  dag::Vector<ID3D12Resource *> userSkippedUavSync;

public:
  bool flushAccess(ID3D12Resource *res)
  {
    auto ref = eastl::lower_bound(begin(outgoingUAVResources), end(outgoingUAVResources), res);
    if (ref == end(outgoingUAVResources))
    {
      return false;
    }
    if (*ref != res)
    {
      return false;
    }
    outgoingUAVResources.erase(ref);
    return true;
  }

  void ignoreNextDependency(ID3D12Resource *res)
  {
    G_ASSERTF(res != nullptr, "DX12: Can not turn off null resource uav flush");
    auto ref = eastl::find(begin(userSkippedUavSync), end(userSkippedUavSync), res);
    if (ref == end(userSkippedUavSync))
    {
      userSkippedUavSync.push_back(res);
    }
  }

  void flushAccessToAll()
  {
    userUavResources.clear();
    userUavResources.push_back(nullptr);
    incomingUAVResources.clear();
    outgoingUAVResources.clear();
  }

  bool explicitAccess(ID3D12Resource *res)
  {
    auto ref = eastl::find(begin(userUavResources), end(userUavResources), res);
    if (ref == end(userUavResources))
    {
      userUavResources.push_back(res);

      auto autoEnd = eastl::lower_bound(begin(incomingUAVResources), end(incomingUAVResources), res);
      if (autoEnd != end(incomingUAVResources) && *autoEnd == res)
        incomingUAVResources.erase(autoEnd);
      autoEnd = eastl::lower_bound(begin(outgoingUAVResources), end(outgoingUAVResources), res);
      if (autoEnd != end(outgoingUAVResources) && *autoEnd == res)
        outgoingUAVResources.erase(autoEnd);
      return true;
    }
    return false;
  }

  bool beginImplicitAccess(ID3D12Resource *res)
  {
    auto ref = eastl::lower_bound(begin(incomingUAVResources), end(incomingUAVResources), res);
    if (ref == end(incomingUAVResources) || *ref != res)
    {
      incomingUAVResources.insert(ref, res);
      return true;
    }
    return false;
  }

  // when a command buffer ends and it is the last one of a batch, then all outstanding uav access
  // will be flushed automatically
  void implicitFlushAll(const char *where, bool report_user_barriers)
  {
#if DX12_VALIDATE_USER_BARRIERS
    if (report_user_barriers)
    {
      char cbuf[MAX_OBJECT_NAME_LENGTH];
      for (auto &&res : userUavResources)
      {
        logwarn("DX12: Dropping UAV barrier for resource %s - %p, during %s", get_resource_name(res, cbuf), res, where);
      }
    }
#else
    G_UNUSED(where);
    G_UNUSED(report_user_barriers);
#endif
    userUavResources.clear();
    incomingUAVResources.clear();
    outgoingUAVResources.clear();
  }

  template <typename T>
  void iterateAccessToFlush(const char *where, bool report_user_barriers, T clb)
  {
    for (auto &&res : userUavResources)
    {
      clb(res);
    }
    userUavResources.clear();
    auto inPos = begin(incomingUAVResources);
    auto inEnd = end(incomingUAVResources);
    auto outPos = begin(outgoingUAVResources);
    auto outEnd = end(outgoingUAVResources);
    for (; inPos != inEnd && outPos != outEnd; ++inPos)
    {
      if (*inPos != *outPos)
      {
        outPos = eastl::lower_bound(outPos, end(outgoingUAVResources), *inPos);
        if (outPos == end(outgoingUAVResources))
          break;
      }
      if (*inPos == *outPos)
      {
        if (end(userSkippedUavSync) == eastl::find(begin(userSkippedUavSync), end(userSkippedUavSync), *inPos))
        {
#if DX12_VALIDATE_USER_BARRIERS
          if (report_user_barriers)
          {
            char cbuf[MAX_OBJECT_NAME_LENGTH];
            D3D_ERROR("DX12: Missing RB_FLUSH_UAV barrier for resource %s - %p, should "
                      "this be on purpose, then add a RB_NONE barrier to silence this message, "
                      "required during %s",
              get_resource_name(*inPos, cbuf), *inPos, where);
          }
#else
          G_UNUSED(where);
          G_UNUSED(report_user_barriers);
#endif
          clb(*inPos);
        }
        ++outPos;
      }
      else
      {
        outPos = outgoingUAVResources.insert(outPos, *inPos) + 1;
        outEnd = end(outgoingUAVResources);
      }
    }
    outgoingUAVResources.insert(end(outgoingUAVResources), inPos, inEnd);
    incomingUAVResources.clear();
    userSkippedUavSync.clear();
  }
};

// This class is responsible for tracking the state of all texture and buffer resources and
// batch up barriers that are flushed by the user at appropriate time.
// Transition rules can be found here:
// - https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12
// - https://devblogs.microsoft.com/directx/a-look-inside-d3d12-resource-state-barriers/
class ResourceStateTracker : protected UnorderedAccessTracker
{
  TextureResourceStateSet textureStates;
  BufferResourceStateSet bufferStates;
#if DX12_USE_AUTO_PROMOTE_AND_DECAY
  bool reportDecay = false;
#endif

public:
  ResourceStateTracker() = default;
  ~ResourceStateTracker() = default;

  ResourceStateTracker(const ResourceStateTracker &) = default;
  ResourceStateTracker &operator=(const ResourceStateTracker &) = default;

  ResourceStateTracker(ResourceStateTracker &&) = default;
  ResourceStateTracker &operator=(ResourceStateTracker &&) = default;

  enum class TransitionResult
  {
    // Executed and turned into a resource barrier
    Transtioned,
    // Executed and handled by auto promote logic of DX12
    AutoPromoted,
    // Executed and merged into existing barrier for this resource
    Merged,
    // Executed and merged end with existing begin barrier to transform into a regular barrier
    Folded,
    // Resource was in requested state already
    Skipped,
    // User requested to skip barrier of that kind
    UserSkipped,
    // Split barrier end had a broader state to cover than the actual call requested
    UnderspecifiedEnd,
    // Similar to merged, but instead to alter a barrier to merge the new state, the tracker had
    // to generate a barrier that transitioned the resource into a state that includes the old
    // state and the new state, which is in many cases a required but redundant barrier that should
    // we avoided by transition to the combined state directly.
    Expanded,
    // Internal only, never returned to the caller
    Fused,
  };

private:
  __forceinline void reportStateTransition(ID3D12Resource *resource, ExtendedImageGlobalSubresourceId global_base,
    SubresourceIndex sub_res_index, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, TransitionResult action)
  {
    G_UNUSED(resource);
    G_UNUSED(global_base);
    G_UNUSED(sub_res_index);
    G_UNUSED(from);
    G_UNUSED(to);
    G_UNUSED(action);
#if DAGOR_DBGLEVEL > 0
    G_ASSERT_RETURN(global_base.isValid(), );
    if (global_base.shouldReportTransitions())
    {
      reportStateTransitionOOL(resource, global_base.index(), sub_res_index.index(), true, from, to, action);
    }
#endif
  }
  __forceinline void reportStateTransition(ID3D12Resource *resource, BufferGlobalId global_base, D3D12_RESOURCE_STATES from,
    D3D12_RESOURCE_STATES to, TransitionResult action)
  {
    G_UNUSED(resource);
    G_UNUSED(global_base);
    G_UNUSED(from);
    G_UNUSED(to);
    G_UNUSED(action);
#if DAGOR_DBGLEVEL > 0
    if (global_base.shouldReportStateTransitions())
    {
      reportStateTransitionOOL(resource, global_base.index(), 0, false, from, to, action);
    }
#endif
  }
  DAGOR_NOINLINE void reportStateTransitionOOL(ID3D12Resource *resource, uint32_t global_base, uint32_t sub_res_index, bool is_texture,
    D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to, TransitionResult action)
  {
#if DX12_USE_AUTO_PROMOTE_AND_DECAY
    reportDecay = true;
#endif

    const char *resTypeTable[] = {"Buffer", "Texture"};
    char resnameBuffer[MAX_OBJECT_NAME_LENGTH];
    char fromTransitionMaskText[256];
    char toTransitionMaskText[256];

    get_resource_name(resource, resnameBuffer);
    resource_state_mask_as_string(from, fromTransitionMaskText);
    resource_state_mask_as_string(to, toTransitionMaskText);
    if (TransitionResult::Transtioned == action)
    {
      logdbg("DX12: StateTrack: Transitioned %s 0x%p <%s>[%u] %u from <%s> to <%s>", resTypeTable[is_texture], resource, resnameBuffer,
        sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
    else if (TransitionResult::AutoPromoted == action)
    {
      logdbg("DX12: StateTrack: Auto promoted %s 0x%p <%s>[%u] %u from <%s> to <%s>", resTypeTable[is_texture], resource,
        resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
    else if (TransitionResult::Merged == action)
    {
      logdbg("DX12: StateTrack: Updated existing transition of %s 0x%p <%s>[%u] %u from <%s> with <%s>", resTypeTable[is_texture],
        resource, resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
    else if (TransitionResult::Folded == action)
    {
      logdbg("DX12: StateTrack: Transformed split barrier into updated regular barrier (Folded) for "
             "%s 0x%p <%s>[%u] %u from <%s> with <%s>",
        resTypeTable[is_texture], resource, resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText,
        toTransitionMaskText);
    }
    else if (TransitionResult::Skipped == action)
    {
      logdbg("DX12: StateTrack: Skipped transition for %s 0x%p <%s>[%u] %u from <%s> to <%s>", resTypeTable[is_texture], resource,
        resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
    else if (TransitionResult::UserSkipped == action)
    {
      logdbg("DX12: StateTrack: Skipped transition on user request for %s 0x%p <%s>[%u] %u from <%s> "
             "to <%s>",
        resTypeTable[is_texture], resource, resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText,
        toTransitionMaskText);
    }
    else if (TransitionResult::UnderspecifiedEnd == action)
    {
      logdbg("DX12: StateTrack: Split barrier end was broader than the begin barrier of %s 0x%p "
             "<%s>[%u] %u from <%s> to <%s>",
        resTypeTable[is_texture], resource, resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText,
        toTransitionMaskText);
    }
    else if (TransitionResult::Expanded == action)
    {
      logdbg("DX12: StateTrack: Expansion %s 0x%p <%s>[%u] %u from <%s> with additional <%s>", resTypeTable[is_texture], resource,
        resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
    else if (TransitionResult::Fused == action)
    {
      logdbg("DX12: StateTrack: Fused back and forth barrier %s 0x%p <%s>[%u] %u from <%s> to <%s>", resTypeTable[is_texture],
        resource, resnameBuffer, sub_res_index, global_base + sub_res_index, fromTransitionMaskText, toTransitionMaskText);
    }
  }

public:
  D3D12_RESOURCE_STATES currentTextureState(Image *texture, SubresourceIndex sub_res)
  {
    return textureStates[texture->getGlobalSubResourceIdBase() + sub_res];
  }

  D3D12_RESOURCE_STATES currentbufferState(BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    return bufferStates[buffer.resourceId];
  }

  // This applies the resource state decay rules. Decays always happen at the end of a command list if the command list
  // is the last in a batch that is submitted to "ExecuteCommandLists". Decay does not happen in between command lits
  // that are part of the same submit to "ExecuteCommandLists"!
  // Rules:
  // Buffers always decay to common state (except for buffers with immutable state from upload or read back heap).
  // Textures decay when their state was changed by first usage, otherwise they state does not decay when the last explicitly
  // transitioned state was not the common state.
#if DX12_USE_AUTO_PROMOTE_AND_DECAY
  void decay()
  {
    bufferStates.decay();
    textureStates.decay();
    REPORT("DX12: Decaying state");
    if (reportDecay)
    {
      logdbg("DX12: StateTrack: Decaying resource state of all regular buffers and auto promoted textures to <COMMON>");
      reportDecay = false;
    }
  }
#endif

  void finishWorkItem(InititalResourceStateSet &initial_state)
  {
#if DX12_USE_AUTO_PROMOTE_AND_DECAY
    decay();
#endif

    initial_state.update(textureStates);
  }

  void setTextureState(InititalResourceStateSet &initial_state, ImageGlobalSubresourceId base, uint32_t count,
    D3D12_RESOURCE_STATES state)
  {
    if (!base.isValid())
    {
      return;
    }
    initial_state.initialize(base, count, state);
    textureStates.initialize(base, count, state);
  }

  void setTextureState(InititalResourceStateSet &initial_state, ValueRange<ExtendedImageGlobalSubresourceId> range,
    D3D12_RESOURCE_STATES state)
  {
    setTextureState(initial_state, range.front(), range.size(), state);
  }

  // TODO: have a optimized version for one subresource
  void transitionTexture(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *image,
    ExtendedImageGlobalSubresourceId global_base, SubresourceIndex res_base, uint32_t count,
    SubresourcePerFormatPlaneCount plane_width, FormatPlaneRange plane_count, D3D12_RESOURCE_STATES state)
  {
    G_ASSERT_RETURN(global_base.isValid(), );

    ID3D12Resource *res = image->getHandle();

    if (global_base.isStatic())
    {
#if DX12_FIX_UNITITALIZED_STATIC_TEXTURE_STATE
      bool isFullState = true;
      // when a read state is requested, upgrade it to full static texture read state
      // usually we don't need to handle 'substates' as we usually transition static
      // textures back to read state after writing to it, but in some rare cases
      // static textures where never written to and then used as a source.
      if ((D3D12_RESOURCE_STATE_COMMON != state) && (D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE != state) &&
          (state == (state & D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE)))
      {
        isFullState = false;
        state = D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE;
      }
#endif
      // Textures with the static bit set use only a very simple state system, which has
      // only 4 basic states:
      // - ready to copy data to by upload queue (or read back)
      // - target for gpu time line texture to texture copy
      // - any read w/o VRS
      // - any read w/ VRS
      if (is_valid_static_texture_state(state))
      {
        // transition to the generic texture read state after either upload or gpu time line copy
        // target
        for ([[maybe_unused]] const auto &p : plane_count)
        {
          auto i = res_base, e = res_base + count;
          res_base += plane_width;
          for (; i < e; ++i)
          {
            auto &currentState = textureStates[global_base + i];
            if (!currentState.needsTransition(state))
            {
              reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Skipped);
              continue;
            }

            if (currentState.canAutoPromote(state))
            {
              reportStateTransition(res, global_base, i, currentState, state, TransitionResult::AutoPromoted);
              currentState.autoPromote(state);
              continue;
            }
// only report errors on dev builds
#if DX12_FIX_UNITITALIZED_STATIC_TEXTURE_STATE && DAGOR_DBGLEVEL > 0
            if (!isFullState)
            {
              if (D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET == currentState)
              {
                char cbuf[MAX_OBJECT_NAME_LENGTH];
                get_resource_name(res, cbuf);
                // The resource state tracker can deduce if a static texture was properly initialized or not.
                // Fresh static textures start out with the D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET state and
                // after either uploading or copying into them, the state is changed to
                // D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE.
                // Copy and draw calls will only ask for the needed state and so isFullState will be false.
                // And finally when the current state is D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET and the next
                // state is not D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE or a write state, then
                // the texture was never initialized before it was used as a source for a copy, draw or
                // dispatch call.
                D3D_ERROR("DX12: Resource tracker deduced a possible uninitialized subresource %u of "
                          "the static texture <%s>, this is probably an engine error, where a static "
                          "texture was used as a source for copying or sampling before its content "
                          "was initialized",
                  i, cbuf);
              }
            }
#endif
            if (currentState.canMerge(state))
            {
              state = currentState.makeMergedState(state);
              // try to find a barrier that already tries to transition this resource
              // The list if barriers is usually less than 10 elements, so this is usually not a
              // problem.
              if (barriers.tryUpdateTransition(res, i, currentState, state))
              {
                reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Merged);
                currentState.transition(state);
                continue;
              }
            }
            else if (D3D12_RESOURCE_STATE_COPY_DEST == state && D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE == currentState)
            {
              // repeated copies to the same texture results in repeated back and forth between copy
              // dst and generic read state, try to avoid this
              if (barriers.tryEraseTransition(res, i, state, currentState))
              {
                reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Fused);
                currentState.transition(state);
                continue;
              }
            }

            barriers.transition(res, i, currentState, state);
            reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Transtioned);
            currentState.transition(state);
          }
        }
      }
      else
      {
#if DAGOR_DBGLEVEL
        char cbuf[MAX_OBJECT_NAME_LENGTH];
        get_resource_name(res, cbuf);
        G_ASSERTF(state == (D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE & state), // -V616
          "Unexpected state request for static texture %s state: 0x%08X / 0x%08X", cbuf, state,
          state & ~D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
        image->setReportStateTransitions();
#endif
      }
      return;
    }
    bool removeFromUav = false;
    for ([[maybe_unused]] const auto &p : plane_count)
    {
      auto i = res_base, e = res_base + count;
      res_base += plane_width;
      for (; i < e; ++i)
      {
        TransitionResult transitionType = TransitionResult::Transtioned;
        auto &currentState = textureStates[global_base + i];
        if (!currentState.needsTransition(state))
        {
          reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Skipped);
          continue;
        }

        if (currentState.canAutoPromote(state))
        {
          reportStateTransition(res, global_base, i, currentState, state, TransitionResult::AutoPromoted);
          currentState.autoPromote(state);
          continue;
        }

        // transitioning away from uav so we can remove the uav barrier for the resource, those barriers
        // are only needed for uav usage of a resource on multiple dispatches where in between the changes
        // have to be flushed.
        // only to do this once, so keep track of it and do it at the end
        removeFromUav = removeFromUav || (D3D12_RESOURCE_STATE_UNORDERED_ACCESS == currentState);

        if (currentState.canMerge(state))
        {
          state = currentState.makeMergedState(state);
          // try to find a barrier that already tries to transition this resource
          // The list if barriers is usually less than 10 elements, so this is usually not a
          // problem.
          if (barriers.tryUpdateTransition(res, i, currentState, state))
          {
            reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Merged);
            currentState.transition(state);
            continue;
          }
          transitionType = TransitionResult::Expanded;
        }

#if DX12_WHATCH_IN_FLIGHT_BARRIERS
        // if this fires, we missed a split barrier end somewhere
        auto optionalEnd = stt.endTransition(global_base + i);
        if (optionalEnd)
        {
          auto splitEndState = *optionalEnd;

          char cbuf[MAX_OBJECT_NAME_LENGTH];
          get_resource_name(res, cbuf);
          // remove it from in flight list or we generate a extra barrier that is invalid
          // we can fix those errors, but this is just a bandaid to keep going, root causes should
          // be properly fixed first see if we can patch a split barrier
          if (barriers.tryFixMissingEndTransition(res, i, state))
          {
            G_ASSERTF(false,
              "Missing split barrier end for %s - %p - %u - %u, queued pending begin "
              "barrier, stared with 0x%08X, ended with 0x%08X, was patched to end with "
              "0x%08X",
              cbuf, res, global_base, i, static_cast<D3D12_RESOURCE_STATES>(currentState), splitEndState, state);
            currentState.transition(state);
            image->setReportStateTransitions();
            global_base.enableTransitionReporting();
            continue;
          }
          else
          {
            G_ASSERTF(false,
              "Missing split barrier end for %s - %p - %u - %u, no pending begin barrier "
              "found, placing end barrier starting with 0x%08X, ending with 0x%08X",
              cbuf, res, global_base, i, static_cast<D3D12_RESOURCE_STATES>(currentState), splitEndState);
            image->setReportStateTransitions();
            global_base.enableTransitionReporting();
            // We insert the end barrier now and check if anything has to be done after that.
            barriers.endTransition(res, i, currentState, splitEndState);
            currentState.transition(splitEndState);

            // now after the resource is put into "splitEndState", we have to check the current state
            // again, with one exception merge can never be optimize by patching a existing barrier.
            if (!currentState.needsTransition(state))
            {
              reportStateTransition(res, global_base, i, currentState, state, TransitionResult::Skipped);
              continue;
            }

            if (currentState.canAutoPromote(state))
            {
              reportStateTransition(res, global_base, i, currentState, state, TransitionResult::AutoPromoted);

              currentState.autoPromote(state);
              continue;
            }
          }
        }
#else
        G_UNUSED(stt);
#endif
        reportStateTransition(res, global_base, i, currentState, state, transitionType);

        barriers.transition(res, i, currentState, state);

        currentState.transition(state);
      }
    }

    if (removeFromUav)
    {
      UnorderedAccessTracker::flushAccess(res);
    }
  }

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
  // This resets a buffer to its initial state, so that the state index or the whole buffer can be
  // reused later in a consistent way.
  // Only relevant for consoles as there auto decay is disabled and auto decay takes care of this on PC.
  void resetBufferState(BarrierBatcher &barriers, ID3D12Resource *res, const BufferGlobalId global_base)
  {
    auto &currentState = bufferStates[global_base];
    if (D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE != currentState)
    {
      barriers.transition(res, {}, currentState, D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE);
      reportStateTransition(res, global_base, static_cast<D3D12_RESOURCE_STATES>(currentState),
        D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE, TransitionResult::Transtioned);
      currentState.transition(D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE);
    }
  }
#endif

  // assumes base has no write states
  static D3D12_RESOURCE_STATES combine_with_static_states(BufferGlobalId buffer, D3D12_RESOURCE_STATES base)
  {
    if (buffer.isUsedAsVertexOrConstBuffer())
    {
      base |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
    if (buffer.isUsedAsIndexBuffer())
    {
      base |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
    }
    if (buffer.isUsedAsIndirectBuffer())
    {
      base |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    }
    if (buffer.isUsedAsNonPixelShaderResourceBuffer())
    {
      base |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (buffer.isUseAsPixelShaderResourceBuffer())
    {
      base |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    if (buffer.isUsedAsCopySourceBuffer())
    {
      base |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    return base;
  }

  TransitionResult transitionBuffer(BarrierBatcher &barriers, ID3D12Resource *res, const BufferGlobalId global_base,
    D3D12_RESOURCE_STATES state)
  {
    auto &currentState = bufferStates[global_base];

    if (!currentState.needsTransition(state))
    {
      reportStateTransition(res, global_base, static_cast<D3D12_RESOURCE_STATES>(currentState), state, TransitionResult::Skipped);
      return TransitionResult::Skipped;
    }

    if (currentState.canAutoPromote(state))
    {
      reportStateTransition(res, global_base, static_cast<D3D12_RESOURCE_STATES>(currentState), state, TransitionResult::AutoPromoted);
      currentState.autoPromote(state);
      return TransitionResult::AutoPromoted;
    }

    if (!has_write_state(state))
    {
      // if we are not writing we want to apply all known read states at once to minimize barrier uses
      state = combine_with_static_states(global_base, state);
    }

    TransitionResult transtionType = TransitionResult::Transtioned;
    if (currentState.canMerge(state))
    {
      // if merge able we want to transition to a state that includes the current state
      state = currentState.makeMergedState(state);

      if (barriers.tryUpdateTransition(res, {}, currentState, state))
      {
        reportStateTransition(res, global_base, static_cast<D3D12_RESOURCE_STATES>(currentState), state, TransitionResult::Merged);

        currentState.transition(state);

        return TransitionResult::Merged;
      }

      transtionType = TransitionResult::Expanded;
    }

    if (D3D12_RESOURCE_STATE_UNORDERED_ACCESS == currentState)
    {
      // transitioning away from uav so we can remove the uav barrier for the resource, those
      // barriers are only needed for uav usage of a resource on multiple dispatches where in
      // between the changes have to be flushed.
      UnorderedAccessTracker::flushAccess(res);
    }

    barriers.transition(res, {}, currentState, state);

    reportStateTransition(res, global_base, static_cast<D3D12_RESOURCE_STATES>(currentState), state, transtionType);

    currentState.transition(state);
    return transtionType;
  }

  void flushPendingUAVActions(BarrierBatcher &barriers, const char *where, bool validate_user_barriers)
  {
    UnorderedAccessTracker::iterateAccessToFlush(where, validate_user_barriers, [&barriers](auto res) { barriers.flushUAV(res); });
  }

  bool preTextureUploadTransition(BarrierBatcher &bb, InititalResourceStateSet &initial_state, ID3D12Resource *res,
    ExtendedImageGlobalSubresourceId global_base, SubresourceIndex sub_res, D3D12_RESOURCE_STATES state)
  {
    G_ASSERT_RETURN(global_base.isValid(), true);

    auto currentState = initial_state.getState(global_base, sub_res);
    if (state == currentState)
    {
      return bb.hasTranistionFor(res, sub_res);
    }

    bb.transition(res, sub_res, currentState, state);
    reportStateTransition(res, global_base, sub_res, currentState, state, TransitionResult::Transtioned);
    initial_state.initialize(global_base + sub_res, 1, state);
    return true;
  }

  void postTextureUploadTransition(BarrierBatcher &bb, ID3D12Resource *res, ExtendedImageGlobalSubresourceId global_base,
    SubresourceIndex sub_res, D3D12_RESOURCE_STATES state)
  {
    G_ASSERT_RETURN(global_base.isValid(), );

#if DX12_USE_AUTO_PROMOTE_AND_DECAY
    G_UNUSED(res);
    G_UNUSED(bb);
    G_UNUSED(state);
    textureStates[global_base + sub_res].initialize(D3D12_RESOURCE_STATE_COMMON);
#else
    if (bb.hasTranistionFor(res, sub_res))
    {
      return;
    }

    bb.transition(res, sub_res, state, D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
    reportStateTransition(res, global_base, sub_res, state, D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE,
      TransitionResult::Transtioned);
    textureStates[global_base + sub_res].initialize(D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
#endif
  }

  void beginTextureTransition(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *image,
    ExtendedImageGlobalSubresourceId global_base, SubresourceIndex subresource, D3D12_RESOURCE_STATES state)
  {
    if (!global_base.isValid())
    {
      return;
    }

    ID3D12Resource *texture = image->getHandle();
    auto &currentState = textureStates[global_base + subresource];
    if (!currentState.needsTransition(state))
    {
      return;
    }

    auto optionalEnd = stt.checkPendingTransition(global_base + subresource);
    if (!optionalEnd)
    {
      // when split barriers are disabled this will return false and we will fall though to a normal transition
      if (stt.beginTransition(global_base + subresource, state))
      {
        reportStateTransition(texture, global_base, subresource, currentState, state, TransitionResult::Transtioned);
        barriers.beginTransition(texture, subresource, currentState, state);
        return;
      }
    }
    else
    {
      auto endState = *optionalEnd;
      if (endState == state)
      {
        return;
      }
#if DAGOR_DBGLEVEL > 0
      char cbuf[MAX_OBJECT_NAME_LENGTH];
      G_ASSERTF(endState == state, // -V547 always false
        "Subsequent split barrier begins for %s with different target state 0x%08X != "
        "0x%08X",
        get_resource_name(texture, cbuf), endState, state);
      image->setReportStateTransitions();
#endif
      endTextureTransition(barriers, stt, image, global_base, subresource, endState);
    }
    // Only transitioning one subresource so we can input plane count and subres per plane as 1
    transitionTexture(barriers, stt, image, global_base, subresource, 1, SubresourcePerFormatPlaneCount::make(1),
      FormatPlaneCount::make(1), state);
  }

  TransitionResult endTextureTransition(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *image,
    ExtendedImageGlobalSubresourceId global_base, SubresourceIndex subresource, D3D12_RESOURCE_STATES state)
  {
    if (!global_base.isValid())
    {
      return TransitionResult::AutoPromoted;
    }

    ID3D12Resource *texture = image->getHandle();
    auto subResId = global_base + subresource;
    auto optionalEnd = stt.endTransition(subResId);
    if (!optionalEnd)
    {
      return TransitionResult::Skipped;
    }

    const auto &endState = *optionalEnd;

#if DAGOR_DBGLEVEL > 0
    if (state != (endState & state))
    {
      char cbuf[MAX_OBJECT_NAME_LENGTH];
      G_ASSERTF(state == (endState & state),
        "Split barrier end with incompatible end state 0x%08X != (0x%08X & 0x%08X) for %s "
        "- %p - %u",
        state, endState, state, get_resource_name(texture, cbuf), texture, subresource);
      image->setReportStateTransitions();
    }
#endif

    TransitionResult result = TransitionResult::Transtioned;
    if (endState != state)
    {
      result = TransitionResult::UnderspecifiedEnd;
    }

    auto &currentState = textureStates[subResId];

#if DX12_FOLD_BATCHED_SPLIT_BARRIERS
    // sometimes we batch up begin and end into the same batch set, this makes no sense and so we
    // optimize the begin/end pair into one regular barrier
    if (barriers.tryFuseBeginEndTransition(texture, subresource, currentState, endState))
    {
      reportStateTransition(texture, global_base, subresource, currentState, state, TransitionResult::Folded);
      result = TransitionResult::Folded;
    }
    else
#endif
    {
      barriers.endTransition(texture, subresource, currentState, endState);
    }
    currentState.transition(endState);
    return result;
  }

  // Returns false if at least one subresource is not in the requested state after completing all outstanding barriers
  // TODO replace res_base and count with subresource range
  bool endTextureTransitions(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *image,
    ExtendedImageGlobalSubresourceId global_base, SubresourceIndex res_base, uint32_t count,
    SubresourcePerFormatPlaneCount plane_width, FormatPlaneCount plane_count, D3D12_RESOURCE_STATES state)
  {
    if (!global_base.isValid())
    {
      return true;
    }

    ID3D12Resource *texture = image->getHandle();
    uint32_t i = 0;
    bool allEndStateMatchRequestedState = true;
    for (;; ++i)
    {
      auto resBase = global_base + res_base;
      auto optionalEnd = stt.endTransitionInRange(resBase, count);
      auto planeIndex = eastl::begin(plane_count);
      auto planeOffset = SubresourceIndex::make(0);
      while (!optionalEnd)
      {
        planeOffset += plane_width;
        ++planeIndex;
        if (planeIndex == eastl::end(plane_count))
        {
          break;
        }
        optionalEnd = stt.endTransitionInRange(resBase + planeOffset, count);
      }

      if (!optionalEnd)
      {
        break;
      }

      auto subresource = optionalEnd->first.toSubresouceIndex(global_base);
      auto endState = optionalEnd->second;

#if DAGOR_DBGLEVEL > 0
      // On certain cases it is fine to fix up split barriers (SRV use after async read back), but in other
      // cases its not (UAV use after asnyc read back, as it changes what is read back).
      if ((state != (endState & state)) && (!has_read_state(endState) || !has_read_state(state)))
      {
        char cbuf[MAX_OBJECT_NAME_LENGTH];
        G_ASSERTF(state == (endState & state),
          "DX12: Split barrier ended early with either previus end state or new end state "
          "not being read states (0x%08X != (0x%08X & 0x%08X)) for %s - %p - %u",
          state, endState, state, get_resource_name(texture, cbuf), texture, subresource);
        image->setReportStateTransitions();
      }
#endif

      TransitionResult result = TransitionResult::Transtioned;
      if (endState != state)
      {
        result = TransitionResult::UnderspecifiedEnd;
        // To not break tracking a later barrier is needed to fix up expected state
        allEndStateMatchRequestedState = false;
      }

      auto &currentState = textureStates[optionalEnd->first];
#if DX12_FOLD_BATCHED_SPLIT_BARRIERS
      // sometimes we batch up begin and end into the same batch set, this makes no sense and so we
      // optimize the begin/end pair into one regular barrier
      if (barriers.tryFuseBeginEndTransition(texture, subresource, currentState, endState))
      {
        result = TransitionResult::Folded;
      }
      else
#endif
      {
        barriers.endTransition(texture, subresource, currentState, endState);
      }
      reportStateTransition(texture, global_base, subresource, currentState, endState, result);
      currentState.transition(endState);
    }
    return allEndStateMatchRequestedState && ((count * plane_count.count()) == i);
  }
};

class ResourceUsageManager : protected ResourceStateTracker
{
  using BaseType = ResourceStateTracker;

  ID3D12Resource *scratchBuffer = nullptr;
  D3D12_RESOURCE_STATES scratchBufferState = D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE;

  void changeScratchBufferState(BarrierBatcher &barriers, ID3D12Resource *buffer, D3D12_RESOURCE_STATES new_state)
  {
    if (scratchBuffer == buffer && scratchBufferState != new_state)
    {
      barriers.transition(buffer, {}, scratchBufferState, new_state);
    }
    scratchBuffer = buffer;
    scratchBufferState = new_state;
  }

public:
  // TODO?
  using BaseType::currentTextureState;
  using BaseType::flushPendingUAVActions;
  using BaseType::setTextureState;
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
  using BaseType::resetBufferState;
#endif

  void useScratchAsCopySource(BarrierBatcher &barriers, ID3D12Resource *buffer)
  {
    changeScratchBufferState(barriers, buffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
  }

  void useScratchAsCopyDestination(BarrierBatcher &barriers, ID3D12Resource *buffer)
  {
    changeScratchBufferState(barriers, buffer, D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void finishWorkItem(InititalResourceStateSet &initial_state)
  {
    BaseType::finishWorkItem(initial_state);
    scratchBuffer = nullptr;
  }

  bool beginImplicitUAVAccess(ID3D12Resource *res) { return UnorderedAccessTracker::beginImplicitAccess(res); }

  void implicitUAVFlushAll(const char *where, bool report_user_barriers)
  {
    UnorderedAccessTracker::implicitFlushAll(where, report_user_barriers);
  }

  // buffers
  void useBufferAsCBV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    G_UNUSED(stage);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  }

  void useBufferAsSRV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    D3D12_RESOURCE_STATES targetState =
      (STAGE_PS == stage) ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, targetState);
  }

  void useBufferAsUAV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    G_UNUSED(stage);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }

  void useBufferAsIB(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_INDEX_BUFFER);
  }

  void useBufferAsVB(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  }

  void useBufferAsRTASBuildSource(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }

  void useBufferAsIA(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
  }

  void useBufferAsCopySource(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_COPY_SOURCE);
  }

  void useBufferAsCopyDestination(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void useBufferAsUAVForClear(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    UnorderedAccessTracker::beginImplicitAccess(buffer.buffer);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
  }

  void useBufferAsQueryResultDestination(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void userBufferTransitionBarrier(BarrierBatcher &barriers, BufferResourceReference buffer, ResourceBarrier barrier)
  {
    // User barriers make never sense for an untracked buffer, such buffers can not be modified by
    // the GPU and so will only reside in generic read state
    G_ASSERT(buffer.resourceId);
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, translate_buffer_barrier_to_state(barrier));
  }

  void useBufferAsPredication(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (!buffer.resourceId)
    {
      return;
    }
    transitionBuffer(barriers, buffer.buffer, buffer.resourceId, D3D12_RESOURCE_STATE_PREDICATION);
  }

  void userBufferUAVAccessFlush(BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    // may returns false, indicating that there was no need for the uav flush
    UnorderedAccessTracker::explicitAccess(buffer.buffer);
  }

  void userBufferUAVAccessFlushSkip(BufferResourceReference buffer)
  {
    G_ASSERT(buffer.resourceId);
    UnorderedAccessTracker::ignoreNextDependency(buffer.buffer);
  }

  // textures
  void useTextureAsSRV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ImageViewState view,
    bool as_const_ds)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    D3D12_RESOURCE_STATES state =
      (as_const_ds ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_COMMON) |
      ((stage == STAGE_PS) ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    auto plane = view.getPlaneIndex();
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
    {
      if (!endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerResourceIndex(view.getMipBase(), ArrayLayerIndex::make(0), plane), view.getMipCount(),
            SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), state))
      {
        transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerResourceIndex(view.getMipBase(), ArrayLayerIndex::make(0), plane), view.getMipCount(),
          SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), state);
      }
    }
    else
    {
      auto arrayRange = view.getArrayRange();
      bool needsRegularBarrier = false;
      for (auto a : arrayRange)
      {
        bool didEnd = endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerResourceIndex(view.getMipBase(), a, plane), view.getMipCount(), SubresourcePerFormatPlaneCount::make(1),
          FormatPlaneCount::make(1), state);
        needsRegularBarrier = needsRegularBarrier || !didEnd;
      }
      if (needsRegularBarrier)
      {
        for (auto a : arrayRange)
        {
          transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerResourceIndex(view.getMipBase(), a, plane), view.getMipCount(),
            SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), state);
        }
      }
    }
  }

  void useTextureAsUAV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ArrayLayerRange arrays,
    MipMapRange mips)
  {
    G_ASSERT(texture->hasTrackedState());
    G_UNUSED(stage);
    auto mipBase = mips.front();
    auto mipCount = mips.size();
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
    {
      if (!endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerIndexToStateIndex(mipBase, ArrayLayerIndex::make(0)), mipCount,
            SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
      {
        transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerIndexToStateIndex(mipBase, ArrayLayerIndex::make(0)), mipCount, SubresourcePerFormatPlaneCount::make(1),
          FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      }
    }
    else
    {
      bool needsRegularBarrier = false;
      for (auto a : arrays)
      {
        bool didEnd = endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerIndexToStateIndex(mipBase, a), mipCount, SubresourcePerFormatPlaneCount::make(1),
          FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        needsRegularBarrier = needsRegularBarrier || !didEnd;
      }
      if (needsRegularBarrier)
      {
        for (auto a : arrays)
        {
          transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerIndexToStateIndex(mipBase, a), mipCount, SubresourcePerFormatPlaneCount::make(1),
            FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
      }
    }
  }

  void useTextureAsUAV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ImageViewState view)
  {
    useTextureAsUAV(barriers, stt, stage, texture, view.getArrayRange(), view.getMipRange());
  }

  void readyTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    endTextureTransition(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }

  void beginReadyTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    beginTextureTransition(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }

  void useTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }

  void useTextureAsSwapImageOutput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_PRESENT);
  }

  void useTextureAsDSVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(mip_level, array_layer, FormatPlaneIndex::make(0)), 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
  }

  void useTextureAsDSVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    useTextureAsDSVForClear(barriers, stt, texture, view.getArrayBase(), view.getMipBase());
  }

  void useTextureAsRTVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(mip_level, array_layer, FormatPlaneIndex::make(0)), 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_RENDER_TARGET);
  }

  void useTextureAsRTVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    useTextureAsRTVForClear(barriers, stt, texture, view.getArrayBase(), view.getMipBase());
  }

  void useTextureAsCopySourceForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(),
      D3D12_RESOURCE_STATE_COPY_SOURCE);
  }

  void useTextureAsCopySourceForWholeCopyOnReadbackQueue(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(),
      D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }

  void useTextureAsCopyDestinationForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    G_ASSERT(texture->hasTrackedState());
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(),
      D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void useTextureAsCopyDestinationForWholeCopyOnReadbackQueue(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(),
      D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET);
  }

  void finishUseTextureAsCopyDestinationForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (texture->isGPUChangeable())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(),
      D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
  }

  void useTextureAsCopySource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_COPY_SOURCE);
  }

  void useTextureAsCopyDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void finishUseTextureAsCopyDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    if (texture->isGPUChangeable())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
  }

  // now only resolving one subresource
  void useTextureAsResolveSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    G_ASSERT(texture->hasTrackedState());
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      texture->getSubresourcesPerPlane(), texture->getPlaneCount(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
  }

  void useTextureAsResolveDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    G_ASSERT(texture->hasTrackedState());
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      texture->getSubresourcesPerPlane(), texture->getPlaneCount(), D3D12_RESOURCE_STATE_RESOLVE_DEST);
  }

  void useTextureAsBlitSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(view.getMipBase(), view.getArrayBase(), FormatPlaneIndex::make(0)), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  }

  void useTextureAsBlitDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(view.getMipBase(), view.getArrayBase(), FormatPlaneIndex::make(0)), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_RENDER_TARGET);
  }

  void useTextureAsRTV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerRange arrays,
    MipMapIndex mip_level)
  {
    auto arrayCount = arrays.size();
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
    {
      arrayCount = 1;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(mip_level, arrays.front(), FormatPlaneIndex::make(0)), arrayCount,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_RENDER_TARGET);
  }

  void useTextureAsRTV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    useTextureAsRTV(barriers, stt, texture, view.getArrayRange(), view.getMipBase());
  }

  void useTextureAsDSV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(mip_level, array_layer, FormatPlaneIndex::make(0)), 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
  }

  void useTextureAsDSV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    useTextureAsDSV(barriers, stt, texture, view.getArrayBase(), view.getMipBase());
  }

  void useTextureAsDSVConst(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(view.getMipBase(), view.getArrayBase(), FormatPlaneIndex::make(0)), 1,
      texture->getSubresourcesPerPlane(), texture->getPlaneCount(), D3D12_RESOURCE_STATE_DEPTH_READ);
  }

  void useTextureAsUAVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerRange arrays,
    MipMapRange mips)
  {
    auto mipBase = mips.front();
    auto mipCount = mips.size();
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
    {
      if (!endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerIndexToStateIndex(mipBase, ArrayLayerIndex::make(0)), mipCount,
            SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
      {
        transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerIndexToStateIndex(mipBase, ArrayLayerIndex::make(0)), mipCount, SubresourcePerFormatPlaneCount::make(1),
          FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
      }
    }
    else
    {
      bool needsRegularBarrier = false;
      for (auto a : arrays)
      {
        bool didEnd = endTextureTransitions(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
          texture->mipAndLayerIndexToStateIndex(mipBase, a), mipCount, SubresourcePerFormatPlaneCount::make(1),
          FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        needsRegularBarrier = needsRegularBarrier || !didEnd;
      }
      if (needsRegularBarrier)
      {
        for (auto a : arrays)
        {
          transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
            texture->mipAndLayerIndexToStateIndex(mipBase, a), mipCount, SubresourcePerFormatPlaneCount::make(1),
            FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
      }
    }

    UnorderedAccessTracker::beginImplicitAccess(texture->getHandle());
  }

  void useTextureAsUAVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    useTextureAsUAVForClear(barriers, stt, texture, view.getArrayRange(), view.getMipRange());
  }

#if !_TARGET_XBOXONE
  void useTextureAsVRSSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
  }
#endif

  void useTextureAsDLSSInput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1),
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
  }

  void useTextureAsDLSSOutput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0), 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    UnorderedAccessTracker::beginImplicitAccess(texture->getHandle());
  }

  void useTextureAsUploadDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_COPY_DEST);
  }

  void finishUseTextureAsUploadDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceIndex sub_res)
  {
    if (texture->isGPUChangeable())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), sub_res, 1,
      SubresourcePerFormatPlaneCount::make(1), FormatPlaneCount::make(1), D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
  }

  void useTextureToAliasFrom(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, uint32_t tex_flags)
  {
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    if (TEXCF_RTARGET & tex_flags)
    {
      if (texture->getFormat().isColor())
      {
        state = D3D12_RESOURCE_STATE_RENDER_TARGET;
      }
      else
      {
        state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
      }
    }
    else if (TEXCF_UNORDERED & tex_flags)
    {
      state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    if (D3D12_RESOURCE_STATE_COMMON != state)
    {
      transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
        texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(), state);
    }
  }

  void useTextureToAliasToAndDiscard(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, uint32_t tex_flags)
  {
    D3D12_RESOURCE_STATES state;
    if (TEXCF_RTARGET & tex_flags)
    {
      if (texture->getFormat().isColor())
      {
        state = D3D12_RESOURCE_STATE_RENDER_TARGET;
      }
      else
      {
        state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
      }
    }
    else if (TEXCF_UNORDERED & tex_flags)
    {
      state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
      D3D_ERROR("DX12: Trying to discard a texture that is neither a render target, a depth stencil "
                "target nor allows unordered access");
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      texture->getSubresourcesPerPlane().count(), texture->getSubresourcesPerPlane(), texture->getPlaneCount(), state);
  }

  void userTextureTransitionSplitBarrierBegin(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    auto resId = texture->getGlobalSubResourceIdBase();
    auto planes = texture->getPlaneCount();
    auto planeWidth = texture->getSubresourcesPerPlane();
    auto state = translate_texture_barrier_to_state(barrier, !texture->getFormat().isColor());
    for (auto i : sub_res_range)
    {
      beginTextureTransition(barriers, stt, texture, resId, i, state);
    }
    if (planes.count() > 1)
    {
      for (auto i : sub_res_range)
      {
        beginTextureTransition(barriers, stt, texture, resId, i + planeWidth, state);
      }
    }
  }

  void userTextureTransitionSplitBarrierEnd(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    auto resId = texture->getGlobalSubResourceIdBase();
    auto planes = texture->getPlaneCount();
    auto planeWidth = texture->getSubresourcesPerPlane();
    auto state = translate_texture_barrier_to_state(barrier, !texture->getFormat().isColor());
    for (auto i : sub_res_range)
    {
      endTextureTransition(barriers, stt, texture, resId, i, state);
    }

    if (planes.count() > 1)
    {
      for (auto i : sub_res_range)
      {
        endTextureTransition(barriers, stt, texture, resId, i + planeWidth, state);
      }
    }
  }

  void userTextureTransitionBarrier(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    auto resId = texture->getGlobalSubResourceIdBase();
    auto planes = texture->getPlaneCount();
    auto planeWidth = texture->getSubresourcesPerPlane();
    auto state = translate_texture_barrier_to_state(barrier, !texture->getFormat().isColor());
    transitionTexture(barriers, stt, texture, resId, sub_res_range.front(), sub_res_range.size(), planeWidth, planes, state);
  }

  void beginTextureCPUAccess(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      (texture->getSubresourcesPerPlane() * texture->getPlaneCount()).count(), SubresourcePerFormatPlaneCount::make(1),
      FormatPlaneCount::make(1), D3D12_RESOURCE_STATE_COMMON);
  }

  void endTextureCPUAccess(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(), SubresourceIndex::make(0),
      (texture->getSubresourcesPerPlane() * texture->getPlaneCount()).count(), SubresourcePerFormatPlaneCount::make(1),
      FormatPlaneCount::make(1), D3D12_RESOURCE_STATES_STATIC_TEXTURE_READ_STATE);
  }

  void useTextureAsMipGenSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, MipMapIndex mip,
    ArrayLayerIndex ary)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    transitionTexture(barriers, stt, texture, texture->getGlobalSubResourceIdBase(),
      texture->mipAndLayerResourceIndex(mip, ary, FormatPlaneIndex::make(0)), 1, texture->getSubresourcesPerPlane(),
      texture->getPlaneCount(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  }

  bool beginUseTextureAsUploadDestinationOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return preTextureUploadTransition(bb, initial_state, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET);
  }

  void finishUseTextureAsUploadDestinationOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    postTextureUploadTransition(bb, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET);
  }

  void userTextureUAVAccessFlush(Image *texture) { UnorderedAccessTracker::explicitAccess(texture->getHandle()); }

  void userTextureUAVAccessFlushSkip(Image *texture) { UnorderedAccessTracker::ignoreNextDependency(texture->getHandle()); }

  // misc

  void userUAVAccessFlush() { UnorderedAccessTracker::flushAccessToAll(); }

  void useRTASAsUpdateSource(BarrierBatcher &, ID3D12Resource *as) { UnorderedAccessTracker::beginImplicitAccess(as); }

  void useRTASAsBuildTarget(BarrierBatcher &, ID3D12Resource *) {}

  // Driver explicit barriers
  TransitionResult transitionBufferExplicit(BarrierBatcher &barriers, BufferResourceReference buffer, D3D12_RESOURCE_STATES state)
  {
    return transitionBuffer(barriers, buffer.buffer, buffer.resourceId, state);
  }

  bool beginUseTextureAsMipTransferDestinationOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return preTextureUploadTransition(bb, initial_state, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET);
  }

  void finishUseTextureAsMipTransferDestinationOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    postTextureUploadTransition(bb, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET);
  }

  bool beginUseTextureAsMipTransferSourceOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return preTextureUploadTransition(bb, initial_state, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }

  void finishUseTextureAsMipTransferSourceOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    postTextureUploadTransition(bb, texture->getHandle(), texture->getGlobalSubResourceIdBase(), sub_res,
      D3D12_RESOURCE_STATE_COPY_QUEUE_SOURCE);
  }
};

#undef REPORT

class ResourceUsageHistoryDataSet
{
public:
  enum class UsageEntryType
  {
    CBV_CS,
    CBV_PS,
    CBV_VS,
    CBV_RT,

    SRV_CS,
    SRV_PS,
    SRV_VS,
    SRV_RT,

    UAV_CS,
    UAV_PS,
    UAV_VS,
    UAV_RT,
    UAV_ANY,

    INDEX_BUFFER,
    VERTEX_BUFFER,

    RT_AS_BUID_SOURCE,

    INDIRECT_BUFFER,

    COPY_SOURCE,
    COPY_DESTINATION,

    COPY_SOURCE_ALL,
    COPY_DESTINATION_ALL,

    COPY_SOURCE_PLANE1,
    COPY_DESTINATION_PLANE1,

    UAV_CLEAR,
    DSV_PLANE0_CLEAR,
    DSV_PLANE1_CLEAR,
    RTV_CLEAR,

    DSV_PLANE0,
    DSV_PLANE1,
    RTV,

    DSV_PLANE0_CONST,
    DSV_PLANE1_CONST,

    BLIT_SOURCE,
    BLIT_DESTINATION,

    READ_BACK_DEFERRED_BEGIN,
    READ_BACK,

    VARIABLE_RATE_SHADING,

    DLSS_INPUT,
    DLSS_OUTPUT,

    UPLOAD_DESTINATION,

    CPU_ACCESS_BEGIN,
    CPU_ACCESS_END,

    MIPGEN_SOURCE,

    USER_BARRIER,
    USER_FLUSH_UAV,

    INVALID_VALUE
  };

private:
  struct BasicUsageEntry
  {
    D3D12_RESOURCE_STATES state;
    ResourceBarrier expectedState;
    size_t segmentIndex;
    size_t nameIndex;
    UsageEntryType what;
  };

  struct TextureUsageEntry : BasicUsageEntry
  {
    Image *texture = nullptr;
    SubresourceIndex subRes;
    FormatStore fmt;
    // part of this type on purpose to make wrong implementation harder
    size_t nextEntry = 0;
  };

  struct BufferUsageEntry : BasicUsageEntry
  {
    BufferResourceReference buffer;
    // part of this type on purpose to make wrong implementation harder
    size_t nextEntry = 0;
  };

  dag::Vector<eastl::string> segments;
  // have to store the resource name separate for later use, when resources may no longer be valid
  dag::Vector<eastl::string> resourceNames;
  dag::Vector<TextureUsageEntry> textureUsageEntries;
  dag::Vector<BufferUsageEntry> bufferUsageEntries;

  static bool is_automatic_transition(UsageEntryType type)
  {
    switch (type)
    {
      case UsageEntryType::CBV_CS:
      case UsageEntryType::CBV_PS:
      case UsageEntryType::CBV_VS:
      case UsageEntryType::CBV_RT: return false;
      case UsageEntryType::SRV_CS:
      case UsageEntryType::SRV_PS:
      case UsageEntryType::SRV_VS:
      case UsageEntryType::SRV_RT: return false;
      case UsageEntryType::UAV_CS:
      case UsageEntryType::UAV_PS:
      case UsageEntryType::UAV_VS:
      case UsageEntryType::UAV_RT:
      case UsageEntryType::UAV_CLEAR:
      case UsageEntryType::UAV_ANY: return true;
      case UsageEntryType::INDEX_BUFFER:
      case UsageEntryType::VERTEX_BUFFER:
      case UsageEntryType::RT_AS_BUID_SOURCE:
      case UsageEntryType::INDIRECT_BUFFER:
      case UsageEntryType::COPY_SOURCE:
      case UsageEntryType::COPY_SOURCE_ALL:
      case UsageEntryType::COPY_SOURCE_PLANE1: return false;
      case UsageEntryType::COPY_DESTINATION:
      case UsageEntryType::COPY_DESTINATION_ALL:
      case UsageEntryType::COPY_DESTINATION_PLANE1: return true;
      case UsageEntryType::DSV_PLANE0_CLEAR:
      case UsageEntryType::DSV_PLANE1_CLEAR:
      case UsageEntryType::RTV_CLEAR:
      case UsageEntryType::DSV_PLANE0:
      case UsageEntryType::DSV_PLANE1:
      case UsageEntryType::RTV: return true;
      case UsageEntryType::DSV_PLANE0_CONST:
      case UsageEntryType::DSV_PLANE1_CONST: return false;
      case UsageEntryType::BLIT_SOURCE: return false;
      case UsageEntryType::BLIT_DESTINATION: return true;
      case UsageEntryType::READ_BACK_DEFERRED_BEGIN:
      case UsageEntryType::READ_BACK: return true;
      case UsageEntryType::VARIABLE_RATE_SHADING: return true;
      case UsageEntryType::DLSS_INPUT:
      case UsageEntryType::DLSS_OUTPUT: return true;
      case UsageEntryType::UPLOAD_DESTINATION: return true;
      case UsageEntryType::CPU_ACCESS_BEGIN:
      case UsageEntryType::CPU_ACCESS_END: return true;
      case UsageEntryType::MIPGEN_SOURCE: return true;
      case UsageEntryType::USER_BARRIER:
      case UsageEntryType::USER_FLUSH_UAV: return true;
      case UsageEntryType::INVALID_VALUE: return false;
    }
    G_ASSERT(false);
    return true;
  }

  // This means that multiple events of the same type in the same execution section can be folded into one (eg only the
  // first occurrence is recorded).
  static bool is_mergable(UsageEntryType type)
  {
    switch (type)
    {
      case UsageEntryType::CBV_CS:
      case UsageEntryType::CBV_PS:
      case UsageEntryType::CBV_VS:
      case UsageEntryType::CBV_RT:
      case UsageEntryType::SRV_CS:
      case UsageEntryType::SRV_PS:
      case UsageEntryType::SRV_VS:
      case UsageEntryType::SRV_RT:
      case UsageEntryType::UAV_CS:
      case UsageEntryType::UAV_PS:
      case UsageEntryType::UAV_VS:
      case UsageEntryType::UAV_RT:
      case UsageEntryType::UAV_CLEAR:
      case UsageEntryType::UAV_ANY:
      case UsageEntryType::INDEX_BUFFER:
      case UsageEntryType::VERTEX_BUFFER:
      case UsageEntryType::RT_AS_BUID_SOURCE:
      case UsageEntryType::INDIRECT_BUFFER:
      case UsageEntryType::COPY_SOURCE:
      case UsageEntryType::COPY_SOURCE_ALL:
      case UsageEntryType::COPY_SOURCE_PLANE1:
      case UsageEntryType::COPY_DESTINATION:
      case UsageEntryType::COPY_DESTINATION_ALL:
      case UsageEntryType::COPY_DESTINATION_PLANE1:
      case UsageEntryType::DSV_PLANE0_CLEAR:
      case UsageEntryType::DSV_PLANE1_CLEAR:
      case UsageEntryType::RTV_CLEAR:
      case UsageEntryType::DSV_PLANE0:
      case UsageEntryType::DSV_PLANE1:
      case UsageEntryType::RTV:
      case UsageEntryType::DSV_PLANE0_CONST:
      case UsageEntryType::DSV_PLANE1_CONST:
      case UsageEntryType::BLIT_SOURCE:
      case UsageEntryType::BLIT_DESTINATION:
      case UsageEntryType::READ_BACK_DEFERRED_BEGIN:
      case UsageEntryType::READ_BACK:
      case UsageEntryType::VARIABLE_RATE_SHADING:
      case UsageEntryType::DLSS_INPUT:
      case UsageEntryType::DLSS_OUTPUT:
      case UsageEntryType::UPLOAD_DESTINATION:
      case UsageEntryType::CPU_ACCESS_BEGIN:
      case UsageEntryType::CPU_ACCESS_END:
      case UsageEntryType::MIPGEN_SOURCE: return true;
      // user barrier actions can not be merged
      case UsageEntryType::USER_BARRIER:
      case UsageEntryType::USER_FLUSH_UAV:
      case UsageEntryType::INVALID_VALUE: return false;
    }
    G_ASSERT(false);
    return true;
  }

  static const char *to_string(UsageEntryType type)
  {
    switch (type)
    {
      case UsageEntryType::CBV_CS: return "CBV on compute";
      case UsageEntryType::CBV_PS: return "CBV on pixel";
      case UsageEntryType::CBV_VS: return "CBV on vertex";
      case UsageEntryType::CBV_RT: return "CBV on raytrace";
      case UsageEntryType::SRV_CS: return "SRV on compute";
      case UsageEntryType::SRV_PS: return "SRV on pixel";
      case UsageEntryType::SRV_VS: return "SRV on vertex";
      case UsageEntryType::SRV_RT: return "SRV on raytrace";
      case UsageEntryType::UAV_CS: return "UAV on compute";
      case UsageEntryType::UAV_PS: return "UAV on pixel";
      case UsageEntryType::UAV_VS: return "UAV on vertex";
      case UsageEntryType::UAV_RT: return "UAV on raytrace";
      case UsageEntryType::UAV_ANY: return "UAV on any stage";
      case UsageEntryType::INDEX_BUFFER: return "index buffer";
      case UsageEntryType::VERTEX_BUFFER: return "vertex buffer";
      case UsageEntryType::RT_AS_BUID_SOURCE: return "RTAS build source";
      case UsageEntryType::INDIRECT_BUFFER: return "indirect buffer";
      case UsageEntryType::COPY_SOURCE: return "copy source";
      case UsageEntryType::COPY_DESTINATION: return "copy destination";
      case UsageEntryType::COPY_SOURCE_ALL: return "copy source all";
      case UsageEntryType::COPY_DESTINATION_ALL: return "copy destination all";
      case UsageEntryType::COPY_SOURCE_PLANE1: return "copy source plane 1";
      case UsageEntryType::COPY_DESTINATION_PLANE1: return "copy destination plane 1";
      case UsageEntryType::UAV_CLEAR: return "UAV clear";
      case UsageEntryType::DSV_PLANE0_CLEAR: return "DSV plane 0 clear";
      case UsageEntryType::DSV_PLANE1_CLEAR: return "DSV plane 1 clear";
      case UsageEntryType::RTV_CLEAR: return "RTV clear";
      case UsageEntryType::DSV_PLANE0: return "DSV plane 0";
      case UsageEntryType::DSV_PLANE1: return "DSV plane 1";
      case UsageEntryType::RTV: return "RTV";
      case UsageEntryType::DSV_PLANE0_CONST: return "DSV plane 0 const";
      case UsageEntryType::DSV_PLANE1_CONST: return "DSV plane 1 const";
      case UsageEntryType::BLIT_SOURCE: return "blit source";
      case UsageEntryType::BLIT_DESTINATION: return "blit destination";
      case UsageEntryType::READ_BACK_DEFERRED_BEGIN: return "read back deferred begin";
      case UsageEntryType::READ_BACK: return "read back";
      case UsageEntryType::VARIABLE_RATE_SHADING: return "variable rate shading";
      case UsageEntryType::DLSS_INPUT: return "DLSS input";
      case UsageEntryType::DLSS_OUTPUT: return "DLSS output";
      case UsageEntryType::UPLOAD_DESTINATION: return "upload destination";
      case UsageEntryType::CPU_ACCESS_BEGIN: return "CPU access begin";
      case UsageEntryType::CPU_ACCESS_END: return "CPU access end";
      case UsageEntryType::MIPGEN_SOURCE: return "mipgen source";
      case UsageEntryType::USER_BARRIER: return "USER BARRIER";
      case UsageEntryType::USER_FLUSH_UAV: return "USER_FLUSH_UAV";
      case UsageEntryType::INVALID_VALUE: return "INVALID_VALUE";
    }
    G_ASSERT(false);
    return "<error>";
  }

  TextureUsageEntry *next(TextureUsageEntry *e)
  {
    if (e->nextEntry > textureUsageEntries.size())
    {
      return nullptr;
    }
    return &textureUsageEntries[e->nextEntry];
  }

  BufferUsageEntry *next(BufferUsageEntry *e)
  {
    if (e->nextEntry > bufferUsageEntries.size())
    {
      return nullptr;
    }
    return &bufferUsageEntries[e->nextEntry];
  }

  eastl::string_view segmentName(size_t index) const
  {
    if (index < segments.size())
    {
      return segments[index];
    }
    return "";
  }

  eastl::string_view resourceName(size_t index) const
  {
    if (index < resourceNames.size())
    {
      return resourceNames[index];
    }
    return "";
  }

  template <typename T>
  bool visit(const TextureUsageEntry &e, T visitor)
  {
    return visitor(e.texture, resourceName(e.nameIndex), e.fmt, e.subRes, e.state, e.expectedState, segmentName(e.segmentIndex),
      e.what, is_automatic_transition(e.what), to_string(e.what));
  }

  template <typename T>
  bool visit(const BufferUsageEntry &e, T visitor)
  {
    return visitor(e.buffer, resourceName(e.nameIndex), e.state, e.expectedState, segmentName(e.segmentIndex), e.what,
      is_automatic_transition(e.what), to_string(e.what));
  }

  size_t addResourceName(eastl::string_view name)
  {
    // for what ever reason, the compiler fails to see the conversion operator to string view and
    // complains on simple find. So have to resort to find_if with a lambda that make this explicit.
    auto ref = eastl::find_if(begin(resourceNames), end(resourceNames),
      [name](auto &s) //
      { return name == static_cast<eastl::string_view>(s); });
    if (ref != end(resourceNames))
    {
      return static_cast<size_t>(eastl::distance(eastl::begin(resourceNames), ref));
    }

    resourceNames.emplace_back(begin(name), end(name));
    return resourceNames.size() - 1;
  }

  size_t addResourceName(const char *name) { return addResourceName({name, strlen(name)}); }

public:
  void recordTexture(Image *texture, SubresourceIndex sub_res, D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state,
    UsageEntryType what)
  {
    auto ref = eastl::find_if(rbegin(textureUsageEntries), rend(textureUsageEntries),
      [=](const TextureUsageEntry &e) //
      { return texture == e.texture && sub_res == e.subRes; });

    auto currentSegmentIndex = segments.size() - 1;
    size_t nameIndex;
    if (ref != rend(textureUsageEntries))
    {
      // don't record the same thing multiple times
      if (is_mergable(what) && ref->what == what && ref->segmentIndex == currentSegmentIndex && ref->expectedState == expected_state)
      {
        return;
      }
      ref->nextEntry = textureUsageEntries.size();
      nameIndex = ref->nameIndex;
    }
    else
    {
      texture->getDebugName([this, &nameIndex](auto &name) { nameIndex = addResourceName(name); });
    }
    TextureUsageEntry e;
    e.texture = texture;
    e.subRes = sub_res;
    e.state = current_state;
    e.expectedState = expected_state;
    e.nextEntry = ~size_t(0);
    e.segmentIndex = currentSegmentIndex;
    e.nameIndex = nameIndex;
    e.fmt = texture->getFormat();
    e.what = what;
    textureUsageEntries.push_back(e);
  }

  void recordBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state,
    UsageEntryType what)
  {
    auto ref = eastl::find_if(rbegin(bufferUsageEntries), rend(bufferUsageEntries),
      [buffer](const BufferUsageEntry &e) { return buffer == e.buffer; });

    auto currentSegmentIndex = segments.size() - 1;
    size_t nameIndex;
    if (ref != rend(bufferUsageEntries))
    {
      // don't record the same thing multiple times
      if (is_mergable(what) && ref->what == what && ref->segmentIndex == currentSegmentIndex && ref->expectedState == expected_state)
      {
        return;
      }
      ref->nextEntry = bufferUsageEntries.size();
      nameIndex = ref->nameIndex;
    }
    else
    {
      char buf[MAX_OBJECT_NAME_LENGTH];
      nameIndex = addResourceName(get_resource_name(buffer.buffer, buf));
    }
    BufferUsageEntry e;
    e.buffer = buffer;
    e.state = current_state;
    e.expectedState = expected_state;
    e.nextEntry = ~size_t(0);
    e.segmentIndex = currentSegmentIndex;
    e.nameIndex = nameIndex;
    e.what = what;
    bufferUsageEntries.push_back(e);
  }

  void beginEvent(eastl::span<const char> name)
  {
    if (segments.empty())
    {
      segments.emplace_back(begin(name), end(name) - 1);
    }
    else
    {
      auto place = segments.push_back_uninitialized();
      auto &last = *(segments.crbegin() + 1);
      auto &s = *::new (place) eastl::string(eastl::string::CtorDoNotInitialize{}, last.size() + name.size());
      s.append(last);
      s.push_back('/');
      s.append(begin(name), end(name) - 1);
    }
  }

  void endEvent()
  {
    if (segments.empty())
    {
      return;
    }
    auto place = segments.push_back_uninitialized();
    auto &last = *(segments.crbegin() + 1);
    if (auto at = last.find_last_of('/'); at != eastl::string::npos)
    {
      ::new (place) eastl::string(last, 0, at);
    }
    else
    {
      ::new (place) eastl::string();
    }
  }

  void marker(eastl::span<const char> name) { G_UNUSED(name); }

  void clear()
  {
    segments.clear();
    textureUsageEntries.clear();
    bufferUsageEntries.clear();
  }

  // Walks the usage chain for each texture subresource
  // NOTE resources may already be no longer valid, so do not try to access the objects!
  template <typename T>
  void walkTextures(T walker)
  {
    for (auto &e : textureUsageEntries)
    {
      if (nullptr == e.texture)
      {
        continue;
      }
      if (!visit(e, walker))
      {
        continue;
      }
      for (auto n = next(&e); n; n = next(n))
      {
        if (!visit(*n, walker))
        {
          break;
        }
        // marks this entry as visited
        n->texture = nullptr;
      }
    }
    for (auto &e : textureUsageEntries)
    {
      auto n = next(&e);
      if (n)
      {
        // remove visited mark
        n->texture = e.texture;
      }
    }
  }

  // Iterates over all texture tracking entries, in order as they where added to the set, which
  // groups them by segment implicitly
  // NOTE resources may already be no longer valid, so do not try to access the objects!
  template <typename T>
  void iterateTextures(T iter)
  {
    for (auto &e : textureUsageEntries)
    {
      if (!visit(e, iter))
      {
        return;
      }
    }
  }

  // Walks the usage chain for each buffer resource
  // NOTE resources may already be no longer valid, so do not try to access the objects!
  template <typename T>
  void walkBuffers(T walker)
  {
    for (auto &e : bufferUsageEntries)
    {
      if (nullptr == e.buffer.buffer)
      {
        continue;
      }
      if (!visit(e, walker))
      {
        continue;
      }
      for (auto n = next(&e); n; n = next(n))
      {
        if (!visit(*n, walker))
        {
          break;
        }
        // marks this entry as visited
        n->buffer.buffer = nullptr;
      }
    }
    for (auto &e : bufferUsageEntries)
    {
      auto n = next(&e);
      if (n)
      {
        // remove visited mark
        n->buffer.buffer = e.buffer.buffer;
      }
    }
  }

  // Iterates over all buffer tracking entries, in order as they where added to the set, which
  // groups them by segment implicitly
  // NOTE resources may already be no longer valid, so do not try to access the objects!
  template <typename T>
  void iterateBuffers(T iter)
  {
    for (auto &e : bufferUsageEntries)
    {
      if (!visit(e, iter))
      {
        return;
      }
    }
  }

  size_t segmentCount() const { return segments.size(); }

  size_t textureRecordCount() const { return textureUsageEntries.size(); }

  size_t bufferRecordCount() const { return bufferUsageEntries.size(); }

  size_t memoryUse() const
  {
    size_t s = 0;
    for (auto &e : segments)
    {
      s += e.capacity() + 1;
    }
    for (auto &n : resourceNames)
    {
      s += n.capacity() + 1;
    }
    s += textureUsageEntries.size() * sizeof(textureUsageEntries[0]);
    s += bufferUsageEntries.size() * sizeof(bufferUsageEntries[0]);
    return s;
  }
};

#if DX12_RESOURCE_USAGE_TRACKER
class ResourceUsageHistoryDataSetDebugger
{
  uint32_t framesToRecord = 0;
  uint32_t processedHistoryCount = 0;
  bool recordHistory = false;
  bool processHistory = false;

  struct FrameDataSet
  {
    dag::Vector<ResourceUsageHistoryDataSet> dataSet;
  };

  struct TextureInfo
  {
    eastl::string name;
    Image *texture;
    FormatStore fmt;
    SubresourceIndex subRes;
    uint32_t entries;
    uint32_t neededTransitionCount;
  };

  struct BufferInfo
  {
    eastl::string name;
    BufferResourceReference buffer;
    uint32_t entries;
    uint32_t neededTransitionCount;
  };

  enum class AnalyzerMode
  {
    UNIQUE_MISSING_BARRIERS,
    UNIQUE_USES,
    ALL_MISSING_BARRIERS,
    ALL_USES,
  };

  enum class AnalyzedIssueType
  {
    OKAY,
    MISSING_BARRIER,
    SUPERFLUOUS_BARRIER,
  };

  struct AnalyzedEntry
  {
    D3D12_RESOURCE_STATES currentState;
    ResourceBarrier expectedState;
    eastl::string_view segment;
    ResourceUsageHistoryDataSet::UsageEntryType what;
    bool isAutomaticTransition;
    eastl::string_view whatString;
    uint32_t count;
    AnalyzedIssueType issueType;
  };

  WinCritSec mutex;
  dag::Vector<FrameDataSet> dataSets;
  dag::Vector<TextureInfo> textures;
  dag::Vector<BufferInfo> buffers;
  dag::Vector<AnalyzedEntry> analyzedTextureEntries;
  dag::Vector<AnalyzedEntry> analyzedBufferEntries;
  FrameDataSet currentFrameDataSet;
  AnalyzerMode inspectinTextureMode = AnalyzerMode::UNIQUE_MISSING_BARRIERS;
  AnalyzerMode inspectinBufferMode = AnalyzerMode::UNIQUE_MISSING_BARRIERS;
  size_t inspectingTexture = ~size_t(0);
  size_t inspectingBuffer = ~size_t(0);
  size_t inspectingTextureProgress = 0;
  size_t inspectingBufferProgress = 0;

  static const char *to_string(AnalyzerMode mode)
  {
    switch (mode)
    {
      case AnalyzerMode::UNIQUE_MISSING_BARRIERS: return "Unique Missing Barriers";
      case AnalyzerMode::UNIQUE_USES: return "Unique Uses";
      case AnalyzerMode::ALL_MISSING_BARRIERS: return "All Missing Barriers";
      case AnalyzerMode::ALL_USES: return "All Uses";
    }
    return "Error";
  }

  static AnalyzerMode drawAnalyzerModeSelector(AnalyzerMode mode);
  void addAnalyzedTextureInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state, eastl::string_view segment,
    ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition, eastl::string_view what_string,
    AnalyzedIssueType issue_type);
  void addUniqueAnalyzedTextureInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state, eastl::string_view segment,
    ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition, eastl::string_view what_string,
    AnalyzedIssueType issue_type);
  void addAnalyzedBufferInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state, eastl::string_view segment,
    ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition, eastl::string_view what_string,
    AnalyzedIssueType issue_type);
  void addUniqueAnalyzedBufferInfo(D3D12_RESOURCE_STATES current_state, ResourceBarrier expected_state, eastl::string_view segment,
    ResourceUsageHistoryDataSet::UsageEntryType what, bool is_automatic_transition, eastl::string_view what_string,
    AnalyzedIssueType issue_type);
  void addTextureInfo(eastl::string_view name, Image *texture, SubresourceIndex sub_res, FormatStore fmt, bool needs_transition);
  void addBufferInfo(eastl::string_view name, BufferResourceReference buffer, bool needs_transition);
  void processDataSet(FrameDataSet &data_set);
  void processDataSets();

public:
  // If enabled and frame_limit is set to 0, then there is no limit on how many frames are to be recorded
  void configure(bool enable_recording, uint32_t frame_limit)
  {
    WinAutoLock lock{mutex};
    recordHistory = enable_recording;
    framesToRecord = frame_limit;
  }
  void add(const ResourceUsageHistoryDataSet &set)
  {
    WinAutoLock lock{mutex};
    currentFrameDataSet.dataSet.push_back(set);
  }
  void finishFrame()
  {
    if (!shouldRecordResourceUsage())
    {
      return;
    }
    WinAutoLock lock{mutex};
    dataSets.push_back(eastl::move(currentFrameDataSet));
    if (framesToRecord > 0)
    {
      --framesToRecord;
    }
    else
    {
      recordHistory = false;
    }
  }
  bool shouldRecordResourceUsage() const { return recordHistory; }
  void debugOverlay();
};

class ResourceUsageManagerWithHistory : protected ResourceUsageManager
{
  using BaseType = ResourceUsageManager;
  using UsageEntryType = ResourceUsageHistoryDataSet::UsageEntryType;

  ResourceUsageHistoryDataSet historicalData;
  bool recordData = false;

  void recordTexture(Image *texture, SubresourceIndex sub_res, ResourceBarrier expected_state, UsageEntryType what)
  {
    historicalData.recordTexture(texture, sub_res, currentTextureState(texture, sub_res), expected_state, what);
  }

  void recordBuffer(BufferResourceReference buffer, ResourceBarrier expected_state, UsageEntryType what)
  {
    historicalData.recordBuffer(buffer, currentbufferState(buffer), expected_state, what);
  }

  bool shouldRecordData(Image *texture)
  {
    G_UNUSED(texture);
    if (!recordData)
    {
      return false;
    }
    return texture->isGPUChangeable();
  }

  bool shouldRecordData(BufferResourceReference buffer)
  {
    if (!recordData)
    {
      return false;
    }
    return static_cast<bool>(buffer.resourceId) && buffer.resourceId.isUsedAsUAVBuffer();
  }

public:
  using BaseType::currentTextureState;
  using BaseType::flushPendingUAVActions;
  using BaseType::setTextureState;
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
  using BaseType::resetBufferState;
#endif
  using BaseType::useScratchAsCopyDestination;
  using BaseType::useScratchAsCopySource;

  bool beginImplicitUAVAccess(ID3D12Resource *res) { return BaseType::beginImplicitUAVAccess(res); }

  void implicitUAVFlushAll(const char *where, bool report_user_barriers)
  {
    BaseType::implicitUAVFlushAll(where, report_user_barriers);
  }

  // buffers
  void useBufferAsCBV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      ResourceBarrier barrier = RB_RO_CONSTANT_BUFFER;
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::CBV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::CBV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::CBV_VS;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      recordBuffer(buffer, barrier, what);
    }
    BaseType::useBufferAsCBV(barriers, stage, buffer);
  }

  void useBufferAsSRV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      ResourceBarrier barrier = RB_RO_SRV;
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::SRV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::SRV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::SRV_VS;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      recordBuffer(buffer, barrier, what);
    }
    BaseType::useBufferAsSRV(barriers, stage, buffer);
  }

  void useBufferAsUAV(BarrierBatcher &barriers, uint32_t stage, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      ResourceBarrier barrier = RB_RW_UAV;
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::UAV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::UAV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_VS;
          break;
        case STAGE_ANY:
          barrier = barrier | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_ANY;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      recordBuffer(buffer, barrier, what);
    }
    BaseType::useBufferAsUAV(barriers, stage, buffer);
  }

  void useBufferAsIB(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RO_INDEX_BUFFER, UsageEntryType::INDEX_BUFFER);
    }
    BaseType::useBufferAsIB(barriers, buffer);
  }

  void useBufferAsVB(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RO_VERTEX_BUFFER, UsageEntryType::VERTEX_BUFFER);
    }
    BaseType::useBufferAsVB(barriers, buffer);
  }

  void useBufferAsRTASBuildSource(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE, UsageEntryType::RT_AS_BUID_SOURCE);
    }
    BaseType::useBufferAsRTASBuildSource(barriers, buffer);
  }

  void useBufferAsIA(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RO_INDIRECT_BUFFER, UsageEntryType::INDIRECT_BUFFER);
    }
    BaseType::useBufferAsIA(barriers, buffer);
  }

  void useBufferAsCopySource(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RO_COPY_SOURCE, UsageEntryType::COPY_SOURCE);
    }
    BaseType::useBufferAsCopySource(barriers, buffer);
  }

  void useBufferAsCopyDestination(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RW_COPY_DEST, UsageEntryType::COPY_DESTINATION);
    }
    BaseType::useBufferAsCopyDestination(barriers, buffer);
  }

  void useBufferAsUAVForClear(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_RW_UAV, UsageEntryType::UAV_CLEAR);
    }
    BaseType::useBufferAsUAVForClear(barriers, buffer);
  }

  void useBufferAsQueryResultDestination(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    // driver internal buffer, nothing to record
    BaseType::useBufferAsQueryResultDestination(barriers, buffer);
  }

  void userBufferTransitionBarrier(BarrierBatcher &barriers, BufferResourceReference buffer, ResourceBarrier barrier)
  {
    BaseType::userBufferTransitionBarrier(barriers, buffer, barrier);
  }

  void useBufferAsPredication(BarrierBatcher &barriers, BufferResourceReference buffer)
  {
    // driver internal buffer, nothing to record
    BaseType::useBufferAsPredication(barriers, buffer);
  }

  void userBufferUAVAccessFlush(BufferResourceReference buffer)
  {
    if (shouldRecordData(buffer))
    {
      recordBuffer(buffer, RB_FLUSH_UAV, UsageEntryType::USER_FLUSH_UAV);
    }
    BaseType::userBufferUAVAccessFlush(buffer);
  }

  void userBufferUAVAccessFlushSkip(BufferResourceReference buffer) { BaseType::userBufferUAVAccessFlushSkip(buffer); }

  // textures
  void useTextureAsSRV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ImageViewState view,
    bool as_const_ds)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      ResourceBarrier barrier = RB_RO_SRV;
      if (as_const_ds)
      {
        barrier = barrier | RB_RO_CONSTANT_DEPTH_STENCIL_TARGET;
      }
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::SRV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::SRV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::SRV_VS;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(), [=](auto sub_res) {
        if (view.getPlaneIndex().index() > 0)
        {
          sub_res += texture->getSubresourcesPerPlane();
        }
        recordTexture(texture, sub_res, barrier, what);
      });
    }
    BaseType::useTextureAsSRV(barriers, stt, stage, texture, view, as_const_ds);
  }

  void useTextureAsUAV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ArrayLayerRange arrays,
    MipMapRange mips)
  {
    if (shouldRecordData(texture))
    {
      ResourceBarrier barrier = RB_RW_UAV;
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::UAV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::UAV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_VS;
          break;
        case STAGE_ANY:
          barrier = barrier | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_ANY;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
      {
        for (auto m : mips)
        {
          recordTexture(texture, SubresourceIndex::make(m.index()), barrier, what);
        }
      }
      else
      {
        for (auto a : arrays)
        {
          for (auto m : mips)
          {
            recordTexture(texture, calculate_subresource_index(m, a, texture->getMipLevelRange()), barrier, what);
          }
        }
      }
    }
    BaseType::useTextureAsUAV(barriers, stt, stage, texture, arrays, mips);
  }

  void useTextureAsUAV(BarrierBatcher &barriers, SplitTransitionTracker &stt, uint32_t stage, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      ResourceBarrier barrier = RB_RW_UAV;
      UsageEntryType what = UsageEntryType::INVALID_VALUE;
      switch (stage)
      {
        case STAGE_CS:
          barrier = barrier | RB_STAGE_COMPUTE;
          what = UsageEntryType::UAV_CS;
          break;
        case STAGE_PS:
          barrier = barrier | RB_STAGE_PIXEL;
          what = UsageEntryType::UAV_PS;
          break;
        case STAGE_VS:
          barrier = barrier | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_VS;
          break;
        case STAGE_ANY:
          barrier = barrier | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
          what = UsageEntryType::UAV_ANY;
          break;
        default: DAG_FATAL("DX12: Invalid shader stage %u", stage); break;
      }
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) { recordTexture(texture, sub_res, barrier, what); });
    }
    BaseType::useTextureAsUAV(barriers, stt, stage, texture, view);
  }

  void readyTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    // Already tracked by beginReadyTextureAsReadBack
    BaseType::readyTextureAsReadBack(barriers, stt, texture, sub_res);
  }

  void beginReadyTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    if (shouldRecordData(texture))
    {
      recordTexture(texture, sub_res, RB_NONE, UsageEntryType::READ_BACK_DEFERRED_BEGIN);
    }
    BaseType::beginReadyTextureAsReadBack(barriers, stt, texture, sub_res);
  }

  void useTextureAsReadBack(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    if (shouldRecordData(texture))
    {
      recordTexture(texture, sub_res, RB_RO_COPY_SOURCE, UsageEntryType::READ_BACK);
    }
    BaseType::useTextureAsReadBack(barriers, stt, texture, sub_res);
  }

  void useTextureAsSwapImageOutput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    // TODO: should we track? Its a driver internal thing...
    BaseType::useTextureAsSwapImageOutput(barriers, stt, texture);
  }

  void useTextureAsDSVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    const auto subRes = texture->mipAndLayerIndexToStateIndex(mip_level, array_layer);
    recordTexture(texture, subRes, RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE0_CLEAR);
    if (texture->getPlaneCount().count() > 1)
    {
      recordTexture(texture, subRes + texture->getSubresourcesPerPlane(), RB_RW_DEPTH_STENCIL_TARGET,
        UsageEntryType::DSV_PLANE1_CLEAR);
    }
    BaseType::useTextureAsDSVForClear(barriers, stt, texture, array_layer, mip_level);
  }

  void useTextureAsDSVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(), [=](auto sub_res) {
        recordTexture(texture, sub_res, RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE0_CLEAR);
        if (texture->getPlaneCount().count() > 1)
        {
          recordTexture(texture, sub_res + texture->getSubresourcesPerPlane(), RB_RW_DEPTH_STENCIL_TARGET,
            UsageEntryType::DSV_PLANE1_CLEAR);
        }
      });
    }
    BaseType::useTextureAsDSVForClear(barriers, stt, texture, view);
  }

  void useTextureAsRTVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    const auto subRes = texture->mipAndLayerIndexToStateIndex(mip_level, array_layer);
    recordTexture(texture, subRes, RB_RW_RENDER_TARGET, UsageEntryType::RTV_CLEAR);
    BaseType::useTextureAsRTVForClear(barriers, stt, texture, array_layer, mip_level);
  }

  void useTextureAsRTVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) { recordTexture(texture, sub_res, RB_RW_RENDER_TARGET, UsageEntryType::RTV_CLEAR); });
    }
    BaseType::useTextureAsRTVForClear(barriers, stt, texture, view);
  }

  void useTextureAsCopySourceForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RO_COPY_SOURCE, UsageEntryType::COPY_SOURCE_ALL);
      }
    }
    BaseType::useTextureAsCopySourceForWholeCopy(barriers, stt, texture);
  }

  void useTextureAsCopySourceForWholeCopyOnReadbackQueue(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RO_COPY_SOURCE, UsageEntryType::COPY_SOURCE_ALL);
      }
    }
    BaseType::useTextureAsCopySourceForWholeCopyOnReadbackQueue(barriers, stt, texture);
  }

  void useTextureAsCopyDestinationForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    G_ASSERT(texture->hasTrackedState());
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RW_COPY_DEST, UsageEntryType::COPY_DESTINATION_ALL);
      }
    }
    BaseType::useTextureAsCopyDestinationForWholeCopy(barriers, stt, texture);
  }

  void useTextureAsCopyDestinationForWholeCopyOnReadbackQueue(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RW_COPY_DEST, UsageEntryType::COPY_DESTINATION_ALL);
      }
    }
    BaseType::useTextureAsCopyDestinationForWholeCopyOnReadbackQueue(barriers, stt, texture);
  }

  void finishUseTextureAsCopyDestinationForWholeCopy(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    // for textures where shouldRecordData would return true, this is a noop, so nothing to track
    BaseType::finishUseTextureAsCopyDestinationForWholeCopy(barriers, stt, texture);
  }

  void useTextureAsCopySource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      recordTexture(texture, sub_res, RB_RO_COPY_SOURCE, UsageEntryType::COPY_SOURCE);
      if (texture->getPlaneCount().count() > 1)
      {
        recordTexture(texture, sub_res + texture->getSubresourcesPerPlane(), RB_RO_COPY_SOURCE, UsageEntryType::COPY_SOURCE_PLANE1);
      }
    }
    BaseType::useTextureAsCopySource(barriers, stt, texture, sub_res);
  }

  void useTextureAsCopyDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    if (shouldRecordData(texture))
    {
      recordTexture(texture, sub_res, RB_RW_COPY_DEST, UsageEntryType::COPY_DESTINATION);
      if (texture->getPlaneCount().count() > 1)
      {
        recordTexture(texture, sub_res + texture->getSubresourcesPerPlane(), RB_RW_COPY_DEST, UsageEntryType::COPY_DESTINATION_PLANE1);
      }
    }
    BaseType::useTextureAsCopyDestination(barriers, stt, texture, sub_res);
  }

  void finishUseTextureAsCopyDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    // for textures where shouldRecordData would return true, this is a noop, so nothing to track
    BaseType::finishUseTextureAsCopyDestination(barriers, stt, texture, sub_res);
  }

  // TODO: Similar to present (should we track? Its a driver internal thing...)
  void useTextureAsResolveSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    BaseType::useTextureAsResolveSource(barriers, stt, texture);
  }

  void useTextureAsResolveDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    BaseType::useTextureAsResolveDestination(barriers, stt, texture);
  }

  void useTextureAsBlitSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) { recordTexture(texture, sub_res, RB_RO_BLIT_SOURCE, UsageEntryType::BLIT_SOURCE); });
    }
    BaseType::useTextureAsBlitSource(barriers, stt, texture, view);
  }

  void useTextureAsBlitDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) { recordTexture(texture, sub_res, RB_RW_BLIT_DEST, UsageEntryType::BLIT_DESTINATION); });
    }
    BaseType::useTextureAsBlitDestination(barriers, stt, texture, view);
  }

  void useTextureAsRTV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerRange arrays,
    MipMapIndex mip_level)
  {
    if (shouldRecordData(texture))
    {
      if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
      {
        recordTexture(texture, SubresourceIndex::make(mip_level.index()), RB_RW_RENDER_TARGET, UsageEntryType::RTV);
      }
      else
      {
        for (auto a : arrays)
        {
          recordTexture(texture, texture->mipAndLayerIndexToStateIndex(mip_level, a), RB_RW_RENDER_TARGET, UsageEntryType::RTV);
        }
      }
    }
    BaseType::useTextureAsRTV(barriers, stt, texture, arrays, mip_level);
  }

  void useTextureAsRTV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) { recordTexture(texture, sub_res, RB_RW_RENDER_TARGET, UsageEntryType::RTV); });
    }
    BaseType::useTextureAsRTV(barriers, stt, texture, view);
  }

  void useTextureAsDSV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerIndex array_layer,
    MipMapIndex mip_level)
  {
    if (shouldRecordData(texture))
    {
      const auto subRes = texture->mipAndLayerIndexToStateIndex(mip_level, array_layer);
      recordTexture(texture, subRes, RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE0);
      if (texture->getPlaneCount().count() > 1)
      {
        recordTexture(texture, subRes + texture->getSubresourcesPerPlane(), RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE1);
      }
    }
    BaseType::useTextureAsDSV(barriers, stt, texture, array_layer, mip_level);
  }

  void useTextureAsDSV(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(), [=](auto sub_res) {
        recordTexture(texture, sub_res, RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE0);
        if (texture->getPlaneCount().count() > 1)
        {
          recordTexture(texture, sub_res + texture->getSubresourcesPerPlane(), RB_RW_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE1);
        }
      });
    }
    BaseType::useTextureAsDSV(barriers, stt, texture, view);
  }

  void useTextureAsDSVConst(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(), [=](auto sub_res) {
        recordTexture(texture, sub_res, RB_RO_CONSTANT_DEPTH_STENCIL_TARGET, UsageEntryType::DSV_PLANE0_CONST);
        if (texture->getPlaneCount().count() > 1)
        {
          recordTexture(texture, sub_res + texture->getSubresourcesPerPlane(), RB_RO_CONSTANT_DEPTH_STENCIL_TARGET,
            UsageEntryType::DSV_PLANE1_CONST);
        }
      });
    }
    BaseType::useTextureAsDSVConst(barriers, stt, texture, view);
  }

  void useTextureAsUAVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ArrayLayerRange arrays,
    MipMapRange mips)
  {
    if (D3D12_RESOURCE_DIMENSION_TEXTURE3D == texture->getType())
    {
      for (auto m : mips)
      {
        recordTexture(texture, SubresourceIndex::make(m.index()), RB_RW_UAV, UsageEntryType::UAV_CLEAR);
      }
    }
    else
    {
      for (auto a : arrays)
      {
        for (auto m : mips)
        {
          recordTexture(texture, calculate_subresource_index(m, a, texture->getMipLevelRange()), RB_RW_UAV, UsageEntryType::UAV_CLEAR);
        }
      }
    }
    BaseType::useTextureAsUAVForClear(barriers, stt, texture, arrays, mips);
  }

  void useTextureAsUAVForClear(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, ImageViewState view)
  {
    if (shouldRecordData(texture))
    {
      view.iterateSubresources(texture->getType(), texture->getMipLevelRange(),
        [=](auto sub_res) //
        { recordTexture(texture, sub_res, RB_RW_UAV, UsageEntryType::UAV_CLEAR); });
    }
    BaseType::useTextureAsUAVForClear(barriers, stt, texture, view);
  }

#if !_TARGET_XBOXONE
  void useTextureAsVRSSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RO_VARIABLE_RATE_SHADING_TEXTURE, UsageEntryType::VARIABLE_RATE_SHADING);
      }
    }
    BaseType::useTextureAsVRSSource(barriers, stt, texture);
  }
#endif

  void useTextureAsDLSSInput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RO_SRV, UsageEntryType::DLSS_INPUT);
      }
    }
    BaseType::useTextureAsDLSSInput(barriers, stt, texture);
  }

  void useTextureAsDLSSOutput(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_RW_UAV, UsageEntryType::DLSS_OUTPUT);
      }
    }
    BaseType::useTextureAsDLSSOutput(barriers, stt, texture);
  }

  void useTextureAsUploadDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, SubresourceIndex sub_res)
  {
    G_ASSERT(texture->hasTrackedState());
    if (shouldRecordData(texture))
    {
      recordTexture(texture, sub_res, RB_RW_COPY_DEST, UsageEntryType::UPLOAD_DESTINATION);
    }
    BaseType::useTextureAsUploadDestination(barriers, stt, texture, sub_res);
  }

  void finishUseTextureAsUploadDestination(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceIndex sub_res)
  {
    // for textures where shouldRecordData would return true, this is a noop, so nothing to track
    BaseType::finishUseTextureAsUploadDestination(barriers, stt, texture, sub_res);
  }

  void useTextureToAliasFrom(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, uint32_t tex_flags)
  {
    BaseType::useTextureToAliasFrom(barriers, stt, texture, tex_flags);
  }

  void useTextureToAliasToAndDiscard(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, uint32_t tex_flags)
  {
    BaseType::useTextureToAliasToAndDiscard(barriers, stt, texture, tex_flags);
  }

  void userTextureTransitionSplitBarrierBegin(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    if (shouldRecordData(texture))
    {
      auto planes = texture->getPlaneCount();
      auto planeWidth = texture->getSubresourcesPerPlane();

      for (auto i : sub_res_range)
      {
        recordTexture(texture, i, barrier, UsageEntryType::USER_BARRIER);
      }
      if (planes.count() > 1)
      {
        for (auto i : sub_res_range)
        {
          recordTexture(texture, i + planeWidth, barrier, UsageEntryType::USER_BARRIER);
        }
      }
    }
    BaseType::userTextureTransitionSplitBarrierBegin(barriers, stt, texture, sub_res_range, barrier);
  }

  void userTextureTransitionSplitBarrierEnd(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    if (shouldRecordData(texture))
    {
      auto planes = texture->getPlaneCount();
      auto planeWidth = texture->getSubresourcesPerPlane();

      for (auto i : sub_res_range)
      {
        recordTexture(texture, i, barrier, UsageEntryType::USER_BARRIER);
      }
      if (planes.count() > 1)
      {
        for (auto i : sub_res_range)
        {
          recordTexture(texture, i + planeWidth, barrier, UsageEntryType::USER_BARRIER);
        }
      }
    }
    BaseType::userTextureTransitionSplitBarrierEnd(barriers, stt, texture, sub_res_range, barrier);
  }

  void userTextureTransitionBarrier(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture,
    SubresourceRange sub_res_range, ResourceBarrier barrier)
  {
    if (shouldRecordData(texture))
    {
      auto planes = texture->getPlaneCount();
      auto planeWidth = texture->getSubresourcesPerPlane();

      for (auto i : sub_res_range)
      {
        recordTexture(texture, i, barrier, UsageEntryType::USER_BARRIER);
      }
      if (planes.count() > 1)
      {
        for (auto i : sub_res_range)
        {
          recordTexture(texture, i + planeWidth, barrier, UsageEntryType::USER_BARRIER);
        }
      }
    }
    BaseType::userTextureTransitionBarrier(barriers, stt, texture, sub_res_range, barrier);
  }

  void beginTextureCPUAccess(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_NONE, UsageEntryType::CPU_ACCESS_BEGIN);
      }
    }
    BaseType::beginTextureCPUAccess(barriers, stt, texture);
  }

  void endTextureCPUAccess(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_NONE, UsageEntryType::CPU_ACCESS_END);
      }
    }
    BaseType::endTextureCPUAccess(barriers, stt, texture);
  }

  void useTextureAsMipGenSource(BarrierBatcher &barriers, SplitTransitionTracker &stt, Image *texture, MipMapIndex mip,
    ArrayLayerIndex ary)
  {
    if (shouldRecordData(texture))
    {
      recordTexture(texture, texture->mipAndLayerResourceIndex(mip, ary, FormatPlaneIndex::make(0)), RB_RO_BLIT_SOURCE,
        UsageEntryType::MIPGEN_SOURCE);
    }
    return BaseType::useTextureAsMipGenSource(barriers, stt, texture, mip, ary);
  }

  bool beginUseTextureAsUploadDestinationOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return BaseType::beginUseTextureAsUploadDestinationOnCopyQueue(bb, initial_state, texture, sub_res);
  }

  void finishUseTextureAsUploadDestinationOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    BaseType::finishUseTextureAsUploadDestinationOnCopyQueue(bb, texture, sub_res);
  }

  void userTextureUAVAccessFlush(Image *texture)
  {
    if (shouldRecordData(texture))
    {
      for (auto s : texture->getSubresourceRange())
      {
        recordTexture(texture, s, RB_FLUSH_UAV, UsageEntryType::USER_FLUSH_UAV);
      }
    }
    BaseType::userTextureUAVAccessFlush(texture);
  }

  void userTextureUAVAccessFlushSkip(Image *texture) { BaseType::userTextureUAVAccessFlushSkip(texture); }

  // misc

  void userUAVAccessFlush() { BaseType::userUAVAccessFlush(); }

  void useRTASAsUpdateSource(BarrierBatcher &bb, ID3D12Resource *as) { BaseType::useRTASAsUpdateSource(bb, as); }

  void useRTASAsBuildTarget(BarrierBatcher &bb, ID3D12Resource *as) { BaseType::useRTASAsBuildTarget(bb, as); }

  // Driver explicit barriers
  TransitionResult transitionBufferExplicit(BarrierBatcher &barriers, BufferResourceReference buffer, D3D12_RESOURCE_STATES state)
  {
    return BaseType::transitionBufferExplicit(barriers, buffer, state);
  }

  void onFlush(InititalResourceStateSet &initial_state)
  {
    historicalData.clear();
    BaseType::finishWorkItem(initial_state);
  }

  void beginEvent(eastl::span<const char> name)
  {
    if (!recordData)
    {
      return;
    }
    historicalData.beginEvent(name);
  }

  void endEvent()
  {
    if (!recordData)
    {
      return;
    }
    historicalData.endEvent();
  }

  void marker(eastl::span<const char> name)
  {
    if (!recordData)
    {
      return;
    }
    historicalData.marker(name);
  }

  void setRecordingState(bool enable_recoding) { recordData = enable_recoding; }

  void addTo(ResourceUsageHistoryDataSetDebugger &collection)
  {
    if (!recordData)
    {
      return;
    }
    collection.add(historicalData);
  }

  bool beginUseTextureAsMipTransferDestinationOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return BaseType::beginUseTextureAsMipTransferDestinationOnCopyQueue(bb, initial_state, texture, sub_res);
  }

  void finishUseTextureAsMipTransferDestinationOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    BaseType::finishUseTextureAsMipTransferDestinationOnCopyQueue(bb, texture, sub_res);
  }

  bool beginUseTextureAsMipTransferSourceOnCopyQueue(BarrierBatcher &bb, InititalResourceStateSet &initial_state, Image *texture,
    SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return true;
    }
    return BaseType::beginUseTextureAsMipTransferDestinationOnCopyQueue(bb, initial_state, texture, sub_res);
  }

  void finishUseTextureAsMipTransferSourceOnCopyQueue(BarrierBatcher &bb, Image *texture, SubresourceIndex sub_res)
  {
    if (!texture->hasTrackedState())
    {
      return;
    }
    BaseType::finishUseTextureAsMipTransferSourceOnCopyQueue(bb, texture, sub_res);
  }
};
#else
class ResourceUsageHistoryDataSetDebugger
{

public:
  void configure(bool, uint32_t) {}
  void finishFrame() {}
  constexpr bool shouldRecordResourceUsage() const { return false; }
  void debugOverlay() {}
};
class ResourceUsageManagerWithHistory : public ResourceUsageManager
{
  using BaseType = ResourceUsageManager;
  using UsageEntryType = ResourceUsageHistoryDataSet::UsageEntryType;

public:
  void onFlush(InititalResourceStateSet &initial_state) { BaseType::finishWorkItem(initial_state); }

  void beginEvent(eastl::span<const char>) {}

  void endEvent() {}

  void marker(eastl::span<const char>) {}

  void setRecordingState(bool) {}

  void addTo(ResourceUsageHistoryDataSetDebugger &) {}
};
#endif

class ResourceActivationTracker
{
  struct ResourceDeactivation
  {
    ID3D12Resource *res;
    ResourceMemory memory;
  };

  dag::Vector<ResourceDeactivation> deactivations;

public:
  void deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemory &memory)
  {
    ResourceDeactivation rd;
    rd.res = buffer.buffer;
    rd.memory = memory;
    deactivations.push_back(rd);
  }

  void deactivateTexture(Image *tex)
  {
    ResourceDeactivation rd;
    rd.res = tex->getHandle();
    rd.memory = tex->getMemory();
    deactivations.push_back(rd);
  }

  static bool activation_needs_barrier_flush(ResourceActivationAction action)
  {
    switch (action)
    {
      case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION:
      case ResourceActivationAction::REWRITE_AS_UAV:
      case ResourceActivationAction::REWRITE_AS_RTV_DSV: return false;
      case ResourceActivationAction::CLEAR_F_AS_UAV:
      case ResourceActivationAction::CLEAR_I_AS_UAV:
      case ResourceActivationAction::DISCARD_AS_UAV:
      case ResourceActivationAction::CLEAR_AS_RTV_DSV:
      case ResourceActivationAction::DISCARD_AS_RTV_DSV: return true;
    }
    return false;
  }

  void activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer, const ResourceMemory &memory,
    ResourceActivationAction action, const ResourceClearValue &value, ResourceUsageManagerWithHistory &rst, BarrierBatcher &bb,
    ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &descriptors, StatefulCommandBuffer &cmd)
  {
    auto beforeBatchSize = bb.batchSize();
    auto finder = [memory](auto &rd) //
    { return memory.intersectsWith(rd.memory); };

    for (auto at = begin(deactivations);;)
    {
      at = eastl::find_if(at, end(deactivations), finder);
      if (at == end(deactivations))
        break;
      bb.flushAlias(at->res, buffer.buffer);
      eastl::swap(*at, deactivations.back());
      deactivations.pop_back();
    }

    switch (action)
    {
      case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: rst.useBufferAsCopyDestination(bb, buffer); break;
      case ResourceActivationAction::REWRITE_AS_UAV:
      case ResourceActivationAction::DISCARD_AS_UAV: rst.useBufferAsUAV(bb, STAGE_ANY, buffer); break;
      case ResourceActivationAction::CLEAR_F_AS_UAV:
      case ResourceActivationAction::CLEAR_I_AS_UAV: rst.useBufferAsUAVForClear(bb, buffer); break;
      case ResourceActivationAction::REWRITE_AS_RTV_DSV:
      case ResourceActivationAction::CLEAR_AS_RTV_DSV:
      case ResourceActivationAction::DISCARD_AS_RTV_DSV:
        // invalid
        break;
    }

    if (beforeBatchSize != bb.batchSize() && activation_needs_barrier_flush(action))
    {
      bb.execute(cmd);
    }

    switch (action)
    {
      case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION:
      case ResourceActivationAction::REWRITE_AS_UAV:
        // user does a rewrite of the resources with either a copy or some shader
        break;
      case ResourceActivationAction::CLEAR_F_AS_UAV:
      case ResourceActivationAction::CLEAR_I_AS_UAV: //
      {
        auto index = descriptors.findInUAVScratchSegment(buffer.clearView);
        if (!index)
        {
          // reserve space for one UAV descriptor
          descriptors.reserveSpace(device, 0, 0, 1, 0, 0);
          index = descriptors.appendToUAVScratchSegment(device, buffer.clearView);
        }
        cmd.setResourceHeap(descriptors.getActiveHandle(), descriptors.getBindlessGpuAddress());
        if (ResourceActivationAction::CLEAR_F_AS_UAV == action)
        {
          cmd.clearUnorderedAccessViewFloat(descriptors.getGpuAddress(index), buffer.clearView, buffer.buffer, value.asFloat, 0,
            nullptr);
        }
        else
        {
          cmd.clearUnorderedAccessViewUint(descriptors.getGpuAddress(index), buffer.clearView, buffer.buffer, value.asUint, 0,
            nullptr);
        }
        break;
      }
      case ResourceActivationAction::DISCARD_AS_UAV: cmd.getHandle()->DiscardResource(buffer.buffer, nullptr); break;
      case ResourceActivationAction::REWRITE_AS_RTV_DSV:
      case ResourceActivationAction::CLEAR_AS_RTV_DSV:
      case ResourceActivationAction::DISCARD_AS_RTV_DSV:
        // invalid
        break;
    }
  }

  void activateTexture(Image *tex, ResourceActivationAction action, const ResourceClearValue &value, ImageViewState view_state,
    D3D12_CPU_DESCRIPTOR_HANDLE view, ResourceUsageManagerWithHistory &rst, BarrierBatcher &bb, SplitTransitionTracker &stt,
    ID3D12Device2 *device, ShaderResourceViewDescriptorHeapManager &descriptors, PipelineManager &pipeMan, StatefulCommandBuffer &cmd)
  {
    auto beforeBatchSize = bb.batchSize();
    // we do only the memory overlap, aliasing barrier and state barriers on mip level 0 and array
    // layer 0 for the whole resource
    if (0 == view_state.getMipBase().index() && 0 == view_state.getArrayBase().index())
    {
      auto finder = [memory = tex->getMemory()](auto &rd) //
      { return memory.intersectsWith(rd.memory); };

      for (auto at = begin(deactivations);;)
      {
        at = eastl::find_if(at, end(deactivations), finder);
        if (at == end(deactivations))
          break;
        bb.flushAlias(at->res, tex->getHandle());
        eastl::swap(*at, deactivations.back());
        deactivations.pop_back();
      }

      switch (action)
      {
        case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: rst.useTextureAsCopyDestinationForWholeCopy(bb, stt, tex); break;
        case ResourceActivationAction::REWRITE_AS_UAV:
        case ResourceActivationAction::DISCARD_AS_UAV:
          rst.useTextureAsUAV(bb, stt, STAGE_ANY, tex, tex->getArrayLayerRange(), tex->getMipLevelRange());
          break;
        case ResourceActivationAction::CLEAR_F_AS_UAV:
        case ResourceActivationAction::CLEAR_I_AS_UAV:
          rst.useTextureAsUAVForClear(bb, stt, tex, tex->getArrayLayerRange(), tex->getMipLevelRange());
          break;
        case ResourceActivationAction::REWRITE_AS_RTV_DSV:
        case ResourceActivationAction::DISCARD_AS_RTV_DSV:
          if (tex->getFormat().isColor())
          {
            for (auto m : tex->getMipLevelRange())
            {
              rst.useTextureAsRTV(bb, stt, tex, tex->getArrayLayerRange(), m);
            }
          }
          else
          {
            for (auto a : tex->getArrayLayerRange())
            {
              for (auto m : tex->getMipLevelRange())
              {
                rst.useTextureAsDSV(bb, stt, tex, a, m);
              }
            }
          }
          break;
        case ResourceActivationAction::CLEAR_AS_RTV_DSV:
          if (tex->getFormat().isColor())
          {
            for (auto a : tex->getArrayLayerRange())
            {
              for (auto m : tex->getMipLevelRange())
              {
                rst.useTextureAsRTVForClear(bb, stt, tex, a, m);
              }
            }
          }
          else
          {
            for (auto a : tex->getArrayLayerRange())
            {
              for (auto m : tex->getMipLevelRange())
              {
                rst.useTextureAsDSVForClear(bb, stt, tex, a, m);
              }
            }
          }
          break;
      }

      if (beforeBatchSize != bb.batchSize() && activation_needs_barrier_flush(action))
      {
        bb.execute(cmd);
      }
    }

    switch (action)
    {
      case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION:
      case ResourceActivationAction::REWRITE_AS_UAV:
      case ResourceActivationAction::REWRITE_AS_RTV_DSV:
        // user does a rewrite of the resources with either a copy or some shader
        break;
      case ResourceActivationAction::CLEAR_F_AS_UAV:
      case ResourceActivationAction::CLEAR_I_AS_UAV: //
      {
        auto index = descriptors.findInUAVScratchSegment(view);
        if (!index)
        {
          // reserve space for one UAV descriptor
          descriptors.reserveSpace(device, 0, 0, 1, 0, 0);
          index = descriptors.appendToUAVScratchSegment(device, view);
        }
        cmd.setResourceHeap(descriptors.getActiveHandle(), descriptors.getBindlessGpuAddress());
        if (ResourceActivationAction::CLEAR_F_AS_UAV == action)
        {
          cmd.clearUnorderedAccessViewFloat(descriptors.getGpuAddress(index), view, tex->getHandle(), value.asFloat, 0, nullptr);
        }
        else
        {
          cmd.clearUnorderedAccessViewUint(descriptors.getGpuAddress(index), view, tex->getHandle(), value.asUint, 0, nullptr);
        }
      }
      break;
      case ResourceActivationAction::CLEAR_AS_RTV_DSV:
        if (tex->getFormat().isColor())
        {
          // To save bit pattern for integer rt need to clear it with shader instead.
          if (tex->getFormat().isSampledAsFloat())
            cmd.getHandle()->ClearRenderTargetView(view, value.asFloat, 0, nullptr);
          else
          {
            cmd.setResourceHeap(descriptors.getActiveHandle(), descriptors.getBindlessGpuAddress());
            auto clearPipeline = pipeMan.getClearPipeline(device, tex->getFormat().asDxGiFormat(), true);

            if (!clearPipeline)
              return;

            DWORD dwordRegs[4]{value.asUint[0], value.asUint[1], value.asUint[2], value.asUint[3]};

            const Extent2D dstExtent = tex->getMipExtents2D(view_state.getMipBase());
            D3D12_RECT dstRect{0, 0, static_cast<LONG>(dstExtent.width), static_cast<LONG>(dstExtent.height)};
            cmd.clearExecute(pipeMan.getClearSignature(), clearPipeline, dwordRegs, view, dstRect);
          };
        }
        else
        {
          D3D12_CLEAR_FLAGS toBeCleared = D3D12_CLEAR_FLAG_DEPTH;
          if (!tex->getFormat().isDepth())
          {
            toBeCleared ^= D3D12_CLEAR_FLAG_DEPTH;
          }
          if (tex->getFormat().isStencil())
          {
            toBeCleared |= D3D12_CLEAR_FLAG_STENCIL;
          }
          cmd.getHandle()->ClearDepthStencilView(view, toBeCleared, value.asDepth, value.asStencil, 0, nullptr);
        }
        break;
      case ResourceActivationAction::DISCARD_AS_UAV:
      case ResourceActivationAction::DISCARD_AS_RTV_DSV: cmd.getHandle()->DiscardResource(tex->getHandle(), nullptr); break;
    }
  }

  void flushAll(BarrierBatcher &bb)
  {
    bb.flushAllUAV();
    deactivations.clear();
  }

  void reset() { deactivations.clear(); }
};

class BufferAccessTracker
{
  class CacheLineAllocator
  {
  public:
    EASTL_ALLOCATOR_EXPLICIT CacheLineAllocator(const char * = nullptr) {}
    CacheLineAllocator(const CacheLineAllocator &) = default;
    CacheLineAllocator(const CacheLineAllocator &, const char *) {}

    CacheLineAllocator &operator=(const CacheLineAllocator &) = default;

    void *allocate(size_t n, int = 0) { return ::operator new[](n, std::align_val_t{std::hardware_constructive_interference_size}); }
    void *allocate(size_t, size_t, size_t, int = 0) { return nullptr; }
    void deallocate(void *p, size_t) { ::operator delete[](p, std::align_val_t{}); }

    const char *get_name() const { return ""; }
    void set_name(const char *) {}

    bool operator==(const CacheLineAllocator &) { return true; }
    bool operator!=(const CacheLineAllocator &) { return false; }
  };

  eastl::array<eastl::bitvector<CacheLineAllocator>, FRAME_FRAME_BACKLOG_LENGTH> frame_accesses;
  uint8_t current_frame = 0;

public:
  BufferAccessTracker()
  {
    for (auto &frame_access : frame_accesses)
      frame_access.reserve(std::hardware_constructive_interference_size / sizeof(eastl::BitvectorWordType)); // reserve a full
                                                                                                             // cacheline
  }

  void updateLastFrameAccess(BufferGlobalId resourceId)
  {
    auto &frame_access = frame_accesses[current_frame];
    frame_access.set(resourceId.index(), true);
  }

  bool wasAccessedPreviousFrames(BufferGlobalId resourceId) const
  {
    for (auto &frame_access : frame_accesses)
      if (auto id = resourceId.index(); id < frame_access.size() && frame_access.at(id))
        return true;
    return false;
  }

  void advance()
  {
    current_frame = (current_frame + 1) % FRAME_FRAME_BACKLOG_LENGTH;
    frame_accesses[current_frame].clear();
  }
};
} // namespace drv3d_dx12