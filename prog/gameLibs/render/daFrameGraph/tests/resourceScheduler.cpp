// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "conVarGuard.h"
#include <memory/dag_framemem.h>

#include <backend/intermediateRepresentation.h>
#include <backend/resourceScheduling/resourceAllocator.h>
#include <backend/resourceScheduling/resourceScheduler.h>
#include <backend/resourceScheduling/resourceSchedulingTypes.h>
#include <backend/resourceScheduling/barrierScheduler.h>
#include <backend/resourceScheduling/resourcePropertyProvider.h>
#include <render/daFrameGraph/history.h>
#include <render/daFrameGraph/usage.h>
#include <render/daFrameGraph/stage.h>
#include <render/daFrameGraph/detail/access.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_texFlags.h>
#include <util/dag_convar.h>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/catch_test_macros.hpp>


extern ConVarT<bool, false> fill_dedicated_fast_gpu_local;
extern ConVarT<bool, false> validate_layout;
extern ConVarT<bool, false> no_aliasing;
extern ConVarT<float, true> defragment_placed_anew_fraction;
extern ConVarT<int, true> defragment_min_waste_mb;
extern ConVarT<int, true> defragment_max_wait_frames;
extern ConVarT<int, true> relocation_max_mb;

namespace
{

// Fake heap groups -- just need to be stable non-null pointers
static int FAKE_GPU_HEAP_STORAGE;
static int FAKE_GPU_HEAP_2_STORAGE;
static int FAKE_GPU_HEAP_3_STORAGE;
static int FAKE_ON_CHIP_HEAP_STORAGE;
static ResourceHeapGroup *const FAKE_GPU_HEAP = reinterpret_cast<ResourceHeapGroup *>(&FAKE_GPU_HEAP_STORAGE);
static ResourceHeapGroup *const FAKE_GPU_HEAP_2 = reinterpret_cast<ResourceHeapGroup *>(&FAKE_GPU_HEAP_2_STORAGE);
static ResourceHeapGroup *const FAKE_GPU_HEAP_3 = reinterpret_cast<ResourceHeapGroup *>(&FAKE_GPU_HEAP_3_STORAGE);
// Stands for ESRAM: small, and resources only land in it when asked to
static ResourceHeapGroup *const FAKE_ON_CHIP_HEAP = reinterpret_cast<ResourceHeapGroup *>(&FAKE_ON_CHIP_HEAP_STORAGE);


struct MockPropertyProvider : dafg::IResourcePropertyProvider
{
  size_t defaultSize = 1024;
  size_t defaultAlignment = 256;
  uint64_t maxHeapSize = 64u * 1024u * 1024u;
  uint64_t onChipMaxHeapSize = 4096;

  // Per-resource overrides keyed by ResourceDescription::asBasicRes.cFlags
  // (each test resource gets a unique cFlags via nextCFlags in the fixture)
  dag::VectorMap<uint32_t, size_t> sizeOverrides;
  dag::VectorMap<uint32_t, ResourceHeapGroup *> heapGroupOverrides;

  // Resources in this set are treated as vacant (not scheduled).
  dag::VectorSet<uint32_t> vacantCFlags;

  uint32_t allocationPropertyQueries = 0;
  bool keepsContents = false;
  bool countsObjects = false;

  bool doHeapsCountObjects() const override { return countsObjects; }
  bool doesResizeKeepContents() const override { return keepsContents; }
  uint64_t freeBudget = dafg::UNKNOWN_MEMORY_BUDGET;
  uint64_t getFreeMemoryBudget() const override { return freeBudget; }

  ResourceAllocationProperties getResourceAllocationProperties(const ResourceDescription &desc, bool force_not_on_chip) override
  {
    ++allocationPropertyQueries;
    if (vacantCFlags.count(desc.asBasicRes.cFlags))
      return {0, 0, nullptr};
    size_t size = defaultSize;
    auto sIt = sizeOverrides.find(desc.asBasicRes.cFlags);
    if (sIt != sizeOverrides.end())
      size = sIt->second;
    ResourceHeapGroup *group = FAKE_GPU_HEAP;
    auto gIt = heapGroupOverrides.find(desc.asBasicRes.cFlags);
    if (gIt != heapGroupOverrides.end() && !(force_not_on_chip && gIt->second == FAKE_ON_CHIP_HEAP))
      group = gIt->second;
    return {size, defaultAlignment, group};
  }

  ResourceHeapGroupProperties getResourceHeapGroupProperties(ResourceHeapGroup *group) override
  {
    ResourceHeapGroupProperties props{};
    props.flags = 0;
    props.isGPULocal = true;
    props.optimalMaxHeapSize = 0;
    props.isDedicatedFastGPULocal = group == FAKE_ON_CHIP_HEAP;
    props.maxHeapSize = props.isDedicatedFastGPULocal ? onChipMaxHeapSize : maxHeapSize;
    return props;
  }
};

static ResourceDescription make_tex_desc(uint32_t cflags)
{
  TextureResourceDescription tex{};
  tex.cFlags = cflags;
  tex.activation = ResourceActivationAction::DISCARD_AS_RTV_DSV;
  tex.clearValue = {};
  tex.mipLevels = 1;
  tex.width = 4;
  tex.height = 4;
  return ResourceDescription{tex};
}

struct Schedule
{
  decltype(dafg::LocationLookup::locationOf) allocationLocations;
  dafg::HeapRequests heapRequests;
};

struct Placements
{
  dag::Vector<dafg::FrameResource> placed;
  dag::Vector<dafg::FrameResource> removed;
};

struct ResourceSchedulerFixture
{
  dafg::intermediate::Graph graph;
  dafg::ResourceLifetimes lifetimes;
  MockPropertyProvider propertyProvider;
  dafg::ResourceScheduler scheduler;

  // Each resource gets a unique cFlags so the mock can apply per-resource size overrides.
  // Starting from 1 to avoid collisions with TEXFMT_A8R8G8B8 (= 0).
  uint32_t nextCFlags = 1;

  dafg::intermediate::ResourceIndex addGpuResource(dafg::History history = dafg::History::No, uint32_t extra_cflags = 0)
  {
    auto desc = make_tex_desc(nextCFlags++ | extra_cflags);

    dafg::intermediate::ScheduledResource scheduled;
    scheduled.description = desc;
    scheduled.resourceType = dafg::ResourceType::Texture;
    scheduled.resolutionType = eastl::nullopt;
    scheduled.clearStage = dafg::intermediate::ClearStage::None;
    scheduled.clearValue = ResourceClearValue{};
    scheduled.clearFlags = RESOURCE_CLEAR_ALL_CONTENT;
    scheduled.history = history;
    scheduled.autoMipCount = false;

    auto [idx, res] = graph.resources.appendNew();
    res.resource = eastl::move(scheduled);
    res.frontendResources.push_back(static_cast<dafg::ResNameId>(eastl::to_underlying(idx)));
    res.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    graph.resourceNames.emplaceAt(idx, "test_resource");
    return idx;
  }

  // Returns the cFlags assigned to a resource (for use as sizeOverrides key)
  uint32_t cFlagsOf(dafg::intermediate::ResourceIndex idx)
  {
    return graph.resources[idx].asScheduled().getGpuDescription().asBasicRes.cFlags;
  }

  // Changes the texture extent without touching cFlags, so the mock keeps
  // reporting the same allocation size. This is what a resolution change looks
  // like whenever both extents pad to the same gAPI tile-aligned image size.
  void resizeResource(dafg::intermediate::ResourceIndex idx, uint32_t width, uint32_t height)
  {
    auto &desc = graph.resources[idx].asScheduled().getGpuDescription();
    desc.asTexRes.width = width;
    desc.asTexRes.height = height;
  }

  dafg::intermediate::NodeIndex addNode()
  {
    auto [idx, node] = graph.nodes.appendNew();
    node.priority = 0;
    node.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    node.hasSideEffects = true;
    graph.nodeNames.emplaceAt(idx, "test_node");
    return idx;
  }

  void addUsage(dafg::intermediate::NodeIndex node, dafg::intermediate::ResourceIndex res, dafg::Usage usage,
    dafg::Access access = dafg::Access::READ_ONLY)
  {
    dafg::intermediate::Request request;
    request.resource = res;
    request.usage = {usage, access, dafg::Stage::POST_RASTER};
    request.fromLastFrame = false;
    graph.nodes[node].resourceRequests.push_back(request);
  }

  void ensureLifetimesSize()
  {
    for (auto &frameLifetimes : lifetimes)
      frameLifetimes.resize(graph.resources.totalKeys());
  }

  void addLifetime(dafg::intermediate::ResourceIndex res, dafg::intermediate::NodeIndex first_use_at,
    dafg::intermediate::NodeIndex release_at)
  {
    ensureLifetimesSize();
    for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
      lifetimes[frame][res] = {{frame, eastl::to_underlying(first_use_at)}, {frame, eastl::to_underlying(release_at)}};
  }

  // Gives a temporal resource a lifetime that starts in one frame and ends in
  // the next. This creates wrapping lifetimes, which is what temporal resources
  // have in practice (produced in one frame, consumed as history in the next).
  void addTemporalLifetime(dafg::intermediate::ResourceIndex res, dafg::intermediate::NodeIndex first_use_at,
    dafg::intermediate::NodeIndex release_at)
  {
    ensureLifetimesSize();
    for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    {
      const uint32_t otherFrame = (frame + 1) % dafg::SCHEDULE_FRAME_WINDOW;
      lifetimes[frame][res] = {{frame, eastl::to_underlying(first_use_at)}, {otherFrame, eastl::to_underlying(release_at)}};
    }
  }

  dafg::intermediate::ResourceIndex addCpuResource(size_t size = 1024, size_t alignment = 16)
  {
    dafg::intermediate::BlobDescription blob;
    blob.typeTag = static_cast<dafg::ResourceSubtypeTag>(nextCFlags++);
    blob.size = size;
    blob.alignment = alignment;

    dafg::intermediate::ScheduledResource scheduled;
    scheduled.description = blob;
    scheduled.resourceType = dafg::ResourceType::Blob;
    scheduled.resolutionType = eastl::nullopt;
    scheduled.clearStage = dafg::intermediate::ClearStage::None;
    scheduled.history = dafg::History::No;

    auto [idx, res] = graph.resources.appendNew();
    res.resource = eastl::move(scheduled);
    res.frontendResources.push_back(static_cast<dafg::ResNameId>(eastl::to_underlying(idx)));
    res.multiplexingIndex = dafg::intermediate::MultiplexingIndex{0};
    graph.resourceNames.emplaceAt(idx, "test_blob");
    return idx;
  }

