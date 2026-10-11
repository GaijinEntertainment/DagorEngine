// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceAllocator.h"
#include "layoutUpdate.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include <perfMon/dag_statDrv.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <memory/dag_framemem.h>

#include <common/resourceUsage.h>
#include <id/idRange.h>
#include <debug/backendDebug.h>


CONSOLE_BOOL_VAL("dafg", set_resource_names, DAGOR_DBGLEVEL > 0);
CONSOLE_BOOL_VAL("dafg", validate_allocator, false);

namespace dafg
{

static uint64_t cpu_storage_size(const LayoutHeap &heap) { return heap.heapClass.group == CPU_HEAP_GROUP ? heap.reservedSize : 0; }

void ResourceAllocator::allocateCpuHeaps(int prev_frame, const LayoutHeaps &layout_heaps,
  const PotentialDeactivationSet &potential_deactivation_set, bool defer_destruction, ChangedHeaps &changed)
{
  TIME_PROFILE(allocateCpuHeaps);

  FRAMEMEM_VALIDATE;

  IdIndexedFlags<HeapIndex, framemem_allocator> dropsStorage(layout_heaps.size(), false);
  bool anyDropsStorage = false;
  for (const auto [heapIdx, heap] : layout_heaps.enumerate())
  {
    cpuHeaps.expandMapping(heapIdx);
    if (!cpuHeaps[heapIdx].empty() && cpu_storage_size(heap) != cpuHeaps[heapIdx].size())
    {
      dropsStorage.set(heapIdx, true);
      anyDropsStorage = true;
    }
  }
  if (anyDropsStorage)
    for (auto &&[resIdx, entry] : placed[prev_frame].enumerate())
      if (entry.where.heap != HeapIndex::Invalid && dropsStorage.test(entry.where.heap, false))
        if (auto *blob = eastl::get_if<BlobDeactivationRequest>(&potential_deactivation_set[resIdx]))
          blob->destructor(blob->blob);

  for (const auto [heapIdx, heap] : layout_heaps.enumerate())
  {
    auto req = make_heap_request(heap);

    const bool leftCpu = req.group != CPU_HEAP_GROUP;
    if (leftCpu && cpuHeaps[heapIdx].empty())
      continue;
    if (leftCpu)
      req.size = 0;

    G_ASSERT(leftCpu || allocatedHeaps[heapIdx].size == cpuHeaps[heapIdx].size());

    if (req.size == cpuHeaps[heapIdx].size())
      continue;

    if (!cpuHeaps[heapIdx].empty())
      debug("daFG: Destroyed CPU pseudo-heap %d because its size changed (was %dKB, now %dKB)", eastl::to_underlying(heapIdx),
        allocatedHeaps[heapIdx].size >> 10, req.size >> 10);

    changed.set(heapIdx, true);

    if (defer_destruction && !cpuHeaps[heapIdx].empty())
      retiredCpuHeaps.push_back(eastl::move(cpuHeaps[heapIdx]));
    cpuHeaps[heapIdx] = {};
    cpuHeaps[heapIdx].resize(req.size, 0);
    if (!leftCpu)
      allocatedHeaps[heapIdx] = req;
    if (req.size > 0)
      debug("daFG: Successfully allocated a new CPU pseudo-heap %d of size %dKB", //
        eastl::to_underlying(heapIdx), req.size >> 10);
  }
}

D3dResource *ResourceAllocator::getD3dResource(int frame, intermediate::ResourceIndex res_idx) const
{
  D3dResource *object = placed[frame].isMapped(res_idx) ? placed[frame][res_idx].object : nullptr;
  G_ASSERT_LOG_ONCE(object, "daFG: resource '%s' has nothing placed for frame %d!", cachedIntermediateResourceNames[res_idx].c_str(),
    frame);
  return object;
}

void ResourceAllocator::updateCachedResources(const intermediate::Graph &graph, const ResourcesChangedFmem &resources_changed,
  const intermediate::HistoryPairing &historyPairing, bool does_pairing_keep_indices)
{
  TIME_PROFILE(updateCachedResources);

  const size_t newCount = graph.resources.totalKeys();

  if (does_pairing_keep_indices)
  {
    IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator> changed(newCount, false);
    for (auto &&[idx, oldIdx] : historyPairing.enumerate())
      if (changed.isMapped(idx) && (oldIdx == intermediate::RESOURCE_NOT_MAPPED || resources_changed.test(idx, true)))
        changed.set(idx, true);
    cachedIntermediateResources.updateFrom(graph.resources, changed);
    cachedIntermediateResourceNames.updateFrom(graph.resourceNames, changed);
  }
  else
  {
    const auto newIndexOf = invert_pairing(historyPairing);

    cachedIntermediateResources = graph.resources;
    cachedIntermediateResourceNames = graph.resourceNames;

    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      IdIndexedMapping<intermediate::ResourceIndex, PlacedResource> moved(newCount, PlacedResource{});
      for (auto &&[idx, oldIdx] : historyPairing.enumerate())
        if (oldIdx != intermediate::RESOURCE_NOT_MAPPED && placed[frame].isMapped(oldIdx))
          moved[idx] = placed[frame][oldIdx];
      placed[frame] = eastl::move(moved);
    }
    for (auto &users : objectUsers)
      for (auto &what : users.second)
        if (newIndexOf.isMapped(what.resIdx) && newIndexOf[what.resIdx] != intermediate::RESOURCE_NOT_MAPPED)
          what.resIdx = newIndexOf[what.resIdx];
  }

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    placed[frame].resize(newCount, PlacedResource{});
}

