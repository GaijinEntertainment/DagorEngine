// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "object_components.h"
#include <descriptor_heap.h>
#include <format_store.h>
#include <pipeline.h>

#include <EASTL/span.h>
#include <generic/dag_expected.h>


namespace drv3d_dx12::resource_manager
{
// Allocs and frees do not take a lock as for textures, view create and free is always already guarded
// by the context lock.
class TextureDescriptorProvider : public ImageObjectProvider
{
  using BaseType = ImageObjectProvider;

  static constexpr uint32_t DESCRIPTOR_BLOCK_SIZE = 1024;

  DescriptorHeap<ShaderResourceViewStagingPolicy<DESCRIPTOR_BLOCK_SIZE>> srvHeap;
  DescriptorHeap<RenderTargetViewPolicy> rtvHeap;
  DescriptorHeap<DepthStencilViewPolicy> dsvHeap;

public:
  struct SetupInfo : BaseType::SetupInfo
  {
    ID3D12Device *device;
  };

protected:
  TextureDescriptorProvider() = default;
  ~TextureDescriptorProvider() = default;
  TextureDescriptorProvider(const TextureDescriptorProvider &) = delete;
  TextureDescriptorProvider &operator=(const TextureDescriptorProvider &) = delete;
  TextureDescriptorProvider(TextureDescriptorProvider &&) = delete;
  TextureDescriptorProvider &operator=(TextureDescriptorProvider &&) = delete;

  void shutdown()
  {
    BaseType::shutdown();
    srvHeap.shutdown();
    rtvHeap.shutdown();
    dsvHeap.shutdown();
  }

  void preRecovery()
  {
    BaseType::preRecovery();
    srvHeap.shutdown();
    rtvHeap.shutdown();
    dsvHeap.shutdown();
  }

  void setup(const SetupInfo &info)
  {
    BaseType::setup(info);

    srvHeap.init(info.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    rtvHeap.init(info.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    dsvHeap.init(info.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
  }

public:
  using DescriptorAllocationResult = dag::Expected<D3D12_CPU_DESCRIPTOR_HANDLE, MemoryAllocationError>;

  DescriptorAllocationResult allocateTextureSRVDescriptor(ID3D12Device *device)
  {
    return srvHeap.allocate(device).or_else(
      [](HRESULT errorCode) -> DescriptorAllocationResult { return unexpected_memory_allocation_error(errorCode); });
  }
  void freeTextureSRVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor) { srvHeap.free(descriptor); }
  void freeTextureSRVDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors)
  {
    for (auto &&descriptor : descriptors)
    {
      srvHeap.free(descriptor);
    }
  }
  DescriptorAllocationResult allocateTextureRTVDescriptor(ID3D12Device *device)
  {
    return rtvHeap.allocate(device).or_else(
      [](HRESULT errorCode) -> DescriptorAllocationResult { return unexpected_memory_allocation_error(errorCode); });
  }
  void freeTextureRTVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor) { rtvHeap.free(descriptor); }
  void freeTextureRTVDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors)
  {
    for (auto &&descriptor : descriptors)
    {
      rtvHeap.free(descriptor);
    }
  }
  DescriptorAllocationResult allocateTextureDSVDescriptor(ID3D12Device *device)
  {
    return dsvHeap.allocate(device).or_else(
      [](HRESULT errorCode) -> DescriptorAllocationResult { return unexpected_memory_allocation_error(errorCode); });
  }
  void freeTextureDSVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor) { dsvHeap.free(descriptor); }
  void freeTextureDSVDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors)
  {
    for (auto &&descriptor : descriptors)
    {
      dsvHeap.free(descriptor);
    }
  }
};

class BufferDescriptorProvider : public TextureDescriptorProvider
{
  using BaseType = TextureDescriptorProvider;

public:
  using BufferViewDescriptorsResult = dag::Expected<eastl::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]>, MemoryAllocationError>;
  using BufferViewCreateResult = dag::Expected<void, MemoryAllocationError>;

