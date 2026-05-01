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


void frontend::BindlessTypesValidatorImplementation::setTypesRange(D3DResourceType type, uint32_t index, uint32_t size)
{
  if (!isEnabled)
    return;

  for (uint32_t i = index; i < min(index + size, resourceTypes.size()); ++i)
  {
    if (resourceTypes[i].has_value())
    {
      D3D_CONTRACT_ERROR("DX12: Bindless slot %u is of type %d (expected %d)", i, static_cast<uint32_t>(*resourceTypes[i]),
        static_cast<uint32_t>(type));
    }
  }

  if (index + size > resourceTypes.size())
    resourceTypes.resize(index + size, eastl::nullopt);

  for (uint32_t i = index; i < index + size; ++i)
    resourceTypes[i] = type;
}

void frontend::BindlessTypesValidatorImplementation::resetTypesRange(uint32_t index, uint32_t size)
{
  if (!isEnabled)
    return;

  D3D_CONTRACT_ASSERTF_RETURN(index + size <= resourceTypes.size(), , "DX12: Bindless slot range %u - %u is out of bounds (size: %u)",
    index, index + size, resourceTypes.size());
  eastl::fill_n(resourceTypes.begin() + index, size, eastl::nullopt);
}

void frontend::BindlessTypesValidatorImplementation::checkTypesRange(D3DResourceType type, uint32_t index, uint32_t size) const
{
  G_UNUSED(type);

  if (!isEnabled)
    return;

  D3D_CONTRACT_ASSERTF(index + size <= resourceTypes.size(), "DX12: Bindless slot range %u - %u is out of bounds (size: %u)", index,
    index + size, resourceTypes.size());

  for (uint32_t i = index; i < min(resourceTypes.size(), index + size); ++i)
  {
    auto slotType = resourceTypes[i];
    D3D_CONTRACT_ASSERTF_CONTINUE(slotType, "DX12: Bindless slot %u is not allocated and does not match with the expected type %d", i,
      static_cast<uint32_t>(type));
    D3D_CONTRACT_ASSERTF(*slotType == type, "DX12: Bindless slot %u is of type %d (expected %d)", i, static_cast<uint32_t>(*slotType),
      static_cast<uint32_t>(type));
  }
}

void frontend::BindlessManager::init(DeviceContext &ctx, SamplerDescriptorAndState default_bindless_sampler,
  const NullResourceTable *null_resource_table, bool enable_types_validation)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  enableTypesValidation(enable_types_validation);

  state.resourceSlotInfo.reserve(::bindless::MAX_RESOURCE_INDEX_COUNT);

  state.samplerTableAllocated = 1;
  state.samplerTable[0] = default_bindless_sampler.state;

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

  const auto ed = &state.samplerTable[state.samplerTableAllocated];
  auto ref = eastl::find(state.samplerTable, ed, sampler);
  if (ref != ed)
  {
    return eastl::distance(state.samplerTable, ref);
  }
  if (state.samplerTableAllocated >= ::bindless::MAX_SAMPLER_INDEX_COUNT)
  {
    logwarn("DX12: Ran out of bindless sampler slots, returning default");
    return 0;
  }
  const uint32_t newIndex = state.samplerTableAllocated++;
  state.samplerTable[newIndex] = sampler;
  ctx.bindlessSetSamplerDescriptorNoLock(newIndex, device.getSampler(sampler));
  return newIndex;
}

uint32_t frontend::BindlessManager::registerSampler(DeviceContext &ctx, SamplerDescriptorAndState sampler)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  const auto ed = &state.samplerTable[state.samplerTableAllocated];
  auto ref = eastl::find(state.samplerTable, ed, sampler.state);
  if (ref != ed)
  {
    return eastl::distance(state.samplerTable, ref);
  }
  if (state.samplerTableAllocated >= ::bindless::MAX_SAMPLER_INDEX_COUNT)
  {
    logwarn("DX12: Ran out of bindlesss ampler slots, returning default");
    return 0;
  }

  const uint32_t newIndex = state.samplerTableAllocated++;
  state.samplerTable[newIndex] = sampler.state;
  ctx.bindlessSetSamplerDescriptorNoLock(newIndex, sampler.sampler);
  return newIndex;
}

