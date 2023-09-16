#include "device.h"

using namespace drv3d_dx12;

#if DX12_REPORT_BINDLESS_HEAP_ACTIVITY
#define BINDLESS_REPORT debug
#else
#define BINDLESS_REPORT(...)
#endif

uint32_t frontend::BindlessManager::registerSampler(Device &device, DeviceContext &ctx, BaseTex *texture)
{
  auto sampler = texture->samplerState;

  uint32_t newIndex;
  {
    auto stateAccess = state.access();
    auto ref = eastl::find(begin(stateAccess->samplerTable), end(stateAccess->samplerTable), sampler);
    if (ref != end(stateAccess->samplerTable))
    {
      return static_cast<uint32_t>(ref - begin(stateAccess->samplerTable));
    }

    newIndex = static_cast<uint32_t>(stateAccess->samplerTable.size());
    stateAccess->samplerTable.push_back(sampler);
  }
  ctx.bindlessSetSamplerDescriptor(newIndex, device.getSampler(sampler));
  return newIndex;
}

void frontend::BindlessManager::unregisterBufferDescriptors(eastl::span<const D3D12_CPU_DESCRIPTOR_HANDLE> descriptors)
{
  auto stateAccess = state.access();
  for (auto descriptor : descriptors)
  {
    auto ref = eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo),
      [descriptor](auto &var) //
      {
        auto *bufferInfo = eastl::get_if<BufferSlotUsage>(&var);
        if (!bufferInfo)
        {
          return false;
        }
        return bufferInfo->descriptor == descriptor;
      });
    if (ref != end(stateAccess->resourceSlotInfo))
    {
      freeBindlessResourceRange(eastl::distance(begin(stateAccess->resourceSlotInfo), ref), 1);
      *ref = eastl::monostate{};
    }
  }
}

uint32_t frontend::BindlessManager::allocateBindlessResourceRange(uint32_t count)
{
  auto stateAccess = state.access();
  auto range = free_list_allocate_smallest_fit<uint32_t>(stateAccess->freeSlotRanges, count);
  if (range.empty())
  {
    auto r = stateAccess->resourceSlotInfo.size();
    stateAccess->resourceSlotInfo.resize(r + count, eastl::monostate{});
    return r;
  }
  else
  {
    return range.front();
  }
}

uint32_t frontend::BindlessManager::resizeBindlessResourceRange(DeviceContext &ctx, uint32_t index, uint32_t current_count,
  uint32_t new_count)
{
  if (new_count == current_count)
  {
    return index;
  }

  uint32_t newIndex;
  {
    auto stateAccess = state.access();
    uint32_t range_end = index + current_count;
    if (range_end == stateAccess->resourceSlotInfo.size())
    {
      // the range is in the end of the heap, so we just update the heap size
      stateAccess->resourceSlotInfo.resize(index + new_count, eastl::monostate{});
      return index;
    }

    if (free_list_try_resize(stateAccess->freeSlotRanges, make_value_range(index, current_count), new_count))
    {
      return index;
    }

    // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the
    // existing descriptors :/
    newIndex = allocateBindlessResourceRange(new_count);
    freeBindlessResourceRange(index, current_count);
  }
  ctx.bindlessCopyResourceDescriptors(index, newIndex, current_count);

  return newIndex;
}

void frontend::BindlessManager::freeBindlessResourceRange(uint32_t index, uint32_t count)
{
  auto stateAccess = state.access();
  G_ASSERTF_RETURN(index + count <= stateAccess->resourceSlotInfo.size(), ,
    "DX12: freeBindlessResourceRange tried to free out of range slots, range %u - "
    "%u, bindless count %u",
    index, index + count, stateAccess->resourceSlotInfo.size());
  eastl::fill_n(begin(stateAccess->resourceSlotInfo) + index, count, ResourceSlotInfoType{eastl::monostate{}});
  if (index + count != stateAccess->resourceSlotInfo.size())
  {
    free_list_insert_and_coalesce(stateAccess->freeSlotRanges, make_value_range(index, count));
    return;
  }

  stateAccess->resourceSlotInfo.resize(index, eastl::monostate{});
  // Now check if we can further shrink the heap
  if (!stateAccess->freeSlotRanges.empty() && (stateAccess->freeSlotRanges.back().stop == index))
  {
    stateAccess->resourceSlotInfo.resize(stateAccess->freeSlotRanges.back().start, eastl::monostate{});
    stateAccess->freeSlotRanges.pop_back();
  }
}

