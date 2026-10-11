// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "resourceScheduler.h"

#include <EASTL/algorithm.h>
#include <EASTL/sort.h>

#include <ioSys/dag_dataBlock.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>
#include <util/dag_string.h>
#include <drv/3d/dag_texFlags.h>
#include <memory/dag_framemem.h>

#include <common/resourceUsage.h>


CONSOLE_BOOL_VAL("dafg", dump_resources, false);
CONSOLE_BOOL_VAL("dafg", validate_layout, false);

CONSOLE_BOOL_VAL("dafg", fill_dedicated_fast_gpu_local, true);

// Weights say how much memory traffic a usage is assumed to cause, which is
// what makes a resource worth putting into the on-chip heap.
CONSOLE_FLOAT_VAL("dafg", dedicated_attachment_weight, 4.f);
CONSOLE_FLOAT_VAL("dafg", dedicated_unordered_access_weight, 3.f);
CONSOLE_FLOAT_VAL("dafg", dedicated_shader_resource_weight, 2.f);
CONSOLE_FLOAT_VAL("dafg", dedicated_transfer_weight, 2.f);
CONSOLE_FLOAT_VAL("dafg", dedicated_other_weight, 1.f);
// How much bigger than the heap the list of considered candidates may get
CONSOLE_FLOAT_VAL_MINMAX("dafg", dedicated_shortlist_size_multiplier, 3.f, 0.f, 64.f);