void frontend::BindlessManager::resetBufferReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  const auto nullDescriptor = nullResourceTable->table[NullResourceTable::SRV_BUFFER];
  resetReferences<BufferSlotUsage>(ctx, nullDescriptor,
    [descriptor](const BufferSlotUsage &info) { return info.descriptor == descriptor; });
}

uint32_t frontend::BindlessManager::allocateBindlessResourceRange(D3DResourceType type, uint32_t count)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};
  auto start = allocateBindlessResourceRangeNoLock(count);
  setTypesRange(type, start, count);
  return start;
}

void frontend::BindlessManager::reportBindlessSlotsInfoNoLock()
{
  logdbg("DX12: Reporting bindless resource heap slots, heap has %u entries", state.resourceSlotInfo.size());
  size_t index = 0;
  while (index < state.resourceSlotInfo.size())
  {
    auto typeId = state.resourceSlotInfo[index].typeIndex();
    auto startIndex = index;
    for (; index < state.resourceSlotInfo.size(); ++index)
    {
      if (typeId != state.resourceSlotInfo[index].typeIndex())
      {
        break;
      }
    }
    if (state.resourceSlotInfo[startIndex].holdsAlternative<eastl::monostate>())
    {
      logdbg("DX12: [%u - %u] %u free", startIndex, index - 1, index - startIndex);
    }
    else if (state.resourceSlotInfo[startIndex].holdsAlternative<BufferSlotUsage>())
    {
      logdbg("DX12: [%u - %u] %u buffers:", startIndex, index - 1, index - startIndex);
      for (auto inspectIndex = startIndex; inspectIndex < index; ++inspectIndex)
      {
        auto info = state.resourceSlotInfo[inspectIndex].getIf<BufferSlotUsage>();
        if (!info)
        {
          D3D_ERROR("DX12: Bindless slot %u was expected to be of buffer type, but cast yielded nullptr", inspectIndex);
        }
        else
        {
          logdbg("DX12: [%u]: %p <%s>", inspectIndex, info->buffer, info->buffer ? info->buffer->getBufName() : "-");
        }
      }
    }
    else if (state.resourceSlotInfo[startIndex].holdsAlternative<TextureSlotUsage>())
    {
      logdbg("DX12: [%u - %u] %u textures:", startIndex, index - 1, index - startIndex);
      for (auto inspectIndex = startIndex; inspectIndex < index; ++inspectIndex)
      {
        auto info = state.resourceSlotInfo[inspectIndex].getIf<TextureSlotUsage>();
        if (!info)
        {
          D3D_ERROR("DX12: Bindless slot %u was expected to be of texture type, but cast yielded nullptr", inspectIndex);
        }
        else
        {
          logdbg("DX12: [%u]: %p <%s>", inspectIndex, info->texture, info->texture ? info->texture->getTexName() : "-");
        }
      }
    }
  }
}

void frontend::BindlessManager::reportBindlessSlotsAllocationInfoNoLock()
{
  logdbg("DX12: Bindless resource heap slot count is currently %u", state.resourceSlotInfo.size());
  logdbg("DX12: Bindless resource heap has %u free segments", state.freeSlotRanges.size());
  uint32_t totalFreeSlots = 0;
  for (auto r : state.freeSlotRanges)
  {
    logdbg("DX12: Free segment with %u slots, from %u to %u", r.size(), *r.begin(), *r.end());
    totalFreeSlots = r.size();
  }
  logdbg("DX12: Bindless resource heap total slots occupied %u, leaving %u slots until exhaustion", state.resourceSlotInfo.size(),
    state.resourceSlotInfo.size() - totalFreeSlots,
    ::bindless::MAX_RESOURCE_INDEX_COUNT - (state.resourceSlotInfo.size() - totalFreeSlots));
}

