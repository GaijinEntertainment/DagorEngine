// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/array.h>
#include <EASTL/optional.h>
#include <dag/dag_vectorMap.h>

#include <id/idIndexedFlags.h>
#include <backend/badResolutionTracker.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>
#include <backend/resourceScheduling/resourcePropertyProvider.h>
#include <backend/resourceScheduling/heapPolicy.h>
#include <backend/resourceScheduling/layoutUpdate.h>


namespace dafg
{

// Applies the settings to our console variables, which the console may then
// override, so this must only be called on startup.
void load_resource_scheduling_settings();

struct HeapsToDefragment
{
  // Allocated before `chosen` and declared first. So frame memory is freed in
  // reverse.
  ClassWaste waste;
  bool everyHeap = true;
  dag::Vector<HeapIndex, framemem_allocator> chosen;

  bool includes(HeapIndex heap) const { return everyHeap || eastl::find(chosen.begin(), chosen.end(), heap) != chosen.end(); }
};

using ReservedSizesFmem = IdIndexedMapping<HeapIndex, uint64_t, framemem_allocator>;

inline Timeline make_timeline(const intermediate::Graph &graph)
{
  return {
    .timepointsPerFrame = static_cast<uint32_t>(graph.nodes.totalKeys()) + 1,
  };
}

class ResourceScheduler
{
public:
  using ResourcesChangedFmem = IdIndexedFlags<intermediate::ResourceIndex, framemem_allocator>;

  struct SchedulingContext
  {
    const intermediate::Graph &graph;
    const ResourceLifetimes &lifetimes;
    const ResourcesChangedFmem &resourcesChanged;
    const ResourcesChangedFmem &lifetimesChanged;
    const intermediate::HistoryPairing &historyPairing;
    const BadResolutionTracker::Corrections &corrections;
    IResourcePropertyProvider &propertyProvider;
    uint32_t frameIndex = 0;
    // This recompilation removed nodes, so the frame has less to render
    bool lightFrame = false;
    // computeSchedule fills these three
    Timeline timeline{};
    uint32_t boundary = 0;
    int prevFrame = 0;
  };

  void resetIncrementalState() { *this = ResourceScheduler(); }

  void computeSchedule(int prev_frame, SchedulingContext ctx);

  // Call once per frame, the policy counts its delays in polls. True asks for a
  // recompilation that runs an armed defragmentation.
  bool isDefragmentationDue(uint32_t frame_index, bool after_node_drop);

  // res_idx is a ResourceIndex of the graph that computeSchedule just
  // scheduled (the freshly compiled intermediateGraph), NOT of the previously
  // cached graph: preservedResources is filled against the new graph's indices.
  bool isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const;

  // Frame resources of the previous frame
  const IdIndexedFlags<intermediate::ResourceIndex> &relocations() const { return relocatedResources; }

  // In frames, and the object caches tick once per frame. 0 means the default
  // window.
  const IdIndexedMapping<intermediate::ResourceIndex, uint32_t> &objectKeepFrames() const { return keepFrames; }

  void invalidateTemporalResources() { historyInvalid = true; }
  // Takes an index of the graph the allocator last applied
  void invalidateTemporalResource(intermediate::ResourceIndex resource) { historyInvalidFor.push_back(resource); }

  const Layout &currentLayout() const { return layout; }
  const PlacementsChanged &placementsChanged() const { return changedPlacements; }
  const ResourceProperties &resourceProperties() const { return properties; }

  bool isStateValid() const;

private:
  struct HeapGroupInfo
  {
    HeapLimits limits{};
    HeapPolicySettings policy{};
    bool onChip = false;
    eastl::array<DefragmentationState, 2> defragmentation{};
  };

  struct SlotStats
  {
    HeapClass lastClass{};
    int64_t vacatedAt = -1;
    uint32_t refills = 0;
    uint32_t awayAverage = 0;
  };

  struct Ghost
  {
    intermediate::ResourceIndex slot = intermediate::RESOURCE_NOT_MAPPED;
    HeapClass heapClass{};
    uint64_t size = 0;
    int64_t until = -1;
  };

  void refreshGroupInfo(HeapGroupInfo &info, ResourceHeapGroup *group, IResourcePropertyProvider &provider) const;
  HeapGroupInfo &getGroupInfo(ResourceHeapGroup *group, IResourcePropertyProvider &provider);
  uint64_t reservedOf(HeapClass heap_class) const;
  eastl::optional<HeapClass> findOnChipClass() const;
  bool isOnChipNative(HeapClass on_chip, FrameResource what) const;
  bool isRectOnChip(FrameResource what, const SchedulingContext &ctx);

