// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_debug.h"
#include "shaders/dag_shaderVar.h"

#if DAGOR_DBGLEVEL > 0

#include "bvh_context.h"
#include <bvh/bvh_processors.h>
#include <shaders/dag_computeShaders.h>
#include <perfMon/dag_statDrv.h>
#include <math/integer/dag_IPoint2.h>
#include <imgui/imgui.h>
#include <gui/dag_imgui.h>
#include <gui/dag_imguiUtil.h>
#include <util/dag_console.h>
#include <render/denoiser.h>
#include <imgui/implot.h>
#include <drv/3d/dag_shaderConstants.h>

namespace bvh
{
uint32_t get_scratch_buffers_memory_statistics();
uint32_t get_transform_buffers_memory_statistics();

extern float mip_range;
extern float mip_scale;
extern float max_water_distance;
extern float water_fade_power;
extern float max_water_depth;
extern float rtr_max_water_depth;

} // namespace bvh
namespace bvh::grass
{
void get_memory_statistics(ContextId context_id, int64_t &vb, int64_t &ib, int64_t &blas, int64_t &meta, int64_t &query);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
} // namespace bvh::grass
namespace bvh::gobj
{
void get_memory_statistics(int64_t &meta, int64_t &query);
}
namespace bvh::gpugrass
{
void get_memory_statistics(ContextId context_id, int &gpuGrassCount, int64_t &gpuGrassMemory, int64_t &gpuGrassTexturesMemory);
} // namespace bvh::gpugrass

namespace bvh
{
MemoryStatistics get_memory_statistics(ContextId context_id)
{
  TIME_PROFILE(bvh::get_memory_statistics);

  MemoryStatistics stats = {};

  auto as = [&](auto &a) { return a ? stats.blasCount++, d3d::get_raytrace_acceleration_structure_size(a.get()) : 0; };
  auto bs = [&](auto &b) { return b ? b->getSize() : 0; };

  context_id->getDeathRowStats(stats.deathRowBufferCount, stats.deathRowBufferSize);

  WinAutoLock lock(context_id->tidyUpTreesLock);
  WinAutoLock lock2(context_id->tidyUpSkinsLock);

  auto &ip = ProcessorInstances::getIndexProcessor();
  stats.indexProcessorBufferCount = countof(ip.outputs);
  for (auto &buffer : ip.outputs)
    stats.indexProcessorBufferSize += buffer ? buffer->getSize() : 0;

  for (auto &object : context_id->objects)
  {
    auto &statBucket = stats.meshStats[object.second.tag];
    statBucket.meshBlasSize += as(object.second.blas);
    for (auto &mesh : object.second.meshes)
    {
      statBucket.meshCount++;
      statBucket.meshVBSize += bs(mesh.geometry.processedVertexBuffer);
      statBucket.meshVBSize += context_id->getSourceBufferSize(mesh.geometry.heapIndex, mesh.geometry.bufferRegion);
      statBucket.meshVBSize += bs(mesh.ahsVertices);
    }
  }
  for (auto &object : context_id->impostors)
  {
    auto &statBucket = stats.meshStats["impostor"];
    statBucket.meshBlasSize += as(object.second.blas);
    for (auto &mesh : object.second.meshes)
    {
      statBucket.meshCount++;
      statBucket.meshVBSize += bs(mesh.geometry.processedVertexBuffer);
      statBucket.meshVBSize += context_id->getSourceBufferSize(mesh.geometry.heapIndex, mesh.geometry.bufferRegion);
    }
  }

  for (auto &uu : context_id->uniqueSkinBuffers)
    for (auto &u : uu.second.elems)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.size;
    }

  for (auto &uu : context_id->freeUniqueSkinBLASes)
    for (auto &blas : uu.second.blases)
    {
      stats.skinCacheCount++;
      stats.skinCacheBLASSize += as(blas);
    }

