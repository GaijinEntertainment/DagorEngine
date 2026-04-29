// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceAllocator.h"

#include <EASTL/algorithm.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <memory/dag_framemem.h>

#include <common/resourceUsage.h>
#include <id/idRange.h>
#include <debug/backendDebug.h>


CONSOLE_BOOL_VAL("dafg", set_resource_names, DAGOR_DBGLEVEL > 0);

namespace dafg
{

DestroyedHeapSet ResourceAllocator::allocateCpuHeaps(int prev_frame, const HeapRequests &requests,
  const PotentialDeactivationSet &potential_deactivation_set)
{
  DestroyedHeapSet result;
  result.reserve(SCHEDULE_FRAME_WINDOW * cachedIntermediateResources.totalKeys());

  FRAMEMEM_VALIDATE;


  for (const auto [heapIdx, req] : requests.enumerate())
  {
    if (req.group != CPU_HEAP_GROUP)
      continue;

    cpuHeaps.expandMapping(heapIdx);

    // sanity check
    G_ASSERT(allocatedHeaps[heapIdx].size == cpuHeaps[heapIdx].size());

    if (req.size <= cpuHeaps[heapIdx].size())
    {
      debug("daFG: Reusing CPU pseudo-heap %d of size %dKB instead of allocating a new %dKB heap", //
        eastl::to_underlying(heapIdx), allocatedHeaps[heapIdx].size >> 10, req.size >> 10);
      continue;
    }

    if (!cpuHeaps[heapIdx].empty())
    {
      // If we are relocating a CPU heap, we need to call destructors
      // on all CPU resources present in it. Note that resource
      // scheduling guarantees that all of these resources are NOT
      // preserved.
      // Also note that this is not the case for GPU heaps, there,
      // deactivation is a simple memory barrier, which we don't in fact
      // need or want.
      for (const auto resIdx : heapToResourceList[heapIdx][prev_frame])
        if (potential_deactivation_set[resIdx].index() == 3)
        {
          auto [f, x] = eastl::get<3>(potential_deactivation_set[resIdx]);
          f(x);
        }

      result.push_back(heapIdx);

      debug("daFG: Destroyed CPU pseudo-heap %d because it was too small (size was %dKB, but %dKB are required)",
        eastl::to_underlying(heapIdx), allocatedHeaps[heapIdx].size >> 10, req.size >> 10);
    }
    else // sanity check
      G_ASSERT(!heapToResourceList.isMapped(heapIdx) || heapToResourceList[heapIdx][prev_frame].empty());

    cpuHeaps[heapIdx].resize(req.size, 0);
    allocatedHeaps[heapIdx].size = req.size;
    debug("daFG: Successfully allocated a new CPU pseudo-heap %d of size %dKB", //
      eastl::to_underlying(heapIdx), req.size >> 10);
  }

  return result;
}

void ResourceAllocator::placeResources(const intermediate::Graph &graph, const ResourceProperties &resource_properties,
  const AllocationLocations &allocation_locations, const DynamicResolutions &dyn_resolutions,
  const BadResolutionTracker::Corrections &corrections)
{
  TIME_PROFILE(placeResources);

  heapToResourceList.resize(allocatedHeaps.size());

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    IdIndexedMapping<HeapIndex, uint32_t> resCountPerHeap(allocatedHeaps.size(), 0);
    for (auto resIdx : graph.resources.keys())
      if (graph.resources[resIdx].isScheduled())
        ++resCountPerHeap[allocation_locations[frame][resIdx].heap];

    for (auto [heapIdx, count] : resCountPerHeap.enumerate())
    {
      heapToResourceList[heapIdx][frame].clear();
      heapToResourceList[heapIdx][frame].reserve(count);
    }
  }

  debug_clear_resource_placements();

  for (auto [i, res] : graph.resources.enumerate())
  {
    if (!res.isScheduled())
      continue;

    auto resProps = resource_properties[i];

    G_ASSERT(resProps.offsetAlignment != 0);

    for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      auto [heapIdx, offset] = allocation_locations[frame][i];

      if (corrections[frame][i] != 0)
        resProps.sizeInBytes = corrections[frame][i];

      heapToResourceList[heapIdx][frame].push_back(i);

      if (res.asScheduled().isGpuResource())
      {
        DynamicResolution dynResolution = eastl::monostate{};

        if (const auto &type = res.asScheduled().resolutionType; type.has_value() && dyn_resolutions.isMapped(type->id))
          dynResolution = dyn_resolutions[type->id];

        placeResource(frame, i, heapIdx, res.asScheduled().getGpuDescription(), offset, resProps, dynResolution);

        validation_add_resource(getD3dResource(frame, i));
      }
      else
      {
        cpuResources[frame][i] = {static_cast<uint32_t>(offset), heapIdx};
      }

      const bool isCpuHeap = allocatedHeaps[heapIdx].group == CPU_HEAP_GROUP;
      for (auto resId : cachedIntermediateResources[i].frontendResources)
        debug_rec_resource_placement(resId, frame, eastl::to_underlying(heapIdx), offset, resProps.sizeInBytes, isCpuHeap);
    }
  }
  if (set_resource_names.get())
    restoreResourceApiNames();
}

