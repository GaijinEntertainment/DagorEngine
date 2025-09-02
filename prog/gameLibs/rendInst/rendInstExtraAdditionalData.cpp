// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendIntsExtraAdditionalData.h>

#include <debug/dag_assert.h>
#include <scene/dag_scene.h>
#include <riGen/riExtraPool.h>
#include <riGen/riGenExtra.h>
#include <3d/dag_lockSbuffer.h>
#include <workCycle/dag_workCycle.h>

using namespace rendinst;

static rendinst::RendinstTiledScene *get_tiled_scene(RiExtraAdditionalDataManager::InstanceId instance_id, scene::node_index &ni)
{
  if (instance_id == RiExtraAdditionalDataManager::INVALID_INSTANCE)
    return nullptr;
  const uint32_t sceneId = instance_id >> 30;
  ni = instance_id & 0x3FFFFFFF;
  auto *scene = &rendinst::riExTiledScenes[sceneId];
  return scene;
}

void RiExtraAdditionalDataManager::clearAll()
{
  WinAutoLock lock(mutex);
  riOffsets.clear();
  riOptionalFlags.clear();
  sortedRiInfos.clear();
  prevTmData.clear();
  initialTmPosData.clear();
  initialTm3x3Data.clear();
  destrRenderData.clear();
  lastProcessFrameId = 0;
  lastFrameIdChecked = 0;
  isDirty = false;
  markModified();
}

RiExtraAdditionalDataManager::InstanceId RiExtraAdditionalDataManager::get_instance_id(uint32_t scene_id, uint32_t node_id)
{
  G_ASSERT((scene_id & 0b11) == scene_id);
  G_ASSERT((node_id & 0x3FFFFFFF) == node_id);
  return (scene_id << 30) | node_id;
}

void RiExtraAdditionalDataManager::transfer(InstanceId old_instance, InstanceId new_instance)
{
  WinAutoLock lock(mutex);

  G_ASSERT(riOffsets.count(new_instance) == 0);
  G_ASSERT(riOptionalFlags.count(new_instance) == 0);

  markModified();

  auto oldEntryId = findRIEntry(old_instance);
  if (oldEntryId != ~0u)
    dataToInstance[sortedRiInfos[oldEntryId].dataId] = new_instance;

  if (auto riOffsetsItr = riOffsets.find(old_instance); riOffsetsItr != riOffsets.end())
  {
    uint32_t offset = riOffsetsItr->second;
    uint32_t entryId = findRIEntryByOffset(offset);
    riOffsets.erase(riOffsetsItr);
    riOffsets[new_instance] = offset;
    if (entryId < sortedRiInfos.size())
      sortedRiInfos[entryId].id = new_instance;
    auto moveData = [&](rendinst::RiExtraPerInstanceDataType data_flag, auto &map) {
      if (sortedRiInfos[entryId].usedDataTypes & static_cast<uint32_t>(data_flag))
      {
        if (auto itr = map.find(old_instance); itr != map.end())
        {
          auto dataCopy = eastl::move(itr->second);
          map.erase(itr);
          map[new_instance] = dataCopy;
        }
      }
    };
    moveData(rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA, prevTmData);
    moveData(rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA, destrRenderData);
    moveData(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS, initialTmPosData);
    moveData(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3, initialTm3x3Data);
  }
  for (InstanceId &modifiedOptionalId : modifiedOptionalFlags)
  {
    if (modifiedOptionalId == old_instance)
      modifiedOptionalId = new_instance;
  }
  if (auto optionalFlagsItr = riOptionalFlags.find(old_instance); optionalFlagsItr != riOptionalFlags.end())
  {
    auto optionalFlags = optionalFlagsItr->second;
    riOptionalFlags.erase(optionalFlagsItr);
    riOptionalFlags[new_instance] = optionalFlags;
  }
}