  for (auto &uu : context_id->uniqueHeliRotorBuffers)
    for (auto &u : uu.second)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.size;
    }

  for (auto &uu : context_id->uniqueDeformedBuffers)
    for (auto &u : uu.second)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.size;
    }

  for (auto &u : context_id->uniqueSplinegenBuffers)
  {
    stats.splineGenCount++;
    stats.splineGenBLASSize += as(u.second.blas);
    stats.splineGenVBSize += u.second.buffer.size;
  }

  for (auto &lod : context_id->uniqueTreeBuffers)
    for (auto &uu : lod)
      for (auto &u : uu.second.elems)
      {
        stats.treeCount++;
        stats.treeBLASSize += as(u.second.blas);
        stats.treeVBSize += u.second.buffer.size;
      }
  for (auto &lod : context_id->uniqueRiExtraTreeBuffers)
    for (auto &uu : lod)
      for (auto &u : uu.second.elems)
      {
        stats.treeRiExCount++;
        stats.treeRiExBLASSize += as(u.second.blas);
        stats.treeRiExVBSize += u.second.buffer.size;
      }
  for (auto &uu : context_id->uniqueRiExtraFlagBuffers)
    for (auto &u : uu.second)
    {
      stats.flagCount++;
      stats.flagBLASSize += as(u.second.blas);
      stats.flagVBSize += u.second.buffer.size;
    }
  for (auto &uu : context_id->freeUniqueTreeBLASes)
    for (auto &blas : uu.second.blases)
    {
      stats.treeCacheCount++;
      stats.treeCacheBLASSize += as(blas);
    }
  for (auto &uu : context_id->freeUniqueRiExtraTreeBLASes)
    for (auto &blas : uu.second.blases)
    {
      stats.treeRiExCacheCount++;
      stats.treeRiExCacheBLASSize += as(blas);
    }

  for (auto &[allocator, _] : context_id->processBufferAllocator)
  {
    stats.dynamicVBAllocatorSize += allocator.getHeapSize();
    stats.dynamicVBAllocatorFreeSize += allocator.getHeapSize() - allocator.allocated();
  }

  for (auto &[object_id, tree] : context_id->stationaryTreeBuffers)
  {
    stats.stationaryTreeCount++;
    stats.stationaryTreeVBSize += tree.buffer.size;
    stats.stationaryTreeBLASSize += as(tree.blas);
  }

  stats.tlasSize = as(context_id->tlasMain) + as(context_id->tlasTerrain) + as(context_id->tlasParticles);
  stats.tlasUploadSize =
    context_id->tlasUploadMain.totalSize() + context_id->tlasUploadTerrain.totalSize() + bs(context_id->tlasUploadParticles);

  stats.meshMetaSize = context_id->meshMeta.totalSize();
  stats.perInstanceDataSize = context_id->perInstanceData.totalSize();

  stats.terrainBlasSize = 0;
  stats.terrainVBSize = 0;
  for (auto &lod : context_id->terrainLods)
  {
    for (auto &patch : lod.patches)
    {
      stats.terrainBlasSize += as(patch.blas);
      stats.terrainVBSize += bs(patch.vertices);
    }
  }

  stats.cableVBSize = bs(context_id->cableVertices);
  stats.cableIBSize = bs(context_id->cableIndices);
  for (auto &blas : context_id->cableBLASes)
    stats.cableBLASSize += as(blas);

  stats.binSceneCount = context_id->binSceneObjectIds.size();

  stats.waterIBSize = bs(context_id->waterFlatIb) + bs(context_id->waterHeightIb);
  stats.waterBLASSize = 0;
  stats.waterCount = 0;
  stats.waterVBSize = 0;
  for (auto &patch : context_id->water_patches)
  {
    stats.waterBLASSize += as(patch.blas);
    stats.waterVBSize += bs(patch.vertexBuffer);
    stats.waterCount += patch.instances.size();
  }

  bvh::grass::get_memory_statistics(context_id, stats.grassVBSize, stats.grassIBSize, stats.grassBlasSize, stats.grassMetaSize,
    stats.grassQuerySize);
  bvh::gobj::get_memory_statistics(stats.gobjMetaSize, stats.gobjQuerySize);
  bvh::gpugrass::get_memory_statistics(context_id, stats.gpuGrassCount, stats.gpuGrassMemory, stats.gpuGrassTexturesMemory);

  stats.atmosphereTextureSize = context_id->atmosphereTexture ? context_id->atmosphereTexture->getSize() : 0;
  stats.scratchBuffersSize = bvh::get_scratch_buffers_memory_statistics();
  stats.transformBuffersSize = bvh::get_transform_buffers_memory_statistics();

  for (auto &compaction : context_id->blasCompactions)
  {
    if (compaction.has_value())
    {
      stats.compactionSize += as(compaction->compactedBlas);
    }
  }

  stats.compactionCount = (int)context_id->blasCompactionsAccel.size();
  stats.compactionSizeBufferSize = bs(context_id->compactedSizeBuffer);

  int64_t meshMemSize = 0;
  for (auto &[_, bucket] : stats.meshStats)
    meshMemSize += bucket.meshBlasSize + bucket.meshVBSize;

  stats.totalMemory =
    stats.tlasSize + stats.tlasUploadSize + stats.scratchBuffersSize + stats.transformBuffersSize + stats.meshMetaSize + meshMemSize +
    stats.skinBLASSize + stats.treeBLASSize + stats.treeCacheBLASSize + stats.terrainBlasSize + stats.terrainVBSize +
    stats.grassBlasSize + stats.grassVBSize + stats.grassIBSize + stats.grassMetaSize + stats.grassQuerySize + stats.cableBLASSize +
    stats.cableVBSize + stats.cableIBSize + stats.waterBLASSize + stats.waterVBSize + stats.waterIBSize + stats.splineGenBLASSize +
    stats.splineGenVBSize + stats.gpuGrassMemory + stats.gpuGrassTexturesMemory + stats.gobjMetaSize + stats.gobjQuerySize +
    stats.perInstanceDataSize + stats.compactionSize + stats.atmosphereTextureSize + stats.dynamicVBAllocatorSize +
    stats.deathRowBufferSize + stats.indexProcessorBufferSize + stats.stationaryTreeBLASSize + stats.stationaryTreeVBSize;

  return stats;
}
} // namespace bvh