void ResourceAllocator::restoreResourceApiNames() const
{
  FRAMEMEM_REGION;

  struct strInfo
  {
    size_t size;
    eastl::basic_string<char, framemem_allocator> str;
  };

  ska::flat_hash_map<const D3dResource *, strInfo, eastl::hash<const D3dResource *>, eastl::equal_to<const D3dResource *>,
    framemem_allocator>
    names;
  names.reserve(SCHEDULE_FRAME_WINDOW * cachedIntermediateResources.totalKeys());

  const char *separatorStr = ", ";
  // node: size of elements should be equal
  const char *frameStr[SCHEDULE_FRAME_WINDOW] = {"|0", "|1"};
  const size_t frameStrSize = strlen(frameStr[0]);
  const size_t separatorStrSize = strlen(separatorStr);

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto resIdx : cachedIntermediateResources.keys())
    {
      if (!cachedIntermediateResources[resIdx].isScheduled() ||
          cachedIntermediateResources[resIdx].asScheduled().resourceType == ResourceType::Blob)
        continue;

      const D3dResource *pResource = getD3dResource(frame, resIdx);
      auto &strRef = names[pResource];
      if (strRef.size != 0)
        strRef.size += separatorStrSize;
      strRef.size += cachedIntermediateResourceNames[resIdx].length() + frameStrSize;
    }

  for (auto &[d3dRes, str] : names)
    names[d3dRes].str.reserve(str.size);

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto resIdx : cachedIntermediateResources.keys())
    {
      if (!cachedIntermediateResources[resIdx].isScheduled() ||
          cachedIntermediateResources[resIdx].asScheduled().resourceType == ResourceType::Blob)
        continue;

      const D3dResource *pResource = getD3dResource(frame, resIdx);
      auto &name = names[pResource];
      if (!name.str.empty())
        name.str += separatorStr;
      name.str += cachedIntermediateResourceNames[resIdx].c_str();
      name.str += frameStr[frame];
    }

  for (auto &[d3dRes, name] : names)
    d3dRes->setApiName(name.str.c_str());
}

void ResourceAllocator::gatherPotentialDeactivationSet(int prev_frame, PotentialDeactivationSet &result)
{
  result.assign(cachedIntermediateResources.totalKeys(), eastl::monostate{});

  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No)
      switch (res.asScheduled().resourceType)
      {
        case ResourceType::Texture: result[idx] = getTexture(prev_frame, idx); break;
        case ResourceType::Buffer: result[idx] = getBuffer(prev_frame, idx); break;
        case ResourceType::Blob:
          result[idx] = BlobDeactivationRequest{res.asScheduled().getCpuDescription().dtor, getBlob(prev_frame, idx).data};
          break;
        default: G_ASSERT(false); break;
      }
}

void ResourceAllocator::closeTransientResources()
{
  TIME_PROFILE(closeTransientResources);
  validation_restart();
}

void ResourceAllocator::setManagedName(int frame, intermediate::ResourceIndex res_idx, D3dResource *res)
{
  // Unique names have to be per frame & per IR resources
  const auto &irName = cachedIntermediateResourceNames[res_idx];
  eastl::basic_string<char, framemem_allocator> buffer;
  static constexpr const char FMT[] = " | %d";
  buffer.reserve(irName.size() + countof(FMT));
  buffer.append(irName.data(), irName.size());
  buffer.append_sprintf(FMT, frame);
  res->setName(buffer.data(), buffer.size());
}

void ResourceAllocator::refreshManagedTexture(int frame, intermediate::ResourceIndex res_idx, BaseTexture *tex)
{
  setManagedName(frame, res_idx, tex);
}