uint32_t frontend::BindlessManager::allocateBindlessResourceRangeNoLock(uint32_t count)
{
  auto range = free_list_allocate_smallest_fit<uint32_t>(state.freeSlotRanges, count);
  if (range.empty())
  {
    auto r = state.resourceSlotInfo.size();
    if ((r + count) > ::bindless::MAX_RESOURCE_INDEX_COUNT)
    {
      reportBindlessSlotsInfoNoLock();
      D3D_CONTRACT_ERROR("DX12: Out of bindless slots, asked for %u while limit is %u and %u is already allocated", count,
        ::bindless::MAX_RESOURCE_INDEX_COUNT, r);
      DAG_FATAL("DX12: Critical D3D contract violation, out of bindless slots, can not continue");
      return 0;
    }
    state.resourceSlotInfo.resize(r + count, eastl::monostate{});
    return r;
  }

  return range.front();
}

uint32_t frontend::BindlessManager::resizeBindlessResourceRange(D3DResourceType type, DeviceContext &ctx, uint32_t index,
  uint32_t current_count, uint32_t new_count)
{
  checkTypesRange(type, index, current_count);

  if (new_count == current_count)
  {
    return index;
  }

  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, current_count);

  const uint32_t rangeEnd = index + current_count;
  const uint32_t rangeExtendEnd = index + new_count;
  const uint32_t extraSpace = new_count - current_count;
  if ((rangeExtendEnd <= ::bindless::MAX_RESOURCE_INDEX_COUNT) && (rangeEnd == state.resourceSlotInfo.size()))
  {
    // the range is in the end of the heap, so we just update the heap size
    state.resourceSlotInfo.resize(rangeExtendEnd, eastl::monostate{});
    setTypesRange(type, rangeEnd, extraSpace);
    return index;
  }

  if (free_list_try_resize(state.freeSlotRanges, make_value_range(index, current_count), new_count))
  {
    setTypesRange(type, rangeEnd, extraSpace);
    return index;
  }

  // we are unable to expand the resource range, so we have to reallocate elsewhere and copy the
  // existing descriptors :/
  const uint32_t newIndex = allocateBindlessResourceRangeNoLock(new_count);
  copyBindlessResourceRangeNoLock(index, newIndex, current_count);
  ctx.bindlessCopyResourceDescriptorsNoLock(index, newIndex, current_count);
  freeBindlessResourceRangeNoLock(type, index, current_count);

  setTypesRange(type, newIndex, new_count);

  return newIndex;
}

void frontend::BindlessManager::freeBindlessResourceRange(D3DResourceType type, uint32_t index, uint32_t count)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  freeBindlessResourceRangeNoLock(type, index, count);
}

void frontend::BindlessManager::freeBindlessResourceRangeNoLock(D3DResourceType type, uint32_t index, uint32_t count)
{
  checkBindlessRangeAllocatedNoLock(index, count);
  checkTypesRange(type, index, count);
  resetTypesRange(index, count);

  eastl::fill_n(state.resourceSlotInfo.begin() + index, count, eastl::monostate{});
  if (index + count != state.resourceSlotInfo.size())
  {
    free_list_insert_and_coalesce(state.freeSlotRanges, make_value_range(index, count));
    return;
  }

  uint32_t newSize = index;
  // Now check if we can further shrink the heap
  if (!state.freeSlotRanges.empty() && (state.freeSlotRanges.back().stop == index))
  {
    newSize = state.freeSlotRanges.back().start;
    state.freeSlotRanges.pop_back();
  }
  state.resourceSlotInfo.resize(newSize, eastl::monostate{});
  // TODO: should send a command to the backend heap to shrink the size there too
}

void frontend::BindlessManager::copyBindlessResourceRangeNoLock(uint32_t src, uint32_t dst, uint32_t count)
{
  G_ASSERT(src + count <= state.resourceSlotInfo.size());
  G_ASSERT(dst + count <= state.resourceSlotInfo.size());

  eastl::copy_n(state.resourceSlotInfo.begin() + src, count, state.resourceSlotInfo.begin() + dst);
}