void RiExtraAdditionalDataManager::removeDataFor(InstanceId instance_id)
{
  if (instance_id == INVALID_INSTANCE)
    return;

  WinAutoLock lock(mutex);
  if (uint32_t riEntryId = findRIEntry(instance_id); riEntryId < sortedRiInfos.size())
  {
    if (sortedRiInfos[riEntryId].id != INVALID_INSTANCE)
    {
      if (sortedRiInfos[riEntryId].lastDataBits != 0)
        instanceWithDataCounter--; // this encoded data won't be unset on the ri, because the ri no longer exist.
      removeInstanceData(instance_id, sortedRiInfos[riEntryId].usedDataTypes | sortedRiInfos[riEntryId].lastDataBits);
      sortedRiInfos[riEntryId].usedDataTypes = 0;
    }
    sortedRiInfos[riEntryId].id = INVALID_INSTANCE;
    dataToInstance.erase(sortedRiInfos[riEntryId].dataId);
  }
  riOffsets.erase(instance_id);
  riOptionalFlags.erase(instance_id);
  modifiedOptionalFlags.erase_first(instance_id);
  markModified();
}

void RiExtraAdditionalDataManager::setOptionalFlag(rendinst::riex_handle_t ri_handle, rendinst::RiExtraOptionalFlag optional_flag)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return;

  WinAutoLock lock(mutex);
  riOptionalFlags[instanceId] |= static_cast<uint32_t>(optional_flag);
  modifiedOptionalFlags.push_back(instanceId);
  markModified();
}

void RiExtraAdditionalDataManager::unsetOptionalFlag(rendinst::riex_handle_t ri_handle, rendinst::RiExtraOptionalFlag optional_flag)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return;

  WinAutoLock lock(mutex);
  riOptionalFlags[instanceId] &= ~static_cast<uint32_t>(optional_flag);
  modifiedOptionalFlags.push_back(instanceId);
  markModified();
}

uint32_t RiExtraAdditionalDataManager::getOptionalFlags(rendinst::riex_handle_t ri_handle)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return 0;

  WinAutoLock lock(mutex);
  return getOptionalFlagsByInstanceId(instanceId);
}

uint32_t RiExtraAdditionalDataManager::getDataFlags(rendinst::riex_handle_t ri_handle)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return 0;

  WinAutoLock lock(mutex);
  uint32_t entryId = findRIEntry(instanceId);
  if (entryId < sortedRiInfos.size() && sortedRiInfos[entryId].id != INVALID_INSTANCE)
    return sortedRiInfos[entryId].usedDataTypes;
  return 0;
}

uint32_t RiExtraAdditionalDataManager::getEncodedAdditionalData(rendinst::riex_handle_t ri_handle)
{
  verifyNotDirty();
  scene::node_index ni;
  auto *tiledScene = get_tiled_scene(get_instance_id(ri_handle), ni);
  if (tiledScene == nullptr)
    return 0;
  return tiledScene->getPerInstanceRenderAdditionalData(ni);
}

dag::ConstSpan<RiExtraPerInstanceGpuItem> RiExtraAdditionalDataManager::getAdditionalData(rendinst::riex_handle_t ri_handle,
  rendinst::RiExtraPerInstanceDataType type) const
{
  verifyNotDirty();

  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return {};

  switch (type)
  {
    case RiExtraPerInstanceDataType::DESTR_RENDER_DATA:
    {
      if (auto it = destrRenderData.find(instanceId); it != destrRenderData.end())
        return dag::ConstSpan<RiExtraPerInstanceGpuItem>(it->second.data(), it->second.size());
    }
    break;
    case RiExtraPerInstanceDataType::INITIAL_TM_POS:
    {
      if (auto it = initialTmPosData.find(instanceId); it != initialTmPosData.end())
        return dag::ConstSpan<RiExtraPerInstanceGpuItem>(it->second.data(), it->second.size());
    }
    break;
    case RiExtraPerInstanceDataType::INITIAL_TM_3X3:
    {
      if (auto it = initialTm3x3Data.find(instanceId); it != initialTm3x3Data.end())
        return dag::ConstSpan<RiExtraPerInstanceGpuItem>(it->second.data(), it->second.size());
    }
    break;
    case RiExtraPerInstanceDataType::PREV_TM_DATA:
    {
      if (auto it = prevTmData.find(instanceId); it != prevTmData.end())
        return dag::ConstSpan<RiExtraPerInstanceGpuItem>(it->second.data(), it->second.size());
    }
    break;
  }
  return {};
}

