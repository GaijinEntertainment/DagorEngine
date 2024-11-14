// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bvh_debug.h"

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

namespace bvh
{
uint32_t get_scratch_buffers_memory_statistics();
uint32_t get_transform_buffers_memory_statistics();

extern float mip_range;
extern float mip_scale;
extern float max_water_distance;
extern float water_fade_power;

} // namespace bvh
namespace bvh::grass
{
void get_memory_statistics(int &vb, int &ib, int &blas, int &meta, int &query);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
} // namespace bvh::grass
namespace bvh::gobj
{
void get_memory_statistics(int &meta, int &query);
}

namespace bvh
{
MemoryStatistics get_memory_statistics(ContextId context_id)
{
  MemoryStatistics stats = {};

  auto as = [&](auto &a) { return a ? stats.blasCount++, d3d::get_raytrace_acceleration_structure_size(a.get()) : 0; };
  auto bs = [&](auto &b) { return b ? b->ressize() : 0; };

  context_id->getDeathRowStats(stats.deathRowBufferCount, stats.deathRowBufferSize);

  WinAutoLock lock(context_id->cutdownTreeLock);

  auto &ip = ProcessorInstances::getIndexProcessor();
  stats.indexProcessorBufferCount = countof(ip.outputs);
  for (auto &buffer : ip.outputs)
    stats.indexProcessorBufferSize += buffer ? buffer->ressize() : 0;

  for (auto &mesh : context_id->meshes)
  {
    stats.meshCount++;
    stats.meshBlasSize += as(mesh.second.blas);
    stats.meshVBSize += bs(mesh.second.processedVertices);
    stats.meshIBSize += bs(mesh.second.processedIndices);
  }
  for (auto &mesh : context_id->impostors)
  {
    stats.meshCount++;
    stats.meshBlasSize += as(mesh.second.blas);
    stats.meshVBSize += bs(mesh.second.processedVertices);
    stats.meshIBSize += bs(mesh.second.processedIndices);
  }

  for (auto &uu : context_id->uniqueSkinBuffers)
    for (auto &u : uu.second)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.allocator;
    }

