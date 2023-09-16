#pragma once
#include <util/dag_stdint.h>
#include <math/dag_adjpow2.h>
#include <EASTL/vector_multimap.h>
#include <EASTL/unique_ptr.h>
#include <generic/dag_smallTab.h>
#include "dataComponentManager.h"
#include <daECS/core/dataComponent.h>
#include <daECS/core/entityId.h>
#include <daECS/core/internal/asserts.h>

namespace ecs
{

struct Archetype
{
  uint32_t getEntitySize() const { return entitySize; }
  uint32_t getComponentsCount() const { return componentsCnt; }
  Archetype(uint16_t entity_size, uint16_t cnt, uint8_t initial_entities_bits) :
    componentsCnt(cnt), entitySize(entity_size), manager(entity_size, initial_entities_bits)
  {}

protected:
  friend class EntityManager;
  friend struct Archetypes;

  DataComponentManager manager;
  uint16_t entitySize = 0, componentsCnt = 0;
};

}; // namespace ecs

namespace ecs

{

struct Archetypes
{
public:
  Archetypes();
  uint32_t size() const { return (uint32_t)archetypes.size(); }
  uint32_t capacity() const { return (uint32_t)archetypes.capacity(); }
  uint32_t getArchetypeSize(uint32_t archetype) const;
  const Archetype &getArchetype(uint32_t archetype) const
  {
#ifdef _DEBUG_TAB_
    G_FAST_ASSERT(archetype < archetypes.size());
#endif
    return archetypes.get<ARCHETYPE>()[archetype];
  }
  Archetype &getArchetype(uint32_t archetype)
  {
#ifdef _DEBUG_TAB_
    G_FAST_ASSERT(archetype < archetypes.size());
#endif
    return archetypes.get<ARCHETYPE>()[archetype];
  }
  uint32_t addArchetype(const component_index_t *__restrict components, uint32_t components_cnt, const DataComponents &dataComponents,
    const ComponentTypes &componentTypes);
  void clear();
  uint32_t getComponentSize(uint32_t archetype, uint32_t id) const;
  uint32_t getComponentOfs(uint32_t archetype, uint32_t id) const;
  uint32_t getComponentInitialOfs(uint32_t archetype, uint32_t id) const;
  component_index_t getComponent(uint32_t archetype, uint32_t id) const;
  component_index_t getComponentUnsafe(uint32_t archetype, uint32_t id) const;
  void *__restrict getComponentData(uint32_t archetype, uint32_t componentId, uint32_t sz, chunk_type_t chunkId, uint32_t id) const;
  uint16_t getComponentsCount(uint32_t archetype) const;
  archetype_component_id getArchetypeComponentId(uint32_t archetype, component_index_t id) const;
  bool shouldDefragment(uint32_t arch) const;
  DataComponentManager::DefragResult defragment(uint32_t arch, chunk_type_t &moved_from_chunk, chunk_type_t &moved_to_chunk,
    uint32_t &moved_at_id, uint32_t &moved_cnt);
  ComponentTypeFlags getArchetypeCombinedTypeFlags(uint32_t archetype) const
  {
    DAECS_EXT_ASSERT(archetype < size());
    return ComponentTypeFlags(archetypes.get<COMPONENT_FLAGS>()[archetype]);
  }

protected:
  friend class EntityManager;
  friend struct InstantiatedTemplate;
  void remap(const archetype_t *map, uint32_t size);
  const component_index_t *__restrict getTrackedPodCidxUnsafe(uint32_t archetype) const;
  void *__restrict getComponentDataUnsafeOfs(uint32_t archetype, uint32_t ofs, uint32_t sz, chunk_type_t chunkId, uint32_t id) const;
  void *__restrict getComponentDataUnsafeOfsNoCheck(uint32_t archetype, uint32_t ofs, uint32_t sz, chunk_type_t chunkId,
    uint32_t id) const;
  void *__restrict getComponentDataUnsafe(uint32_t archetype, uint32_t componentId, uint32_t sz, chunk_type_t chunkId,
    uint32_t id) const;
  void *__restrict getComponentDataUnsafeNoCheck(uint32_t archetype, uint32_t componentId, uint32_t sz, chunk_type_t chunkId,
    uint32_t id) const;
  uint32_t getComponentSizeUnsafe(uint32_t archetype, uint32_t id) const;
  uint32_t getComponentOfsUnsafe(uint32_t archetype, uint32_t id) const;
  uint32_t generation() const { return size(); } // as we are currently not erasing archetypes
  archetype_component_id getArchetypeComponentIdUnsafe(uint32_t archetype, component_index_t id) const;