static eastl::unordered_set<bvh::ContextId> context_ids;
static bvh::ContextId debugged_context_id = bvh::InvalidContextId;

static bvh::DebugMode debug_mode = bvh::DebugMode::Unknown;

static UniqueTex debugTex;
static UniqueTex intermediateDebugTex;
static eastl::unique_ptr<ComputeShaderElement> debugShader;
static eastl::unique_ptr<ComputeShaderElement> postfxShader;

static int last_available_width = 1;
static int target_width = 1;
static int resolution_change_cooldown = 0;

static bool do_super_sampling = true;
static bool use_atmosphere = true;
static bool detect_identical_geometry = false;

bool bvh_ri_extra_range_enable = false;
float bvh_ri_extra_range = 100;

bool bvh_ri_gen_range_enable = false;
float bvh_ri_gen_range = 100;

bool bvh_dyn_range_enable = false;
float bvh_dyn_range = 100;

bool bvh_gpuobject_enable = true;

bool bvh_grass_enable = true;

bool bvh_particles_enable = true;

bool bvh_cables_enable = true;


bool bvh_splinegen_enable = true;

float intersection_count_threshold = 16.f;

extern int bvh_terrain_lod_count;
extern bool bvh_terrain_lock;

int bvh_memory_pie_chart_merge_threshold = 0;

inline const char *operator!(bvh::DebugMode mode)
{
  switch (mode)
  {
    case bvh::DebugMode::None: return "None";
    case bvh::DebugMode::Lit: return "Lit";
    case bvh::DebugMode::DiffuseColor: return "Diffuse color";
    case bvh::DebugMode::Normal: return "Normal";
    case bvh::DebugMode::Texcoord: return "Texcoord";
    case bvh::DebugMode::SecTexcoord: return "SecTexcoord";
    case bvh::DebugMode::CamoTexcoord: return "CamoTexcoord";
    case bvh::DebugMode::VertexColor: return "Vertex color";
    case bvh::DebugMode::GI: return "GI";
    case bvh::DebugMode::TwoSided: return "Two Sided";
    case bvh::DebugMode::Paint: return "Paint";
    case bvh::DebugMode::IntersectionCount: return "Intersection count";
    case bvh::DebugMode::Instances: return "Instances";
    default: return "Unknown";
  }
}