  for (auto &uu : context_id->uniqueHeliRotorBuffers)
    for (auto &u : uu.second)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.allocator;
    }

  for (auto &uu : context_id->uniqueDeformedBuffers)
    for (auto &u : uu.second)
    {
      stats.skinCount++;
      stats.skinBLASSize += as(u.second.blas);
      stats.skinVBSize += u.second.buffer.allocator;
    }

  for (auto &uu : context_id->uniqueTreeBuffers)
    for (auto &u : uu.second.elems)
    {
      stats.treeCount++;
      stats.treeBLASSize += as(u.second.blas);
      stats.treeVBSize += u.second.buffer.allocator;
    }
  for (auto &uu : context_id->uniqueRiExtraTreeBuffers)
    for (auto &u : uu.second)
    {
      stats.treeCount++;
      stats.treeBLASSize += as(u.second.blas);
      stats.treeVBSize += u.second.buffer.allocator;
    }
  for (auto &uu : context_id->uniqueRiExtraFlagBuffers)
    for (auto &u : uu.second)
    {
      stats.treeCount++;
      stats.treeBLASSize += as(u.second.blas);
      stats.treeVBSize += u.second.buffer.allocator;
    }
  for (auto &uu : context_id->freeUniqueTreeBLASes)
    for (auto &blas : uu.second.blases)
    {
      stats.treeCacheCount++;
      stats.treeCacheBLASSize += as(blas);
    }

  for (auto &allocator : context_id->processBufferAllocators)
    for (auto &pool : allocator.second.pools)
      stats.dynamicVBAllocatorSize += pool.buffer->ressize();

  stats.tlasSize = as(context_id->tlasMain) + as(context_id->tlasTerrain) + as(context_id->tlasParticles);
  stats.tlasUploadSize = bs(context_id->tlasUploadMain) + bs(context_id->tlasUploadTerrain) + bs(context_id->tlasUploadParticles);

  stats.meshMetaSize = bs(context_id->meshMeta);
  stats.perInstanceDataSize = bs(context_id->perInstanceData);

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

  bvh::grass::get_memory_statistics(stats.grassVBSize, stats.grassIBSize, stats.grassBlasSize, stats.grassMetaSize,
    stats.grassQuerySize);
  bvh::gobj::get_memory_statistics(stats.gobjMetaSize, stats.gobjQuerySize);
  denoiser::get_memory_stats(stats.demoiserCommonSizeMB, stats.demoiserAoSizeMB, stats.demoiserReflectionSizeMB,
    stats.demoiserTransientSizeMB);

  stats.atmosphereTextureSize = context_id->atmosphereTexture ? context_id->atmosphereTexture->ressize() : 0;
  stats.scratchBuffersSize = bvh::get_scratch_buffers_memory_statistics();
  stats.transformBuffersSize = bvh::get_transform_buffers_memory_statistics();

  for (auto &compaction : context_id->blasCompactions)
  {
    stats.compactionSize += as(compaction.compactedBlas);
    stats.compactionSize += bs(compaction.compactedSize);
  }

  static int peakCompactionSize = 0;
  peakCompactionSize = max(peakCompactionSize, stats.compactionSize);

  stats.totalMemory = stats.tlasSize + stats.tlasUploadSize + stats.scratchBuffersSize + stats.transformBuffersSize +
                      stats.meshMetaSize + stats.meshBlasSize + stats.meshVBSize + stats.meshVBCopySize + stats.meshIBSize +
                      stats.skinBLASSize + stats.treeBLASSize + stats.treeCacheBLASSize + stats.terrainBlasSize + stats.terrainVBSize +
                      stats.grassBlasSize + stats.grassVBSize + stats.grassIBSize + stats.grassMetaSize + stats.grassQuerySize +
                      stats.cableBLASSize + stats.cableVBSize + stats.cableIBSize + stats.gobjMetaSize + stats.gobjQuerySize +
                      stats.perInstanceDataSize + stats.compactionSize + stats.atmosphereTextureSize + stats.dynamicVBAllocatorSize +
                      stats.deathRowBufferSize + stats.indexProcessorBufferSize;

  return stats;
}
} // namespace bvh

static eastl::unordered_set<bvh::ContextId> context_ids;
static bvh::ContextId debugged_context_id = bvh::InvalidContextId;

static bvh::DebugMode debug_mode = bvh::DebugMode::Unknown;

static UniqueTex debugTex;
static eastl::unique_ptr<ComputeShaderElement> debugShader;

static int last_available_width = 1;
static int target_width = 1;
static int resolution_change_cooldown = 0;

static bool do_super_sampling = true;

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