  dag::VectorSet<dafg::intermediate::ResourceIndex> newResources;

  dag::Vector<dafg::intermediate::ResourceIndex> pairedWith;

  // With allValuesChanged off, or all_lifetimes_changed false, only these get a flag
  dag::VectorSet<dafg::intermediate::ResourceIndex> changedValues;
  dag::VectorSet<dafg::intermediate::ResourceIndex> changedLifetimes;
  eastl::array<dag::VectorMap<dafg::intermediate::ResourceIndex, size_t>, dafg::SCHEDULE_FRAME_WINDOW> corrections;

  bool allValuesChanged = true;
  uint32_t frame = 0;
  bool lightFrame = true;

  Schedule computeSchedule(int prev_frame = 0, bool all_lifetimes_changed = true)
  {
    FRAMEMEM_REGION;

    ensureLifetimesSize();

    const auto resCount = graph.resources.totalKeys();

    dafg::ResourceLifetimeCalculator::LifetimesChanged lifetimeChanged(resCount, all_lifetimes_changed);
    dafg::ResourceScheduler::ResourcesChangedFmem resourcesChanged(resCount, allValuesChanged);
    for (const auto idx : changedValues)
      resourcesChanged.set(idx, true);
    for (const auto idx : changedLifetimes)
      lifetimeChanged.set(idx, true);
    dafg::intermediate::HistoryPairing historyPairing;
    historyPairing.resize(resCount, dafg::intermediate::RESOURCE_NOT_MAPPED);
    for (uint32_t i = 0; i < resCount; ++i)
    {
      auto idx = static_cast<dafg::intermediate::ResourceIndex>(i);
      if (graph.resources.isMapped(idx) && newResources.find(idx) == newResources.end())
        historyPairing[idx] = idx;
    }
    for (size_t i = 0; i < pairedWith.size() && i < resCount; ++i)
      historyPairing[static_cast<dafg::intermediate::ResourceIndex>(i)] = pairedWith[i];

    dafg::BadResolutionTracker::Corrections ctxCorrections;
    for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    {
      ctxCorrections[frame].resize(resCount, 0);
      for (const auto &[idx, size] : corrections[frame])
        ctxCorrections[frame][idx] = size;
    }

    dafg::ResourceScheduler::SchedulingContext ctx{
      .graph = graph,
      .lifetimes = lifetimes,
      .resourcesChanged = resourcesChanged,
      .lifetimesChanged = lifetimeChanged,
      .historyPairing = historyPairing,
      .corrections = ctxCorrections,
      .propertyProvider = propertyProvider,
      .frameIndex = frame,
      .lightFrame = lightFrame,
    };

    scheduler.isDefragmentationDue(frame, lightFrame);
    scheduler.computeSchedule(prev_frame, ctx);

    const auto &layout = scheduler.currentLayout();
    Schedule result{layout.where.locationOf, {}};
    for (const auto &heap : layout.heaps)
      result.heapRequests.appendNew(dafg::make_heap_request(heap));
    return result;
  }

  Placements placements() const
  {
    Placements result;
    for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
      for (const auto idx : scheduler.placementsChanged()[frame].trueKeys())
      {
        const dafg::FrameResource what{
          .resIdx = idx,
          .frame = frame,
        };
        (scheduler.currentLayout().has(what) ? result.placed : result.removed).push_back(what);
      }
    return result;
  }
};

static eastl::optional<dafg::HeapIndex> find_heap_of_group(Schedule &schedule, ResourceHeapGroup *group)
{
  for (auto &&[idx, req] : schedule.heapRequests.enumerate())
    if (req.group == group)
      return idx;
  return eastl::nullopt;
}

static bool is_in_on_chip_heap(Schedule &schedule, dafg::intermediate::ResourceIndex res, uint32_t frame)
{
  const auto onChipHeap = find_heap_of_group(schedule, FAKE_ON_CHIP_HEAP);
  return onChipHeap.has_value() && schedule.allocationLocations[frame].isMapped(res) &&
         schedule.allocationLocations[frame][res].heap == *onChipHeap;
}

struct MockHeapAllocator
{
  dag::Vector<dafg::HeapRequest> heaps;
  uint32_t recreated = 0;

  void follow(const Schedule &schedule)
  {
    for (auto &&[idx, req] : schedule.heapRequests.enumerate())
    {
      const size_t i = eastl::to_underlying(idx);
      if (i >= heaps.size())
        heaps.resize(i + 1, dafg::HeapRequest{});
      else if (heaps[i].size != 0 && (heaps[i].size != req.size || heaps[i].group != req.group))
        ++recreated;
      heaps[i] = req;
    }
  }
};

// Without a native there is no on-chip heap request at all and nothing can be promoted into it
static dafg::intermediate::ResourceIndex add_on_chip_resource(ResourceSchedulerFixture &f)
{
  auto res = f.addGpuResource();
  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_ON_CHIP_HEAP;
  return res;
}

} // namespace


TEST_CASE("single resource gets allocated", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto res = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(res, node0, node1);

  auto schedule = f.computeSchedule();

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    REQUIRE(schedule.allocationLocations[frame].isMapped(res));
    CHECK(schedule.allocationLocations[frame][res].heap != dafg::HeapIndex::Invalid);
  }

  bool hasNonZeroHeap = false;
  for (const auto &heap : schedule.heapRequests.values())
    if (heap.size > 0)
      hasNonZeroHeap = true;
  CHECK(hasNonZeroHeap);
}

TEST_CASE("non-overlapping resources alias", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();

  // A lives [node0, node1), B lives [node1, node2) -- non-overlapping
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node1, node2);

  auto schedule = f.computeSchedule();

  REQUIRE(schedule.allocationLocations[0].isMapped(resA));
  REQUIRE(schedule.allocationLocations[0].isMapped(resB));

  auto &locA = schedule.allocationLocations[0][resA];
  auto &locB = schedule.allocationLocations[0][resB];

  // They should be in the same heap
  CHECK(locA.heap == locB.heap);

  // With non-overlapping lifetimes and equal sizes, they alias perfectly
  for (const auto &heap : schedule.heapRequests.values())
    if (heap.size > 0 && heap.group == FAKE_GPU_HEAP)
      CHECK(heap.size < 2 * f.propertyProvider.defaultSize);
}

TEST_CASE("overlapping resources don't alias", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  auto node3 = f.addNode();

  // A lives [node0, node2), B lives [node1, node3) -- overlapping
  f.addLifetime(resA, node0, node2);
  f.addLifetime(resB, node1, node3);

  auto schedule = f.computeSchedule();

  REQUIRE(schedule.allocationLocations[0].isMapped(resA));
  REQUIRE(schedule.allocationLocations[0].isMapped(resB));

  uint64_t totalGpuHeapSize = 0;
  for (const auto &heap : schedule.heapRequests.values())
    if (heap.size > 0 && heap.group == FAKE_GPU_HEAP)
      totalGpuHeapSize += heap.size;

  CHECK(totalGpuHeapSize >= 2 * f.propertyProvider.defaultSize);
}

TEST_CASE("rebuild produces same schedule", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto res = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(res, node0, node1);

  auto schedule1 = f.computeSchedule(0);
  auto schedule2 = f.computeSchedule(1);

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    REQUIRE(schedule1.allocationLocations[frame].isMapped(res));
    REQUIRE(schedule2.allocationLocations[frame].isMapped(res));
    CHECK(schedule1.allocationLocations[frame][res].heap == schedule2.allocationLocations[frame][res].heap);
    CHECK(schedule1.allocationLocations[frame][res].offset == schedule2.allocationLocations[frame][res].offset);
  }
}

TEST_CASE("a recompilation with no changes queries no properties and keeps the places", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node1, node2);

  auto schedule1 = f.computeSchedule(0);
  const uint32_t queriesAfterFirst = f.propertyProvider.allocationPropertyQueries;
  REQUIRE(queriesAfterFirst > 0);

  f.allValuesChanged = false;
  auto schedule2 = f.computeSchedule(1, false);

  CHECK(f.propertyProvider.allocationPropertyQueries == queriesAfterFirst);
  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    for (const auto res : {resA, resB})
    {
      REQUIRE(schedule1.allocationLocations[frame].isMapped(res));
      REQUIRE(schedule2.allocationLocations[frame].isMapped(res));
      CHECK(schedule2.allocationLocations[frame][res].heap == schedule1.allocationLocations[frame][res].heap);
      CHECK(schedule2.allocationLocations[frame][res].offset == schedule1.allocationLocations[frame][res].offset);
    }
}

TEST_CASE("temporal resource preservation", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  // Resource A is temporal (history), B and C are not
  auto resA = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  auto node3 = f.addNode();

  // resA is temporal: wrapping lifetime (activated in one frame, deactivated in next)
  f.addTemporalLifetime(resA, node1, node1);
  // B and C are non-temporal with overlapping lifetimes in each frame
  f.addLifetime(resB, node0, node2);
  f.addLifetime(resC, node1, node3);

  // First schedule on frame 0
  auto schedule1 = f.computeSchedule(0);

  REQUIRE(schedule1.allocationLocations[1].isMapped(resA));

  f.propertyProvider.sizeOverrides[f.cFlagsOf(resB)] = 2048;
  f.propertyProvider.sizeOverrides[f.cFlagsOf(resC)] = 512;

  auto schedule2 = f.computeSchedule(1);

  REQUIRE(schedule2.allocationLocations[1].isMapped(resA));
  CHECK(schedule2.allocationLocations[1][resA].offset == schedule1.allocationLocations[1][resA].offset);
}

TEST_CASE("a heap with no changed resources keeps its size", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto res = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(res, node0, node1);

  // First schedule -- everything is new
  auto schedule1 = f.computeSchedule(0);

  uint64_t heapSize1 = 0;
  for (const auto &heap : schedule1.heapRequests.values())
    if (heap.size > 0 && heap.group == FAKE_GPU_HEAP)
      heapSize1 = heap.size;
  REQUIRE(heapSize1 > 0);

  auto schedule2 = f.computeSchedule(1, false);

  uint64_t heapSize2 = 0;
  for (const auto &heap : schedule2.heapRequests.values())
    if (heap.group == FAKE_GPU_HEAP)
      heapSize2 = heap.size;
  CHECK(heapSize2 == heapSize1);
}