void ResourceAllocator::placeFrameResource(FrameResource what, AllocationLocation location, uint64_t size,
  const ResourceProperties &properties, const DynamicResolutions &dyn_resolutions, TouchedObjects &touched)
{
  const auto [resIdx, frame] = what;
  const auto &res = cachedIntermediateResources[resIdx];
  G_ASSERT_RETURN(res.isScheduled(), );

  auto resProps = properties[resIdx];
  G_ASSERT(resProps.offsetAlignment != 0);
  resProps.sizeInBytes = size;

  G_ASSERT(placed[frame][resIdx].where.heap == HeapIndex::Invalid);
  placed[frame][resIdx].where = location;
  placed[frame][resIdx].size = resProps.sizeInBytes;

  if (res.asScheduled().isGpuResource())
  {
    DynamicResolution dynResolution = eastl::monostate{};
    if (const auto &type = res.asScheduled().resolutionType; type.has_value() && dyn_resolutions.isMapped(type->id))
      dynResolution = dyn_resolutions[type->id];

    D3dResource *object =
      placeResource(resIdx, location.heap, res.asScheduled().getGpuDescription(), location.offset, resProps, dynResolution);
    placed[frame][resIdx].object = object;
    if (object != nullptr)
    {
      validation_add_resource(object);
      setManagedName(frame, resIdx, object);
      if (set_resource_names.get())
      {
        objectUsers[object].push_back(what);
        touched.push_back(object);
      }
    }
  }

  ++stats.frameResourcesPlaced;
}

void ResourceAllocator::dropPlacement(FrameResource what, uint32_t keep_frames)
{
  auto &entry = placed[what.frame][what.resIdx];
  const HeapIndex heapIdx = eastl::exchange(entry.where, NOWHERE).heap;

  if (const D3dResource *object = eastl::exchange(entry.object, nullptr))
  {
    releaseObject(heapIdx, object, keep_frames);
    removeObjectUser(object, what);
  }
}

void ResourceAllocator::removeObjectUser(const D3dResource *object, FrameResource what)
{
  if (auto it = objectUsers.find(object); it != objectUsers.end())
  {
    auto &users = it->second;
    users.erase(eastl::remove(users.begin(), users.end(), what), users.end());
    if (users.empty())
      objectUsers.erase(it);
  }
}

void ResourceAllocator::rebindObject(FrameResource what, D3dResource *object)
{
  auto &entry = placed[what.frame][what.resIdx];
  const D3dResource *old = eastl::exchange(entry.object, object);
  if (old != nullptr)
    releaseObject(entry.where.heap, old, 0);
  if (old == object)
    return;

  if (old != nullptr)
    removeObjectUser(old, what);
  if (object != nullptr && set_resource_names.get())
    objectUsers[object].push_back(what);
}

void ResourceAllocator::unplaceFrameResource(FrameResource what, TouchedObjects &touched, uint32_t keep_frames)
{
  const D3dResource *object = placed[what.frame][what.resIdx].object;
  dropPlacement(what, keep_frames);
  if (object != nullptr && objectUsers.find(object) != objectUsers.end())
    touched.push_back(object);
}