uint64_t RiExtraAdditionalDataManager::addAdditionalDataImpl(InstanceId instance_id, rendinst::RiExtraPerInstanceDataType type,
  rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification, const rendinst::RiExtraPerInstanceGpuItem *data,
  uint32_t gpu_data_count)
{
  G_ASSERTF_ONCE(lastFrameIdChecked < ::dagor_get_global_frame_id(),
    "All modifications to RiExtra Additional data must be done BEFORE prepareAndUploadBuffer() is called within a frame.");
  markModified();

  rendinst::RiExtraPerInstanceGpuItem *dst = nullptr;
  int dstDataCount = 0;
  rendinst::RiExtraPerInstanceDataPersistence dstDataPersistence = persistence_mode_verification;
  switch (type)
  {
    case rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA:
      dstDataPersistence = PERSISTENCE_DESTR_RENDER_DATA;
      dstDataCount = RIEXTRA_PER_INSTANCE_RENDER_DATA__DESTR_RENDER_DATA__SIZE;
      dst = destrRenderData[instance_id].data();
      break;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS:
      dstDataPersistence = PERSISTENCE_INITIAL_TM_POS;
      dstDataCount = RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_POS__SIZE;
      dst = initialTmPosData[instance_id].data();
      break;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3:
      dstDataPersistence = PERSISTENCE_INITIAL_TM_3X3;
      dstDataCount = RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_3X3__SIZE;
      dst = initialTm3x3Data[instance_id].data();
      break;
    case rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA:
      dstDataPersistence = PERSISTENCE_PREV_TM_DATA;
      dstDataCount = RIEXTRA_PER_INSTANCE_RENDER_DATA__PREV_TRANSFORM__SIZE;
      dst = prevTmData[instance_id].data();
      break;
  }
  G_ASSERT_RETURN(dst != nullptr, INVALID_DATA_ID);
  G_ASSERTF_RETURN(dstDataCount == gpu_data_count, INVALID_DATA_ID,
    "This data types (%d) is registered with a different data size (%d != %d)", static_cast<uint32_t>(type), dstDataCount,
    gpu_data_count);
  if (dstDataPersistence != persistence_mode_verification)
    LOGERR_ONCE("This data type (%d) is registered with a different persistence mode than requested. The registered persistence "
                "mode will be used",
      static_cast<uint32_t>(type));

  if (dstDataPersistence == rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME)
    hasSingleFrameEntries = true;

  for (uint32_t i = 0; i < dstDataCount; ++i)
    dst[i] = data[i];
  uint32_t riInfoIndex = ensureRIEntryAdded(instance_id);
  if (riInfoIndex >= sortedRiInfos.size() || sortedRiInfos[riInfoIndex].id == INVALID_INSTANCE)
    return INVALID_DATA_ID;
  modifyRIEntry(riInfoIndex, sortedRiInfos[riInfoIndex].usedDataTypes | static_cast<uint32_t>(type));
  handleSizeChange(riInfoIndex);
  return sortedRiInfos[riInfoIndex].dataId;
}

void RiExtraAdditionalDataManager::addAdditionalDataDeferred(rendinst::riex_handle_t ri_handle,
  rendinst::RiExtraPerInstanceDataType type, rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification,
  const rendinst::RiExtraPerInstanceGpuItem *data, uint32_t gpu_data_count)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return;

  WinAutoLock lock(mutex);

  if (lastFrameIdChecked < ::dagor_get_global_frame_id())
  {
    // No need to defer the request

    addAdditionalDataImpl(instanceId, type, persistence_mode_verification, data, gpu_data_count);
    return;
  }

  uint32_t deferredDataItemStart = deferredDataItems.size();
  deferredDataItems.reserve(deferredDataItems.size() + gpu_data_count);
  for (int i = 0; i < gpu_data_count; ++i)
    deferredDataItems.push_back(data[i]);
  deferredDataRequests.push_back(
    DeferredDataRequest{instanceId, type, persistence_mode_verification, gpu_data_count, deferredDataItemStart});
}