TEST_CASE("temporal resource with frame resources in different heaps", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 768;
  f.propertyProvider.defaultAlignment = 256;
  // resT(768) + resX(256) = 1024 fits, but resT(768) + resT(768) = 1536 doesn't
  f.propertyProvider.maxHeapSize = 1024;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  f.propertyProvider.sizeOverrides[f.nextCFlags] = 256;
  auto resX = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addNode(); // timeline length
  f.addNode(); // timeline length

  f.addTemporalLifetime(resT, node0, node1);
  f.addLifetime(resX, node0, node1);

  auto schedule1 = f.computeSchedule(0);

  REQUIRE(schedule1.allocationLocations[0][resT].heap != schedule1.allocationLocations[1][resT].heap);

  f.propertyProvider.sizeOverrides[f.cFlagsOf(resX)] = 128;

  auto schedule2 = f.computeSchedule(1, false);

  // All allocations must be within heap bounds
  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto res : {resT, resX})
    {
      auto [heap, offset] = schedule2.allocationLocations[frame][res];
      size_t resSize = (res == resX) ? 128 : f.propertyProvider.defaultSize;
      CHECK(offset + resSize <= schedule2.heapRequests[heap].size);
    }
}

TEST_CASE("a frame resource whose heap group changed moves to a heap of the new class", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();

  // Initially: A in group 1, B in group 2 -- one heap per group
  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(resA)] = FAKE_GPU_HEAP;
  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(resB)] = FAKE_GPU_HEAP_2;

  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  auto schedule1 = f.computeSchedule(0);

  REQUIRE(schedule1.allocationLocations[0].isMapped(resA));
  REQUIRE(schedule1.allocationLocations[0].isMapped(resB));
  const auto heapA1 = schedule1.allocationLocations[0][resA].heap;
  const auto heapB1 = schedule1.allocationLocations[0][resB].heap;
  REQUIRE(heapA1 != heapB1);
  REQUIRE(schedule1.heapRequests[heapA1].group == FAKE_GPU_HEAP);
  REQUIRE(schedule1.heapRequests[heapB1].group == FAKE_GPU_HEAP_2);

  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(resB)] = FAKE_GPU_HEAP;

  auto schedule2 = f.computeSchedule(1, false);

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    REQUIRE(schedule2.allocationLocations[0].isMapped(resB));
    const auto [heapB2, offsetB2] = schedule2.allocationLocations[frame][resB];
    // B must end up in a heap of its new group
    CHECK(schedule2.heapRequests[heapB2].group == FAKE_GPU_HEAP);
    // B's allocation must fit inside its heap
    CHECK(offsetB2 + f.propertyProvider.defaultSize <= schedule2.heapRequests[heapB2].size);
  }
}

TEST_CASE("a history frame resource without a pairing is not preserved", "[resourceScheduler]")
{
  // Its memory holds data of an unrelated resource, so reporting it as
  // preserved would skip its activation.
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resA, node0, node1);

  auto schedule1 = f.computeSchedule(0);

  // Simulate a frontend rename: the next schedule sees resA but with no
  // matching old-side history pair (historyPairing[resA] = NOT_MAPPED).
  f.newResources.insert(resA);

  auto schedule2 = f.computeSchedule(1, false);

  // Without a valid history pair, the scheduler must not mark resA as
  // preserved -- otherwise the runtime would skip its activation and
  // expose stale heap memory to its consumers.
  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK_FALSE(f.scheduler.isResourcePreserved(frame, resA));
}

// A new description with the same size gets a different object at the same
// offset, which the runtime must activate.

TEST_CASE("a history frame resource alone in its heap whose description changed is not preserved", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource(dafg::History::ClearZeroOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resA, node0, node1);

  auto schedule1 = f.computeSchedule(0);

  f.resizeResource(resA, 8, 8);

  auto schedule2 = f.computeSchedule(1, false);

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK_FALSE(f.scheduler.isResourcePreserved(frame, resA));
}

TEST_CASE("a history frame resource whose description changed next to a changed frame resource is not preserved",
  "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource(dafg::History::ClearZeroOnFirstFrame);
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  auto schedule1 = f.computeSchedule(0);
  REQUIRE(schedule1.allocationLocations[0][resA].heap == schedule1.allocationLocations[0][resB].heap);

  f.propertyProvider.sizeOverrides[f.cFlagsOf(resB)] = 2048;
  f.resizeResource(resA, 8, 8);

  auto schedule2 = f.computeSchedule(1, false);

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK_FALSE(f.scheduler.isResourcePreserved(frame, resA));
}

TEST_CASE("schedule is self-consistent after back-to-back recompilations", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultAlignment = 256;
  // Tight heaps to force overflow and multi-heap layouts.
  f.propertyProvider.maxHeapSize = 4096;

  constexpr int NUM_TEMPORAL = 10;
  constexpr int NUM_NONTEMPORAL = 20;
  constexpr int NUM_RESOURCES = NUM_TEMPORAL + NUM_NONTEMPORAL;
  dafg::intermediate::ResourceIndex resources[NUM_RESOURCES];
  for (int i = 0; i < NUM_TEMPORAL; ++i)
    resources[i] = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  for (int i = NUM_TEMPORAL; i < NUM_RESOURCES; ++i)
    resources[i] = f.addGpuResource();

  constexpr int NUM_NODES = 8;
  dafg::intermediate::NodeIndex nodes[NUM_NODES];
  for (int i = 0; i < NUM_NODES; ++i)
    nodes[i] = f.addNode();

  for (int i = 0; i < NUM_TEMPORAL; ++i)
  {
    f.propertyProvider.sizeOverrides[f.cFlagsOf(resources[i])] = 256 + (i % 4) * 256;
    f.addTemporalLifetime(resources[i], nodes[i % NUM_NODES], nodes[(i + 2) % NUM_NODES]);
  }
  for (int i = NUM_TEMPORAL; i < NUM_RESOURCES; ++i)
  {
    f.propertyProvider.sizeOverrides[f.cFlagsOf(resources[i])] = 128 + (i % 5) * 128;
    auto start = nodes[i % NUM_NODES];
    auto end = nodes[(i + 1 + i % 3) % NUM_NODES];
    if (eastl::to_underlying(start) >= eastl::to_underlying(end))
      end = nodes[(eastl::to_underlying(start) + 1) % NUM_NODES];
    f.addLifetime(resources[i], start, end);
  }

  f.computeSchedule(0);

  for (int iter = 0; iter < 30; ++iter)
  {
    // Remove ~40% of resources each iteration
    for (int i = 0; i < NUM_RESOURCES; ++i)
      if (((iter * 7 + i * 13) % 5) < 2)
        f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resources[i]));

    f.computeSchedule(iter % 2);

    f.propertyProvider.vacantCFlags.clear();

    auto restored = f.computeSchedule((iter + 1) % 2);

    for (int i = 0; i < NUM_RESOURCES; ++i)
    {
      for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
      {
        REQUIRE(restored.allocationLocations[frame].isMapped(resources[i]));
        auto [heap, offset] = restored.allocationLocations[frame][resources[i]];
        REQUIRE(restored.heapRequests.isMapped(heap));
        auto sIt = f.propertyProvider.sizeOverrides.find(f.cFlagsOf(resources[i]));
        auto resSize = sIt != f.propertyProvider.sizeOverrides.end() ? sIt->second : f.propertyProvider.defaultSize;
        CHECK(offset + resSize <= restored.heapRequests[heap].size);
      }
    }
  }
}

TEST_CASE("a frame resource evicted by a fixed neighbour that grew is placed once", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  const auto fixed = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  const auto neighbour = f.addGpuResource();
  f.propertyProvider.sizeOverrides[f.cFlagsOf(fixed)] = 32768;
  f.propertyProvider.sizeOverrides[f.cFlagsOf(neighbour)] = 32768;
  dafg::intermediate::ResourceIndex filler[12];
  for (auto &res : filler)
  {
    res = f.addGpuResource();
    f.propertyProvider.sizeOverrides[f.cFlagsOf(res)] = 256;
  }
  dafg::intermediate::NodeIndex nodes[8];
  for (auto &node : nodes)
    node = f.addNode();

  f.addTemporalLifetime(fixed, nodes[6], nodes[2]);
  f.addLifetime(neighbour, nodes[3], nodes[5]);
  for (int i = 0; i < 12; ++i)
    f.addLifetime(filler[i], nodes[i % 7], nodes[i % 7 + 1]);
  const auto first = f.computeSchedule(0);
  REQUIRE(first.allocationLocations[0][fixed].heap == first.allocationLocations[0][neighbour].heap);
  REQUIRE(first.allocationLocations[0][fixed].offset == first.allocationLocations[0][neighbour].offset);

  f.addTemporalLifetime(fixed, nodes[4], nodes[2]);
  const auto second = f.computeSchedule(1);
  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    REQUIRE(second.allocationLocations[frame].isMapped(fixed));
    REQUIRE(second.allocationLocations[frame].isMapped(neighbour));
    const auto fixedAt = second.allocationLocations[frame][fixed];
    const auto neighbourAt = second.allocationLocations[frame][neighbour];
    CHECK((fixedAt.heap != neighbourAt.heap || fixedAt.offset != neighbourAt.offset));
  }
  CHECK(second.allocationLocations[0][fixed].offset == first.allocationLocations[0][fixed].offset);
}

TEST_CASE("temporal resource on overflow heap is preserved across frames", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 768;
  f.propertyProvider.defaultAlignment = 256;
  // T(768) fits alongside X(256), but T(768)+T(768) = 1536 > 1024
  f.propertyProvider.maxHeapSize = 1024;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  f.propertyProvider.sizeOverrides[f.nextCFlags] = 256;
  auto resX = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addNode();
  f.addNode();

  f.addTemporalLifetime(resT, node0, node1);
  f.addLifetime(resX, node0, node1);

  auto s1 = f.computeSchedule(0);
  REQUIRE(s1.allocationLocations[0][resT].heap != s1.allocationLocations[1][resT].heap);

  auto s2 = f.computeSchedule(1);
  CHECK(f.scheduler.isResourcePreserved(1, resT));

  auto s3 = f.computeSchedule(0);
  CHECK(f.scheduler.isResourcePreserved(0, resT));
}

TEST_CASE("a resource that becomes temporal keeps the layout valid", "[resourceScheduler]")
{
  // Its new wrapping lifetime overlaps the frame resources it aliased
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 512;
  f.propertyProvider.defaultAlignment = 256;
  f.propertyProvider.maxHeapSize = 4096;

  auto resT1 = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resN = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  auto node3 = f.addNode();

  f.addTemporalLifetime(resT1, node0, node1);
  // N has a non-overlapping lifetime with T1 (they can alias at the same offset)
  f.addLifetime(resN, node2, node3);

  // Schedule 1: T1 and N placed on the same heap.
  // N may alias with T1 at the same offset due to non-overlapping lifetimes.
  auto s1 = f.computeSchedule(0);
  REQUIRE(s1.allocationLocations[0].isMapped(resT1));
  REQUIRE(s1.allocationLocations[0].isMapped(resN));

  f.graph.resources[resN].resource = f.graph.resources[resT1].resource;
  f.addTemporalLifetime(resN, node2, node3);

  // Schedule 2: both T1 and N are temporal. The scheduler must not crash.
  auto s2 = f.computeSchedule(1);
  // Basic sanity: both resources are allocated within heap bounds.
  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto res : {resT1, resN})
    {
      REQUIRE(s2.allocationLocations[frame].isMapped(res));
      auto [heap, offset] = s2.allocationLocations[frame][res];
      REQUIRE(s2.heapRequests.isMapped(heap));
      CHECK(offset + f.propertyProvider.defaultSize <= s2.heapRequests[heap].size);
    }
}

