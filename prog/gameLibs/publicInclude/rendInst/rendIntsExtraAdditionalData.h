//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/riextra_per_instance.hlsli>
#include <util/dag_stdint.h>
#include <rendInst/riexHandle.h>
#include <drv/3d/dag_buffers.h>
#include <EASTL/vector.h>
#include <EASTL/array.h>
#include <EASTL/vector_map.h>
#include <scene/dag_scene.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_resPtr.h>
#include <osApiWrappers/dag_critSec.h>

namespace rendinst
{

enum class RiExtraOptionalFlag : uint32_t
{
  HIDE_MARKED_MATERIAL = 1 << 0
};

enum class RiExtraPerInstanceDataType
{
  PREV_TM_DATA = RIEXTRA_PER_INSTANCE_RENDER_DATA__PREV_TRANSFORM__FLAG,
  INITIAL_TM_POS = RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_POS__FLAG,
  INITIAL_TM_3X3 = RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_3X3__FLAG,
  DESTR_RENDER_DATA = RIEXTRA_PER_INSTANCE_RENDER_DATA__DESTR_RENDER_DATA__FLAG,
};

enum class RiExtraPerInstanceDataPersistence
{
  SINGLE_FRAME,
  PERSISTENT
};

struct RiExtraPerInstanceGpuItem
{
  uint32_t data[4];
};

class RiExtraAdditionalDataManager
{
public:
  /**
   * Logic loop (runs per frame):
   *  1. Add/remove data per instances: this is called from external modules
   *    - Data is stored into associative containers, key=ri_handle
   *    - Non-overlapping offsets are allocated and maintained for the instances with data, with least number of changed offsets,
   * allowing gaps
   *  2. Data is processed
   *    - Gaps are removed, data is compacted
   *    - Buffer range to be updated is computed
   *    - Instance offsets (and data flags) are sent to the instance riExtra data
   *  3. Data is uploaded
   *    - Only the modified parts. No change = no upload
   *  4. Removing single-frame data entries:
   *    - all single-frame data types are removed after uploading the new data to the buffer
   *    - gaps are left open, because the same instance might get data again in the next frame
   */

  static constexpr uint32_t MAX_SLOT_COUNT = 1024;

  using InstanceId = uint32_t; // scene id + node id
  static constexpr InstanceId INVALID_INSTANCE = 0xFFFFFFFF;
  static constexpr uint64_t INVALID_DATA_ID = ~0ull;

  using PrevTMData = eastl::array<rendinst::RiExtraPerInstanceGpuItem, RIEXTRA_PER_INSTANCE_RENDER_DATA__PREV_TRANSFORM__SIZE>;
  using InitialTmPosData = eastl::array<rendinst::RiExtraPerInstanceGpuItem, RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_POS__SIZE>;
  using InitialTm3x3Data = eastl::array<rendinst::RiExtraPerInstanceGpuItem, RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_3X3__SIZE>;
  using DestrRenderData = eastl::array<rendinst::RiExtraPerInstanceGpuItem, RIEXTRA_PER_INSTANCE_RENDER_DATA__DESTR_RENDER_DATA__SIZE>;

  static constexpr rendinst::RiExtraPerInstanceDataPersistence PERSISTENCE_PREV_TM_DATA =
    rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME;
  static constexpr rendinst::RiExtraPerInstanceDataPersistence PERSISTENCE_INITIAL_TM_POS =
    rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT;
  static constexpr rendinst::RiExtraPerInstanceDataPersistence PERSISTENCE_INITIAL_TM_3X3 =
    rendinst::RiExtraPerInstanceDataPersistence::PERSISTENT;
  static constexpr rendinst::RiExtraPerInstanceDataPersistence PERSISTENCE_DESTR_RENDER_DATA =
    rendinst::RiExtraPerInstanceDataPersistence::SINGLE_FRAME;

  void clearAll();
  static InstanceId get_instance_id(uint32_t scene_id, uint32_t node_id);
  void transfer(InstanceId old_instance, InstanceId new_instance);
  void removeDataFor(InstanceId instance_id);
  void setOptionalFlag(rendinst::riex_handle_t ri_handle, rendinst::RiExtraOptionalFlag optional_flag);
  void unsetOptionalFlag(rendinst::riex_handle_t ri_handle, rendinst::RiExtraOptionalFlag optional_flag);
  uint32_t getOptionalFlags(rendinst::riex_handle_t ri_handle);
  uint32_t getDataFlags(rendinst::riex_handle_t ri_handle);
  uint32_t getEncodedAdditionalData(rendinst::riex_handle_t ri_handle);
  dag::ConstSpan<RiExtraPerInstanceGpuItem> getAdditionalData(rendinst::riex_handle_t ri_handle,
    rendinst::RiExtraPerInstanceDataType type) const;
  uint64_t addAdditionalData(rendinst::riex_handle_t ri_handle, rendinst::RiExtraPerInstanceDataType type,
    rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification, const rendinst::RiExtraPerInstanceGpuItem *data,
    uint32_t gpu_data_count);
  // Adds data when the next prepareAndUploadBuffer() is called. It can be used outside the normal scheduling limitations
  void addAdditionalDataDeferred(rendinst::riex_handle_t ri_handle, rendinst::RiExtraPerInstanceDataType type,
    rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification, const rendinst::RiExtraPerInstanceGpuItem *data,
    uint32_t gpu_data_count);
  void removeAdditionalData(uint64_t data_id, rendinst::RiExtraPerInstanceDataType type);