void frontend::BindlessManager::updateBindlessBuffer(DeviceContext &ctx, uint32_t index, Sbuffer *buffer)
{
  auto buf = (GenericBufferInterface *)buffer;
  auto descriptor = BufferResourceReferenceAndShaderResourceView{buf->getDeviceBuffer()}.srv;

  BufferSlotUsage info;
  info.buffer = buffer;
  info.descriptor = descriptor;

  {
    auto stateAccess = state.access();
    // if this slot already used the given descriptor, then we skip the update, no point in rewriting the same thing
    auto &slot = stateAccess->resourceSlotInfo[index];
    auto bufferInfo = eastl::get_if<BufferSlotUsage>(&slot);
    if (bufferInfo)
    {
      if (buffer == bufferInfo->buffer && descriptor == bufferInfo->descriptor)
      {
        return;
      }
    }
    slot = info;
  }

  ctx.bindlessSetResourceDescriptor(index, descriptor);
}

void frontend::BindlessManager::updateBindlessTexture(DeviceContext &ctx, uint32_t index, BaseTex *texture)
{
  auto view = texture->getViewInfo();
  auto image = texture->getDeviceImage();

  TextureSlotUsage info;
  info.texture = texture;
  info.image = image;
  info.view = view;

  {
    auto stateAccess = state.access();
    auto &slot = stateAccess->resourceSlotInfo[index];
    // if this slot already used the given view, then we skip the update, no point in rewriting the same thing
    auto imageInfo = eastl::get_if<TextureSlotUsage>(&slot);
    if (imageInfo)
    {
      if (texture == imageInfo->texture && image == imageInfo->image && view == imageInfo->view)
      {
        return;
      }
    }
    slot = info;
  }

  texture->setUsedWithBindless();

  ctx.bindlessSetResourceDescriptor(index, image, view);
}

frontend::BindlessManager::CheckTextureImagePairResultType frontend::BindlessManager::checkTextureImagePair(BaseTex *tex, Image *image)
{
  CheckTextureImagePairResultType result;
  // Find first entry with tex
  auto stateAccess = state.access();
  auto ref = eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [tex](auto &slot) {
    auto info = eastl::get_if<TextureSlotUsage>(&slot);
    return info && info->texture == tex;
  });
  if (ref != end(stateAccess->resourceSlotInfo))
  {
    auto &info = eastl::get<TextureSlotUsage>(*ref);
    result.firstFound = eastl::distance(begin(stateAccess->resourceSlotInfo), ref);
    // See if we are matching
    result.mismatchFound = (info.image != image);
  }
  if (!result.mismatchFound)
  {
    result.matchCount = 1;
    // This keeps looking until we reach the end or we found a mismatch, then we update the result
    // and stop
    for (++ref; ref != end(stateAccess->resourceSlotInfo); ++ref)
    {
      auto info = eastl::get_if<TextureSlotUsage>(&*ref);
      if (!info)
      {
        continue;
      }
      if (info->texture != tex)
      {
        continue;
      }
      if (info->image != image)
      {
        result.mismatchFound = true;
        break;
      }
      ++result.matchCount;
    }
  }

  return result;
}

