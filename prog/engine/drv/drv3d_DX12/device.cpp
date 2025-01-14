// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"

#if _TARGET_PC_WIN
#include "debug/global_state.h"
#endif

#include <dxgi_utils.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <nvLowLatency.h>


using namespace drv3d_dx12;

namespace drv3d_dx12
{
void to_debuglog(const dxil::ShaderDeviceRequirement &req)
{
  logdbg("DX12: ShaderDeviceRequirement:shaderModel %u.%u", (req.shaderModel >> 4) & 0xF, req.shaderModel & 0xF);
  struct TableEntry
  {
    const char *name;
    uint64_t mask;
  };
  static const TableEntry table[] = {
#define ADD_E(name) {#name, D3D_SHADER_REQUIRES_##name}
    ADD_E(DOUBLES),
    ADD_E(EARLY_DEPTH_STENCIL),
    ADD_E(UAVS_AT_EVERY_STAGE),
    ADD_E(64_UAVS),
    ADD_E(MINIMUM_PRECISION),
    ADD_E(11_1_DOUBLE_EXTENSIONS),
    ADD_E(11_1_SHADER_EXTENSIONS),
    ADD_E(LEVEL_9_COMPARISON_FILTERING),
    ADD_E(TILED_RESOURCES),
    ADD_E(STENCIL_REF),
    ADD_E(INNER_COVERAGE),
    ADD_E(TYPED_UAV_LOAD_ADDITIONAL_FORMATS),
    ADD_E(ROVS),
    ADD_E(VIEWPORT_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER_FEEDING_RASTERIZER),
    ADD_E(WAVE_OPS),
    ADD_E(INT64_OPS),
    ADD_E(VIEW_ID),
    ADD_E(BARYCENTRICS),
    ADD_E(NATIVE_16BIT_OPS),
    ADD_E(SHADING_RATE),
    ADD_E(RAYTRACING_TIER_1_1),
    ADD_E(SAMPLER_FEEDBACK),
    ADD_E(ATOMIC_INT64_ON_TYPED_RESOURCE),
    ADD_E(ATOMIC_INT64_ON_GROUP_SHARED),
    ADD_E(DERIVATIVES_IN_MESH_AND_AMPLIFICATION_SHADERS),
    ADD_E(RESOURCE_DESCRIPTOR_HEAP_INDEXING),
    ADD_E(SAMPLER_DESCRIPTOR_HEAP_INDEXING),
    ADD_E(WAVE_MMA),
    ADD_E(ATOMIC_INT64_ON_DESCRIPTOR_HEAP_RESOURCE),
#undef ADD_E
  };
  uint64_t combined = static_cast<uint64_t>(req.shaderFeatureFlagsLow) | (static_cast<uint64_t>(req.shaderFeatureFlagsHigh) << 32);
  for (auto &e : table)
  {
    logdbg("DX12: ShaderDeviceRequirement:%s = %s", e.name, (e.mask & combined) ? "Yes" : "No");
  }
}
} // namespace drv3d_dx12

#if D3D_HAS_RAY_TRACING
namespace
{

DXGI_FORMAT VSDTToDXGIFormat(uint32_t format)
{
  // TODO:: somewhat redundant with getLocationFormat in shader.h
  switch (format)
  {
    case VSDT_FLOAT1: return DXGI_FORMAT_R32_FLOAT;
    case VSDT_FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
    case VSDT_FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
    case VSDT_FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case VSDT_INT1: return DXGI_FORMAT_R32_SINT;
    case VSDT_INT2: return DXGI_FORMAT_R32G32_SINT;
    case VSDT_INT3: return DXGI_FORMAT_R32G32B32_SINT;
    case VSDT_INT4: return DXGI_FORMAT_R32G32B32A32_SINT;
    case VSDT_UINT1: return DXGI_FORMAT_R32_UINT;
    case VSDT_UINT2: return DXGI_FORMAT_R32G32_UINT;
    case VSDT_UINT3: return DXGI_FORMAT_R32G32B32_UINT;
    case VSDT_UINT4: return DXGI_FORMAT_R32G32B32A32_UINT;
    case VSDT_HALF2: return DXGI_FORMAT_R16G16_FLOAT;
    case VSDT_SHORT2N: return DXGI_FORMAT_R16G16_SNORM;
    case VSDT_SHORT2: return DXGI_FORMAT_R16G16_SINT;
    case VSDT_USHORT2N: return DXGI_FORMAT_R16G16_UNORM;

    case VSDT_HALF4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case VSDT_SHORT4N: return DXGI_FORMAT_R16G16B16A16_SNORM;
    case VSDT_SHORT4: return DXGI_FORMAT_R16G16B16A16_SINT;
    case VSDT_USHORT4N: return DXGI_FORMAT_R16G16B16A16_UNORM;

    case VSDT_UDEC3: return DXGI_FORMAT_R10G10B10A2_UINT;
    case VSDT_DEC3N: return DXGI_FORMAT_R10G10B10A2_UNORM;

    case VSDT_E3DCOLOR: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case VSDT_UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT;
    default: G_ASSERTF(false, "invalid vertex declaration type"); break;
  }
  return DXGI_FORMAT_UNKNOWN;
}

D3D12_RAYTRACING_GEOMETRY_FLAGS toGeometryFlags(RaytraceGeometryDescription::Flags flags)
{
  D3D12_RAYTRACING_GEOMETRY_FLAGS result = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::IS_OPAQUE))
  {
    result |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
  }
  if (RaytraceGeometryDescription::Flags::NONE != (flags & RaytraceGeometryDescription::Flags::NO_DUPLICATE_ANY_HIT_INVOCATION))
  {
    result |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
  }
  return result;
}

eastl::pair<D3D12_RAYTRACING_GEOMETRY_DESC, RaytraceGeometryDescriptionBufferResourceReferenceSet>
raytraceGeometryDescriptionToGeometryDescAABBs(const RaytraceGeometryDescription::AABBsInfo &info)
{
  auto buf = (GenericBufferInterface *)info.buffer;
  buf->updateDeviceBuffer([](auto &b) { b.resourceId.isUsedAsNonPixelShaderResourceBuffer(); });
  BufferResourceReferenceAndAddress devBuf = get_any_buffer_ref(buf);

  RaytraceGeometryDescriptionBufferResourceReferenceSet refs{};
  refs.vertexOrAABBBuffer = devBuf;

  D3D12_RAYTRACING_GEOMETRY_DESC desc{};
  desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
  desc.AABBs.AABBCount = info.count;
  desc.AABBs.AABBs.StartAddress = devBuf.gpuPointer + info.offset;
  desc.AABBs.AABBs.StrideInBytes = info.stride;
  desc.Flags = toGeometryFlags(info.flags);

  return {desc, refs};
}

eastl::pair<D3D12_RAYTRACING_GEOMETRY_DESC, RaytraceGeometryDescriptionBufferResourceReferenceSet>
raytraceGeometryDescriptionToGeometryDescTriangles(const RaytraceGeometryDescription::TrianglesInfo &info)
{
  auto vbuf = (GenericBufferInterface *)info.vertexBuffer;
  auto ibuf = (GenericBufferInterface *)info.indexBuffer;
  auto tbuf = (GenericBufferInterface *)info.transformBuffer;

  vbuf->updateDeviceBuffer([](auto &b) { b.resourceId.isUsedAsNonPixelShaderResourceBuffer(); });
  if (ibuf)
    ibuf->updateDeviceBuffer([](auto &b) { b.resourceId.isUsedAsNonPixelShaderResourceBuffer(); });
  if (tbuf)
    tbuf->updateDeviceBuffer([](auto &b) { b.resourceId.isUsedAsNonPixelShaderResourceBuffer(); });

  BufferResourceReferenceAndAddress devVbuf = get_any_buffer_ref(vbuf);
  BufferResourceReferenceAndAddress devIbuf = ibuf ? get_any_buffer_ref(ibuf) : BufferResourceReferenceAndAddress();
  BufferResourceReferenceAndAddress devTbuf = tbuf ? get_any_buffer_ref(tbuf) : BufferResourceReferenceAndAddress();

  RaytraceGeometryDescriptionBufferResourceReferenceSet refs{};
  refs.vertexOrAABBBuffer = devVbuf;
  refs.indexBuffer = devIbuf;
  refs.transformBuffer = devTbuf;

  D3D12_RAYTRACING_GEOMETRY_DESC desc{};
  desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  desc.Triangles.IndexBuffer = devIbuf.gpuPointer;
  desc.Triangles.IndexCount = info.indexCount;
  if (ibuf)
  {
    desc.Triangles.IndexFormat = get_index_format(ibuf);
    if (DXGI_FORMAT_R32_UINT == desc.Triangles.IndexFormat)
    {
      desc.Triangles.IndexBuffer += info.indexOffset * 4;
    }
    else
    {
      G_ASSERT(DXGI_FORMAT_R16_UINT == desc.Triangles.IndexFormat); // -V547
      desc.Triangles.IndexBuffer += info.indexOffset * 2;
    }
  }
  else
  {
    desc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
  }
  desc.Triangles.Transform3x4 = devTbuf.gpuPointer + info.transformOffset * sizeof(float) * 3 * 4;
  desc.Triangles.VertexFormat = VSDTToDXGIFormat(info.vertexFormat);
  desc.Triangles.VertexCount = info.vertexCount;
  desc.Triangles.VertexBuffer.StartAddress = devVbuf.gpuPointer + info.vertexStride * info.vertexOffset + info.vertexOffsetExtraBytes;
  desc.Triangles.VertexBuffer.StrideInBytes = info.vertexStride;
  desc.Flags = toGeometryFlags(info.flags);

  return {desc, refs};
}

} // namespace

namespace drv3d_dx12
{

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS toAccelerationStructureBuildFlags(RaytraceBuildFlags flags)
{
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS result = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::ALLOW_UPDATE))
    result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::ALLOW_COMPACTION))
    result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::FAST_TRACE))
    result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::FAST_BUILD))
    result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
  if (RaytraceBuildFlags::NONE != (flags & RaytraceBuildFlags::LOW_MEMORY))
    result |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
  // TODO:: D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE
  return result;
}

