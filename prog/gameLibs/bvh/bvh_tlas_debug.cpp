// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_tlas_debug.h"

#if DAGOR_DBGLEVEL > 0

#include "bvh_context.h"
#include "shaders/bvh_validate_tlas.hlsli"
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVar.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_rwResource.h>
#include <3d/dag_lockSbuffer.h>
#include <perfMon/dag_statDrv.h>
#include <imgui/imgui.h>
#include <util/dag_convar.h>
#include <generic/dag_enumerate.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <EASTL/unique_ptr.h>

namespace bvh::ri
{
void collect_staged_blas_addresses(ContextId context_id, dag::Vector<uint64_t> &addresses);
}
namespace bvh::grass
{
void collect_blas_addresses(dag::Vector<uint64_t> &addresses);
}
namespace bvh::fx
{
void collect_blas_addresses(dag::Vector<uint64_t> &addresses);
}
namespace bvh::smoke_tracers
{
void collect_blas_addresses(dag::Vector<uint64_t> &addresses);
}
namespace bvh::gpugrass
{
void collect_blas_addresses(ContextId context_id, dag::Vector<uint64_t> &addresses);
}

CONSOLE_BOOL_VAL("raytracing", bvh_probe_enable, false);
CONSOLE_INT_VAL("raytracing", bvh_probe_tlas, 0, 0, 2);
CONSOLE_INT_VAL("raytracing", bvh_probe_budget, 256, 0, 8192);
CONSOLE_INT_VAL("raytracing", bvh_probe_range_begin, 0, 0, 1 << 22);
CONSOLE_INT_VAL("raytracing", bvh_probe_range_count, 0, 0, 1 << 22);
CONSOLE_INT_VAL("raytracing", bvh_probe_rays, 8, 8, 64);
CONSOLE_FLOAT_VAL_MINMAX("raytracing", bvh_probe_tmax, 5.f, 0.1f, 10000.f);
CONSOLE_BOOL_VAL("raytracing", bvh_probe_serialize, true);
CONSOLE_BOOL_VAL("raytracing", bvh_probe_log_table, false);
CONSOLE_BOOL_VAL("raytracing", bvh_validate_tlas, false);
CONSOLE_BOOL_VAL("raytracing", bvh_validate_tlas_cpu, false);

namespace bvh::debug
{

// Maps an instance index of a TLAS upload buffer back to the subsystem that filled it. The GPU fed
// regions have no CPU side descriptor, so this is the only identification they have.
struct TlasRegions
{
  struct Region
  {
    const char *tag;
    uint32_t start;
    uint32_t count;
    // Only the GPU fed regions have one. Everything past it has to be zeroed by the producer.
    Sbuffer *counter;
  };

  static constexpr int maxRegions = 12;

  Region regions[maxRegions] = {};
  int regionCount = 0;

  void add(const char *tag, uint32_t start, uint32_t count, Sbuffer *counter = nullptr)
  {
    if (count > 0 && regionCount < maxRegions)
      regions[regionCount++] = Region{tag, start, count, counter};
  }