  void prepareAndUploadBuffer();

private:
  struct RIInfo
  {
    uint64_t dataId = 0;
    InstanceId id = 0;
    uint32_t slotOffset = 0;
    uint32_t usedDataTypes = 0;

    uint32_t lastEncodedInstanceData = 0; // Last value written to the riExtra buffer
    uint32_t lastDataBits = 0;            // Used for clearing up data maps
    uint32_t lastModifiedFrameId = 0;

    uint32_t getRequiredSlotCount() const
    {
      uint32_t requiredSlots = 0;
      if (usedDataTypes & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::DESTR_RENDER_DATA))
        requiredSlots += RIEXTRA_PER_INSTANCE_RENDER_DATA__DESTR_RENDER_DATA__SIZE;
      if (usedDataTypes & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_POS))
        requiredSlots += RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_POS__SIZE;
      if (usedDataTypes & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::INITIAL_TM_3X3))
        requiredSlots += RIEXTRA_PER_INSTANCE_RENDER_DATA__INITIAL_TM_3X3__SIZE;
      if (usedDataTypes & static_cast<uint32_t>(rendinst::RiExtraPerInstanceDataType::PREV_TM_DATA))
        requiredSlots += RIEXTRA_PER_INSTANCE_RENDER_DATA__PREV_TRANSFORM__SIZE;
      return requiredSlots;
    }

    bool operator<(const RIInfo &rhs) const { return slotOffset < rhs.slotOffset; }
  };

  struct DeferredDataRequest
  {
    InstanceId instanceId;
    rendinst::RiExtraPerInstanceDataType dataType;
    rendinst::RiExtraPerInstanceDataPersistence persistence;
    uint32_t gpuDataCount;
    uint32_t deferredDataItemStart;
  };

  eastl::vector_map<InstanceId, uint32_t> riOffsets;       // instance id -> slot offset
  eastl::vector_map<uint64_t, InstanceId> dataToInstance;  // This is needed, because instance id can change, data id is persistent
  eastl::vector_map<InstanceId, uint32_t> riOptionalFlags; // ri index -> optional flags
  eastl::vector<InstanceId> modifiedOptionalFlags;         // List of ri that need update due to changed optional flags
  eastl::vector<RIInfo> sortedRiInfos;                     // sorted by offset
  eastl::vector_map<InstanceId, PrevTMData> prevTmData;
  eastl::vector_map<InstanceId, InitialTmPosData> initialTmPosData;
  eastl::vector_map<InstanceId, InitialTm3x3Data> initialTm3x3Data;
  eastl::vector_map<InstanceId, DestrRenderData> destrRenderData;
  eastl::vector<DeferredDataRequest> deferredDataRequests;
  eastl::vector<rendinst::RiExtraPerInstanceGpuItem> deferredDataItems;

  uint64_t nextDataId = 0;
  uint32_t lastProcessFrameId = 0; // This is only set if actually processing takes place
  uint32_t lastFrameIdChecked = 0; // This is set every frame
  bool isDirty = false;
  bool hasEntriesToClear = false;
  bool hasSingleFrameEntries = false;

  // used for debug checks
  // increments when the encoded data is set on an ri
  // decrements when the encoded data is unset from an ri
  uint32_t instanceWithDataCounter = 0;

  UniqueBufHolder additionalDataBuffer;

  mutable WinCritSec mutex;

  RiExtraPerInstanceDataPersistence getPersistenceOfDataType(rendinst::RiExtraPerInstanceDataType type) const;
  uint32_t getOptionalFlagsByInstanceId(InstanceId instance_id);
  void modifyRIEntry(uint32_t ri_entry_id, uint32_t data_flags);
  uint32_t ensureRIEntryAdded(InstanceId instance_id); // returns entry_id
  uint32_t findRIEntry(InstanceId instance_id);
  uint32_t findRIEntryByOffset(uint32_t offset);
  void moveRIEntryToEnd(uint32_t entry_id);
  void handleSizeChange(uint32_t entry_id);
  static InstanceId get_instance_id(rendinst::riex_handle_t ri_handle);
  void removeInstanceData(InstanceId instance_id, uint32_t types);
  uint64_t addAdditionalDataImpl(InstanceId instance_id, rendinst::RiExtraPerInstanceDataType type,
    rendinst::RiExtraPerInstanceDataPersistence persistence_mode_verification, const rendinst::RiExtraPerInstanceGpuItem *data,
    uint32_t gpu_data_count);

  // Returns entry range to be updated (start entry id, entry count)
  IPoint2 processInstances();

  void processDeferredRequests();

  // This doesn't remove values from data maps, because new values might arrive during the next frame.
  // Removal from data maps is deferred to the processing of instances in the next frame
  void clearPerFrameData();

  void markModified();
  void verifyNotDirty() const;
  void markFrameChecked();
};

} // namespace rendinst