eastl::pair<D3D12_RAYTRACING_GEOMETRY_DESC, RaytraceGeometryDescriptionBufferResourceReferenceSet>
raytraceGeometryDescriptionToGeometryDesc(const RaytraceGeometryDescription &desc)
{
  switch (desc.type)
  {
    case RaytraceGeometryDescription::Type::TRIANGLES: return raytraceGeometryDescriptionToGeometryDescTriangles(desc.data.triangles);
    case RaytraceGeometryDescription::Type::AABBS: return raytraceGeometryDescriptionToGeometryDescAABBs(desc.data.aabbs);
  }
  G_ASSERT(false);
  return {};
}

} // namespace drv3d_dx12
#endif

// for whatever reason this is missing on GDK, GDK is a mess...
#ifndef D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
#define D3D12_TEXTURE_DATA_PITCH_ALIGNMENT D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT
#endif

namespace
{
template <typename T>
T block_count(T value, T block_size)
{
  return (value + block_size - 1) / block_size;
}
} // namespace

TextureSubresourceInfo drv3d_dx12::calculate_texture_region_info(Extent3D ext, ArrayLayerCount arrays, FormatStore fmt)
{
  TextureSubresourceInfo result{};
  uint32_t blockX = 1, blockY = 1;
  auto blockBytes = fmt.getBytesPerPixelBlock(&blockX, &blockY);
  result.rowCount = block_count(ext.height, blockY);
  result.rowByteSize = block_count(ext.width, blockX) * blockBytes;
  result.footprint.Format = fmt.asDxGiFormat();
  // width and height has to be aligned to block size, otherwise we crash the device and fail
  // validation.
  result.footprint.Width = align_value<uint32_t>(ext.width, blockX);
  result.footprint.Height = align_value<uint32_t>(ext.height, blockY);
  result.footprint.Depth = ext.depth;
  result.footprint.RowPitch = align_value<uint32_t>(result.rowByteSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
  result.totalByteSize = align_value<uint32_t>(result.rowCount * result.footprint.RowPitch * ext.depth * arrays.count(),
    D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  return result;
}

uint64_t drv3d_dx12::calculate_texture_staging_buffer_size(Extent3D size, MipMapCount mips, FormatStore format,
  SubresourceRange subresource_range)
{
  uint64_t totalSize = 0;
  uint32_t blockX = 1, blockY = 1;
  auto blockBytes = format.getBytesPerPixelBlock(&blockX, &blockY);

  for (auto subres_index : subresource_range)
  {
    auto mip = calculate_mip_slice_from_index(subres_index.index(), mips.count());
    auto ext = mip_extent(size, mip);
    auto rowCount = block_count(ext.height, blockY);
    auto rowByteSize = block_count(ext.width, blockX) * blockBytes;
    auto rowPitch = align_value<uint32_t>(rowByteSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    totalSize += align_value<uint32_t>(rowCount * rowPitch * ext.depth, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  }

  return totalSize;
}

bool Device::isSamplesCountSupported(DXGI_FORMAT format, int32_t samples_count)
{
  if (samples_count == 1)
    return true;

  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQualityLevels{};
  multisampleQualityLevels.Format = format;
  multisampleQualityLevels.SampleCount = samples_count;

  auto result = device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, //
    &multisampleQualityLevels, sizeof(multisampleQualityLevels));

  return SUCCEEDED(result) && multisampleQualityLevels.NumQualityLevels > 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE Device::getNonRecentImageViews(Image *img, ImageViewState state)
{
  auto iter = eastl::find_if(begin(img->getOldViews()), end(img->getOldViews()),
    [state](const Image::ViewInfo &view) { return view.state == state; });
  if (iter == img->getOldViews().end())
  {
    if (isInErrorState())
    {
      if (state.isDSV())
        return {};
      if (state.isRTV())
        return nullResourceTable.table[NullResourceTable::RENDER_TARGET_VIEW];
      if (state.isUAV())
        return nullResourceTable.get(D3D_SIT_UAV_RWTYPED, img->getSRVDimension());
      if (state.isSRV())
        return nullResourceTable.get(D3D_SIT_TEXTURE, img->getSRVDimension());
    }
    Image::ViewInfo viewInfo;
    viewInfo.state = state;
    if (state.isSRV())
    {
      D3D12_SHADER_RESOURCE_VIEW_DESC desc = state.asSRVDesc(img->getType(), img->isMultisampled());
      viewInfo.handle = resources.allocateTextureSRVDescriptor(device.get());
      device->CreateShaderResourceView(img->getHandle(), &desc, viewInfo.handle);
    }
    else if (state.isUAV())
    {
      D3D12_UNORDERED_ACCESS_VIEW_DESC desc = state.asUAVDesc(img->getType());
      viewInfo.handle = resources.allocateTextureSRVDescriptor(device.get());
      device->CreateUnorderedAccessView(img->getHandle(), nullptr, &desc, viewInfo.handle);
    }
    else if (state.isRTV())
    {
      D3D12_RENDER_TARGET_VIEW_DESC desc = state.asRTVDesc(img->getType(), img->isMultisampled());
      viewInfo.handle = resources.allocateTextureRTVDescriptor(device.get());
      device->CreateRenderTargetView(img->getHandle(), &desc, viewInfo.handle);
    }
    else if (state.isDSV())
    {
      D3D12_DEPTH_STENCIL_VIEW_DESC desc = state.asDSVDesc(img->getType(), img->isMultisampled());
      viewInfo.handle = resources.allocateTextureDSVDescriptor(device.get());
      device->CreateDepthStencilView(img->getHandle(), &desc, viewInfo.handle);
    }
    img->addView(viewInfo);
    if (is_swapchain_color_image(img))
    {
      context.addSwapchainView(img, img->getRecentView());
    }
  }
  else
  {
    img->updateRecentView(iter);
  }

  return img->getRecentView().handle;
}

Device::~Device() { shutdown({}); }

void Device::setupNullViews()
{
  {
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = 1;
    desc.Buffer.StructureByteStride = 4;
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    nullResourceTable.table[NullResourceTable::SRV_BUFFER] = resources.allocateBufferSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_BUFFER]);

    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MostDetailedMip = 0;
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.PlaneSlice = 0;
    desc.Texture2D.ResourceMinLODClamp = 0.f;
    nullResourceTable.table[NullResourceTable::SRV_TEX2D] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TEX2D]);

    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray.MostDetailedMip = 0;
    desc.Texture2DArray.MipLevels = 1;
    desc.Texture2DArray.FirstArraySlice = 0;
    desc.Texture2DArray.ArraySize = 1;
    desc.Texture2DArray.PlaneSlice = 0;
    desc.Texture2DArray.ResourceMinLODClamp = 0.f;
    nullResourceTable.table[NullResourceTable::SRV_TEX2D_ARRAY] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TEX2D_ARRAY]);

    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    desc.Texture3D.MostDetailedMip = 0;
    desc.Texture3D.MipLevels = 1;
    desc.Texture3D.ResourceMinLODClamp = 0.f;
    nullResourceTable.table[NullResourceTable::SRV_TEX3D] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TEX3D]);

    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    desc.TextureCube.MostDetailedMip = 0;
    desc.TextureCube.MipLevels = 1;
    desc.TextureCube.ResourceMinLODClamp = 0.f;
    nullResourceTable.table[NullResourceTable::SRV_TEX_CUBE] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TEX_CUBE]);

    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
    desc.TextureCubeArray.MostDetailedMip = 0;
    desc.TextureCubeArray.MipLevels = 1;
    desc.TextureCubeArray.First2DArrayFace = 0;
    desc.TextureCubeArray.NumCubes = 1;
    desc.TextureCubeArray.ResourceMinLODClamp = 0.f;
    nullResourceTable.table[NullResourceTable::SRV_TEX_CUBE_ARRAY] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TEX_CUBE_ARRAY]);

#ifdef D3D_HAS_RAY_TRACING
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    desc.RaytracingAccelerationStructure.Location = 0;
    nullResourceTable.table[NullResourceTable::SRV_TLAS] = resources.allocateBufferSRVDescriptor(device.get());
    device->CreateShaderResourceView(nullptr, &desc, nullResourceTable.table[NullResourceTable::SRV_TLAS]);
#endif
  }

  {
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = 1;
    desc.Buffer.StructureByteStride = 4;
    desc.Buffer.CounterOffsetInBytes = 0;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    nullResourceTable.table[NullResourceTable::UAV_BUFFER] = resources.allocateBufferSRVDescriptor(device.get());
    device->CreateUnorderedAccessView(nullptr, nullptr, &desc, nullResourceTable.table[NullResourceTable::UAV_BUFFER]);

    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    desc.Buffer.StructureByteStride = 0;
    nullResourceTable.table[NullResourceTable::UAV_BUFFER_RAW] = resources.allocateBufferSRVDescriptor(device.get());
    device->CreateUnorderedAccessView(nullptr, nullptr, &desc, nullResourceTable.table[NullResourceTable::UAV_BUFFER_RAW]);

    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;
    nullResourceTable.table[NullResourceTable::UAV_TEX2D] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateUnorderedAccessView(nullptr, nullptr, &desc, nullResourceTable.table[NullResourceTable::UAV_TEX2D]);

    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    desc.Texture2DArray.MipSlice = 0;
    desc.Texture2DArray.FirstArraySlice = 0;
    desc.Texture2DArray.ArraySize = 1;
    nullResourceTable.table[NullResourceTable::UAV_TEX2D_ARRAY] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateUnorderedAccessView(nullptr, nullptr, &desc, nullResourceTable.table[NullResourceTable::UAV_TEX2D_ARRAY]);

    desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    desc.Texture3D.MipSlice = 0;
    desc.Texture3D.FirstWSlice = 0;
    desc.Texture3D.WSize = 1;
    nullResourceTable.table[NullResourceTable::UAV_TEX3D] = resources.allocateTextureSRVDescriptor(device.get());
    device->CreateUnorderedAccessView(nullptr, nullptr, &desc, nullResourceTable.table[NullResourceTable::UAV_TEX3D]);
  }

  {
    D3D12_RENDER_TARGET_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;
    desc.Texture2D.PlaneSlice = 0;
    nullResourceTable.table[NullResourceTable::RENDER_TARGET_VIEW] = resources.allocateTextureRTVDescriptor(device.get());
    device->CreateRenderTargetView(nullptr, &desc, nullResourceTable.table[NullResourceTable::RENDER_TARGET_VIEW]);
  }

  {
    const uint32_t const_element_width = 4 * sizeof(float);
    // Effectively 64k
    const uint32_t max_const_buffer_width = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * const_element_width;
    // Use a dedicated buffer to simplify resource state management of the null buffer
    nullBuffer = createDedicatedBuffer(max_const_buffer_width, const_element_width, 1, DeviceMemoryClass::DEVICE_RESIDENT_BUFFER,
      D3D12_RESOURCE_FLAG_NONE, SBCF_BIND_CONSTANT, "Null Buffer");
    auto mem = allocateTemporaryUploadMemory(max_const_buffer_width, 1);
    memset(mem.pointer, 0, max_const_buffer_width);
    mem.flush();
    context.uploadToBuffer(nullBuffer, mem, 0);
    context.freeMemory(mem);
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    // Only needed when auto promote / decay is disabled, as the default state is copy-src / copy-dst and not
    // read all.
    D3D12_RESOURCE_STATES nullBufferState =
      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_COPY_SOURCE;
    context.transitionBuffer(nullBuffer, nullBufferState);
#endif
  }
}