void frontend::BindlessManager::checkBindlessRangeAllocatedNoLock(uint32_t index, uint32_t count) const
{
  D3D_CONTRACT_ASSERT_LOG(count > 0, "DX12: Bindkess resource range of size 0 is not valid (index: %u)", index);
  D3D_CONTRACT_ASSERT_LOG(index + count <= state.resourceSlotInfo.size(),
    "DX12: Bindless slot is not allocated (index: %d, count: %d, total slots: %d)", index, count, state.resourceSlotInfo.size());
#if DAGOR_DBGLEVEL > 0 // redundant for release
  const auto it = eastl::find_if(begin(state.freeSlotRanges), end(state.freeSlotRanges),
    [index, count](const auto &range) { return range.overlaps(ValueRange{index, index + count}); });
  D3D_CONTRACT_ASSERTF(it == state.freeSlotRanges.end(),
    "DX12: Bindless slot is not allocated (index: %d, count: %d, free range found: %d-%d)", index, count,
    it != state.freeSlotRanges.end() ? it->start : 0, it != state.freeSlotRanges.end() ? it->stop : 0);
#endif
}

OSSpinlock &frontend::BindlessManager::get_state_guard() { return get_resource_binding_guard(); }

namespace
{
uint32_t DAGOR_NOINLINE res_type_to_null_resource_table_entry(D3DResourceType type)
{
  switch (type)
  {
    case D3DResourceType::TEX: return NullResourceTable::SRV_TEX2D;
    case D3DResourceType::CUBETEX: return NullResourceTable::SRV_TEX_CUBE;
    case D3DResourceType::VOLTEX: return NullResourceTable::SRV_TEX3D;
    case D3DResourceType::ARRTEX: return NullResourceTable::SRV_TEX2D_ARRAY;
    case D3DResourceType::CUBEARRTEX: return NullResourceTable::SRV_TEX_CUBE_ARRAY;
    case D3DResourceType::SBUF: return NullResourceTable::SRV_BUFFER;
  }
  G_ASSERT(!"DX12: Invalid D3DResourceType::* value");
  DAGOR_UNREACHABLE;
}
} // namespace

bool frontend::BindlessManager::updateBindlessBuffer(DeviceContext &ctx, uint32_t index, Sbuffer *buffer)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, 1);
  checkTypesRange(D3DResourceType::SBUF, index, 1);

  auto buf = (GenericBufferInterface *)buffer;
  buf->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedInBlindessHeap(); });
  auto descriptor = BufferResourceReferenceAndShaderResourceView{buf->getDeviceBuffer()}.srv;

  // if this slot already used the given descriptor, then we skip the update, no point in rewriting the same thing
  const auto bufferInfo = state.resourceSlotInfo.getIf<BufferSlotUsage>(index);
  if (bufferInfo && buffer == bufferInfo->buffer && descriptor == bufferInfo->descriptor)
  {
    return false;
  }
  state.resourceSlotInfo[index] = BufferSlotUsage{.buffer = buffer, .descriptor = descriptor};

  // may be possible during device reset events
  if (descriptor.ptr)
  {
    ctx.bindlessSetResourceDescriptorNoLock(index, descriptor);
  }

  return true;
}

void frontend::BindlessManager::updateBindlessBufferRange(DeviceContext &ctx, D3DResourceType type, uint32_t index,
  const dag::Span<Sbuffer *> &buffers)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, buffers.size());
  checkTypesRange(D3DResourceType::SBUF, index, buffers.size());

  for (uint32_t i = 0; i < buffers.size(); ++i)
  {
    if (buffers[i] == nullptr)
    {
      state.resourceSlotInfo[size_t(index) + i] = eastl::monostate{};
      ctx.bindlessSetResourceDescriptorNoLock(index + i, nullResourceTable->table[res_type_to_null_resource_table_entry(type)]);
      continue;
    }

    auto buffer = (GenericBufferInterface *)buffers[i];
    buffer->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedInBlindessHeap(); });
    auto descriptor = BufferResourceReferenceAndShaderResourceView{buffer->getDeviceBuffer()}.srv;

    // if this slot already used the given descriptor, then we skip the update, no point in rewriting the same thing
    const auto bufferInfo = state.resourceSlotInfo.getIf<BufferSlotUsage>(size_t(index) + i);
    if (bufferInfo && buffers[i] == bufferInfo->buffer && descriptor == bufferInfo->descriptor)
    {
      continue;
    }
    state.resourceSlotInfo[size_t(index) + i] = BufferSlotUsage{.buffer = buffers[i], .descriptor = descriptor};

    // may be possible during device reset events
    if (descriptor.ptr)
    {
      ctx.bindlessSetResourceDescriptorNoLock(index + i, descriptor);
    }
  }
}