void frontend::BindlessManager::updateTextureReferences(DeviceContext &ctx, BaseTex *tex, Image *old_image, Image *new_image,
  uint32_t search_offset, uint32_t change_count)
{
  for (; change_count; --change_count)
  {
    uint32_t index;
    ImageViewState view;
    {
      auto stateAccess = state.access();
      if (search_offset >= stateAccess->resourceSlotInfo.size())
      {
        break;
      }
      auto ref = eastl::find_if(begin(stateAccess->resourceSlotInfo) + search_offset, end(stateAccess->resourceSlotInfo),
        [tex, old_image](const auto &slot) {
          auto info = eastl::get_if<TextureSlotUsage>(&slot);
          if (!info)
          {
            return false;
          }
          return tex == info->texture && old_image == info->image;
        });
      if (end(stateAccess->resourceSlotInfo) == ref)
      {
        break;
      }
      view = eastl::get<TextureSlotUsage>(*ref).view;
      index = eastl::distance(begin(stateAccess->resourceSlotInfo), ref);
      search_offset = index + 1;
    }
    ctx.bindlessSetResourceDescriptor(index, new_image, view);
    {
      auto stateAccess = state.access();
      auto info = eastl::get_if<TextureSlotUsage>(&stateAccess->resourceSlotInfo[index]);
      if (info)
      {
        info->image = new_image;
      }
      else
      {
        // This should never happen but who knows...
        logwarn(
          "DX12: frontend::BindlessManager::updateTextureReferences slot %u was identified as texture slot and descriptor update "
          "was send to device context, but on later slot update it was no longer a texture slot!",
          index);
      }
    }
  }
}

void frontend::BindlessManager::updateBufferReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
  D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor)
{
  uint32_t nextIndex = 0;
  for (;;)
  {
    uint32_t index;
    {
      auto stateAccess = state.access();
      auto ref =
        eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [old_descriptor](const auto &slot) {
          auto info = eastl::get_if<BufferSlotUsage>(&slot);
          if (!info)
          {
            return false;
          }
          return old_descriptor == info->descriptor;
        });
      if (end(stateAccess->resourceSlotInfo) == ref)
      {
        break;
      }
      index = eastl::distance(begin(stateAccess->resourceSlotInfo), ref);
      nextIndex = index + 1;
    }
    ctx.bindlessSetResourceDescriptor(index, new_descriptor);
    {
      auto stateAccess = state.access();
      auto info = eastl::get_if<BufferSlotUsage>(&stateAccess->resourceSlotInfo[index]);
      if (info)
      {
        info->descriptor = new_descriptor;
      }
      else
      {
        // This should never happen but who knows...
        logwarn("DX12: frontend::BindlessManager::updateBufferReferences slot %u was identified as buffer slot and descriptor update "
                "was send to device context, but on later slot update it was no longer a buffer slot!",
          index);
      }
    }
  }
}

bool frontend::BindlessManager::hasTextureReference(BaseTex *tex)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [tex](auto &slot) {
           auto info = eastl::get_if<TextureSlotUsage>(&slot);
           return info && info->texture == tex;
         });
}

bool frontend::BindlessManager::hasTextureImageReference(BaseTex *tex, Image *image)
{
  auto stateAccess = state.access();

  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [tex, image](auto &slot) {
           auto info = eastl::get_if<TextureSlotUsage>(&slot);
           return info && info->texture == tex && info->image == image;
         });
}

bool frontend::BindlessManager::hasTextureImageViewReference(BaseTex *tex, Image *image, ImageViewState view)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [tex, image, view](auto &slot) {
           auto info = eastl::get_if<TextureSlotUsage>(&slot);
           return info && info->texture == tex && info->image == image && info->view == view;
         });
}

bool frontend::BindlessManager::hasImageReference(Image *image)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [image](auto &slot) {
           auto info = eastl::get_if<TextureSlotUsage>(&slot);
           return info && info->image == image;
         });
}

bool frontend::BindlessManager::hasImageViewReference(Image *image, ImageViewState view)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [image, view](auto &slot) {
           auto info = eastl::get_if<TextureSlotUsage>(&slot);
           return info && info->image == image && info->view == view;
         });
}

bool frontend::BindlessManager::hasBufferReference(Sbuffer *buf)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [buf](auto &slot) {
           auto info = eastl::get_if<BufferSlotUsage>(&slot);
           return info && info->buffer == buf;
         });
}