void Device::configureFeatureCaps()
{
  caps.reset();
  D3D12_FEATURE_DATA_D3D12_OPTIONS2 opt2 = {};
  device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &opt2, sizeof(opt2));
  caps.set(Caps::DEPTH_BOUNDS_TEST, opt2.DepthBoundsTestSupported != FALSE);
}

#if DX12_USE_ESRAM
Image *Device::createEsramBackedImage(const ImageInfo &ii, Image *base_image, const char *name)
{
  if (!resources.hasESRam())
  {
    return nullptr;
  }

  ImageCreateResult result{};
  if (base_image)
  {
    result = resources.aliasESRamTexture(device.get(), ii, base_image, name);
  }
  else
  {
    result = resources.createESRamTexture(device.get(), ii, name);
  }

  if (result.image)
  {
    context.setImageResourceState(result.state, result.image->getGlobalSubresourceIdRange());
  }

  return result.image;
}
#endif

#if _TARGET_PC_WIN
bool Device::deviceUsesSoftwareRaytracing(const D3D12_FEATURE_DATA_D3D12_OPTIONS5 &op5)
{
  // NOTE: Currently this will incorrectly determine that SW is used with old drivers that predate T1.1 support for RTX device
  // this shouldn't be a problem though...
  // software implementation only supports tier 1.0, so do the simple check first
  if (D3D12_RAYTRACING_TIER_1_0 != op5.RaytracingTier)
  {
    return false;
  }
  DXGI_ADAPTER_DESC adapterDesc{};
  adapter->GetDesc(&adapterDesc);
  // only NV has a software implementation
  if (gpu::VENDOR_ID_NVIDIA != adapterDesc.VendorId)
  {
    return false;
  }
  // there is a more elaborate check that is 100% accurate, nv api can report the number of RT cores
  // if its 0, then its software mode if RaytracingTier is not D3D12_RAYTRACING_TIER_NOT_SUPPORTED.
  return true;
}

bool Device::createD3d12DeviceWithCheck(const Direct3D12Enviroment &d3d_env, D3D_FEATURE_LEVEL feature_level)
{
  return device.autoQuery([&d3d_env, this, feature_level](auto uuid, auto ptr) {
    const auto hr = d3d_env.D3D12CreateDevice(this->adapter.Get(), feature_level, uuid, ptr);
    if (FAILED(hr))
      logdbg("DX12: D3D12CreateDevice returned: %s (HRESULT: 0x%x)", dxgi_error_code_to_string(hr), hr);
    return SUCCEEDED(hr);
  });
}

bool Device::init(const Direct3D12Enviroment &d3d_env, debug::GlobalState &debug_state, ComPtr<DXGIFactory> &factory,
  AdapterInfo &&adapter_info, D3D_FEATURE_LEVEL feature_level, SwapchainCreateInfo swapchain_create_info, const Config &cfg,
  const DataBlock *dx_cfg)
{
  adapter = eastl::move(adapter_info.adapter);

  logdbg("DX12: D3D12CreateDevice...");
  bool isToolDevice = device.autoQuery([&debug_state, this, feature_level](auto uuid, auto ptr) {
    return debug_state.captureTool().tryCreateDevice(this->adapter.Get(), uuid, feature_level, ptr);
  });

  if (!isToolDevice)
  {
    isToolDevice = device.autoQuery([&debug_state, this, feature_level](auto uuid, auto ptr) {
      // Some debug tools require the device to be created with special API functions (e.g. AGS)
      return debug_state.postmortemTrace().tryCreateDevice(this->adapter.Get(), uuid, feature_level, ptr);
    });
  }
  if (isToolDevice)
    logdbg("DX12: Device has been created with debug tools");
  if (!isToolDevice && !createD3d12DeviceWithCheck(d3d_env, feature_level))
  {
    logdbg("DX12: Failed...");
    shutdown({});
    return false;
  }
  logdbg("DX12: ...Device has been successfully created");

  {
    D3D12_FEATURE_DATA_ARCHITECTURE data = {};
    if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &data, sizeof(data))))
      adapter_info.integrated = data.UMA;
  }

  config = update_config_for_vendor(cfg, dx_cfg, adapter_info);

  const bool validationLayerAvailable = debug::DeviceState::setup(debug_state, device.get(), d3d_env);

  if (validationLayerAvailable)
  {
    // When validation is enabled we should name objects to aid debugging
#if DX12_DOES_SET_DEBUG_NAMES
    logdbg("DX12: Detected enabled validation layer, enabling object naming");
    config.features.set(DeviceFeaturesConfig::NAME_OBJECTS);
#else
    logwarn("DX12: Detected enabled validation layer, but this build can not name objects!");
#endif
  }
  startDeviceErrorObserver(device.get());

  logdbg("DX12: Checking shader model...");
  // Right now we require shader model 6.0 as our shaders are DXIL and they are 6.0
  // and above. If we want to support 5.1 shader than the shader compiler has to
  // compile them into DXBC _and_ DXIL. This is possible but increases the shader
  // blob size significantly for little gain.
  D3D12_FEATURE_DATA_SHADER_MODEL sm = {D3D_SHADER_MODEL_6_0};
  device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm));
  if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_0)
  {
    logdbg("DX12: Failed, does not support shader model 6.0 or higher");
    // on debug builds we allow dxbc shaders to work with debug tools, dxbc has better support
#if DAGOR_DBGLEVEL > 0
    logdbg("DX12: Debug build, allowing target without DXIL support");
#else
    shutdown({});
    return false;
#endif
  }

  // initStreamline will hook the factory in order to inject it's own swapchain
  // which means this have to precede swapchain setup
  context.initStreamline(factory, adapter.Get());

  logdbg("DX12: Creating queues...");
  if (!queues.init(device.get(), shouldNameObjects()))
  {
    logdbg("DX12: Failed...");
    shutdown({});
    return false;
  }

  logdbg("DX12: Creating swapchain...");
  if (!context.back.swapchain.setup(*this, context.front.swapchain, factory.Get(), queues[DeviceQueueType::GRAPHICS].getHandle(),
        eastl::move(swapchain_create_info)))
  {
    logdbg("DX12: Failed...");
    shutdown({});
    return false;
  }

  driverVersion = get_driver_version_from_registry(adapter_info.info.AdapterLuid);

  DXGI_ADAPTER_DESC2 adapterDesc = {};
  adapter->GetDesc2(&adapterDesc);
  // Format table need some adjustments depending on hardware layout and features
  FormatStore::patchFormatTalbe(device.get(), adapterDesc.VendorId);

  configureFeatureCaps();

  auto resourceMemoryHeapSetup = cfg.memorySetup;
  resourceMemoryHeapSetup.adapter = adapter.Get();
  resourceMemoryHeapSetup.device = device.get();
  resources.setup(resourceMemoryHeapSetup);

  {
    SamplerState desc = SamplerState::make_default();
    defaultSampler = getSampler(desc);
    desc.isCompare = 1;
    defaultCmpSampler = getSampler(desc);
    nullResourceTable.table[NullResourceTable::SAMPLER] = defaultSampler;
    nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE] = defaultCmpSampler;
  }

  D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureVersion = {D3D_ROOT_SIGNATURE_VERSION_1_1};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureVersion, sizeof(rootSignatureVersion))))
  {
    rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1;
  }

  D3D12_SHADER_CACHE_SUPPORT_FLAGS cacheModes = D3D12_SHADER_CACHE_SUPPORT_NONE;
  cacheModes |= D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO;
  if (!config.features.test(DeviceFeaturesConfig::DISABLE_PIPELINE_LIBRARY_CACHE))
  {
    cacheModes |= D3D12_SHADER_CACHE_SUPPORT_LIBRARY;
  }
  // those modes are toggle able with configuration, because devices/drivers that report
  // support for those features seem to either not using it or the cache is pretty bad
  if (config.features.test(DeviceFeaturesConfig::ALLOW_OS_MANAGED_SHADER_CACHE))
  {
    cacheModes |= D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE;
    cacheModes |= D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE;
  }

  PipelineCache::SetupParameters pipelineCacheSetup;
  pipelineCacheSetup.device = device.get();
  pipelineCacheSetup.allowedModes = cacheModes;
  pipelineCacheSetup.fileName = CACHE_FILE_NAME;
  pipelineCacheSetup.deviceVendorID = adapterDesc.VendorId;
  pipelineCacheSetup.deviceID = adapterDesc.DeviceId;
  pipelineCacheSetup.subSystemID = adapterDesc.SubSysId;
  pipelineCacheSetup.revisionID = adapterDesc.Revision;
  pipelineCacheSetup.rawDriverVersion = driverVersion.raw;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  pipelineCacheSetup.rootSignaturesUsesCBVDescriptorRanges = rootSignaturesUsesCBVDescriptorRanges();
#endif

  pipelineCache.init(pipelineCacheSetup);

  PipelineManager::SetupParameters pipelineManagerSetup;
  pipelineManagerSetup.device = device.get();
  pipelineManagerSetup.serializeRootSignature = d3d_env.D3D12SerializeRootSignature;
  pipelineManagerSetup.rootSignatureVersion = rootSignatureVersion.HighestVersion;
  pipelineManagerSetup.pipelineCache = &pipelineCache;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  pipelineManagerSetup.rootSignaturesUsesCBVDescriptorRanges = rootSignaturesUsesCBVDescriptorRanges();
#endif

  pipeMan.init(pipelineManagerSetup);

#if !FIXED_EXECUTION_MODE
  DeviceContext::ExecutionMode execMode = DeviceContext::ExecutionMode::IMMEDIATE;
  if (config.features.test(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION))
  {
    execMode = DeviceContext::ExecutionMode::CONCURRENT;
  }

  context.initMode(execMode);