bool frontend::BindlessManager::updateBindlessTexture(DeviceContext &ctx, uint32_t index, BaseTex *texture)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, 1);
  checkTypesRange(texture->getType(), index, 1);

  auto textureRes = texture;
  if (textureRes->isStub())
    textureRes = textureRes->getStubTex();

  texture->usedInBindless.test_and_set(dag::mo::relaxed);
  auto view = textureRes->getViewInfo();
  auto image = textureRes->getDeviceImage();

  // if this slot already used the given view, then we skip the update, no point in rewriting the same thing
  const auto imageInfo = state.resourceSlotInfo.getIf<TextureSlotUsage>(index);
  if (imageInfo && texture == imageInfo->texture && image == imageInfo->image && view == imageInfo->view)
    return false;

  state.resourceSlotInfo[index] = TextureSlotUsage{
    .texture = texture, // I think, we need to keep reference to the resource from args here to have opportunity to update it later.
    .image = image,
    .view = view,
  };

#if DAGOR_DBGLEVEL > 0
  texture->setWasUsed();
#endif

  if (image)
  {
    ctx.bindlessSetResourceDescriptorNoLock(index, image, view);
  }
  else
  {
    ctx.bindlessSetResourceDescriptorNoLock(index,
      nullResourceTable->table[res_type_to_null_resource_table_entry(textureRes->getType())]);
    logwarn("DX12: frontend::BindlessManager::updateBindlessTexture slot %u was updated to null descriptor (texture is not valid)",
      index);
  }

  return true;
}

void frontend::BindlessManager::updateBindlessTextureRange(DeviceContext &ctx, D3DResourceType type, uint32_t index,
  const dag::Span<BaseTex *> &textures)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, textures.size());
  checkTypesRange(type, index, textures.size());

  for (uint32_t i = 0; i < textures.size(); ++i)
  {
    if (textures[i] == nullptr)
    {
      state.resourceSlotInfo[size_t(index) + i] = eastl::monostate{};
      ctx.bindlessSetResourceDescriptorNoLock(index + i, nullResourceTable->table[res_type_to_null_resource_table_entry(type)]);
      continue;
    }

    auto texture = textures[i];
    if (texture->isStub())
      texture = texture->getStubTex();

    texture->usedInBindless.test_and_set(dag::mo::relaxed);
    auto view = texture->getViewInfo();
    auto image = texture->getDeviceImage();

    // if this slot already used the given view, then we skip the update, no point in rewriting the same thing
    const auto imageInfo = state.resourceSlotInfo.getIf<TextureSlotUsage>(size_t(index) + i);
    if (imageInfo && texture == imageInfo->texture && image == imageInfo->image && view == imageInfo->view)
      continue;

    state.resourceSlotInfo[size_t(index) + i] = TextureSlotUsage{
      .texture = texture, // I think, we need to keep reference to the resource from args here to have opportunity to update it later.
      .image = image,
      .view = view,
    };

#if DAGOR_DBGLEVEL > 0
    texture->setWasUsed();
#endif

    if (image)
    {
      ctx.bindlessSetResourceDescriptorNoLock(index + i, image, view);
    }
    else
    {
      ctx.bindlessSetResourceDescriptorNoLock(index + i,
        nullResourceTable->table[res_type_to_null_resource_table_entry(texture->getType())]);
      logwarn(
        "DX12: frontend::BindlessManager::updateBindlessTextureRange slot %u was updated to null descriptor (texture is not valid)",
        index + i);
    }
  }
}

