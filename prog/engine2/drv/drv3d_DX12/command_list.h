#pragma once

namespace drv3d_dx12
{
#define DX12_BEGIN_VALIATION() bool hadError = false
#define DX12_FINALIZE_VALIDATAION() \
  if (hadError)                     \
  {                                 \
    return;                         \
  }
#define DX12_RETURN_VALIDATION_RESULT() \
  {                                     \
    return hadError;                    \
  }
#define DX12_UPDATE_VALIATION_STATE(result) hadError = (result)
#define DX12_ON_VALIDATION_ERROR(...)                           \
  {                                                             \
    logerr("DX12: " DX12_VALIDATAION_CONTEXT ": " __VA_ARGS__); \
    DX12_UPDATE_VALIATION_STATE(true);                          \
  }
#define DX12_VALIDATE_CONDITION(cond, ...) \
  if (!(cond))                             \
  {                                        \
    DX12_ON_VALIDATION_ERROR(__VA_ARGS__); \
  }

#define DX12_VALIDATAION_CONTEXT "ResourceBarrier"

// Barrier validator, behavior is controlled with the policy type P.
template <typename P>
class BarrierValidator
{
public:
  static UINT getFormatLayers(DXGI_FORMAT format)
  {
    switch (format)
    {
        // most formats have only one layer
      default: return 1;
      // depth stencil formats have each a plane for depth and stencil
      case DXGI_FORMAT_R32G8X24_TYPELESS:
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      case DXGI_FORMAT_R24G8_TYPELESS:
        // planar video formats with Y and UV planes
      case DXGI_FORMAT_NV12:
      case DXGI_FORMAT_P010:
      case DXGI_FORMAT_P016:
      case DXGI_FORMAT_420_OPAQUE:
      case DXGI_FORMAT_NV11:
      case DXGI_FORMAT_P208: return 2;
    }
  }

  static UINT getSubResRange(const D3D12_RESOURCE_DESC &desc)
  {
    if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
    {
      return 1;
    }
    if ((D3D12_RESOURCE_DIMENSION_TEXTURE1D == desc.Dimension) || (D3D12_RESOURCE_DIMENSION_TEXTURE2D == desc.Dimension) ||
        (D3D12_RESOURCE_DIMENSION_TEXTURE3D == desc.Dimension))
    {
      UINT subResourceRange = desc.MipLevels;
      if (D3D12_RESOURCE_DIMENSION_TEXTURE3D != desc.Dimension)
      {
        subResourceRange *= desc.DepthOrArraySize;
      }
      subResourceRange *= getFormatLayers(desc.Format);
      return subResourceRange;
    }
    return 0;
  }

  static bool isValidBufferResourceState(const D3D12_RESOURCE_DESC &desc, D3D12_RESOURCE_STATES state)
  {
    if (state != (state & (P::VALID_BUFFER_READ_STATE_MASK | P::VALID_BUFFER_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK)))
    {
      return false;
    }

    if (state & P::VALID_UAV_STATE_MASK)
    {
      if (D3D12_RESOURCE_FLAG_NONE == (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS & desc.Flags))
      {
        return false;
      }
    }

    if (state & (P::VALID_BUFFER_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK))
    {
      if (state & (P::VALID_BUFFER_READ_STATE_MASK))
      {
        return false;
      }

      // Can only have one read bit set at a time
      if (__popcount(state & (P::VALID_BUFFER_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK)) > 1)
      {
        return false;
      }
    }
    return true;
  }

  static bool isValidTextureResourceState(const D3D12_RESOURCE_DESC &desc, D3D12_RESOURCE_STATES state)
  {
    if (state != (state & (P::VALID_TEXTURE_READ_STATE_MASK | P::VALID_TEXTURE_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK |
                            P::VALID_RTV_STATE_MASK | P::VALID_DSV_READ_STATE_MASK | P::VALID_DSV_WRITE_STATE_MASK)))
    {
      return false;
    }

    if (state & P::VALID_UAV_STATE_MASK)
    {
      if (D3D12_RESOURCE_FLAG_NONE == (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS & desc.Flags))
      {
        return false;
      }
    }

    if (state & P::VALID_RTV_STATE_MASK)
    {
      if (D3D12_RESOURCE_FLAG_NONE == (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & desc.Flags))
      {
        return false;
      }
    }

    if (state & (P::VALID_DSV_READ_STATE_MASK | P::VALID_DSV_WRITE_STATE_MASK))
    {
      if (D3D12_RESOURCE_FLAG_NONE == (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL & desc.Flags))
      {
        return false;
      }
    }

    if (
      state & (P::VALID_TEXTURE_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK | P::VALID_RTV_STATE_MASK | P::VALID_DSV_WRITE_STATE_MASK))
    {
      if (state & (P::VALID_TEXTURE_READ_STATE_MASK | P::VALID_DSV_READ_STATE_MASK))
      {
        return false;
      }

      // Can only have one read bit set at a time
      if (__popcount(state & (P::VALID_TEXTURE_WRITE_STATE_MASK | P::VALID_UAV_STATE_MASK | P::VALID_RTV_STATE_MASK |
                               P::VALID_DSV_WRITE_STATE_MASK)) > 1)
      {
        return false;
      }
    }

    return true;
  }

  static bool validateTransitionBarrier(D3D12_RESOURCE_BARRIER_FLAGS flags, const D3D12_RESOURCE_TRANSITION_BARRIER &transition)
  {
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(flags == (P::VALID_TRANSITION_FLAGS & flags), "Invalid barrier flags (0x%08X) for transition barrier",
      (uint32_t)flags);
    DX12_VALIDATE_CONDITION(transition.pResource != nullptr, "transition barrier resource was null");
    if (transition.pResource)
    {
      auto desc = transition.pResource->GetDesc();
      auto subResRange = getSubResRange(desc);
      DX12_VALIDATE_CONDITION(transition.Subresource < subResRange, "subresource index %u of transition barrier is out of range %u",
        transition.Subresource, subResRange);

      DX12_VALIDATE_CONDITION(transition.StateBefore != transition.StateAfter,
        "transition barriers StateBefore and StateAfter are identical %08X", transition.StateBefore);
      if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
      {
        DX12_VALIDATE_CONDITION(isValidBufferResourceState(desc, transition.StateBefore), "invalid buffer StateBefore states %08X",
          (uint32_t)transition.StateBefore);
        DX12_VALIDATE_CONDITION(isValidBufferResourceState(desc, transition.StateAfter), "invalid buffer StateAfter states %08X",
          (uint32_t)transition.StateAfter);
      }
      else if (D3D12_RESOURCE_DIMENSION_UNKNOWN != desc.Dimension)
      {
        DX12_VALIDATE_CONDITION(isValidTextureResourceState(desc, transition.StateBefore), "invalid texture StateBefore states %08X",
          (uint32_t)transition.StateBefore);
        DX12_VALIDATE_CONDITION(isValidTextureResourceState(desc, transition.StateAfter), "invalid texture StateAfter states %08X",
          (uint32_t)transition.StateAfter);
      }
    }
    DX12_RETURN_VALIDATION_RESULT();
  }
  static bool validateAliasingBarrier(D3D12_RESOURCE_BARRIER_FLAGS flags, const D3D12_RESOURCE_ALIASING_BARRIER &alias)
  {
    DX12_BEGIN_VALIATION();
    G_UNUSED(alias);
    // We can not validate any of the aliases as they can be null (which means all possible memory), or we can't query
    // any data that can be used to determine of they overlap or not.
    DX12_VALIDATE_CONDITION(0 == uint32_t(flags), "Invalid barrier flags (0x%08X) for aliasing barrier", (uint32_t)flags);
    DX12_RETURN_VALIDATION_RESULT();
  }
  static bool validateUAVBarrier(D3D12_RESOURCE_BARRIER_FLAGS flags, const D3D12_RESOURCE_UAV_BARRIER &uav)
  {
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 == uint32_t(flags), "Invalid barrier flags (0x%08X) for UAV barrier", (uint32_t)flags);
    // Null is valid here!
    if (uav.pResource)
    {
      // Only thing to validate is to see if the resource supports UAV access or not.
      auto desc = uav.pResource->GetDesc();
      DX12_VALIDATE_CONDITION(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS == (D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS & desc.Flags),
        "UAV flush barrier on a resource without UAV flag");
    }
    DX12_RETURN_VALIDATION_RESULT();
  }
  static bool validateBarrier(const D3D12_RESOURCE_BARRIER &barrier)
  {
    DX12_BEGIN_VALIATION();
    if (P::ALLOW_TRANSITION && D3D12_RESOURCE_BARRIER_TYPE_TRANSITION == barrier.Type)
    {
      DX12_UPDATE_VALIATION_STATE(validateTransitionBarrier(barrier.Flags, barrier.Transition));
    }
    else if (P::ALLOW_UAV && D3D12_RESOURCE_BARRIER_TYPE_UAV == barrier.Type)
    {
      DX12_UPDATE_VALIATION_STATE(validateUAVBarrier(barrier.Flags, barrier.UAV));
    }
    else if (P::ALLOW_ALIASING && D3D12_RESOURCE_BARRIER_TYPE_ALIASING == barrier.Type)
    {
      DX12_UPDATE_VALIATION_STATE(validateAliasingBarrier(barrier.Flags, barrier.Aliasing));
    }
    else
    {
      DX12_ON_VALIDATION_ERROR("Invalid barrier type %u", (uint32_t)barrier.Type);
    }
    DX12_RETURN_VALIDATION_RESULT();
  }
};

struct CopyCommandListBarrierValidationPolicy
{
  static constexpr bool ALLOW_TRANSITION = true;
  static constexpr bool ALLOW_UAV = false;
  static constexpr bool ALLOW_ALIASING = false;

  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_READ_STATE_MASK = D3D12_RESOURCE_STATE_COPY_SOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_WRITE_STATE_MASK = D3D12_RESOURCE_STATE_COPY_DEST;

  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_READ_STATE_MASK = D3D12_RESOURCE_STATE_COPY_SOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_WRITE_STATE_MASK = D3D12_RESOURCE_STATE_COPY_DEST;

  static constexpr D3D12_RESOURCE_STATES VALID_UAV_STATE_MASK = D3D12_RESOURCE_STATE_COMMON;
  static constexpr D3D12_RESOURCE_STATES VALID_RTV_STATE_MASK = D3D12_RESOURCE_STATE_COMMON;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_READ_STATE_MASK = D3D12_RESOURCE_STATE_COMMON;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_WRITE_STATE_MASK = D3D12_RESOURCE_STATE_COMMON;

  static constexpr D3D12_RESOURCE_BARRIER_FLAGS VALID_TRANSITION_FLAGS = D3D12_RESOURCE_BARRIER_FLAG_NONE;
};

struct ComputeCommandListBarrierValidationPolicy
{
  static constexpr bool ALLOW_TRANSITION = true;
  static constexpr bool ALLOW_UAV = true;
  static constexpr bool ALLOW_ALIASING = true;

  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_READ_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_BUFFER_READ_STATE_MASK | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
    D3D12_RESOURCE_STATE_PREDICATION | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_WRITE_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_BUFFER_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_READ_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_TEXTURE_READ_STATE_MASK | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_WRITE_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_TEXTURE_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_STATES VALID_UAV_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_UAV_STATE_MASK | D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  static constexpr D3D12_RESOURCE_STATES VALID_RTV_STATE_MASK = CopyCommandListBarrierValidationPolicy::VALID_RTV_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_READ_STATE_MASK = CopyCommandListBarrierValidationPolicy::VALID_DSV_READ_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_WRITE_STATE_MASK =
    CopyCommandListBarrierValidationPolicy::VALID_DSV_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_BARRIER_FLAGS VALID_TRANSITION_FLAGS =
    CopyCommandListBarrierValidationPolicy::VALID_TRANSITION_FLAGS;
};

struct BasicGraphicsCommandListBarrierValidationPolicy
{
  static constexpr bool ALLOW_TRANSITION = true;
  static constexpr bool ALLOW_UAV = true;
  static constexpr bool ALLOW_ALIASING = true;

  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_READ_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_BUFFER_READ_STATE_MASK | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
    D3D12_RESOURCE_STATE_INDEX_BUFFER;
  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_WRITE_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_BUFFER_WRITE_STATE_MASK | D3D12_RESOURCE_STATE_STREAM_OUT;

  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_READ_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_TEXTURE_READ_STATE_MASK | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_WRITE_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_TEXTURE_WRITE_STATE_MASK | D3D12_RESOURCE_STATE_RESOLVE_DEST |
    D3D12_RESOURCE_STATE_RESOLVE_SOURCE;