#else
  context.initMode();
#endif

  context.initFrameStates();
  context.makeReadyForFrame(context.front.frameIndex);
  context.initDLSS();
  context.initXeSS();

  context.initFsr2();
  nvlowlatency::init();

  // create null views to allow empty slots
  setupNullViews();

  return true;
}
#elif _TARGET_XBOX
#include "device_xbox.inl.cpp"
#endif

bool Device::isInitialized() const { return nullptr != device.get(); }

void Device::shutdown(const DeviceCapsAndShaderModel &features)
{
  if (!isInitialized())
    return;

  context.destroyBuffer(eastl::move(nullBuffer));

  nvlowlatency::close();

  frontendQueryManager.shutdownPredicate(context);

  context.shutdownFsr2();
  context.shutdownXess();

  context.shutdownSwapchain();
  context.wait();

  context.shutdownWorkerThread();
  context.shutdownFrameStates();

  pipeMan.unloadAll();

#if _TARGET_PC_WIN
  DXGI_ADAPTER_DESC2 adapterDesc = {};
  adapter->GetDesc2(&adapterDesc);

  PipelineCache::ShutdownParameters pipelineCacheSetup;
  pipelineCacheSetup.fileName = CACHE_FILE_NAME;
  pipelineCacheSetup.deviceVendorID = adapterDesc.VendorId;
  pipelineCacheSetup.deviceID = adapterDesc.DeviceId;
  pipelineCacheSetup.subSystemID = adapterDesc.SubSysId;
  pipelineCacheSetup.revisionID = adapterDesc.Revision;
  pipelineCacheSetup.rawDriverVersion = driverVersion.raw;
#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  pipelineCacheSetup.rootSignaturesUsesCBVDescriptorRanges = rootSignaturesUsesCBVDescriptorRanges();
#endif
  auto dxBlock = dgs_get_settings()->getBlockByNameEx("dx12");
  pipelineCacheSetup.features = features;
  // smNone is a indicator for default constructed, eg on error case
  if (features.shaderModel != d3d::smNone)
  {
    pipelineCacheSetup.generateBlks = dxBlock->getBool("generateCacheBlks", pipeMan.needToUpdateCache);
    pipelineCacheSetup.alwaysGenerateBlks = dxBlock->getBool("alwaysGenerateCacheBlks", pipeMan.needToUpdateCache);
  }
  else
  {
    pipelineCacheSetup.generateBlks = false;
    pipelineCacheSetup.alwaysGenerateBlks = false;
  }

  pipelineCache.shutdown(pipelineCacheSetup);
#else
  G_UNUSED(features);
#endif

  resources.shutdown(getDXGIAdapter(), &bindlessManager);
  debug::DeviceState::teardown();
  queues.shutdown();

#if _TARGET_PC_WIN
  stopDeviceErrorObserver(enterDeviceErrorObserverInShutdownMode());
#endif

  device.reset();
#if _TARGET_PC_WIN
  adapter.Reset();
#endif

  context.shutdownStreamline();
}

void Device::initializeBindlessManager()
{
  bindlessManager.init(context, {nullResourceTable.table[NullResourceTable::SAMPLER], SamplerState::make_default()},
    &nullResourceTable);
}

namespace gpu
{
uint32_t get_video_nvidia_version();
}