private:
  static constexpr uint32_t DESCRIPTOR_BLOCK_SIZE = 1024;

  ContainerMutexWrapper<DescriptorHeap<ShaderResourceViewStagingPolicy<DESCRIPTOR_BLOCK_SIZE>>, OSSpinlock> srvHeap;

protected:
  BufferDescriptorProvider() = default;
  ~BufferDescriptorProvider() = default;
  BufferDescriptorProvider(const BufferDescriptorProvider &) = delete;
  BufferDescriptorProvider &operator=(const BufferDescriptorProvider &) = delete;
  BufferDescriptorProvider(BufferDescriptorProvider &&) = delete;
  BufferDescriptorProvider &operator=(BufferDescriptorProvider &&) = delete;

  void shutdown()
  {
    BaseType::shutdown();
    srvHeap.access()->shutdown();
  }

  void preRecovery()
  {
    BaseType::preRecovery();
    srvHeap.access()->shutdown();
  }

  void setup(const SetupInfo &info)
  {
    BaseType::setup(info);
    srvHeap.access()->init(info.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  }

  /// All or nothing: a half filled array would hand out null handles that are then used to create
  /// views against nothing.
  BufferViewDescriptorsResult createBufferSRVs(ID3D12Device *device, ID3D12Resource *buffer, uint32_t count,
    D3D12_SHADER_RESOURCE_VIEW_DESC desc)
  {
    auto descriptors = eastl::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(count);
    auto srvHeapAccess = srvHeap.access();
    uint32_t createdCount = 0;
    for (auto &descriptor : eastl::span{descriptors.get(), count})
    {
      auto allocationResult = srvHeapAccess->allocate(device);
      if (!allocationResult.has_value())
      {
        for (auto created : eastl::span{descriptors.get(), createdCount})
        {
          srvHeapAccess->free(created);
        }
        return unexpected_memory_allocation_error(allocationResult.error());
      }
      descriptor = allocationResult.value();
      device->CreateShaderResourceView(buffer, &desc, descriptor);
      ++createdCount;
      desc.Buffer.FirstElement += desc.Buffer.NumElements;
    }
    return descriptors;
  }

  BufferViewDescriptorsResult createBufferUAVs(ID3D12Device *device, ID3D12Resource *buffer, uint32_t count,
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc)
  {
    auto descriptors = eastl::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(count);
    auto srvHeapAccess = srvHeap.access();
    uint32_t createdCount = 0;
    for (auto &descriptor : eastl::span{descriptors.get(), count})
    {
      auto allocationResult = srvHeapAccess->allocate(device);
      if (!allocationResult.has_value())
      {
        for (auto created : eastl::span{descriptors.get(), createdCount})
        {
          srvHeapAccess->free(created);
        }
        return unexpected_memory_allocation_error(allocationResult.error());
      }
      descriptor = allocationResult.value();
      device->CreateUnorderedAccessView(buffer, nullptr, &desc, descriptor);
      ++createdCount;
      desc.Buffer.FirstElement += desc.Buffer.NumElements;
    }
    return descriptors;
  }

public:
  DescriptorAllocationResult allocateBufferSRVDescriptor(ID3D12Device *device)
  {
    return srvHeap.access()->allocate(device).or_else(
      [](HRESULT errorCode) -> DescriptorAllocationResult { return unexpected_memory_allocation_error(errorCode); });
  }
  void freeBufferSRVDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor) { srvHeap.access()->free(descriptor); }
  void freeBufferSRVDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors)
  {
    for (auto &&descriptor : descriptors)
    {
      srvHeap.access()->free(descriptor);
    }
  }

  BufferViewCreateResult createBufferTextureSRV(ID3D12Device *device, BufferState &buffer, FormatStore format)
  {
    G_ASSERTF(0 == (buffer.offset % format.getBytesPerPixelBlock()), "DX12: Offset %u has to be multiples of element size %u",
      buffer.offset, format.getBytesPerPixelBlock());
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = format.asDxGiFormat<true>();
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = buffer.offset / format.getBytesPerPixelBlock();
    desc.Buffer.NumElements = buffer.size / format.getBytesPerPixelBlock();
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    auto descriptors = createBufferSRVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }
    buffer.srvs = eastl::move(descriptors.value());
    return {};
  }

  BufferViewCreateResult createBufferStructureSRV(ID3D12Device *device, BufferState &buffer, uint32_t struct_size)
  {
    G_ASSERTF(0 == (buffer.offset % struct_size), "DX12: Offset %u has to be multiples of element size %u", buffer.offset,
      struct_size);
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = buffer.offset / struct_size;
    desc.Buffer.NumElements = buffer.size / struct_size;
    desc.Buffer.StructureByteStride = struct_size;
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    auto descriptors = createBufferSRVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }
    buffer.srvs = eastl::move(descriptors.value());
    return {};
  }

  BufferViewCreateResult createBufferRawSRV(ID3D12Device *device, BufferState &buffer)
  {
    // RAW has a 16 byte offset alignment rule
    if (buffer.discardCount > 1)
    {
      G_ASSERTF(0 == (buffer.size % 16), "DX12: Buffer size %u has to be multiples of 16", buffer.size);
    }
    G_ASSERTF(0 == (buffer.offset % 16), "DX12: Offset %u has to be multiples of 16", buffer.offset);
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Buffer.FirstElement = buffer.offset / 4;
    desc.Buffer.NumElements = buffer.size / 4;
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    auto descriptors = createBufferSRVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }
    buffer.srvs = eastl::move(descriptors.value());
    return {};
  }

  BufferViewCreateResult createBufferTextureUAV(ID3D12Device *device, BufferState &buffer, FormatStore format)
  {
    G_ASSERTF(0 == buffer.offset, "DX12: Buffers with offsets can't have UAVs");
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = format.asDxGiFormat<false>();
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = buffer.size / format.getBytesPerPixelBlock();
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.CounterOffsetInBytes = 0;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    auto descriptors = createBufferUAVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }
    buffer.uavs = eastl::move(descriptors.value());

    buffer.uavForClear.reset();
    return {};
  }

  BufferViewCreateResult createBufferStructureUAV(ID3D12Device *device, BufferState &buffer, uint32_t struct_size)
  {
    G_ASSERTF(0 == buffer.offset, "DX12: Buffers with offsets can't have UAVs");
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = buffer.size / struct_size;
    desc.Buffer.StructureByteStride = struct_size;
    desc.Buffer.CounterOffsetInBytes = 0;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    auto descriptors = createBufferUAVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }

    // need extra views for clearing that are formatted, DX12 does not allow clearing of
    // structured views
    desc.Format = DXGI_FORMAT_R32_UINT;
    desc.Buffer.FirstElement = 0; // -V1048
    desc.Buffer.NumElements = buffer.size / sizeof(uint32_t);
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.CounterOffsetInBytes = 0;           // -V1048
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // -V1048

    auto clearDescriptors = createBufferUAVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!clearDescriptors.has_value())
    {
      // Without the clear views currentClearUAV falls back to the structured view and the clear
      // commands reject it, so the buffer gets no UAV at all.
      freeBufferSRVDescriptors({descriptors.value().get(), descriptors.value().get() + buffer.discardCount});
      return dag::Unexpected{clearDescriptors.error()};
    }
    buffer.uavs = eastl::move(descriptors.value());
    buffer.uavForClear = eastl::move(clearDescriptors.value());
    return {};
  }

  BufferViewCreateResult createBufferRawUAV(ID3D12Device *device, BufferState &buffer)
  {
    G_ASSERTF(0 == buffer.offset, "DX12: Buffers with offsets can't have UAVs");
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = buffer.size / 4;
    desc.Buffer.StructureByteStride = 0;
    desc.Buffer.CounterOffsetInBytes = 0;
    desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    auto descriptors = createBufferUAVs(device, buffer.buffer, buffer.discardCount, desc);
    if (!descriptors.has_value())
    {
      return dag::Unexpected{descriptors.error()};
    }
    buffer.uavs = eastl::move(descriptors.value());
    return {};
  }
};
} // namespace drv3d_dx12::resource_manager