static void imguiWindow()
{
  if (debugged_context_id == bvh::InvalidContextId)
    return;

  if (debug_mode == bvh::DebugMode::Unknown)
  {
    if (debugged_context_id->name == "GI")
      debug_mode = bvh::DebugMode::GI;
    else
      debug_mode = bvh::DebugMode::Lit;
  }

  Sbuffer *grassInstances = nullptr;
  Sbuffer *grassInstanceCount = nullptr;
  bvh::grass::get_instances(debugged_context_id, grassInstances, grassInstanceCount);

  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
  if (ImGui::BeginCombo("##debugged_context_id", debugged_context_id->name.data(), 0))
  {
    for (auto &context_id : context_ids)
      if (ImGui::Selectable(context_id->name.data(), debugged_context_id == context_id))
        debugged_context_id = context_id;
    ImGui::EndCombo();
  }

  if (ImGui::CollapsingHeader("Memory statistics"))
  {
    auto stats = bvh::get_memory_statistics(debugged_context_id);
    auto mb = [](auto v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), decltype(v)(1))); };

    if (ImGui::CollapsingHeader("Memory Pie Chart"))
    {
      ImGui::SliderInt("Merge Threshold Size (MB)", &bvh_memory_pie_chart_merge_threshold, 0, 100);

      // ImPlot::SetNextAxesLimits(0, 1, 0, 1, ImGuiCond_Always);
      if (ImPlot::BeginPlot("##BvhMemoryPie", NULL, NULL, ImVec2(-1, 0), ImPlotFlags_Equal | ImPlotFlags_NoMouseText,
            ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations))
      {
        struct PlotElement
        {
          String label;
          int size;
        };
        eastl::vector<PlotElement> plotElements;
        int otherSize = 0;
        // Build pie chart data (skip zero-sized categories to keep chart readable)
        auto addElem = [&](const char *label, int64_t size_raw, const char *tag = nullptr) {
          int sizeMB = mb(size_raw);
          if (sizeMB > bvh_memory_pie_chart_merge_threshold)
          {
            if (tag)
              plotElements.push_back(PlotElement{String(0, "%s - %s: %d", tag, label, sizeMB), sizeMB});
            else
              plotElements.push_back(PlotElement{String(0, "%s: %d", label, sizeMB), sizeMB});
          }
          else
            otherSize += sizeMB;
        };
        addElem("TLAS", stats.tlasSize);
        addElem("TLAS Upload", stats.tlasUploadSize);
        addElem("Scratch", stats.scratchBuffersSize);
        addElem("Transform", stats.transformBuffersSize);
        addElem("Mesh Meta", stats.meshMetaSize);
        for (auto &[tag, bucket] : stats.meshStats)
        {
          addElem("Mesh BLAS", bucket.meshBlasSize, tag);
          addElem("Mesh VB", bucket.meshVBSize, tag);
        }
        addElem("Skin BLAS", stats.skinBLASSize);
        addElem("Tree BLAS", stats.treeBLASSize);
        addElem("Tree Cache BLAS", stats.treeCacheBLASSize);
        addElem("Terrain BLAS", stats.terrainBlasSize);
        addElem("Terrain VB", stats.terrainVBSize);
        addElem("Grass BLAS", stats.grassBlasSize);
        addElem("Grass VB", stats.grassVBSize);
        addElem("Grass IB", stats.grassIBSize);
        addElem("Grass Meta", stats.grassMetaSize);
        addElem("Grass Query", stats.grassQuerySize);
        addElem("Cable BLAS", stats.cableBLASSize);
        addElem("Cable VB", stats.cableVBSize);
        addElem("Cable IB", stats.cableIBSize);
        addElem("Water BLAS", stats.waterBLASSize);
        addElem("Water VB", stats.waterVBSize);
        addElem("Water IB", stats.waterIBSize);
        addElem("SplineGen BLAS", stats.splineGenBLASSize);
        addElem("SplineGen VB", stats.splineGenVBSize);
        addElem("GPUGrass Memory", stats.gpuGrassMemory);
        addElem("GPUGrass Textures Memory", stats.gpuGrassTexturesMemory);
        addElem("GPU Obj Meta", stats.gobjMetaSize);
        addElem("GPU Obj Query", stats.gobjQuerySize);
        addElem("Per Instance", stats.perInstanceDataSize);
        addElem("Compaction", stats.compactionSize);
        addElem("Atmosphere", stats.atmosphereTextureSize);
        addElem("Dynamic VB Alloc", stats.dynamicVBAllocatorSize);
        addElem("Death Row", stats.deathRowBufferSize);
        addElem("IndexProcessor", stats.indexProcessorBufferSize);
        addElem("Stat Tree BLAS", stats.stationaryTreeBLASSize);
        addElem("Stat Tree VB", stats.stationaryTreeVBSize);

        if (otherSize > 0)
          plotElements.push_back(PlotElement{String(0, "Other: %d", otherSize), otherSize});

        eastl::vector<const char *> labelVec;
        eastl::vector<int> sizeVec;
        for (auto &elem : plotElements)
        {
          labelVec.push_back(elem.label.data());
          sizeVec.push_back(elem.size);
        }

        ImPlot::PlotPieChart(labelVec.data(), sizeVec.data(), labelVec.size(), 0.6, 0.4, 0.4, "%.0f");
        ImPlot::EndPlot();
      }
    }

    ImGui::Text("TLAS: %d MB - Upload: %d MB", mb(stats.tlasSize), mb(stats.tlasUploadSize));
    ImGui::Text("Scratch space: %d MB", mb(stats.scratchBuffersSize));
    ImGui::Text("Transform buffers: %d MB", mb(stats.transformBuffersSize));
    for (auto &[tag, bucket] : stats.meshStats)
    {
      ImGui::Text("* %s - ", tag ? tag : "untagged");
      ImGui::SameLine();
      ImGui::Text("Mesh count: %d", bucket.meshCount);
      ImGui::SameLine();
      ImGui::Text("Mesh VB: %d MB", mb(bucket.meshVBSize));
      ImGui::SameLine();
      ImGui::Text("Mesh BLAS: %d MB", mb(bucket.meshBlasSize));
    }
    ImGui::Text("Mesh meta: %d MB", mb(stats.meshMetaSize));
    ImGui::Text("Skin count: %d", stats.skinCount);
    ImGui::SameLine();
    ImGui::Text("Skin VB: %d MB", mb(stats.skinVBSize));
    ImGui::SameLine();
    ImGui::Text("Skin BLAS: %d MB", mb(stats.skinBLASSize));
    ImGui::Text("Skin cache count: %d", stats.skinCacheCount);
    ImGui::SameLine();
    ImGui::Text("Skin cache BLAS: %d MB", mb(stats.skinCacheBLASSize));
    ImGui::Text("RiGen Tree count: %d", stats.treeCount);
    ImGui::SameLine();
    ImGui::Text("RiGen Tree VB: %d MB", mb(stats.treeVBSize));
    ImGui::SameLine();
    ImGui::Text("RiGen Tree BLAS: %d MB", mb(stats.treeBLASSize));
    ImGui::Text("Stat tree count: %d", stats.stationaryTreeCount);
    ImGui::SameLine();
    ImGui::Text("Stat tree VB: %d MB", mb(stats.stationaryTreeVBSize));
    ImGui::SameLine();
    ImGui::Text("Stat tree BLAS: %d MB", mb(stats.stationaryTreeBLASSize));
    ImGui::Text("RiGen Tree cache count: %d", stats.treeCacheCount);
    ImGui::SameLine();
    ImGui::Text("RiGen Tree cache BLAS: %d MB", mb(stats.treeCacheBLASSize));
    ImGui::Text("RiEx Tree count: %d", stats.treeRiExCount);
    ImGui::SameLine();
    ImGui::Text("RiEx Tree VB: %d MB", mb(stats.treeRiExVBSize));
    ImGui::SameLine();
    ImGui::Text("RiEx Tree BLAS: %d MB", mb(stats.treeRiExBLASSize));
    ImGui::Text("RiEx Tree cache count: %d", stats.treeRiExCacheCount);
    ImGui::SameLine();
    ImGui::Text("RiEx Tree cache BLAS: %d MB", mb(stats.treeRiExCacheBLASSize));
    ImGui::Text("Flag count: %d", stats.flagCount);
    ImGui::SameLine();
    ImGui::Text("Flag VB: %d MB", mb(stats.flagVBSize));
    ImGui::SameLine();
    ImGui::Text("Flag BLAS: %d MB", mb(stats.flagBLASSize));
    ImGui::Text("Dynamic allocation size: %d MB", mb(stats.dynamicVBAllocatorSize));
    ImGui::SameLine();
    ImGui::Text("free: %d MB", mb(stats.dynamicVBAllocatorFreeSize));
    ImGui::Text("Terrain BLAS: %d MB", mb(stats.terrainBlasSize));
    ImGui::SameLine();
    ImGui::Text("Terrain VB: %d MB", mb(stats.terrainVBSize));
    ImGui::Text("Grass count: %d", grassInstances ? grassInstances->getNumElements() : 0);
    ImGui::SameLine();
    ImGui::Text("BLAS: %d MB", mb(stats.grassBlasSize));
    ImGui::SameLine();
    ImGui::Text("VB: %d MB", mb(stats.grassVBSize));
    ImGui::SameLine();
    ImGui::Text("IB: %d MB", mb(stats.grassIBSize));
    ImGui::SameLine();
    ImGui::Text("Meta: %d MB", mb(stats.grassMetaSize));
    ImGui::SameLine();
    ImGui::Text("Query: %d MB", mb(stats.grassQuerySize));
    ImGui::Text("Cable BLAS: %d MB", mb(stats.cableBLASSize));
    ImGui::SameLine();
    ImGui::Text("Cable VB: %d MB", mb(stats.cableVBSize));
    ImGui::SameLine();
    ImGui::Text("Cable IB: %d MB", mb(stats.cableIBSize));
    ImGui::Text("bin scene instances: %d", stats.binSceneCount);
    ImGui::Text("Water instances: %d", stats.waterCount);
    ImGui::SameLine();
    ImGui::Text("Water BLAS: %d MB", mb(stats.waterBLASSize));
    ImGui::SameLine();
    ImGui::Text("Water VB: %d MB", mb(stats.waterVBSize));
    ImGui::SameLine();
    ImGui::Text("Water IB: %d MB", mb(stats.waterIBSize));
    ImGui::Text("SplineGen count: %d", stats.splineGenCount);
    ImGui::SameLine();
    ImGui::Text("SplineGen VB: %d MB", mb(stats.splineGenVBSize));
    ImGui::SameLine();
    ImGui::Text("SplineGen BLAS: %d MB", mb(stats.splineGenBLASSize));
    ImGui::Text("GPUGrass instances: %d", stats.gpuGrassCount);
    ImGui::SameLine();
    ImGui::Text("GPUGrass memory: %d MB", mb(stats.gpuGrassMemory));
    ImGui::SameLine();
    ImGui::Text("GPUGrass Textures memory: %d MB", mb(stats.gpuGrassTexturesMemory));
    ImGui::Text("GPU object query: %d MB", mb(stats.gobjQuerySize));
    ImGui::Text("Per instance data: %d MB", mb(stats.perInstanceDataSize));
    ImGui::Text("Compaction data: %d MB - Peak: %d MB - Count: %d - SizeBufferSize: %d", mb(stats.compactionSize),
      mb(stats.peakCompactionSize), stats.compactionCount, stats.compactionSizeBufferSize);
    ImGui::Text("Atmosphere texture: %d MB", mb(stats.atmosphereTextureSize));
    ImGui::Text("BLAS count: %d", stats.blasCount);
    ImGui::Text("Death row buffer count: %d", stats.deathRowBufferCount);
    ImGui::SameLine();
    ImGui::Text("buffers size: %d MB", mb(stats.deathRowBufferCount));
    ImGui::Text("IndexProcessor buffer count: %d", stats.indexProcessorBufferCount);
    ImGui::SameLine();
    ImGui::Text("buffers size: %d MB", mb(stats.indexProcessorBufferSize));
    ImGui::Separator();
    ImGui::Text("Total: %d MB", mb(stats.totalMemory));
  }

  ImGui::Text("riGen index type per frame: %d", debugged_context_id->riGenIndexTypePerFrame);
  ImGui::Text("riGen process time: %dus", debugged_context_id->lastRiGenProcessTimeUs);

  ImGui::Checkbox("Enable riExtra range", &bvh_ri_extra_range_enable);
  if (bvh_ri_extra_range_enable)
    ImGui::SliderFloat("riExtra range", &bvh_ri_extra_range, 0, 200);

  ImGui::Checkbox("Enable riGen range", &bvh_ri_gen_range_enable);
  if (bvh_ri_gen_range_enable)
    ImGui::SliderFloat("riGen range", &bvh_ri_gen_range, 0, 200);

  ImGui::Checkbox("Enable dynrend range", &bvh_dyn_range_enable);
  if (bvh_dyn_range_enable)
    ImGui::SliderFloat("Dynrend range", &bvh_dyn_range, 0, 200);

  ImGui::Separator();

  ImGui::SliderFloat("Mip range", &bvh::mip_range, 10, 2000);
  ImGui::SliderFloat("Mip scale", &bvh::mip_scale, 1, 20);

  ImGui::Separator();

  ImGui::Checkbox("Enable GPU objects", &bvh_gpuobject_enable);
  ImGui::Checkbox("Enable grass", &bvh_grass_enable);
  ImGui::Checkbox("Enable particles", &bvh_particles_enable);
  ImGui::Checkbox("Enable cables", &bvh_cables_enable);
  ImGui::Checkbox("Enable splinegen", &bvh_splinegen_enable);

  ImGui::Separator();

  ImGui::Checkbox("Lock terrain", &bvh_terrain_lock);
  ImGui::SliderInt("Terrain lods", &bvh_terrain_lod_count, 1, 6);

  ImGui::Separator();

  ImGui::SliderFloat("Max water distance", &bvh::max_water_distance, 0.1, 50);
  ImGui::SliderFloat("Water fade power", &bvh::water_fade_power, 0, 5);
  ImGui::SliderFloat("Max water depth", &bvh::max_water_depth, 0, 10);
  ImGui::SliderFloat("RTR max water depth", &bvh::rtr_max_water_depth, 0, 5);

  ImGui::Separator();

  ImGuiDagor::EnumCombo("Debug mode", bvh::DebugMode::None, bvh::DebugMode::Instances, debug_mode, &operator!);

  ImGui::Checkbox("Super sampling", &do_super_sampling);
  ImGui::Checkbox("Use atmosphere", &use_atmosphere);
  ImGui::SameLine();
  ImGui::Checkbox("Detect identical geometry", &detect_identical_geometry);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::Button("Make capture"))
    console::command("render.pix_capture_n_frames");

  int availableWidth = max(ImGui::GetContentRegionAvail().x * (do_super_sampling ? 2 : 1), 10.0f);

  if (availableWidth != last_available_width)
    resolution_change_cooldown = debugTex ? 50 : 0;

  if (resolution_change_cooldown > 0)
    resolution_change_cooldown--;
  else
    target_width = availableWidth;

  last_available_width = availableWidth;

  if (debug_mode == bvh::DebugMode::IntersectionCount)
  {
    ImGui::Separator();
    ImGui::SliderFloat("Intersection count threshold", &intersection_count_threshold, 0.f, 256.f);
  }

  if (debug_mode != bvh::DebugMode::None && debugTex)
    ImGuiDagor::Image(debugTex.getTexId(), d3d::get_screen_aspect_ratio());
}

