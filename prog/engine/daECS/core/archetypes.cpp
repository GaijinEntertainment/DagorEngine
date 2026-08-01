// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <hash/wyhash.h>
#include <debug/dag_debug.h>
#include <daECS/core/internal/archetypes.h>
#include "archetypeBloomMask.h"
namespace ecs
{

static constexpr uint32_t INITIAL_CONT_SIZE = 256;
template <typename T>
static inline void clear_cont(T &cont)
{
  T().swap(cont);
  cont.reserve(INITIAL_CONT_SIZE);
}

void Archetypes::clear()
{
  clear_cont(archetypes); // all archetypes all components data;
#if DAECS_EXTENSIVE_CHECKS
  clear_cont(queryingArchetypeCount);
#endif
  clear_cont(initialOffsets);
  clear_cont(archetypesByComponents);
  clear_cont(archetypeComponents);
  clear_cont(allTrackedPodsCidx);
}

void Archetypes::ArchetypeInfo::init(const component_index_t *__restrict components, uint32_t components_cnt)
{
  if (components_cnt <= 1) // components[0] is eid, resolved without the blocks
  {
    firstCidx = 1;
    lastCidx = 0;
    blocks.reset();
    return;
  }
  firstCidx = components[1]; // components are sorted ascending
  lastCidx = components[components_cnt - 1];
  const uint32_t nWords = (uint32_t(lastCidx) - firstCidx + 32) >> 5;
  blocks.reset(new uint64_t[nWords]);
  memset(blocks.get(), 0, nWords * sizeof(uint64_t));
  for (uint32_t i = 1; i < components_cnt; ++i)
  {
    const uint32_t at = uint32_t(components[i]) - firstCidx;
    blocks[at >> 5] |= 1ull << (at & 31);
  }
  uint32_t run = 0; // stamp the running rank into the high halves
  for (uint32_t w = 0; w < nWords; ++w)
  {
    blocks[w] |= uint64_t(run) << 32;
    run += dag::popcount(uint32_t(blocks[w]));
  }
}

// unlike getComponentId, the eid check rides the range-gate miss path (cidx 0
// always fails the gate as firstCidx >= 1), so hits pay nothing for it; fully
// select-based and sorted-walk bodies measured slower on real data (benchmarkCidx)
void Archetypes::ArchetypeInfo::getComponentIds(const component_index_t *__restrict cidx, uint32_t n,
  archetype_component_id *__restrict out) const
{
  if (DAGOR_UNLIKELY(firstCidx > lastCidx)) // eid-only archetype: no blocks
  {
    for (uint32_t i = 0; i < n; ++i)
      out[i] = cidx[i] == 0 ? 0 : INVALID_ARCHETYPE_COMPONENT_ID;
    return;
  }
  const uint64_t *__restrict bl = blocks.get();
  const uint32_t first = firstCidx, span = uint32_t(lastCidx) - first;
  for (uint32_t i = 0; i < n; ++i)
  {
    const uint32_t at = uint32_t(cidx[i]) - first; // wraps below first, rejected by the span gate
    if (at > span)
      out[i] = cidx[i] == 0 ? 0 : INVALID_ARCHETYPE_COMPONENT_ID; // eid is always archetype component 0
    else
    {
      const uint64_t br = bl[at >> 5]; // low 32: presence bits, high 32: rank
      const uint32_t bit = 1u << (at & 31);
      out[i] = (br & bit) ? archetype_component_id(uint32_t(br >> 32) + dag::popcount(uint32_t(br) & (bit - 1)) + 1)
                          : INVALID_ARCHETYPE_COMPONENT_ID;
    }
  }
}

Archetypes::Archetypes()
{
  archetypes.reserve(INITIAL_CONT_SIZE);
#if DAECS_EXTENSIVE_CHECKS
  queryingArchetypeCount.reserve(INITIAL_CONT_SIZE);
#endif
  initialOffsets.reserve(INITIAL_CONT_SIZE);
  archetypesByComponents.reserve(INITIAL_CONT_SIZE);
  archetypeComponents.reserve(INITIAL_CONT_SIZE);
  allTrackedPodsCidx.reserve(INITIAL_CONT_SIZE);
}

inline uint32_t Archetypes::findArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
  uint32_t &hash) const
{
  hash = wyhash(components, sizeof(component_index_t) * components_cnt, 0, _wyp);
  auto currentArchetypesIt = archetypesByComponents.equal_range(hash);
  for (auto archIt = currentArchetypesIt.first; archIt != currentArchetypesIt.second; archIt++)
  {
    const Archetype &archetype = getArchetype(archIt->second);
    if (archetype.componentsCnt == components_cnt &&
        memcmp(getArchetypeComponents(archIt->second), components, components_cnt * sizeof(component_index_t)) == 0)
      return archIt->second;
  }
  return eastl::numeric_limits<uint32_t>::max();
}