bool frontend::BindlessManager::hasBufferViewReference(Sbuffer *buf, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  auto stateAccess = state.access();
  return end(stateAccess->resourceSlotInfo) !=
         eastl::find_if(begin(stateAccess->resourceSlotInfo), end(stateAccess->resourceSlotInfo), [buf, view](auto &slot) {
           auto info = eastl::get_if<BufferSlotUsage>(&slot);
           return info && info->buffer == buf && info->descriptor == view;
         });
}

namespace
{
uint32_t res_type_to_null_resource_table_entry(int res_type)
{
  switch (res_type)
  {
    case RES3D_TEX: return NullResourceTable::SRV_TEX2D;
    case RES3D_CUBETEX: return NullResourceTable::SRV_TEX_CUBE;
    case RES3D_VOLTEX: return NullResourceTable::SRV_TEX3D;
    case RES3D_ARRTEX: return NullResourceTable::SRV_TEX2D_ARRAY;
    case RES3D_CUBEARRTEX: return NullResourceTable::SRV_TEX_CUBE_ARRAY;
    case RES3D_SBUF: return NullResourceTable::SRV_BUFFER;
  }
  G_ASSERT(!"DX12: Invalid RES3D_* value");
  return NullResourceTable::SRV_BUFFER;
}
} // namespace

void frontend::BindlessManager::onTextureDeletion(DeviceContext &ctx, BaseTex *texture, const NullResourceTable &null_table)
{
  auto descriptor = null_table.table[res_type_to_null_resource_table_entry(texture->restype())];
  uint32_t nextIndex = 0;
  for (;;)
  {
    uint32_t index;
    {
      auto stateAccess = state.access();
      auto ref = eastl::find_if(begin(stateAccess->resourceSlotInfo) + nextIndex, end(stateAccess->resourceSlotInfo),
        [texture](const auto &slot) {
          auto info = eastl::get_if<TextureSlotUsage>(&slot);
          if (!info)
          {
            return false;
          }
          return texture == info->texture;
        });
      if (end(stateAccess->resourceSlotInfo) == ref)
      {
        break;
      }
      index = eastl::distance(begin(stateAccess->resourceSlotInfo), ref);
      nextIndex = index + 1;
    }
    // reset slots descriptor to the corresponding null descriptor to make it safe to still read from
    // NOTE calls to ctx methods while having access to state may result in a deadlock, so we have to unlock first to send the command
    ctx.bindlessSetResourceDescriptor(index, descriptor);
    {
      auto stateAccess = state.access();
      // reset slot to empty, has to be done after we send the reset descriptor command, otherwise we risk overwriting valid
      // descriptors on a race condition
      stateAccess->resourceSlotInfo[index] = eastl::monostate{};
    }
  }
}

void frontend::BindlessManager::updateBindlessNull(DeviceContext &ctx, uint32_t resource_type, uint32_t index, uint32_t count,
  const NullResourceTable &null_table)
{
  {
    auto stateAccess = state.access();
    eastl::fill_n(stateAccess->resourceSlotInfo.begin() + index, count, eastl::monostate{});
  }
  for (uint32_t i = index; i < index + count; ++i)
  {
    ctx.bindlessSetResourceDescriptor(i, null_table.table[res_type_to_null_resource_table_entry(resource_type)]);
  }
}

void frontend::BindlessManager::preRecovery()
{
  auto stateAccess = state.access();
  for (auto &slot : stateAccess->resourceSlotInfo)
  {
    slot = eastl::monostate{};
  }
}