REGISTER_IMGUI_WINDOW("Render", "BVH", imguiWindow);

namespace bvh::debug
{

inline int operator*(bvh::DebugMode mode) { return static_cast<int>(mode); }

void init(ContextId id)
{
  context_ids.insert(id);
  if (context_ids.size() == 1)
    debugged_context_id = id;
}

void teardown(ContextId id)
{
  context_ids.erase(id);

  if (debugged_context_id == id)
  {
    if (context_ids.empty())
      debugged_context_id = bvh::InvalidContextId;
    else
      debugged_context_id = *context_ids.begin();
  }
}

void teardown()
{
  debugShader.reset();
  postfxShader.reset();
  debugTex.close();
  intermediateDebugTex.close();
}

void render_debug_context(ContextId context_id, float min_t)
{
  if (context_id != debugged_context_id)
    return;

  if (!debugged_context_id->tlasMain)
    return;

  if (debug_mode == DebugMode::Unknown || debug_mode == DebugMode::None)
    return;

  if (imgui_get_state() == ImGuiState::OFF)
    return;

  TIME_D3D_PROFILE(bvh_debug);

  if (debugTex)
  {
    TextureInfo ti;
    debugTex->getinfo(ti);
    if (ti.w != target_width)
    {
      debugTex.close();
      intermediateDebugTex.close();
    }
  }

  if (!debugShader)
    debugShader.reset(new_compute_shader("bvh_debug"));

  if (!postfxShader)
    postfxShader.reset(new_compute_shader("bvh_debug_postfx"));

  auto createTargetTex = [](const char *name) {
    float aspect = d3d::get_screen_aspect_ratio();
    UniqueTex tex = dag::create_tex(nullptr, target_width, max(int(target_width / aspect), 1), TEXCF_UNORDERED | TEXFMT_A16B16G16R16F,
      1, name, RESTAG_BVH);
    return tex;
  };

  if (!debugTex)
    debugTex = createTargetTex("bvh_debug_tex");

  if (!intermediateDebugTex)
    intermediateDebugTex = createTargetTex("bvh_intermediate_tex");

  bvh::bind_resources(debugged_context_id, target_width);

  TextureInfo ti;
  debugTex->getinfo(ti);

  static int bvh_debug_target = ::get_shader_variable_id("bvh_debug_target");
  static int bvh_postx_source = ::get_shader_variable_id("bvh_postfx_source");
  static int bvh_debug_mode = ::get_shader_variable_id("bvh_debug_mode");
  static int bvh_debug_use_atmosphere = ::get_shader_variable_id("bvh_debug_use_atmosphere");
  static int bvh_detect_identical_geometryVarId = ::get_shader_variable_id("bvh_detect_identical_geometry");
  static int rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true);
  static int bvh_debug_intersection_count_thresholdVarId = get_shader_variable_id("bvh_debug_intersection_count_threshold", true);
  static int bvh_debug_min_tVarId = get_shader_variable_id("bvh_debug_min_t", true);