extern int bvh_terrain_lod_count;
extern bool bvh_terrain_lock;

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

  auto mb = [](auto v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), decltype(v)(1))); };

  auto stats = bvh::get_memory_statistics(debugged_context_id);

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

  ImGui::Separator();

  ImGui::Text("Memory statistics");

  ImGui::Text("TLAS: %d MB - Upload: %d MB", mb(stats.tlasSize), mb(stats.tlasUploadSize));
  ImGui::Text("Scratch space: %d MB", mb(stats.scratchBuffersSize));
  ImGui::Text("Transform buffers: %d MB", mb(stats.transformBuffersSize));
  ImGui::Text("Mesh count: %d", stats.meshCount);
  ImGui::SameLine();
  ImGui::Text("Mesh VB: %d MB", mb(stats.meshVBSize));
  ImGui::SameLine();
  ImGui::Text("Mesh VB copy: %d MB", mb(stats.meshVBCopySize));
  ImGui::SameLine();
  ImGui::Text("Mesh IB: %d MB", mb(stats.meshIBSize));
  ImGui::Text("Mesh meta: %d MB", mb(stats.meshMetaSize));
  ImGui::SameLine();
  ImGui::Text("Mesh BLAS: %d MB", mb(stats.meshBlasSize));
  ImGui::Text("Skin count: %d", stats.skinCount);
  ImGui::SameLine();
  ImGui::Text("Skin VB: %d MB", mb(stats.skinVBSize));
  ImGui::SameLine();
  ImGui::Text("Skin BLAS: %d MB", mb(stats.skinBLASSize));
  ImGui::Text("Tree count: %d", stats.treeCount);
  ImGui::SameLine();
  ImGui::Text("Tree VB: %d MB", mb(stats.treeVBSize));
  ImGui::SameLine();
  ImGui::Text("Tree BLAS: %d MB", mb(stats.treeBLASSize));
  ImGui::Text("Tree cache count: %d", stats.treeCacheCount);
  ImGui::SameLine();
  ImGui::Text("Tree cache BLAS: %d MB", mb(stats.treeCacheBLASSize));
  ImGui::SameLine();
  ImGui::Text("Dynamic allocation size: %d MB", mb(stats.dynamicVBAllocatorSize));
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
  ImGui::Text("GPU object query: %d MB", mb(stats.gobjQuerySize));
  ImGui::Text("Per instance data: %d MB", mb(stats.perInstanceDataSize));
  ImGui::Text("Compaction data: %d MB - Peak: %d MB", mb(stats.compactionSize), mb(stats.peakCompactionSize));
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

  ImGui::Text(" ");

  ImGui::Text("Denoiser - Common: %d MB - AO: %d MB - Reflection: %d MB - Shared: %d MB", stats.demoiserCommonSizeMB,
    stats.demoiserAoSizeMB, stats.demoiserReflectionSizeMB, stats.demoiserTransientSizeMB);

  ImGui::Separator();

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

  ImGui::Separator();

  ImGui::Checkbox("Lock terrain", &bvh_terrain_lock);
  ImGui::SliderInt("Terrain lods", &bvh_terrain_lod_count, 1, 6);

  ImGui::Separator();

  ImGui::SliderFloat("Max water depth", &bvh::max_water_distance, 0.1, 50);
  ImGui::SliderFloat("Water fade power", &bvh::water_fade_power, 0, 5);

  ImGui::Separator();

  ImGuiDagor::EnumCombo("Debug mode", bvh::DebugMode::None, bvh::DebugMode::Paint, debug_mode, &operator!);

  ImGui::Checkbox("Super sampling", &do_super_sampling);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(-FLT_MIN);
  if (ImGui::Button("Make capture"))
    console::command("render.pix_capture_n_frames");

  int availableWidth = max(ImGui::GetContentRegionAvail().x * (do_super_sampling ? 2 : 1), 10.0f);
  if (availableWidth != last_available_width)
    resolution_change_cooldown = debugTex ? 50 : 0;
  else if (resolution_change_cooldown > 0)
    resolution_change_cooldown--;
  else
    target_width = availableWidth;

  last_available_width = availableWidth;

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
  debugTex.close();
}

void render_debug_context(ContextId context_id)
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
      debugTex.close();
  }

  if (!debugShader)
    debugShader.reset(new_compute_shader("bvh_debug"));

  if (!debugTex)
  {
    float aspect = d3d::get_screen_aspect_ratio();
    debugTex =
      dag::create_tex(nullptr, target_width, target_width / aspect, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "bvh_debug_tex");
    debugTex->texfilter(do_super_sampling ? TEXFILTER_LINEAR : TEXFILTER_POINT);
  }

  bvh::bind_resources(debugged_context_id, target_width);

  TextureInfo ti;
  debugTex->getinfo(ti);

  static int bvh_debug_target = ::get_shader_variable_id("bvh_debug_target");
  static int bvh_debug_mode = ::get_shader_variable_id("bvh_debug_mode");

  ShaderGlobal::set_texture(bvh_debug_target, debugTex.getTexId());
  ShaderGlobal::set_int(bvh_debug_mode, *debug_mode - *DebugMode::Lit);

  debugShader->dispatchThreads(ti.w, ti.h, 1);
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
void render_debug_context(ContextId) {}
void teardown() {}
} // namespace bvh::debug

#endif