static ResourceSetFmem mark_paired_old_indices(const intermediate::HistoryPairing &pairing, size_t old_count)
{
  ResourceSetFmem result(old_count, false);
  for (const auto oldIdx : pairing.values())
    if (oldIdx != intermediate::RESOURCE_NOT_MAPPED)
      result.set(oldIdx, true);
  return result;
}

void ResourceAllocator::releaseUnpaired(const ResourceSetFmem &paired, bool does_pairing_keep_indices,
  const IdIndexedMapping<intermediate::ResourceIndex, uint32_t> &keep_frames, TouchedObjects &touched)
{
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto &&[resIdx, entry] : placed[frame].enumerate())
      if (entry.where.heap != HeapIndex::Invalid && !paired.test(resIdx, false))
        unplaceFrameResource(
          {
            .resIdx = resIdx,
            .frame = frame,
          },
          touched, does_pairing_keep_indices && keep_frames.isMapped(resIdx) ? keep_frames[resIdx] : 0);
}

void ResourceAllocator::forgetChangedHeaps(int prev_frame, const ChangedHeaps &changed,
  PotentialDeactivationSet &potential_deactivations)
{
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto &&[resIdx, entry] : placed[frame].enumerate())
    {
      if (entry.where.heap == HeapIndex::Invalid || !changed.test(entry.where.heap, false))
        continue;
      if (frame == static_cast<uint32_t>(prev_frame) && potential_deactivations.isMapped(resIdx))
        potential_deactivations[resIdx] = eastl::monostate{};
      dropPlacement({
        .resIdx = resIdx,
        .frame = frame,
      });
    }
}

void ResourceAllocator::updateApiNames(TouchedObjects &touched) const
{
  TIME_PROFILE(updateApiNames);
  eastl::sort(touched.begin(), touched.end());
  touched.erase(eastl::unique(touched.begin(), touched.end()), touched.end());
  FRAMEMEM_REGION;

  eastl::basic_string<char, framemem_allocator> name;
  for (const D3dResource *object : touched)
  {
    const auto it = objectUsers.find(object);
    if (it == objectUsers.end())
      continue;
    name.clear();
    for (const auto [resIdx, frame] : it->second)
    {
      if (!name.empty())
        name += ", ";
      name += cachedIntermediateResourceNames[resIdx].c_str();
      name.append_sprintf("|%d", frame);
    }
    object->setApiName(name.c_str());
  }
}