  static constexpr D3D12_RESOURCE_STATES VALID_UAV_STATE_MASK = ComputeCommandListBarrierValidationPolicy::VALID_UAV_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_RTV_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_RTV_STATE_MASK | D3D12_RESOURCE_STATE_RENDER_TARGET;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_READ_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_DSV_READ_STATE_MASK | D3D12_RESOURCE_STATE_DEPTH_READ;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_WRITE_STATE_MASK =
    ComputeCommandListBarrierValidationPolicy::VALID_DSV_WRITE_STATE_MASK | D3D12_RESOURCE_STATE_DEPTH_WRITE;

  static constexpr D3D12_RESOURCE_BARRIER_FLAGS VALID_TRANSITION_FLAGS =
    ComputeCommandListBarrierValidationPolicy::VALID_TRANSITION_FLAGS | D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY |
    D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
};

#if !_TARGET_XBOXONE
struct ExtendedGraphicsCommandListBarrierValidationPolicy
{
  static constexpr bool ALLOW_TRANSITION = true;
  static constexpr bool ALLOW_UAV = true;
  static constexpr bool ALLOW_ALIASING = true;

  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_READ_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_BUFFER_READ_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_BUFFER_WRITE_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_BUFFER_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_READ_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_TEXTURE_READ_STATE_MASK | D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
  static constexpr D3D12_RESOURCE_STATES VALID_TEXTURE_WRITE_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_TEXTURE_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_STATES VALID_UAV_STATE_MASK = BasicGraphicsCommandListBarrierValidationPolicy::VALID_UAV_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_RTV_STATE_MASK = BasicGraphicsCommandListBarrierValidationPolicy::VALID_RTV_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_READ_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_DSV_READ_STATE_MASK;
  static constexpr D3D12_RESOURCE_STATES VALID_DSV_WRITE_STATE_MASK =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_DSV_WRITE_STATE_MASK;

  static constexpr D3D12_RESOURCE_BARRIER_FLAGS VALID_TRANSITION_FLAGS =
    BasicGraphicsCommandListBarrierValidationPolicy::VALID_TRANSITION_FLAGS;
};
#endif

#undef DX12_VALIDATAION_CONTEXT

// Command list type has to be a template, as those types may be different for different use cases.
// Graphics and compute list types are layered on top of this list type and use different
// underlying command list types.
// T has to expose a VersionedPtr<Is...> or VersionedComPtr<Is...> for which Is... is a list of
// any of the ID3D12GraphicsCommandList versions.
template <typename T> //
class CopyCommandListImplementation
{
protected:
  T list;

  ComPtr<ID3D12Device> getDevice()
  {
    ComPtr<ID3D12Device> result;
    if (list)
    {
      list->GetDevice(COM_ARGS(&result));
    }
    return result;
  }

public:
  CopyCommandListImplementation() = default;
  CopyCommandListImplementation(const CopyCommandListImplementation &) = default;
  CopyCommandListImplementation(CopyCommandListImplementation &&) = default;
  CopyCommandListImplementation(T value) : list{value} {}

  ~CopyCommandListImplementation() = default;

  CopyCommandListImplementation &operator=(const CopyCommandListImplementation &) = default;
  CopyCommandListImplementation &operator=(CopyCommandListImplementation &&) = default;

  T release()
  {
    T value = list;
    list.reset();
    return value;
  }
  void reset() { list.reset(); }
  void reset(T value) { list = value; }
  explicit operator bool() const { return static_cast<bool>(list); }

  template <typename T>
  bool is() const
  {
    return list.template is<T>();
  }
  auto get() { return list.get(); }

  void copyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
    const D3D12_BOX *src_box)
  {
    list->CopyTextureRegion(dst, x, y, z, src, src_box);
  }

  void copyBufferRegion(ID3D12Resource *dst, UINT64 dst_offset, ID3D12Resource *src, UINT64 src_offset, UINT64 num_bytes)
  {
    list->CopyBufferRegion(dst, dst_offset, src, src_offset, num_bytes);
  }

  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers) { list->ResourceBarrier(num_barriers, barriers); }

  void copyResource(ID3D12Resource *dst, ID3D12Resource *src) { list->CopyResource(dst, src); }

  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index) { list->EndQuery(query_heap, type, index); }

  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
    list->ResolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }
};

template <typename T> //
class CopyCommandListParameterValidataion : public CopyCommandListImplementation<T>
{
  using BaseType = CopyCommandListImplementation<T>;

protected:
  using BaseType::getDevice;

