// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bindless.h"
#include "device.h"
#include "device_context.h"
#include "frontend_state.h"


using namespace drv3d_dx12;

#if DX12_REPORT_BINDLESS_HEAP_ACTIVITY
#define BINDLESS_REPORT logdbg
#else
#define BINDLESS_REPORT(...)
#endif

void frontend::BindlessManager::init(DeviceContext &ctx, SamplerDescriptorAndState default_bindless_sampler,
  const NullResourceTable *null_resource_table)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  state.samplerTable.clear();
  state.samplerTable.push_back(default_bindless_sampler.state);

  const uint32_t defaultSamplerBindlessIdx = 0;
  ctx.bindlessSetSamplerDescriptorNoLock(defaultSamplerBindlessIdx, default_bindless_sampler.sampler);

  nullResourceTable = null_resource_table;
}

uint32_t frontend::BindlessManager::registerSampler(Device &device, DeviceContext &ctx, BaseTex *texture)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  auto sampler = texture->samplerState;

  uint32_t newIndex;
  auto ref = eastl::find(begin(state.samplerTable), end(state.samplerTable), sampler);
  if (ref != end(state.samplerTable))
    return static_cast<uint32_t>(ref - begin(state.samplerTable));

  newIndex = state.samplerTable.size();
  state.samplerTable.push_back(sampler);
  ctx.bindlessSetSamplerDescriptorNoLock(newIndex, device.getSampler(sampler));
  return newIndex;
}

uint32_t frontend::BindlessManager::registerSampler(DeviceContext &ctx, SamplerDescriptorAndState sampler)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  uint32_t newIndex;

  auto ref = eastl::find(begin(state.samplerTable), end(state.samplerTable), sampler.state);
  if (ref != end(state.samplerTable))
  {
    return static_cast<uint32_t>(ref - begin(state.samplerTable));
  }

  newIndex = state.samplerTable.size();
  state.samplerTable.push_back(sampler.state);

  ctx.bindlessSetSamplerDescriptorNoLock(newIndex, sampler.sampler);
  return newIndex;
}

void frontend::BindlessManager::resetBufferReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  const auto nullDescriptor = nullResourceTable->table[NullResourceTable::SRV_BUFFER];
  resetReferences<BufferSlotUsage>(ctx, nullDescriptor,
    [descriptor](const BufferSlotUsage &info) { return info.descriptor == descriptor; });
}

uint32_t frontend::BindlessManager::allocateBindlessResourceRange(uint32_t count)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  return allocateBindlessResourceRangeNoLock(count);
}

uint32_t frontend::BindlessManager::allocateBindlessResourceRangeNoLock(uint32_t count)
{
  auto range = free_list_allocate_smallest_fit<uint32_t>(state.freeSlotRanges, count);
  if (range.empty())
  {
    auto r = state.resourceSlotInfo.size();
    state.resourceSlotInfo.resize(r + count, eastl::monostate{});
    return r;
  }

  return range.front();
}

uint32_t frontend::BindlessManager::resizeBindlessResourceRange(DeviceContext &ctx, uint32_t index, uint32_t current_count,
  uint32_t new_count)
{
  if (new_count == current_count)
  {
    return index;
  }

  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, current_count);

  uint32_t newIndex;
  uint32_t rangeEnd = index + current_count;
  if (rangeEnd == state.resourceSlotInfo.size())
  {
    // the range is in the end of the heap, so we just update the heap size
    state.resourceSlotInfo.resize(index + new_count, eastl::monostate{});
    return index;
  }

  if (free_list_try_resize(state.freeSlotRanges, make_value_range(index, current_count), new_count))
  {
    return index;
  }

  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the
  // existing descriptors :/
  newIndex = allocateBindlessResourceRangeNoLock(new_count);
  freeBindlessResourceRangeNoLock(index, current_count);
  ctx.bindlessCopyResourceDescriptorsNoLock(index, newIndex, current_count);

  return newIndex;
}

void frontend::BindlessManager::freeBindlessResourceRange(uint32_t index, uint32_t count)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  freeBindlessResourceRangeNoLock(index, count);
}