frontend::BindlessManager::CheckTextureImagePairResultType frontend::BindlessManager::checkTextureImagePairNoLock(BaseTex *tex,
  Image *image)
{
  CheckTextureImagePairResultType result;
  if (!tex->usedInBindless.test(dag::mo::relaxed))
    return result;

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
  uint32_t search_offset, uint32_t change_max_count, bool deferred)
{
  if (!tex->usedInBindless.test(dag::mo::relaxed))
    return;

  const auto bot = state.resourceSlotInfo.beginOfStartingAt<TextureSlotUsage>(search_offset);
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  const auto newImage = tex->getDeviceImage();
  const auto newView = tex->getViewInfo();

  for (auto at = bot; change_max_count; --change_max_count, ++at)
  {
    at = eastl::find_if(at, eot, [tex, old_image](const auto &info) {
      const bool shouldUpdateImage = info.image == old_image || info.image == nullptr;
      return tex == info.texture && shouldUpdateImage;
    });
    if (eot == at)
      break;

    const auto index = eastl::distance(state.resourceSlotInfo.begin(), at.asUntyped());
    if (deferred)
      ctx.deferredBindlessSetResourceDescriptorNoLock(index, newImage, newView);
    else
      ctx.bindlessSetResourceDescriptorNoLock(index, newImage, newView);

    at->image = newImage;
    at->view = newView;
  }
}

bool frontend::BindlessManager::updateBufferReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor,
  D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor)
{
  bool foundAny = false;
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
    foundAny = true;
  }
  return foundAny;
}

bool frontend::BindlessManager::hasValidTextureReferenceNoLock(BaseTex *tex)
{
  if (!tex->usedInBindless.test(dag::mo::relaxed))
    return false;

  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [tex](const auto &info) { return info.texture == tex && info.image; });
}

bool frontend::BindlessManager::hasValidTextureReference(BaseTex *tex)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};
  return hasValidTextureReferenceNoLock(tex);
}

bool frontend::BindlessManager::hasTextureImageReference(BaseTex *tex, Image *image)
{
  if (!tex->usedInBindless.test(dag::mo::relaxed))
    return false;

  OSSpinlockScopedLock stateLock{get_state_guard()};
  const auto bot = state.resourceSlotInfo.beginOf<TextureSlotUsage>();
  const auto eot = state.resourceSlotInfo.endOf<TextureSlotUsage>();
  return eot != eastl::find_if(bot, eot, [tex, image](const auto &info) { return info.texture == tex && info.image == image; });
}

bool frontend::BindlessManager::hasTextureImageViewReference(BaseTex *tex, Image *image, ImageViewState view)
{
  if (!tex->usedInBindless.test(dag::mo::relaxed))
    return false;

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
  if (!texture->usedInBindless.test(dag::mo::relaxed))
    return;

  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};
  const auto nullDescriptor = nullResourceTable->table[res_type_to_null_resource_table_entry(texture->getType())];
  resetReferencesNoLock<TextureSlotUsage>(ctx, nullDescriptor, [texture](const auto &info) { return texture == info.texture; });
}

void frontend::BindlessManager::resetTextureImagePairReferencesNoLock(DeviceContext &ctx, BaseTex *texture, Image *image)
{
  if (!texture->usedInBindless.test(dag::mo::relaxed))
    return;

  const auto nullDescriptor = nullResourceTable->table[res_type_to_null_resource_table_entry(texture->getType())];
  resetReferencesNoLock<TextureSlotUsage>(ctx, nullDescriptor,
    [texture, image](const auto &info) { return texture == info.texture && image == info.image; });
}

void frontend::BindlessManager::updateBindlessNull(DeviceContext &ctx, D3DResourceType type, uint32_t index, uint32_t count)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};

  checkBindlessRangeAllocatedNoLock(index, count);

  eastl::fill_n(state.resourceSlotInfo.begin() + index, count, eastl::monostate{});
  for (uint32_t i = index; i < index + count; ++i)
  {
    ctx.bindlessSetResourceDescriptorNoLock(i, nullResourceTable->table[res_type_to_null_resource_table_entry(type)]);
  }
}

