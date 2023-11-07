#include "landMesh/biomeQuery.h"

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

#include "math/dag_mathUtils.h"
#include "math/dag_hlsl_floatx.h"
#include "math/integer/dag_IPoint4.h"
#include "3d/dag_drv3d.h"
#include "3d/dag_drv3dCmd.h"
#include "3d/dag_drv3dReset.h"
#include <3d/dag_resPtr.h>
#include "shaders/dag_shaders.h"
#include "util/dag_console.h"
#include "util/dag_convar.h"
#include "util/dag_oaHashNameMap.h"
#include "gpuReadbackQuery/gpuReadbackQuerySystem.h"
#include <math/dag_hlsl_floatx.h>
#include <osApiWrappers/dag_critSec.h>
#include "landMesh/biome_query.hlsli"

// #define DEBUG_DRAW_QUERIES 1

#if DEBUG_DRAW_QUERIES
#include "debug/dag_debug3d.h"
#include "collisionGlobals.h"
#include "landMesh/lmeshManager.h"
float2 hammersley(uint Index, uint NumSamples)
{
  return float2(float(Index) / float(NumSamples), float(reverse_bits32(Index)) * 2.3283064365386963e-10);
}
float3 hammersley_hemisphere_cos(uint Index, uint NumSamples)
{
  float2 h = hammersley(Index, NumSamples);
  float phi = h.y * 2.0 * 3.14159265;
  float cosTheta = sqrt(1.0 - h.x);
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
#endif

static WinCritSec g_biome_query_cs;
#define BIOME_QUERY_BLOCK WinAutoLock biomeQueryLock(g_biome_query_cs);

CONSOLE_BOOL_VAL("biome_query", run_test_queries, false);

class BiomeQueryCtx
{
public:
  bool init();
  void initLandClass(TEXTUREID lc_texture, float tile, dag::ConstSpan<const char *> biome_group_names,
    dag::ConstSpan<int> biome_group_indices);
  void update();
  void beforeDeviceReset();
  void afterDeviceReset();
  int query(const Point3 &world_pos, const float radius);
  GpuReadbackResultState getQueryResult(int query_id, BiomeQueryResult &result);
  int getBiomeGroup(int biome_id);
  int getBiomeGroupId(const char *biome_group_name);
  const char *getBiomeGroupName(int biome_group_id);
  int getNumBiomeGroups();
  void setDetailsCBAndSlot(Sbuffer *buffer);

private:
  eastl::unique_ptr<GpuReadbackQuerySystem<BiomeQueryInput, BiomeQueryResult>> grqSystem;
  float minSampleScaleRadius = MIN_SAMPLE_SCALE_RADIUS_TEXEL_SPACE;
  Tab<int> biomeGroupIndices;
  FastNameMap biomeGroupNameMap;
  UniqueBufHolder biomeGroupIndicesBuffer;
  int numBiomeGroups = 0;
  Sbuffer *detailsCB = 0;
#if DEBUG_DRAW_QUERIES
  const int MAX_DEBUG_DRAW_QUERIES = 10;
  eastl::vector<BiomeQueryInput> debugDrawInputs;
#endif

  void setBiomeGroupIndicesBufferData();
  void debugDrawQueries();
};

static eastl::unique_ptr<BiomeQueryCtx> biome_query_ctx;
static int num_biome_groups_slot = -1;
static int details_cb_slot = -1;
#if DAGOR_DBGLEVEL > 0
static int console_query_id = -1;
static int console_query_elapsed_frames = 0;
#endif

bool BiomeQueryCtx::init()
{
  int resultBufferSlot = -1;
  if (!ShaderGlobal::get_int_by_name("biome_query_results_buf_slot", resultBufferSlot))
  {
    logerr("BiomeQuery: Could not find shader variable \"biome_query_results_buf_slot\"");
    return false;
  }
  int resultsOffsetSlot = -1;
  if (!ShaderGlobal::get_int_by_name("biome_results_offset_slot", resultsOffsetSlot))
  {
    logerr("BiomeQuery: Could not find shader variable \"biome_results_offset_slot\"");
    return false;
  }
  if (!ShaderGlobal::get_int_by_name("num_biome_groups_slot", num_biome_groups_slot))
  {
    logerr("BiomeQuery: Could not find shader variable \"num_biome_groups_slot\"");
    return false;
  }

  if (!ShaderGlobal::get_int_by_name("landmesh_indexed_landclass_lc_ps_details_cb_register", details_cb_slot))
  {
    logerr("BiomeQuery: Could not find shader variable \"landmesh_indexed_landclass_lc_ps_details_cb_register\"");
    return false;
  }

  GpuReadbackQuerySystemDescription grqsDesc;
  grqsDesc.maxQueriesPerFrame = MAX_BIOME_QUERIES_PER_FRAME;
  grqsDesc.maxQueriesPerDispatch = MAX_BIOME_QUERIES_PER_DISPATCH;
  grqsDesc.computeShaderName = "biome_query_cs";
  grqsDesc.resultBufferName = "biome_query_results_ring_buf";
  grqsDesc.resultBufferSlot = resultBufferSlot;
  grqsDesc.resultsOffsetSlot = resultsOffsetSlot;
  grqsDesc.inputBufferName = "biome_query_input_buf";

  grqSystem = eastl::make_unique<GpuReadbackQuerySystem<BiomeQueryInput, BiomeQueryResult>>(grqsDesc);
  if (!grqSystem->isValid())
  {
    logerr("BiomeQuery: Failed to initialize GpuReadbackQuerySystem.");
    grqSystem.reset();
    return false;
  }

  biomeGroupIndicesBuffer = dag::buffers::create_persistent_sr_structured(sizeof(int), MAX_BIOMES, "biome_group_indices_buffer");
  setBiomeGroupIndicesBufferData();

  return true;
}

void biome_query::init()
{
  BIOME_QUERY_BLOCK;
  biome_query_ctx = eastl::make_unique<BiomeQueryCtx>();
  if (!biome_query_ctx->init())
  {
    logerr("Biome query system failed to initialize.");
    biome_query_ctx.reset();
  }
}

void BiomeQueryCtx::initLandClass(TEXTUREID lc_texture, float tile, dag::ConstSpan<const char *> biome_group_names,
  dag::ConstSpan<int> biome_group_indices)
{
  int minSampleScaleRadiusVarId = ::get_shader_variable_id("min_sample_scale_radius", true);
  if (!VariableMap::isGlobVariablePresent(minSampleScaleRadiusVarId))
    return;

  BaseTexture *biomeTex = ::acquire_managed_tex(lc_texture);
  if (!biomeTex)
    return;
  TextureInfo biomeTexInfo;
  biomeTex->getinfo(biomeTexInfo);
  ::release_managed_tex(lc_texture);
  G_ASSERT(biomeTexInfo.w == biomeTexInfo.h);

  minSampleScaleRadius = (1.0f / (tile * biomeTexInfo.w)) * MIN_SAMPLE_SCALE_RADIUS_TEXEL_SPACE;
  ShaderGlobal::set_real(minSampleScaleRadiusVarId, minSampleScaleRadius);

  numBiomeGroups = min<int>(MAX_BIOME_GROUPS, biome_group_names.size());
  G_ASSERT(numBiomeGroups <= MAX_BIOME_GROUPS);

  biomeGroupNameMap.reset();
  for (size_t i = 0; i < numBiomeGroups; i++)
    biomeGroupNameMap.addNameId(biome_group_names[i]);

  clear_and_shrink(biomeGroupIndices);
  if (numBiomeGroups > 0)
    biomeGroupIndices = biome_group_indices;
  else // default is to have an identical mapping
    for (int i = 0; i < MAX_BIOMES; i++)
      biomeGroupIndices.push_back(i);
  G_ASSERT(biomeGroupIndices.size() <= MAX_BIOMES);

  setBiomeGroupIndicesBufferData();
}

void BiomeQueryCtx::setBiomeGroupIndicesBufferData()
{
  bool success = false;
  if (biomeGroupIndices.size() > 0)
  {
    uint32_t sizeBytes = min<int>(MAX_BIOMES, biomeGroupIndices.size()) * sizeof(int);
    success = biomeGroupIndicesBuffer.getBuf()->updateData(0, sizeBytes, &biomeGroupIndices[0], 0);
  }
  else
  {
    int zeroes[MAX_BIOMES] = {0};
    success = biomeGroupIndicesBuffer.getBuf()->updateData(0, (uint32_t)(MAX_BIOMES * sizeof(int)), &zeroes[0], 0);
  }
  G_ASSERT(success);
}

void biome_query::init_land_class(TEXTUREID lc_texture, float tile, dag::ConstSpan<const char *> biome_group_names,
  dag::ConstSpan<int> biome_group_indices)
{
  BIOME_QUERY_BLOCK;
  if (biome_query_ctx)
    biome_query_ctx->initLandClass(lc_texture, tile, biome_group_names, biome_group_indices);
}

void biome_query::close()
{
  BIOME_QUERY_BLOCK;
  biome_query_ctx.reset();
}

void BiomeQueryCtx::debugDrawQueries()
{
#if DEBUG_DRAW_QUERIES
  ::begin_draw_cached_debug_lines();
  E3DCOLOR color = E3DCOLOR_MAKE(255, 0, 0, 255);
  for (size_t i = 0; i < debugDrawInputs.size(); i++)
  {
    Point3 terrainAlignedWorldPos = debugDrawInputs[i].worldPos;
    dacoll::get_lmesh()->getHeight(Point2::xz(terrainAlignedWorldPos), terrainAlignedWorldPos.y, nullptr);
    float inputRadius = debugDrawInputs[i].radius;
    float sampleScaleRadius = max(minSampleScaleRadius, inputRadius);
    ::draw_cached_debug_xz_circle(terrainAlignedWorldPos, inputRadius, color);
    const uint32_t maxNumSamples = BIOME_QUERY_WARP_SIZE * BIOME_QUERY_WARP_SIZE;
    for (uint32_t j = 0; j < maxNumSamples; j++)
    {
      Point3 sampleOffset = Point3::x0y(hammersley_hemisphere_cos(j, maxNumSamples)) * sampleScaleRadius;
      if (sampleOffset.lengthSq() > inputRadius * inputRadius)
        continue;

      Point3 pos = terrainAlignedWorldPos + sampleOffset;
      dacoll::get_lmesh()->getHeight(Point2::xz(pos), pos.y, nullptr);
      ::draw_cached_debug_xz_circle(pos, 0.1f, color);
    }
  }
  ::end_draw_cached_debug_lines();
#endif
}

void BiomeQueryCtx::update()
{
  // FIXME: This is ugly
  // 1. We could store this in a float, then we wouldn't need to set this manually
  // 2. Maybe we need to figure out generic additional data setting for GpuReadbackQuerySystem, so it
  //    can be configured to handle setting extra stuff to the shader like this
  // 3. Probably the best option: Integer shader consts are on the way, let's wait for it, and refactor
  //    this once it's available
  alignas(16) int numBiomeGroupsForShader[] = {numBiomeGroups > 0 ? numBiomeGroups : MAX_BIOMES, 0, 0, 0};
  d3d::set_const_buffer(STAGE_CS, details_cb_slot, detailsCB);
  d3d::set_const(STAGE_CS, num_biome_groups_slot, numBiomeGroupsForShader, 1);

  grqSystem->update();

  debugDrawQueries();

#if DAGOR_DBGLEVEL > 0
  if (console_query_id != -1)
  {
    BiomeQueryResult result;
    GpuReadbackResultState consoleQueryResultState = grqSystem->getQueryResult(console_query_id, result);
    if (::is_gpu_readback_query_successful(consoleQueryResultState))
    {
      int idx1 = result.mostFrequentBiomeGroupIndex;
      float w1 = result.mostFrequentBiomeGroupWeight;
      int idx2 = result.secondMostFrequentBiomeGroupIndex;
      float w2 = result.secondMostFrequentBiomeGroupWeight;
      float4 color = result.averageBiomeColor;
      console::print_d("Biome query completed after %d updates: "
                       "Most frequent: [idx:%d weight:%.3f name:%s], 2nd most frequent: [idx:%d weight:%.3f name:%s],"
                       "AvarageColor color: %.2f,%.2f,%.2f,%.2f",
        console_query_elapsed_frames, idx1, w1, getBiomeGroupName(idx1), idx2, w2, getBiomeGroupName(idx2), color.x, color.y, color.z,
        color.w);
      console_query_id = -1;
    }
    else if (::is_gpu_readback_query_failed(consoleQueryResultState))
    {
      console::print_d("Biome query completed after %d updates", console_query_elapsed_frames);
      console_query_id = -1;
    }
    else
    {
      console_query_elapsed_frames++;
    }
  }
#endif
}

static void do_test_queries()
{
#if DAGOR_DBGLEVEL > 0
  if (!run_test_queries.get())
    return;
  static int testQueryId = -1;
  static int framesSinceQuery = 0;
  if (testQueryId != -1)
  {
    framesSinceQuery++;
    BiomeQueryResult r;
    GpuReadbackResultState rs = biome_query::get_query_result(testQueryId, r);
    if (rs != GpuReadbackResultState::IN_PROGRESS)
    {
      console::print_d("Test biome query result: 1st: %d, 2nd: %d, rs: %d, frames: %d", r.mostFrequentBiomeGroupIndex,
        r.secondMostFrequentBiomeGroupIndex, int(rs), framesSinceQuery);
      testQueryId = -1;
    }
  }
  if (testQueryId == -1)
  {
    testQueryId = biome_query::query(Point3(0, 0, 0), 1);
    framesSinceQuery = 0;
  }
#endif
}

void biome_query::update()
{
  BIOME_QUERY_BLOCK;
  if (!biome_query_ctx)
    return;
  do_test_queries();
  biome_query_ctx->update();
}

void BiomeQueryCtx::beforeDeviceReset() { grqSystem->beforeDeviceReset(); }

void BiomeQueryCtx::afterDeviceReset()
{
  grqSystem->afterDeviceReset();
  setBiomeGroupIndicesBufferData();
}

int BiomeQueryCtx::query(const Point3 &world_pos, const float radius)
{
  BiomeQueryInput input;
  input.worldPos = world_pos;
  input.radius = radius;

  int queryId = grqSystem->query(input);

#if DEBUG_DRAW_QUERIES
  if (queryId > 0)
  {
    debugDrawInputs.push_back(input);
    while (debugDrawInputs.size() > MAX_DEBUG_DRAW_QUERIES)
      debugDrawInputs.erase(debugDrawInputs.begin());
  }
#endif

  return queryId;
}

int biome_query::query(const Point3 &world_pos, const float radius)
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->query(world_pos, radius) : -1;
}