void frontend::BindlessManager::freeBindlessResourceRangeNoLock(uint32_t index, uint32_t count)
{
  checkBindlessRangeAllocatedNoLock(index, count);
  eastl::fill_n(state.resourceSlotInfo.begin() + index, count, eastl::monostate{});
  if (index + count != state.resourceSlotInfo.size())
  {
    free_list_insert_and_coalesce(state.freeSlotRanges, make_value_range(index, count));
    return;
  }

  state.resourceSlotInfo.resize(index, eastl::monostate{});
  // Now check if we can further shrink the heap
  if (!state.freeSlotRanges.empty() && (state.freeSlotRanges.back().stop == index))
  {
    state.resourceSlotInfo.resize(state.freeSlotRanges.back().start, eastl::monostate{});
    state.freeSlotRanges.pop_back();
  }
}

void frontend::BindlessManager::checkBindlessRangeAllocatedNoLock(uint32_t index, uint32_t count) const
{
  G_UNUSED(index);
  G_UNUSED(count);
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(count > 0);
  G_ASSERTF(index + count <= state.resourceSlotInfo.size(),
    "DX12: Bindless slot is not allocated (index: %d, count: %d, total slots: %d)", index, count, state.resourceSlotInfo.size());
  const auto it = eastl::find_if(begin(state.freeSlotRanges), end(state.freeSlotRanges), [index, count](const auto &range) {
    return range.overlaps(ValueRange{index, index + count});
  });
  G_ASSERTF(it == state.freeSlotRanges.end(), "DX12: Bindless slot is not allocated (index: %d, count: %d, free range found: %d-%d)",
    index, count, it != state.freeSlotRanges.end() ? it->start : 0, it != state.freeSlotRanges.end() ? it->stop : 0);
#endif
}

OSSpinlock &frontend::BindlessManager::get_state_guard() { return get_resource_binding_guard(); }

void frontend::BindlessManager::updateBindlessBuffer(DeviceContext &ctx, uint32_t index, Sbuffer *buffer)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, 1);

  auto buf = (GenericBufferInterface *)buffer;
  auto descriptor = BufferResourceReferenceAndShaderResourceView{buf->getDeviceBuffer()}.srv;

  BufferSlotUsage info;
  info.buffer = buffer;
  info.descriptor = descriptor;

  // if this slot already used the given descriptor, then we skip the update, no point in rewriting the same thing
  const auto bufferInfo = state.resourceSlotInfo.getIf<BufferSlotUsage>(index);
  if (bufferInfo && buffer == bufferInfo->buffer && descriptor == bufferInfo->descriptor)
  {
    return;
  }
  state.resourceSlotInfo[index] = info;

  ctx.bindlessSetResourceDescriptorNoLock(index, descriptor);
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

void frontend::BindlessManager::updateBindlessTexture(DeviceContext &ctx, uint32_t index, BaseTex *texture)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, 1);

  auto textureRes = texture;
  if (textureRes->isStub())
    textureRes = textureRes->getStubTex();

  if (!textureRes->getDeviceImage())
  {
    state.resourceSlotInfo[index] = eastl::monostate{};
    ctx.bindlessSetResourceDescriptorNoLock(index,
      nullResourceTable->table[res_type_to_null_resource_table_entry(textureRes->restype())]);
    logwarn("DX12: frontend::BindlessManager::updateBindlessTexture slot %u was updated to null descriptor (texture is not valid)",
      index);
    return;
  }

  auto view = textureRes->getViewInfo();
  auto image = textureRes->getDeviceImage();

  TextureSlotUsage info;
  info.texture = texture; // I think, we need to keep reference to the resource from args here to have oportunity to update it later.
  info.image = image;
  info.view = view;

  // if this slot already used the given view, then we skip the update, no point in rewriting the same thing
  const auto imageInfo = state.resourceSlotInfo.getIf<TextureSlotUsage>(index);
  if (imageInfo && texture == imageInfo->texture && image == imageInfo->image && view == imageInfo->view)
  {
    return;
  }
  state.resourceSlotInfo[index] = info;

#if DAGOR_DBGLEVEL > 0
  texture->setWasUsed();