CONSOLE_INT_VAL("dafg", heap_growth_step_mb, 8, 1, 4096);
CONSOLE_INT_VAL("dafg", cpu_heap_growth_step_kb, 64, 1, 1 << 20);
static constexpr dafg::HeapPolicySettings DEFAULT_POLICY{};
CONSOLE_FLOAT_VAL_MINMAX("dafg", defragment_placed_anew_fraction, DEFAULT_POLICY.defragmentPlacedAnewFraction, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("dafg", defragment_waste_fraction, DEFAULT_POLICY.defragmentWasteFraction, 0.f, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("dafg", defragment_waste_fraction_stop, DEFAULT_POLICY.defragmentWasteFractionStop, 0.f, 1.f);
CONSOLE_INT_VAL("dafg", defragment_min_waste_mb, static_cast<int>(DEFAULT_POLICY.defragmentMinWaste >> 20), 0, 1 << 16);
CONSOLE_INT_VAL("dafg", defragment_min_waste_objects, 16, 0, 1 << 20);
CONSOLE_INT_VAL("dafg", defragment_delay_frames, static_cast<int>(DEFAULT_POLICY.defragmentDelayFrames), 0, 1 << 24);
CONSOLE_INT_VAL("dafg", defragment_check_interval_frames, static_cast<int>(DEFAULT_POLICY.defragmentCheckIntervalFrames), 0, 1 << 24);
CONSOLE_INT_VAL("dafg", defragment_max_wait_frames, static_cast<int>(DEFAULT_POLICY.defragmentMaxWaitFrames), 0, 1 << 24);
CONSOLE_INT_VAL("dafg", relocation_max_mb, static_cast<int>(DEFAULT_POLICY.relocationMaxBytes >> 20), 0, 1 << 16);
CONSOLE_FLOAT_VAL_MINMAX("dafg", relocation_waste_ratio, DEFAULT_POLICY.relocationWasteRatio, 0.f, 64.f);
CONSOLE_FLOAT_VAL_MINMAX("dafg", defragment_headroom, DEFAULT_POLICY.defragmentHeadroom, 0.f, 16.f);
CONSOLE_BOOL_VAL("dafg", no_aliasing, false);

namespace dafg
{


void load_resource_scheduling_settings()
{
  const DataBlock *settings = dgs_get_settings ? dgs_get_settings() : nullptr;
  if (settings == nullptr)
    return;

  const DataBlock *dafgBlk = settings->getBlockByNameEx("dafg");
  fill_dedicated_fast_gpu_local.set(dafgBlk->getBool("fillDedicatedFastGPULocal", fill_dedicated_fast_gpu_local.get()));

  const DataBlock *blk = dafgBlk->getBlockByNameEx("dedicatedFastGPULocal");
  dedicated_attachment_weight.set(blk->getReal("attachmentWeight", dedicated_attachment_weight.get()));
  dedicated_unordered_access_weight.set(blk->getReal("unorderedAccessWeight", dedicated_unordered_access_weight.get()));
  dedicated_shader_resource_weight.set(blk->getReal("shaderResourceWeight", dedicated_shader_resource_weight.get()));
  dedicated_transfer_weight.set(blk->getReal("transferWeight", dedicated_transfer_weight.get()));
  dedicated_other_weight.set(blk->getReal("otherWeight", dedicated_other_weight.get()));
  dedicated_shortlist_size_multiplier.set(blk->getReal("shortlistSizeMultiplier", dedicated_shortlist_size_multiplier.get()));

  const DataBlock *heaps = dafgBlk->getBlockByNameEx("heaps");
  heap_growth_step_mb.set(heaps->getInt("growthStepMb", heap_growth_step_mb.get()));
  cpu_heap_growth_step_kb.set(heaps->getInt("cpuGrowthStepKb", cpu_heap_growth_step_kb.get()));
  defragment_placed_anew_fraction.set(heaps->getReal("defragmentPlacedAnewFraction", defragment_placed_anew_fraction.get()));
  defragment_waste_fraction.set(heaps->getReal("defragmentWasteFraction", defragment_waste_fraction.get()));
  defragment_waste_fraction_stop.set(heaps->getReal("defragmentWasteFractionStop", defragment_waste_fraction_stop.get()));
  defragment_min_waste_mb.set(heaps->getInt("defragmentMinWasteMb", defragment_min_waste_mb.get()));
  defragment_min_waste_objects.set(heaps->getInt("defragmentMinWasteObjects", defragment_min_waste_objects.get()));
  defragment_delay_frames.set(heaps->getInt("defragmentDelayFrames", defragment_delay_frames.get()));
  defragment_check_interval_frames.set(heaps->getInt("defragmentCheckIntervalFrames", defragment_check_interval_frames.get()));
  defragment_max_wait_frames.set(heaps->getInt("defragmentMaxWaitFrames", defragment_max_wait_frames.get()));
  relocation_max_mb.set(heaps->getInt("relocationMaxMb", relocation_max_mb.get()));
  relocation_waste_ratio.set(heaps->getReal("relocationWasteRatio", relocation_waste_ratio.get()));
  defragment_headroom.set(heaps->getReal("defragmentHeadroom", defragment_headroom.get()));
  no_aliasing.set(heaps->getBool("noAliasing", no_aliasing.get()));
}

static uint64_t choose_growth_step(ResourceHeapGroup *group, bool heaps_count_objects)
{
  if (group == CPU_HEAP_GROUP)
    return eastl::max<uint64_t>(static_cast<uint64_t>(cpu_heap_growth_step_kb.get()) << 10, 1);
  return heaps_count_objects ? 1 : eastl::max<uint64_t>(static_cast<uint64_t>(heap_growth_step_mb.get()) << 20, 1);
}

static HeapPolicySettings read_policy_settings(bool heaps_count_objects)
{
  return {
    .defragmentMinWaste = heaps_count_objects ? static_cast<uint64_t>(defragment_min_waste_objects.get())
                                              : static_cast<uint64_t>(defragment_min_waste_mb.get()) << 20,
    .relocationMaxBytes = static_cast<uint64_t>(relocation_max_mb.get()) << 20,
    .defragmentDelayFrames = static_cast<uint32_t>(defragment_delay_frames.get()),
    .defragmentCheckIntervalFrames = static_cast<uint32_t>(defragment_check_interval_frames.get()),
    .defragmentMaxWaitFrames = static_cast<uint32_t>(defragment_max_wait_frames.get()),
    .defragmentPlacedAnewFraction = defragment_placed_anew_fraction.get(),
    .defragmentWasteFraction = defragment_waste_fraction.get(),
    .defragmentWasteFractionStop = defragment_waste_fraction_stop.get(),
    .relocationWasteRatio = relocation_waste_ratio.get(),
    .defragmentHeadroom = defragment_headroom.get(),
  };
}


struct DescribedResource
{
  ResourceAllocationProperties props{};
  ResourceDescription::HashT hash = 0;
};

struct Footprint
{
  uint64_t size = 0;
  uint64_t align = 1;
};

static uint64_t corrected_resource_size(const ResourceAllocationProperties &props,
  const BadResolutionTracker::Corrections &corrections, intermediate::ResourceIndex idx, uint32_t frame)
{
  const bool corrected = corrections[frame].isMapped(idx) && corrections[frame][idx] != 0;
  return corrected ? corrections[frame][idx] : props.sizeInBytes;
}

struct QueuedClassLess
{
  bool operator()(const WantedRect &a, HeapClass b) const { return HeapClassLess{}(a.heapClass, b); }
  bool operator()(HeapClass a, const WantedRect &b) const { return HeapClassLess{}(a, b.heapClass); }
};

// Each frame resource is in the queue once. So an unstable sort gives the same
// order every time, and it needs no buffer.
static bool compare_by_class_then_resource(const WantedRect &a, const WantedRect &b)
{
  if (!(a.heapClass == b.heapClass))
    return HeapClassLess{}(a.heapClass, b.heapClass);
  if (a.what.resIdx != b.what.resIdx)
    return a.what.resIdx < b.what.resIdx;
  return a.what.frame < b.what.frame;
}

static HeapClass get_class_of(const Layout &layout, FrameResource what) { return layout.heaps[layout.where[what].heap].heapClass; }

static HeapClass get_natural_class(const ResourceAllocationProperties &props, const intermediate::Resource &res)
{
  return HeapClass{
    .group = props.heapGroup,
    .untracked = res.isUntracked(),
  };
}

static DescribedResource describe(const intermediate::Resource &res, IResourcePropertyProvider &provider)
{
  if (!res.isScheduled())
    return {};
  if (res.asScheduled().isGpuResource())
  {
    const auto &desc = res.asScheduled().getGpuDescription();
    return {
      .props = provider.getResourceAllocationProperties(desc),
      .hash = desc.hash(),
    };
  }
  const auto &desc = res.asScheduled().getCpuDescription();
  return {
    .props =
      {
        .sizeInBytes = desc.size,
        .offsetAlignment = desc.alignment,
        .heapGroup = CPU_HEAP_GROUP,
      },
    .hash = eastl::to_underlying(desc.typeTag),
  };
}

static bool can_be_placed_on_chip(const ResourceDescription &desc)
{
  const uint32_t flags = desc.asBasicRes.cFlags;
  if (desc.type == D3DResourceType::SBUF)
    return (flags & (SBCF_DYNAMIC | SBCF_CPU_ACCESS_MASK | SBCF_USAGE_READ_BACK)) == 0;
  return (flags & (TEXCF_SYSMEM | TEXCF_CPU_CACHED_MEMORY | TEXCF_LINEAR_LAYOUT | TEXCF_TILED_RESOURCE)) == 0;
}

static bool is_promotable(const intermediate::Resource &res, uint64_t size, uint64_t on_chip_heap_size)
{
  if (!res.isScheduled() || !res.asScheduled().isGpuResource() || res.isUntracked())
    return false;
  // History resources wrap around the timeline, which optional resources
  // are not allowed to do.
  if (res.asScheduled().history != History::No)
    return false;
  if (!can_be_placed_on_chip(res.asScheduled().getGpuDescription()))
    return false;
  if (size == 0 || size > on_chip_heap_size)
    return false;
  if (res.debugRelated)
    return false;
  return true;
}

static float usage_bandwidth_weight(const intermediate::ResourceUsage &usage)
{
  const auto has = [&](Usage bit) { return (usage.type & bit) == bit; };

  if (has(Usage::COLOR_ATTACHMENT) || has(Usage::DEPTH_ATTACHMENT) || has(Usage::RESOLVE_ATTACHMENT))
    return dedicated_attachment_weight.get();
  if (has(Usage::SHADER_RESOURCE))
    return usage.access == Access::READ_WRITE ? dedicated_unordered_access_weight.get() : dedicated_shader_resource_weight.get();
  if (has(Usage::COPY) || has(Usage::BLIT))
    return dedicated_transfer_weight.get();
  return dedicated_other_weight.get();
}

static IdIndexedMapping<intermediate::ResourceIndex, float, framemem_allocator> measure_bandwidth_per_resource(
  const intermediate::Graph &graph)
{
  IdIndexedMapping<intermediate::ResourceIndex, float, framemem_allocator> result(graph.resources.totalKeys(), 0.f);
  for (const auto &node : graph.nodes.values())
    for (const auto &req : node.resourceRequests)
      result[req.resource] += usage_bandwidth_weight(req.usage);
  return result;
}

static dag::Vector<uint32_t, framemem_allocator> select_on_chip_candidates(eastl::span<const WantedRect> candidates,
  uint64_t heap_size, const intermediate::Graph &graph)
{
  TIME_PROFILE(selectOnChipCandidates);
  dag::Vector<uint32_t, framemem_allocator> result;
  result.reserve(candidates.size());

  const auto activity = measure_bandwidth_per_resource(graph);

  struct Candidate
  {
    uint32_t item = 0;
    float score = 0.f;
  };
  dag::Vector<Candidate, framemem_allocator> shortlist;
  shortlist.reserve(candidates.size());
  for (uint32_t i = 0; i < candidates.size(); ++i)
  {
    const auto &rect = candidates[i];
    const auto idx = rect.what.resIdx;
    shortlist.push_back({
      .item = i,
      .score = activity[idx] / static_cast<float>(rect.size),
    });
  }
  eastl::sort(shortlist.begin(), shortlist.end(), [](const Candidate &a, const Candidate &b) { return a.score > b.score; });

  const uint64_t shortlistCap = static_cast<uint64_t>(static_cast<double>(heap_size) * dedicated_shortlist_size_multiplier.get());
  uint64_t accumulated = 0;
  for (const auto &candidate : shortlist)
  {
    if (accumulated > shortlistCap)
      break;
    accumulated += candidates[candidate.item].size;
    result.push_back(candidate.item);
  }
  return result;
}

void ResourceScheduler::refreshGroupInfo(HeapGroupInfo &info, ResourceHeapGroup *group, IResourcePropertyProvider &provider) const
{
  const bool countObjects = group != CPU_HEAP_GROUP && provider.doHeapsCountObjects();
  info.policy = read_policy_settings(countObjects);
  if (group == CPU_HEAP_GROUP)
  {
    info.onChip = false;
    info.limits = {
      .growthStep = choose_growth_step(group, countObjects),
      .noAliasing = no_aliasing.get(),
    };
    return;
  }
  const auto props = provider.getResourceHeapGroupProperties(group);
  info.onChip = props.isDedicatedFastGPULocal;
  info.limits = {
    .maxHeapSize = props.maxHeapSize,
    .optimalMaxHeapSize = props.optimalMaxHeapSize,
    .growthStep = choose_growth_step(group, countObjects),
    .fixedSize = info.onChip,
    .resizeKeepsContents = provider.doesResizeKeepContents(),
    .noAliasing = no_aliasing.get(),
  };
}

ResourceScheduler::HeapGroupInfo &ResourceScheduler::getGroupInfo(ResourceHeapGroup *group, IResourcePropertyProvider &provider)
{
  auto it = groups.find(group);
  if (it == groups.end())
  {
    it = groups.emplace(group, HeapGroupInfo{}).first;
    refreshGroupInfo(it->second, group, provider);
  }
  return it->second;
}

eastl::optional<HeapClass> ResourceScheduler::findOnChipClass() const
{
  for (const auto &[group, info] : groups)
    if (info.onChip)
      return HeapClass{
        .group = group,
        .untracked = false,
      };
  return eastl::nullopt;
}

// Const groups give the visitor const group info
template <class Groups, class Visit>
static void for_each_class(Groups &groups, const Visit &visit)
{
  for (auto &[group, info] : groups)
    for (uint32_t untracked = 0; untracked < 2; ++untracked)
      visit(
        HeapClass{
          .group = group,
          .untracked = untracked != 0,
        },
        info, info.defragmentation[untracked]);
}

bool ResourceScheduler::isOnChipNative(HeapClass on_chip, FrameResource what) const
{
  return properties[what.resIdx].heapGroup == on_chip.group;
}

static intermediate::ResourceIndex rekey_index(const InverseOfPairingFmem &inverse, intermediate::ResourceIndex old_idx)
{
  return inverse.isMapped(old_idx) ? inverse[old_idx] : intermediate::RESOURCE_NOT_MAPPED;
}

static void rekey_indices(dag::Vector<intermediate::ResourceIndex> &indices, const InverseOfPairingFmem &inverse)
{
  size_t kept = 0;
  for (const auto oldIdx : indices)
    if (const auto newIdx = rekey_index(inverse, oldIdx); newIdx != intermediate::RESOURCE_NOT_MAPPED)
      indices[kept++] = newIdx;
  indices.resize(kept);
}

void ResourceScheduler::trimCaches(size_t res_count)
{
  properties.resize(res_count, ResourceAllocationProperties{});
  descriptionHashes.resize(res_count, 0);
  for (auto &perFrame : correctedSizes)
    perFrame.resize(res_count, 0);
  slots.resize(res_count, SlotStats{});
  for (auto &ghost : ghosts)
    if (!slots.isMapped(ghost.slot))
      ghost.slot = intermediate::RESOURCE_NOT_MAPPED;
  while (!leftNatives.empty() && !slots.isMapped(leftNatives.back().first))
    leftNatives.pop_back();
}

void ResourceScheduler::remapCaches(const SchedulingContext &ctx, const InverseOfPairingFmem &inverse)
{
  const auto resCount = ctx.graph.resources.totalKeys();
  ResourceProperties newProps(resCount, ResourceAllocationProperties{});
  IdIndexedMapping<intermediate::ResourceIndex, ResourceDescription::HashT> newHashes(resCount, 0);
  eastl::array<IdIndexedMapping<intermediate::ResourceIndex, uint64_t>, SCHEDULE_FRAME_WINDOW> newCorrected;
  for (auto &perFrame : newCorrected)
    perFrame.assign(resCount, 0);
  IdIndexedMapping<intermediate::ResourceIndex, SlotStats> newSlots(resCount, SlotStats{});

  for (auto &&[idx, oldIdx] : ctx.historyPairing.enumerate())
  {
    if (!newProps.isMapped(idx) || oldIdx == intermediate::RESOURCE_NOT_MAPPED || !properties.isMapped(oldIdx))
      continue;
    newProps[idx] = properties[oldIdx];
    newHashes[idx] = descriptionHashes[oldIdx];
    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
      newCorrected[frame][idx] = correctedSizes[frame][oldIdx];
    newSlots[idx] = slots[oldIdx];
  }

  dag::VectorMap<intermediate::ResourceIndex, ResourceAllocationProperties> newLeft;
  for (const auto &[oldIdx, onChipProps] : leftNatives)
    if (const auto newIdx = rekey_index(inverse, oldIdx); newIdx != intermediate::RESOURCE_NOT_MAPPED)
      newLeft[newIdx] = onChipProps;

  properties = eastl::move(newProps);
  descriptionHashes = eastl::move(newHashes);
  correctedSizes = eastl::move(newCorrected);
  slots = eastl::move(newSlots);
  leftNatives = eastl::move(newLeft);
}

void ResourceScheduler::rekeyForPairing(const SchedulingContext &ctx)
{
  if (intermediate::does_pairing_keep_indices(ctx.historyPairing))
  {
    trimCaches(ctx.graph.resources.totalKeys());
    return;
  }
  const auto inverse = invert_pairing(ctx.historyPairing);
  reindex(layout, inverse, ctx.historyPairing.totalKeys());
  remapCaches(ctx, inverse);
  for (auto &ghost : ghosts)
    ghost.slot = rekey_index(inverse, ghost.slot);
  rekey_indices(historyInvalidFor, inverse);
}

void ResourceScheduler::noteDeparture(intermediate::ResourceIndex idx, HeapClass heap_class, uint64_t size, uint64_t align,
  uint32_t frame)
{
  auto &slot = slots[idx];
  slot.lastClass = heap_class;
  slot.vacatedAt = frame;
  if (slot.refills == 0)
    return;
  auto ghost = findGhostOf(idx);
  if (ghost == ghosts.end())
    ghost = &ghosts.push_back();
  const uint32_t horizon = 2 * slot.awayAverage;
  *ghost = Ghost{
    .slot = idx,
    .heapClass = heap_class,
    .size = size + (align > 1 ? align - 1 : 0),
    .until = static_cast<int64_t>(frame) + horizon,
  };
  keepFrames[idx] = horizon;
}

static uint32_t blend_into_moving_average(uint32_t average, uint32_t sample)
{
  return static_cast<uint32_t>((uint64_t{average} * 3 + sample) / 4);
}

void ResourceScheduler::noteRefill(intermediate::ResourceIndex idx, HeapClass heap_class, uint32_t frame)
{
  auto &slot = slots[idx];
  if (slot.vacatedAt < 0)
    return;
  if (const auto ghost = findGhostOf(idx); ghost != ghosts.end() && ghost->heapClass == heap_class)
    ghosts.erase(ghost);
  if (slot.lastClass != heap_class)
  {
    slot.vacatedAt = -1;
    slot.refills = 0;
    slot.awayAverage = 0;
    return;
  }
  const uint32_t away = static_cast<uint32_t>(eastl::max<int64_t>(static_cast<int64_t>(frame) - slot.vacatedAt, 0));
  slot.awayAverage = slot.refills == 0 ? away : blend_into_moving_average(slot.awayAverage, away);
  ++slot.refills;
  slot.vacatedAt = -1;
}

ResourceScheduler::Ghost *ResourceScheduler::findGhostOf(intermediate::ResourceIndex idx)
{
  return eastl::find_if(ghosts.begin(), ghosts.end(), [idx](const Ghost &ghost) { return ghost.slot == idx; });
}

void ResourceScheduler::expireGhosts(uint32_t frame)
{
  ghosts.erase(
    eastl::remove_if(ghosts.begin(), ghosts.end(), [frame](const Ghost &ghost) { return ghost.until <= static_cast<int64_t>(frame); }),
    ghosts.end());
}

uint64_t ResourceScheduler::ghostBytes(HeapClass heap_class) const
{
  uint64_t result = 0;
  for (const auto &ghost : ghosts)
    if (ghost.heapClass == heap_class)
      result += ghost.size;
  return result;
}

WantedRect ResourceScheduler::makeWantedRect(FrameResource what, HeapClass heap_class, const SchedulingContext &ctx) const
{
  const auto [idx, frame] = what;
  const auto &props = properties[idx];
  return {
    .what = what,
    .size = correctedSizes[frame][idx],
    .align = props.offsetAlignment,
    .lifetime = ctx.lifetimes[frame][idx],
    .hash = descriptionHashes[idx],
    .heapClass = heap_class,
  };
}

WantedRect ResourceScheduler::leaveOnChip(FrameResource what, const SchedulingContext &ctx)
{
  const auto idx = what.resIdx;
  const auto &res = ctx.graph.resources[idx];
  auto &props = properties[idx];
  if (!getGroupInfo(props.heapGroup, ctx.propertyProvider).onChip)
    return makeWantedRect(what, get_natural_class(props, res), ctx);

  const auto regular = ctx.propertyProvider.getResourceAllocationProperties(res.asScheduled().getGpuDescription(), true);
  if (regular.heapGroup == props.heapGroup)
  {
    logerr("daFG: resource '%s' does not fit into the on-chip heap and has nowhere else to go", ctx.graph.resourceNames[idx].c_str());
    DAG_FATAL("daFG: failed to give every resource memory, rendering would crash");
  }
  if (!res.isUntracked())
    leftNatives[idx] = props;
  props = regular;
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    correctedSizes[frame][idx] = corrected_resource_size(props, ctx.corrections, idx, frame);
  return makeWantedRect(what, get_natural_class(props, res), ctx);
}

HeapClass ResourceScheduler::keepPromotedClass(FrameResource what, HeapClass natural, uint64_t size, const SchedulingContext &ctx)
{
  if (!layout.has(what))
    return natural;
  const auto current = get_class_of(layout, what);
  if (current == natural || current.untracked || !getGroupInfo(current.group, ctx.propertyProvider).onChip)
    return natural;
  return is_promotable(ctx.graph.resources[what.resIdx], size, eastl::numeric_limits<uint64_t>::max()) ? current : natural;
}

auto ResourceScheduler::collectResourcesToRevisit(const SchedulingContext &ctx, size_t old_count) const -> ResourceSetFmem
{
  TIME_PROFILE(collectResourcesToRevisit);

  const size_t resCount = ctx.graph.resources.totalKeys();
  ResourceSetFmem revisit(eastl::max(resCount, old_count), false);
  for (size_t i = resCount; i < old_count; ++i)
    revisit.set(static_cast<intermediate::ResourceIndex>(i), true);

  for (const auto idx : ctx.resourcesChanged.trueKeys())
    revisit.set(idx, true);
  for (const auto idx : ctx.lifetimesChanged.trueKeys())
    revisit.set(idx, true);

  for (auto &&[idx, oldIdx] : ctx.historyPairing.enumerate())
    if (oldIdx == intermediate::RESOURCE_NOT_MAPPED && ctx.graph.resources.isMapped(idx))
      revisit.set(idx, true);
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (auto &&[idx, size] : correctedSizes[frame].enumerate())
      if (size != 0 && ctx.corrections[frame].isMapped(idx) &&
          corrected_resource_size(properties[idx], ctx.corrections, idx, frame) != size)
        revisit.set(idx, true);

  return revisit;
}

bool ResourceScheduler::isRectOnChip(FrameResource what, const SchedulingContext &ctx)
{
  return getGroupInfo(get_class_of(layout, what).group, ctx.propertyProvider).onChip;
}

void ResourceScheduler::stripTo(const WantedRect &wanted, WantedRectsFmem &to_be_placed, FrameResourceSet &stripped,
  const SchedulingContext &ctx)
{
  if (isRectOnChip(wanted.what, ctx))
    onChipRoomFreed = true;
  strip_rect(layout, wanted, to_be_placed);
  stripped.insert(wanted.what);
}

void ResourceScheduler::dropGoneSlot(intermediate::ResourceIndex idx)
{
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    if (const FrameResource what{
          .resIdx = idx,
          .frame = frame,
        };
        layout.has(what))
      remove_rect(layout, what, changedPlacements);
}

void ResourceScheduler::updateRect(FrameResource what, const WantedRect &want, const Preservation &preservation,
  WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, FrameResourceSet &evicted, const SchedulingContext &ctx)
{
  const LayoutRect *rect = layout.find(what);
  if (rect == nullptr)
  {
    to_be_placed.push_back(want);
    return;
  }
  WantedRect sameObject = want;
  sameObject.wasAt = layout.where[what];
  const HeapClass currentClass = layout.heaps[sameObject.wasAt.heap].heapClass;
  if (rect->size != want.size || rect->align != want.align || rect->hash != want.hash)
    stripTo(want, to_be_placed, stripped, ctx);
  else if (!(currentClass == want.heapClass))
    stripTo(sameObject, to_be_placed, stripped, ctx);
  else if (rect->lifetime != want.lifetime)
  {
    if (preservation.covers(layout, what))
      set_lifetime(layout, what, want.lifetime, ctx.timeline, evicted);
    else if (fits_in_place(layout, what, want.lifetime, ctx.timeline))
      layout.setLifetime(what, want.lifetime);
    else
      stripTo(sameObject, to_be_placed, stripped, ctx);
  }
}

static Footprint measure_departed_footprint(const eastl::array<LayoutRect, SCHEDULE_FRAME_WINDOW> &departed, const Timeline &timeline)
{
  const bool overlap = departed[0].size > 0 && departed[1].size > 0 && timeline.overlap(departed[0].lifetime, departed[1].lifetime);
  return {
    .size = overlap ? departed[0].size + departed[1].size : eastl::max(departed[0].size, departed[1].size),
    .align = eastl::max(departed[0].align, departed[1].align),
  };
}

void ResourceScheduler::updateResource(intermediate::ResourceIndex idx, const SchedulingContext &ctx, const Preservation &preservation,
  WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, FrameResourceSet &evicted)
{
  if (idx >= ctx.graph.resources.totalKeys())
  {
    dropGoneSlot(idx);
    return;
  }

  const bool inGraph = ctx.graph.resources.isMapped(idx);
  const bool isNew = !inGraph || !ctx.historyPairing.isMapped(idx) || ctx.historyPairing[idx] == intermediate::RESOURCE_NOT_MAPPED;
  const bool resourceChanged = isNew || ctx.resourcesChanged.test(idx, true);

  auto &props = properties[idx];
  if (resourceChanged)
  {
    if (const auto left = leftNatives.find(idx); left != leftNatives.end())
      leftNatives.erase(left);
    const auto described = inGraph ? describe(ctx.graph.resources[idx], ctx.propertyProvider) : DescribedResource{};
    props = described.props;
    descriptionHashes[idx] = described.hash;
  }
  const bool needsMemory = props.offsetAlignment != 0;
  const HeapClass natural = inGraph ? get_natural_class(props, ctx.graph.resources[idx]) : HeapClass{};
  const bool naturalOnChip = needsMemory && getGroupInfo(natural.group, ctx.propertyProvider).onChip;
  if (inGraph && isNew)
    noteRefill(idx, natural, ctx.frameIndex);

  eastl::array<LayoutRect, SCHEDULE_FRAME_WINDOW> departed{};
  HeapClass departedClass{};
  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
  {
    const FrameResource what{
      .resIdx = idx,
      .frame = frame,
    };
    const uint64_t size = needsMemory ? corrected_resource_size(props, ctx.corrections, idx, frame) : 0;
    correctedSizes[frame][idx] = size;

    if (const LayoutRect *rect = (isNew || size == 0) ? layout.find(what) : nullptr)
    {
      if (!inGraph)
      {
        departed[frame] = *rect;
        departedClass = get_class_of(layout, what);
      }
      if (isRectOnChip(what, ctx))
        onChipRoomFreed = true;
      remove_rect(layout, what, changedPlacements);
    }
    if (size == 0)
      continue;

    const WantedRect want = natural.untracked && naturalOnChip
                              ? leaveOnChip(what, ctx)
                              : makeWantedRect(what, keepPromotedClass(what, natural, size, ctx), ctx);
    updateRect(what, want, preservation, to_be_placed, stripped, evicted, ctx);
  }
  if (!inGraph && departed[0].size + departed[1].size > 0)
  {
    const auto footprint = measure_departed_footprint(departed, ctx.timeline);
    noteDeparture(idx, departedClass, footprint.size, footprint.align, ctx.frameIndex);
  }
}

void ResourceScheduler::updateLayout(const SchedulingContext &ctx, const ResourceSetFmem &revisit, const Preservation &preservation,
  WantedRectsFmem &to_be_placed, FrameResourceSet &stripped, FrameResourceSet &evicted)
{
  TIME_PROFILE(updateLayout);

  for (const auto idx : revisit.trueKeys())
    updateResource(idx, ctx, preservation, to_be_placed, stripped, evicted);

  for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    for (const auto idx : evicted.perFrame[frame].trueKeys())
    {
      const FrameResource neighbour{
        .resIdx = idx,
        .frame = frame,
      };
      if (!layout.has(neighbour))
        continue;
      G_ASSERT(!preservation.covers(layout, neighbour));
      const auto wasAt = layout.where[neighbour];
      stripTo(convert_to_wanted(*layout.find(neighbour), layout.heaps[wasAt.heap].heapClass, wasAt), to_be_placed, stripped, ctx);
    }
}

uint64_t ResourceScheduler::reservedOf(HeapClass heap_class) const
{
  uint64_t result = 0;
  for (const auto heapIdx : layout.heapsOf(heap_class))
    result += layout.heaps[heapIdx].reservedSize;
  return result;
}

static ReservedSizesFmem collect_reserved_sizes(const Layout &layout)
{
  ReservedSizesFmem result(layout.heaps.size(), 0);
  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
    result[heapIdx] = heap.reservedSize;
  return result;
}

// constraint: a relocation leaves the heaps packed anew out, the allocator
// copies from their old objects
static ProposedHeapsFmem propose_heaps_chosen_last(const Layout &layout, HeapClass heap_class, const Preservation &preservation,
  bool size_to_content, const HeapLimits &limits, const HeapsToDefragment &chosen, bool relocate)
{
  auto result = propose_heaps_of(layout, heap_class, preservation, size_to_content, limits);
  size_t notChosen = 0;
  for (size_t i = 0; i < result.size(); ++i)
    if (!chosen.includes(result[i].heap))
      eastl::rotate(result.begin() + notChosen++, result.begin() + i, result.begin() + i + 1);
  if (relocate)
    result.resize(notChosen);
  return result;
}

static void fail_on_unplaced(const UnplacedIndicesFmem &unplaced, eastl::span<const WantedRect> wanted,
  const intermediate::Graph &graph)
{
  for (const auto item : unplaced)
    logerr("daFG: resource '%s' does not fit into any heap of its group", graph.resourceNames[wanted[item].what.resIdx].c_str());
  if (!unplaced.empty())
    DAG_FATAL("daFG: the resource packer failed to schedule every resource, rendering would crash");
}

static void retire_heaps(ProposedHeapsFmem &proposal, const Layout &layout, HeapClass heap_class, const HeapsToDefragment &chosen)
{
  for (const auto heapIdx : layout.heapsOf(heap_class))
    if (chosen.includes(heapIdx))
      proposal.push_back(ProposedHeap{
        .reservedSize = 0,
        .heap = heapIdx,
      });
}

static void reserve_room_for_ghosts(ProposedHeapsFmem &proposal, uint64_t ghost, const HeapLimits &limits,
  const HeapsToDefragment &chosen)
{
  ProposedHeap *room = nullptr;
  for (auto &heap : proposal)
    if (!heap.pinned && (heap.heap == HeapIndex::Invalid || chosen.includes(heap.heap)) &&
        (room == nullptr || heap.reservedSize > room->reservedSize))
      room = &heap;
  if (room != nullptr)
    room->reservedSize = eastl::min(room->reservedSize + ghost, eastl::max(compute_heap_size_limit(limits, 0), room->reservedSize));
}

static bool choose_heaps_to_defragment(const DefragmentationState &state, uint64_t reserved, uint64_t ghost, uint64_t free_budget,
  bool relocate, const HeapPolicySettings &policy, HeapsToDefragment &heaps)
{
  heaps.chosen = {};
  heaps.everyHeap = true;
  if (!is_class_wasteful(state, reserved, heaps.waste, relocate, policy, true))
    return false;
  heaps.chosen = select_heaps_to_defragment(heaps.waste, reserved, relocate, policy);
  heaps.everyHeap = false;
  if (!heaps.chosen.empty() &&
      is_burst_affordable(measure_defragmentation_burst(heaps.waste, heaps.chosen, relocate) + ghost, free_budget, policy))
    return true;
  heaps.chosen = {};
  heaps.everyHeap = true;
  return false;
}

DefragmentationDecision ResourceScheduler::decideDefragmentation(HeapClass heap_class, DefragmentationState &state,
  const HeapGroupInfo &info, size_t rects_in_layout, size_t to_be_placed, uint64_t reserved, uint64_t ghost, bool defragment_all,
  const SchedulingContext &ctx, const Preservation &preservation, HeapsToDefragment &heaps_to_defragment) const
{
  const HeapLimits &limits = info.limits;
  const HeapPolicySettings &policy = info.policy;
  const bool classEmpty = rects_in_layout == 0 && to_be_placed == 0;
  DefragmentationDecision decision{};
  if (state.pending.defragment && (ctx.lightFrame || is_pending_overdue(state, policy, ctx.frameIndex)))
  {
    decision = disarm_defragmentation(state);
    if (decision.sizeToContent && rects_in_layout > 0)
    {
      auto &waste = heaps_to_defragment.waste;
      waste = measure_recoverable_waste(layout, heap_class, preservation, limits.resizeKeepsContents);
      reserve_for_ghosts(waste, ghost);
      decision.relocate = decision.relocate && is_relocation_worth_it(waste, policy);
      const bool budgeted = heap_class.group != CPU_HEAP_GROUP && !limits.fixedSize && !ctx.propertyProvider.doHeapsCountObjects();
      const uint64_t freeBudget = budgeted ? ctx.propertyProvider.getFreeMemoryBudget() : UNKNOWN_MEMORY_BUDGET;
      bool chosen = choose_heaps_to_defragment(state, reserved, ghost, freeBudget, decision.relocate, policy, heaps_to_defragment);
      if (!chosen && decision.relocate)
      {
        decision.relocate = false;
        chosen = choose_heaps_to_defragment(state, reserved, ghost, freeBudget, false, policy, heaps_to_defragment);
      }
      if (!chosen)
        decision = {};
    }
  }
  if (defragment_all || (classEmpty && !limits.fixedSize && reserved > 0 && should_release_empty_class(classEmpty, ghost)))
  {
    decision.defragment = true;
    decision.sizeToContent = true;
  }
  decision.defragment = decision.defragment || should_defragment_on_placed_anew(rects_in_layout, to_be_placed, policy);
  return decision;
}

void ResourceScheduler::settleDefragmentationState(HeapClass heap_class, DefragmentationState &state,
  const DefragmentationDecision &decision, uint64_t ghost, const HeapLimits &limits, const Preservation &preservation)
{
  if (decision.defragment && decision.sizeToContent)
  {
    auto waste = measure_recoverable_waste(layout, heap_class, preservation, limits.resizeKeepsContents);
    reserve_for_ghosts(waste, ghost);
    state.recoverableAfterDefragmentation = waste.recoverable(decision.relocate);
  }
  if (reservedOf(heap_class) == 0)
  {
    disarm_defragmentation(state);
    state.wastefulSinceFrame = -1;
  }
}

void ResourceScheduler::packClass(HeapClass heap_class, eastl::span<const WantedRect> queued, const SchedulingContext &ctx,
  bool defragment_all, const Preservation &preservation, FrameResourceSet &relocating)
{
  TIME_PROFILE(packClass);
  auto &info = getGroupInfo(heap_class.group, ctx.propertyProvider);
  const HeapLimits limits = info.limits;
  auto &state = info.defragmentation[heap_class.untracked];

  const size_t rectsInLayout = count_rects(layout, heap_class);
  const uint64_t reserved = reservedOf(heap_class);
  const uint64_t ghost = ghostBytes(heap_class);
  HeapsToDefragment heapsToDefragment;
  DefragmentationDecision decision = decideDefragmentation(heap_class, state, info, rectsInLayout, queued.size(), reserved, ghost,
    defragment_all, ctx, preservation, heapsToDefragment);
  if (!decision.defragment && queued.empty())
    return;

  const bool relocate = decision.relocate;
  const auto packOnce = [&](bool from_zero) {
    WantedRectsFmem withStripped;
    eastl::span<const WantedRect> wanted = queued;
    if (from_zero)
    {
      withStripped.reserve(queued.size() + rectsInLayout);
      withStripped.assign(queued.begin(), queued.end());
      strip_class(layout, heap_class, withStripped, [&](HeapIndex heap, const LayoutRect &rect) {
        if (!heapsToDefragment.includes(heap))
          return false;
        const bool preserved = preservation.covers(layout, rect.what);
        if (preserved && relocate)
          relocating.insert(rect.what);
        return relocate || !preserved;
      });
      wanted = withStripped;
    }
    const auto heaps = propose_heaps_chosen_last(layout, heap_class, preservation, from_zero && decision.sizeToContent, limits,
      heapsToDefragment, relocate);
    ProposedPacking packing = try_pack_into_layout(layout, heaps, wanted, limits, ctx.timeline, ctx.boundary);
    // A rect does not fit around the kept rects. Pack the class from zero.
    if (!from_zero && !packing.unplaced.empty())
      return false;
    fail_on_unplaced(packing.unplaced, wanted, ctx.graph);

    if (relocate)
      retire_heaps(packing.heaps, layout, heap_class, heapsToDefragment);
    if (decision.defragment && decision.sizeToContent && !limits.fixedSize && ghost > 0)
      reserve_room_for_ghosts(packing.heaps, ghost, limits, heapsToDefragment);
    apply_packing(layout, heap_class, packing, wanted, changedPlacements);
    return true;
  };
  if (decision.defragment || !packOnce(false))
  {
    decision.defragment = true;
    packOnce(true);
  }
  settleDefragmentationState(heap_class, state, decision, ghost, limits, preservation);
}

static void erase_rects_in(WantedRectsFmem &to_be_placed, const FrameResourceSet &set)
{
  to_be_placed.erase(
    eastl::remove_if(to_be_placed.begin(), to_be_placed.end(), [&](const WantedRect &want) { return set.contains(want.what); }),
    to_be_placed.end());
}

// The room was reserved before the on-chip pass. The queue is no longer the
// newest frame memory block, so it must not grow.
static void requeue(WantedRectsFmem &to_be_placed, const WantedRect &want)
{
  G_ASSERT(to_be_placed.size() < to_be_placed.capacity());
  to_be_placed.push_back(want);
}

void ResourceScheduler::splitOnChipQueue(HeapClass on_chip, WantedRectsFmem &to_be_placed, WantedRectsFmem &natives,
  WantedRectsFmem &guests) const
{
  size_t kept = 0;
  for (const auto &want : to_be_placed)
    if (want.heapClass == on_chip)
      (isOnChipNative(on_chip, want.what) ? natives : guests).push_back(want);
    else
      to_be_placed[kept++] = want;
  to_be_placed.resize(kept);
}

void ResourceScheduler::reclaimLeftNatives(HeapClass on_chip, WantedRectsFmem &natives, WantedRectsFmem &to_be_placed,
  const SchedulingContext &ctx)
{
  FrameResourceSet refitted(ctx.graph.resources.totalKeys());
  for (const auto &[idx, onChipProps] : leftNatives)
  {
    if (!ctx.graph.resources.isMapped(idx) || !ctx.graph.resources[idx].isScheduled())
      continue;
    properties[idx] = onChipProps;
    for (uint32_t frame = 0; frame < SCHEDULE_FRAME_WINDOW; ++frame)
    {
      correctedSizes[frame][idx] = corrected_resource_size(onChipProps, ctx.corrections, idx, frame);
      if (correctedSizes[frame][idx] == 0)
        continue;
      const FrameResource what{
        .resIdx = idx,
        .frame = frame,
      };
      AllocationLocation wasAt = NOWHERE;
      if (layout.has(what))
      {
        wasAt = layout.where[what];
        layout.take(what);
      }
      natives.push_back(makeWantedRect(what, on_chip, ctx));
      natives.back().wasAt = wasAt;
      refitted.insert(what);
    }
  }
  leftNatives.clear();
  erase_rects_in(to_be_placed, refitted);
}

void ResourceScheduler::packOnChipNatives(HeapClass on_chip, WantedRectsFmem &natives, WantedRectsFmem &guests, bool from_zero,
  bool &defragment, WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, const Preservation &preservation)
{
  const HeapLimits limits = getGroupInfo(on_chip.group, ctx.propertyProvider).limits;

  const auto packNatives = [&](bool natives_from_zero) {
    if (natives_from_zero)
    {
      strip_class(layout, on_chip, natives, [&](HeapIndex, const LayoutRect &rect) {
        return isOnChipNative(on_chip, rect.what) && !preservation.covers(layout, rect.what);
      });
      strip_class(layout, on_chip, guests, [&](HeapIndex, const LayoutRect &rect) { return !isOnChipNative(on_chip, rect.what); });
      reclaimLeftNatives(on_chip, natives, to_be_placed, ctx);
    }
    const auto heaps = propose_heaps_of(layout, on_chip, preservation, natives_from_zero, limits);
    ProposedPacking packing = try_pack_into_layout(layout, heaps, natives, limits, ctx.timeline, ctx.boundary);
    if (!packing.unplaced.empty() && !defragment)
    {
      defragment = true;
      if (!natives_from_zero)
        return false;
    }

    apply_packing(layout, on_chip, packing, natives, changedPlacements);
    for (const auto item : packing.unplaced)
    {
      auto left = leaveOnChip(natives[item].what, ctx);
      left.wasAt = natives[item].wasAt;
      requeue(to_be_placed, left);
    }
    return true;
  };
  if (!packNatives(from_zero))
    packNatives(true);
}

bool ResourceScheduler::promoteGuests(HeapClass on_chip, const WantedRectsFmem &to_be_placed, bool from_layout,
  eastl::span<const WantedRect> guests, WantedRectsFmem &promoted, const SchedulingContext &ctx)
{
  const HeapLimits limits = getGroupInfo(on_chip.group, ctx.propertyProvider).limits;

  const auto promotable = [&](FrameResource what, uint64_t size) {
    return is_promotable(ctx.graph.resources[what.resIdx], size, limits.maxHeapSize);
  };
  const auto isCandidateClass = [on_chip](HeapClass heap_class) { return !(heap_class == on_chip) && !heap_class.untracked; };
  const auto forEachCandidate = [&](const auto &visit) {
    for (const auto &want : to_be_placed)
      if (isCandidateClass(want.heapClass) && promotable(want.what, want.size))
        visit(want);
    if (from_layout)
      for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
        if (isCandidateClass(heap.heapClass))
          for (const auto &rect : heap.rects)
            if (promotable(rect.what, rect.size))
              visit(convert_to_wanted(rect, heap.heapClass,
                AllocationLocation{
                  .heap = heapIdx,
                  .offset = rect.offset,
                }));
  };

  size_t candidateCount = 0;
  forEachCandidate([&](const WantedRect &) { ++candidateCount; });
  if (candidateCount == 0)
    return false;

  promoted.reserve(guests.size() + candidateCount);
  promoted.assign(guests.begin(), guests.end());
  forEachCandidate([&](const WantedRect &want) { promoted.push_back(want); });

  const eastl::span<const WantedRect> candidates(promoted.data() + guests.size(), candidateCount);
  const auto chosen = select_on_chip_candidates(candidates, limits.maxHeapSize, ctx.graph);
  WantedRectsFmem selected;
  selected.reserve(chosen.size());
  for (const auto item : chosen)
    selected.push_back(candidates[item]);
  promoted.resize(guests.size());
  promoted.insert(promoted.end(), selected.begin(), selected.end());
  return true;
}

void ResourceScheduler::packOnChipGuests(HeapClass on_chip, eastl::span<const WantedRect> guests, WantedRectsFmem &to_be_placed,
  const SchedulingContext &ctx, const Preservation &preservation)
{
  const HeapLimits limits = getGroupInfo(on_chip.group, ctx.propertyProvider).limits;

  const auto heaps = propose_heaps_of(layout, on_chip, preservation, false, limits);
  ProposedPacking packing = try_pack_into_layout(layout, heaps, guests, limits, ctx.timeline, ctx.boundary);
  // A guest promoted from the layout is still in its old heap. It leaves that
  // heap only if it got an on-chip place.
  for (const auto &placed : packing.placed)
    if (layout.has(placed.rect.what))
      layout.take(placed.rect.what);
  apply_packing(layout, on_chip, packing, guests, changedPlacements);

  for (const auto item : packing.unplaced)
  {
    const auto &want = guests[item];
    if (!(want.heapClass == on_chip))
      continue;
    const auto left = leaveOnChip(want.what, ctx);
    if (!(left.heapClass == on_chip))
      requeue(to_be_placed, left);
  }
}

void ResourceScheduler::erasePromotedFromOtherClasses(HeapClass on_chip, WantedRectsFmem &to_be_placed, size_t res_count) const
{
  FrameResourceSet promoted(res_count);
  for (const auto heapIdx : layout.heapsOf(on_chip))
    for (const auto &rect : layout.heaps[heapIdx].rects)
      if (!isOnChipNative(on_chip, rect.what))
        promoted.insert(rect.what);
  erase_rects_in(to_be_placed, promoted);
}

void ResourceScheduler::packOnChip(HeapClass on_chip, WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, bool defragment_all,
  const Preservation &preservation)
{
  TIME_PROFILE(packOnChip);

  const HeapPolicySettings policy = getGroupInfo(on_chip.group, ctx.propertyProvider).policy;
  const auto resCount = ctx.graph.resources.totalKeys();
  const bool fillOnChip = fill_dedicated_fast_gpu_local.get();

  size_t queuedNatives = 0;
  size_t queuedGuests = 0;
  bool hasGuestCandidates = false;
  for (const auto &want : to_be_placed)
    if (want.heapClass == on_chip)
      ++(isOnChipNative(on_chip, want.what) ? queuedNatives : queuedGuests);
    else if (fillOnChip && !want.heapClass.untracked)
      hasGuestCandidates = true;
  hasGuestCandidates = hasGuestCandidates || queuedGuests > 0;

  const size_t inLayout = count_rects(layout, on_chip);
  bool defragment = defragment_all || should_defragment_on_placed_anew(inLayout, queuedNatives + queuedGuests, policy);
  const bool nativesFromZero = defragment || (onChipRoomFreed && !leftNatives.empty());
  onChipRoomFreed = false;
  if (!nativesFromZero && queuedNatives == 0 && !hasGuestCandidates)
    return;

  // Rects that do not fit the on-chip heap go back to the queue. We reserve
  // room for them now, while the queue still grows in place.
  const size_t leftFrames = SCHEDULE_FRAME_WINDOW * leftNatives.size();
  to_be_placed.reserve(to_be_placed.size() + queuedNatives + queuedGuests + inLayout + leftFrames);
  WantedRectsFmem natives;
  WantedRectsFmem guests;
  natives.reserve(queuedNatives + inLayout + leftFrames);
  guests.reserve(queuedGuests + inLayout);
  splitOnChipQueue(on_chip, to_be_placed, natives, guests);

  packOnChipNatives(on_chip, natives, guests, nativesFromZero, defragment, to_be_placed, ctx, preservation);

  WantedRectsFmem promoted;
  eastl::span<const WantedRect> allGuests = guests;
  if (fillOnChip && promoteGuests(on_chip, to_be_placed, defragment, guests, promoted, ctx))
    allGuests = promoted;
  packOnChipGuests(on_chip, allGuests, to_be_placed, ctx, preservation);
  erasePromotedFromOtherClasses(on_chip, to_be_placed, resCount);
}

static eastl::span<const WantedRect> find_queued(const WantedRectsFmem &to_be_placed, HeapClass heap_class)
{
  const auto [first, last] = eastl::equal_range(to_be_placed.begin(), to_be_placed.end(), heap_class, QueuedClassLess{});
  return {first, static_cast<size_t>(last - first)};
}

void ResourceScheduler::packAllClasses(WantedRectsFmem &to_be_placed, const SchedulingContext &ctx, bool defragment_all,
  const Preservation &preservation, FrameResourceSet &relocating)
{
  const auto onChip = findOnChipClass();
  if (onChip)
    packOnChip(*onChip, to_be_placed, ctx, defragment_all, preservation);

  eastl::sort(to_be_placed.begin(), to_be_placed.end(), compare_by_class_then_resource);
  const auto isOnChip = [&](HeapClass heap_class) { return onChip && heap_class == *onChip; };

  // A class with heaps can need a defragmentation with nothing queued. So the
  // first pass packs every class of the layout. The second pass packs the
  // classes that get their first heap.
  for (size_t i = 0; i < layout.heapsOfClass.size(); ++i)
  {
    const auto &[heapClass, heaps] = *(layout.heapsOfClass.begin() + i);
    const auto queued = find_queued(to_be_placed, heapClass);
    if (!isOnChip(heapClass) && !(heaps.empty() && queued.empty()))
      packClass(heapClass, queued, ctx, defragment_all, preservation, relocating);
  }
  for (size_t first = 0; first < to_be_placed.size();)
  {
    const HeapClass heapClass = to_be_placed[first].heapClass;
    const auto queued = find_queued(to_be_placed, heapClass);
    first += queued.size();
    if (!isOnChip(heapClass) && layout.heapsOfClass.find(heapClass) == layout.heapsOfClass.end())
      packClass(heapClass, queued, ctx, defragment_all, preservation, relocating);
  }
}

static bool has_heap_changed_size(const Layout &layout, HeapIndex heap_idx, const ReservedSizesFmem &reserved_before)
{
  return !(reserved_before.isMapped(heap_idx) && reserved_before[heap_idx] == layout.heaps[heap_idx].reservedSize);
}

void ResourceScheduler::markPreserved(const SchedulingContext &ctx, const Preservation &preservation,
  const ReservedSizesFmem &reserved_before, const FrameResourceSet &stripped, const FrameResourceSet &relocating)
{
  TIME_PROFILE(markPreserved);

  const auto resCount = ctx.graph.resources.totalKeys();
  for (auto &perFrame : preservedResources)
    perFrame.assign(resCount, false);
  relocatedResources.assign(resCount, false);
  const uint32_t prevFrame = static_cast<uint32_t>(ctx.prevFrame);

  G_ASSERT_RETURN(preservation.spanningBefore != nullptr, );
  for (const auto idx : preservation.spanningBefore->trueKeys())
  {
    const FrameResource what{
      .resIdx = idx,
      .frame = prevFrame,
    };
    if (!ctx.graph.resources.isMapped(idx) || !preservation.covers(layout, what) || stripped.contains(what))
      continue;
    const auto &res = ctx.graph.resources[idx];
    if (!res.isScheduled() || res.asScheduled().history == History::No)
      continue;

    const auto heapIdx = layout.where[what].heap;
    if (heapIdx == HeapIndex::Invalid)
      continue;
    const auto heapClass = layout.heaps[heapIdx].heapClass;
    const bool relocatingNow = relocating.contains(what);
    const bool moved = changedPlacements[prevFrame].test(idx, false);
    const bool heapRecreated = !getGroupInfo(heapClass.group, ctx.propertyProvider).limits.resizeKeepsContents &&
                               has_heap_changed_size(layout, heapIdx, reserved_before);
    if ((moved || heapRecreated) && !relocatingNow)
    {
      G_ASSERTF(!heapRecreated, "daFG: resource '%s' is preserved, but the heap it sits in changed its size",
        ctx.graph.resourceNames[idx].c_str());
      continue;
    }
    if (moved || heapRecreated)
      relocatedResources.set(idx, true);
    preservedResources[prevFrame].set(idx, true);
  }
}

bool ResourceScheduler::isStateValid() const
{
  ResourceSetFmem listed(slots.totalKeys(), false);
  for (const auto &ghost : ghosts)
  {
    if (ghost.size == 0)
      return false;
    if (ghost.slot == intermediate::RESOURCE_NOT_MAPPED)
      continue;
    if (!slots.isMapped(ghost.slot) || listed.test(ghost.slot, false))
      return false;
    listed.set(ghost.slot, true);
  }
  bool valid = true;
  for_each_class(groups, [&](HeapClass heapClass, const HeapGroupInfo &, const DefragmentationState &state) {
    if (state.pending.defragment && reservedOf(heapClass) == 0)
      valid = false;
  });
  return valid;
}

void ResourceScheduler::dumpLayout(const SchedulingContext &ctx) const
{
  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
  {
    debug("daFG: dump of resources in heap %d (%d bytes reserved)", eastl::to_underlying(heapIdx), heap.reservedSize);
    debug("daFG: name\tframe\tarea (KB)\tstart\tend\tsize\toffset");
    for (const auto &rect : heap.rects)
    {
      const uint32_t start = ctx.timeline.moment(rect.lifetime.firstUse);
      const uint32_t end = ctx.timeline.moment(rect.lifetime.release);
      debug("daFG: %s\t%d\t%d\t%d\t%d\t%d\t%d", ctx.graph.resourceNames[rect.what.resIdx].c_str(), rect.what.frame,
        (rect.size * ctx.timeline.duration(rect.lifetime)) >> 10, start, end, rect.size, rect.offset);
    }
  }
}

void ResourceScheduler::logHeapStatistics(const Timeline &timeline)
{
  // Don't spam logs, every once in a while is enough.
  const uint32_t frame = dagor_frame_no();
  if (frame - heapStatisticsLastFramePrinted <= 3600)
    return;
  heapStatisticsLastFramePrinted = frame;

  // do NOT replace with eastl::string, it cannot expand inplace
  String buffer(nullptr, framemem_ptr());
  for (auto &&[heapIdx, heap] : layout.heaps.enumerate())
  {
    uint64_t totalMemory = 0;
    uint64_t totalArea = 0;
    for (const auto &rect : heap.rects)
    {
      totalMemory += rect.size;
      totalArea += (rect.size * timeline.duration(rect.lifetime)) >> 10;
    }
    const uint64_t heapArea = (heap.reservedSize * timeline.length()) >> 10;
    const float occupancy = heapArea > 0 ? static_cast<float>(totalArea) / static_cast<float>(heapArea) : 1.f;
    const float aliasingRatio = heap.reservedSize > 0 ? static_cast<float>(totalMemory) / static_cast<float>(heap.reservedSize) : 1.f;
    buffer.aprintf(0, "%d\t%.3f\t%.3f\n", eastl::to_underlying(heapIdx), occupancy, aliasingRatio);
  }
  debug("daFG: heap schedules are packed:\nidx\toccupancy\taliasing ratio\n%s", buffer.c_str());
}

void ResourceScheduler::computeSchedule(int prev_frame, SchedulingContext ctx)
{
  TIME_PROFILE(computeSchedule);
  FRAMEMEM_VALIDATE;

  ctx.timeline = make_timeline(ctx.graph);
  ctx.boundary = ((prev_frame + 1) % SCHEDULE_FRAME_WINDOW) * ctx.timeline.timepointsPerFrame;
  ctx.prevFrame = prev_frame;

  for (auto &[group, info] : groups)
    refreshGroupInfo(info, group, ctx.propertyProvider);

  const size_t resCount = ctx.graph.resources.totalKeys();
  const size_t oldCount = properties.totalKeys();

  for (auto &perFrame : changedPlacements)
    perFrame.assign(resCount, false);
  keepFrames.assign(resCount, 0);
  rekeyForPairing(ctx);
  layout.reserveResources(resCount);
  const ResourceSetFmem spanningBefore(layout.spanning[prev_frame]);
  const Preservation preservation{
    .prevFrame = static_cast<uint32_t>(prev_frame),
    .historyInvalid = historyInvalid,
    .historyInvalidFor = historyInvalidFor,
    .spanningBefore = &spanningBefore,
  };
  const ReservedSizesFmem reservedBefore = collect_reserved_sizes(layout);

  FrameResourceSet stripped(resCount);
  FrameResourceSet relocating(resCount);
  const ResourceSetFmem revisit = collectResourcesToRevisit(ctx, oldCount);
  FrameResourceSet evicted(revisit.size());
  // Allocated last, so it grows in place while the layout is updated
  WantedRectsFmem toBePlaced;
  updateLayout(ctx, revisit, preservation, toBePlaced, stripped, evicted);

  const bool defragmentAll = no_aliasing.get() != layoutNoAliasing;
  layoutNoAliasing = no_aliasing.get();

  expireGhosts(ctx.frameIndex);
  packAllClasses(toBePlaced, ctx, defragmentAll, preservation, relocating);

  markPreserved(ctx, preservation, reservedBefore, stripped, relocating);

  historyInvalid = false;
  historyInvalidFor.clear();

  if (validate_layout.get())
  {
    G_ASSERT(is_layout_valid(layout, ctx.timeline));
    G_ASSERT(isStateValid());
  }

  if (dump_resources.get())
    dumpLayout(ctx);
  logHeapStatistics(ctx.timeline);
}

bool ResourceScheduler::isClassDefragmentationDue(HeapClass heap_class, HeapGroupInfo &info, DefragmentationState &state,
  uint32_t frame, bool after_node_drop, bool &ghosts_expired)
{
  if (state.pending.defragment)
  {
    if (reservedOf(heap_class) == 0)
    {
      disarm_defragmentation(state);
      return false;
    }
    return after_node_drop || is_pending_overdue(state, info.policy, frame);
  }
  if (info.limits.fixedSize || !is_defragmentation_check_due(state, info.policy, frame))
    return false;
  const uint64_t reserved = reservedOf(heap_class);
  if (reserved == 0)
    return false;
  const bool empty = count_rects(layout, heap_class) == 0;
  if (!eastl::exchange(ghosts_expired, true))
    expireGhosts(frame);
  const uint64_t ghost = ghostBytes(heap_class);
  DefragmentationDecision decision{};
  if (empty)
  {
    state.lastCheckFrame = frame;
    state.wastefulSinceFrame = -1;
    if (reserved > ghost)
      decision = {
        .defragment = true,
        .sizeToContent = true,
      };
  }
  else
  {
    const Preservation preservation{
      .prevFrame = frame % SCHEDULE_FRAME_WINDOW,
      .historyInvalid = historyInvalid,
      .historyInvalidFor = historyInvalidFor,
    };
    auto waste = measure_recoverable_waste(layout, heap_class, preservation, info.limits.resizeKeepsContents);
    reserve_for_ghosts(waste, ghost);
    decision = decide_defragmentation(state, reserved, waste, info.policy, frame);
  }
  arm_defragmentation(state, decision, frame);
  if (!state.pending.defragment)
    return false;
  return after_node_drop || is_pending_overdue(state, info.policy, frame);
}

bool ResourceScheduler::isDefragmentationDue(uint32_t frame_index, bool after_node_drop)
{
  bool due = false;
  bool ghostsExpired = false;
  for_each_class(groups, [&](HeapClass heapClass, HeapGroupInfo &info, DefragmentationState &state) {
    due = isClassDefragmentationDue(heapClass, info, state, frame_index, after_node_drop, ghostsExpired) || due;
  });
  return due;
}

bool ResourceScheduler::isResourcePreserved(int frame, intermediate::ResourceIndex res_idx) const
{
  G_ASSERT(frame < preservedResources.size());
  return preservedResources[frame].test(res_idx, false);
}

} // namespace dafg