  // returns 0 if a desc does not describe a buffer.
  static uint64_t getBufferSize(const D3D12_RESOURCE_DESC &desc)
  {
    if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
    {
      return desc.Width;
    }
    return 0;
  }
  // returns 0 in all dimensions if desc does not describe a texture.
  static Extent3D getExtentOfRes(const D3D12_RESOURCE_DESC &desc, UINT sub_res_index)
  {
    Extent3D ext{};
    switch (desc.Dimension)
    {
      case D3D12_RESOURCE_DIMENSION_UNKNOWN:
      case D3D12_RESOURCE_DIMENSION_BUFFER: //
        return ext;
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        ext.width = desc.Width;
        // 1d texture will report a height of 1
        ext.height = desc.Height;
        ext.depth = 1;
        break;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        ext.width = desc.Width;
        ext.height = desc.Height;
        ext.depth = desc.DepthOrArraySize;
        break;
    }

    auto mipIndex = getMipIndexFromSubResIndex(desc, sub_res_index);
    ext.width = max<uint32_t>(1, ext.width >> mipIndex);
    ext.height = max<uint32_t>(1, ext.height >> mipIndex);
    ext.depth = max<uint32_t>(1, ext.depth >> mipIndex);

    auto align = getFormatAlignment(desc.Format);
    ext = align_value(ext, align);

    return ext;
  }
  static UINT getMipRange(const D3D12_RESOURCE_DESC &desc)
  {
    if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
    {
      return 1;
    }
    if ((D3D12_RESOURCE_DIMENSION_TEXTURE1D == desc.Dimension) || (D3D12_RESOURCE_DIMENSION_TEXTURE2D == desc.Dimension) ||
        (D3D12_RESOURCE_DIMENSION_TEXTURE3D == desc.Dimension))
    {
      return desc.MipLevels;
    }
    return 0;
  }
  // will return 0 if desc is invalid.
  static UINT getSubResRange(const D3D12_RESOURCE_DESC &desc)
  {
    if (D3D12_RESOURCE_DIMENSION_BUFFER == desc.Dimension)
    {
      return 1;
    }
    if ((D3D12_RESOURCE_DIMENSION_TEXTURE1D == desc.Dimension) || (D3D12_RESOURCE_DIMENSION_TEXTURE2D == desc.Dimension) ||
        (D3D12_RESOURCE_DIMENSION_TEXTURE3D == desc.Dimension))
    {
      UINT subResourceRange = desc.MipLevels;
      if (D3D12_RESOURCE_DIMENSION_TEXTURE3D != desc.Dimension)
      {
        subResourceRange *= desc.DepthOrArraySize;
      }
      subResourceRange *= getFormatLayers(desc.Format);
      return subResourceRange;
    }
    return 0;
  }
  // will return 0 if desc does not describe a texture.
  static UINT getMipIndexFromSubResIndex(const D3D12_RESOURCE_DESC &desc, UINT sub_res_index)
  {
    auto range = getMipRange(desc);
    return range > 0 ? sub_res_index % range : 0;
  }
  static bool isDepthStencilFormat(DXGI_FORMAT format)
  {
    switch (format)
    {
      default: return false;
      case DXGI_FORMAT_R32G8X24_TYPELESS:
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT: return true;
    }
  }
  static UINT getFormatLayers(DXGI_FORMAT format)
  {
    switch (format)
    {
        // most formats have only one laer
      default: return 1;
      // depth stencil formats have each a plane for depth and stencil
      case DXGI_FORMAT_R32G8X24_TYPELESS:
      case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      case DXGI_FORMAT_R24G8_TYPELESS:
        // planar video formats with Y and UV planes
      case DXGI_FORMAT_NV12:
      case DXGI_FORMAT_P010:
      case DXGI_FORMAT_P016:
      case DXGI_FORMAT_420_OPAQUE:
      case DXGI_FORMAT_NV11:
      case DXGI_FORMAT_P208: return 2;
    }
  }
  static Extent3D getFormatAlignment(DXGI_FORMAT format)
  {
    Extent3D blockSize;
    switch (format)
    {
      default: blockSize.width = blockSize.height = blockSize.depth = 1; break;
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
      case DXGI_FORMAT_BC4_TYPELESS:
      case DXGI_FORMAT_BC4_UNORM:
      case DXGI_FORMAT_BC4_SNORM:
      case DXGI_FORMAT_BC5_TYPELESS:
      case DXGI_FORMAT_BC5_UNORM:
      case DXGI_FORMAT_BC5_SNORM:
      case DXGI_FORMAT_BC6H_TYPELESS:
      case DXGI_FORMAT_BC6H_UF16:
      case DXGI_FORMAT_BC6H_SF16:
      case DXGI_FORMAT_BC7_TYPELESS:
      case DXGI_FORMAT_BC7_UNORM:
      case DXGI_FORMAT_BC7_UNORM_SRGB:
        blockSize.width = blockSize.height = 4;
        blockSize.depth = 1;
        break;
    }
    return blockSize;
  }
  static DXGI_FORMAT getBaseFormat(DXGI_FORMAT format)
  {
    // special case, this special biased format is a special interpretation of RGB10A2 UNORM and its not ordered as other formats in
    // the DXGI list
    if (DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM == format)
    {
      return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    }

    // This table has all typeless formats and regular formats that have no typeless equivalent.
    // Ordering of DXGI_FORMAT values is typeless comes first then all individual variants. With that
    // if a format is bigger than the previous entry and smaller as the current entry, then the formats
    // typeless variation is the previous entry.
    // Note that DXGI_FORMAT_D32_FLOAT_S8X24_UINT and DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS are no typeless
    // formats, just views for either depth or stencil components. Same goes for DXGI_FORMAT_R24_UNORM_X8_TYPELESS
    // and DXGI_FORMAT_X24_TYPELESS_G8_UINT.
    static const DXGI_FORMAT typelessTable[] = {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32G32B32A32_TYPELESS, DXGI_FORMAT_R32G32B32_TYPELESS,
      DXGI_FORMAT_R16G16B16A16_TYPELESS, DXGI_FORMAT_R32G32_TYPELESS, DXGI_FORMAT_R32G8X24_TYPELESS, DXGI_FORMAT_R10G10B10A2_TYPELESS,
      DXGI_FORMAT_R8G8B8A8_TYPELESS, DXGI_FORMAT_R16G16_TYPELESS, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_R24G8_TYPELESS,
      DXGI_FORMAT_R8G8_TYPELESS, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_R8_TYPELESS, DXGI_FORMAT_R1_UNORM,
      DXGI_FORMAT_R9G9B9E5_SHAREDEXP, DXGI_FORMAT_R8G8_B8G8_UNORM, DXGI_FORMAT_G8R8_G8B8_UNORM, DXGI_FORMAT_BC1_TYPELESS,
      DXGI_FORMAT_BC2_TYPELESS, DXGI_FORMAT_BC3_TYPELESS, DXGI_FORMAT_BC4_TYPELESS, DXGI_FORMAT_BC5_TYPELESS, DXGI_FORMAT_B5G6R5_UNORM,
      DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8A8_TYPELESS,
      DXGI_FORMAT_B8G8R8X8_TYPELESS, DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC7_TYPELESS, DXGI_FORMAT_AYUV, DXGI_FORMAT_Y410,
      DXGI_FORMAT_Y416, DXGI_FORMAT_NV12, DXGI_FORMAT_P010, DXGI_FORMAT_P016, DXGI_FORMAT_420_OPAQUE, DXGI_FORMAT_YUY2,
      DXGI_FORMAT_Y210, DXGI_FORMAT_Y216, DXGI_FORMAT_NV11, DXGI_FORMAT_AI44, DXGI_FORMAT_IA44, DXGI_FORMAT_P8, DXGI_FORMAT_A8P8,
      DXGI_FORMAT_B4G4R4A4_UNORM, DXGI_FORMAT_P208, DXGI_FORMAT_V208, DXGI_FORMAT_V408};

    // could use binary search, but probably not much (if at all) faster but much more complicated.
    for (auto at = eastl::begin(typelessTable), lastAt = eastl::begin(typelessTable), endAt = eastl::end(typelessTable); at != endAt;
         lastAt = at++)
    {
      if (format == *at)
      {
        return format;
      }
      if (format < *at)
      {
        // as with the rule that the typeless comes before the concrete formats, the last typeless we visited is the one we want.
        return *lastAt;
      }
    }
    return format;
  }
  static bool areCopyConvertibleFormats(DXGI_FORMAT from, DXGI_FORMAT to)
  {
    // same format can always be copied
    if (from == to)
    {
      return true;
    }

    // same base format can also be copied
    if (getBaseFormat(from) == getBaseFormat(to))
    {
      return true;
    }

    // this cuts down on possibilities if we have ordering of parameters
    auto first = min(from, to);
    auto second = max(from, to);

    // DXGI_FORMAT_R32_UINT - 42
    // DXGI_FORMAT_R32_SINT - 43
    // DXGI_FORMAT_R9G9B9E5_SHAREDEXP - 67
    if ((DXGI_FORMAT_R32_UINT == first) || (DXGI_FORMAT_R32_SINT == first))
    {
      return DXGI_FORMAT_R9G9B9E5_SHAREDEXP == second;
    }

    // DXGI_FORMAT_R16G16B16A16_UINT - 12
    // DXGI_FORMAT_R16G16B16A16_SINT - 14
    // DXGI_FORMAT_R32G32_UINT - 17
    // DXGI_FORMAT_R32G32_SINT - 18
    // DXGI_FORMAT_BC1_UNORM - 71
    // DXGI_FORMAT_BC1_UNORM_SRGB - 72
    // DXGI_FORMAT_BC4_UNORM - 80
    // DXGI_FORMAT_BC4_SNORM - 81
    if ((DXGI_FORMAT_R16G16B16A16_UINT == first) || (DXGI_FORMAT_R16G16B16A16_SINT == first) || (DXGI_FORMAT_R32G32_UINT == first) ||
        (DXGI_FORMAT_R32G32_SINT == first))
    {
      return (DXGI_FORMAT_BC1_UNORM == second) || (DXGI_FORMAT_BC1_UNORM_SRGB == second) || (DXGI_FORMAT_BC4_UNORM == second) ||
             (DXGI_FORMAT_BC4_SNORM == second);
    }

    // DXGI_FORMAT_R32G32B32A32_UINT - 3
    // DXGI_FORMAT_R32G32B32A32_SINT - 4
    // DXGI_FORMAT_BC2_UNORM - 74
    // DXGI_FORMAT_BC2_UNORM_SRGB - 75
    // DXGI_FORMAT_BC3_UNORM - 77
    // DXGI_FORMAT_BC3_UNORM_SRGB - 78
    // DXGI_FORMAT_BC5_UNORM - 83
    // DXGI_FORMAT_BC5_SNORM - 84
    // DXGI_FORMAT_BC6H_UF16 - 95
    // DXGI_FORMAT_BC6H_SF16 - 96
    // DXGI_FORMAT_BC7_UNORM - 98
    // DXGI_FORMAT_BC7_UNORM_SRGB - 99
    if ((DXGI_FORMAT_R32G32B32A32_UINT == first) || (DXGI_FORMAT_R32G32B32A32_SINT == first))
    {
      return (DXGI_FORMAT_BC2_UNORM == second) || (DXGI_FORMAT_BC2_UNORM_SRGB == second) || (DXGI_FORMAT_BC3_UNORM == second) ||
             (DXGI_FORMAT_BC3_UNORM_SRGB == second) || (DXGI_FORMAT_BC5_UNORM == second) || (DXGI_FORMAT_BC5_SNORM == second) ||
             (DXGI_FORMAT_BC6H_UF16 == second) || (DXGI_FORMAT_BC6H_SF16 == second) || (DXGI_FORMAT_BC7_UNORM == second) ||
             (DXGI_FORMAT_BC7_UNORM_SRGB == second);
    }

    return false;
  }

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // It validates the following properties of the inputs:
  // - dst and src are not null
  // - pResource of dst and src are not null
  // - x, y and z lies within dst extends
  // - x, y and z is aligned to the dst format block size
  // - if src_box is not null it lies within src extends
  // - if src_box is not null all its parameters are aligned to the src format block size
  // - the copy box lies within dst
  // - the copy box size is aligned to dst format block size
  // - src and dst formats are copy convertible
  void copyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION *dst, UINT x, UINT y, UINT z, const D3D12_TEXTURE_COPY_LOCATION *src,
    const D3D12_BOX *src_box)
  {
#define DX12_VALIDATAION_CONTEXT "CopyTextureRegion"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(dst, "dst can not be null");
    DX12_VALIDATE_CONDITION(src, "src can not be null");

    Extent3D dstSize{};
    Extent3D dstSizeAlign{1, 1, 1};
    DXGI_FORMAT dstFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_SAMPLE_DESC dstSampleDesc{};
    if (dst)
    {
      DX12_VALIDATE_CONDITION(dst->pResource, "dst->pResource can not be null");

      if (dst->pResource)
      {
        auto desc = dst->pResource->GetDesc();
        // This does the right thing, even when the resource is a buffer
        dstSampleDesc = desc.SampleDesc;

        if (D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT == dst->Type)
        {
          dstSize.width = dst->PlacedFootprint.Footprint.Width;
          dstSize.height = dst->PlacedFootprint.Footprint.Height;
          dstSize.depth = dst->PlacedFootprint.Footprint.Depth;
          dstFormat = dst->PlacedFootprint.Footprint.Format;
          dstSizeAlign = getFormatAlignment(dstFormat);

          auto sz = getBufferSize(desc);
          // buffers can not have a size of 0
          DX12_VALIDATE_CONDITION(sz > 0, "dst->pResource is not a buffer");
          DX12_VALIDATE_CONDITION(dst->PlacedFootprint.Offset < sz, "dst->PlacedFootprint.Offset %u is out of range %u",
            dst->PlacedFootprint.Offset, sz);

          auto totalSize = uint64_t(dst->PlacedFootprint.Footprint.RowPitch) * dstSize.depth;
          DX12_VALIDATE_CONDITION(dst->PlacedFootprint.Offset + totalSize <= sz,
            "dst->PlacedFootprint.Offset %u plus totalSize %u = %u is out of range %u", dst->PlacedFootprint.Offset, totalSize,
            dst->PlacedFootprint.Offset + totalSize, sz);
        }
        else if (D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX == dst->Type) // -V547
        {
          dstFormat = desc.Format;
          dstSizeAlign = getFormatAlignment(dstFormat);

          auto subResRange = getSubResRange(desc);
          if (dst->SubresourceIndex < subResRange)
          {
            dstSize = getExtentOfRes(desc, dst->SubresourceIndex);
          }
          else
          {
            DX12_ON_VALIDATION_ERROR("dst->SubresourceIndex %u out of range %u", dst->SubresourceIndex, subResRange);
          }
        }
        else
        {
          DX12_ON_VALIDATION_ERROR("dst->Type %u is invalid", (uint32_t)dst->Type);
        }
      }
    }
    // Only report if x, y or z where out of bounds if there was no error that prevented getting the extends.
    if (dstSize.width > 0)
    {
      DX12_VALIDATE_CONDITION(dstSize.width > x, "x %u is not within the destination extends %u", x, dstSize.width);
      DX12_VALIDATE_CONDITION(dstSize.height > y, "y %u is not within the destination extends %u", y, dstSize.height);
      DX12_VALIDATE_CONDITION(dstSize.depth > z, "z %u is not within the destination extends %u", z, dstSize.depth);

      DX12_VALIDATE_CONDITION(x == align_value(x, dstSizeAlign.width), "x %u is not aligned to the formats block width %u", x,
        dstSizeAlign.width);
      DX12_VALIDATE_CONDITION(y == align_value(y, dstSizeAlign.height), "y %u is not aligned to the formats block height %u", y,
        dstSizeAlign.height);
      DX12_VALIDATE_CONDITION(z == align_value(z, dstSizeAlign.depth), "z %u is not aligned to the formats block depth %u", z,
        dstSizeAlign.depth);
    }

    Extent3D srcSize{};
    Extent3D srcSizeAlign{1, 1, 1};
    DXGI_FORMAT srcFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_SAMPLE_DESC srcSampleDesc{};
    if (src)
    {
      DX12_VALIDATE_CONDITION(src->pResource, "src->pResource can not be null");

      if (src->pResource)
      {
        auto desc = src->pResource->GetDesc();
        // This does the right thing, even when the resource is a buffer
        srcSampleDesc = desc.SampleDesc;

        if (D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT == src->Type)
        {
          srcSize.width = src->PlacedFootprint.Footprint.Width;
          srcSize.height = src->PlacedFootprint.Footprint.Height;
          srcSize.depth = src->PlacedFootprint.Footprint.Depth;
          srcFormat = src->PlacedFootprint.Footprint.Format;
          srcSizeAlign = getFormatAlignment(srcFormat);

          auto sz = getBufferSize(desc);
          // buffers can not have a size of 0
          DX12_VALIDATE_CONDITION(sz > 0, "src->pResource is not a buffer");
          DX12_VALIDATE_CONDITION(src->PlacedFootprint.Offset < sz, "src->PlacedFootprint.Offset %u is out of range %u",
            src->PlacedFootprint.Offset, sz);
          auto totalSize = uint64_t(src->PlacedFootprint.Footprint.RowPitch) * srcSize.depth;
          DX12_VALIDATE_CONDITION(src->PlacedFootprint.Offset + totalSize <= sz,
            "src->PlacedFootprint.Offset %u plus totalSize %u = %u is out of range %u", src->PlacedFootprint.Offset, totalSize,
            src->PlacedFootprint.Offset + totalSize, sz);
        }
        else if (D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX == src->Type) // -V547
        {
          srcFormat = desc.Format;
          srcSizeAlign = getFormatAlignment(srcFormat);

          auto subResRange = getSubResRange(desc);
          if (src->SubresourceIndex < subResRange)
          {
            srcSize = getExtentOfRes(desc, src->SubresourceIndex);
          }
          else
          {
            DX12_ON_VALIDATION_ERROR("src->SubresourceIndex %u out of range %u", src->SubresourceIndex, subResRange);
          }
        }
        else
        {
          DX12_ON_VALIDATION_ERROR("src->Type %u is invalid", (uint32_t)src->Type);
        }
      }
    }

    Extent3D copyExtent{};
    if (srcSize.width > 0)
    {
      if (srcSampleDesc.Count > 1 || dstSampleDesc.Count > 1)
      {
        DX12_VALIDATE_CONDITION(srcSampleDesc.Count == dstSampleDesc.Count,
          "copy of multisampled textures, src %u and dst %u sample count have to be equal", srcSampleDesc.Count, dstSampleDesc.Count);

        DX12_VALIDATE_CONDITION(nullptr == src_box, "src_box has to be null for copies of multisampled textures");

        DX12_VALIDATE_CONDITION(0 == x, "x has to be 0 for copies of multisampled textures");
        DX12_VALIDATE_CONDITION(0 == y, "y has to be 0 for copies of multisampled textures");
        DX12_VALIDATE_CONDITION(0 == z, "z has to be 0 for copies of multisampled textures");

        copyExtent = srcSize;
      }
      else if (isDepthStencilFormat(srcFormat) || isDepthStencilFormat(dstFormat))
      {
        DX12_VALIDATE_CONDITION(nullptr == src_box, "src_box has to be null for copies of resources with depth stencil format %u, %u",
          (uint32_t)dstFormat, (uint32_t)srcFormat);

        DX12_VALIDATE_CONDITION(0 == x, "x has to be 0 for copies of resources with stencil format %u, %u", (uint32_t)dstFormat,
          (uint32_t)srcFormat);
        DX12_VALIDATE_CONDITION(0 == y, "y has to be 0 for copies of resources with stencil format %u, %u", (uint32_t)dstFormat,
          (uint32_t)srcFormat);
        DX12_VALIDATE_CONDITION(0 == z, "z has to be 0 for copies of resources with stencil format %u, %u", (uint32_t)dstFormat,
          (uint32_t)srcFormat);

        copyExtent = srcSize;
      }
      else if (src_box)
      {
        DX12_VALIDATE_CONDITION(src_box->left < srcSize.width, "src_box->left %u is not within the source extends %u", src_box->left,
          srcSize.width);
        DX12_VALIDATE_CONDITION(src_box->right <= srcSize.width, "src_box->right %u is not within the source extends %u",
          src_box->right, srcSize.width);
        DX12_VALIDATE_CONDITION(src_box->top < srcSize.height, "src_box->top %u is not within the source extends %u", src_box->top,
          srcSize.height);
        DX12_VALIDATE_CONDITION(src_box->bottom <= srcSize.height, "src_box->bottom %u is not within the source extends %u",
          src_box->bottom, srcSize.height);
        DX12_VALIDATE_CONDITION(src_box->front < srcSize.depth, "src_box->front %u is not within the source extends %u",
          src_box->front, srcSize.depth);
        DX12_VALIDATE_CONDITION(src_box->back <= srcSize.depth, "src_box->back %u is not within the source extends %u", src_box->back,
          srcSize.depth);

        DX12_VALIDATE_CONDITION(src_box->left == align_value(src_box->left, srcSizeAlign.width),
          "src_box->left %u is not aligned to the formats block size %u", src_box->left, srcSizeAlign.width);
        DX12_VALIDATE_CONDITION(src_box->right == align_value(src_box->right, srcSizeAlign.height),
          "src_box->right %u is not aligned to the formats block size %u", src_box->right, srcSizeAlign.height);
        DX12_VALIDATE_CONDITION(src_box->top == align_value(src_box->top, srcSizeAlign.depth),
          "src_box->top %u is not aligned to the formats block size %u", src_box->top, srcSizeAlign.depth);
        DX12_VALIDATE_CONDITION(src_box->bottom == align_value(src_box->bottom, srcSizeAlign.width),
          "src_box->bottom %u is not aligned to the formats block size %u", src_box->bottom, srcSizeAlign.width);
        DX12_VALIDATE_CONDITION(src_box->front == align_value(src_box->front, srcSizeAlign.height),
          "src_box->front %u is not aligned to the formats block size %u", src_box->front, srcSizeAlign.height);
        DX12_VALIDATE_CONDITION(src_box->back == align_value(src_box->back, srcSizeAlign.depth),
          "src_box->depth %u is not aligned to the formats block size %u", src_box->back, srcSizeAlign.depth);

        DX12_VALIDATE_CONDITION(src_box->left < src_box->right, "src_box->left %u has to be less than src_box->right %u",
          src_box->left, src_box->right);
        DX12_VALIDATE_CONDITION(src_box->top < src_box->bottom, "src_box->top %u has to be less than src_box->bottom %u", src_box->top,
          src_box->bottom);
        DX12_VALIDATE_CONDITION(src_box->front < src_box->back, "src_box->front %u has to be less than src_box->back %u",
          src_box->front, src_box->back);

        if ((src_box->left < src_box->right) && (src_box->top < src_box->bottom) && (src_box->front < src_box->back))
        {
          copyExtent.width = src_box->right - src_box->left;
          copyExtent.height = src_box->bottom - src_box->top;
          copyExtent.depth = src_box->back - src_box->front;
        }
      }
      else
      {
        copyExtent = srcSize;
      }
    }

    if (dstSize.width > 0)
    {
      if (copyExtent.width)
      {
        // We have to divide the coordinates by the block size of src and dst to get coordinates in
        // a uniform range, otherwise we will incorrectly compare copies between block compressed formats
        // and non block compressed formats.
        DX12_VALIDATE_CONDITION(copyExtent.width / srcSizeAlign.width <= (dstSize.width - x) / dstSizeAlign.width,
          "copied width %u (%u / %u) is out of destination bounds %u (%u - %u / %u)", copyExtent.width / srcSizeAlign.width,
          copyExtent.width, srcSizeAlign.width, (dstSize.width - x) / dstSizeAlign.width, dstSize.width, x, dstSizeAlign.width);

        DX12_VALIDATE_CONDITION(copyExtent.height / srcSizeAlign.height <= (dstSize.height - y) / dstSizeAlign.height,
          "copied height %u (%u / %u) is out of destination bounds %u (%u - %u / %u)", copyExtent.height / srcSizeAlign.height,
          copyExtent.height, srcSizeAlign.height, (dstSize.height - y) / dstSizeAlign.height, dstSize.height, y, dstSizeAlign.height);

        DX12_VALIDATE_CONDITION(copyExtent.depth / srcSizeAlign.depth <= (dstSize.depth - z) / dstSizeAlign.depth,
          "copied depth %u (%u / %u) is out of destination bounds %u (%u - %u / %u)", copyExtent.depth / srcSizeAlign.depth,
          copyExtent.depth, srcSizeAlign.depth, (dstSize.depth - z) / dstSizeAlign.depth, dstSize.depth, z, dstSizeAlign.depth);

        auto destCopyExtent = copyExtent / srcSizeAlign * dstSizeAlign;
        DX12_VALIDATE_CONDITION(destCopyExtent.width == align_value(destCopyExtent.width, dstSizeAlign.width),
          "copy region %u is not aligned to dst format block width %u", destCopyExtent.width, dstSizeAlign.width);
        DX12_VALIDATE_CONDITION(destCopyExtent.height == align_value(destCopyExtent.height, dstSizeAlign.height),
          "copy region %u is not aligned to dst format block height %u", destCopyExtent.height, dstSizeAlign.height);
        DX12_VALIDATE_CONDITION(destCopyExtent.depth == align_value(destCopyExtent.depth, dstSizeAlign.depth),
          "copy region %u is not aligned to dst format block depth %u", destCopyExtent.depth, dstSizeAlign.depth);
      }
    }

    if (DXGI_FORMAT_UNKNOWN != srcFormat && DXGI_FORMAT_UNKNOWN != dstFormat)
    {
      DX12_VALIDATE_CONDITION(areCopyConvertibleFormats(srcFormat, dstFormat), "src %u and dst %u formats are not copy convertible",
        (uint32_t)srcFormat, (uint32_t)dstFormat);
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT

    BaseType::copyTextureRegion(dst, x, y, z, src, src_box);
  }

  // It validates the following properties of the inputs:
  // - dst and src are not null
  // - dst_offset is within the size of dst
  // - src_offset is within the size of src
  // - dst_offset + num_bytes is within the size of dst
  // - src_offset + num_bytes is within the size of src
  void copyBufferRegion(ID3D12Resource *dst, UINT64 dst_offset, ID3D12Resource *src, UINT64 src_offset, UINT64 num_bytes)
  {
#define DX12_VALIDATAION_CONTEXT "CopyBufferRegion"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(dst, "dst can not be null");
    DX12_VALIDATE_CONDITION(src, "src can not be null");

    if (dst)
    {
      auto desc = dst->GetDesc();
      auto size = getBufferSize(desc);
      // buffers can not have a size of 0
      DX12_VALIDATE_CONDITION(size > 0, "dst is not a buffer");
      DX12_VALIDATE_CONDITION(dst_offset < size, "dst_offset %u is out of range %u", dst_offset, size);
      DX12_VALIDATE_CONDITION(dst_offset + num_bytes <= size, "dst_offset %u + num_bytes %u = %u is out of range %u", dst_offset,
        num_bytes, dst_offset, num_bytes, size);
    }

    if (src)
    {
      auto desc = src->GetDesc();
      auto size = getBufferSize(desc);
      // buffers can not have a size of 0
      DX12_VALIDATE_CONDITION(size > 0, "src is not a buffer");
      DX12_VALIDATE_CONDITION(src_offset < size, "src_offset %u is out of range %u", src_offset, size);
      DX12_VALIDATE_CONDITION(src_offset + num_bytes <= size, "src_offset %u + num_bytes %u = %u is out of range %u",
        src_offset + num_bytes, src_offset + num_bytes, size);
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT

    BaseType::copyBufferRegion(dst, dst_offset, src, src_offset, num_bytes);
  }

  // NOTE: right now it will drop all barriers if one is wrong!
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
#define DX12_VALIDATAION_CONTEXT "resourceBarrier"
    DX12_BEGIN_VALIATION();

    for (UINT b = 0; b < num_barriers; ++b)
    {
      DX12_UPDATE_VALIATION_STATE(BarrierValidator<CopyCommandListBarrierValidationPolicy>::validateBarrier(barriers[b]));
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resourceBarrier(num_barriers, barriers);
  }

  // It validates the following properties of the inputs:
  // - If the copy queue does support resolve or not
  // - If query_heap is not null
  // - If the type is D3D12_QUERY_TYPE_TIMESTAMP
  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
#define DX12_VALIDATAION_CONTEXT "endQuery"
    DX12_BEGIN_VALIATION();
    auto device = getDevice();
    D3D12_FEATURE_DATA_D3D12_OPTIONS3 level3Options{};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &level3Options, sizeof(level3Options));
    DX12_VALIDATE_CONDITION(FALSE != level3Options.CopyQueueTimestampQueriesSupported,
      "used on a copy queue on a device that does not support copy of timestamps on the copy queue");
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(D3D12_QUERY_TYPE_TIMESTAMP == type, "query type (%u) can only be D3D12_QUERY_TYPE_TIMESTAMP",
      uint32_t(type));
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::endQuery(query_heap, type, index);
  }

  // It validates the following properties of the inputs:
  // - If the copy queue does support resolve or not
  // - If query_heap is not null
  // - If destination_buffer is not null
  // - If the type is D3D12_QUERY_TYPE_TIMESTAMP
  // - If destination_buffer is a buffer resource
  // - If aligned_destination_buffer_offset is multiples of 8
  // - If aligned_destination_buffer_offset is within the size of destination_buffer
  // - If the offset and total size is within the size of destination_buffer
  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