TEST_CASE("resources get promoted into the on-chip heap", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  auto onChip = add_on_chip_resource(f);
  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {onChip, resA, resB})
    f.addLifetime(res, node0, node1);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto res : {onChip, resA, resB})
      CHECK(is_in_on_chip_heap(schedule, res, frame));

  auto regularHeap = find_heap_of_group(schedule, FAKE_GPU_HEAP);
  if (regularHeap.has_value())
    CHECK(schedule.heapRequests[*regularHeap].size == 0);

  auto onChipHeap = find_heap_of_group(schedule, FAKE_ON_CHIP_HEAP);
  REQUIRE(onChipHeap.has_value());
  CHECK(schedule.heapRequests[*onChipHeap].size == f.propertyProvider.onChipMaxHeapSize);
}

TEST_CASE("resources that do not fit the on-chip heap stay where they were", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  // Room for the native resource plus exactly one promoted one
  f.propertyProvider.onChipMaxHeapSize = 2 * f.propertyProvider.defaultSize;

  auto onChip = add_on_chip_resource(f);
  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {onChip, resA, resB, resC})
    f.addLifetime(res, node0, node1);

  auto schedule = f.computeSchedule();

  uint32_t promotedCount = 0;
  for (auto res : {resA, resB, resC})
    if (is_in_on_chip_heap(schedule, res, 0))
    {
      ++promotedCount;
      // Which candidate wins is up to the scoring, but it wins on every frame
      CHECK(is_in_on_chip_heap(schedule, res, 1));
    }
  CHECK(promotedCount == 1);

  // The rejected ones did not get lost on the way: they are packed into the
  // regular heap, which is sized to hold exactly the two of them (their frame
  // resources have disjoint lifetimes and hence alias).
  auto regularHeap = find_heap_of_group(schedule, FAKE_GPU_HEAP);
  REQUIRE(regularHeap.has_value());
  CHECK(schedule.heapRequests[*regularHeap].size == 2 * f.propertyProvider.defaultSize);

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto res : {resA, resB, resC})
    {
      REQUIRE(schedule.allocationLocations[frame].isMapped(res));
      auto [heap, offset] = schedule.allocationLocations[frame][res];
      CHECK((heap == *regularHeap || is_in_on_chip_heap(schedule, res, frame)));
      CHECK(offset + f.propertyProvider.defaultSize <= schedule.heapRequests[heap].size);
    }
}

TEST_CASE("cpu accessible resources are not promoted", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  auto onChip = add_on_chip_resource(f);
  auto resGpu = f.addGpuResource();
  auto resCpu = f.addGpuResource(dafg::History::No, TEXCF_SYSMEM);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {onChip, resGpu, resCpu})
    f.addLifetime(res, node0, node1);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule, resGpu, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule, resCpu, frame));
  }
}

TEST_CASE("history resources are not promoted", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  auto onChip = add_on_chip_resource(f);
  auto resPlain = f.addGpuResource();
  auto resHistory = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(onChip, node0, node1);
  f.addLifetime(resPlain, node0, node1);
  f.addTemporalLifetime(resHistory, node0, node1);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule, resPlain, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule, resHistory, frame));
  }
}

TEST_CASE("debug resources are never promoted", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  auto onChip = add_on_chip_resource(f);
  auto resPlain = f.addGpuResource();
  auto resDebug = f.addGpuResource();
  f.graph.resources[resDebug].debugRelated = true;
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {onChip, resPlain, resDebug})
    f.addLifetime(res, node0, node1);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule, resPlain, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule, resDebug, frame));
  }
}

TEST_CASE("heavily used resources are promoted first", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  // Room for the native resource plus exactly one promoted one
  f.propertyProvider.onChipMaxHeapSize = 2 * f.propertyProvider.defaultSize;

  auto onChip = add_on_chip_resource(f);
  auto resHot = f.addGpuResource();
  auto resCold = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {onChip, resHot, resCold})
    f.addLifetime(res, node0, node1);

  // Same size, so the only thing telling them apart is bandwidth usage
  f.addUsage(node0, resHot, dafg::Usage::COLOR_ATTACHMENT);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule, resHot, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule, resCold, frame));
  }
}

TEST_CASE("nothing is promoted unless asked for", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, false);
  ResourceSchedulerFixture f;

  auto onChip = add_on_chip_resource(f);
  auto resA = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(onChip, node0, node1);
  f.addLifetime(resA, node0, node1);

  auto schedule = f.computeSchedule();

  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule, onChip, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule, resA, frame));
  }
}

TEST_CASE("a native on-chip resource takes the place of a promoted one", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;

  // Room for the native resource plus exactly one promoted one
  f.propertyProvider.onChipMaxHeapSize = 2 * f.propertyProvider.defaultSize;

  auto onChip = add_on_chip_resource(f);
  auto resA = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(onChip, node0, node1);
  f.addLifetime(resA, node0, node1);

  auto schedule1 = f.computeSchedule(0);
  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK(is_in_on_chip_heap(schedule1, resA, frame));

  auto onChip2 = add_on_chip_resource(f);
  f.addLifetime(onChip2, node0, node1);

  auto schedule2 = f.computeSchedule(1, false);
  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(is_in_on_chip_heap(schedule2, onChip, frame));
    CHECK(is_in_on_chip_heap(schedule2, onChip2, frame));
    CHECK_FALSE(is_in_on_chip_heap(schedule2, resA, frame));
    REQUIRE(schedule2.allocationLocations[frame].isMapped(resA));
    const auto [heap, offset] = schedule2.allocationLocations[frame][resA];
    CHECK(schedule2.heapRequests[heap].group == FAKE_GPU_HEAP);
  }
  const auto onChipHeap = find_heap_of_group(schedule2, FAKE_ON_CHIP_HEAP);
  REQUIRE(onChipHeap.has_value());
  CHECK(schedule2.heapRequests[*onChipHeap].size == f.propertyProvider.onChipMaxHeapSize);
}

TEST_CASE("an on-chip resource that does not fit its heap gets regular memory", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, true);
  ResourceSchedulerFixture f;
  f.propertyProvider.onChipMaxHeapSize = 2 * f.propertyProvider.defaultSize;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  dafg::intermediate::ResourceIndex natives[3];
  for (auto &res : natives)
  {
    res = add_on_chip_resource(f);
    f.addLifetime(res, node0, node1);
  }

  auto schedule = f.computeSchedule(0);

  uint32_t inOnChip = 0;
  for (auto res : natives)
  {
    REQUIRE(schedule.allocationLocations[0].isMapped(res));
    const auto [heap, offset] = schedule.allocationLocations[0][res];
    REQUIRE(schedule.heapRequests.isMapped(heap));
    CHECK(offset + f.propertyProvider.defaultSize <= schedule.heapRequests[heap].size);
    inOnChip += is_in_on_chip_heap(schedule, res, 0);
  }
  CHECK(inOnChip == 2);
  uint32_t onChipHeaps = 0;
  for (const auto &req : schedule.heapRequests)
    onChipHeaps += req.group == FAKE_ON_CHIP_HEAP;
  CHECK(onChipHeaps == 1);
}

TEST_CASE("a resource added again gets its old memory", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto resD = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {resA, resB, resC, resD})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto placeB = first.allocationLocations[0][resB];

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resB));
  f.computeSchedule(1);

  f.propertyProvider.vacantCFlags.clear();
  const auto third = f.computeSchedule(0);

  CHECK(third.allocationLocations[0][resB].heap == placeB.heap);
  CHECK(third.allocationLocations[0][resB].offset == placeB.offset);
}

TEST_CASE("the defragmentation triggers on the fraction of unplaced frame resources", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[0][resC].heap;
  REQUIRE(first.heapRequests[heap].size == 3 * 1024);

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  {
    const ConVarGuard guard(defragment_placed_anew_fraction, 1.f);
    const auto second = f.computeSchedule(1);
    CHECK(second.heapRequests[heap].size == 3 * 1024);
    CHECK(second.allocationLocations[0][resC].offset == first.allocationLocations[0][resC].offset);
  }

  // A defragmentation from the unplaced fraction keeps the size: only the poll returns memory
  {
    const ConVarGuard guard(defragment_placed_anew_fraction, 0.f);
    const auto third = f.computeSchedule(0);
    CHECK(third.heapRequests[heap].size == 3 * 1024);
    CHECK(dafg::measure_used_size(f.scheduler.currentLayout().heaps[heap]) == 2 * 1024);
  }
}

TEST_CASE("a history frame resource that does not live to the next frame is not preserved", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto resA = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);

  f.computeSchedule(0);
  f.computeSchedule(1, false);

  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK_FALSE(f.scheduler.isResourcePreserved(frame, resA));
}

TEST_CASE("a frame resource that only now lives to the next frame is not preserved", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);

  const auto first = f.computeSchedule(0);
  REQUIRE(first.allocationLocations[1].isMapped(resA));

  f.graph.resources[resA].asScheduled().history = dafg::History::ClearZeroOnFirstFrame;
  f.addTemporalLifetime(resA, node0, node1);

  const auto second = f.computeSchedule(1);
  CHECK(second.allocationLocations[1][resA].offset == first.allocationLocations[1][resA].offset);
  CHECK_FALSE(f.scheduler.isResourcePreserved(1, resA));

  f.allValuesChanged = false;
  f.computeSchedule(0, false);
  CHECK(f.scheduler.isResourcePreserved(0, resA));
}