GpuReadbackResultState BiomeQueryCtx::getQueryResult(int query_id, BiomeQueryResult &result)
{
  GpuReadbackResultState resultState = grqSystem->getQueryResult(query_id, result);

  // To ensure no crashes:
  if (resultState == GpuReadbackResultState::SUCCEEDED)
  {
    int maxIdx = numBiomeGroups > 0 ? (numBiomeGroups - 1) : (MAX_BIOMES - 1);
    int firstIdx = result.mostFrequentBiomeGroupIndex;
    int secondIdx = result.secondMostFrequentBiomeGroupIndex;
    static bool resultOutOfRangeLogged = false;
    if (!resultOutOfRangeLogged && (firstIdx < 0 || firstIdx > maxIdx || secondIdx < 0 || secondIdx > maxIdx))
    {
      BiomeQueryInput qInput;
      grqSystem->getQueryInput(query_id, qInput);
      logmessage(DAGOR_DBGLEVEL > 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN,
        "Biome query result is out of range! firstIdx: %d, secondIdx: %d, maxIdx: %d, pos (%f %f %f),  radius %f", firstIdx, secondIdx,
        maxIdx, qInput.worldPos.x, maxIdx, qInput.worldPos.y, maxIdx, qInput.worldPos.z, qInput.radius);
      resultOutOfRangeLogged = true;
    }
    result.mostFrequentBiomeGroupIndex = clamp(firstIdx, 0, maxIdx);
    result.secondMostFrequentBiomeGroupIndex = clamp(secondIdx, 0, maxIdx);
  }

  return resultState;
}