#define DX12_VALIDATAION_CONTEXT "resolveQueryData"
    DX12_BEGIN_VALIATION();
    auto device = getDevice();
    D3D12_FEATURE_DATA_D3D12_OPTIONS3 level3Options{};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &level3Options, sizeof(level3Options));
    DX12_VALIDATE_CONDITION(FALSE != level3Options.CopyQueueTimestampQueriesSupported,
      "used on a copy queue on a device that does not support copy of timestamps on the copy queue");
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(nullptr != destination_buffer, "destination buffer can not be null");
    DX12_VALIDATE_CONDITION(D3D12_QUERY_TYPE_TIMESTAMP == type, "query type (%u) can only be D3D12_QUERY_TYPE_TIMESTAMP",
      uint32_t(type));
    if (destination_buffer && D3D12_QUERY_TYPE_TIMESTAMP == type)
    {
      auto desc = destination_buffer->GetDesc();
      auto bufferSize = getBufferSize(desc);
      DX12_VALIDATE_CONDITION(0 != bufferSize, "destination resource has to be a buffer");
      DX12_VALIDATE_CONDITION(0 == (aligned_destination_buffer_offset % 8), "destination offset (%u) has to be multiples of 8",
        aligned_destination_buffer_offset);
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset < bufferSize,
        "destination offset (%u) is outside of the buffer bounds (%u)", aligned_destination_buffer_offset, bufferSize);

      static constexpr uint32_t queryResultSize = sizeof(uint64_t);
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset + num_queries * queryResultSize <= bufferSize,
        "destination offset plus memory needed to store all query results (%u) (%u) is out of the buffer bounds (%u)", num_queries,
        aligned_destination_buffer_offset + num_queries * queryResultSize, bufferSize);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }
};

#if DX12_VALIDATA_COPY_COMMAND_LIST
template <typename T> //
using CopyCommandList = CopyCommandListParameterValidataion<T>;
#else
template <typename T>
using CopyCommandList = CopyCommandListImplementation<T>;
#endif

template <typename T>
class ComputeCommandListImplementation : public CopyCommandList<T>
{
  using BaseType = CopyCommandList<T>;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // If the BaseType is a validator, it will validate against the rules of the less capable command list.
  // This will then result in validation errors for barriers that are okay on our command list type.
  // So with that we re-implement the basic implementation here and do not call the BaseType version to
  // implement it.
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
    this->list->ResourceBarrier(num_barriers, barriers);
  }

  void setDescriptorHeaps(UINT num_descriptor_heaps, ID3D12DescriptorHeap *const *descriptor_heaps)
  {
    this->list->SetDescriptorHeaps(num_descriptor_heaps, descriptor_heaps);
  }

  void clearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE view_GPU_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const FLOAT values[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
    this->list->ClearUnorderedAccessViewFloat(view_GPU_handle_in_current_heap, view_CPU_handle, resource, values, num_rects, rects);
  }

  void clearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE view_GPU_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const UINT values[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
    this->list->ClearUnorderedAccessViewUint(view_GPU_handle_in_current_heap, view_CPU_handle, resource, values, num_rects, rects);
  }

  void setComputeRootSignature(ID3D12RootSignature *root_signature) { this->list->SetComputeRootSignature(root_signature); }

  void setPipelineState(ID3D12PipelineState *pipeline_state) { this->list->SetPipelineState(pipeline_state); }

  void dispatch(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
    this->list->Dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
  }

  void executeIndirect(ID3D12CommandSignature *command_signature, UINT max_command_count, ID3D12Resource *argument_buffer,
    UINT64 argument_buffer_offset, ID3D12Resource *count_buffer, UINT64 count_buffer_offset)
  {
    this->list->ExecuteIndirect(command_signature, max_command_count, argument_buffer, argument_buffer_offset, count_buffer,
      count_buffer_offset);
  }

  void setComputeRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
    this->list->SetComputeRootConstantBufferView(root_parameter_index, buffer_location);
  }

  void setComputeRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
    this->list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
  }

  void setComputeRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
    this->list->SetComputeRoot32BitConstants(root_parameter_index, num_32bit_values_to_set, src_data, dest_offset_in_32bit_values);
  }

  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index) { this->list->EndQuery(query_heap, type, index); }

  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
    this->list->ResolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }
  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region) { this->list->DiscardResource(resource, region); }
  void setPredication(ID3D12Resource *buffer, UINT64 aligned_buffer_offset, D3D12_PREDICATION_OP operation)
  {
    this->list->SetPredication(buffer, aligned_buffer_offset, operation);
  }
};