void Device::adjustCaps(Driver3dDesc &capabilities)
{
  capabilities.caps = {};
  capabilities.issues = {};


#if _TARGET_PC_WIN
  capabilities.caps.hasDepthReadOnly = true;
  capabilities.caps.hasStructuredBuffers = true;
  capabilities.caps.hasNoOverwriteOnShaderResourceBuffers = true;
  capabilities.caps.hasForcedSamplerCount = true;
  capabilities.caps.hasVolMipMap = true;
  capabilities.caps.hasAsyncCompute = false;
  capabilities.caps.hasOcclusionQuery = true;
  capabilities.caps.hasConstBufferOffset = true;
  capabilities.caps.hasDepthBoundsTest = hasDepthBoundsTest();
  capabilities.caps.hasConditionalRender = true;
  capabilities.caps.hasResourceCopyConversion = true;
  capabilities.caps.hasReadMultisampledDepth = false;
  capabilities.caps.hasInstanceID = true;
  capabilities.caps.hasQuadTessellation = true;
  capabilities.caps.hasGather4 = true;
  capabilities.caps.hasWellSupportedIndirect = true;
  capabilities.caps.hasNVApi = false;
  capabilities.caps.hasATIApi = false;
  capabilities.caps.hasAliasedTextures = true;
  capabilities.caps.hasResourceHeaps = true;
  capabilities.caps.hasBufferOverlapCopy = true;
  capabilities.caps.hasBufferOverlapRegionsCopy = true;
  capabilities.caps.hasUAVOnlyForcedSampleCount = true;
  capabilities.caps.hasNativeRenderPassSubPasses = false;
  capabilities.caps.hasDrawID = true;
  capabilities.caps.hasRenderPassDepthResolve = false;
  capabilities.caps.hasUAVOnEveryStage = true;

#if HAS_NVAPI
  // This is a bloody workaround for broken terrain tessellation.
  // All versions newer than 460.79 are affected.
  // TODO: remove this code snippet when those nvidia driver versions aren't supported anymore.
  const uint32_t version = gpu::get_video_nvidia_version();
  if (version >= 46079 && version < 46140)
  {
    capabilities.caps.hasQuadTessellation = false;
  }
#endif

  auto op0 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS>();
  auto op1 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS1>();
  // auto op2 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS2>();
  auto op3 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS3>();
  auto op4 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS4>();
  auto op5 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS5>();
  auto op6 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS6>();
  auto op7 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS7>();
  // auto op8 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS8>();
  auto op9 = checkFeatureSupport<D3D12_FEATURE_DATA_D3D12_OPTIONS9>();
  auto sm = get_shader_model(device.get());

  capabilities.shaderModel = shader_model_from_dx(sm.HighestShaderModel);
  logdbg("DX12: GPU has support for Shader Model %u.%u", capabilities.shaderModel.major, capabilities.shaderModel.minor);

  capabilities.caps.hasConservativeRassterization =
    D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED != op0.ConservativeRasterizationTier;

  capabilities.caps.hasTiled2DResources = D3D12_TILED_RESOURCES_TIER_1 <= op0.TiledResourcesTier;
  capabilities.caps.hasTiledMemoryAliasing = D3D12_TILED_RESOURCES_TIER_1 <= op0.TiledResourcesTier;
  capabilities.caps.hasTiledSafeResourcesAccess = D3D12_TILED_RESOURCES_TIER_2 <= op0.TiledResourcesTier;
  capabilities.caps.hasTiled3DResources = D3D12_TILED_RESOURCES_TIER_3 <= op0.TiledResourcesTier;
  // We don't support the limited max 128 SRVs per stage tier 1 supports.
  // Xbox always supports full heap of SRVs
  capabilities.caps.hasBindless = D3D12_RESOURCE_BINDING_TIER_2 <= op0.ResourceBindingTier;

  if (config.features.test(DeviceFeaturesConfig::DISABLE_BINDLESS))
  {
    capabilities.caps.hasBindless = false;
  }
  capabilities.caps.hasWaveOps = op1.WaveOps;
  capabilities.minWarpSize = op1.WaveLaneCountMin;
  capabilities.maxWarpSize = op1.WaveLaneCountMax;

  logdbg("GPU has tier %d for view instancing.", op3.ViewInstancingTier);
  capabilities.caps.castingFullyTypedFormatsSupported = op3.CastingFullyTypedFormatSupported;
  capabilities.caps.hasBasicViewInstancing = D3D12_VIEW_INSTANCING_TIER_1 <= op3.ViewInstancingTier;
  capabilities.caps.hasOptimizedViewInstancing = D3D12_VIEW_INSTANCING_TIER_2 <= op3.ViewInstancingTier;
  capabilities.caps.hasAcceleratedViewInstancing = D3D12_VIEW_INSTANCING_TIER_3 <= op3.ViewInstancingTier;

  capabilities.caps.hasShaderFloat16Support = op4.Native16BitShaderOpsSupported;

  caps.set(Caps::RAY_TRACING, D3D12_RAYTRACING_TIER_1_0 <= op5.RaytracingTier);
  caps.set(Caps::RAY_TRACING_T1_1, D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier);
  capabilities.caps.hasRayAccelerationStructure = D3D12_RAYTRACING_TIER_1_0 <= op5.RaytracingTier;
  capabilities.caps.hasRayQuery = D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier && (capabilities.shaderModel >= 6.5_sm);
  capabilities.caps.hasRayDispatch = D3D12_RAYTRACING_TIER_1_0 <= op5.RaytracingTier;
  capabilities.caps.hasIndirectRayDispatch = D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier;
  capabilities.caps.hasGeometryIndexInRayAccelerationStructure =
    D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier && (capabilities.shaderModel >= 6.5_sm);
  capabilities.caps.hasSkipPrimitiveTypeInRayTracingShaders =
    D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier && (capabilities.shaderModel >= 6.5_sm);
  capabilities.caps.hasNativeRayTracePipelineExpansion = D3D12_RAYTRACING_TIER_1_1 <= op5.RaytracingTier;
  capabilities.caps.hasPersistentShaderHandles = capabilities.caps.hasRayDispatch;

  if (D3D12_RAYTRACING_TIER_1_0 <= op5.RaytracingTier)
  {
    bool isSoftwareRT = deviceUsesSoftwareRaytracing(op5);
    bool allowSoftwareRT = dgs_get_settings()->getBlockByNameEx("dx12")->getBool("allowSoftwareRT", false);
    if (isSoftwareRT && !allowSoftwareRT)
    {
      caps.set(Caps::RAY_TRACING, false);
      caps.set(Caps::RAY_TRACING_T1_1, false);
      capabilities.caps.hasRayAccelerationStructure = false;
      capabilities.caps.hasRayQuery = false;
      capabilities.caps.hasRayDispatch = false;
      capabilities.caps.hasIndirectRayDispatch = false;
      capabilities.caps.hasGeometryIndexInRayAccelerationStructure = false;
      capabilities.caps.hasSkipPrimitiveTypeInRayTracingShaders = false;
      capabilities.caps.hasNativeRayTracePipelineExpansion = false;
      capabilities.caps.hasPersistentShaderHandles = false;
    }
  }

  capabilities.caps.hasVariableRateShading = D3D12_VARIABLE_SHADING_RATE_TIER_1 <= op6.VariableShadingRateTier;
  caps.set(Caps::SHADING_RATE_T1, D3D12_VARIABLE_SHADING_RATE_TIER_1 <= op6.VariableShadingRateTier);

  capabilities.caps.hasVariableRateShadingTexture = D3D12_VARIABLE_SHADING_RATE_TIER_2 <= op6.VariableShadingRateTier;
  capabilities.caps.hasVariableRateShadingShaderOutput = D3D12_VARIABLE_SHADING_RATE_TIER_2 <= op6.VariableShadingRateTier;
  capabilities.caps.hasVariableRateShadingCombiners = D3D12_VARIABLE_SHADING_RATE_TIER_2 <= op6.VariableShadingRateTier;
  caps.set(Caps::SHADING_RATE_T2, D3D12_VARIABLE_SHADING_RATE_TIER_2 <= op6.VariableShadingRateTier);

  capabilities.variableRateTextureTileSizeX = capabilities.variableRateTextureTileSizeY = op6.ShadingRateImageTileSize;

  capabilities.caps.hasVariableRateShadingBy4 = FALSE != op6.AdditionalShadingRatesSupported;

  capabilities.caps.hasMeshShader = D3D12_MESH_SHADER_TIER_1 <= op7.MeshShaderTier;

  capabilities.caps.hasShader64BitIntegerResources =
    (FALSE != op9.AtomicInt64OnTypedResourceSupported) && (capabilities.shaderModel >= 6.6_sm);

  capabilities.caps.hasDLSS =
    getContext().streamlineAdapter && getContext().streamlineAdapter->isDlssSupported() == nv::SupportState::Supported;
  capabilities.caps.hasXESS = getContext().getXessState() >= XessState::SUPPORTED;
#else
  xbox_adjust_caps<Caps>(capabilities, caps, config);
#endif

  capabilities.maxtexw = min(capabilities.maxtexw, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
  capabilities.maxtexh = min(capabilities.maxtexh, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
  capabilities.maxcubesize = min(capabilities.maxcubesize, D3D12_REQ_TEXTURECUBE_DIMENSION);
  capabilities.maxvolsize = min(capabilities.maxvolsize, D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
  capabilities.maxtexaspect = max(capabilities.maxtexaspect, 0);
  capabilities.maxtexcoord = min(capabilities.maxtexcoord, D3D12_VS_INPUT_REGISTER_COUNT);
  capabilities.maxsimtex = min(capabilities.maxsimtex, (int)dxil::MAX_T_REGISTERS);
  capabilities.maxvertexsamplers = min(capabilities.maxvertexsamplers, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT);
  capabilities.maxclipplanes = min(capabilities.maxclipplanes, 0);
  capabilities.maxstreams = min(capabilities.maxstreams, D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
  capabilities.maxstreamstr = min(capabilities.maxstreamstr, 0x10000);
  capabilities.maxvpconsts = min(capabilities.maxvpconsts, D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT);
  // do not overflow into negative numbers
  capabilities.maxprims = min(capabilities.maxprims, (int)min(0x7FFFFFFFull, 1llu << D3D12_REQ_DRAW_VERTEX_COUNT_2_TO_EXP));
  capabilities.maxvertind = min(capabilities.maxvertind, (int)min(0x7FFFFFFFull, 1llu << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP));
  capabilities.maxSimRT = min(capabilities.maxSimRT, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);
#if !_TARGET_XBOXONE
  capabilities.raytrace.topAccelerationStructureInstanceElementSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
  capabilities.raytrace.accelerationStructureBuildScratchBufferOffsetAlignment =
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
  capabilities.raytrace.maxRecursionDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;
#endif
}

uint64_t Device::getGpuTimestampFrequency()
{
  uint64_t ticksPerSecond = 1;
  // We only measure on graphics queue only
  DX12_CHECK_RESULT(queues[DeviceQueueType::GRAPHICS].getHandle()->GetTimestampFrequency(&ticksPerSecond));
  return ticksPerSecond;
}

int Device::getGpuClockCalibration(uint64_t *gpu, uint64_t *cpu, int *cpu_freq_type)
{
  if (cpu_freq_type)
    *cpu_freq_type = DRV3D_CPU_FREQ_TYPE_QPC;
  if (!DX12_CHECK_OK(queues[DeviceQueueType::GRAPHICS].getHandle()->GetClockCalibration(gpu, cpu)))
  {
    *gpu = 0;
    *cpu = 0;
    return 0;
  }
  return 1;
}
Image *Device::createImage(const ImageInfo &ii, Image *base_image, const char *name)
{
  if (!checkResourceCreateState("createImage", name))
  {
    return nullptr;
  }

  ImageCreateResult result;
  if (base_image)
  {
    result = resources.aliasTexture(getDXGIAdapter(), device.get(), ii, base_image, name);
  }
  else
  {
    result = resources.createTexture(getDXGIAdapter(), *this, ii, name);
  }
  if (result.image)
  {
    context.setImageResourceState(result.state, result.image->getGlobalSubresourceIdRange());
  }
  return result.image;
}

Texture *Device::wrapD3DTex(ID3D12Resource *tex_res, ResourceBarrier current_state, const char *name, int flg)
{
  if (!checkResourceCreateState("wrapD3DTex", name))
  {
    return nullptr;
  }
  if (!tex_res)
  {
    D3D_ERROR("DX12: Texture is nullptr in Device::wrapD3DTex (name: %s)", name ? name : "");
    return nullptr;
  }

  auto image = resources.adoptTexture(tex_res, name);
  if (!image)
  {
    return nullptr;
  }

  auto layers = image->getArrayLayers();

  BaseTex *tex = newTextureObject(layers.count() > 1 ? RES3D_ARRTEX : RES3D_TEX, flg);
  tex->image = image;
  G_ASSERT(1 << tex->image->getMsaaLevel() == get_sample_count(tex->cflg));

  auto &ext = image->getBaseExtent();
  tex->setParams(ext.width, ext.height, layers.count() > 1 ? layers.count() : ext.depth, image->getMipLevelRange().count(), name);
  tex->realMipLevels = image->getMipLevelRange().count();
  tex->fmt = image->getFormat();
  tex->memSize = tex->ressize();
  TEXQL_ON_ALLOC(tex);

  context.setImageResourceState(translate_texture_barrier_to_state(current_state, tex->fmt.isDepth()),
    image->getGlobalSubresourceIdRange());

  return tex;
}

BufferState Device::createBuffer(uint32_t size, uint32_t structure_size, uint32_t discard_count, DeviceMemoryClass memory_class,
  D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name)
{
  if (!checkResourceCreateState("createBuffer", name))
  {
    return {};
  }

  return resources.allocateBuffer(getDXGIAdapter(), *this, size, structure_size, discard_count, memory_class, flags, cflags, name,
    config.features.test(DeviceFeaturesConfig::DISABLE_BUFFER_SUBALLOCATION), shouldNameObjects());
}

BufferState Device::createDedicatedBuffer(uint32_t size, uint32_t structure_size, uint32_t discard_count,
  DeviceMemoryClass memory_class, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name)
{
  if (!checkResourceCreateState("createBuffer", name))
  {
    return {};
  }

  // Always disable buffer sub-allocation
  return resources.allocateBuffer(getDXGIAdapter(), *this, size, structure_size, discard_count, memory_class, flags, cflags, name,
    true, shouldNameObjects());
}

void Device::addBufferView(BufferState &buffer, BufferViewType view_type, BufferViewFormating formating, FormatStore format,
  uint32_t struct_size)
{
  if (BufferViewType::SRV == view_type)
  {
    if (BufferViewFormating::RAW == formating)
    {
      resources.createBufferRawSRV(device.get(), buffer);
    }
    else if (BufferViewFormating::STRUCTURED == formating)
    {
      resources.createBufferStructureSRV(device.get(), buffer, struct_size);
    }
    else
    {
      resources.createBufferTextureSRV(device.get(), buffer, format);
    }
  }
  else if (BufferViewType::UAV == view_type)
  {
    if (BufferViewFormating::RAW == formating)
    {
      resources.createBufferRawUAV(device.get(), buffer);
    }
    else if (BufferViewFormating::STRUCTURED == formating)
    {
      resources.createBufferStructureUAV(device.get(), buffer, struct_size);
    }
    else
    {
      resources.createBufferTextureUAV(device.get(), buffer, format);
    }
  }
  else
  {
    DAG_FATAL("DX12: Invalid input to Device::addBufferView");
    return;
  }
}

int Device::createPredicate() { return frontendQueryManager.createPredicate(*this, device.get()); }

void Device::deletePredicate(int name) { frontendQueryManager.deletePredicate(name); }

void Device::setTexName(Image *img, const char *name)
{
  if (!img || !name)
    return;

#if DX12_IMAGE_DEBUG_NAMES || DX12_DOES_SET_DEBUG_NAMES
  if (!shouldNameObjects())
    return;

#if DX12_IMAGE_DEBUG_NAMES
  img->setDebugName(name);
#endif
#if DX12_DOES_SET_DEBUG_NAMES
  if (img->getHandle())
  {
    wchar_t stringBuf[MAX_OBJECT_NAME_LENGTH];
    DX12_SET_DEBUG_OBJ_NAME(img->getHandle(), lazyToWchar(name, stringBuf, MAX_OBJECT_NAME_LENGTH));
  }
#endif
#else
  G_UNUSED(img);
  G_UNUSED(name);
#endif
}

ID3D12CommandQueue *drv3d_dx12::Device::getGraphicsCommandQueue() const { return queues[DeviceQueueType::GRAPHICS].getHandle(); }

D3DDevice *drv3d_dx12::Device::getDevice() { return device.get(); }

void Device::processEmergencyDefragmentation(uint32_t heap_group, bool is_alternate_heap_allowed, bool is_uav, bool is_rtv)
{
  if (!isHealthy())
    return;

  if (!is_main_thread())
    return;

  logwarn("DX12: Emergency defragmentation, wait for current work finish");

  context.wait();
  context.updateFenceProgress();

  logwarn("DX12: Emergency defragmentation is started");

  resources.processEmergencyDefragmentation(*this, getDXGIAdapter(), bindlessManager, heap_group, is_alternate_heap_allowed, is_uav,
    is_rtv);

  context.wait();
  context.updateFenceProgress();

  logdbg("DX12: Emergency defragmentation has been finished");
}

#if D3D_HAS_RAY_TRACING
RaytraceAccelerationStructure *Device::createRaytraceAccelerationStructure(RaytraceGeometryDescription *desc, uint32_t count,
  RaytraceBuildFlags flags, uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  dag::Vector<D3D12_RAYTRACING_GEOMETRY_DESC, framemem_allocator> geometryDesc;
  geometryDesc.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    geometryDesc.push_back(raytraceGeometryDescriptionToGeometryDesc(desc[i]).first);
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
  bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  bottomLevelInputs.NumDescs = geometryDesc.size();
  bottomLevelInputs.pGeometryDescs = geometryDesc.data();
  bottomLevelInputs.Flags = toAccelerationStructureBuildFlags(flags);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
  device.as<ID3D12Device5>()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
  G_ASSERT(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

  const bool updateable = !!(bottomLevelInputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE);

#if _TARGET_SCARLETT
  scarlett_update_bottom_level_prebuild_info(device, bottomLevelInputs, bottomLevelPrebuildInfo);
#endif

  build_scratch_size_in_bytes = bottomLevelPrebuildInfo.ScratchDataSizeInBytes;
  if (update_scratch_size_in_bytes)
  {
    if (updateable)
    {
      *update_scratch_size_in_bytes = bottomLevelPrebuildInfo.UpdateScratchDataSizeInBytes;
    }
    else
    {
      *update_scratch_size_in_bytes = 0;
    }
  }

  return resources.newRaytraceBottomAccelerationStructure(getDXGIAdapter(), *this, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes);
}

RaytraceAccelerationStructure *Device::createRaytraceAccelerationStructure(uint32_t size)
{
  return resources.newRaytraceBottomAccelerationStructure(getDXGIAdapter(), *this, size);
}

RaytraceAccelerationStructure *Device::createRaytraceAccelerationStructure(uint32_t elements, RaytraceBuildFlags flags,
  uint32_t &build_scratch_size_in_bytes, uint32_t *update_scratch_size_in_bytes)
{
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.NumDescs = elements;
  topLevelInputs.Flags = toAccelerationStructureBuildFlags(flags);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  device.as<ID3D12Device5>()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
  G_ASSERT(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

  bool updateable = !!(topLevelInputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE);

  build_scratch_size_in_bytes = topLevelPrebuildInfo.ScratchDataSizeInBytes;
  if (update_scratch_size_in_bytes)
  {
    if (updateable)
    {
      *update_scratch_size_in_bytes = topLevelPrebuildInfo.UpdateScratchDataSizeInBytes;
    }
    else
    {
      *update_scratch_size_in_bytes = 0;
    }
  }

  return resources.newRaytraceTopAccelerationStructure(getDXGIAdapter(), *this, topLevelPrebuildInfo.ResultDataMaxSizeInBytes);
}

#if _TARGET_PC_WIN
uint64_t Device::getAdapterLuid()
{
  DXGI_ADAPTER_DESC3 info3 = {};
  adapter->GetDesc3(&info3);
  return static_cast<uint64_t>(info3.AdapterLuid.LowPart) | (static_cast<uint64_t>(info3.AdapterLuid.HighPart) << 32ul);
}

Device::AdapterInfo Device::getAdapterInfo()
{
  Device::AdapterInfo info{};
  adapter->GetDesc1(&info.info);

  D3D12_FEATURE_DATA_ARCHITECTURE data = {};
  if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &data, sizeof(data))))
    info.integrated = data.UMA;

  return info;
}

void Device::enumerateDisplayModes(Tab<String> &list)
{
  clear_and_shrink(list);
  ScopedCommitLock ctxLock{context};
  if (auto output = context.back.swapchain.getOutput())
  {
    enumerateDisplayModesFromOutput(output, list);
  }
}
void Device::enumerateDisplayModesFromOutput(IDXGIOutput *dxgi_output, Tab<String> &list)
{
  static constexpr UINT MIN_MODE_WIDTH = 800;
  static constexpr UINT MIN_MODE_HEIGHT = 600;
  UINT numModes = 0;

  auto recommended_resolution = get_recommended_resolution(dxgi_output);

  if (SUCCEEDED(dxgi_output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &numModes, nullptr)))
  {
    dag::Vector<DXGI_MODE_DESC> displayModes(numModes);
    if (SUCCEEDED(dxgi_output->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &numModes, displayModes.data())))
    {
      displayModes.resize(numModes);
      // first drop modes that are to small
      displayModes.erase(eastl::remove_if(begin(displayModes), end(displayModes),
                           [](const DXGI_MODE_DESC &mode) //
                           { return mode.Width < MIN_MODE_WIDTH || mode.Height < MIN_MODE_HEIGHT; }),
        end(displayModes));
      // sort them to prep remove of duplicated modes
      eastl::sort(begin(displayModes), end(displayModes),
        [](const DXGI_MODE_DESC &l, const DXGI_MODE_DESC &r) //
        {
          if (l.Width < r.Width)
            return true;
          if (l.Width > r.Width)
            return false;
          return l.Height < r.Height;
        });
      // remove modes with same width and height
      displayModes.erase(eastl::unique(begin(displayModes), end(displayModes),
                           [](const DXGI_MODE_DESC &l, const DXGI_MODE_DESC &r) //
                           { return l.Width == r.Width && l.Height == r.Height; }),
        end(displayModes));

      if (recommended_resolution)
      {
        // remove modes larger than recommended mode
        displayModes.erase(eastl::remove_if(eastl::begin(displayModes), eastl::end(displayModes),
                             [&recommended_resolution](const DXGI_MODE_DESC &mode) {
                               return mode.Width > recommended_resolution->first || mode.Height > recommended_resolution->second;
                             }),
          eastl::end(displayModes));

        // remove modes with differect aspect ratio than recommended
        displayModes.erase(eastl::remove_if(eastl::begin(displayModes), eastl::end(displayModes),
                             [&recommended_resolution](const DXGI_MODE_DESC &mode) {
                               return !resolutions_have_same_ratio(*recommended_resolution, {mode.Width, mode.Height});
                             }),
          eastl::end(displayModes));
      }

      list.resize(displayModes.size());
      // turn into W x H string
      eastl::transform(begin(displayModes), end(displayModes), list.data(),
        [](const DXGI_MODE_DESC &mode) //
        { return String(64, "%d x %d", mode.Width, mode.Height); });
    }
  }
}

void Device::enumerateActiveMonitors(Tab<String> &result)
{
  clear_and_shrink(result);
  ComPtr<IDXGIOutput> activeOutput;
  for (uint32_t outputIndex = 0; SUCCEEDED(adapter->EnumOutputs(outputIndex, &activeOutput)); outputIndex++)
    result.push_back(get_monitor_name_from_output(activeOutput.Get()));
}

HRESULT Device::findClosestMatchingMode(DXGI_MODE_DESC *out_desc)
{
  DXGI_SWAP_CHAIN_DESC desc;
  HRESULT hr = context.getSwapchainDesc(&desc);
  if (SUCCEEDED(hr))
  {
    if (desc.BufferDesc.RefreshRate.Numerator == 0)
    {
      IDXGIOutput *dxgiOutput = context.getSwapchainOutput();
      if (dxgiOutput)
      {
        DXGI_MODE_DESC newModeDesc; // Maximum monitor refresh, not rounded, exact value requred for flip to work with vsync.
        hr = dxgiOutput->FindClosestMatchingMode(&desc.BufferDesc, &newModeDesc, NULL); // device.get()
        if (SUCCEEDED(hr))
          desc.BufferDesc = newModeDesc;
      }
    }
    if (out_desc)
      *out_desc = desc.BufferDesc;
  }
  return hr;
}
#endif

#endif

D3D12_FEATURE_DATA_FORMAT_SUPPORT Device::getFormatFeatures(FormatStore fmt)
{
  D3D12_FEATURE_DATA_FORMAT_SUPPORT query = {};
  if (!isHealthyOrRecovering()) // We don't want to wait for healthy state here. Otherwise there is a risk of an infinite loop.
  {
    return query;
  }
  query.Format = fmt.asDxGiFormat();
  device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &query, sizeof(query));
  // If this is true we also need to check the linear variant to accurately represent possible uses when in linear mode
  if (fmt.isSrgb && fmt.isSrgbCapableFormatType())
  {
    D3D12_FEATURE_DATA_FORMAT_SUPPORT query2 = {};
    query2.Format = fmt.asLinearDxGiFormat();
    device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &query2, sizeof(query2));
    query.Support1 |= query2.Support1;
    query.Support2 |= query2.Support2;
  }
  return query;
}

