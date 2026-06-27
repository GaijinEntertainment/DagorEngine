// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <runtime/bindlessSlotManager.h>

#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_d3dResource.h>
#include <drv/3d/dag_driverDesc.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <debug/dag_assert.h>

namespace dafg
{

// One descriptor slot per in-flight frame (frame f uses baseBindlessSlot + f).
static constexpr uint32_t FRAME_WINDOW = BarrierScheduler::SCHEDULE_FRAME_WINDOW;

// Allocates FRAME_WINDOW slots per resource and turns each resource's provisional
// in-range index into its final base slot.
static void allocate_slot_range(D3DResourceType type, intermediate::Graph &graph,
  const dag::Vector<intermediate::ResourceIndex, framemem_allocator> &resources, uint32_t &range_base, uint32_t &range_count,
  dag::Vector<D3dResource *> &slot_cache)
{
  if (resources.empty())
    return;
  range_count = resources.size() * FRAME_WINDOW;
  range_base = d3d::allocate_bindless_resource_range(type, range_count);
  for (auto resIdx : resources)
    graph.resources[resIdx].baseBindlessSlot = range_base + graph.resources[resIdx].baseBindlessSlot * FRAME_WINDOW;
  slot_cache.assign(range_count, nullptr);
}

void BindlessSlotManager::rebuild(intermediate::Graph &graph)
{
  freeRanges();

  for (auto resIdx : graph.resources.keys())
    graph.resources[resIdx].baseBindlessSlot = INVALID_SLOT;

  if (!d3d::get_driver_desc().caps.hasBindless)
    return;

  // Collect distinct bindless texture/buffer resources; baseBindlessSlot doubles as
  // dedup marker and provisional in-range index. Samplers are driver-deduped.
  dag::Vector<intermediate::ResourceIndex, framemem_allocator> texResources;
  dag::Vector<intermediate::ResourceIndex, framemem_allocator> bufResources;
  for (auto [nodeIdx, state] : graph.nodeStates.enumerate())
    for (const auto &[bindIdx, binding] : state.bindings)
    {
      if (binding.type != BindingType::BindlessShaderVar || !binding.resource)
        continue;
      const auto resIdx = *binding.resource;

      // Deduplicate: a resource used by several nodes shares one slot range.
      if (graph.resources[resIdx].baseBindlessSlot != INVALID_SLOT)
        continue;

      const auto resType = graph.resources[resIdx].getResType();
      if (resType != ResourceType::Texture && resType != ResourceType::Buffer)
        continue;

      auto &bucket = resType == ResourceType::Texture ? texResources : bufResources;
      graph.resources[resIdx].baseBindlessSlot = bucket.size();
      bucket.push_back(resIdx);
    }

  allocate_slot_range(D3DResourceType::TEX, graph, texResources, texRangeBase, texRangeCount, texSlotResources);
  allocate_slot_range(D3DResourceType::SBUF, graph, bufResources, bufRangeBase, bufRangeCount, bufSlotResources);
}

void BindlessSlotManager::invalidateSlotCache()
{
  for (auto &res : texSlotResources)
    res = nullptr;
  for (auto &res : bufSlotResources)
    res = nullptr;
}

void BindlessSlotManager::updateBindlessResource(bool is_tex, uint32_t slot, D3dResource *res)
{
  auto &cache = is_tex ? texSlotResources : bufSlotResources;
  const uint32_t cacheIdx = slot - (is_tex ? texRangeBase : bufRangeBase);
  G_ASSERT_RETURN(cacheIdx < cache.size(), );

  if (cache[cacheIdx] == res)
    return;

  d3d::update_bindless_resource(is_tex ? D3DResourceType::TEX : D3DResourceType::SBUF, slot, res);
  cache[cacheIdx] = res;
}

void BindlessSlotManager::freeRanges()
{
  if (texRangeCount)
    d3d::free_bindless_resource_range(D3DResourceType::TEX, texRangeBase, texRangeCount);
  if (bufRangeCount)
    d3d::free_bindless_resource_range(D3DResourceType::SBUF, bufRangeBase, bufRangeCount);

  texRangeBase = bufRangeBase = INVALID_SLOT;
  texRangeCount = bufRangeCount = 0;
  texSlotResources.clear();
  bufSlotResources.clear();
}

BindlessSlotManager::~BindlessSlotManager() { freeRanges(); }

} // namespace dafg