  __forceinline const uint32_t getArchetypeComponentOfsUnsafe(uint32_t archetype) const
  {
    return archetypes.get<COMPONENT_OFS>()[archetype];
  }
  __forceinline const uint32_t getArchetypeComponentOfs(uint32_t archetype) const
  {
    return archetype < size() ? archetypes.get<COMPONENT_OFS>()[archetype] : 0;
  }
  const component_index_t *getArchetypeComponents(uint32_t archetype) const
  {
    return &archetypeComponents.get<INDEX>()[getArchetypeComponentOfsUnsafe(archetype)];
  }

  __forceinline uint16_t getComponentSizeFromOfs(archetype_component_id component_id, uint32_t ofs) const;
  __forceinline uint16_t getComponentOfsFromOfs(archetype_component_id component_id, uint32_t ofs) const;
  __forceinline uint16_t getComponentInitialOfsUnsafe(uint32_t archetype, archetype_component_id component_id) const;
  __forceinline const uint16_t *initialComponentDataOffset(uint32_t archetype) const;
  __forceinline const uint16_t *componentDataOffsets(uint32_t archetype) const;
  __forceinline const uint16_t *componentDataSizes(uint32_t archetype) const;
  __forceinline const component_index_t *componentIndices(uint32_t archetype) const;

  uint8_t *__restrict getChunkData(uint32_t archetype, chunk_type_t chunkId, uint32_t &stride) const; // in entities
  void allocateEntity(uint32_t archetype, chunk_type_t &chunkId, uint32_t &id);
  void entityAllocated(uint32_t archetype, chunk_type_t chunkId);
  void addEntity(uint32_t to_archetype, chunk_type_t &chunkId, uint32_t &id, const uint8_t *__restrict initial_data,
    const uint16_t *__restrict offsets);
  bool delEntity(uint32_t to_archetype, chunk_type_t chunkId, uint32_t id, uint32_t &moved_id);
  struct ArchetypeInfo
  {
    component_index_t firstNonEidIndex, count;
    eastl::unique_ptr<archetype_component_id[]> componentIndexToArchetypeOffset; // todo: make it soa as well
    inline archetype_component_id getComponentId(component_index_t cidx) const;
  };
#pragma pack(push, 1)
  struct TrackedPod
  {
    archetype_component_id shadowArchComponentId;
    uint16_t dataOffset, trackedFromOfs, size;
  };
  struct CreatableComponent
  {
    archetype_component_id archComponentId;
    uint16_t dataOffset, trackedFromOfs, size; // trackedFromOfs means INITIAL offset for getCreatables
    component_index_t originalCidx;            // trackedFromOfs is needed only for trackable created, but we keep it for padding
    type_index_t typeIndex;
    // ComponentTypeManager *typeManager;//we use typeIndex, instead of pointer to typeManager. It adds one indirection, and can be
    // easily replaced with typeManager
  };
  struct ResourceComponent
  {
    archetype_component_id archComponentId;
    component_index_t cIndex;
    type_index_t typeIndex;
  };
#pragma pack(pop)

  const SmallTab<TrackedPod> &getTrackedPodPairs(uint32_t arch) const { return archetypes.get<TRACKED_PODS>()[arch]; }

  // not including creatable tracked pairs
  const SmallTab<CreatableComponent> &getCreatables(uint32_t arch) const { return archetypes.get<CREATABLES>()[arch]; }
  const SmallTab<CreatableComponent> &getCreatableTrackeds(uint32_t arch) const { return archetypes.get<TRACKED_CREATABLES>()[arch]; }
  const SmallTab<ResourceComponent> &getComponentsWithResources(uint32_t arch) const { return archetypes.get<WITH_RESOURCES>()[arch]; }
  static constexpr int HAS_TRACKED_COMPONENT = (COMPONENT_TYPE_LAST_PLUS_1 - 1) * 2;
  // SoA
  uint32_t findArchetype(const component_index_t *__restrict components, uint32_t components_cnt, uint32_t &hash) const;
  const ArchetypeInfo &getArchetypeInfoUnsafe(uint32_t archetype) const { return eastl::get<INFO>(archetypes[archetype]); }

  enum
  {
    ARCHETYPE,
    COMPONENT_OFS,
    INFO,
    COMPONENT_FLAGS,
    TRACKED_PODS,
    TRACKED_PODS_CIDX,
    CREATABLES,
    TRACKED_CREATABLES,
    WITH_RESOURCES
  };
  eastl::tuple_vector<Archetype, uint32_t, ArchetypeInfo, eastl::underlying_type_t<ComponentTypeFlags>, SmallTab<TrackedPod>, uint32_t,
    SmallTab<CreatableComponent>, // not including creatable tracked pairs
    SmallTab<CreatableComponent>,
    SmallTab<ResourceComponent>>
    archetypes; // all archetypes all components data
  dag::Vector<component_index_t> allTrackedPodsCidx;
  dag::Vector<eastl::unique_ptr<uint16_t[]>> initialOffsets; // can't be made part of tuple_vector :(