uint64_t RiExtraAdditionalDataManager::addAdditionalData(rendinst::riex_handle_t ri_handle, rendinst::RiExtraPerInstanceDataType type,
  rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification, const rendinst::RiExtraPerInstanceGpuItem *data,
  uint32_t gpu_data_count)
{
  InstanceId instanceId = get_instance_id(ri_handle);
  if (instanceId == INVALID_INSTANCE)
    return INVALID_DATA_ID;

  WinAutoLock lock(mutex);
  return addAdditionalDataImpl(instanceId, type, persistence_mode_verification, data, gpu_data_count);
}

void RiExtraAdditionalDataManager::removeAdditionalData(uint64_t data_id, rendinst::RiExtraPerInstanceDataType type)
{
  if (data_id == INVALID_DATA_ID)
    return;
  WinAutoLock lock(mutex);
  auto instanceItr = dataToInstance.find(data_id);
  InstanceId instanceId = instanceItr != dataToInstance.end() ? instanceItr->second : INVALID_INSTANCE;
  if (instanceId == INVALID_INSTANCE)
    return;
  switch (type)
  {
    case rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA:
      G_ASSERTF(PERSISTENCE_DESTR_RENDER_DATA == rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
        "Single frame type data doesn't need to be removed");
      destrRenderData.erase(instanceId);
      break;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS:
      G_ASSERTF(PERSISTENCE_INITIAL_TM_POS == rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
        "Single frame type data doesn't need to be removed");
      initialTmPosData.erase(instanceId);
      break;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3:
      G_ASSERTF(PERSISTENCE_INITIAL_TM_3X3 == rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
        "Single frame type data doesn't need to be removed");
      initialTm3x3Data.erase(instanceId);
      break;
    case rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA:
      G_ASSERTF(PERSISTENCE_PREV_TM_DATA == rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT,
        "Single frame type data doesn't need to be removed");
      prevTmData.erase(instanceId);
      break;
  }
  uint32_t riEntryId = findRIEntry(instanceId);
  if (riEntryId < sortedRiInfos.size() && sortedRiInfos[riEntryId].id != INVALID_INSTANCE)
    modifyRIEntry(riEntryId, sortedRiInfos[riEntryId].usedDataTypes & (~static_cast<uint32_t>(type)));
}

void RiExtraAdditionalDataManager::prepareAndUploadBuffer()
{
  TIME_PROFILE(prepare_riextra_additional_data);
  WinAutoLock lock(mutex);
  processDeferredRequests();
  IPoint2 updateEntryRange = processInstances();
#if DAGOR_DBGLEVEL > 0
  // sortedRiInfos array must be sorted and overlap-free
  // There must not be any empty entries
  for (int i = 0; i < sortedRiInfos.size(); ++i)
  {
    G_ASSERTF_ONCE(sortedRiInfos[i].getRequiredSlotCount() > 0, "RiExtra additional data in inconsistent state");
    G_ASSERTF_ONCE(sortedRiInfos[i].id != INVALID_INSTANCE, "RiExtra additional data in inconsistent state");
    if (i + 1 < sortedRiInfos.size())
    {
      G_ASSERTF_ONCE(sortedRiInfos[i] < sortedRiInfos[i + 1], "RiExtra additional data in inconsistent state");
      G_ASSERTF_ONCE(sortedRiInfos[i].slotOffset + sortedRiInfos[i].getRequiredSlotCount() <= sortedRiInfos[i + 1].slotOffset,
        "RiExtra additional data in inconsistent state");
    }
  }
  // Data to instance references are maintained
  G_ASSERTF_ONCE(dataToInstance.size() == sortedRiInfos.size(), "RiExtra additional data in inconsistent state");
  for (const auto &[dataId, instanceId] : dataToInstance)
  {
    auto entryId = findRIEntry(instanceId);
    G_ASSERTF_ONCE(entryId < sortedRiInfos.size() && sortedRiInfos[entryId].dataId == dataId,
      "RiExtra additional data in inconsistent state");
  }
  // Offsets are maintained
  G_ASSERTF_ONCE(riOffsets.size() == sortedRiInfos.size(), "RiExtra additional data in inconsistent state");
  uint32_t numEntriesWithData = 0;
  for (const auto &[instanceId, offset] : riOffsets)
  {
    uint32_t riEntryId = findRIEntry(instanceId);
    bool found = riEntryId < sortedRiInfos.size() && sortedRiInfos[riEntryId].id == instanceId;
    G_ASSERTF_ONCE(found, "RiExtra additional data in inconsistent state");
    if (found)
    {
      if (sortedRiInfos[riEntryId].usedDataTypes != 0)
        numEntriesWithData++;
      G_ASSERTF_ONCE(static_cast<bool>(sortedRiInfos[riEntryId].usedDataTypes &
                                       static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA)) ==
                       prevTmData.count(instanceId) > 0,
        "RiExtra additional data in inconsistent state");
      G_ASSERTF_ONCE(static_cast<bool>(sortedRiInfos[riEntryId].usedDataTypes &
                                       static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA)) ==
                       destrRenderData.count(instanceId) > 0,
        "RiExtra additional data in inconsistent state");
      G_ASSERTF_ONCE(static_cast<bool>(sortedRiInfos[riEntryId].usedDataTypes &
                                       static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS)) ==
                       initialTmPosData.count(instanceId) > 0,
        "RiExtra additional data in inconsistent state");
      G_ASSERTF_ONCE(static_cast<bool>(sortedRiInfos[riEntryId].usedDataTypes &
                                       static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3)) ==
                       initialTm3x3Data.count(instanceId) > 0,
        "RiExtra additional data in inconsistent state");
    }
  }
  G_ASSERTF_ONCE(instanceWithDataCounter == numEntriesWithData, "RiExtra additional data in inconsistent state");