#if _TARGET_PC_WIN
LUID Device::preRecovery()
{
  // ensure we are in error state, might not be if the engine requested a reset.
  enterErrorState();
  resources.waitForTemporaryUsesToFinish();
  // error observer has to enter shutdown state or on some special cases preRecovery will lockout the observer from
  // reporting errors during wind down phase and result in a stuck thread, we wait to finish for.
  auto errorObserverShutdownToken = enterDeviceErrorObserverInShutdownMode();
  // also shuts down the swapchain
  nvlowlatency::close();
  context.preRecovery();
  for (auto &handler : deviceResetEventHandlers)
    handler->preRecovery();
  resources.preRecovery(getDXGIAdapter(), &bindlessManager);
  frontendQueryManager.preRecovery();
  debug::DeviceState::preRecovery();
  pipeMan.preRecovery();
  pipelineCache.preRecovery();
  queues.shutdown();
  stopDeviceErrorObserver(eastl::move(errorObserverShutdownToken));
  device.reset();
  DXGI_ADAPTER_DESC desc = {};
  adapter->GetDesc(&desc);
  adapter.Reset();
  context.preRecoverStreamline();
  bindlessManager.preRecovery();
  return desc.AdapterLuid;
}

bool Device::recover(const Direct3D12Enviroment &d3d_env, DXGIFactory *factory, ComPtr<DXGIAdapter> &&input_adapter,
  D3D_FEATURE_LEVEL feature_level, SwapchainCreateInfo &&sci, HWND wnd)
{
  enterRecoveringState();
  logdbg("DX12: Entering recovering state...");

  adapter = eastl::move(input_adapter);

  logdbg("DX12: D3D12CreateDevice...");
  if (!createD3d12DeviceWithCheck(d3d_env, feature_level))
  {
    logdbg("DX12: Failed...");
    enterErrorState();
    return false;
  }
  logdbg("DX12: ...Device has been successfully recreated");

  context.recoverStreamline(adapter.Get());

  debug::DeviceState::recover(device.get(), d3d_env);
  startDeviceErrorObserver(device.get());

  logdbg("DX12: Checking shader model...");
  // Right now we require shader model 6.0 as our shaders are DXIL and they are 6.0
  // and above. If we want to support 5.1 shader than the shader compiler has to
  // compile them into DXBC _and_ DXIL. This is possible but increases the shader
  // blob size significantly for little gain.
  D3D12_FEATURE_DATA_SHADER_MODEL sm = {D3D_SHADER_MODEL_6_0};
  device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm));
  if (sm.HighestShaderModel < D3D_SHADER_MODEL_6_0)
  {
    logdbg("DX12: Failed, does not support shader model 6.0 or higher");
    // on debug builds we allow dxbc shaders to work with debug tools, dxbc has better support
#if DAGOR_DBGLEVEL > 0
    logdbg("DX12: Debug build, allowing target without DXIL support");
#else
    enterErrorState();
    return false;
#endif
  }

  logdbg("DX12: Creating queues...");
  if (!queues.init(device.get(), shouldNameObjects()))
  {
    logdbg("DX12: Failed...");
    enterErrorState();
    return false;
  }

  logdbg("DX12: Creating swapchain...");
  // reconstruct from current state
  sci.window = wnd;
  sci.presentMode = context.front.swapchain.getPresentationMode();
  sci.windowed = !context.back.swapchain.isInExclusiveFullscreenMode();
  if (!context.back.swapchain.setup(*this, context.front.swapchain, factory, queues[DeviceQueueType::GRAPHICS].getHandle(),
        eastl::move(sci)))
  {
    logdbg("DX12: Failed...");
    enterErrorState();
    return false;
  }

  // debugState.setup(debug_state, device.get());
  DXGI_ADAPTER_DESC2 adapterDesc = {};
  adapter->GetDesc2(&adapterDesc);
  FormatStore::patchFormatTalbe(device.get(), adapterDesc.VendorId);

  configureFeatureCaps();

  ResourceMemoryHeap::SetupInfo resourceMemoryHeapSetup;
  resourceMemoryHeapSetup.adapter = adapter.Get();
  resourceMemoryHeapSetup.device = device.get();
  resourceMemoryHeapSetup.collectedMetrics.set();
  resources.setup(resourceMemoryHeapSetup);

  {
    SamplerState desc;
    desc.setMip(D3D12_FILTER_TYPE_LINEAR);
    desc.setFilter(D3D12_FILTER_TYPE_LINEAR);
    desc.setU(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setV(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setW(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
    desc.setBias(0.f);
    desc.setAniso(1.f);
    desc.setBorder(0);
    defaultSampler = getSampler(desc);
    desc.isCompare = 1;
    defaultCmpSampler = getSampler(desc);
    nullResourceTable.table[NullResourceTable::SAMPLER] = defaultSampler;
    nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE] = defaultCmpSampler;
  }

  D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSignatureVersion = {D3D_ROOT_SIGNATURE_VERSION_1_1};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSignatureVersion, sizeof(rootSignatureVersion))))
  {
    rootSignatureVersion.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1;
  }

  D3D12_SHADER_CACHE_SUPPORT_FLAGS cacheModes = D3D12_SHADER_CACHE_SUPPORT_NONE;
  cacheModes |= D3D12_SHADER_CACHE_SUPPORT_SINGLE_PSO;
  cacheModes |= D3D12_SHADER_CACHE_SUPPORT_LIBRARY;
  // those modes are toggle able with configuration, because devices/drivers that report
  // support for those features seem to either not using it or the cache is pretty bad
  if (config.features.test(DeviceFeaturesConfig::ALLOW_OS_MANAGED_SHADER_CACHE))
  {
    cacheModes |= D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_INPROC_CACHE;
    cacheModes |= D3D12_SHADER_CACHE_SUPPORT_AUTOMATIC_DISK_CACHE;
  }

  pipelineCache.recover(device.get(), cacheModes);

  pipeMan.recover(device.get(), pipelineCache);

  dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> unboundedTable;
  bindlessManager.visitSamplers([this, &unboundedTable](auto desc) { unboundedTable.push_back(getSampler(desc)); });
  context.recover(unboundedTable);
  frontendQueryManager.postRecovery(*this, device.get());

  nvlowlatency::init();

  for (auto &handler : deviceResetEventHandlers)
    handler->recovery();

  // create null views to allow empty slots
  setupNullViews();

  return true;
}