  const char *tagOf(uint32_t instance_index) const
  {
    for (int i = 0; i < regionCount; ++i)
      if (instance_index >= regions[i].start && instance_index - regions[i].start < regions[i].count)
        return regions[i].tag;
    return "unknown";
  }
};

struct TlasRegionTables
{
  TlasRegions main;
  TlasRegions terrain;
  TlasRegions particles;
};

// The order here has to match the order build() copies the instances in, since a region's start is the
// sum of the counts before it. The assert catches that going out of sync.
static TlasRegionTables make_region_tables(const TlasSizes &sizes)
{
  TlasRegionTables tables;

  uint32_t at = 0;
  auto addMain = [&](const char *tag, uint32_t count, Sbuffer *counter = nullptr) {
    tables.main.add(tag, at, count, counter);
    at += count;
  };
  addMain("main:impostor", sizes.impostorCount);
  addMain("main:riExtra", sizes.riExtraCount);
  addMain("main:cpu", sizes.cpuCount);
  addMain("main:grass", sizes.grassCount, sizes.grassCounter);
  addMain("main:gpuObject", sizes.gpuObjectCount, sizes.gpuObjectCounter);
  addMain("main:dagdp", sizes.dagdpCount, sizes.dagdpCounter);
  addMain("main:gpuGrass", sizes.gpuGrassCount, sizes.gpuGrassCounter);
  G_ASSERTF(at == sizes.mainInstanceCount, "BVH: derived TLAS layout covers %u of %u instances", at, sizes.mainInstanceCount);

  tables.terrain.add("terrain", 0, sizes.terrainCount);

  tables.particles.add("particles:fx", 0, sizes.fxCount, sizes.fxCounter);
  tables.particles.add("particles:smokeTracer", sizes.fxCount, sizes.smokeTracerCount, sizes.smokeTracerCounter);

  return tables;
}

static constexpr int maxValidateReports = 1024;
static constexpr uint32_t addressCapacityStep = 4096;
static constexpr int validateReportDwords = BVH_VALIDATE_REPORT_HEADER_DWORDS + maxValidateReports * BVH_VALIDATE_REPORT_DWORDS;
static_assert(TlasRegions::maxRegions == BVH_VALIDATE_MAX_REGIONS);

struct SelectedTlas
{
  Sbuffer *instances = nullptr;
  D3DRESID instancesId = BAD_D3DRESID;
  uint32_t instanceCount = 0;
  const TlasRegions *regions = nullptr; // never null once select_tlas ran
  const char *name = "";
  bool isMain = false;
};

struct ProbeEntry
{
  uint64_t blasAddress; // 0 when only the GPU knows it
  uint32_t instanceIndex;
  const char *region;
};

// Must mirror the record layout in bvh_validate_tlas.hlsli, padding included: the shader writes at a
// fixed dword stride, so a shorter struct would read every record but the first at the wrong offset.
struct ValidateReport
{
  uint32_t instanceIndex;
  uint32_t kind;
  uint32_t blasLo;
  uint32_t blasHi;
  uint32_t instanceIdAndMask;
  uint32_t regionIndex;
  uint32_t sequence;
  uint32_t unused;
};
static_assert(sizeof(ValidateReport) == BVH_VALIDATE_REPORT_DWORDS * sizeof(uint32_t));

static eastl::unique_ptr<ComputeShaderElement> probeShader;
static eastl::unique_ptr<ComputeShaderElement> validateShader;
static eastl::unique_ptr<ComputeShaderElement> tailShader;
static bool probeShaderRequested = false;
static bool validateShaderRequested = false;
static bool tailShaderRequested = false;

static UniqueBuf probeSink;
static dag::Vector<ProbeEntry> probeList;
static dag::Vector<ProbeEntry> probeListScratch;
static dag::Vector<eastl::string> probeNames;
static uint32_t probeCursor = 0;
static int probeListTlas = -1;
static TlasSizes lastTlasSizes;

static UniqueBuf validateReportBuffer;
static UniqueBuf liveBlasBuffer;
static UniqueBuf pooledBlasBuffer;
static dag::Vector<uint64_t> liveBlasAddresses;
static dag::Vector<uint64_t> pooledBlasAddresses;
static bool validateReportPending = false;
static dag::Vector<uint64_t> reportedOffenders;
static dag::Vector<eastl::pair<const char *, uint32_t>> reportedTailRegions;
static bool regionTableLogged = false;

#define BVH_TLAS_DEBUG_VARS           \
  VAR(bvh_probe_instances)            \
  VAR(bvh_probe_sink)                 \
  VAR(bvh_probe_ray_count)            \
  VAR(bvh_probe_tmax)                 \
  VAR(bvh_probe_tlas)                 \
  VAR(bvh_validate_instances)         \
  VAR(bvh_validate_live_blas)         \
  VAR(bvh_validate_pooled_blas)       \
  VAR(bvh_validate_report)            \
  VAR(bvh_validate_tail_counter)      \
  VAR(bvh_validate_instance_count)    \
  VAR(bvh_validate_live_blas_count)   \
  VAR(bvh_validate_pooled_blas_count) \
  VAR(bvh_validate_tail_region_index) \
  VAR(bvh_validate_tail_region_start) \
  VAR(bvh_validate_tail_region_count) \
  VAR(bvh_validate_max_reports)

#define VAR(a) static int a##VarId = -1;
BVH_TLAS_DEBUG_VARS
#undef VAR

static void resolve_shader_vars()
{
  static bool resolved = false;
  if (resolved)
    return;
  resolved = true;
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  BVH_TLAS_DEBUG_VARS
#undef VAR
}

static SelectedTlas select_tlas(ContextId context_id, const TlasSizes &sizes, const TlasRegionTables &regions, int which)
{
  SelectedTlas selected;
  switch (which)
  {
    case 1:
      selected.instances = context_id->tlasUploadTerrain.getBuf();
      selected.instancesId = context_id->tlasUploadTerrain.getBufId();
      selected.instanceCount = context_id->tlasTerrainValid ? sizes.terrainCount : 0;
      selected.regions = &regions.terrain;
      selected.name = "terrain";
      break;
    case 2:
      selected.instances = context_id->tlasUploadParticles.getBuf();
      selected.instancesId = context_id->tlasUploadParticles.getBufId();
      selected.instanceCount = context_id->tlasParticlesValid ? sizes.fxCount + sizes.smokeTracerCount : 0;
      selected.regions = &regions.particles;
      selected.name = "particles";
      break;
    default:
      selected.instances = context_id->tlasUploadMain.getBuf();
      selected.instancesId = context_id->tlasUploadMain.getBufId();
      selected.instanceCount = context_id->tlasMainValid ? sizes.mainInstanceCount : 0;
      selected.regions = &regions.main;
      selected.name = "main";
      selected.isMain = true;
      break;
  }
  return selected;
}

static void collect_blas_addresses(ContextId context_id, dag::Vector<uint64_t> &live, dag::Vector<uint64_t> &pooled)
{
  TIME_PROFILE(bvh_collect_blas_addresses);

  live.clear();
  pooled.clear();

  auto add = [](dag::Vector<uint64_t> &addresses, const UniqueBLAS &blas) {
    if (blas)
      addresses.push_back(blas.getGPUAddress());
  };
  auto addLive = [&](const UniqueBLAS &blas) { add(live, blas); };
  auto addElems = [&addLive](auto &container) {
    for (auto &outer : container)
      for (auto &inner : outer.second)
        addLive(inner.second.blas);
  };
  auto addAgedElems = [&addLive](auto &container) {
    for (auto &outer : container)
      for (auto &inner : outer.second.elems)
        addLive(inner.second.blas);
  };
  // Handing an entry out swaps the BLAS into the live element and leaves an empty handle behind
  // so the entries add() keeps exactly the unclaimed ones, whatever the cursor says.
  auto addPool = [&](auto &pools) {
    for (auto &pool : pools)
      for (auto &blas : pool.second.blases)
        add(pooled, blas);
  };

  {
    Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
    WinAutoLock treesGuard(context_id->tidyUpTreesLock);
    WinAutoLock skinsGuard(context_id->tidyUpSkinsLock);

    for (auto &object : context_id->objects)
      addLive(object.second.blas);
    for (auto &object : context_id->impostors)
      addLive(object.second.blas);

    addAgedElems(context_id->uniqueSkinBuffers);
    addElems(context_id->uniqueHeliRotorBuffers);
    addElems(context_id->uniqueDeformedBuffers);
    addElems(context_id->uniqueRiExtraFlagBuffers);
    for (auto &lod : context_id->uniqueTreeBuffers)
      addAgedElems(lod);
    for (auto &lod : context_id->uniqueRiExtraTreeBuffers)
      addAgedElems(lod);
    for (auto &entry : context_id->uniqueSplinegenBuffers)
      addLive(entry.second.blas);
    for (auto &entry : context_id->stationaryTreeBuffers)
      addLive(entry.second.blas);

    addPool(context_id->freeUniqueTreeBLASes);
    addPool(context_id->freeUniqueRiExtraTreeBLASes);
    addPool(context_id->freeUniqueSkinBLASes);

    // Trees that appeared this frame are referenced by the descriptors but are only merged into the
    // context after the TLAS build, so they have to be gathered from the gather jobs.
    ri::collect_staged_blas_addresses(context_id, live);

    for (auto &lod : context_id->terrainLods)
      for (auto &patch : lod.patches)
        addLive(patch.blas);
    for (auto &blas : context_id->cableBLASes)
      addLive(blas);
    for (auto &patch : context_id->water_patches)
      addLive(patch.blas);

    gpugrass::collect_blas_addresses(context_id, live);

    for (auto &compaction : context_id->blasCompactions)
      if (compaction.has_value())
        addLive(compaction->compactedBlas);
  }

  grass::collect_blas_addresses(live);
  fx::collect_blas_addresses(live);
  smoke_tracers::collect_blas_addresses(live);

  auto sortUnique = [](dag::Vector<uint64_t> &addresses) {
    eastl::sort(addresses.begin(), addresses.end());
    addresses.erase(eastl::unique(addresses.begin(), addresses.end()), addresses.end());
  };
  sortUnique(live);
  sortUnique(pooled);
}

static const char *tag_of_meta_slot(ContextId context_id, uint32_t meta_slot)
{
  Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
  auto find = [&](const ObjectMap &objects) -> const char * {
    for (auto &object : objects)
    {
      const int base = MeshMetaAllocator::decode(object.second.metaAllocId);
      if (base >= 0 && meta_slot >= uint32_t(base) && meta_slot < uint32_t(base) + object.second.meshes.size())
        return object.second.tag ? object.second.tag : "untagged";
    }
    return nullptr;
  };
  if (auto tag = find(context_id->objects))
    return tag;
  if (auto tag = find(context_id->impostors))
    return tag;
  return "<gpu generated or freed>";
}

static bool is_pooled_blas(uint64_t blas)
{
  return eastl::binary_search(pooledBlasAddresses.begin(), pooledBlasAddresses.end(), blas);
}

static bool should_report_offender(uint64_t blas)
{
  if (eastl::find(reportedOffenders.begin(), reportedOffenders.end(), blas) != reportedOffenders.end())
    return false;
  reportedOffenders.push_back(blas);
  return true;
}

// Which producers a scene actually has decides which regions can carry a stale tail at all, and the
// region layout is only known here, inside build().
static void log_region_table(const TlasRegionTables &regions)
{
  auto logTable = [](const char *name, const TlasRegions &table) {
    logdbg("[BVH] TLAS %s regions: %d", name, table.regionCount);
    for (int i = 0; i < table.regionCount; ++i)
      logdbg("[BVH]   %-24s [%u, %u) %s", table.regions[i].tag, table.regions[i].start,
        table.regions[i].start + table.regions[i].count, table.regions[i].counter ? "gpu fed, tail checked" : "cpu filled");
  };
  logTable("main", regions.main);
  logTable("terrain", regions.terrain);
  logTable("particles", regions.particles);
}

// The readback has no fence, so a record can be torn. sequence is the writer's own index plus one,
// which makes a half written or stale record recognizable.
static bool is_valid_report(const SelectedTlas &selected, const ValidateReport &r, uint32_t index)
{
  if (r.sequence != index + 1)
    return false;
  if (r.regionIndex != BVH_VALIDATE_NO_REGION && int(r.regionIndex) >= selected.regions->regionCount)
    return false;
  return true;
}

// The tail check counts stale slots per region and writes only one record for the first of them, so
// the count comes from the counter array and the record supplies the example slot and address.
static void report_stale_tails(const SelectedTlas &selected, const uint32_t *region_counters, const ValidateReport *reports,
  uint32_t count)
{
  for (int regionIndex = 0; regionIndex < selected.regions->regionCount; ++regionIndex)
  {
    const uint32_t slots = region_counters[regionIndex];
    if (slots == 0)
      continue;

    const char *tag = selected.regions->regions[regionIndex].tag;
    uint32_t firstSlot = 0;
    uint64_t firstBlas = 0;
    for (uint32_t i = 0; i < count; ++i)
    {
      const ValidateReport &r = reports[i];
      if (r.kind != BVH_VALIDATE_STALE_TAIL || int(r.regionIndex) != regionIndex || !is_valid_report(selected, r, i))
        continue;
      firstSlot = r.instanceIndex;
      firstBlas = uint64_t(r.blasLo) | (uint64_t(r.blasHi) << 32);
      break;
    }

    // Stale descriptors reaching the TLAS is always an error, but logerr drives visuallog and the
    // netlog handler, so only report when the count for this region changes.
    uint32_t *reported = nullptr;
    for (auto &entry : reportedTailRegions)
      if (entry.first == tag)
        reported = &entry.second;
    if (reported && *reported == slots)
      continue;
    if (reported)
      *reported = slots;
    else
      reportedTailRegions.push_back({tag, slots});

    logerr("[BVH] TLAS %s region '%s' has %u stale instance(s) past its live count, first at slot %u with blas %016llX. The build "
           "consumes the whole region, so these are live instances.",
      selected.name, tag, slots, firstSlot, firstBlas);
  }
}

static void report_validation_results(ContextId context_id, const SelectedTlas &selected)
{
  auto locked = lock_sbuffer<const uint32_t>(validateReportBuffer.getBuf(), 0, 0, VBLOCK_READONLY);
  if (!locked)
    return;

  const uint32_t *report = locked.get();
  const uint32_t reportCount = report[0];
  if (reportCount == 0)
    return;

  const uint32_t shown = min(reportCount, uint32_t(maxValidateReports));
  if (reportCount > shown)
    logerr("[BVH] TLAS %s validation hit the report limit, %u of %u instances recorded", selected.name, shown, reportCount);

  const ValidateReport *reports = (const ValidateReport *)(report + BVH_VALIDATE_REPORT_HEADER_DWORDS);
  report_stale_tails(selected, report + BVH_VALIDATE_REGION_COUNTERS, reports, shown);

  uint32_t malformed = 0;
  for (uint32_t i = 0; i < shown; ++i)
  {
    const ValidateReport &r = reports[i];
    if (!is_valid_report(selected, r, i))
    {
      malformed++;
      continue;
    }
    if (r.kind == BVH_VALIDATE_STALE_TAIL)
      continue;

    const uint64_t blas = uint64_t(r.blasLo) | (uint64_t(r.blasHi) << 32);
    if (!should_report_offender(blas))
      continue;

    const uint32_t metaSlot = r.instanceIdAndMask & 0xFFFFFF;
    const char *region = selected.regions->tagOf(r.instanceIndex);
    const char *tag = tag_of_meta_slot(context_id, metaSlot);

    if (r.kind == BVH_VALIDATE_BAD_TRANSFORM)
      logerr("[BVH] TLAS %s instance %u region '%s' blas %016llX has a broken transform, meta slot %u, tag '%s'", selected.name,
        r.instanceIndex, region, blas, metaSlot, tag);
    else if (r.kind == BVH_VALIDATE_POOLED_BLAS)
      // Allocated but unreferenced, so this renders stale geometry instead of faulting.
      logwarn("[BVH] TLAS %s instance %u region '%s' references recycled blas %016llX, meta slot %u, tag '%s'", selected.name,
        r.instanceIndex, region, blas, metaSlot, tag);
    else
      logerr("[BVH] TLAS %s instance %u region '%s' references blas %016llX which is not live and can fault, meta slot %u, tag '%s'",
        selected.name, r.instanceIndex, region, blas, metaSlot, tag);
  }

  // A torn readback would otherwise surface as a plausible looking finding with a garbled address.
  if (malformed > 0)
    logerr("[BVH] TLAS %s validation dropped %u malformed record(s) of %u, the readback is unreliable", selected.name, malformed,
      shown);
}

static void validate_instances_on_cpu(ContextId context_id, const SelectedTlas &selected)
{
  TIME_PROFILE(bvh_validate_instances_cpu);

  for (auto [index, instance] : enumerate(context_id->instanceDescsCpu))
  {
    const uint64_t blas = instance.blasGpuAddress;
    if (blas == 0)
      continue;
    if (eastl::binary_search(liveBlasAddresses.begin(), liveBlasAddresses.end(), blas))
      continue;
    if (!should_report_offender(blas))
      continue;

    const uint32_t metaSlot = instance.instanceID;
    const char *tag = tag_of_meta_slot(context_id, metaSlot);

    if (is_pooled_blas(blas))
      logwarn("[BVH] CPU instance desc %u of TLAS %s references recycled blas %016llX, meta slot %u, tag '%s'", uint32_t(index),
        selected.name, blas, metaSlot, tag);
    else
      logerr("[BVH] CPU instance desc %u of TLAS %s references blas %016llX which is not live and can fault, meta slot %u, tag '%s'",
        uint32_t(index), selected.name, blas, metaSlot, tag);
  }
}

// The address count moves with streaming, so the capacity is rounded up and only ever grows to keep
// the buffer from being recreated every frame while the scene loads in.
static bool upload_blas_addresses(UniqueBuf &buffer, const dag::Vector<uint64_t> &addresses, const char *name)
{
  const uint32_t count = uint32_t(addresses.size());
  const uint32_t capacity = max((count + addressCapacityStep - 1) / addressCapacityStep * addressCapacityStep, addressCapacityStep);

  if (buffer && buffer->getNumElements() < count)
    buffer.close();
  if (!buffer)
  {
    buffer = dag::buffers::create_persistent_sr_structured(sizeof(uint64_t), capacity, name, d3d::buffers::Init::No, RESTAG_BVH);
    if (!buffer)
      return false;
    logdbg("[BVH] %s sized for %u addresses", name, capacity);
  }

  if (count == 0)
    return true;

  auto upload = lock_sbuffer<uint64_t>(buffer.getBuf(), 0, count, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  if (!upload)
    return false;
  memcpy(upload.get(), addresses.data(), count * sizeof(uint64_t));
  return true;
}

// A GPU fed region is consumed at full capacity, so the producer has to zero everything past its live
// count. One dispatch per region, sized to that region, so its counter can be bound directly instead
// of gathering all counters for a single pass.
static void dispatch_tail_checks(const SelectedTlas &selected)
{
  if (!tailShaderRequested)
  {
    tailShaderRequested = true;
    tailShader.reset(new_compute_shader("bvh_validate_tlas_tail", true));
  }
  if (!tailShader)
    return;

  for (int i = 0; i < selected.regions->regionCount; ++i)
  {
    const TlasRegions::Region &region = selected.regions->regions[i];
    if (!region.counter)
      continue;

    d3d::resource_barrier({region.counter, RB_RO_SRV | RB_STAGE_COMPUTE});

    ShaderGlobal::set_buffer_unsafe(bvh_validate_tail_counterVarId, region.counter);
    ShaderGlobal::set_int(bvh_validate_tail_region_indexVarId, i);
    ShaderGlobal::set_int(bvh_validate_tail_region_startVarId, region.start);
    ShaderGlobal::set_int(bvh_validate_tail_region_countVarId, region.count);

    tailShader->dispatchThreads(region.count, 1, 1);
  }

  ShaderGlobal::set_buffer_unsafe(bvh_validate_tail_counterVarId, nullptr);
}

void validate_tlas_instances(ContextId context_id, const TlasSizes &sizes)
{
  lastTlasSizes = sizes;

  if (!bvh_validate_tlas.get() && !bvh_validate_tlas_cpu.get())
    return;

  const TlasRegionTables regions = make_region_tables(sizes);
  const SelectedTlas selected = select_tlas(context_id, sizes, regions, bvh_probe_tlas.get());
  if (!selected.instances || !selected.instanceCount)
    return;

  TIME_D3D_PROFILE_NAME(bvh_validate_tlas, selected.name);
  DA_PROFILE_TAG(bvh_validate_tlas, "%u instances", selected.instanceCount);

  resolve_shader_vars();

  if (!regionTableLogged)
  {
    regionTableLogged = true;
    log_region_table(regions);
  }

  collect_blas_addresses(context_id, liveBlasAddresses, pooledBlasAddresses);
  if (liveBlasAddresses.empty())
    return;

  if (bvh_validate_tlas_cpu.get())
    validate_instances_on_cpu(context_id, selected);

  if (!bvh_validate_tlas.get())
    return;

  if (!validateShaderRequested)
  {
    validateShaderRequested = true;
    validateShader.reset(new_compute_shader("bvh_validate_tlas", true));
  }
  if (!validateShader)
    return;

  if (!validateReportBuffer)
    validateReportBuffer =
      dag::buffers::create_ua_byte_address_readback(validateReportDwords, "bvh_validate_report", d3d::buffers::Init::Zero, RESTAG_BVH);
  if (!validateReportBuffer)
    return;

  // The report of the previous dispatch is only guaranteed to be on the CPU a frame later.
  if (validateReportPending)
  {
    report_validation_results(context_id, selected);
    validateReportPending = false;
  }

  if (!upload_blas_addresses(liveBlasBuffer, liveBlasAddresses, "bvh_validate_live_blas") ||
      !upload_blas_addresses(pooledBlasBuffer, pooledBlasAddresses, "bvh_validate_pooled_blas"))
    return;

  d3d::zero_rwbufi(validateReportBuffer.getBuf());
  d3d::resource_barrier({validateReportBuffer.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
  d3d::resource_barrier({selected.instances, RB_RO_SRV | RB_STAGE_COMPUTE});

  ShaderGlobal::set_buffer(bvh_validate_instancesVarId, selected.instancesId);
  ShaderGlobal::set_buffer(bvh_validate_live_blasVarId, liveBlasBuffer.getBufId());
  ShaderGlobal::set_buffer(bvh_validate_pooled_blasVarId, pooledBlasBuffer.getBufId());
  ShaderGlobal::set_buffer(bvh_validate_reportVarId, validateReportBuffer.getBufId());
  ShaderGlobal::set_int(bvh_validate_instance_countVarId, selected.instanceCount);
  ShaderGlobal::set_int(bvh_validate_live_blas_countVarId, uint32_t(liveBlasAddresses.size()));
  ShaderGlobal::set_int(bvh_validate_pooled_blas_countVarId, uint32_t(pooledBlasAddresses.size()));
  ShaderGlobal::set_int(bvh_validate_max_reportsVarId, maxValidateReports);

  validateShader->dispatchThreads(selected.instanceCount, 1, 1);

  dispatch_tail_checks(selected);

  ShaderGlobal::set_buffer(bvh_validate_instancesVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(bvh_validate_live_blasVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(bvh_validate_pooled_blasVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(bvh_validate_reportVarId, BAD_D3DRESID);

  validateReportPending = true;
}

// The main TLAS upload buffer starts with the impostor and riExtra descriptors that
// parallel_instance_processing::upload_main_data memcpys in this exact order, followed by
// instanceDescsCpu. All three still hold this frame's data while build() is running.
static Point3 instance_translation(const NativeInstance &instance)
{
#if _TARGET_C2



#else
  return Point3(v_extract_w(instance.transform.row0), v_extract_w(instance.transform.row1), v_extract_w(instance.transform.row2));
#endif
}

template <typename Callback>
static void for_each_cpu_instance(ContextId context_id, Callback callback)
{
  uint32_t index = 0;
  for (auto &instances : context_id->impostorInstances)
    for (auto &instance : instances)
      callback(index++, instance);
  for (auto &instances : context_id->riExtraInstances)
    for (auto &instance : instances)
      callback(index++, instance);
  for (auto &instance : context_id->instanceDescsCpu)
    callback(index++, instance);
}

// One probe per distinct BLAS address wherever the CPU knows the descriptors, using the first
// instance that references it as the ray origin. The GPU fed regions have no CPU side descriptor,
// so they get one probe per instance index instead; the shader reads the origin from the upload
// buffer either way, only the deduplication and the marker name need the address.
static void rebuild_probe_list(ContextId context_id, const SelectedTlas &selected)
{
  dag::Vector<ProbeEntry> &newList = probeListScratch;
  newList.clear();
  newList.reserve(selected.instanceCount);

  uint32_t cpuInstanceCount = 0;
  if (selected.isMain)
  {
    for_each_cpu_instance(context_id, [&](uint32_t index, const NativeInstance &instance) {
      cpuInstanceCount = index + 1;
      if (instance.blasGpuAddress != 0 && index < selected.instanceCount)
        newList.push_back(ProbeEntry{instance.blasGpuAddress, index, selected.regions->tagOf(index)});
    });

    eastl::sort(newList.begin(), newList.end(),
      [](const ProbeEntry &a, const ProbeEntry &b) { return a.blasAddress < b.blasAddress; });
    newList.erase(eastl::unique(newList.begin(), newList.end(),
                    [](const ProbeEntry &a, const ProbeEntry &b) { return a.blasAddress == b.blasAddress; }),
      newList.end());
  }

  for (uint32_t index = min(cpuInstanceCount, selected.instanceCount); index < selected.instanceCount; ++index)
    newList.push_back(ProbeEntry{0, index, selected.regions->tagOf(index)});

  bool changed = probeListTlas != bvh_probe_tlas.get() || newList.size() != probeList.size();
  for (uint32_t i = 0; !changed && i < newList.size(); ++i)
    changed = newList[i].blasAddress != probeList[i].blasAddress || newList[i].instanceIndex != probeList[i].instanceIndex;
  if (!changed)
    return;

  probeListTlas = bvh_probe_tlas.get();
  probeList.swap(newList);

  // The driver keeps only the pointer of a debug marker name and resolves it when a crash dump is
  // written, so the names have to outlive the dispatch that used them.
  probeNames.clear();
  probeNames.reserve(probeList.size());
  for (auto [probeIndex, entry] : enumerate(probeList))
  {
    if (entry.blasAddress)
      probeNames.push_back(eastl::string(eastl::string::CtorSprintf(), "bvh_probe_%s_p%u_i%u_blas%llX", selected.name,
        uint32_t(probeIndex), entry.instanceIndex, entry.blasAddress));
    else
      probeNames.push_back(eastl::string(eastl::string::CtorSprintf(), "bvh_probe_%s_p%u_i%u_%s", selected.name, uint32_t(probeIndex),
        entry.instanceIndex, entry.region));
  }
}

static void log_instance_table(ContextId context_id, const SelectedTlas &selected, const Point3 &camera_pos)
{
  logdbg("[BVH] TLAS %s instance table, %u instances, %u probes", selected.name, selected.instanceCount, uint32_t(probeList.size()));
  if (!selected.isMain)
    return;

  for_each_cpu_instance(context_id, [&](uint32_t index, const NativeInstance &instance) {
    const Point3 position = instance_translation(instance) + camera_pos;
    logdbg("[BVH]   %6u region '%s' blas %016llX meta %u mask %02X flags %02X world %.2f %.2f %.2f tag '%s'", index,
      selected.regions->tagOf(index), instance.blasGpuAddress, instance.instanceID, instance.instanceMask, instance.flags, position.x,
      position.y, position.z, tag_of_meta_slot(context_id, instance.instanceID));
  });
}

void probe_tlas(ContextId context_id, const TlasSizes &sizes, const Point3 &camera_pos)
{
  lastTlasSizes = sizes;

  if (!bvh_probe_enable.get())
    return;

  const TlasRegionTables regions = make_region_tables(sizes);
  const SelectedTlas selected = select_tlas(context_id, sizes, regions, bvh_probe_tlas.get());
  if (!selected.instances || !selected.instanceCount)
    return;

  TIME_D3D_PROFILE(bvh_tlas_probe);

  resolve_shader_vars();

  if (!probeShaderRequested)
  {
    probeShaderRequested = true;
    probeShader.reset(new_compute_shader("bvh_tlas_probe", true));
  }
  if (!probeShader)
    return;

  const uint32_t instanceSize = d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize;
  if (instanceSize != sizeof(HWInstance))
  {
    logerr("[BVH] TLAS probe needs a 64 byte instance descriptor, but this driver uses %u bytes", instanceSize);
    bvh_probe_enable.set(false);
    return;
  }

  if (!probeSink)
    probeSink = dag::buffers::create_ua_byte_address(16, "bvh_probe_sink", RESTAG_BVH);
  if (!probeSink)
    return;

  rebuild_probe_list(context_id, selected);
  if (probeList.empty())
    return;

  if (bvh_probe_log_table.get())
  {
    log_instance_table(context_id, selected, camera_pos);
    bvh_probe_log_table.set(false);
  }

  const uint32_t probeCount = uint32_t(probeList.size());
  const uint32_t rangeBegin = min<uint32_t>(bvh_probe_range_begin.get(), probeCount);
  const uint32_t rangeEnd =
    bvh_probe_range_count.get() ? min<uint32_t>(rangeBegin + bvh_probe_range_count.get(), probeCount) : probeCount;
  if (rangeBegin >= rangeEnd)
    return;

  if (probeCursor < rangeBegin || probeCursor >= rangeEnd)
    probeCursor = rangeBegin;

  const uint32_t rangeSize = rangeEnd - rangeBegin;
  const uint32_t budget = bvh_probe_budget.get() ? min<uint32_t>(bvh_probe_budget.get(), rangeSize) : rangeSize;

  const auto threadGroupSizes = probeShader->getThreadGroupSizes();
  const uint32_t rayPitch = threadGroupSizes[0];
  const uint32_t threadsPerGroup = threadGroupSizes[0] * threadGroupSizes[1];
  const uint32_t rayCount = ((bvh_probe_rays.get() + threadsPerGroup - 1) / threadsPerGroup) * threadsPerGroup;

  // The width only feeds the texture mip bias, which the probe never samples.
  bvh::bind_resources(context_id, 1920);

  d3d::resource_barrier({selected.instances, RB_RO_SRV | RB_STAGE_COMPUTE});

  ShaderGlobal::set_buffer(bvh_probe_instancesVarId, selected.instancesId);
  ShaderGlobal::set_buffer(bvh_probe_sinkVarId, probeSink.getBufId());
  ShaderGlobal::set_int(bvh_probe_ray_countVarId, rayCount);
  ShaderGlobal::set_int(bvh_probe_tlasVarId, bvh_probe_tlas.get());
  ShaderGlobal::set_float(bvh_probe_tmaxVarId, bvh_probe_tmax.get());

  const uint32_t firstProbe = probeCursor;
  for (uint32_t probed = 0; probed < budget; ++probed)
  {
    const uint32_t probeIndex = probeCursor;

    d3d::beginEvent(probeNames[probeIndex].c_str());

    const uint32_t immediateConst[] = {probeList[probeIndex].instanceIndex, rayPitch};
    d3d::set_immediate_const(STAGE_CS, immediateConst, countof(immediateConst));
    probeShader->dispatchThreads(rayPitch, rayCount / rayPitch, 1);

    d3d::endEvent();

    if (bvh_probe_serialize.get())
      d3d::resource_barrier({probeSink.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});

    if (++probeCursor >= rangeEnd)
      probeCursor = rangeBegin;
  }
  d3d::set_immediate_const(STAGE_CS, nullptr, 0);

  ShaderGlobal::set_buffer(bvh_probe_instancesVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(bvh_probe_sinkVarId, BAD_D3DRESID);

  bvh::unbind_resources();

  // The log is the only artifact that survives a hard GPU crash.
  logdbg("[BVH] probed TLAS %s: %u instances, %u probes, dispatched [%u, %u) of [%u, %u)", selected.name, selected.instanceCount,
    probeCount, firstProbe, probeCursor, rangeBegin, rangeEnd);
}

void draw_tlas_debug_imgui()
{
  if (!ImGui::CollapsingHeader("TLAS probe and validation"))
    return;

  static const char *tlasNames[] = {"main", "terrain", "particles"};
  int targetTlas = bvh_probe_tlas.get();
  if (ImGui::Combo("Target TLAS", &targetTlas, tlasNames, countof(tlasNames)))
    bvh_probe_tlas.set(targetTlas);

  ImGui::Text("Instances: main %u, terrain %u, particles %u", lastTlasSizes.mainInstanceCount, lastTlasSizes.terrainCount,
    lastTlasSizes.fxCount + lastTlasSizes.smokeTracerCount);

  ImGui::SeparatorText("Descriptor validation");

  bvh_validate_tlas.imguiWidget("Validate descriptors on GPU");
  bvh_validate_tlas_cpu.imguiWidget("Validate CPU descriptors (control)");

  ImGui::Text("Live BLAS %u, pooled %u, reported %u", uint32_t(liveBlasAddresses.size()), uint32_t(pooledBlasAddresses.size()),
    uint32_t(reportedOffenders.size()));
  ImGui::SameLine();
  // Each offending address is logged once, so the list has to be dropped to see it again.
  if (ImGui::Button("Forget reported"))
  {
    reportedOffenders.clear();
    reportedTailRegions.clear();
  }

  ImGui::SeparatorText("Per BLAS trace probe");

  bvh_probe_enable.imguiWidget("Enable probe");
  bvh_probe_budget.imguiWidget("Dispatches per frame (0 = all)");
  bvh_probe_rays.imguiWidget("Rays per probe");

  // Short rays keep a fault attributable to the probed instance's neighbourhood, so the interesting
  // part of the range is the low end.
  float rayLength = bvh_probe_tmax.get();
  if (ImGui::SliderFloat("Ray length", &rayLength, bvh_probe_tmax.getMin(), bvh_probe_tmax.getMax(), "%.2f",
        ImGuiSliderFlags_Logarithmic))
    bvh_probe_tmax.set(rayLength);

  const int probeCount = int(probeList.size());
  bvh_probe_range_begin.setMinMax(0, max(probeCount - 1, 0));
  bvh_probe_range_count.setMinMax(0, probeCount);
  bvh_probe_range_begin.imguiWidget("Probe range begin");
  bvh_probe_range_count.imguiWidget("Probe range count (0 = to end)");

  bvh_probe_serialize.imguiWidget("Serialize dispatches");

  ImGui::Text("Probes %d, cursor at %u", probeCount, probeCursor);
  ImGui::SameLine();
  if (ImGui::Button("Dump instance table"))
    bvh_probe_log_table.set(true);
}

void tlas_debug_teardown()
{
  probeShader.reset();
  validateShader.reset();
  tailShader.reset();
  probeShaderRequested = false;
  validateShaderRequested = false;
  tailShaderRequested = false;
  probeSink.close();
  probeList.clear();
  probeListScratch.clear();
  probeNames.clear();
  probeListTlas = -1;
  lastTlasSizes = {};
  validateReportBuffer.close();
  liveBlasBuffer.close();
  pooledBlasBuffer.close();
  liveBlasAddresses.clear();
  pooledBlasAddresses.clear();
  validateReportPending = false;
  reportedOffenders.clear();
  reportedTailRegions.clear();
  regionTableLogged = false;
}

} // namespace bvh::debug

#else

namespace bvh::debug
{
void validate_tlas_instances(ContextId, const TlasSizes &) {}
void probe_tlas(ContextId, const TlasSizes &, const Point3 &) {}
void draw_tlas_debug_imgui() {}
void tlas_debug_teardown() {}
} // namespace bvh::debug

#endif