  enum
  {
    INDEX = 0,       // all archetypes all components index data
    DATA_OFFSET = 1, // all archetypes component offset
    // all archetypes component size. We keep it separately from archetypeComponentDataOfs,
    // because we only need them together on add/del Entity and getComponent.
    // For making Queries we need only offsets. Obviously, invalidating/faster performing queries
    //  is more important even if we add/del entity (since it causes invalidation anyway).
    DATA_SIZE = 2,
  };
  eastl::tuple_vector<component_index_t, uint16_t, uint16_t> archetypeComponents; // INDEX, DATA_OFFSET, DATA_SIZE. indexed by
                                                                                  // COMPONENT_OFS+component
  //-SoA

  // const ArchetypeInfo &getArchetypeInfo(uint32_t archetype) const {return archetypes.get<INFO>()[archetype];}
  eastl::vector_multimap<uint32_t, archetype_t> archetypesByComponents; // hash(of component_indices) -> list of archetypes
#if DAECS_EXTENSIVE_CHECKS
  dag::Vector<int> queryingArchetypeCount;
#endif
};


inline void *__restrict Archetypes::getComponentDataUnsafeOfsNoCheck(uint32_t archetype, uint32_t ofs, uint32_t sz,
  chunk_type_t chunkId, uint32_t id) const
{
  return archetypes.get<ARCHETYPE>()[archetype].manager.getDataUnsafeNoCheck(ofs, sz, chunkId, id);
}

inline void *__restrict Archetypes::getComponentDataUnsafeOfs(uint32_t archetype, uint32_t ofs, uint32_t sz, chunk_type_t chunkId,
  uint32_t id) const
{
  return archetypes.get<ARCHETYPE>()[archetype].manager.getDataUnsafe(ofs, sz, chunkId, id);
}

inline void *__restrict Archetypes::getComponentDataUnsafeNoCheck(uint32_t archetype, uint32_t componentId, uint32_t sz,
  chunk_type_t chunkId, uint32_t id) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size() &&
                   sz == archetypeComponents.get<DATA_SIZE>()[componentId + getArchetypeComponentOfsUnsafe(archetype)]);
  return getComponentDataUnsafeOfsNoCheck(archetype, getComponentOfsUnsafe(archetype, componentId), sz, chunkId, id);
}

inline void *__restrict Archetypes::getComponentDataUnsafe(uint32_t archetype, uint32_t componentId, uint32_t sz, chunk_type_t chunkId,
  uint32_t id) const
{
  DAECS_EXT_ASSERTF(archetype < archetypes.size() && componentId < getComponentsCount(archetype) &&
                      sz == archetypeComponents.get<DATA_SIZE>()[componentId + getArchetypeComponentOfs(archetype)],
    "%d arch (%d); compId %d (%d); requested %d, componentSz %d", archetype, archetypes.size(), componentId, componentId,
    getComponentsCount(archetype), sz,
    componentId < getComponentsCount(archetype)
      ? archetypeComponents.get<DATA_SIZE>()[componentId + getArchetypeComponentOfs(archetype)]
      : ~0);
  return getComponentDataUnsafeOfs(archetype, getComponentOfsUnsafe(archetype, componentId), sz, chunkId, id);
}

inline void *__restrict Archetypes::getComponentData(uint32_t archetype, uint32_t componentId, uint32_t sz, chunk_type_t chunkId,
  uint32_t id) const
{
  if (archetype >= archetypes.size())
    return nullptr;
  DAECS_EXT_ASSERTF(componentId < getComponentsCount(archetype) &&
                      sz == archetypeComponents.get<DATA_SIZE>()[componentId + getArchetypeComponentOfs(archetype)],
    "%d arch (%d); compId %d (%d); requested %d, componentSz %d", archetype, archetypes.size(), componentId, componentId,
    getComponentsCount(archetype), sz,
    componentId < getComponentsCount(archetype)
      ? archetypeComponents.get<DATA_SIZE>()[componentId + getArchetypeComponentOfs(archetype)]
      : ~0);
  return getComponentDataUnsafe(archetype, componentId, sz, chunkId, id);
}

__forceinline const uint16_t *Archetypes::componentDataOffsets(uint32_t archetype) const
{
  return archetypeComponents.get<DATA_OFFSET>() + getArchetypeComponentOfsUnsafe(archetype);
}

__forceinline const component_index_t *Archetypes::componentIndices(uint32_t archetype) const
{
  return archetypeComponents.get<INDEX>() + getArchetypeComponentOfsUnsafe(archetype);
}
__forceinline const uint16_t *Archetypes::componentDataSizes(uint32_t archetype) const
{
  return archetypeComponents.get<DATA_SIZE>() + getArchetypeComponentOfsUnsafe(archetype);
}