#endif
  if (updateEntryRange.y > 0)
  {
    uint32_t lastEntry = updateEntryRange.x + updateEntryRange.y - 1;
    IPoint2 updateSlotRange = IPoint2(sortedRiInfos[updateEntryRange.x].slotOffset, sortedRiInfos[lastEntry].slotOffset +
                                                                                      sortedRiInfos[lastEntry].getRequiredSlotCount() -
                                                                                      sortedRiInfos[updateEntryRange.x].slotOffset);
    G_ASSERT(updateSlotRange.x + updateSlotRange.y <= MAX_SLOT_COUNT);
    if (!additionalDataBuffer) // buffer is created
    {
      additionalDataBuffer = dag::buffers::create_persistent_sr_structured(sizeof(rendinst::RiExtraPerInstanceGpuItem), MAX_SLOT_COUNT,
        "per_instance_render_data");
    }

    if (additionalDataBuffer.getBuf()) // Incremental buffer update
    {
      uint32_t offset = updateSlotRange.x * sizeof(rendinst::RiExtraPerInstanceGpuItem);
      if (auto bufferData = lock_sbuffer<rendinst::RiExtraPerInstanceGpuItem>(additionalDataBuffer.getBuf(), offset, updateSlotRange.y,
            VBLOCK_WRITEONLY))
      {
        for (uint32_t i = 0; i < updateEntryRange.y; ++i)
        {
          uint32_t riEntryId = i + updateEntryRange.x;
          const auto copyData = [&](const auto &data_map, rendinst::RiExtraPerInstanceDataType type, uint32_t data_entry_offset) {
            if (sortedRiInfos[riEntryId].usedDataTypes & static_cast<uint32_t>(type))
            {
              auto dataItr = data_map.find(sortedRiInfos[riEntryId].id);
              if (dataItr != data_map.end())
              {
                G_ASSERT(sortedRiInfos[riEntryId].slotOffset + data_entry_offset >= updateSlotRange.x);
                G_ASSERT(sortedRiInfos[riEntryId].slotOffset + data_entry_offset + dataItr->second.size() <=
                         updateSlotRange.x + updateSlotRange.y);
                memcpy(&bufferData[sortedRiInfos[riEntryId].slotOffset + data_entry_offset - updateSlotRange.x],
                  dataItr->second.data(), dataItr->second.size() * sizeof(dataItr->second[0]));
              }
            }
          };
          copyData(initialTmPosData, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS,
            RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_POS__OFFSET(sortedRiInfos[riEntryId].usedDataTypes));
          copyData(initialTm3x3Data, rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3,
            RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_3X3__OFFSET(sortedRiInfos[riEntryId].usedDataTypes));
          copyData(prevTmData, rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA,
            RIEXTRA_PER_INSTANCE_RENDER_DATA__PREV_TRANSFORM__OFFSET(sortedRiInfos[riEntryId].usedDataTypes));
          copyData(destrRenderData, rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA,
            RIEXTRA_PER_INSTANCE_RENDER_DATA__DESTR_RENDER_DATA__OFFSET(sortedRiInfos[riEntryId].usedDataTypes));
        }
      }
    }
  }
  clearPerFrameData();
}