uint32_t Archetypes::addArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
  const DataComponents &dataComponents, const ComponentTypes &componentTypes)
{
  // G_ASSERT_RETURN(components_cnt > 1, -1);//1 is just entityId
  // we check if it is already existing archetype
  uint32_t componentsHash;
  uint32_t archetypeId = findArchetype(components, components_cnt, componentsHash);
  if (archetypeId != eastl::numeric_limits<uint32_t>::max())
    return archetypeId;
  // ids are archetype_t/archetype_component_id (uint16); a silent wrap would corrupt
  // every entity of the wrapped archetype, so refuse loudly even in release
  if (DAGOR_UNLIKELY(archetypes.size() >= INVALID_ARCHETYPE || components_cnt >= INVALID_ARCHETYPE_COMPONENT_ID))
  {
    logerr("can't add archetype: %d archetypes, %d components (limits %d, %d)", archetypes.size(), components_cnt, INVALID_ARCHETYPE,
      INVALID_ARCHETYPE_COMPONENT_ID);
    return INVALID_ARCHETYPE;
  }
  // bound the ALIGNED size: initialComponentDataOffset below is uint16_t, and padding
  // makes it exceed the raw sum; the alignment rule must match the layout loop
  uint32_t entitySizeCheck = 0;
  for (uint32_t i = 0; i < components_cnt; ++i)
  {
    const uint32_t size = componentTypes.getTypeInfo(dataComponents.getComponentById(components[i]).componentType).size;
    if (size)
    {
      const uint32_t alignTo = eastl::min(size, (uint32_t)max_alignment);
      if (entitySizeCheck % alignTo != 0)
        entitySizeCheck += alignTo - entitySizeCheck % alignTo;
    }
    entitySizeCheck += size;
  }
  if (DAGOR_UNLIKELY(entitySizeCheck > eastl::numeric_limits<uint16_t>::max()))
  {
    logerr("can't add archetype: aligned entity size %d exceeds %d", entitySizeCheck, eastl::numeric_limits<uint16_t>::max());
    return INVALID_ARCHETYPE;
  }
  // it is new one. Let's add it
  archetypesByComponents.insert(
    decltype(archetypesByComponents)::value_type(componentsHash, (uint32_t)archetypes.size())); // add new archetype

  const uint32_t componentsAt = (uint32_t)archetypeComponents.size();
  archetypeComponents.resize(componentsAt + components_cnt);
  memcpy(archetypeComponents.get<INDEX>() + componentsAt, components, components_cnt * sizeof(component_index_t));

  uint32_t entitySize = 0, minComponentSizeBits = 32, maxComponentSize = 0, alignedEntitySize = 0;
  eastl::underlying_type_t<ComponentTypeFlags> typeFlags = 0;
  G_ASSERT(components_cnt <= eastl::numeric_limits<archetype_component_id>::max());
  eastl::unique_ptr<uint16_t[]> initialComponentDataOffset(new uint16_t[components_cnt]);

  for (archetype_component_id i = 0; i < components_cnt; ++i)
  {
    const auto typeIndex = dataComponents.getComponentById(components[i]).componentType;
    const ComponentType type = componentTypes.getTypeInfo(typeIndex);
    typeFlags |= eastl::underlying_type_t<ComponentTypeFlags>(type.flags);
    archetypeComponents.get<DATA_OFFSET>()[componentsAt + i] = entitySize;
    archetypeComponents.get<DATA_SIZE>()[componentsAt + i] = type.size;
    if (type.size)
    {
      // alignment
      uint32_t alignTo = eastl::min((uint32_t)type.size, (uint32_t)max_alignment); // not aligning to more than vector
      if (alignedEntitySize % alignTo != 0)
        alignedEntitySize += alignTo - alignedEntitySize % alignTo;
    }
    initialComponentDataOffset[i] = alignedEntitySize;

    alignedEntitySize += type.size;
    entitySize += type.size;
    if (type.size > 0)
      minComponentSizeBits = eastl::min(minComponentSizeBits, __bsf((uint32_t)type.size));
    maxComponentSize = eastl::max(maxComponentSize, (uint32_t)type.size);
  }
  G_ASSERTF(entitySize >= sizeof(EntityId) && alignedEntitySize == entitySizeCheck, "%d %d != %d", entitySize, alignedEntitySize,
    entitySizeCheck);
  uint8_t initial_bits = 0;
  if (maxComponentSize > 0)
  {
    uint32_t alignTo = eastl::min((uint32_t)get_bigger_pow2(maxComponentSize), (uint32_t)max_alignment); //
    initial_bits = eastl::clamp<int>(get_log2i(alignTo) - minComponentSizeBits, 0, max_alignment_bits);
    // debug("alignTo = %d, maxComponentSize = %d, minComponentSizeBits = %d, initial bits = %d",
    //   alignTo, maxComponentSize, minComponentSizeBits, initial_bits);
  }
  // we keep data aligned in archetype, by simply limiting minimum initial bits

  // first is always zero
  SmallTab<CreatableComponent> creatables;
  SmallTab<ResourceComponent> withResources;
  SmallTab<TrackedPod> trackedPods;
  SmallTab<CreatableComponent> trackedCreatables;
  const uint32_t trackedPodsCidxStart = allTrackedPodsCidx.size();
  ArchetypeInfo info;
  info.init(components, components_cnt);
#if DAECS_EXTENSIVE_CHECKS
  // round-trip oracle: every component resolves to its slot, absent neighbors miss
  DAECS_EXT_ASSERT(info.getComponentId(0) == 0);
  for (uint32_t i = 1; i < components_cnt; ++i)
  {
    DAECS_EXT_ASSERT(info.getComponentId(components[i]) == i);
    const component_index_t below = components[i] - 1, above = components[i] + 1;
    if (below != 0 && !eastl::binary_search(components + 1, components + components_cnt, below))
      DAECS_EXT_ASSERT(info.getComponentId(below) == INVALID_ARCHETYPE_COMPONENT_ID);
    if (!eastl::binary_search(components + 1, components + components_cnt, above))
      DAECS_EXT_ASSERT(info.getComponentId(above) == INVALID_ARCHETYPE_COMPONENT_ID);
  }
  {
    dag::Vector<archetype_component_id> batchIds(components_cnt);
    info.getComponentIds(components, components_cnt, batchIds.data());
    for (uint32_t i = 0; i < components_cnt; ++i)
      DAECS_EXT_ASSERT(batchIds[i] == info.getComponentId(components[i]));
  }
#endif
  if (components_cnt > 1)
  {
    for (archetype_component_id i = 1; i < components_cnt; ++i)
    {
      const component_index_t cidx = components[i];
      const DataComponent dataComponent = dataComponents.getComponentById(cidx);
      if (dataComponent.flags & DataComponent::IS_COPY)
        continue;
      const auto typeIndex = dataComponent.componentType;
      const ComponentType type = componentTypes.getTypeInfo(typeIndex);
      const component_index_t replicatedCIndex = dataComponents.getTrackedPair(cidx);
      if (type.flags & COMPONENT_TYPE_NEED_RESOURCES)
        withResources.emplace_back(ResourceComponent{i, cidx, typeIndex});
      if (type.flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
      {
        creatables.emplace_back(CreatableComponent{i, archetypeComponents.get<DATA_OFFSET>()[componentsAt + i],
          initialComponentDataOffset[i], eastl::move(uint16_t(type.size)), cidx, typeIndex});
      }

      if (replicatedCIndex == INVALID_COMPONENT_INDEX)
        continue;
      const archetype_component_id archetypeTrackedIndex = info.getComponentId(replicatedCIndex);
      if (archetypeTrackedIndex == INVALID_ARCHETYPE_COMPONENT_ID)
        continue;
      G_ASSERT(archetypeTrackedIndex > i);
      G_STATIC_ASSERT(HAS_TRACKED_COMPONENT < eastl::numeric_limits<eastl::underlying_type_t<ComponentTypeFlags>>::max());
      typeFlags |= HAS_TRACKED_COMPONENT;
      G_ASSERT(getComponentSizeFromOfs(i, componentsAt) == type.size);
      if (type.flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE)
      {
        trackedCreatables.emplace_back(
          CreatableComponent{archetypeTrackedIndex, archetypeComponents.get<DATA_OFFSET>()[componentsAt + archetypeTrackedIndex],
            archetypeComponents.get<DATA_OFFSET>()[componentsAt + i], eastl::move(uint16_t(type.size)), cidx, typeIndex});
      }
      else
      {
        G_ASSERT(is_pod(type.flags));
        allTrackedPodsCidx.push_back(cidx);
        trackedPods.emplace_back(
          TrackedPod{archetypeTrackedIndex, archetypeComponents.get<DATA_OFFSET>()[componentsAt + archetypeTrackedIndex],
            archetypeComponents.get<DATA_OFFSET>()[componentsAt + i], eastl::move(uint16_t(type.size))});
      }
    }
  }
  G_ASSERT(components_cnt < 65535 && entitySize <= 65535);
  // Compute bloom bitmask before emplace_back (need component indices which are already stored in archetypeComponents)
  uint64_t bloomBitmask = 0;
  for (uint32_t i = 0; i < components_cnt; ++i)
    bloomBitmask |= componentBit(components[i]);
  archetypes.emplace_back(Archetype{(uint16_t)entitySize, (uint16_t)components_cnt, initial_bits}, eastl::move(uint32_t(componentsAt)),
    eastl::move(info), eastl::move(typeFlags), eastl::move(trackedPods), eastl::move(uint32_t(trackedPodsCidxStart)),
    eastl::move(creatables), eastl::move(trackedCreatables), eastl::move(withResources), eastl::move(bloomBitmask));
  initialOffsets.emplace_back(eastl::move(initialComponentDataOffset));
#if DAECS_EXTENSIVE_CHECKS
  queryingArchetypeCount.emplace_back(0);
  G_ASSERT(queryingArchetypeCount.size() == archetypes.size());
#endif
  G_ASSERT(initialOffsets.size() == archetypes.size());
  return (uint32_t)archetypes.size() - 1;
}

void Archetypes::remap(const archetype_t *map, uint32_t used_count) // supports only ordered removals
{
  // compact archetypeComponents / allTrackedPodsCidx: squeeze out dead archetypes'
  // ranges and rewrite survivors' offsets, so GC actually reclaims descriptor
  // memory. Ranges are appended in archetype order and the map is monotonic, so
  // in-place forward moves never overwrite still-unread data.
  {
    uint32_t compDst = 0, podDst = 0;
    for (uint32_t i = 0, e = archetypes.size(); i < e; ++i)
    {
      if (map[i] == INVALID_ARCHETYPE)
        continue;
      const uint32_t compSrc = archetypes.get<COMPONENT_OFS>()[i];
      const uint32_t compCnt = archetypes.get<ARCHETYPE>()[i].componentsCnt;
      G_ASSERT(compSrc >= compDst);
      if (compSrc != compDst)
      {
        memmove(archetypeComponents.get<INDEX>() + compDst, archetypeComponents.get<INDEX>() + compSrc,
          compCnt * sizeof(component_index_t));
        memmove(archetypeComponents.get<DATA_OFFSET>() + compDst, archetypeComponents.get<DATA_OFFSET>() + compSrc,
          compCnt * sizeof(uint16_t));
        memmove(archetypeComponents.get<DATA_SIZE>() + compDst, archetypeComponents.get<DATA_SIZE>() + compSrc,
          compCnt * sizeof(uint16_t));
      }
      archetypes.get<COMPONENT_OFS>()[i] = compDst;
      compDst += compCnt;

      const uint32_t podSrc = archetypes.get<TRACKED_PODS_CIDX>()[i];
      const uint32_t podCnt = archetypes.get<TRACKED_PODS>()[i].size();
      G_ASSERT(podSrc >= podDst);
      if (podSrc != podDst)
        memmove(allTrackedPodsCidx.data() + podDst, allTrackedPodsCidx.data() + podSrc, podCnt * sizeof(component_index_t));
      archetypes.get<TRACKED_PODS_CIDX>()[i] = podDst;
      podDst += podCnt;
    }
    archetypeComponents.resize(compDst);
    archetypeComponents.shrink_to_fit();
    allTrackedPodsCidx.resize(podDst);
    allTrackedPodsCidx.shrink_to_fit();
  }
  uint32_t eraseCount = 0, currentRemapped = used_count - 1;
  for (int i = archetypes.size() - 1; i >= 0; --i) // not allowes shuffle!
  {
    const archetype_t newA = map[i];
    G_ASSERTF(newA == INVALID_ARCHETYPE || currentRemapped == newA, "%d map=%d, should be %d", i, newA, currentRemapped);
    if (newA == INVALID_ARCHETYPE)
    {
      eraseCount++;
    }
    else
    {
      currentRemapped--;
      if (eraseCount != 0)
      {
        const uint32_t at = i + 1;
        archetypes.erase(archetypes.begin() + at, archetypes.begin() + at + eraseCount);
        initialOffsets.erase(initialOffsets.begin() + at, initialOffsets.begin() + at + eraseCount);
        eraseCount = 0;
      }
    }
  }
  G_UNUSED(currentRemapped);
  if (eraseCount)
  {
    archetypes.erase(archetypes.begin(), archetypes.begin() + eraseCount);
    initialOffsets.erase(initialOffsets.begin(), initialOffsets.begin() + eraseCount);
  }
  G_UNUSED(used_count);
  G_ASSERT(used_count == archetypes.size());
  // may be faster to just create new one
  for (auto it = archetypesByComponents.begin(); it != archetypesByComponents.end();)
  {
    const archetype_t newA = map[it->second];
    if (newA == INVALID_ARCHETYPE)
      it = archetypesByComponents.erase(it);
    else
    {
      it->second = newA;
      ++it;
    }
  }
#if DAECS_EXTENSIVE_CHECKS
  queryingArchetypeCount.resize(used_count);
#endif
}

}; // namespace ecs