  void rekeyForPairing(const SchedulingContext &ctx);
  void trimCaches(size_t res_count);
  void remapCaches(const SchedulingContext &ctx, const InverseOfPairingFmem &inverse);
  void noteDeparture(intermediate::ResourceIndex idx, HeapClass heap_class, uint64_t size, uint64_t align, uint32_t frame);
  void noteRefill(intermediate::ResourceIndex idx, HeapClass heap_class, uint32_t frame);
  Ghost *findGhostOf(intermediate::ResourceIndex idx);
  void expireGhosts(uint32_t frame);
  uint64_t ghostBytes(HeapClass heap_class) const;
  WantedRect makeWantedRect(FrameResource what, HeapClass heap_class, const SchedulingContext &ctx) const;
  WantedRect leaveOnChip(FrameResource what, const SchedulingContext &ctx);
  HeapClass keepPromotedClass(FrameResource what, HeapClass natural, uint64_t size, const SchedulingContext &ctx);
  ResourceSetFmem collectResourcesToRevisit(const SchedulingContext &ctx, size_t old_count) const;
  void stripTo(const WantedRect &wanted, WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, const SchedulingContext &ctx);
  void dropGoneSlot(intermediate::ResourceIndex idx);
  void updateRect(FrameResource what, const WantedRect &want, const Preservation &preservation, WantedRectsFmem &to_be_placed,
    FrameResourceSet &stripped, FrameResourceSet &evicted, const SchedulingContext &ctx);
  void updateResource(intermediate::ResourceIndex idx, const SchedulingContext &ctx, const Preservation &preservation,
    WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, FrameResourceSet &evicted);
  void updateLayout(const SchedulingContext &ctx, const ResourceSetFmem &revisit, const Preservation &preservation,
    WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, FrameResourceSet &evicted);
  void splitOnChipQueue(HeapClass on_chip, WantedRectsFmem &to_be_placed, WantedRectsFmem &natives, WantedRectsFmem &guests) const;
  void reclaimLeftNatives(HeapClass on_chip, WantedRectsFmem &natives, WantedRectsFmem &to_be_placed, const SchedulingContext &ctx);
  void packOnChipNatives(HeapClass on_chip, WantedRectsFmem &natives, WantedRectsFmem &guests, bool from_zero, bool &defragment,
    WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, const Preservation &preservation);
  bool promoteGuests(HeapClass on_chip, const WantedRectsFmem &to_be_placed, bool from_layout, eastl::span<const WantedRect> guests,
    WantedRectsFmem &promoted, const SchedulingContext &ctx);
  void packOnChipGuests(HeapClass on_chip, eastl::span<const WantedRect> guests, WantedRectsFmem &to_be_placed,
    const SchedulingContext &ctx, const Preservation &preservation);
  void erasePromotedFromOtherClasses(HeapClass on_chip, WantedRectsFmem &to_be_placed, size_t res_count) const;
  void packOnChip(HeapClass on_chip, WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, bool defragment_all,
    const Preservation &preservation);
  DefragmentationDecision decideDefragmentation(HeapClass heap_class, DefragmentationState &state, const HeapGroupInfo &info,
    size_t rects_in_layout, size_t to_be_placed, uint64_t reserved, uint64_t ghost, bool defragment_all, const SchedulingContext &ctx,
    const Preservation &preservation, HeapsToDefragment &heaps_to_defragment) const;
  void settleDefragmentationState(HeapClass heap_class, DefragmentationState &state, const DefragmentationDecision &decision,
    uint64_t ghost, const HeapLimits &limits, const Preservation &preservation);
  void packClass(HeapClass heap_class, eastl::span<const WantedRect> queued, const SchedulingContext &ctx, bool defragment_all,
    const Preservation &preservation, FrameResourceSet &relocating);
  void packAllClasses(WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, bool defragment_all,
    const Preservation &preservation, FrameResourceSet &relocating);
  void markPreserved(const SchedulingContext &ctx, const Preservation &preservation, const ReservedSizesFmem &reserved_before,
    const FrameResourceSet &stripped, const FrameResourceSet &relocating);
  bool isClassDefragmentationDue(HeapClass heap_class, HeapGroupInfo &info, DefragmentationState &state, uint32_t frame,
    bool after_node_drop, bool &ghosts_expired);
  void dumpLayout(const SchedulingContext &ctx) const;
  void logHeapStatistics(const Timeline &timeline);

  Layout layout{};
  dag::VectorMap<intermediate::ResourceIndex, ResourceAllocationProperties> leftNatives;
  bool onChipRoomFreed = false;
  dag::VectorMap<ResourceHeapGroup *, HeapGroupInfo> groups;
  ResourceProperties properties;
  PlacementsChanged changedPlacements;
  IdIndexedMapping<intermediate::ResourceIndex, ResourceDescription::HashT> descriptionHashes;
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint64_t>, SCHEDULE_FRAME_WINDOW> correctedSizes;
  IdIndexedMapping<intermediate::ResourceIndex, SlotStats> slots;
  dag::Vector<Ghost> ghosts;
  IdIndexedMapping<intermediate::ResourceIndex, uint32_t> keepFrames;
  eastl::array<IdIndexedFlags<intermediate::ResourceIndex>, SCHEDULE_FRAME_WINDOW> preservedResources;
  IdIndexedFlags<intermediate::ResourceIndex> relocatedResources;
  bool layoutNoAliasing = false;
  bool historyInvalid = false;
  dag::Vector<intermediate::ResourceIndex> historyInvalidFor;
  uint32_t heapStatisticsLastFramePrinted = 0;
};

} // namespace dafg