RiExtraPerInstanceDataPersistence RiExtraAdditionalDataManager::getPersistenceOfDataType(
  rendinst::RiExtraPerInstanceDataType type) const
{
  switch (type)
  {
    case rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA: return PERSISTENCE_PREV_TM_DATA;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS: return PERSISTENCE_INITIAL_TM_POS;
    case rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3: return PERSISTENCE_INITIAL_TM_3X3;
    case rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA: return PERSISTENCE_DESTR_RENDER_DATA;
    default: return RiExtraPerInstanceDataPersistence ::PERSISTENT;
  }
}

uint32_t RiExtraAdditionalDataManager::getOptionalFlagsByInstanceId(InstanceId instance_id)
{
  if (auto itr = riOptionalFlags.find(instance_id); itr != riOptionalFlags.end())
    return itr->second;
  return 0;
}

void RiExtraAdditionalDataManager::modifyRIEntry(uint32_t ri_entry_id, uint32_t data_flags)
{
  sortedRiInfos[ri_entry_id].usedDataTypes = data_flags;
  sortedRiInfos[ri_entry_id].lastModifiedFrameId = ::dagor_get_global_frame_id();
  G_ASSERT(sortedRiInfos[ri_entry_id].id != INVALID_INSTANCE);
}

uint32_t RiExtraAdditionalDataManager::ensureRIEntryAdded(InstanceId instance_id)
{
  G_ASSERT(instance_id != INVALID_INSTANCE);
  uint32_t existing = findRIEntry(instance_id);
  if (existing < sortedRiInfos.size())
    return existing;
  uint32_t offset =
    !sortedRiInfos.empty() ? (sortedRiInfos.back().slotOffset + max(1u, sortedRiInfos.back().getRequiredSlotCount())) : 0;
  riOffsets.emplace(eastl::make_pair(instance_id, offset));
  uint32_t riInfoEntryId = sortedRiInfos.size();
  uint64_t dataId = nextDataId++;
  sortedRiInfos.push_back({dataId, instance_id, offset});
  dataToInstance[dataId] = instance_id;
  return riInfoEntryId;
}

uint32_t RiExtraAdditionalDataManager::findRIEntry(InstanceId instance_id)
{
  auto offsetItr = riOffsets.find(instance_id);
  if (offsetItr == riOffsets.end())
    return ~0u;
  uint32_t riEntryId = findRIEntryByOffset(offsetItr->second);
  G_ASSERTF_ONCE(riEntryId < sortedRiInfos.size() && instance_id == sortedRiInfos[riEntryId].id,
    "RiExtra additional data in inconsistent state");
  return riEntryId;
}

uint32_t RiExtraAdditionalDataManager::findRIEntryByOffset(uint32_t offset)
{
  auto riInfoItr = eastl::lower_bound(sortedRiInfos.begin(), sortedRiInfos.end(), offset,
    [](const RIInfo &info, uint32_t offset) { return info.slotOffset < offset; });
  if (riInfoItr != sortedRiInfos.end() && riInfoItr->slotOffset == offset)
    return eastl::distance(sortedRiInfos.begin(), riInfoItr);
  return ~0u;
}

void RiExtraAdditionalDataManager::moveRIEntryToEnd(uint32_t entry_id)
{
  auto info = sortedRiInfos[entry_id];
  sortedRiInfos.erase(sortedRiInfos.begin() + entry_id);
  info.slotOffset =
    !sortedRiInfos.empty() ? (sortedRiInfos.back().slotOffset + max(1u, sortedRiInfos.back().getRequiredSlotCount())) : 0;
  if (info.id != INVALID_INSTANCE)
    riOffsets[info.id] = info.slotOffset;
  sortedRiInfos.push_back(eastl::move(info));
}