void ResourceAllocator::gatherPotentialDeactivationSet(int prev_frame, PotentialDeactivationSet &result)
{
  TIME_PROFILE(gatherPotentialDeactivationSet);
  result.assign(cachedIntermediateResources.totalKeys(), eastl::monostate{});

  for (auto [idx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().history != History::No && !wipedBlobs.test(idx, false))
    {
      const auto &releaseBarrier = cachedReleaseBarriers[idx];
      switch (res.asScheduled().resourceType)
      {
        case ResourceType::Texture:
        {
          eastl::optional<d3d::TextureBarrier> release;
          if (auto *bar = eastl::get_if<d3d::TextureBarrier>(&releaseBarrier))
            release = *bar;
          result[idx] = TextureDeactivation{getTexture(prev_frame, idx), release};
          break;
        }
        case ResourceType::Buffer:
        {
          eastl::optional<d3d::BufferBarrier> release;
          if (auto *bar = eastl::get_if<d3d::BufferBarrier>(&releaseBarrier))
            release = *bar;
          result[idx] = BufferDeactivation{getBuffer(prev_frame, idx), release};
          break;
        }
        case ResourceType::Blob:
          result[idx] = BlobDeactivationRequest{res.asScheduled().getCpuDescription().dtor, getBlob(prev_frame, idx).data};
          break;
        default: G_ASSERT(false); break;
      }
    }

  wipedBlobs.clear();
}

void ResourceAllocator::closeTransientResources()
{
  TIME_PROFILE(closeTransientResources);
  validation_restart();
}

void ResourceAllocator::setManagedName(int frame, intermediate::ResourceIndex res_idx, D3dResource *res)
{
  // names are debug only, and a resize renames on every frame
  if (!set_resource_names.get())
    return;
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

bool ResourceAllocator::doTablesAgree(const Layout &layout) const
{
  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
  {
    const auto req = make_heap_request(heap);
    if (!allocatedHeaps.isMapped(heapIdx))
      return false;
    const auto &allocated = allocatedHeaps[heapIdx];
    if (req.size > 0 && (allocated.group != req.group || allocated.untracked != req.untracked || allocated.size != req.size))
      return false;
    const size_t cpuSize = cpuHeaps.isMapped(heapIdx) ? cpuHeaps[heapIdx].size() : 0;
    if (cpuSize != (req.group == CPU_HEAP_GROUP ? req.size : 0))
      return false;
    if (req.group != CPU_HEAP_GROUP && !doesHeapAgree(heapIdx, req))
      return false;
  }
  return true;
}

const ResourceAllocator::Relocation *ResourceAllocator::findRelocation(intermediate::ResourceIndex res_idx) const
{
  for (const auto &relocation : pendingRelocations)
    if (relocation.resIdx == res_idx)
      return &relocation;
  return nullptr;
}

void ResourceAllocator::finishRelocations()
{
  destroyRetired();
  retiredCpuHeaps.clear();
  pendingRelocations.clear();
}

void ResourceAllocator::applySchedule(int prev_frame, const Layout &layout, const PlacementsChanged &placements_changed,
  const IdIndexedFlags<intermediate::ResourceIndex> &relocated,
  const IdIndexedMapping<intermediate::ResourceIndex, uint32_t> &keep_frames, const ResourceProperties &properties,
  const intermediate::Graph &graph, const ResourcesChangedFmem &resources_changed, const intermediate::HistoryPairing &historyPairing,
  const ReleaseBarriers &release_barriers, const DynamicResolutions &dyn_resolutions,
  PotentialDeactivationSet &potential_deactivations)
{
  TIME_PROFILE(applySchedule);
  FRAMEMEM_VALIDATE;
  stats = {
    .schedulesApplied = stats.schedulesApplied + 1,
  };

  G_ASSERT(pendingRelocations.empty());
  for (const auto idx : relocated.trueKeys())
  {
    const auto oldIdx = historyPairing.isMapped(idx) ? historyPairing[idx] : intermediate::RESOURCE_NOT_MAPPED;
    if (oldIdx == intermediate::RESOURCE_NOT_MAPPED || !placed[prev_frame].isMapped(oldIdx))
      continue;
    const auto &entry = placed[prev_frame][oldIdx];
    if (entry.where.heap == HeapIndex::Invalid)
      continue;
    const bool cpu = allocatedHeaps[entry.where.heap].group == CPU_HEAP_GROUP;
    pendingRelocations.push_back(Relocation{
      .resIdx = idx,
      .object = entry.object,
      .blob = cpu ? cpuHeaps[entry.where.heap].data() + entry.where.offset : nullptr,
    });
  }
  const bool deferHeaps = eastl::any_of(pendingRelocations.begin(), pendingRelocations.end(),
    [](const Relocation &relocation) { return relocation.blob == nullptr; });
  const bool deferCpuHeaps = eastl::any_of(pendingRelocations.begin(), pendingRelocations.end(),
    [](const Relocation &relocation) { return relocation.blob != nullptr; });
  stats.relocations = static_cast<uint32_t>(pendingRelocations.size());

  if (allocatedHeaps.size() < layout.heaps.size())
    for (auto it = layout.heaps.begin() + allocatedHeaps.size(); it != layout.heaps.end(); ++it)
      allocatedHeaps.appendNew(HeapRequest{
        .group = it->heapClass.group,
        .size = 0,
        .untracked = it->heapClass.untracked,
      });

  ChangedHeaps changedHeaps(allocatedHeaps.size(), false);
  allocateHeaps(layout.heaps, deferHeaps, changedHeaps);
  allocateCpuHeaps(prev_frame, layout.heaps, potential_deactivations, deferCpuHeaps, changedHeaps);
  if (const auto changed = changedHeaps.trueKeys(); changed.begin() != changed.end())
    forgetChangedHeaps(prev_frame, changedHeaps, potential_deactivations);

  const bool pairingKeepsIndices = intermediate::does_pairing_keep_indices(historyPairing);
  // Declared before the touched list, which grows above it in frame memory
  const auto paired = mark_paired_old_indices(historyPairing, cachedIntermediateResources.totalKeys());
  TouchedObjects touched;
  releaseUnpaired(paired, pairingKeepsIndices, keep_frames, touched);

  updateCachedResources(graph, resources_changed, historyPairing, pairingKeepsIndices);
  cachedReleaseBarriers = release_barriers;

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (const auto resIdx : placements_changed[frame].trueKeys())
      if (placed[frame].isMapped(resIdx) && placed[frame][resIdx].where.heap != HeapIndex::Invalid)
        unplaceFrameResource(
          {
            .resIdx = resIdx,
            .frame = frame,
          },
          touched, keep_frames.isMapped(resIdx) ? keep_frames[resIdx] : 0);

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (const auto resIdx : placements_changed[frame].trueKeys())
    {
      const FrameResource what{
        .resIdx = resIdx,
        .frame = frame,
      };
      if (const LayoutRect *rect = layout.find(what))
        placeFrameResource(what, layout.where[what], rect->size, properties, dyn_resolutions, touched);
    }

  for (const auto heapIdx : changedHeaps.trueKeys())
    for (const auto &rect : layout.heaps[heapIdx].rects)
      if (placed[rect.what.frame][rect.what.resIdx].where.heap == HeapIndex::Invalid)
        placeFrameResource(rect.what,
          {
            .heap = heapIdx,
            .offset = rect.offset,
          },
          rect.size, properties, dyn_resolutions, touched);

  if (set_resource_names.get())
    updateApiNames(touched);

  lruCleanup();

  if (validate_allocator.get())
    G_ASSERT(doTablesAgree(layout));
}

void ResourceAllocator::shutdown(int frame)
{
  finishRelocations();
  closeTransientResources();

  // Call destructor on all CPU resources. Asan complains otherwise.
  for (auto [i, res] : cachedIntermediateResources.enumerate())
    if (
      res.isScheduled() && res.asScheduled().history != History::No && res.asScheduled().isCpuResource() && !wipedBlobs.test(i, false))
      res.asScheduled().getCpuDescription().dtor(getBlob(frame, i).data);
  cpuHeaps.clear();
  wipedBlobs.clear();

  for (auto &perFrame : placed)
    for (auto &entry : perFrame)
      entry = PlacedResource{};
  objectUsers.clear();

  shutdownInternal();

  allocatedHeaps.clear();
  cachedIntermediateResources.clear();
}

BlobView ResourceAllocator::getBlob(int frame, intermediate::ResourceIndex res_idx)
{
  const auto [heapIdx, offset] = placed[frame][res_idx].where;
  G_ASSERT_RETURN(offset != UNSCHEDULED && heapIdx != HeapIndex::Invalid, {});

  return BlobView{cpuHeaps[heapIdx].data() + offset, cachedIntermediateResources[res_idx].asScheduled().getCpuDescription().typeTag};
}

BaseTexture *ResourceAllocator::getTexture(int frame, intermediate::ResourceIndex res_idx)
{
  const auto resPtr = getD3dResource(frame, res_idx);
  if (resPtr)
    resPtr->setName(cachedIntermediateResourceNames[res_idx].c_str(), cachedIntermediateResourceNames[res_idx].size());
  return static_cast<BaseTexture *>(resPtr);
}

Sbuffer *ResourceAllocator::getBuffer(int frame, intermediate::ResourceIndex res_idx)
{
  const auto resPtr = getD3dResource(frame, res_idx);
  if (resPtr)
    resPtr->setName(cachedIntermediateResourceNames[res_idx].c_str(), cachedIntermediateResourceNames[res_idx].size());
  return static_cast<Sbuffer *>(resPtr);
}

auto ResourceAllocator::emergencyWipeBlobs(int frame,
  const dag::VectorSet<ResNameId, eastl::less<ResNameId>, framemem_allocator> &resources)
  -> dag::Vector<intermediate::ResourceIndex, framemem_allocator>
{
  dag::Vector<intermediate::ResourceIndex, framemem_allocator> wiped;
  wipedBlobs.resize(cachedIntermediateResources.totalKeys(), false);
  for (auto [resIdx, res] : cachedIntermediateResources.enumerate())
    if (res.isScheduled() && res.asScheduled().isCpuResource() && resources.count(res.frontendResources.front()))
    {
      if (res.asScheduled().history != History::No && !wipedBlobs.test(resIdx, false))
        res.asScheduled().getCpuDescription().dtor(getBlob(frame, resIdx).data);

      wipedBlobs.set(resIdx, true);
      wiped.push_back(resIdx);

      debug("daFG: Wiped resource %s", cachedIntermediateResourceNames[resIdx]);
    }
  return wiped;
}

} // namespace dafg