void backend::BindlessSetManager::init(ID3D12Device *device)
{
  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heapDesc.NumDescriptors = (SRV_HEAP_SIZE * 3) / 4; // TODO have a constant for this
  if (!DX12_CHECK_OK(device->CreateDescriptorHeap(&heapDesc, COM_ARGS(&resourceDescriptorHeap))))
  {
    return;
  }
  resourceDescriptorHeapRevision = 0;
  resourceDescriptorSize = 0;
  resourceDescriptorWidth = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  resourceDescriptorStart = resourceDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  heapDesc.NumDescriptors = SAMPLER_HEAP_SIZE;
  if (!DX12_CHECK_OK(device->CreateDescriptorHeap(&heapDesc, COM_ARGS(&samplerHeap))))
  {
    return;
  }
  samplerDescriptorSize = 0;
  samplerDescriptorWidth = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  samplerHeapStart = samplerHeap->GetCPUDescriptorHandleForHeapStart();
}

void backend::BindlessSetManager::shutdown()
{
  resourceDescriptorHeap.Reset();
  samplerHeap.Reset();
}

void backend::BindlessSetManager::preRecovery()
{
  resourceDescriptorHeap.Reset();
  samplerHeap.Reset();
}

void backend::BindlessSetManager::recover(ID3D12Device *device, const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> &bindless_samplers)
{
  init(device);
  // not very optimal, but good enough for now
  for (size_t i = 0; i < bindless_samplers.size(); ++i)
  {
    setSamplerDescriptor(device, i, bindless_samplers[i]);
  }
}

void backend::BindlessSetManager::setResourceDescriptor(ID3D12Device *device, uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  if (!resourceDescriptorHeap)
  {
    return;
  }
  resourceDescriptorSize = max(resourceDescriptorSize, slot + 1);
  ++resourceDescriptorHeapRevision;
  device->CopyDescriptorsSimple(1, resourceDescriptorStart + slot * resourceDescriptorWidth, descriptor,
    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  BINDLESS_REPORT("Bindless resource copied from %u to %u -> %u[%u]", descriptor.ptr,
    (resourceDescriptorStart + slot * resourceDescriptorWidth).ptr, resourceDescriptorStart.ptr, slot);
}

void backend::BindlessSetManager::copyResourceDescriptors(ID3D12Device *device, uint32_t src, uint32_t dst, uint32_t count)
{
  if (!resourceDescriptorHeap)
  {
    return;
  }
  resourceDescriptorSize = max(resourceDescriptorSize, dst + count);
  ++resourceDescriptorHeapRevision;
  device->CopyDescriptorsSimple(count, resourceDescriptorStart + dst * resourceDescriptorWidth,
    resourceDescriptorStart + src * resourceDescriptorWidth, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  BINDLESS_REPORT("%d bindless resources copied from %u to %u", count, (resourceDescriptorStart + src * resourceDescriptorWidth).ptr,
    (resourceDescriptorStart + dst * resourceDescriptorWidth).ptr);
}

void backend::BindlessSetManager::pushToResourceHeap(ID3D12Device *device, ShaderResourceViewDescriptorHeapManager &target)
{
  if (!resourceDescriptorHeap)
  {
    return;
  }
  target.updateBindlessSegment(device, resourceDescriptorStart, resourceDescriptorSize, resourceDescriptorHeapRevision);
}

void backend::BindlessSetManager::setSamplerDescriptor(ID3D12Device *device, uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  if (!samplerHeap)
  {
    return;
  }
  samplerDescriptorSize = max(samplerDescriptorSize, slot + 1);
  device->CopyDescriptorsSimple(1, samplerHeapStart + slot * samplerDescriptorWidth, descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  BINDLESS_REPORT("Bindless sampler copied from %u to %u -> %u[%u]", descriptor.ptr,
    (samplerHeapStart + slot * samplerDescriptorWidth).ptr, samplerHeapStart.ptr, slot);
}

void backend::BindlessSetManager::reserveSamplerHeap(ID3D12Device *device, SamplerDescriptorHeapManager &target)
{
  target.reserveBindlessSegmentSpace(device, samplerDescriptorSize);
}

void backend::BindlessSetManager::pushToSamplerHeap(ID3D12Device *device, SamplerDescriptorHeapManager &target)
{
  if (!samplerHeap)
  {
    return;
  }
  target.updateBindlessSegment(device, samplerHeapStart, samplerDescriptorSize);
}