void RiExtraAdditionalDataManager::handleSizeChange(uint32_t entry_id)
{
  if (entry_id + 1 >= sortedRiInfos.size())
    return;

  uint32_t requiredSlotCount = sortedRiInfos[entry_id].getRequiredSlotCount();
  uint32_t firstValidSlot = sortedRiInfos[entry_id].slotOffset + max(1u, requiredSlotCount);

  // Move next riInfo entries to the end until they overlap with the current one
  while (sortedRiInfos[entry_id + 1].slotOffset < firstValidSlot)
    moveRIEntryToEnd(entry_id + 1);
}

RiExtraAdditionalDataManager::InstanceId RiExtraAdditionalDataManager::get_instance_id(rendinst::riex_handle_t ri_handle)
{
  uint32_t resIdx = rendinst::handle_to_ri_type(ri_handle);
  rendinst::RiExtraPool &pool = rendinst::riExtra[resIdx];
  uint32_t idx = rendinst::handle_to_ri_inst(ri_handle);

  if (idx >= pool.tsNodeIdx.size() || pool.tsNodeIdx[idx] == scene::INVALID_NODE)
    return INVALID_INSTANCE;

  scene::node_index nodeId = pool.tsNodeIdx[idx];
  return nodeId;
}

void RiExtraAdditionalDataManager::processDeferredRequests()
{
  for (const auto &request : deferredDataRequests)
  {
    addAdditionalData(request.instanceId, request.dataType, request.persistence, &deferredDataItems[request.deferredDataItemStart],
      request.gpuDataCount);
  }
  deferredDataRequests.clear();
  deferredDataItems.clear();
}

IPoint2 RiExtraAdditionalDataManager::processInstances()
{
  markFrameChecked();
  if (!isDirty && !hasEntriesToClear)
    return IPoint2(0, 0);
  // Filter and compact entries
  // + set new data to tiled scene if it changed (such events are minimized since they imply data transfer to gpu)
  uint32_t index = 0;
  uint32_t nextOffset = 0;
  uint32_t firstEntryToUpdate = sortedRiInfos.size();
  uint32_t lastEntryToUpdate = 0;
  for (uint32_t i = 0; i < sortedRiInfos.size(); ++i)
  {
    if (sortedRiInfos[i].usedDataTypes == 0 || sortedRiInfos[i].id == INVALID_INSTANCE)
      continue;
    if (index < i)
      eastl::swap(sortedRiInfos[index], sortedRiInfos[i]);
    if (sortedRiInfos[index].slotOffset != nextOffset)
    {
      sortedRiInfos[index].slotOffset = nextOffset;
      riOffsets[sortedRiInfos[index].id] = sortedRiInfos[index].slotOffset;
    }
    uint32_t requiredSize = sortedRiInfos[index].getRequiredSlotCount();
    G_ASSERT(requiredSize > 0);
    nextOffset = sortedRiInfos[index].slotOffset + requiredSize;
    const bool fitsInBuffer = nextOffset <= MAX_SLOT_COUNT;
    const bool hadDataPreviously = sortedRiInfos[index].lastDataBits != 0;
    bool updateNeeded = false;
    if (sortedRiInfos[index].usedDataTypes != sortedRiInfos[index].lastDataBits)
    {
      updateNeeded = true;
      uint32_t removedDataTypes = sortedRiInfos[index].lastDataBits & ~(sortedRiInfos[index].usedDataTypes);
      sortedRiInfos[index].lastDataBits = sortedRiInfos[index].usedDataTypes;
      removeInstanceData(sortedRiInfos[index].id, removedDataTypes);
    }
    uint32_t encodedInstanceData = rendinst::encodeRiExtraPerInstanceRenderData(sortedRiInfos[index].slotOffset,
      sortedRiInfos[index].usedDataTypes, getOptionalFlagsByInstanceId(sortedRiInfos[index].id));
    if (encodedInstanceData != sortedRiInfos[index].lastEncodedInstanceData)
    {
      updateNeeded = true;
      if (!hadDataPreviously)
        instanceWithDataCounter++;
      scene::node_index ni;
      auto *tiledScene = get_tiled_scene(sortedRiInfos[index].id, ni);
      if (tiledScene && fitsInBuffer)
        tiledScene->setPerInstanceRenderAdditionalDataImm(ni, encodedInstanceData);
      sortedRiInfos[index].lastEncodedInstanceData = encodedInstanceData;
    }
    if (sortedRiInfos[index].lastModifiedFrameId > lastProcessFrameId)
      updateNeeded = true;
    if (updateNeeded && fitsInBuffer)
    {
      firstEntryToUpdate = min(firstEntryToUpdate, index);
      lastEntryToUpdate = max(lastEntryToUpdate, index);
    }
    index++;
  }

  // Handle deleted instances (remove riOffsets and unset them from tiled scene)
  // This doesn't run if data is removed from an instance then it's added back in the same frame
  for (uint32_t i = index; i < sortedRiInfos.size(); ++i)
  {
    if (sortedRiInfos[i].id == INVALID_INSTANCE)
      continue; // data is already cleared
    riOffsets.erase(sortedRiInfos[i].id);
    dataToInstance.erase(sortedRiInfos[i].dataId);
    uint32_t encodedInstanceData =
      rendinst::encodeRiExtraPerInstanceRenderData(0, 0, getOptionalFlagsByInstanceId(sortedRiInfos[i].id));
    if (sortedRiInfos[i].lastDataBits != 0)
      instanceWithDataCounter--;
    scene::node_index ni;
    auto *tiledScene = get_tiled_scene(sortedRiInfos[i].id, ni);
    if (tiledScene)
      tiledScene->setPerInstanceRenderAdditionalDataImm(ni, encodedInstanceData);
  }
  sortedRiInfos.resize(index);

  for (const auto &modifiedHandleItr : modifiedOptionalFlags)
  {
    if (riOffsets.count(modifiedHandleItr) > 0)
      continue; // It's already updated
    uint32_t encodedInstanceData = rendinst::encodeRiExtraPerInstanceRenderData(0, 0, getOptionalFlagsByInstanceId(modifiedHandleItr));
    scene::node_index ni;
    auto *tiledScene = get_tiled_scene(modifiedHandleItr, ni);
    if (tiledScene)
      tiledScene->setPerInstanceRenderAdditionalDataImm(ni, encodedInstanceData);
  }
  modifiedOptionalFlags.clear();

  lastProcessFrameId = ::dagor_get_global_frame_id();
  hasEntriesToClear = false;
  isDirty = false;

  return (lastEntryToUpdate >= firstEntryToUpdate && !sortedRiInfos.empty())
           ? IPoint2(firstEntryToUpdate, lastEntryToUpdate - firstEntryToUpdate + 1)
           : IPoint2(0, 0);
}