__forceinline uint16_t Archetypes::getComponentSizeFromOfs(archetype_component_id component_id, uint32_t ofs) const
{
  return archetypeComponents.get<DATA_SIZE>()[component_id + ofs];
}
__forceinline uint16_t Archetypes::getComponentOfsFromOfs(archetype_component_id component_id, uint32_t ofs) const
{
  return archetypeComponents.get<DATA_OFFSET>()[component_id + ofs];
}
__forceinline uint16_t Archetypes::getComponentInitialOfsUnsafe(uint32_t archetype, archetype_component_id component_id) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  return initialOffsets[archetype][component_id];
}

__forceinline uint32_t Archetypes::getComponentSizeUnsafe(uint32_t archetype, uint32_t id) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  return getComponentSizeFromOfs(id, getArchetypeComponentOfsUnsafe(archetype));
}
__forceinline uint32_t Archetypes::getComponentOfsUnsafe(uint32_t archetype, uint32_t id) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  return getComponentOfsFromOfs(id, getArchetypeComponentOfsUnsafe(archetype));
}

__forceinline uint32_t Archetypes::getComponentSize(uint32_t archetype, uint32_t id) const
{
  if (archetype >= archetypes.size())
    return 0;
  return getComponentSizeUnsafe(archetype, id);
}

inline uint32_t Archetypes::getComponentOfs(uint32_t archetype, uint32_t id) const
{
  if (archetype >= archetypes.size())
    return uint32_t(-1);
  return getComponentOfsUnsafe(archetype, id);
}

__forceinline const uint16_t *Archetypes::initialComponentDataOffset(uint32_t archetype) const
{
  return initialOffsets[archetype].get();
}

inline uint32_t Archetypes::getComponentInitialOfs(uint32_t archetype, uint32_t id) const
{
  if (archetype >= archetypes.size())
    return uint32_t(-1);
  return getComponentInitialOfsUnsafe(archetype, id);
}

inline component_index_t Archetypes::getComponent(uint32_t archetype, uint32_t id) const
{
  if (archetype >= archetypes.size())
    return INVALID_COMPONENT_INDEX;
  return archetypeComponents.get<INDEX>()[id + getArchetypeComponentOfsUnsafe(archetype)];
}

inline component_index_t Archetypes::getComponentUnsafe(uint32_t archetype, uint32_t id) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  uint32_t at = id + getArchetypeComponentOfsUnsafe(archetype);
  DAECS_EXT_ASSERT(at < archetypeComponents.size());
  return archetypeComponents.get<INDEX>()[at];
}

inline archetype_component_id Archetypes::ArchetypeInfo::getComponentId(component_index_t cidx) const
{
  if (cidx == 0) // eid
    return 0;
  uint32_t at = (uint32_t)((int)cidx - (int)firstNonEidIndex);
  if (at >= count)
    return INVALID_ARCHETYPE_COMPONENT_ID;
  return componentIndexToArchetypeOffset[at];
}

inline archetype_component_id Archetypes::getArchetypeComponentIdUnsafe(uint32_t archetype, component_index_t cidx) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  return getArchetypeInfoUnsafe(archetype).getComponentId(cidx);
}
inline const component_index_t *__restrict Archetypes::getTrackedPodCidxUnsafe(uint32_t archetype) const
{
  DAECS_EXT_ASSERT(archetype < archetypes.size());
  return allTrackedPodsCidx.data() + archetypes.get<TRACKED_PODS_CIDX>()[archetype];
}

inline archetype_component_id Archetypes::getArchetypeComponentId(uint32_t archetype, component_index_t cidx) const
{
  if (archetypes.size() <= archetype)
    return INVALID_ARCHETYPE_COMPONENT_ID;
  return getArchetypeComponentIdUnsafe(archetype, cidx);
}

inline uint8_t *__restrict Archetypes::getChunkData(uint32_t archetype, chunk_type_t chunkId, uint32_t &stride) const
{
  return eastl::get<ARCHETYPE>(archetypes[archetype]).manager.getChunkData(chunkId, stride);
}
inline uint32_t Archetypes::getArchetypeSize(uint32_t archetypeId) const
{
  DAECS_EXT_ASSERT(archetypeId < archetypes.size());
  return eastl::get<ARCHETYPE>(archetypes[archetypeId]).entitySize;
}
inline uint16_t Archetypes::getComponentsCount(uint32_t archetypeId) const
{
  DAECS_EXT_ASSERT(archetypeId < archetypes.size());
  return eastl::get<ARCHETYPE>(archetypes[archetypeId]).componentsCnt;
}

inline bool Archetypes::shouldDefragment(uint32_t) const
{
  // todo: if gc ever slow, we can switch off defragmentation (or optimize it :) )
  return true;
}

}; // namespace ecs