  ShaderGlobal::set_texture(bvh_debug_target, debug_mode == DebugMode::Lit ? intermediateDebugTex.getTexId() : debugTex.getTexId());
  ShaderGlobal::set_int(bvh_debug_mode, *debug_mode - *DebugMode::Lit);
  ShaderGlobal::set_int(bvh_debug_use_atmosphere, use_atmosphere ? 1 : 0);
  ShaderGlobal::set_int(bvh_detect_identical_geometryVarId, detect_identical_geometry ? 1 : 0);
  ShaderGlobal::set_int(rtr_shadowVarId, 1);
  ShaderGlobal::set_float(bvh_debug_intersection_count_thresholdVarId, intersection_count_threshold);
  ShaderGlobal::set_float(bvh_debug_min_tVarId, min_t);

  d3d::set_cs_constbuffer_register_count(192);

  debugShader->dispatchThreads(ti.w, ti.h, 1);

  d3d::set_cs_constbuffer_register_count(0);

  if (debug_mode == DebugMode::Lit)
  {
    ShaderGlobal::set_texture(bvh_postx_source, intermediateDebugTex.getTexId());
    ShaderGlobal::set_texture(bvh_debug_target, debugTex.getTexId());

    postfxShader->dispatchThreads(ti.w, ti.h, 1);
  }
}

} // namespace bvh::debug

#else

#include <bvh/bvh.h>

namespace bvh
{
MemoryStatistics get_memory_statistics(ContextId) { return MemoryStatistics{}; }
} // namespace bvh

namespace bvh::debug
{
void init(ContextId) {}
void teardown(ContextId) {}
void render_debug_context(ContextId, float) {}
void teardown() {}
} // namespace bvh::debug

#endif