void ResourceAllocator::applySchedule(int prev_frame, const ResourceSchedule &schedule, const intermediate::Graph &graph,
  const DynamicResolutions &dyn_resolutions, const BadResolutionTracker::Corrections &corrections,
  PotentialDeactivationSet &potentialDeactivations)
{
  cachedIntermediateResources = graph.resources;
  cachedIntermediateResourceNames = graph.resourceNames;

  auto &allocationLocations = schedule.allocationLocations;
  auto &heapRequests = schedule.heapRequests;

  const auto oldHeapCount = allocatedHeaps.size();

  // Start tracking newly requested heap groups
  if (allocatedHeaps.size() < heapRequests.size())
    for (auto it = heapRequests.begin() + allocatedHeaps.size(); it != heapRequests.end(); ++it)
      allocatedHeaps.appendNew(HeapRequest{it->group, 0u});

  {
    // allocateHeaps might destroy some physical resources due to
    // their heap being re-allocated, so don't try to deactivate those
    // (otherwise we'll get assertions and maybe a crash)
    const auto wipeDeactivations = [this, prev_frame, &potentialDeactivations](const DestroyedHeapSet &set) {
      for (auto heapIdx : set)
        for (auto resIdx : heapToResourceList[heapIdx][prev_frame])
          potentialDeactivations[resIdx] = eastl::monostate{};
    };

    {
      const auto destroyedGpuHeaps = allocateHeaps(heapRequests);
      // NOTE: this method is responsible for updating allocatedHeaps size
      wipeDeactivations(destroyedGpuHeaps);
    }

    {
      const auto destroyedCpuHeaps = allocateCpuHeaps(prev_frame, heapRequests, potentialDeactivations);
      wipeDeactivations(destroyedCpuHeaps);
    }
  }

  for (int i = 0; i < SCHEDULE_FRAME_WINDOW; ++i)
    cpuResources[i].assign(cachedIntermediateResources.totalKeys(), CpuResourcePlacement{UNSCHEDULED, HeapIndex::Invalid});

  resetResourcePlacement();
  placeResources(graph, schedule.resourceProperties, allocationLocations, dyn_resolutions, corrections);

  // Without native heaps it is perfectly normal to have single-resource heaps.
  if (!d3d::get_driver_code().is(d3d::dx11))
    for (auto i = oldHeapCount; i < heapToResourceList.size(); ++i)
      if (const auto &list = heapToResourceList[static_cast<HeapIndex>(i)][0]; list.size() == 1)
        logwarn("daFG: Heap %d had to be created for containing a single resource '%s'", i,
          cachedIntermediateResourceNames[list[0]].c_str());

  for (int frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    for (auto [resIdx, irRes] : cachedIntermediateResources.enumerate())
    {
      if (!irRes.isScheduled() || !irRes.asScheduled().isGpuResource())
        continue;

      D3dResource *res = getD3dResource(frame, resIdx);
      if (!res)
        continue;

      G_ASSERT(irRes.asScheduled().resourceType == ResourceType::Texture || irRes.asScheduled().resourceType == ResourceType::Buffer);
      setManagedName(frame, resIdx, res);
    }
  }
}

void ResourceAllocator::shutdown(int frame)
{
  closeTransientResources();

  // Call destructor on all CPU resources. Asan complains otherwise.
  for (auto [i, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No && res.asScheduled().isCpuResource())
      res.asScheduled().getCpuDescription().dtor(getBlob(frame, i).data);
  cpuHeaps.clear();

  resetResourcePlacement();
  for (int i = 0; i < SCHEDULE_FRAME_WINDOW; ++i)
  {
    eastl::fill(cpuResources[i].begin(), cpuResources[i].end(), CpuResourcePlacement{UNSCHEDULED, HeapIndex::Invalid});
  }

  shutdownInternal();

  heapToResourceList.clear();
  allocatedHeaps.clear();
  cachedIntermediateResources.clear();
}

BlobView ResourceAllocator::getBlob(int frame, intermediate::ResourceIndex res_idx)
{
  const auto [offset, heapIdx] = cpuResources[frame][res_idx];
  G_ASSERT_RETURN(offset != UNSCHEDULED && heapIdx != HeapIndex::Invalid, {});

  return BlobView{cpuHeaps[heapIdx].data() + offset, cachedIntermediateResources[res_idx].asScheduled().getCpuDescription().typeTag};
}

BaseTexture *ResourceAllocator::getTexture(int frame, intermediate::ResourceIndex res_idx)
{
  const auto resPtr = getD3dResource(frame, res_idx);
  resPtr->setName(cachedIntermediateResourceNames[res_idx].c_str(), cachedIntermediateResourceNames[res_idx].size());
  return static_cast<BaseTexture *>(resPtr);
}

Sbuffer *ResourceAllocator::getBuffer(int frame, intermediate::ResourceIndex res_idx)
{
  const auto resPtr = getD3dResource(frame, res_idx);
  resPtr->setName(cachedIntermediateResourceNames[res_idx].c_str(), cachedIntermediateResourceNames[res_idx].size());
  return static_cast<Sbuffer *>(resPtr);
}

void ResourceAllocator::emergencyWipeBlobs(int frame, dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> resources)
{
  for (auto [resIdx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().isCpuResource() && resources.count(res.frontendResources.front()))
    {
      auto &desc = res.asScheduled().getCpuDescription();
      if (res.asScheduled().history != History::No)
        desc.dtor(getBlob(frame, resIdx).data);

      res.asScheduled().history = History::No;

      debug("daFG: Wiped resource %s", cachedIntermediateResourceNames[resIdx]);
    }
}

} // namespace dafg