TEST_CASE("a CPU heap that holds a preserved blob keeps its size", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  const auto history = f.addCpuResource();
  f.graph.resources[history].asScheduled().history = dafg::History::ClearZeroOnFirstFrame;
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  f.addTemporalLifetime(history, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto historyHeap = first.allocationLocations[1][history].heap;
  REQUIRE(historyHeap != dafg::HeapIndex::Invalid);
  const uint64_t heapSize = first.heapRequests[historyHeap].size;

  const auto bulk = f.addCpuResource(1 << 16);
  f.newResources.insert(bulk);
  f.addLifetime(bulk, node0, node2);

  const auto second = f.computeSchedule(1);
  CHECK(second.heapRequests[historyHeap].size == heapSize);
  CHECK(second.allocationLocations[1][history].heap == historyHeap);
  CHECK(second.allocationLocations[1][history].offset == first.allocationLocations[1][history].offset);
  CHECK(f.scheduler.isResourcePreserved(1, history));
  CHECK(second.allocationLocations[0][bulk].heap != historyHeap);
}

TEST_CASE("a wasteful heap with a preserved frame resource is defragmented by a relocation when the policy allows it",
  "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  const int relocationMb = GENERATE(64, 0);
  const ConVarGuard relocation(relocation_max_mb, relocationMb);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resT, node0, node1);
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heapT = first.allocationLocations[0][resT].heap;
  const uint64_t sizeBefore = first.heapRequests[heapT].size;
  REQUIRE(sizeBefore > 2 * 1024);

  for (auto res : {resA, resB, resC})
    f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(res));
  f.frame = 60;
  const auto second = f.computeSchedule(1);
  REQUIRE(second.heapRequests[heapT].size == sizeBefore);
  REQUIRE(f.scheduler.isResourcePreserved(1, resT));

  f.frame = 120;
  f.computeSchedule(0, false);

  f.frame = 440;
  const auto third = f.computeSchedule(0, false);
  CHECK(f.scheduler.isResourcePreserved(0, resT));
  if (relocationMb > 0)
  {
    CHECK(f.scheduler.relocations().test(resT, false));
    CHECK(third.heapRequests[heapT].size == 0);
    const auto newHeap = third.allocationLocations[0][resT].heap;
    CHECK(newHeap != heapT);
    CHECK(third.heapRequests[newHeap].size == 2 * 1024);
  }
  else
  {
    CHECK_FALSE(f.scheduler.relocations().test(resT, false));
    CHECK(third.heapRequests[heapT].size == sizeBefore);
    CHECK(third.allocationLocations[0][resT].heap == heapT);
    CHECK(third.allocationLocations[0][resT].offset == first.allocationLocations[0][resT].offset);
  }
}

TEST_CASE("a due defragmentation waits for a recompilation that removed nodes, or for the deadline", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  const ConVarGuard maxWait(defragment_max_wait_frames, 1000);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.lightFrame = false;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[0][resC].heap;
  REQUIRE(first.heapRequests[heap].size == 3 * 1024);

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);

  f.frame = 440;
  CHECK(f.computeSchedule(1, false).heapRequests[heap].size == 3 * 1024);
  f.frame = 500;
  CHECK(f.computeSchedule(0, false).heapRequests[heap].size == 3 * 1024);

  SECTION("a recompilation that removed nodes runs it")
  {
    f.lightFrame = true;
    f.frame = 560;
    CHECK(f.computeSchedule(1, false).heapRequests[heap].size == 2 * 1024);
  }

  SECTION("the deadline runs it")
  {
    f.frame = 1440;
    CHECK(f.computeSchedule(1, false).heapRequests[heap].size == 2 * 1024);
  }
}

TEST_CASE("a due defragmentation waits until the free memory budget holds the heaps it creates", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.freeBudget = 2 * 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[0][resC].heap;
  REQUIRE(first.heapRequests[heap].size == 3 * 1024);

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);

  f.frame = 440;
  CHECK(f.computeSchedule(1, false).heapRequests[heap].size == 3 * 1024);

  f.propertyProvider.freeBudget = 4 * 1024;
  uint64_t size = 0;
  for (uint32_t frame = 500; frame <= 800; frame += 60)
  {
    f.frame = frame;
    size = f.computeSchedule(static_cast<int>(frame / 60 % 2), false).heapRequests[heap].size;
  }
  CHECK(size == 2 * 1024);
}

TEST_CASE("a relocation that the free memory budget cannot hold is skipped", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  const ConVarGuard relocation(relocation_max_mb, 64);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.freeBudget = 2 * 1024;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resT, node0, node1);
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heapT = first.allocationLocations[0][resT].heap;
  const uint64_t sizeBefore = first.heapRequests[heapT].size;
  REQUIRE(sizeBefore > 2 * 1024);

  for (auto res : {resA, resB, resC})
    f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(res));
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);

  f.frame = 440;
  const auto third = f.computeSchedule(0, false);
  CHECK(f.scheduler.isResourcePreserved(0, resT));
  CHECK_FALSE(f.scheduler.relocations().test(resT, false));
  CHECK(third.heapRequests[heapT].size == sizeBefore);
  CHECK(third.allocationLocations[0][resT].heap == heapT);
}

TEST_CASE("a CPU heap class is defragmented like any other", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard guard(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;

  auto blobA = f.addCpuResource();
  auto blobB = f.addCpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  f.addLifetime(blobA, node0, node2);
  f.addLifetime(blobB, node0, node2);

  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[0][blobB].heap;
  REQUIRE(first.heapRequests[heap].size == 2 * 1024);
  const auto offsetOfB = first.allocationLocations[0][blobB].offset;
  REQUIRE(offsetOfB == 1024);

  // The two blobs no longer share a moment, so half of the heap is waste.
  f.addLifetime(blobA, node0, node1);
  f.addLifetime(blobB, node1, node2);
  f.allValuesChanged = false;
  f.frame = 60;
  f.computeSchedule(0, true);

  f.frame = 120;
  const auto soon = f.computeSchedule(0, false);
  CHECK(soon.heapRequests[heap].size == 2 * 1024);
  CHECK(soon.allocationLocations[0][blobB].offset == offsetOfB);

  f.frame = 440;
  const auto last = f.computeSchedule(0, false);
  CHECK(last.heapRequests[heap].size == 1024);
  CHECK(last.allocationLocations[0][blobB].offset == 0);
}

TEST_CASE("an empty class releases its heaps at once without retention", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(resA)] = FAKE_GPU_HEAP_2;
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heapA = first.allocationLocations[0][resA].heap;
  REQUIRE(first.heapRequests[heapA].size == 1024);
  REQUIRE(first.allocationLocations[0][resB].heap != heapA);

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  const auto second = f.computeSchedule(1);
  CHECK(second.heapRequests[heapA].size == 0);
}

TEST_CASE("a resource past the end of a graph that shrank leaves the layout", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto resC = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  for (auto res : {resA, resB, resC})
    f.addLifetime(res, node0, node1);

  const auto first = f.computeSchedule(0);
  REQUIRE(first.allocationLocations[0].isMapped(resC));

  f.graph.resources.resize(2);
  f.computeSchedule(1);
  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK_FALSE(f.scheduler.currentLayout().has({
      .resIdx = resC,
      .frame = frame,
    }));
  const dafg::Timeline timeline = dafg::make_timeline(f.graph);
  CHECK(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));
}

TEST_CASE("a new heap takes the slot of a heap that lost its memory", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  f.propertyProvider.heapGroupOverrides[f.cFlagsOf(resA)] = FAKE_GPU_HEAP_2;
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  const auto first = f.computeSchedule(0);
  const auto heapA = first.allocationLocations[0][resA].heap;
  REQUIRE(first.heapRequests[heapA].size == 1024);

  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  const auto second = f.computeSchedule(1);
  REQUIRE(second.heapRequests[heapA].size == 0);

  const auto blob = f.addCpuResource();
  f.newResources.insert(blob);
  f.addLifetime(blob, node0, node1);
  const auto third = f.computeSchedule(0);
  CHECK(third.allocationLocations[0][blob].heap == heapA);
  CHECK(third.heapRequests[heapA].group == dafg::CPU_HEAP_GROUP);
  CHECK(third.heapRequests.size() == first.heapRequests.size());
  const dafg::Timeline timeline = dafg::make_timeline(f.graph);
  CHECK(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));
}

TEST_CASE("a slot that came back before keeps its memory for twice its away time", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  const auto own = [&]() {
    auto res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_GPU_HEAP_2;
    f.newResources.insert(res);
    f.addLifetime(res, node0, node1);
    return res;
  };
  const auto leave = [&](dafg::intermediate::ResourceIndex res) {
    f.graph.resources.erase(res);
    f.newResources.erase(res);
  };

  MockHeapAllocator allocator;
  auto resA = own();
  const auto first = f.computeSchedule(0);
  allocator.follow(first);
  const auto heapA = first.allocationLocations[0][resA].heap;
  REQUIRE(first.heapRequests[heapA].size == 1024);

  leave(resA);
  f.frame = 10;
  allocator.follow(f.computeSchedule(1));
  CHECK(f.scheduler.currentLayout().heaps[heapA].reservedSize == 0);

  resA = own();
  REQUIRE(resA == static_cast<dafg::intermediate::ResourceIndex>(eastl::to_underlying(resB) + 1));
  f.frame = 50;
  const auto back = f.computeSchedule(0);
  allocator.follow(back);
  const auto heapAgain = back.allocationLocations[0][resA].heap;
  REQUIRE(back.heapRequests[heapAgain].size == 1024);
  allocator.recreated = 0;

  // The slot was away 40 frames, so its ghost holds the memory for 80
  leave(resA);
  f.frame = 100;
  allocator.follow(f.computeSchedule(1));
  CHECK(f.scheduler.currentLayout().heaps[heapAgain].reservedSize == 1024);
  f.frame = 150;
  allocator.follow(f.computeSchedule(0, false));
  CHECK(f.scheduler.currentLayout().heaps[heapAgain].reservedSize == 1024);

  resA = own();
  f.frame = 170;
  const auto third = f.computeSchedule(1);
  allocator.follow(third);
  CHECK(third.allocationLocations[0][resA].heap == heapAgain);
  CHECK(third.heapRequests[heapAgain].size == 1024);
  CHECK(allocator.recreated == 0);

  leave(resA);
  f.frame = 200;
  allocator.follow(f.computeSchedule(0));
  CHECK(f.scheduler.currentLayout().heaps[heapAgain].reservedSize == 1024);
  f.frame = 400;
  allocator.follow(f.computeSchedule(1, false));
  CHECK(f.scheduler.currentLayout().heaps[heapAgain].reservedSize == 0);
}