void frontend::BindlessManager::preRecovery()
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  eastl::fill(state.resourceSlotInfo.begin(), state.resourceSlotInfo.end(), eastl::monostate{});
  resetTypesValidation();
}

void frontend::BindlessManager::completeFrameRecording(DeviceContext &ctx)
{
  OSSpinlockScopedLock stateLock{get_state_guard()};

  {
    auto bufferAccess = state.pendingBufferFrees.access();
    for (auto descriptor : *bufferAccess)
    {
      removeReferencesNoLock<BufferSlotUsage>(ctx, nullResourceTable->table[NullResourceTable::SRV_BUFFER],
        [descriptor](const auto &info) { return descriptor == info.descriptor; });
    }
    bufferAccess->clear();
  }

  {
    auto textureAccess = state.pendingTextureFrees.access();
    for (auto texture : *textureAccess)
    {
      removeReferencesNoLock<TextureSlotUsage>(ctx,
        nullResourceTable->table[res_type_to_null_resource_table_entry(texture->getType())],
        [texture](const auto &info) { return texture == info.texture; });
    }
    textureAccess->clear();
  }
}

void frontend::BindlessManager::onBufferFree(D3D12_CPU_DESCRIPTOR_HANDLE buffer_descriptor)
{
  state.pendingBufferFrees.access()->push_back(buffer_descriptor);
}

void frontend::BindlessManager::onTextureFree(BaseTex *texture)
{
  if (!texture->usedInBindless.test(dag::mo::relaxed))
    return;
  state.pendingTextureFrees.access()->push_back(texture);
}

void frontend::BindlessManager::reportBindlessSlotsInfo()
{
  OSSpinlockScopedLock stateLock{get_state_guard()};
  reportBindlessSlotsInfoNoLock();
}

void frontend::BindlessManager::reportBindlessSlotsAllocationInfo()
{
  OSSpinlockScopedLock stateLock{get_state_guard()};
  reportBindlessSlotsAllocationInfoNoLock();
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
    at->reset();
  }
}

template <typename SlotType, typename CheckerType>
void frontend::BindlessManager::removeReferencesNoLock(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor,
  const CheckerType &checker)
{
  auto at = state.resourceSlotInfo.beginOf<SlotType>();
  const auto typedEnd = state.resourceSlotInfo.endOf<SlotType>();
  for (;; ++at)
  {
    at = eastl::find_if(at, typedEnd, checker);
    if (at == typedEnd)
      break;

    auto untypedAt = at.asUntyped();
    auto index = eastl::distance(state.resourceSlotInfo.begin(), untypedAt);

    ctx.bindlessSetResourceDescriptorNoLock(index, nullDescriptor);

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

template <typename SlotType, typename CheckerType>
void frontend::BindlessManager::removeReferences(DeviceContext &ctx, D3D12_CPU_DESCRIPTOR_HANDLE nullDescriptor,
  const CheckerType &checker)
{
  ScopedCommitLock ctxLock{ctx};
  OSSpinlockScopedLock stateLock{get_state_guard()};
  removeReferencesNoLock<SlotType>(ctx, nullDescriptor, checker);
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

void backend::BindlessSetManager::deferResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  deferredResourceDescriptors.push_back({slot, descriptor});
}

void backend::BindlessSetManager::applyDeferredResourceDescriptors(ID3D12Device *device)
{
  for (auto &[slot, descriptor] : deferredResourceDescriptors)
    setResourceDescriptor(device, slot, descriptor);
  deferredResourceDescriptors.clear();
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
  target.flushBindlessToHeaps(device, resourceDescriptorStart, resourceDescriptorSize, resourceDescriptorHeapRevision);
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

void backend::BindlessSetManager::pushToSamplerHeaps(ID3D12Device *device, SamplerDescriptorHeapManager &target)
{
  if (!samplerHeap)
  {
    return;
  }
  target.flushBindlessToHeaps(device, samplerHeapStart, samplerDescriptorSize);
}