template <typename T>
class ComputeCommandListParameterValidataion : public ComputeCommandListImplementation<T>
{
  using BaseType = ComputeCommandListImplementation<T>;

protected:
  using BaseType::getBufferSize;
  using BaseType::getSubResRange;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
#define DX12_VALIDATAION_CONTEXT "resourceBarrier"
    DX12_BEGIN_VALIATION();

    for (UINT b = 0; b < num_barriers; ++b)
    {
      DX12_UPDATE_VALIATION_STATE(BarrierValidator<ComputeCommandListBarrierValidationPolicy>::validateBarrier(barriers[b]));
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resourceBarrier(num_barriers, barriers);
  }

  // It validates the following properties of the inputs:
  // - If num_descriptor_heaps is not 0
  // - If num_descriptor_heaps is less or equal to 2
  // - If descriptor_heaps is not null
  // - If each entry of descriptor_heaps is not null
  void setDescriptorHeaps(UINT num_descriptor_heaps, ID3D12DescriptorHeap *const *descriptor_heaps)
  {
#define DX12_VALIDATAION_CONTEXT "clearUnorderedAccessViewFloat"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != num_descriptor_heaps, "num of descriptor heaps can not be 0");
    DX12_VALIDATE_CONDITION(2 >= num_descriptor_heaps, "num of descriptor heaps %u must be less or equal than 2",
      num_descriptor_heaps);
    DX12_VALIDATE_CONDITION(nullptr != descriptor_heaps, "descriptor heap array pointer can not be null");
    if (descriptor_heaps)
    {
      for (UINT i = 0; i < num_descriptor_heaps; ++i)
      {
        DX12_VALIDATE_CONDITION(nullptr != descriptor_heaps[i], "descriptor heap pointer [%i] can not be null", i);
      }
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setDescriptorHeaps(num_descriptor_heaps, descriptor_heaps);
  }

  // It validates the following properties of the inputs:
  // - If view_GPU_handle_in_current_heap is not 0
  // - If view_CPU_handle is not 0
  // - If resource is not null
  // - When num_rects is not 0 then rects is not null
  void clearUnorderedAccessViewFloat(D3D12_GPU_DESCRIPTOR_HANDLE view_GPU_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const FLOAT values[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
#define DX12_VALIDATAION_CONTEXT "clearUnorderedAccessViewFloat"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != view_GPU_handle_in_current_heap.ptr, "GPU descriptor can not be 0");
    DX12_VALIDATE_CONDITION(0 != view_CPU_handle.ptr, "CPU descriptor can not be 0");
    DX12_VALIDATE_CONDITION(nullptr != resource, "Resource can not be null");
    if (num_rects > 0)
    {
      DX12_VALIDATE_CONDITION(nullptr != rects, "rects can not be null when num rects (%u) is not 0", num_rects);
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::clearUnorderedAccessViewFloat(view_GPU_handle_in_current_heap, view_CPU_handle, resource, values, num_rects, rects);
  }

  // It validates the following properties of the inputs:
  // - If view_GPU_handle_in_current_heap is not 0
  // - If view_CPU_handle is not 0
  // - If resource is not null
  // - When num_rects is not 0 then rects is not null
  void clearUnorderedAccessViewUint(D3D12_GPU_DESCRIPTOR_HANDLE view_GPU_handle_in_current_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE view_CPU_handle, ID3D12Resource *resource, const UINT values[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
#define DX12_VALIDATAION_CONTEXT "clearUnorderedAccessViewUint"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != view_GPU_handle_in_current_heap.ptr, "GPU descriptor can not be 0");
    DX12_VALIDATE_CONDITION(0 != view_CPU_handle.ptr, "CPU descriptor can not be 0");
    DX12_VALIDATE_CONDITION(nullptr != resource, "Resource can not be null");
    if (num_rects > 0)
    {
      DX12_VALIDATE_CONDITION(nullptr != rects, "rects can not be null when num rects (%u) is not 0", num_rects);
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::clearUnorderedAccessViewUint(view_GPU_handle_in_current_heap, view_CPU_handle, resource, values, num_rects, rects);
  }

  // It validates the following properties of the inputs:
  // - If root_signature is not null
  void setComputeRootSignature(ID3D12RootSignature *root_signature)
  {
#define DX12_VALIDATAION_CONTEXT "setComputeRootSignature"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(nullptr != root_signature, "Root signature can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setComputeRootSignature(root_signature);
  }

  // It validates the following properties of the inputs:
  // - If pipeline_state is not null
  void setPipelineState(ID3D12PipelineState *pipeline_state)
  {
#define DX12_VALIDATAION_CONTEXT "setPipelineState"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(nullptr != pipeline_state, "Pipeline state can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setPipelineState(pipeline_state);
  }

  // No validation done
  void dispatch(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
    BaseType::dispatch(thread_group_count_x, thread_group_count_y, thread_group_count_z);
  }

  // It validates the following properties of the inputs:
  // - If command_signature not null
  // - If argument_buffer not null
  // - If argument_buffer_offset is multiples of 4
  // - If argument_buffer_offset is within the size of argument_buffer
  // - If count_buffer_offset is multiples of 4
  // - When count_buffer is not null then count_buffer is a buffer resource
  // - When count_buffer is not null then count_buffer_offset is within the size of count_buffer
  void executeIndirect(ID3D12CommandSignature *command_signature, UINT max_command_count, ID3D12Resource *argument_buffer,
    UINT64 argument_buffer_offset, ID3D12Resource *count_buffer, UINT64 count_buffer_offset)
  {
#define DX12_VALIDATAION_CONTEXT "executeIndirect"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(nullptr != command_signature, "Command signature can not be null");
    DX12_VALIDATE_CONDITION(nullptr != argument_buffer, "Argument buffer can not be null");
    DX12_VALIDATE_CONDITION(0 == (argument_buffer_offset % 4), "Argument buffer offset %u has to be multiples of 4",
      argument_buffer_offset);
    if (argument_buffer)
    {
      auto size = getBufferSize(argument_buffer->GetDesc());
      DX12_VALIDATE_CONDITION(0 < size, "Argument buffer resource has be a buffer");
      DX12_VALIDATE_CONDITION(argument_buffer_offset < size, "Argument buffer offset %u is out of bounds %u", argument_buffer_offset,
        size);
    }
    DX12_VALIDATE_CONDITION(0 == (count_buffer_offset % 4), "Count buffer offset %u has to be multiples of 4", count_buffer_offset);
    if (count_buffer)
    {
      auto size = getBufferSize(count_buffer->GetDesc());
      DX12_VALIDATE_CONDITION(0 < size, "Count buffer resource has be a buffer");
      DX12_VALIDATE_CONDITION(count_buffer_offset < size, "Count buffer offset %u is out of bounds %u", count_buffer_offset, size);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::executeIndirect(command_signature, max_command_count, argument_buffer, argument_buffer_offset, count_buffer,
      count_buffer_offset);
  }

  // It validates the following properties of the inputs:
  // - If buffer_location is not 0
  void setComputeRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
#define DX12_VALIDATAION_CONTEXT "setComputeRootConstantBufferView"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != buffer_location, "Buffer address can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setComputeRootConstantBufferView(root_parameter_index, buffer_location);
  }

  // It validates the following properties of the inputs:
  // - If base_descriptor is not 0
  void setComputeRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
#define DX12_VALIDATAION_CONTEXT "setComputeRootDescriptorTable"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != base_descriptor.ptr, "Base descriptor can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setComputeRootDescriptorTable(root_parameter_index, base_descriptor);
  }

  // It validates the following properties of the inputs:
  // - If num_32bit_values_to_set is not 0
  // - If src_data is not null
  void setComputeRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
#define DX12_VALIDATAION_CONTEXT "setComputeRoot32BitConstants"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != num_32bit_values_to_set, "Can not set 0 constants");
    DX12_VALIDATE_CONDITION(nullptr != src_data, "Source data can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setComputeRoot32BitConstants(root_parameter_index, num_32bit_values_to_set, src_data, dest_offset_in_32bit_values);
  }

  // It validates the following properties of the inputs:
  // - If the copy queue does support resolve or not
  // - If query_heap is not null
  // - If the type is D3D12_QUERY_TYPE_TIMESTAMP
  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
#define DX12_VALIDATAION_CONTEXT "endQuery"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(D3D12_QUERY_TYPE_TIMESTAMP == type, "query type (%u) can only be D3D12_QUERY_TYPE_TIMESTAMP",
      uint32_t(type));
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::endQuery(query_heap, type, index);
  }

  // It validates the following properties of the inputs:
  // - If query_heap is not null
  // - If destination_buffer is not null
  // - If the type is D3D12_QUERY_TYPE_TIMESTAMP
  // - If destination_buffer is a buffer resource
  // - If aligned_destination_buffer_offset is multiples of 8
  // - If aligned_destination_buffer_offset is within the size of destination_buffer
  // - If the offset and total size is within the size of destination_buffer
  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
#define DX12_VALIDATAION_CONTEXT "resolveQueryData"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(nullptr != destination_buffer, "destination buffer can not be null");
    DX12_VALIDATE_CONDITION(D3D12_QUERY_TYPE_TIMESTAMP == type, "query type (%u) can only be D3D12_QUERY_TYPE_TIMESTAMP",
      uint32_t(type));
    if (destination_buffer && D3D12_QUERY_TYPE_TIMESTAMP == type)
    {
      auto desc = destination_buffer->GetDesc();
      auto bufferSize = getBufferSize(desc);
      DX12_VALIDATE_CONDITION(0 != bufferSize, "destination resource has to be a buffer");
      DX12_VALIDATE_CONDITION(0 == (aligned_destination_buffer_offset % 8), "destination offset (%u) has to be multiples of 8",
        aligned_destination_buffer_offset);
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset < bufferSize,
        "destination offset (%u) is outside of the buffer bounds (%u)", aligned_destination_buffer_offset, bufferSize);

      static constexpr uint32_t queryResultSize = sizeof(uint64_t);
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset + num_queries * queryResultSize <= bufferSize,
        "destination offset plus memory needed to store all query results (%u) (%u) is out of the buffer bounds (%u)", num_queries,
        aligned_destination_buffer_offset + num_queries * queryResultSize, bufferSize);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }

  // It validates the following properties of the inputs:
  // - If resource is not null
  // - If resource has the D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS flag
  // - When region is not null and its NumRects is not 0 then its pRects is not null
  // - When region is not null then its subresource range of FirstSubresource and NumSubresources is within the subresource range of
  // resource
  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region)
  {
#define DX12_VALIDATAION_CONTEXT "discardResource"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(nullptr != resource, "resource can not be null");
    if (resource)
    {
      auto desc = resource->GetDesc();
      DX12_VALIDATE_CONDITION(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS & desc.Flags,
        "discard on compute requires UAV resource flag");
      if (region)
      {
        if (region->NumRects)
        {
          DX12_VALIDATE_CONDITION(nullptr != region->pRects, "when region.NumRects is not 0, then region.pRects can not be null");
        }
        auto range = getSubResRange(desc);
        DX12_VALIDATE_CONDITION(range >= region->FirstSubresource + region->NumSubresources,
          "region.FirstSubresource %u + region.NumSubresources %u (%u) is out of bounds %u", region->FirstSubresource,
          region->NumSubresources, region->FirstSubresource + region->NumSubresources, range);
      }
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::discardResource(resource, region);
  }

  // It validates the following properties of the inputs:
  // - If aligned_buffer_offset is multiples of 8
  // - When buffer is not null then buffer is a buffer resource
  // - When buffer is not null then aligned_buffer_offset + 8 is withing the size of buffer
  void setPredication(ID3D12Resource *buffer, UINT64 aligned_buffer_offset, D3D12_PREDICATION_OP operation)
  {
#define DX12_VALIDATAION_CONTEXT "setPredication"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 == (aligned_buffer_offset % 8), "buffer offset %u has to be multiples of 8", aligned_buffer_offset);
    if (buffer)
    {
      auto size = getBufferSize(buffer->GetDesc());
      DX12_VALIDATE_CONDITION(size > 0, "resource has to be a buffer");
      DX12_VALIDATE_CONDITION(size >= aligned_buffer_offset + 8, "buffer offset %u is out of bounds %u", aligned_buffer_offset, size);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setPredication(buffer, aligned_buffer_offset, operation);
  }
};

#if DX12_VALIDATE_COMPUTE_COMMAND_LIST
template <typename T> //
using ComputeCommandList = ComputeCommandListParameterValidataion<T>;
#else
template <typename T>
using ComputeCommandList = ComputeCommandListImplementation<T>;
#endif

#if D3D_HAS_RAY_TRACING
template <typename T>
class RaytraceCommandListImplementation : public ComputeCommandList<T>
{
  using BaseType = ComputeCommandList<T>;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  void buildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *dst,
    UINT num_postbuild_info_descs, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *postbuild_info_descs)
  {
    this->list.template as<ID3D12GraphicsCommandList4>()->BuildRaytracingAccelerationStructure(dst, num_postbuild_info_descs,
      postbuild_info_descs);
  }

  void setPipelineState1(ID3D12StateObject *state_object)
  {
    this->list.template as<ID3D12GraphicsCommandList4>()->SetPipelineState1(state_object);
  }

  void dispatchRays(const D3D12_DISPATCH_RAYS_DESC *desc) { this->list.template as<ID3D12GraphicsCommandList4>()->DispatchRays(desc); }
};