bool Device::finalizeRecovery()
{
  if (isDead())
    return false;
  // wait until the context has processed all our recovery actions and lets see if we where able
  // to recover on our part.
  context.wait();
  return enterHealthyState();
}
#endif

void Device::registerDeviceResetEventHandler(DeviceResetEventHandler *handler)
{
  ScopedCommitLock ctxLock{context};
  auto it = eastl::find(eastl::begin(deviceResetEventHandlers), eastl::end(deviceResetEventHandlers), handler);
  if (it == eastl::end(deviceResetEventHandlers))
    deviceResetEventHandlers.emplace_back(handler);
  else
    G_ASSERTF(false, "Device reset event handler is already registered.");
}

void Device::unregisterDeviceResetEventHandler(DeviceResetEventHandler *handler)
{
  ScopedCommitLock ctxLock{context};
  auto it = eastl::find(eastl::begin(deviceResetEventHandlers), eastl::end(deviceResetEventHandlers), handler);
  if (it != eastl::end(deviceResetEventHandlers))
    deviceResetEventHandlers.erase(it);
  else
    G_ASSERTF(false, "Could not find device reset event handler to unregister.");
}

void Device::updateTextureBindlessReferencesNoLock(BaseTex *tex, Image *old_image, bool ignore_previous_view)
{
  if (tex->image == nullptr)
  {
    logwarn("DX12: Texture (%s) has no image, resetting all bindless references...", tex->getResName());
    bindlessManager.resetTextureImagePairReferencesNoLock(context, tex, old_image);
    G_ASSERT(!bindlessManager.hasTextureReferenceNoLock(tex));
    return;
  }
  bindlessManager.updateTextureReferencesNoLock(context, tex, old_image, 0, eastl::numeric_limits<uint32_t>::max(),
    ignore_previous_view);
}

BufferState Device::placeBufferInHeap(::ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  return resources.placeBufferInHeap(getDXGIAdapter(), device.get(), heap, desc, offset, alloc_info, name);
}

Image *Device::placeTextureInHeap(::ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  auto result = resources.placeTextureInHeap(getDXGIAdapter(), device.get(), heap, desc, offset, alloc_info, name);
  if (result.image)
  {
    context.setImageResourceState(result.state, result.image->getGlobalSubresourceIdRange());
  }

  return result.image;
}