TEST_CASE("a slot refilled by another heap class starts its statistics again", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  const auto own = [&](bool other_class) {
    auto res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = other_class ? FAKE_GPU_HEAP_2 : FAKE_GPU_HEAP_3;
    f.newResources.insert(res);
    f.addLifetime(res, node0, node1);
    return res;
  };
  const auto leave = [&](dafg::intermediate::ResourceIndex res) {
    f.graph.resources.erase(res);
    f.newResources.erase(res);
  };
  const auto heapOf = [&](const Schedule &s, dafg::intermediate::ResourceIndex res) { return s.allocationLocations[0][res].heap; };

  MockHeapAllocator allocator;
  auto res = own(true);
  allocator.follow(f.computeSchedule(0));
  leave(res);
  f.frame = 10;
  allocator.follow(f.computeSchedule(1));
  res = own(true);
  f.frame = 50;
  const auto second = f.computeSchedule(0);
  allocator.follow(second);
  const auto ownHeap = heapOf(second, res);

  // The slot was refilled once, so its second departure leaves a ghost until frame 180
  leave(res);
  f.frame = 100;
  allocator.follow(f.computeSchedule(1));
  CHECK(f.scheduler.currentLayout().heaps[ownHeap].reservedSize == 1024);

  res = own(false);
  f.frame = 120;
  const auto other = f.computeSchedule(0);
  allocator.follow(other);
  const auto otherHeap = heapOf(other, res);
  REQUIRE(otherHeap != ownHeap);
  CHECK(f.scheduler.currentLayout().heaps[ownHeap].reservedSize == 1024);

  leave(res);
  f.frame = 150;
  allocator.follow(f.computeSchedule(1));
  CHECK(f.scheduler.currentLayout().heaps[otherHeap].reservedSize == 0);
  CHECK(f.scheduler.currentLayout().heaps[ownHeap].reservedSize == 1024);

  res = own(false);
  f.frame = 160;
  allocator.follow(f.computeSchedule(0));
  CHECK(f.scheduler.currentLayout().heaps[ownHeap].reservedSize == 1024);
  leave(res);
  f.frame = 170;
  allocator.follow(f.computeSchedule(1));
  CHECK(f.scheduler.currentLayout().heaps[otherHeap].reservedSize == 1024);
  CHECK(f.scheduler.isStateValid());

  f.frame = 400;
  allocator.follow(f.computeSchedule(0, false));
  CHECK(f.scheduler.currentLayout().heaps[ownHeap].reservedSize == 0);
  CHECK(f.scheduler.currentLayout().heaps[otherHeap].reservedSize == 0);
  CHECK(f.scheduler.isStateValid());
}

TEST_CASE("only the wasteful heap of a class is packed anew", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.maxHeapSize = 2048;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  dafg::intermediate::ResourceIndex res[4];
  for (auto &r : res)
  {
    r = f.addGpuResource();
    f.addLifetime(r, node0, node1);
  }
  const auto first = f.computeSchedule(0);
  const auto heapOf = [&](dafg::intermediate::ResourceIndex r) { return first.allocationLocations[0][r].heap; };
  REQUIRE((heapOf(res[0]) != heapOf(res[1]) || heapOf(res[0]) != heapOf(res[2]) || heapOf(res[0]) != heapOf(res[3])));

  dag::Vector<dafg::intermediate::ResourceIndex> kept;
  for (const auto r : res)
    if (heapOf(r) == heapOf(res[0]))
      f.graph.resources.erase(r);
    else
      kept.push_back(r);
  REQUIRE(kept.size() == 2);

  f.frame = 60;
  f.computeSchedule(1, false);
  f.frame = 120;
  f.computeSchedule(0, false);
  f.frame = 440;
  const auto after = f.computeSchedule(1, false);
  CHECK(after.heapRequests[heapOf(res[0])].size == 0);
  CHECK(after.heapRequests[heapOf(kept[0])].size == 2048);
  for (const auto r : kept)
    for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    {
      CHECK(after.allocationLocations[frame][r].heap == first.allocationLocations[frame][r].heap);
      CHECK(after.allocationLocations[frame][r].offset == first.allocationLocations[frame][r].offset);
      CHECK_FALSE(f.scheduler.placementsChanged()[frame].test(r, false));
    }
}

TEST_CASE("waste spread over every heap packs the whole class anew", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.maxHeapSize = 2048;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  dafg::intermediate::ResourceIndex res[4];
  for (auto &r : res)
  {
    r = f.addGpuResource();
    f.addLifetime(r, node0, node1);
  }
  const auto first = f.computeSchedule(0);
  const auto heapOf = [&](dafg::intermediate::ResourceIndex r) { return first.allocationLocations[0][r].heap; };

  dafg::HeapIndex seen[2] = {dafg::HeapIndex::Invalid, dafg::HeapIndex::Invalid};
  for (const auto r : res)
  {
    const auto heap = heapOf(r);
    if (heap == seen[0] || heap == seen[1])
      continue;
    (seen[0] == dafg::HeapIndex::Invalid ? seen[0] : seen[1]) = heap;
    f.graph.resources.erase(r);
  }
  REQUIRE(seen[1] != dafg::HeapIndex::Invalid);

  f.frame = 60;
  f.computeSchedule(1, false);
  f.frame = 120;
  f.computeSchedule(0, false);
  f.frame = 440;
  f.computeSchedule(1, false);
  const dafg::HeapClass regular{
    .group = FAKE_GPU_HEAP,
    .untracked = false,
  };
  uint64_t reserved = 0;
  uint32_t nonEmpty = 0;
  for (const auto heapIdx : f.scheduler.currentLayout().heapsOf(regular))
  {
    reserved += f.scheduler.currentLayout().heaps[heapIdx].reservedSize;
    nonEmpty += f.scheduler.currentLayout().heaps[heapIdx].reservedSize > 0 ? 1 : 0;
  }
  CHECK(reserved == 2048);
  CHECK(nonEmpty == 1);
}

TEST_CASE("a native that left the on-chip heap comes back once a rect of the heap is gone", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ConVarGuard guard(fill_dedicated_fast_gpu_local, false);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.onChipMaxHeapSize = 2048;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  dafg::intermediate::ResourceIndex natives[3];
  for (auto &r : natives)
  {
    r = add_on_chip_resource(f);
    f.addLifetime(r, node0, node1);
  }
  auto first = f.computeSchedule(0);
  dafg::intermediate::ResourceIndex left = dafg::intermediate::RESOURCE_NOT_MAPPED;
  dafg::intermediate::ResourceIndex onChip = dafg::intermediate::RESOURCE_NOT_MAPPED;
  for (const auto r : natives)
    (is_in_on_chip_heap(first, r, 0) ? onChip : left) = r;
  REQUIRE(left != dafg::intermediate::RESOURCE_NOT_MAPPED);
  REQUIRE(onChip != dafg::intermediate::RESOURCE_NOT_MAPPED);
  const auto regularHeap = find_heap_of_group(first, FAKE_GPU_HEAP);
  REQUIRE(regularHeap.has_value());
  CHECK(first.heapRequests[*regularHeap].size == 1024);

  f.graph.resources.erase(onChip);
  f.frame = 10;
  auto after = f.computeSchedule(1, false);
  for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
    CHECK(is_in_on_chip_heap(after, left, frame));
  CHECK(f.scheduler.currentLayout().heaps[*regularHeap].reservedSize == 0);
}

TEST_CASE("slots cycled by different subgraphs keep their memory", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  const auto own = [&]() {
    auto res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_GPU_HEAP_2;
    f.newResources.insert(res);
    f.addLifetime(res, node0, node1);
    return res;
  };

  MockHeapAllocator allocator;
  dafg::HeapIndex heap = dafg::HeapIndex::Invalid;
  uint32_t frame = 0;
  for (uint32_t cycle = 0; cycle < 10; ++cycle)
  {
    const auto res = own();
    frame += 10;
    f.frame = frame;
    const auto on = f.computeSchedule(frame % 2);
    allocator.follow(on);
    if (cycle == 0)
      heap = on.allocationLocations[0][res].heap;
    else
      CHECK(on.allocationLocations[0][res].heap == heap);
    CHECK(on.heapRequests[heap].size == 1024);

    f.graph.resources.erase(res);
    f.newResources.erase(res);
    frame += 10;
    f.frame = frame;
    allocator.follow(f.computeSchedule(frame % 2));
    CHECK(f.scheduler.currentLayout().heaps[heap].reservedSize == (cycle == 0 ? 0 : 1024));
    if (cycle == 0)
      allocator.recreated = 0;
  }
  CHECK(allocator.recreated == 0);
}

TEST_CASE("a subgraph that does not fit the heap of a history frame resource gets a new heap", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  const auto history = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  f.addTemporalLifetime(history, node0, node1);

  MockHeapAllocator allocator;
  const auto first = f.computeSchedule(0);
  allocator.follow(first);
  const auto historyHeap = first.allocationLocations[1][history].heap;
  REQUIRE(historyHeap != dafg::HeapIndex::Invalid);
  const uint64_t heapSize = first.heapRequests[historyHeap].size;

  dafg::intermediate::ResourceIndex added[4];
  for (auto &res : added)
  {
    res = f.addGpuResource();
    f.newResources.insert(res);
    f.addLifetime(res, node0, node2);
  }

  const auto second = f.computeSchedule(1);
  allocator.follow(second);

  CHECK(second.heapRequests[historyHeap].size == heapSize);
  CHECK(second.allocationLocations[1][history].heap == historyHeap);
  CHECK(second.allocationLocations[1][history].offset == first.allocationLocations[1][history].offset);
  CHECK(f.scheduler.isResourcePreserved(1, history));
  CHECK(allocator.recreated == 0);
  for (const auto res : added)
    CHECK(second.allocationLocations[0][res].heap != dafg::HeapIndex::Invalid);
}

TEST_CASE("a resource index that gets a new IR resource places both frame resources again", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  const auto res = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(res, node0, node1);

  f.computeSchedule(0);

  f.newResources.insert(res);
  f.computeSchedule(1);
  const auto changes = f.placements();

  CHECK(changes.removed.empty());
  REQUIRE(changes.placed.size() == dafg::SCHEDULE_FRAME_WINDOW);
  for (const auto what : changes.placed)
    CHECK(what.resIdx == res);
}

TEST_CASE("the invalidation of a history moves with its resource to a new index", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  const auto first = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  const auto second = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(first, node0, node1);
  f.addTemporalLifetime(second, node0, node1);

  f.computeSchedule(0);
  f.computeSchedule(1);
  REQUIRE(f.scheduler.isResourcePreserved(1, first));
  REQUIRE(f.scheduler.isResourcePreserved(1, second));

  f.scheduler.invalidateTemporalResource(first);
  const auto secondWasAt = f.scheduler.currentLayout().where[dafg::FrameResource{
    .resIdx = second,
    .frame = 1,
  }];

  eastl::swap(f.graph.resources[first], f.graph.resources[second]);
  eastl::swap(f.lifetimes[0][first], f.lifetimes[0][second]);
  eastl::swap(f.lifetimes[1][first], f.lifetimes[1][second]);
  f.pairedWith.push_back(second);
  f.pairedWith.push_back(first);

  f.computeSchedule(1);

  CHECK(f.scheduler.isResourcePreserved(1, first));
  CHECK_FALSE(f.scheduler.isResourcePreserved(1, second));
  const auto firstIsAt = f.scheduler.currentLayout().where[dafg::FrameResource{
    .resIdx = first,
    .frame = 1,
  }];
  CHECK(firstIsAt.heap == secondWasAt.heap);
  CHECK(firstIsAt.offset == secondWasAt.offset);
}

