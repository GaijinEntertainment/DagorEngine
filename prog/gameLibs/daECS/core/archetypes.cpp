// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <hash/xxh3.h>
#include <daECS/core/internal/archetypes.h>
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
  hash = XXH3_64bits(components, sizeof(component_index_t) * components_cnt);
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
  G_ASSERTF(entitySize >= sizeof(EntityId) && entitySize < eastl::numeric_limits<uint16_t>::max(), "%d", entitySize);
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
  if (components_cnt > 1)
  {
    component_index_t firstNonEidIndex = components[1], lastIndex = components[components_cnt - 1];
    uint32_t indicesCount = lastIndex - firstNonEidIndex + 1;
    eastl::unique_ptr<uint16_t[]> indicesMap(new uint16_t[indicesCount]);
    memset(indicesMap.get(), 0xFF, indicesCount * sizeof(uint16_t));
    for (int i = 1; i < components_cnt; ++i)
    {
      G_ASSERT(components[i] >= firstNonEidIndex && components[i] <= lastIndex);
      indicesMap[components[i] - firstNonEidIndex] = i;
    }
    info = ArchetypeInfo{firstNonEidIndex, ecs::component_index_t(indicesCount), eastl::move(indicesMap)};

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
  else
    info = ArchetypeInfo{INVALID_COMPONENT_INDEX, 0, nullptr};
  G_ASSERT(components_cnt < 65535 && entitySize <= 65535);
  archetypes.emplace_back(Archetype{(uint16_t)entitySize, (uint16_t)components_cnt, initial_bits}, eastl::move(uint32_t(componentsAt)),
    eastl::move(info), eastl::move(typeFlags), eastl::move(trackedPods), eastl::move(uint32_t(trackedPodsCidxStart)),
    eastl::move(creatables), eastl::move(trackedCreatables), eastl::move(withResources));
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
  // todo:
  // eastl::vector<component_index_t> allTrackedPodsCidx;//fixme, remove unused allTrackedPodsCidx
  // eastl::tuple_vector<component_index_t, uint16_t, uint16_t> archetypeComponents;//fixme, remove unused archetypeComponents
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