namespace
{
void load_config_into_memory_setup(ResourceMemoryHeap::SetupInfo &setup, const DataBlock *config)
{
  // setup defaults first
  setup.collectedMetrics.reset();

  if (!config)
  {
    return;
  }

  auto metricsConfig = config->getBlockByNameEx("metrics");
  if (!metricsConfig)
  {
    return;
  }

  bool default_state = metricsConfig->getBool("enable", false);

  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::EVENT_LOG, metricsConfig->getBool("event-log", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::METRIC_LOG, metricsConfig->getBool("metric-log", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::MEMORY_USE, metricsConfig->getBool("memory-use", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::HEAPS, metricsConfig->getBool("heaps", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::MEMORY, metricsConfig->getBool("memory", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::BUFFERS, metricsConfig->getBool("buffers", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::TEXTURES, metricsConfig->getBool("textures", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::CONST_RING, metricsConfig->getBool("const-ring", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::TEMP_RING, metricsConfig->getBool("temp-ring", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::TEMP_MEMORY, metricsConfig->getBool("temp-memory", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::PERSISTENT_UPLOAD,
    metricsConfig->getBool("persistent-upload", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::PERSISTENT_READ_BACK,
    metricsConfig->getBool("persistent-read-back", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::PERSISTENT_BIDIRECTIONAL,
    metricsConfig->getBool("persistent-bidirectional", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::SCRATCH_BUFFER, metricsConfig->getBool("scratch-buffer", default_state));
  setup.collectedMetrics.set(ResourceMemoryHeap::Metric::RAYTRACING, metricsConfig->getBool("raytracing", default_state));
}

static const char immediate_exection_name[] = "immediate";
static const char threaded_exection_name[] = "threaded";
static const char concurrent_execution_name[] = "concurrent";
bool is_threaded_command_execution(const char *name)
{
#if DAGOR_DBGLEVEL != 0
  static const char random_execution_name[] = "random";
  if (0 == strcmp(random_execution_name, name))
  {
    if (0 == (grnd() % 2))
    {
      logdbg("random execution mode: concurrent mode selected");
      return true;
    }
    else
    {
      logdbg("random execution mode: immediate mode selected");
      return false;
    }
  }
#endif
  if ((0 == strcmp(concurrent_execution_name, name)) || (0 == strcmp(threaded_exection_name, name)))
  {
    return true;
  }

  if (0 != strcmp(immediate_exection_name, name))
  {
    logwarn("DX12: config 'exectionMode' can only be %s, %s, or %s, but found %s", immediate_exection_name, threaded_exection_name,
      concurrent_execution_name, name);
  }
  return false;
}
} // namespace

Device::Config drv3d_dx12::get_device_config(const DataBlock *cfg)
{
  Device::Config result;
  result.features.set(DeviceFeaturesConfig::OPTIMIZE_BUFFER_UPLOADS, cfg->getBool("optimizeBufferUploads", true));
  // DX12 only officially allows render to multiple layers to vol tex with
  // layered render but mrt works for most devices anyways (dump restriction tbh...)
  // result.features.set(DeviceFeaturesConfig::DISABLE_RENDER_TO_3D_IMAGE,
  //  cfg->getBool("disableRenderTo3DImage", false));

  const char *config_exection = cfg->getStr("executionMode", concurrent_execution_name);
  result.features.set(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION, is_threaded_command_execution(config_exection));

  if (result.features.test(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION))
  {
    logdbg("DX12: concurrent execution mode selected");
  }
  else
  {
    logdbg("DX12: immediate execution mode selected");
  }

  result.features.set(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL,
    cfg->getBool("PipelineCompilationErrorIsFatal", false));
  result.features.set(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR,
    cfg->getBool("AssertOnPipelineCompilationError", false));

  bool enableDefragmentation = cfg->getBool("enableDefragmentation", true);
  if (enableDefragmentation && get_console_model() == ConsoleModel::XBOX_LOCKHART)
  {
    // TODO: https://youtrack.gaijin.team/issue/RE-1838
    enableDefragmentation = false;
    logdbg("DX12: Defragmentation has been disabled on Xbox Series S due to an issue.");
  }
  result.features.set(DeviceFeaturesConfig::ENABLE_DEFRAGMENTATION, enableDefragmentation);

  // measurements resulted in no benefit using it, pipeline library is always faster.
  result.features.set(DeviceFeaturesConfig::ALLOW_OS_MANAGED_SHADER_CACHE, cfg->getBool("allowOsManagedShaderCache", false));

#if _TARGET_SCARLETT
  result.features.set(DeviceFeaturesConfig::REPORT_WAVE_64, cfg->getBool("scarlettWave64CompatibilityMode", true));
#endif

#if DX12_CONFIGUREABLE_BARRIER_MODE
  result.features.set(DeviceFeaturesConfig::PROCESS_USER_BARRIERS,
    cfg->getBool("processUserBarriers", DX12_PROCESS_USER_BARRIERS_DEFAULT));
  result.features.set(DeviceFeaturesConfig::VALIDATE_USER_BARRIERS, cfg->getBool("validateUserBarriers", 0));
  result.features.set(DeviceFeaturesConfig::GENERATE_ALL_BARRIERS, cfg->getBool("generateAllBarriers", DX12_AUTOMATIC_BARRIERS));

  if (!result.features.test(DeviceFeaturesConfig::PROCESS_USER_BARRIERS))
  {
    // when we don't process user barriers, it makes no sense to validate anything, it will complain all the time
    result.features.set(DeviceFeaturesConfig::VALIDATE_USER_BARRIERS, false);
    // if user barriers is off, we need to generate all barriers otherwise we are getting corruption and crashes
    result.features.set(DeviceFeaturesConfig::GENERATE_ALL_BARRIERS, true);
  }
  if (!result.features.test(DeviceFeaturesConfig::GENERATE_ALL_BARRIERS))
  {
    // when we don't generate all barriers to compare the user barriers against, we can not validate them either
    result.features.set(DeviceFeaturesConfig::VALIDATE_USER_BARRIERS, false);
    // when we don't generate all barriers, then we have to use user barriers otherwise we are getting corruption and crashes
    result.features.set(DeviceFeaturesConfig::PROCESS_USER_BARRIERS, true);
  }
#endif

  load_config_into_memory_setup(result.memorySetup, cfg->getBlockByNameEx("memory"));

  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_BUFFERS, cfg->getBool("allowStreamBuffers", true));
  // only for constant buffer enabled by default as its the only variant that is using this "hack"
  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_CONST_BUFFERS, cfg->getBool("allowStreamConstBuffers", true));
  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_VERTEX_BUFFERS, cfg->getBool("allowStreamVertexBuffers", true));
  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_INDEX_BUFFERS, cfg->getBool("allowStreamIndexBuffers", true));
  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_INDIRECT_BUFFERS, cfg->getBool("allowStreamIndirectBuffers", true));
  result.features.set(DeviceFeaturesConfig::ALLOW_STREAM_STAGING_BUFFERS, cfg->getBool("allowStreamStagingBuffers", true));

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  result.features.set(DeviceFeaturesConfig::ROOT_SIGNATURES_USES_CBV_DESCRIPTOR_RANGES,
    cfg->getBool("rootSignaturesUsesCBVDescriptorRanges", false));
#endif

  result.features.set(DeviceFeaturesConfig::DISABLE_BINDLESS, cfg->getBool("disableBindless", false));

  result.features.set(DeviceFeaturesConfig::DISABLE_BUFFER_SUBALLOCATION, cfg->getBool("disableBufferSuballocation", false));

  result.features.set(DeviceFeaturesConfig::IGNORE_PREDICATION, cfg->getBool("ignorePredication", false));

  return result;
}

#if _TARGET_PC_WIN
Device::Config drv3d_dx12::update_config_for_vendor(Device::Config config, const DataBlock *cfg, Device::AdapterInfo &adapter_info)
{
  const char *defeault_mode = concurrent_execution_name;
  // intel randomly resets the device or has internal driver errors when using worker thread
  if (0x8086 == adapter_info.info.VendorId && adapter_info.integrated)
  {
    defeault_mode = immediate_exection_name;
  }
  const char *config_exection = cfg->getStr("executionMode", defeault_mode);
  config.features.set(DeviceFeaturesConfig::USE_THREADED_COMMAND_EXECUTION, is_threaded_command_execution(config_exection));
  return config;
}
#endif

uint32_t Device::getGpuMemUsageStats(uint64_t additionalVramUsageInBytes, uint32_t *out_mem_size, uint32_t *out_free_mem_kb,
  uint32_t *out_used_mem_kb)
{
#if _TARGET_PC_WIN
  uint64_t budget = 0;
  uint64_t available_budget = 0;

  if (resources.isUMASystem())
  {
    budget = resources.getDeviceLocalBudget() + resources.getHostLocalBudget();
    available_budget = resources.getDeviceLocalAvailablePoolBudget() + resources.getHostLocalAvailablePoolBudget();
  }
  else
  {
    budget = resources.getDeviceLocalBudget();
    available_budget = resources.getDeviceLocalAvailablePoolBudget();
  }
  if (available_budget > additionalVramUsageInBytes)
    available_budget -= additionalVramUsageInBytes;

  if (out_mem_size)
    *out_mem_size = uint32_t(budget >> 10);

  if (out_free_mem_kb)
    *out_free_mem_kb = uint32_t(available_budget >> 10);

  if (out_used_mem_kb)
    *out_used_mem_kb = budget > available_budget ? uint32_t((budget - available_budget) >> 10) : 0;

  return uint32_t(resources.getDeviceLocalPhysicalLimit() >> 10);
#else
  // on GDK everything is "device local".
  uint64_t budget = resources.getDeviceLocalBudget();
  uint64_t available_budget = resources.getDeviceLocalAvailablePoolBudget();

  if (out_mem_size)
    *out_mem_size = uint32_t(budget >> 10);

  if (out_free_mem_kb)
    *out_free_mem_kb = uint32_t(available_budget >> 10);

  if (out_used_mem_kb)
    *out_used_mem_kb = budget > available_budget ? uint32_t((budget - available_budget) >> 10) : 0;

  G_UNUSED(additionalVramUsageInBytes);

  // on console budget == HWE usable space
  return uint32_t(budget >> 10);
#endif
}

TextureTilingInfo Device::getTextureTilingInfo(BaseTex *tex, size_t subresource)
{
  TextureTilingInfo info = {};

  auto image = tex->getDeviceImage();
  G_ASSERTF_RETURN(image, info, "DX12: getTextureTilingInfo: image is nullptr for texture %p <%s>", tex, tex->getResName());
  if (ID3D12Resource *d3dTexture = image->getHandle())
  {
    UINT numTilesForEntireResource;
    D3D12_PACKED_MIP_INFO packedMipDesc;
    D3D12_TILE_SHAPE standardTileShapeForNonPackedMips;
    UINT numSubresourceTilings = 1;
    UINT firstSubresourceTilingToGet = subresource;
    D3D12_SUBRESOURCE_TILING subresourceTilingsForNonPackedMips;

    device->GetResourceTiling(d3dTexture, &numTilesForEntireResource, &packedMipDesc, &standardTileShapeForNonPackedMips,
      &numSubresourceTilings, firstSubresourceTilingToGet, &subresourceTilingsForNonPackedMips);

    info.totalNumberOfTiles = numTilesForEntireResource;
    info.numUnpackedMips = packedMipDesc.NumStandardMips;
    info.numPackedMips = packedMipDesc.NumPackedMips;
    info.numTilesNeededForPackedMips = packedMipDesc.NumTilesForPackedMips;
    info.firstPackedTileIndex = packedMipDesc.StartTileIndexInOverallResource;
    info.tileWidthInPixels = standardTileShapeForNonPackedMips.WidthInTexels;
    info.tileHeightInPixels = standardTileShapeForNonPackedMips.HeightInTexels;
    info.tileDepthInPixels = standardTileShapeForNonPackedMips.DepthInTexels;
    info.tileMemorySize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

    info.subresourceWidthInTiles = subresourceTilingsForNonPackedMips.WidthInTiles;
    info.subresourceHeightInTiles = subresourceTilingsForNonPackedMips.HeightInTiles;
    info.subresourceDepthInTiles = subresourceTilingsForNonPackedMips.DepthInTiles;
    info.subresourceStartTileIndex = subresourceTilingsForNonPackedMips.StartTileIndexInOverallResource;
  }

  return info;
}
#if !_TARGET_XBOXONE
drv3d_dx12::ShaderLibrary *Device::createShaderLibrary(const ::ShaderLibraryCreateInfo &info)
{
  auto d5 = device.as<ID3D12Device5>();
  if (!d5)
  {
    return nullptr;
  }
  return drv3d_dx12::ShaderLibrary::build(d5, info);
}
#endif

#if D3D_HAS_RAY_TRACING
#if _TARGET_PC_WIN
RayTracePipeline *Device::createPipeline(const Direct3D12Enviroment &env, const ::raytrace::PipelineCreateInfo &pci)
{
  if (!device.is<ID3D12Device5>())
  {
    return nullptr;
  }
  bool hasNativeExpand = false;
  auto pipe = eastl::make_unique<RayTracePipeline>();
  if (!pipe->build(device, hasNativeExpand, env.D3D12SerializeRootSignature, nullptr, pci))
  {
    return nullptr;
  }
  return pipe.release();
}
#else
RayTracePipeline *Device::createPipeline(const ::raytrace::PipelineCreateInfo &pci)
{
  auto pipe = eastl::make_unique<RayTracePipeline>();
  if (!pipe->build(device, true, D3D12SerializeRootSignature, nullptr, pci))
  {
    return nullptr;
  }
  return pipe.release();
}
#endif

RayTracePipeline *Device::expandPipeline(RayTracePipeline *base, const ::raytrace::PipelineExpandInfo &pei)
{
  auto d7 = device.as<ID3D12Device7>();
  if (!d7)
  {
    return nullptr;
  }
  return base->expand(d7, pei);
}
#endif