TEST_CASE("an armed decision of a class that lost its memory is cleared", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  const ConVarGuard maxWait(defragment_max_wait_frames, 1000);
  const ConVarGuard validate(validate_layout, true);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.lightFrame = false;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  dafg::intermediate::ResourceIndex own[2];
  for (auto &res : own)
  {
    res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_GPU_HEAP_2;
    f.addLifetime(res, node0, node1);
  }
  f.computeSchedule(0);

  f.graph.resources.erase(own[0]);
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);
  f.frame = 440;
  f.computeSchedule(1, false);
  f.graph.resources.erase(own[1]);
  f.frame = 460;
  f.computeSchedule(0);

  f.frame = 2000;
  CHECK_FALSE(f.scheduler.isDefragmentationDue(2000, false));
  CHECK_FALSE(f.scheduler.isDefragmentationDue(2001, false));
  CHECK(f.scheduler.isStateValid());
}

TEST_CASE("an armed decision is dropped when the class is tight again", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  const ConVarGuard maxWait(defragment_max_wait_frames, 1000);
  const ConVarGuard fraction(defragment_placed_anew_fraction, 1.f);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.lightFrame = false;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto base = f.addGpuResource();
  f.addLifetime(base, node0, node1);
  dafg::intermediate::ResourceIndex sub[3];
  for (auto &res : sub)
  {
    res = f.addGpuResource();
    f.addLifetime(res, node0, node1);
  }
  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[0][base].heap;
  const auto baseOffset = first.allocationLocations[0][base].offset;
  REQUIRE(first.heapRequests[heap].size == 4 * 1024);

  for (auto res : sub)
    f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(res));
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);
  f.frame = 440;
  f.computeSchedule(1, false);

  for (auto res : sub)
    f.propertyProvider.vacantCFlags.erase(f.cFlagsOf(res));
  f.frame = 500;
  f.computeSchedule(0);
  f.frame = 1500;
  const auto late = f.computeSchedule(1, false);
  CHECK(late.heapRequests[heap].size == 4 * 1024);
  CHECK(late.allocationLocations[0][base].offset == baseOffset);
  CHECK_FALSE(f.scheduler.placementsChanged()[0].test(base, false));
}

TEST_CASE("a heap grows by the byte step unless its class counts objects", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const bool countsObjects = GENERATE(false, true);
  const ConVarGuard placedAnew(defragment_placed_anew_fraction, 1.f);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.countsObjects = countsObjects;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resA = f.addGpuResource();
  f.addLifetime(resA, node0, node1);
  const auto first = f.computeSchedule();
  const auto heap = first.allocationLocations[0][resA].heap;
  const uint64_t before = first.heapRequests[heap].size;
  REQUIRE(before < (1 << 20));

  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  const auto second = f.computeSchedule();
  REQUIRE(second.allocationLocations[0][resB].heap == heap);
  const uint64_t after = second.heapRequests[heap].size;
  CHECK(after > before);
  CHECK((after >= (1 << 20)) == !countsObjects);
}

TEST_CASE("a heap with a kept rect shrinks where a resize keeps its contents", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.keepsContents = true;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto history = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  f.addTemporalLifetime(history, node0, node1);
  dafg::intermediate::ResourceIndex sub[3];
  for (auto &res : sub)
  {
    res = f.addGpuResource();
    f.addLifetime(res, node0, node1);
  }
  const auto first = f.computeSchedule(0);
  const auto heap = first.allocationLocations[1][history].heap;
  const uint64_t before = first.heapRequests[heap].size;
  REQUIRE(before > 2 * 1024);

  for (auto res : sub)
    f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(res));
  f.frame = 60;
  f.computeSchedule(1);
  f.frame = 120;
  f.computeSchedule(0, false);
  f.frame = 440;
  const auto shrunk = f.computeSchedule(1, false);
  CHECK(shrunk.heapRequests[heap].size < before);
  CHECK(shrunk.heapRequests[heap].size >= 2 * 1024);
  CHECK(f.scheduler.isResourcePreserved(1, history));
}

TEST_CASE("an empty class keeps only the memory of its ghosts", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto resB = f.addGpuResource();
  f.addLifetime(resB, node0, node1);
  const auto own = [&](size_t size) {
    auto res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_GPU_HEAP_2;
    f.propertyProvider.sizeOverrides[f.cFlagsOf(res)] = size;
    f.newResources.insert(res);
    f.addLifetime(res, node0, node1);
    return res;
  };
  const auto leave = [&](dafg::intermediate::ResourceIndex res) {
    f.graph.resources.erase(res);
    f.newResources.erase(res);
  };

  auto small = own(1024);
  f.computeSchedule(0);
  leave(small);
  f.frame = 10;
  f.computeSchedule(1);
  small = own(1024);
  f.frame = 50;
  f.computeSchedule(0);
  auto big = own(64 * 1024);
  f.frame = 60;
  const auto grown = f.computeSchedule(1);
  const auto heap = grown.allocationLocations[0][big].heap;
  REQUIRE(grown.heapRequests[heap].size >= 65 * 1024);

  leave(small);
  leave(big);
  f.frame = 100;
  f.computeSchedule(0);
  f.frame = 170;
  const auto shrunk = f.computeSchedule(1, false);
  uint64_t reserved = 0;
  for (const auto &req : shrunk.heapRequests)
    if (req.group == FAKE_GPU_HEAP_2)
      reserved += req.size;
  CHECK(reserved > 0);
  CHECK(reserved < 8 * 1024);
}

TEST_CASE("a ghost keeps its memory when a recompilation moves the indices of other resources", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  const auto first = f.addGpuResource();
  const auto second = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(first, node0, node1);
  f.addLifetime(second, node0, node1);
  const auto own = [&] {
    auto res = f.addGpuResource();
    f.propertyProvider.heapGroupOverrides[f.cFlagsOf(res)] = FAKE_GPU_HEAP_2;
    f.newResources.insert(res);
    f.addLifetime(res, node0, node1);
    return res;
  };
  const auto leave = [&](dafg::intermediate::ResourceIndex res) {
    f.graph.resources.erase(res);
    f.newResources.erase(res);
  };
  const auto measure_reserved_on_heap_2 = [](const auto &schedule) {
    uint64_t reserved = 0;
    for (const auto &req : schedule.heapRequests)
      if (req.group == FAKE_GPU_HEAP_2)
        reserved += req.size;
    return reserved;
  };

  auto returning = own();
  f.computeSchedule(0);
  leave(returning);
  f.frame = 10;
  f.computeSchedule(1);
  returning = own();
  f.frame = 50;
  f.computeSchedule(0);
  leave(returning);
  f.frame = 100;
  REQUIRE(measure_reserved_on_heap_2(f.computeSchedule(1)) > 0);

  eastl::swap(f.graph.resources[first], f.graph.resources[second]);
  eastl::swap(f.lifetimes[0][first], f.lifetimes[0][second]);
  eastl::swap(f.lifetimes[1][first], f.lifetimes[1][second]);
  f.pairedWith.push_back(second);
  f.pairedWith.push_back(first);
  f.frame = 110;

  CHECK(measure_reserved_on_heap_2(f.computeSchedule(0)) > 0);
}

TEST_CASE("switching dafg.no_aliasing packs the layout anew", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  auto node3 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node2, node3);
  const dafg::Timeline timeline = dafg::make_timeline(f.graph);
  const auto countSharedMemory = [&] {
    uint32_t result = 0;
    for (const auto &heap : f.scheduler.currentLayout().heaps)
      for (size_t i = 0; i < heap.rects.size(); ++i)
        for (size_t j = i + 1; j < heap.rects.size(); ++j)
          if (heap.rects[i].offset < heap.rects[j].end() && heap.rects[j].offset < heap.rects[i].end())
            ++result;
    return result;
  };

  f.computeSchedule();
  const uint32_t sharedBefore = countSharedMemory();
  REQUIRE(sharedBefore > 0);

  {
    ConVarGuard guard(no_aliasing, true);
    f.computeSchedule();
    CHECK(countSharedMemory() == 0);
    CHECK(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));
  }

  f.computeSchedule();
  CHECK(countSharedMemory() == sharedBefore);
  CHECK(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));
}