template <typename T>
class RaytraceCommandListParameterValidataion : public RaytraceCommandListImplementation<T>
{
  using BaseType = RaytraceCommandListImplementation<T>;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // It validates the following properties of the inputs:
  // - If dst is not null
  // - If dsts DestAccelerationStructureData is multiples of D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
  // - If dsts DestAccelerationStructureData is not 0
  // - When dsts Inputs.Flags is has D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE then dsts
  // SourceAccelerationStructureData is multiples of D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT
  // - When dsts Inputs.Flags is has D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE then dsts
  // SourceAccelerationStructureData is not 0
  // - When dsts Inputs.Flags is has not D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE then dsts
  // SourceAccelerationStructureData is 0
  // - When dsts Inputs.Type is D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL then dsts Inputs.InstanceDescs is not null
  // - When dsts Inputs.Type is D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL and its Inputs.DescsLayout is
  // D3D12_ELEMENTS_LAYOUT_ARRAY then dsts Inputs.DescsLayout is not null
  // - When dsts Inputs.Type is D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL and its Inputs.DescsLayout is
  // D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS then dsts Inputs.ppGeometryDescs is not null
  // - When dsts Inputs.Type is D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL and its Inputs.DescsLayout is
  // D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS then each entry of dsts Inputs.ppGeometryDescs is not null
  // - If dsts ScratchAccelerationStructureData is not null
  // - When num_postbuild_info_descs is not 0 then postbuild_info_descs is not null
  void buildRaytracingAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC *dst,
    UINT num_postbuild_info_descs, const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC *postbuild_info_descs)
  {
#define DX12_VALIDATAION_CONTEXT "buildRaytracingAccelerationStructure"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != dst, "dst can not be null");
    if (dst)
    {
      DX12_VALIDATE_CONDITION(0 == (dst->DestAccelerationStructureData % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT),
        "dst.DestAccelerationStructureData (%u) has to be aligned to %u", dst->DestAccelerationStructureData,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
      DX12_VALIDATE_CONDITION(0 != dst->DestAccelerationStructureData, "dst.DestAccelerationStructureData can not be null");
      if (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE !=
          (dst->Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE))
      {
        DX12_VALIDATE_CONDITION(0 == (dst->SourceAccelerationStructureData % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT),
          "dst.SourceAccelerationStructureData (%u) has to be aligned to %u", dst->SourceAccelerationStructureData,
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        DX12_VALIDATE_CONDITION(0 != dst->SourceAccelerationStructureData,
          "dst.SourceAccelerationStructureData can not be null when updating");
      }
      else
      {
        DX12_VALIDATE_CONDITION(0 == dst->SourceAccelerationStructureData,
          "dst.SourceAccelerationStructureData has to be null when not updating");
      }
      if (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL == dst->Inputs.Type)
      {
        DX12_VALIDATE_CONDITION(0 != dst->Inputs.InstanceDescs, "dst->Inputs.InstanceDescs can not be null");
      }
      else if (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL == dst->Inputs.Type) // -V547
      {
        if (D3D12_ELEMENTS_LAYOUT_ARRAY == dst->Inputs.DescsLayout)
        {
          DX12_VALIDATE_CONDITION(nullptr != dst->Inputs.pGeometryDescs, "dst->Inputs.pGeometryDescs can not be null");
        }
        else if (D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS == dst->Inputs.DescsLayout) // -V547
        {
          DX12_VALIDATE_CONDITION(nullptr != dst->Inputs.ppGeometryDescs, "dst->Inputs.ppGeometryDescs can not be null");
          if (dst->Inputs.ppGeometryDescs)
          {
            for (UINT i = 0; i < dst->Inputs.NumDescs; ++i)
            {
              DX12_VALIDATE_CONDITION(nullptr != dst->Inputs.ppGeometryDescs[i], "dst->Inputs.ppGeometryDescs[%u] can not be null", i);
            }
          }
        }
      }
      DX12_VALIDATE_CONDITION(0 != dst->ScratchAccelerationStructureData, "dst.ScratchAccelerationStructureData can not be null");
    }
    if (num_postbuild_info_descs)
    {
      DX12_VALIDATE_CONDITION(nullptr != postbuild_info_descs,
        "postbuild info descs can not be null when num postbuild info descs is not 0");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::buildRaytracingAccelerationStructure(dst, num_postbuild_info_descs, postbuild_info_descs);
  }

  // It validates the following properties of the inputs:
  // - If state_object not null
  void setPipelineState1(ID3D12StateObject *state_object)
  {
#define DX12_VALIDATAION_CONTEXT "setPipelineState1"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != state_object, "state object can not be null");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setPipelineState1(state_object);
  }

  // It validates the following properties of the inputs:
  // - If desc is not null
  // - If descs RayGenerationShaderRecord.StartAddress is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs RayGenerationShaderRecord.SizeInBytes is less or equal to 4096
  // - If descs RayGenerationShaderRecord.SizeInBytes is not 0
  // - If descs MissShaderTable.StartAddress is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs MissShaderTable.SizeInBytes is less or equal to 4096
  // - If descs MissShaderTable.StrideInBytes is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs HitGroupTable.StartAddress is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs HitGroupTable.SizeInBytes is less or equal to 4096
  // - If descs HitGroupTable.StrideInBytes is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs CallableShaderTable.StartAddress is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs CallableShaderTable.SizeInBytes is less or equal to 4096
  // - If descs CallableShaderTable.StrideInBytes is multiples of D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT
  // - If descs Width times Height times Depth is less than 2 to the power of 30
  void dispatchRays(const D3D12_DISPATCH_RAYS_DESC *desc)
  {
#define DX12_VALIDATAION_CONTEXT "dispatchRays"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != desc, "desc can not be null");
    if (desc)
    {
      DX12_VALIDATE_CONDITION(0 == (desc->RayGenerationShaderRecord.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.RayGenerationShaderRecord.StartAddress (%u) has to be aligned to %u", desc->RayGenerationShaderRecord.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
      DX12_VALIDATE_CONDITION(4096 >= desc->RayGenerationShaderRecord.SizeInBytes,
        "desc.RayGenerationShaderRecord.SizeInBytes (%u) has to be less or equal to 4096",
        desc->RayGenerationShaderRecord.SizeInBytes);
      DX12_VALIDATE_CONDITION(0 != desc->RayGenerationShaderRecord.SizeInBytes,
        "desc.RayGenerationShaderRecord.SizeInBytes can not be 0");

      DX12_VALIDATE_CONDITION(0 == (desc->MissShaderTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.MissShaderTable.StartAddress (%u) has to be aligned to %u", desc->MissShaderTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
      DX12_VALIDATE_CONDITION(4096 >= desc->MissShaderTable.SizeInBytes,
        "desc.MissShaderTable.SizeInBytes (%u) has to be less or equal to 4096", desc->MissShaderTable.SizeInBytes);
      DX12_VALIDATE_CONDITION(0 == (desc->MissShaderTable.StrideInBytes % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.MissShaderTable.StrideInBytes (%u) has to be aligned to %u", desc->MissShaderTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

      DX12_VALIDATE_CONDITION(0 == (desc->HitGroupTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.HitGroupTable.StartAddress (%u) has to be aligned to %u", desc->HitGroupTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
      DX12_VALIDATE_CONDITION(4096 >= desc->HitGroupTable.SizeInBytes,
        "desc.HitGroupTable.SizeInBytes (%u) has to be less or equal to 4096", desc->HitGroupTable.SizeInBytes);
      DX12_VALIDATE_CONDITION(0 == (desc->HitGroupTable.StrideInBytes % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.HitGroupTable.StrideInBytes (%u) has to be aligned to %u", desc->HitGroupTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

      DX12_VALIDATE_CONDITION(0 == (desc->CallableShaderTable.StartAddress % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.CallableShaderTable.StartAddress (%u) has to be aligned to %u", desc->CallableShaderTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
      DX12_VALIDATE_CONDITION(4096 >= desc->CallableShaderTable.SizeInBytes,
        "desc.CallableShaderTable.SizeInBytes (%u) has to be less or equal to 4096", desc->CallableShaderTable.SizeInBytes);
      DX12_VALIDATE_CONDITION(0 == (desc->CallableShaderTable.StrideInBytes % D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT),
        "desc.CallableShaderTable.StrideInBytes (%u) has to be aligned to %u", desc->CallableShaderTable.StartAddress,
        D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

      DX12_VALIDATE_CONDITION(desc->Width * uint64_t(desc->Height) * desc->Depth < (uint64_t(1) << 30),
        "desc.Width * desc.Height * desc.Depth exceeds 2^30 limit");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::dispatchRays(desc);
  }
};

#if DX12_VALIDATE_RAYTRACE_COMMAND_LIST
template <typename T> //
using RaytraceCommandList = RaytraceCommandListParameterValidataion<T>;
#else
template <typename T>
using RaytraceCommandList = RaytraceCommandListImplementation<T>;
#endif

#else
template <typename T>
using RaytraceCommandList = ComputeCommandList<T>;
#endif

template <typename T>
class BasicGraphicsCommandListImplementation : public RaytraceCommandList<T>
{
  using BaseType = RaytraceCommandList<T>;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // If the BaseType is a validator, it will validate against the rules of the less capable command list.
  // This will then result in validation errors for barriers that are okay on our command list type.
  // So with that we re-implement the basic implementation here and do not call the BaseType version to
  // implement it.
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
    this->list->ResourceBarrier(num_barriers, barriers);
  }

  void clearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE render_target_view, const FLOAT color_rgba[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
    this->list->ClearRenderTargetView(render_target_view, color_rgba, num_rects, rects);
  }

  void clearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view, D3D12_CLEAR_FLAGS clear_flags, FLOAT depth, UINT8 stencil,
    UINT num_rects, const D3D12_RECT *rects)
  {
    this->list->ClearDepthStencilView(depth_stencil_view, clear_flags, depth, stencil, num_rects, rects);
  }

  void beginQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index) { this->list->BeginQuery(query_heap, type, index); }

  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index) { this->list->EndQuery(query_heap, type, index); }

  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
    this->list->ResolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }

  void writeBufferImmediate(UINT count, const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *params,
    const D3D12_WRITEBUFFERIMMEDIATE_MODE *modes)
  {
    this->list->WriteBufferImmediate(count, params, modes);
  }

  void omSetRenderTargets(UINT num_render_target_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE *render_target_descriptors,
    BOOL rts_single_handle_to_descriptor_range, const D3D12_CPU_DESCRIPTOR_HANDLE *depth_stencil_descriptor)
  {
    this->list->OMSetRenderTargets(num_render_target_descriptors, render_target_descriptors, rts_single_handle_to_descriptor_range,
      depth_stencil_descriptor);
  }

  void setGraphicsRootSignature(ID3D12RootSignature *root_signature) { this->list->SetGraphicsRootSignature(root_signature); }
  void iaSetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_topology) { this->list->IASetPrimitiveTopology(primitive_topology); }
  void omSetDepthBounds(FLOAT min_value, FLOAT max_value) { this->list->OMSetDepthBounds(min_value, max_value); }
  void omSetBlendFactor(const FLOAT blend_factor[4]) { this->list->OMSetBlendFactor(blend_factor); }
  void rsSetScissorRects(UINT num_rects, const D3D12_RECT *rects) { this->list->RSSetScissorRects(num_rects, rects); }
  void rsSetViewports(UINT num_viewports, const D3D12_VIEWPORT *viewports) { this->list->RSSetViewports(num_viewports, viewports); }
  void omSetStencilRef(UINT stencil_ref) { this->list->OMSetStencilRef(stencil_ref); }
  void drawInstanced(UINT vertex_count_per_instance, UINT instance_count, UINT start_vertex_location, UINT start_instance_location)
  {
    this->list->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
  }
  void drawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, UINT start_index_location, INT base_vertex_location,
    UINT start_instance_location)
  {
    this->list->DrawIndexedInstanced(index_count_per_instance, instance_count, start_index_location, base_vertex_location,
      start_instance_location);
  }
  void setGraphicsRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
    this->list->SetGraphicsRoot32BitConstants(root_parameter_index, num_32bit_values_to_set, src_data, dest_offset_in_32bit_values);
  }
  void setGraphicsRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
    this->list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
  }

  void setGraphicsRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
    this->list->SetGraphicsRootConstantBufferView(root_parameter_index, buffer_location);
  }

  void iaSetVertexBuffers(UINT start_slot, UINT num_views, const D3D12_VERTEX_BUFFER_VIEW *views)
  {
    this->list->IASetVertexBuffers(start_slot, num_views, views);
  }

  void iaSetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW *view) { this->list->IASetIndexBuffer(view); }
  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region) { this->list->DiscardResource(resource, region); }
};

template <typename T>
class BasicGraphicsCommandListParameterValidataion : public BasicGraphicsCommandListImplementation<T>
{
  using BaseType = BasicGraphicsCommandListImplementation<T>;

protected:
  using BaseType::getBufferSize;
  using BaseType::getSubResRange;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
#define DX12_VALIDATAION_CONTEXT "resourceBarrier"
    DX12_BEGIN_VALIATION();

    for (UINT b = 0; b < num_barriers; ++b)
    {
      DX12_UPDATE_VALIATION_STATE(BarrierValidator<BasicGraphicsCommandListBarrierValidationPolicy>::validateBarrier(barriers[b]));
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resourceBarrier(num_barriers, barriers);
  }

  // It validates the following properties of the inputs:
  // - If render_target_view is not 0
  // - When num_rects is not 0 then rects is not null
  void clearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE render_target_view, const FLOAT color_rgba[4], UINT num_rects,
    const D3D12_RECT *rects)
  {
#define DX12_VALIDATAION_CONTEXT "clearRenderTargetView"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 != render_target_view.ptr, "depth stencil view descriptor can not be 0");
    if (num_rects > 0)
    {
      DX12_VALIDATE_CONDITION(nullptr != rects, "rects can not be null when num rects (%u) is not 0", num_rects);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::clearRenderTargetView(render_target_view, color_rgba, num_rects, rects);
  }

  // It validates the following properties of the inputs:
  // - If render_target_view is not 0
  // - When num_rects is not 0 then rects is not null
  void clearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view, D3D12_CLEAR_FLAGS clear_flags, FLOAT depth, UINT8 stencil,
    UINT num_rects, const D3D12_RECT *rects)
  {
#define DX12_VALIDATAION_CONTEXT "clearDepthStencilView"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 != depth_stencil_view.ptr, "depth stencil view descriptor can not be 0");
    if (num_rects > 0)
    {
      DX12_VALIDATE_CONDITION(nullptr != rects, "rects can not be null when num rects (%u) is not 0", num_rects);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::clearDepthStencilView(depth_stencil_view, clear_flags, depth, stencil, num_rects, rects);
  }

  // It validates the following properties of the inputs:
  // - If query_heap is not null
  // - If type is not D3D12_QUERY_TYPE_TIMESTAMP
  void beginQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
#define DX12_VALIDATAION_CONTEXT "beginQuery"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(D3D12_QUERY_TYPE_TIMESTAMP != type, "D3D12_QUERY_TYPE_TIMESTAMP is a invalid type");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::beginQuery(query_heap, type, index);
  }

  // It validates the following properties of the inputs:
  // - If query_heap is not null
  void endQuery(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT index)
  {
#define DX12_VALIDATAION_CONTEXT "endQuery"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::endQuery(query_heap, type, index);
  }

  // It validates the following properties of the inputs:
  // - If query_heap is not null
  // - If destination_buffer is not null
  // - If destination_buffer is a buffer
  // - If aligned_destination_buffer_offset is multiples of 8
  // - If aligned_destination_buffer_offset is within the bounds of destination_buffer
  // - If total size needed is within the bound of destination_buffer
  void resolveQueryData(ID3D12QueryHeap *query_heap, D3D12_QUERY_TYPE type, UINT start_index, UINT num_queries,
    ID3D12Resource *destination_buffer, UINT64 aligned_destination_buffer_offset)
  {
#define DX12_VALIDATAION_CONTEXT "resolveQueryData"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(nullptr != query_heap, "query heap can not be null");
    DX12_VALIDATE_CONDITION(nullptr != destination_buffer, "destination buffer can not be null");
    if (destination_buffer)
    {
      auto desc = destination_buffer->GetDesc();
      auto bufferSize = getBufferSize(desc);
      DX12_VALIDATE_CONDITION(0 != bufferSize, "destination resource has to be a buffer");
      DX12_VALIDATE_CONDITION(0 == (aligned_destination_buffer_offset % 8), "destination offset (%u) has to be multiples of 8",
        aligned_destination_buffer_offset);
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset < bufferSize,
        "destination offset (%u) is outside of the buffer bounds (%u)", aligned_destination_buffer_offset, bufferSize);
      uint32_t queryResultSize = 0;
      if (D3D12_QUERY_TYPE_OCCLUSION == type || D3D12_QUERY_TYPE_BINARY_OCCLUSION == type || D3D12_QUERY_TYPE_TIMESTAMP == type)
      {
        queryResultSize = sizeof(uint64_t);
      }
      else if (D3D12_QUERY_TYPE_PIPELINE_STATISTICS == type)
      {
        queryResultSize = sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS);
      }
      else if (D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0 == type || D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1 == type ||
               D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2 == type || D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3 == type)
      {
        queryResultSize = sizeof(D3D12_QUERY_DATA_SO_STATISTICS);
      }
      // Would need to pull in d3d12video.h, which we better not do.
#if 0
      else if (D3D12_QUERY_TYPE_VIDEO_DECODE_STATISTICS == type)
      {
        queryResultSize = sizeof(D3D12_QUERY_DATA_VIDEO_DECODE_STATISTICS);
      }
#endif
      DX12_VALIDATE_CONDITION(aligned_destination_buffer_offset + num_queries * queryResultSize <= bufferSize,
        "destination offset plus memory needed to store all query results (%u) (%u) is out of the buffer bounds (%u)", num_queries,
        aligned_destination_buffer_offset + num_queries * queryResultSize, bufferSize);
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resolveQueryData(query_heap, type, start_index, num_queries, destination_buffer, aligned_destination_buffer_offset);
  }

  // It validates the following properties of the inputs:
  // - If count is not 0
  // - If params is not null
  void writeBufferImmediate(UINT count, const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER *params,
    const D3D12_WRITEBUFFERIMMEDIATE_MODE *modes)
  {
#define DX12_VALIDATAION_CONTEXT "writeBufferImmediate"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 != count, "count can not be 0");
    DX12_VALIDATE_CONDITION(nullptr != params, "params can not be null");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::writeBufferImmediate(count, params, modes);
  }

  // It validates the following properties of the inputs:
  // - When num_render_target_descriptors is not 0 then render_target_descriptors is not null
  // - When num_render_target_descriptors is not 0 and rts_single_handle_to_descriptor_range then render_target_descriptors first entry
  // is not 0
  // - When num_render_target_descriptors is not 0 and not rts_single_handle_to_descriptor_range then render_target_descriptors all
  // entries are not 0
  // - When depth_stencil_descriptor is not null then its ptr is not 0
  void omSetRenderTargets(UINT num_render_target_descriptors, const D3D12_CPU_DESCRIPTOR_HANDLE *render_target_descriptors,
    BOOL rts_single_handle_to_descriptor_range, const D3D12_CPU_DESCRIPTOR_HANDLE *depth_stencil_descriptor)
  {
#define DX12_VALIDATAION_CONTEXT "omSetRenderTargets"
    DX12_BEGIN_VALIATION();
    if (num_render_target_descriptors > 0)
    {
      DX12_VALIDATE_CONDITION(nullptr != render_target_descriptors,
        "render target descriptor pointer can not be null when counter is not 0 (%u)", num_render_target_descriptors);
      if (nullptr != render_target_descriptors)
      {
        if (rts_single_handle_to_descriptor_range)
        {
          DX12_VALIDATE_CONDITION(0 != render_target_descriptors[0].ptr, "render target descriptor 0 can not be 0");
        }
        else
        {
          for (UINT i = 0; i < num_render_target_descriptors; ++i)
          {
            DX12_VALIDATE_CONDITION(0 != render_target_descriptors[i].ptr, "render target descriptor %i can not be 0", i);
          }
        }
      }
    }
    if (nullptr != depth_stencil_descriptor)
    {
      DX12_VALIDATE_CONDITION(0 != depth_stencil_descriptor->ptr, "depth stencil target descriptor can not be 0");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::omSetRenderTargets(num_render_target_descriptors, render_target_descriptors, rts_single_handle_to_descriptor_range,
      depth_stencil_descriptor);
  }

  void setGraphicsRootSignature(ID3D12RootSignature *root_signature) { BaseType::setGraphicsRootSignature(root_signature); }
  void iaSetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY primitive_topology) { BaseType::iaSetPrimitiveTopology(primitive_topology); }
  // It validates the following properties of the inputs:
  // - If device supports depth bounds
  void omSetDepthBounds(FLOAT min_value, FLOAT max_value)
  {
#define DX12_VALIDATAION_CONTEXT "omSetDepthBounds"
    DX12_BEGIN_VALIATION();
    auto device = this->getDevice();
    if (device)
    {
      D3D12_FEATURE_DATA_D3D12_OPTIONS2 level2Features{};
      device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &level2Features, sizeof(level2Features));
      DX12_VALIDATE_CONDITION(FALSE != level2Features.DepthBoundsTestSupported,
        "set depth bounds on a device without depth bounds support");
    }
    else
    {
      // not a validation error when we were not able to retrieve the device from a command list
      logerr("DX12: ID3D12GraphicsCommandList::GetDevice returned null as a device");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::omSetDepthBounds(min_value, max_value);
  }
  void omSetBlendFactor(const FLOAT blend_factor[4]) { BaseType::omSetBlendFactor(blend_factor); }
  // It validates the following properties of the inputs:
  // - If num_rects is not 0
  // - If rects is not null
  void rsSetScissorRects(UINT num_rects, const D3D12_RECT *rects)
  {
#define DX12_VALIDATAION_CONTEXT "rsSetScissorRects"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 != num_rects, "num of rects can not be 0");
    DX12_VALIDATE_CONDITION(nullptr != rects, "rects can not be null");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::rsSetScissorRects(num_rects, rects);
  }
  // It validates the following properties of the inputs:
  // - If num_viewports is not 0
  // - If viewports is not null
  void rsSetViewports(UINT num_viewports, const D3D12_VIEWPORT *viewports)
  {
#define DX12_VALIDATAION_CONTEXT "rsSetViewports"
    DX12_BEGIN_VALIATION();
    DX12_VALIDATE_CONDITION(0 != num_viewports, "num of viewports can not be 0");
    DX12_VALIDATE_CONDITION(nullptr != viewports, "viewports can not be null");
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::rsSetViewports(num_viewports, viewports);
  }
  void omSetStencilRef(UINT stencil_ref) { BaseType::omSetStencilRef(stencil_ref); }
  void drawInstanced(UINT vertex_count_per_instance, UINT instance_count, UINT start_vertex_location, UINT start_instance_location)
  {
    BaseType::drawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
  }
  void drawIndexedInstanced(UINT index_count_per_instance, UINT instance_count, UINT start_index_location, INT base_vertex_location,
    UINT start_instance_location)
  {
    BaseType::drawIndexedInstanced(index_count_per_instance, instance_count, start_index_location, base_vertex_location,
      start_instance_location);
  }
  // It validates the following properties of the inputs:
  // - If num_32bit_values_to_set is not 0
  // - If src_data is not null
  void setGraphicsRoot32BitConstants(UINT root_parameter_index, UINT num_32bit_values_to_set, const void *src_data,
    UINT dest_offset_in_32bit_values)
  {
#define DX12_VALIDATAION_CONTEXT "setGraphicsRoot32BitConstants"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != num_32bit_values_to_set, "Can not set 0 constants");
    DX12_VALIDATE_CONDITION(nullptr != src_data, "Source data can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setGraphicsRoot32BitConstants(root_parameter_index, num_32bit_values_to_set, src_data, dest_offset_in_32bit_values);
  }
  // It validates the following properties of the inputs:
  // - If base_descriptor is not 0
  void setGraphicsRootDescriptorTable(UINT root_parameter_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor)
  {
#define DX12_VALIDATAION_CONTEXT "setGraphicsRootDescriptorTable"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != base_descriptor.ptr, "Base descriptor can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
  }

  // It validates the following properties of the inputs:
  // - If buffer_location is not 0
  void setGraphicsRootConstantBufferView(UINT root_parameter_index, D3D12_GPU_VIRTUAL_ADDRESS buffer_location)
  {
#define DX12_VALIDATAION_CONTEXT "setGraphicsRootConstantBufferView"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(0 != buffer_location, "Buffer address can not be null");

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::setGraphicsRootConstantBufferView(root_parameter_index, buffer_location);
  }

  void iaSetVertexBuffers(UINT start_slot, UINT num_views, const D3D12_VERTEX_BUFFER_VIEW *views)
  {
    BaseType::iaSetVertexBuffers(start_slot, num_views, views);
  }

  void iaSetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW *view) { BaseType::iaSetIndexBuffer(view); }
  // It validates the following properties of the inputs:
  // - If resource is not null
  // - If resource has any of the following flags D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
  // and D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
  // - When region is not null and its NumRects is not null then its pRects is not null
  // - When region is not null then its subresource range defined by FirstSubresource and NumSubresources is within the the subresource
  // range of resource
  void discardResource(ID3D12Resource *resource, const D3D12_DISCARD_REGION *region)
  {
#define DX12_VALIDATAION_CONTEXT "discardResource"
    DX12_BEGIN_VALIATION();

    DX12_VALIDATE_CONDITION(nullptr != resource, "resource can not be null");
    if (resource)
    {
      auto desc = resource->GetDesc();
      static constexpr D3D12_RESOURCE_FLAGS required_flags_mask =
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
      DX12_VALIDATE_CONDITION(required_flags_mask & desc.Flags, "discard on graphics requires UAV, RTV or DSV resource flag");
      if (region)
      {
        if (region->NumRects)
        {
          DX12_VALIDATE_CONDITION(nullptr != region->pRects, "when region.NumRects is not 0, then region.pRects can not be null");
        }
        auto range = getSubResRange(desc);
        DX12_VALIDATE_CONDITION(range > region->FirstSubresource, "region.FirstSubresource %u is out of bounds %u",
          region->FirstSubresource, range);
        DX12_VALIDATE_CONDITION(range >= region->FirstSubresource + region->NumSubresources,
          "region.FirstSubresource %u + region.NumSubresources %u (%u) is out of bounds %u", region->FirstSubresource,
          region->NumSubresources, region->FirstSubresource + region->NumSubresources, range);
      }
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::discardResource(resource, region);
  }
};

#if DX12_VALIDATE_GRAPHICS_COMMAND_LIST
template <typename T> //
using BasicGraphicsCommandList = BasicGraphicsCommandListParameterValidataion<T>;
#else
template <typename T>
using BasicGraphicsCommandList = BasicGraphicsCommandListImplementation<T>;
#endif

#if _TARGET_XBOXONE
template <typename T>
using ExtendedGraphicsCommandListParameterValidation = BasicGraphicsCommandList<T>;
template <typename T>
using ExtendedGraphicsCommandListImplementation = BasicGraphicsCommandList<T>;
#else
template <typename T>
class ExtendedGraphicsCommandListImplementation : public BasicGraphicsCommandList<T>
{
  using BaseType = BasicGraphicsCommandList<T>;

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // If the BaseType is a validator, it will validate against the rules of the less capable command list.
  // This will then result in validation errors for barriers that are okay on our command list type.
  // So with that we re-implement the basic implementation here and do not call the BaseType version to
  // implement it.
  // Needs to be overwritten as a new resource state is added and needs extra validation
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
    this->list->ResourceBarrier(num_barriers, barriers);
  }

  void rsSetShadingRate(D3D12_SHADING_RATE base_shading_rate, const D3D12_SHADING_RATE_COMBINER *combiners)
  {
    this->list.template as<ID3D12GraphicsCommandList5>()->RSSetShadingRate(base_shading_rate, combiners);
  }
  void rsSetShadingRateImage(ID3D12Resource *shading_rate_image)
  {
    this->list.template as<ID3D12GraphicsCommandList5>()->RSSetShadingRateImage(shading_rate_image);
  }

  void dispatchMesh(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
    this->list.template as<ID3D12GraphicsCommandList6>()->DispatchMesh(thread_group_count_x, thread_group_count_y,
      thread_group_count_z);
  }
};

template <typename T>
class ExtendedGraphicsCommandListParameterValidation : public ExtendedGraphicsCommandListImplementation<T>
{
  using BaseType = ExtendedGraphicsCommandListImplementation<T>;

protected:
  static bool isExtendedShadingRate(D3D12_SHADING_RATE rate)
  {
    switch (rate)
    {
      default: return false;
      case D3D12_SHADING_RATE_1X1:
      case D3D12_SHADING_RATE_1X2:
      case D3D12_SHADING_RATE_2X1:
      case D3D12_SHADING_RATE_2X2: return false;
      case D3D12_SHADING_RATE_2X4:
      case D3D12_SHADING_RATE_4X2:
      case D3D12_SHADING_RATE_4X4: return true;
    }
  }

public:
  using BaseType::BaseType;
  using BaseType::operator bool;

  // Adds validation for the new D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE read state
  void resourceBarrier(UINT num_barriers, const D3D12_RESOURCE_BARRIER *barriers)
  {
#define DX12_VALIDATAION_CONTEXT "resourceBarrier"
    DX12_BEGIN_VALIATION();

    for (UINT b = 0; b < num_barriers; ++b)
    {
      DX12_UPDATE_VALIATION_STATE(BarrierValidator<ExtendedGraphicsCommandListBarrierValidationPolicy>::validateBarrier(barriers[b]));
    }

    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::resourceBarrier(num_barriers, barriers);
  }

  // Validates the following:
  // - if the device supports VRS or not
  // - if the device supports extended VRS rates when they are used
  // - if the device supports combiners when they are used
  void rsSetShadingRate(D3D12_SHADING_RATE base_shading_rate, const D3D12_SHADING_RATE_COMBINER *combiners)
  {
#define DX12_VALIDATAION_CONTEXT "rsSetShadingRate"
    DX12_BEGIN_VALIATION();
    auto device = this->getDevice();
    if (device)
    {
      // check if VRS is supported
      D3D12_FEATURE_DATA_D3D12_OPTIONS6 level6Features{};
      device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &level6Features, sizeof(level6Features));
      DX12_VALIDATE_CONDITION(level6Features.VariableShadingRateTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED,
        "used VRS while not supported by the device");
      DX12_VALIDATE_CONDITION(
        !isExtendedShadingRate(base_shading_rate) ||
          (isExtendedShadingRate(base_shading_rate) && (FALSE != level6Features.AdditionalShadingRatesSupported)),
        "used extended VRS rates while not supported by the device");

      if (combiners)
      {
        DX12_VALIDATE_CONDITION(level6Features.VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_2,
          "used VRS combiners while not supported by the device");
      }
    }
    else
    {
      // not a validation error when we were not able to retrieve the device from a command list
      logerr("DX12: ID3D12GraphicsCommandList::GetDevice returned null as a device");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::rsSetShadingRate(base_shading_rate, combiners);
  }
  // Validates the following:
  // - if the device supports VRS textures or not
  // - the texture is 2d
  // - the texture has only one layer
  // - the texture has only one mip level
  // - the texture uses the r8 unorm format
  // - the texture has the unknown layout
  // - the texture has not the render target flag
  // - the texture has not the depth stencil flag
  // - the texture has not the simultaneous access flag
  void rsSetShadingRateImage(ID3D12Resource *shading_rate_image)
  {
#define DX12_VALIDATAION_CONTEXT "rsSetShadingRateImage"
    DX12_BEGIN_VALIATION();
    auto device = this->getDevice();
    if (device)
    {
      // check if VRS is supported
      D3D12_FEATURE_DATA_D3D12_OPTIONS6 level6Features{};
      device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &level6Features, sizeof(level6Features));
      DX12_VALIDATE_CONDITION(level6Features.VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_2,
        "used VRS with rate image while not supported by the device");
    }
    else
    {
      // not a validation error when we were not able to retrieve the device from a command list
      logerr("DX12: ID3D12GraphicsCommandList::GetDevice returned null as a device");
    }

    if (shading_rate_image)
    {
      auto desc = shading_rate_image->GetDesc();
      DX12_VALIDATE_CONDITION(D3D12_RESOURCE_DIMENSION_TEXTURE2D == desc.Dimension, "shading rate texture is not a 2d texture");
      if (D3D12_RESOURCE_DIMENSION_TEXTURE2D == desc.Dimension)
      {
        DX12_VALIDATE_CONDITION(1 == desc.DepthOrArraySize, "shading rate texture is not a single layer 2d texture");
      }
      DX12_VALIDATE_CONDITION(1 == desc.MipLevels, "shading rate texture has %u mip levels, which is more than one", desc.MipLevels);
      DX12_VALIDATE_CONDITION(DXGI_FORMAT_R8_UINT == desc.Format, "shading rate texture does not have R8 UINT format");
      DX12_VALIDATE_CONDITION(D3D12_TEXTURE_LAYOUT_UNKNOWN == desc.Layout, "shading rate texture does not have a 'unknown' layout");
      DX12_VALIDATE_CONDITION(0 == (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & desc.Flags),
        "shading rate texture can not be a render target");
      DX12_VALIDATE_CONDITION(0 == (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL & desc.Flags),
        "shading rate texture can not be a depth stencil target");
      DX12_VALIDATE_CONDITION(0 == (D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS & desc.Flags),
        "shading rate texture can not simultaneous access resource");
    }
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::rsSetShadingRateImage(shading_rate_image);
  }

  void dispatchMesh(UINT thread_group_count_x, UINT thread_group_count_y, UINT thread_group_count_z)
  {
#define DX12_VALIDATAION_CONTEXT "dispatchMesh"
    DX12_BEGIN_VALIATION();
    auto device = this->getDevice();
    if (device)
    {
      // check if mesh shaders are supported
      D3D12_FEATURE_DATA_D3D12_OPTIONS7 level7Features{};
      device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &level7Features, sizeof(level7Features));
      DX12_VALIDATE_CONDITION(level7Features.MeshShaderTier == D3D12_MESH_SHADER_TIER_1,
        "used dispatchMesh while not supported by the device");
    }
    else
    {
      // not a validation error when we were not able to retrieve the device from a command list
      logerr("DX12: ID3D12GraphicsCommandList::GetDevice returned null as a device");
    }

    DX12_VALIDATE_CONDITION(thread_group_count_x < 0xFFFF, "Thread group count of %u for x is too large, has to be smaller than 64k",
      thread_group_count_x);
    DX12_VALIDATE_CONDITION(thread_group_count_y < 0xFFFF, "Thread group count of %u for y is too large, has to be smaller than 64k",
      thread_group_count_y);
    DX12_VALIDATE_CONDITION(thread_group_count_z < 0xFFFF, "Thread group count of %u for z is too large, has to be smaller than 64k",
      thread_group_count_z);

    uint64_t totalCount = uint64_t(thread_group_count_x) * thread_group_count_y * thread_group_count_z;
    static const uint64_t total_count_limit = 1u << 22;
    DX12_VALIDATE_CONDITION(totalCount <= total_count_limit, "Total thread group size is of %u too large, can not exceed %u",
      totalCount, total_count_limit);
    DX12_FINALIZE_VALIDATAION();
#undef DX12_VALIDATAION_CONTEXT
    BaseType::dispatchMesh(thread_group_count_x, thread_group_count_y, thread_group_count_z);
  }
};
#endif

#if DX12_VALIDATE_GRAPHICS_COMMAND_LIST
template <typename T> //
using GraphicsCommandList = ExtendedGraphicsCommandListParameterValidation<T>;
#else
template <typename T>
using GraphicsCommandList = ExtendedGraphicsCommandListImplementation<T>;
#endif

} // namespace drv3d_dx12