GpuReadbackResultState biome_query::get_query_result(int query_id, BiomeQueryResult &result)
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->getQueryResult(query_id, result) : GpuReadbackResultState::SYSTEM_NOT_INITIALIZED;
}

int BiomeQueryCtx::getBiomeGroup(int biome_id)
{
  if (biome_id < 0 || biome_id >= biomeGroupIndices.size())
    return -1;
  return biomeGroupIndices[biome_id];
}

int biome_query::get_biome_group(int biome_id)
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->getBiomeGroup(biome_id) : -1;
}

int BiomeQueryCtx::getBiomeGroupId(const char *biome_group_name) { return biomeGroupNameMap.getNameId(biome_group_name); }

int biome_query::get_biome_group_id(const char *biome_group_name)
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->getBiomeGroupId(biome_group_name) : -1;
}

const char *BiomeQueryCtx::getBiomeGroupName(int biome_group_id) { return biomeGroupNameMap.getName(biome_group_id); }

const char *biome_query::get_biome_group_name(int biome_group_id)
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->getBiomeGroupName(biome_group_id) : nullptr;
}

int BiomeQueryCtx::getNumBiomeGroups() { return numBiomeGroups; }

void BiomeQueryCtx::setDetailsCBAndSlot(Sbuffer *buffer) { detailsCB = buffer; }