TEST_CASE("a random sequence of graph changes keeps the layout valid and the places stable", "[resourceScheduler]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.maxHeapSize = 64 * 1024;

  constexpr int RESOURCE_COUNT = 12;
  constexpr int HISTORY_COUNT = 4;
  constexpr int NODE_COUNT = 8;
  constexpr int STEPS = 200;

  dafg::intermediate::ResourceIndex resources[RESOURCE_COUNT];
  for (int i = 0; i < RESOURCE_COUNT; ++i)
    resources[i] = f.addGpuResource(i < HISTORY_COUNT ? dafg::History::DiscardOnFirstFrame : dafg::History::No);
  dafg::intermediate::NodeIndex nodes[NODE_COUNT];
  for (int i = 0; i < NODE_COUNT; ++i)
    nodes[i] = f.addNode();

  for (int i = 0; i < RESOURCE_COUNT; ++i)
  {
    f.propertyProvider.sizeOverrides[f.cFlagsOf(resources[i])] = 256 + (i % 4) * 256;
    if (i < HISTORY_COUNT)
      f.addTemporalLifetime(resources[i], nodes[i % NODE_COUNT], nodes[(i + 1) % NODE_COUNT]);
    else
      f.addLifetime(resources[i], nodes[i % NODE_COUNT],
        nodes[(i + 3) % NODE_COUNT < i % NODE_COUNT ? NODE_COUNT - 1 : (i + 3) % NODE_COUNT]);
  }

  uint32_t seed = 12345u;
  const auto next = [&seed](uint32_t bound) {
    seed = seed * 1664525u + 1013904223u;
    return (seed >> 16) % bound;
  };

  eastl::array<dag::Vector<dafg::AllocationLocation>, dafg::SCHEDULE_FRAME_WINDOW> was;
  for (auto &perFrame : was)
    perFrame.assign(RESOURCE_COUNT, dafg::AllocationLocation{
                                      .heap = dafg::HeapIndex::Invalid,
                                      .offset = dafg::UNSCHEDULED,
                                    });
  bool alive[RESOURCE_COUNT];
  for (auto &a : alive)
    a = true;
  const ConVarGuard validate(validate_layout, true);
  const ConVarGuard minWaste(defragment_min_waste_mb, 0);

  for (int step = 0; step < STEPS; ++step)
  {
    const int resource = static_cast<int>(next(RESOURCE_COUNT));
    const auto res = resources[resource];
    f.frame += 1 + next(40);
    f.lightFrame = next(2) == 0;
    uint32_t action = next(6);
    if (!alive[resource] && action != 5)
      action = 5;
    switch (action)
    {
      case 5:
        if (resource < HISTORY_COUNT)
          break;
        if (alive[resource])
        {
          f.graph.resources.erase(res);
          f.newResources.erase(res);
          alive[resource] = false;
        }
        else
        {
          resources[resource] = f.addGpuResource();
          f.newResources.insert(resources[resource]);
          f.propertyProvider.sizeOverrides[f.cFlagsOf(resources[resource])] = 256 + next(4) * 256;
          const uint32_t first = next(NODE_COUNT - 1);
          f.addLifetime(resources[resource], nodes[first], nodes[first + 1 + next(NODE_COUNT - 1 - first)]);
          for (auto &perFrame : was)
            perFrame[resource] = dafg::AllocationLocation{
              .heap = dafg::HeapIndex::Invalid,
              .offset = dafg::UNSCHEDULED,
            };
          alive[resource] = true;
        }
        break;
      case 0: f.propertyProvider.vacantCFlags.erase(f.cFlagsOf(res)); break;
      case 1: f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(res)); break;
      case 2: f.propertyProvider.sizeOverrides[f.cFlagsOf(res)] = 256 + next(4) * 256; break;
      case 3:
      {
        const uint32_t first = next(NODE_COUNT - 1);
        const uint32_t last = first + 1 + next(NODE_COUNT - 1 - first);
        if (resource < HISTORY_COUNT)
          f.addTemporalLifetime(res, nodes[first], nodes[last]);
        else
          f.addLifetime(res, nodes[first], nodes[last]);
        break;
      }
      case 4:
        if (resource < HISTORY_COUNT)
          f.scheduler.invalidateTemporalResource(res);
        else
          f.scheduler.invalidateTemporalResources();
        break;
    }

    const auto schedule = f.computeSchedule(step % dafg::SCHEDULE_FRAME_WINDOW);

    const dafg::Timeline timeline = dafg::make_timeline(f.graph);
    REQUIRE(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));

    for (int i = 0; i < RESOURCE_COUNT; ++i)
      for (uint32_t frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
      {
        if (!alive[i])
          continue;
        const bool touched = f.scheduler.placementsChanged()[frame].test(resources[i], false);
        const auto now = schedule.allocationLocations[frame][resources[i]];
        const auto before = was[frame][i];
        if (!touched && before.heap != dafg::HeapIndex::Invalid)
        {
          CHECK(now.heap == before.heap);
          CHECK(now.offset == before.offset);
        }
        was[frame][i] = now;
      }
  }
}

TEST_CASE("adding a resource places only its own frame resources", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  const auto first = f.computeSchedule(0);

  f.allValuesChanged = false;
  auto resC = f.addGpuResource();
  f.addLifetime(resC, node0, node1);
  f.newResources.insert(resC);
  const uint32_t queriesBefore = f.propertyProvider.allocationPropertyQueries;

  const auto second = f.computeSchedule(1, false);
  const auto changes = f.placements();

  CHECK(f.propertyProvider.allocationPropertyQueries == queriesBefore + 1);
  for (int frame = 0; frame < dafg::SCHEDULE_FRAME_WINDOW; ++frame)
  {
    CHECK(second.allocationLocations[frame][resA].heap == first.allocationLocations[frame][resA].heap);
    CHECK(second.allocationLocations[frame][resA].offset == first.allocationLocations[frame][resA].offset);
    CHECK(second.allocationLocations[frame][resB].offset == first.allocationLocations[frame][resB].offset);
    CHECK(second.allocationLocations[frame][resC].heap != dafg::HeapIndex::Invalid);
  }
  REQUIRE(changes.placed.size() == 2);
  CHECK(changes.placed[0].resIdx == resC);
  CHECK(changes.placed[1].resIdx == resC);
  CHECK(changes.removed.empty());
}

TEST_CASE("an unchanged IR graph with every flag set places nothing again", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto resA = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  f.computeSchedule(0);

  f.computeSchedule(1);
  const auto changes = f.placements();
  CHECK(changes.placed.empty());
  CHECK(changes.removed.empty());
  CHECK(f.scheduler.isResourcePreserved(1, resA));
}

TEST_CASE("a frame resource with a new corrected size and no flag is placed again", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  const auto first = f.computeSchedule(0);

  f.allValuesChanged = false;
  f.corrections[0][resA] = 2048;
  const auto second = f.computeSchedule(1, false);
  const auto changes = f.placements();

  REQUIRE(changes.placed.size() == 1);
  CHECK(changes.placed[0].resIdx == resA);
  CHECK(changes.placed[0].frame == 0);
  CHECK(second.heapRequests[second.allocationLocations[0][resA].heap].size >= 3 * 1024);
  CHECK(second.allocationLocations[1][resA].offset == first.allocationLocations[1][resA].offset);
  CHECK(second.allocationLocations[0][resB].offset == first.allocationLocations[0][resB].offset);

  f.computeSchedule(0, false);
  CHECK(f.placements().placed.empty());
}

TEST_CASE("a grown lifetime without preserved history places only its own frame resources again", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  // Two resources are too few for the unplaced fraction to mean anything
  const ConVarGuard guard(defragment_placed_anew_fraction, 1.f);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  auto node2 = f.addNode();
  auto node3 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node2, node3);

  const auto first = f.computeSchedule(0);
  REQUIRE(first.allocationLocations[0][resA].offset == first.allocationLocations[0][resB].offset);
  const auto heap = first.allocationLocations[0][resA].heap;
  REQUIRE(first.heapRequests[heap].size == 1024);

  f.allValuesChanged = false;
  f.addLifetime(resA, node0, node3);
  f.changedLifetimes.insert(resA);
  const auto second = f.computeSchedule(1, false);
  const auto changes = f.placements();

  CHECK(second.allocationLocations[0][resB].offset == first.allocationLocations[0][resB].offset);
  CHECK(second.allocationLocations[0][resA].offset != second.allocationLocations[0][resB].offset);
  REQUIRE(changes.placed.size() == 2);
  CHECK(changes.placed[0].resIdx == resA);
  CHECK(changes.placed[1].resIdx == resA);
  const auto aAt = second.allocationLocations[0][resA];
  const auto bAt = second.allocationLocations[0][resB];
  CHECK((aAt.heap != bAt.heap || aAt.offset != bAt.offset));
}

TEST_CASE("a grown preserved lifetime keeps its place and evicts only the rect it now overlaps", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  // Two resources are too few for the unplaced fraction to mean anything
  const ConVarGuard guard(defragment_placed_anew_fraction, 1.f);
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;
  f.propertyProvider.maxHeapSize = 1024;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resX = f.addGpuResource();
  dafg::intermediate::NodeIndex nodes[5];
  for (auto &node : nodes)
    node = f.addNode();
  f.addTemporalLifetime(resT, nodes[3], nodes[1]);
  f.addLifetime(resX, nodes[2], nodes[3]);

  const auto first = f.computeSchedule(0);
  const auto tAt = first.allocationLocations[1][resT];
  REQUIRE(first.allocationLocations[0][resX].heap == tAt.heap);
  REQUIRE(first.allocationLocations[0][resX].offset == tAt.offset);

  // The frame resource of T that starts in frame 1 is preserved now, and its
  // longer lifetime overlaps the frame resource of X in frame 0.
  f.allValuesChanged = false;
  f.addTemporalLifetime(resT, nodes[3], nodes[3]);
  f.changedLifetimes.insert(resT);
  const auto second = f.computeSchedule(1, false);
  const auto changes = f.placements();

  CHECK(second.allocationLocations[1][resT].heap == tAt.heap);
  CHECK(second.allocationLocations[1][resT].offset == tAt.offset);
  CHECK(f.scheduler.isResourcePreserved(1, resT));
  const dafg::Timeline timeline = dafg::make_timeline(f.graph);
  CHECK(dafg::is_layout_valid(f.scheduler.currentLayout(), timeline));
  bool xMoved = false;
  for (const auto &placed : changes.placed)
  {
    CHECK_FALSE((placed.resIdx == resT && placed.frame == 1));
    CHECK_FALSE((placed.resIdx == resX && placed.frame == 1));
    xMoved = xMoved || (placed.resIdx == resX && placed.frame == 0);
  }
  CHECK(xMoved);
}

TEST_CASE("a resource that becomes vacant is removed and nothing else moves", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;
  f.propertyProvider.defaultSize = 1024;

  auto resA = f.addGpuResource();
  auto resB = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addLifetime(resA, node0, node1);
  f.addLifetime(resB, node0, node1);

  const auto first = f.computeSchedule(0);

  f.allValuesChanged = false;
  f.propertyProvider.vacantCFlags.insert(f.cFlagsOf(resA));
  f.changedValues.insert(resA);
  const auto second = f.computeSchedule(1, false);
  const auto changes = f.placements();

  CHECK(changes.placed.empty());
  REQUIRE(changes.removed.size() == 2);
  CHECK(changes.removed[0].resIdx == resA);
  CHECK(second.allocationLocations[0][resA].heap == dafg::HeapIndex::Invalid);
  CHECK(second.allocationLocations[0][resB].offset == first.allocationLocations[0][resB].offset);
}

TEST_CASE("an untouched history frame resource stays preserved with the flags off", "[resourceScheduler][incremental]")
{
  FRAMEMEM_REGION;
  ResourceSchedulerFixture f;

  auto resT = f.addGpuResource(dafg::History::DiscardOnFirstFrame);
  auto resX = f.addGpuResource();
  auto node0 = f.addNode();
  auto node1 = f.addNode();
  f.addTemporalLifetime(resT, node0, node1);
  f.addLifetime(resX, node0, node1);

  f.computeSchedule(0);
  f.allValuesChanged = false;

  f.computeSchedule(1, false);
  CHECK(f.scheduler.isResourcePreserved(1, resT));
  CHECK_FALSE(f.scheduler.isResourcePreserved(1, resX));

  f.computeSchedule(0, false);
  CHECK(f.scheduler.isResourcePreserved(0, resT));
}