#endif

  ctx.bindlessSetResourceDescriptorNoLock(index, image, view);
}

frontend::BindlessManager::CheckTextureImagePairResultType frontend::BindlessManager::checkTextureImagePairNoLock(BaseTex *tex,
  Image *image)
{
  CheckTextureImagePairResultType result;
  // Find first entry with tex
  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  auto ref = eastl::find_if(bot, eot, [tex](const auto &info) { return info.texture == tex; });
  if (ref == eot)
  {
    return result;
  }

  result.firstFound = eastl::distance(state.resourceSlotInfo.begin(), ref.asUntyped());
  // See if we are matching
  result.mismatchFound = (ref->image != image);

  if (!result.mismatchFound)
  {
    result.matchCount = 1;
    // This keeps looking until we reach the end or we found a mismatch, then we update the result
    // and stop
    for (++ref; ref != eot; ++ref)
    {
      if (ref->texture != tex)
      {
        continue;
      }
      if (ref->image != image)
      {
        result.mismatchFound = true;
        break;
      }
      ++result.matchCount;
    }
  }

  return result;
}

void frontend::BindlessManager::updateTextureReferencesNoLock(DeviceContext &ctx, BaseTex *tex, Image *old_image,
  uint32_t search_offset, uint32_t change_max_count, bool ignore_previous_view)
{
  const auto bot = state.resourceSlotInfo.beginOfStartingAt<TextureSlotUsage>(search_offset);
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  const auto newImage = tex->getDeviceImage();
  for (auto at = bot; change_max_count; --change_max_count, ++at)
  {
    at = eastl::find_if(at, eot, [tex, old_image](const auto &info) { return tex == info.texture && old_image == info.image; });
    if (eot == at)
    {
      break;
    }
    const auto previousView = at->view;
    const auto index = eastl::distance(state.resourceSlotInfo.begin(), at.asUntyped());

    const auto adjustedPreviousView =
      adjust_previous_view(previousView, old_image->getMipLevelRange().count(), newImage->getMipLevelRange().count());
    const auto newView = ignore_previous_view ? tex->getViewInfo() : adjustedPreviousView;
    ctx.bindlessSetResourceDescriptorNoLock(index, newImage, newView);

    at->image = newImage;
    at->view = newView;
  }
}

void frontend::BindlessManager::updateBufferReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
  D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor)
{
  const auto bot = state.resourceSlotInfo.beginOf<BufferSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<BufferSlotUsage>();
  auto at = bot;
  for (;; ++at)
  {
    at = eastl::find_if(at, eot, [old_descriptor](const auto &info) { return old_descriptor == info.descriptor; });
    if (eot == at)
    {
      break;
    }
    const auto index = eastl::distance(state.resourceSlotInfo.begin(), at.asUntyped());

    ctx.bindlessSetResourceDescriptorNoLock(index, new_descriptor);

    at->descriptor = new_descriptor;
  }
}

bool frontend::BindlessManager::hasTextureReferenceNoLock(BaseTex *tex)
{
  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [tex](const auto &info) { return info.texture == tex; });
}

bool frontend::BindlessManager::hasTextureReference(BaseTex *tex)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};
  return hasTextureReferenceNoLock(tex);
}

bool frontend::BindlessManager::hasTextureImageReference(BaseTex *tex, Image *image)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [tex, image](const auto &info) { return info.texture == tex && info.image == image; });
}

bool frontend::BindlessManager::hasTextureImageViewReference(BaseTex *tex, Image *image, ImageViewState view)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot,
                  [tex, image, view](const auto &info) { return info.texture == tex && info.image == image && info.view == view; });
}

bool frontend::BindlessManager::hasImageReference(Image *image)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [image](const auto &info) { return info.image == image; });
}

bool frontend::BindlessManager::hasImageViewReference(Image *image, ImageViewState view)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [image, view](const auto &info) { return info.image == image && info.view == view; });
}

bool frontend::BindlessManager::hasBufferReference(Sbuffer *buf)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<BufferSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<BufferSlotUsage>();
  return eot != eastl::find_if(bot, eot, [buf](const auto &info) { return info.buffer == buf; });
}