int biome_query::get_num_biome_groups()
{
  BIOME_QUERY_BLOCK;
  return biome_query_ctx ? biome_query_ctx->getNumBiomeGroups() : -1;
}

int biome_query::get_max_num_biome_groups() { return MAX_BIOME_GROUPS; }

void biome_query::set_details_cb(Sbuffer *buffer)
{
  BIOME_QUERY_BLOCK;
  if (biome_query_ctx)
    biome_query_ctx->setDetailsCBAndSlot(buffer);
}

static void biome_query_before_device_reset(bool)
{
  BIOME_QUERY_BLOCK;
  if (biome_query_ctx)
    biome_query_ctx->beforeDeviceReset();
}

static void biome_query_after_device_reset(bool)
{
  BIOME_QUERY_BLOCK;
  if (biome_query_ctx)
    biome_query_ctx->afterDeviceReset();
}

#if DAGOR_DBGLEVEL > 0
void biome_query::console_query_pos(const char *argv[], int argc)
{
  BIOME_QUERY_BLOCK;
  if (!biome_query_ctx)
    return;

  if (argc < 4)
    console::print("Usage: biome_query.queryPos x y z [radius=1]");
  else
  {
    Point3 pos(atof(argv[1]), atof(argv[2]), atof(argv[3]));
    float rad = argc >= 5 ? atof(argv[4]) : 1.f;
    int id = biome_query::query(pos, rad);
    console_query_id = id;
    console_query_elapsed_frames = 0;
    console::print_d("Query added: id: %d, pos: (%.3f, %.3f, %.3f), rad: %.3f", id, pos.x, pos.y, pos.z, rad);
  }
}

void biome_query::console_query_camera_pos(const char *argv[], int argc, const TMatrix &view_itm)
{
  if (!biome_query_ctx)
    return;

  Point3 pos = view_itm.getcol(3);
  float rad = argc >= 2 ? atof(argv[1]) : 1.f;
  int id = biome_query::query(pos, rad);
  console_query_id = id;
  console_query_elapsed_frames = 0;
  console::print_d("Query added: id: %d, pos: (%.3f, %.3f, %.3f), rad: %.3f", id, pos.x, pos.y, pos.z, rad);
}
#else
void biome_query::console_query_pos(const char *[], int) {}
void biome_query::console_query_camera_pos(const char *[], int, const TMatrix &) {}
#endif

REGISTER_D3D_BEFORE_RESET_FUNC(biome_query_before_device_reset);
REGISTER_D3D_AFTER_RESET_FUNC(biome_query_after_device_reset);