void RiExtraAdditionalDataManager::removeInstanceData(InstanceId instance_id, uint32_t types)
{
  if (types & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA))
    destrRenderData.erase(instance_id);
  if (types & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA))
    prevTmData.erase(instance_id);
  if (types & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS))
    initialTmPosData.erase(instance_id);
  if (types & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3))
    initialTm3x3Data.erase(instance_id);
}

void RiExtraAdditionalDataManager::clearPerFrameData()
{
  if (!hasSingleFrameEntries)
    return;
  for (auto &riInfo : sortedRiInfos)
  {
    if constexpr (PERSISTENCE_PREV_TM_DATA == rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME)
      riInfo.usedDataTypes &= ~static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA);
    if constexpr (PERSISTENCE_INITIAL_TM_POS == rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME)
      riInfo.usedDataTypes &= ~static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS);
    if constexpr (PERSISTENCE_INITIAL_TM_3X3 == rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME)
      riInfo.usedDataTypes &= ~static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3);
    if constexpr (PERSISTENCE_DESTR_RENDER_DATA == rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME)
      riInfo.usedDataTypes &= ~static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA);
  }
  hasSingleFrameEntries = false;
  hasEntriesToClear = true;
}

void RiExtraAdditionalDataManager::markModified() { isDirty = true; }

void RiExtraAdditionalDataManager::verifyNotDirty() const { G_ASSERTF(!isDirty, "Ri extra additional data is in dirty state."); }

void RiExtraAdditionalDataManager::markFrameChecked() { lastFrameIdChecked = ::dagor_get_global_frame_id(); }