bool frontend::BindlessManager::hasBufferViewReference(Sbuffer *buf, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<BufferSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<BufferSlotUsage>();
  return eot != eastl::find_if(bot, eot, [buf, view](const auto &info) { return info.buffer == buf && info.descriptor == view; });
}

bool frontend::BindlessManager::hasBufferViewReference(D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto bot = state.resourceSlotInfo.beginOf<BufferSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<BufferSlotUsage>();
  return eot != eastl::find_if(bot, eot, [view](const auto &info) { return info.descriptor == view; });
}

void frontend::BindlessManager::resetTextureReferences(DeviceContext &ctx, BaseTex *texture)
{
  const auto nullDescriptor = nullResourceTable->table[res_type_to_null_resource_table_entry(texture->restype())];
  resetReferences<TextureSlotUsage>(ctx, nullDescriptor, [texture](const auto &info) { return texture == info.texture; });
}

void frontend::BindlessManager::resetTextureImagePairReferencesNoLock(DeviceContext &ctx, BaseTex *texture, Image *image)
{
  const auto nullDescriptor = nullResourceTable->table[res_type_to_null_resource_table_entry(texture->restype())];
  resetReferencesNoLock<TextureSlotUsage>(ctx, nullDescriptor,
    [texture, image](const auto &info) { return texture == info.texture && image == info.image; });
}

void frontend::BindlessManager::updateBindlessNull(DeviceContext &ctx, uint32_t resource_type, uint32_t index, uint32_t count)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, count);

  eastl::fill_n(state.resourceSlotInfo.begin() + index, count, eastl::monostate{});
  for (uint32_t i = index; i < index + count; ++i)
  {
    ctx.bindlessSetResourceDescriptorNoLock(i, nullResourceTable->table[res_type_to_null_resource_table_entry(resource_type)]);
  }
}

void frontend::BindlessManager::preRecovery()
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  eastl::fill(state.resourceSlotInfo.begin(), state.resourceSlotInfo.end(), eastl::monostate{});
}

ImageViewState frontend::BindlessManager::adjust_previous_view(ImageViewState previous_view, int old_count, int new_count)
{
  const int newBaseMipIndex = clamp(static_cast<int>(previous_view.getMipBase().index()), 0, new_count - 1);
  const int previousLastMipInverseIndex = old_count - (previous_view.getMipBase().index() + previous_view.getMipCount());
  const int newLastMipIndex = clamp((new_count - 1) - previousLastMipInverseIndex, newBaseMipIndex, old_count - 1);
  const int newMipCount = newLastMipIndex - newBaseMipIndex + 1;
  auto adjustedView = previous_view;
  adjustedView.setMipMapRange(MipMapIndex::make(newBaseMipIndex), newMipCount);
  return adjustedView;
}

template <typename SlotType, typename CheckerType>
void frontend::BindlessManager::resetReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor,
  const CheckerType &checker)
{
  auto at = state.resourceSlotInfo.beginOf<SlotType>();
  const auto typedEnd = state.resourceSlotInfo.endOf<SlotType>();
  for (;; ++at)
  {
    at = eastl::find_if(at, typedEnd, checker);
    if (at == typedEnd)
    {
      break;
    }
    auto untypedAt = at.asUntyped();
    auto index = eastl::distance(state.resourceSlotInfo.begin(), untypedAt);

    // reset slots descriptor to the corresponding null descriptor to make it safe to still read from
    // NOTE calls to ctx methods while having access to state may result in a deadlock, so we have to unlock first to send the command
    ctx.bindlessSetResourceDescriptorNoLock(index, nullDescriptor);

    // reset slot to empty, has to be done after we send the reset descriptor command, otherwise we risk overwriting valid
    // descriptors on a race condition
    *untypedAt = eastl::monostate{};
  }
}

template <typename SlotType, typename CheckerType>
void frontend::BindlessManager::resetReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor,
  const CheckerType &checker)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};
  resetReferencesNoLock<SlotType>(ctx, nullDescriptor, checker);
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

void backend::BindlessSetManager::recover(ID3D12Device *device, const dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> &bindless_samplers)
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