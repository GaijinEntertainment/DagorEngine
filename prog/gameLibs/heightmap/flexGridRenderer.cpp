// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/flexGridRenderer.h>

#include <heightmap/flexGridDefines.h>
#include <heightmap/flexGridConsts.hlsli>
#include <heightmap/flexGridSubdivisionUtils.h>
#include <heightmap/heightmapHandler.h>
#include <osApiWrappers/dag_critSec.h>
#include <3d/dag_lockSbuffer.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_bounds2.h>
#include <math/dag_adjpow2.h>
#include <shaders/dag_shaders.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <frustumCulling/frustumPlanes.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <EASTL/bit.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_shaderConstants.h>
#include <gui/dag_visualLog.h>

#include <generic/dag_staticTab.h>

WinCritSec initPrepareMutex;

namespace convar
{
CONSOLE_BOOL_VAL("render", flex_grid_nonindexed, true);
CONSOLE_FLOAT_VAL("flexgrid", metrics_patch_radius, 0.f);
CONSOLE_FLOAT_VAL("flexgrid", metrics_height, 0.0f);
CONSOLE_FLOAT_VAL("flexgrid", metrics_abs_height, 0.f);
} // namespace convar

namespace SFlexGrid
{
// special case: "negative size" rects
static bool intersectRectanglesCustom(const IBBox2 &rect, const IBBox2 &other)
{
  Point2 size = rect.size();
  Point2 otherSize = other.size();

  if ((rect.top() + size.y) < other.top())
    return false;

  if (rect.top() > (other.top() + otherSize.y))
    return false;

  if ((rect.left() + size.x) < other.left())
    return false;

  if (rect.left() > (other.left() + otherSize.x))
    return false;

  return true;
}

static uint8_t encodePosUint8(uint32_t vertexIndex) { return uint8_t(round(vertexIndex * (255.0f / 8.0f))); }
} // namespace SFlexGrid

bool FlexGridRenderer::isInited() const { return initialized; }

bool FlexGridRenderer::init(const FlexGridConfig &cfg, bool do_fatal)
{
  WinAutoLock lock(initPrepareMutex);

  if (isInited())
  {
    return true;
  }

  if (!initMaterial("heightmap_flexgrid", "", do_fatal))
  {
    return false;
  }

  createPatchBuffers();

  instanceData.resize(FLEXGRID_MAX_INSTANCES_PER_DRAW);

  initialized = true;

  return true;
}

bool FlexGridRenderer::initMaterial(const char *shader_name, const char *mat_script, bool do_fatal)
{
  shmat = new_shader_material_by_name(shader_name, mat_script);

  if (!shmat)
  {
    if (do_fatal)
      DAG_FATAL("can't create ShaderMaterial for '%s'", shader_name);
    return false;
  }
  shElem = shmat->make_elem();
  if (!shElem)
  {
    if (do_fatal)
      DAG_FATAL("can't create ShaderElement for ShaderMaterial '%s'", shader_name);
    return false;
  }

  return true;
}

void FlexGridRenderer::createPatchBuffers()
{
  constexpr uint32_t patch_size_vertices = FLEXGRID_PATCH_QUADS + 1u;
  constexpr uint32_t patch_vertices_count = patch_size_vertices * patch_size_vertices;
  constexpr uint32_t patch_indices_count = FLEXGRID_PATCH_QUADS * FLEXGRID_PATCH_QUADS * 6;
  constexpr uint32_t patch_triangles_count = FLEXGRID_PATCH_QUADS * FLEXGRID_PATCH_QUADS * 2;
  constexpr uint32_t patch_nonindexed_vertices_count = patch_triangles_count * 3;

  patchTriangleCount = patch_indices_count / 3;

  dag::Vector<uint16_t, framemem_allocator> patchIndices;
  patchIndices.reserve(patch_indices_count);
  for (uint16_t y = 0u; y < uint16_t(FLEXGRID_PATCH_QUADS); ++y)
  {
    uint16_t y_row_offset = y * uint16_t(patch_size_vertices);
    for (uint16_t x = 0u; x < uint16_t(FLEXGRID_PATCH_QUADS); ++x)
    {
      patchIndices.emplace_back(y_row_offset + x);                       // A
      patchIndices.emplace_back(y_row_offset + x + 1u);                  // B
      patchIndices.emplace_back(y_row_offset + patch_size_vertices + x); // C

      patchIndices.emplace_back(y_row_offset + patch_size_vertices + x);      // C
      patchIndices.emplace_back(y_row_offset + x + 1u);                       // B
      patchIndices.emplace_back(y_row_offset + patch_size_vertices + x + 1u); // D
    }
  }
  G_ASSERTF(patchIndices.size() == size_t(patch_indices_count), "FlexGridRenderer::createTerrainBatches: wrong patchIndices");

  dag::Vector<VertexData, framemem_allocator> patchVertices;
  patchVertices.resize(patch_vertices_count);

  for (uint32_t y = 0u; y < patch_size_vertices; ++y)
  {
    for (uint32_t x = 0u; x < patch_size_vertices; ++x)
    {
      VertexData &vertex = patchVertices[y * patch_size_vertices + x];

      vertex.posEdgeShiftDirection.r = SFlexGrid::encodePosUint8(x); // [0, 255] -> [0, 1] in shader
      vertex.posEdgeShiftDirection.g = SFlexGrid::encodePosUint8(y); // [0, 255] -> [0, 1] in shader
      vertex.posEdgeShiftDirection.b = 128;                          // -> means 0
      vertex.posEdgeShiftDirection.a = 128;                          // -> means 0

      // averageShift.xy
      vertex.data1.g = ((x & 1) != 0u) ? 0 : 128;   // -1 : 0 in shader
      vertex.data1.b = ((y & 1) != 0u) ? 255 : 128; // 1 : 0 in shader

      vertex.averageShiftPrevLod.r = vertex.data1.g;
      vertex.averageShiftPrevLod.g = vertex.data1.b;
      vertex.averageShiftPrevLod.b = vertex.data1.g;
      vertex.averageShiftPrevLod.a = vertex.data1.b;
    }
  }

  for (uint32_t i = 1u; i < FLEXGRID_PATCH_QUADS; ++i)
  {
    uint32_t l_side_offset = i;
    uint32_t r_side_offset = FLEXGRID_PATCH_QUADS * patch_size_vertices + i;
    uint32_t t_side_offset = i * patch_size_vertices;
    uint32_t b_side_offset = i * patch_size_vertices + FLEXGRID_PATCH_QUADS;

    const uint32_t iInv = FLEXGRID_PATCH_QUADS - i;
    const float float_index = float(i);
    const float float_index_inv = float(iInv);

    patchVertices[l_side_offset].posEdgeShiftDirection.b = 0;   // -> -1
    patchVertices[r_side_offset].posEdgeShiftDirection.b = 255; // -> 1
    patchVertices[t_side_offset].posEdgeShiftDirection.a = 0;   // -1
    patchVertices[b_side_offset].posEdgeShiftDirection.a = 255; // -> 1

    patchVertices[l_side_offset].edgeShiftMask.g = 255;
    patchVertices[r_side_offset].edgeShiftMask.a = 255;
    patchVertices[t_side_offset].edgeShiftMask.r = 255;
    patchVertices[b_side_offset].edgeShiftMask.b = 255;

    // edgeVertexMask
    patchVertices[l_side_offset].data1.r = SFlexGrid::encodePosUint8(i);
    patchVertices[r_side_offset].data1.r = SFlexGrid::encodePosUint8(iInv);
    patchVertices[t_side_offset].data1.r = SFlexGrid::encodePosUint8(i);
    patchVertices[b_side_offset].data1.r = SFlexGrid::encodePosUint8(iInv);

    // morph glue with parent LOD
    if (i % 4 == 1)
    {
      patchVertices[l_side_offset].averageShiftPrevLod.r = 128; // -> 0
      patchVertices[t_side_offset].averageShiftPrevLod.g = 128; // -> 0
    }
    else if (i % 4 != 0)
    {
      patchVertices[l_side_offset].averageShiftPrevLod.r = 0;   // -> -1
      patchVertices[t_side_offset].averageShiftPrevLod.g = 255; // -> 1
    }
    if (iInv % 4 == 1)
    {
      patchVertices[r_side_offset].averageShiftPrevLod.r = 128; // -> 0
      patchVertices[b_side_offset].averageShiftPrevLod.g = 128; // -> 0
    }
    else if (iInv % 4 != 0)
    {
      patchVertices[r_side_offset].averageShiftPrevLod.r = 0;   // -> -1
      patchVertices[b_side_offset].averageShiftPrevLod.g = 255; // -> 1
    }

    // morph glue with grandparent LOD
    if (i % 8 >= 1 && i % 8 <= 3)
    {
      patchVertices[l_side_offset].averageShiftPrevLod.b = 128; // -> 0
      patchVertices[t_side_offset].averageShiftPrevLod.a = 128; // -> 0
    }
    else if (i % 8 != 0)
    {
      patchVertices[l_side_offset].averageShiftPrevLod.b = 0;   // -> -1
      patchVertices[t_side_offset].averageShiftPrevLod.a = 255; // -> 1
    }
    if (iInv % 8 >= 1 && iInv % 8 <= 3)
    {
      patchVertices[r_side_offset].averageShiftPrevLod.b = 128; // -> 0
      patchVertices[b_side_offset].averageShiftPrevLod.a = 128; // -> 0
    }
    else if (iInv % 8 != 0)
    {
      patchVertices[r_side_offset].averageShiftPrevLod.b = 0;   // -> -1
      patchVertices[b_side_offset].averageShiftPrevLod.a = 255; // -> 1
    }
  }

  // create gpu buffers
  size_t vDataBytes = patchVertices.size() * sizeof(VertexData);
  vb = dag::create_vb(vDataBytes, 0, "patch_vertices");
  vb->updateData(0, vDataBytes, patchVertices.data(), VBLOCK_WRITEONLY);

  size_t indexDataBytes = sizeof(uint16_t) * patch_indices_count;
  ib = dag::create_ib(indexDataBytes, 0, "patch_indices");
  ib->updateData(0, indexDataBytes, patchIndices.data(), VBLOCK_WRITEONLY);

  // --- Non-indexed vb ---
  dag::Vector<VertexData, framemem_allocator> patchVerticesNonIndexed;
  patchVerticesNonIndexed.reserve(patch_nonindexed_vertices_count);
  for (uint16_t y = 0u; y < uint16_t(FLEXGRID_PATCH_QUADS); ++y)
  {
    uint16_t y_row_offset = y * uint16_t(patch_size_vertices);
    for (uint16_t x = 0u; x < uint16_t(FLEXGRID_PATCH_QUADS); ++x)
    {
      uint16_t A = y_row_offset + x;
      uint16_t B = y_row_offset + x + 1u;
      uint16_t C = y_row_offset + patch_size_vertices + x;
      uint16_t D = y_row_offset + patch_size_vertices + x + 1u;
      patchVerticesNonIndexed.push_back(patchVertices[A]);
      patchVerticesNonIndexed.push_back(patchVertices[B]);
      patchVerticesNonIndexed.push_back(patchVertices[C]);
      patchVerticesNonIndexed.push_back(patchVertices[C]);
      patchVerticesNonIndexed.push_back(patchVertices[B]);
      patchVerticesNonIndexed.push_back(patchVertices[D]);
    }
  }

  size_t vDataBytesNonIndexed = patchVerticesNonIndexed.size() * sizeof(VertexData);
  vb_nonindexed = dag::create_vb(vDataBytesNonIndexed, 0, "patch_vertices_nonindexed");
  vb_nonindexed->updateData(0, vDataBytesNonIndexed, patchVerticesNonIndexed.data(), VBLOCK_WRITEONLY);

  static_assert(patch_nonindexed_vertices_count == 384);                     // patchsize 8
  static_assert(FLEXGRID_PATCH_QUADS * FLEXGRID_PATCH_QUADS * 2 * 3 == 384); // patchsize 8
}

void FlexGridRenderer::prepareSubdivisionForHeightmap(const HeightmapHandler &hmap_handler, bool force_rebuild)
{
  WinAutoLock lock(initPrepareMutex);
  if (!isInited())
  {
    G_ASSERTF(false, "FlexGridRenderer: prepareSubdivisionForHeightmap: Terrain is not inited");
    return;
  }
  const bool shouldRebuild = force_rebuild || !heightfieldSubdivAsset.isInitialized;
  if (!shouldRebuild)
    return;

  heightfieldSubdivAsset.subdivBBox = hmap_handler.getWorldBox();

  const uint32_t size = hmap_handler.getHeightmapSizeX();

  heightfieldSubdivAsset.heightRangeChainLevels = (size > 0) ? (log2(size) + 1) : 1;
  heightfieldSubdivAsset.edgeDirectionChainLevels = (size > 0) ? (log2(size) + 1) : 1;
  heightfieldSubdivAsset.heightErrorChainLevels = (size > 0) ? (log2(size / heightfieldSubdivAsset.patchSizeQuads) + 1) : 1;

  heightfieldSubdivAsset.heightRangeChain.resize(
    FlexGridSubdivisionUtils::get_level_offset(heightfieldSubdivAsset.heightRangeChainLevels));
  heightfieldSubdivAsset.heightErrorChain.resize(
    FlexGridSubdivisionUtils::get_level_offset(heightfieldSubdivAsset.heightErrorChainLevels));
  heightfieldSubdivAsset.edgeDirectionChain.resize(
    FlexGridSubdivisionUtils::get_level_offset(heightfieldSubdivAsset.edgeDirectionChainLevels));

  updateHeightInfoRecursive(hmap_handler, &heightfieldSubdivAsset, 0, 0, 0, nullptr, nullptr,
    IBBox2(IPoint2(-1, -1), IPoint2(-2, -2)));

  heightfieldSubdivAsset.isInitialized = true;
}

bool FlexGridRenderer::isSubdivisionForHeightmapPrepared() const { return isInited() && heightfieldSubdivAsset.isInitialized; }

static Point3 computeBarycentric(const Point2 &v0, const Point2 &v1, const Point2 &p)
{
  Point2 v1v0 = v1 - v0;
  Point2 pv0 = p - v0;
  float d00 = dot(v0, v0);
  float d01 = dot(v0, v1);
  float d11 = dot(v1, v1);
  float d20 = dot(p, v0);
  float d21 = dot(p, v1);
  float denom = d00 * d11 - d01 * d01;

  float v = (d11 * d20 - d01 * d21) / denom;
  float w = (d00 * d21 - d01 * d20) / denom;
  float u = 1.0f - v - w;

  return Point3(u, v, w);
}

static inline float saturate(float v) { return clamp(v, 0.f, 1.f); }

static int march_square(float x0, float y0, float x1, float y1, float ht00, float ht10, float ht01, float ht11, float center,
  float iso_level, Point2 segments[4])
{
  const uint32_t code = uint32_t(ht00 >= iso_level) | (uint32_t(ht10 >= iso_level) << 1) | (uint32_t(ht11 >= iso_level) << 2) |
                        (uint32_t(ht01 >= iso_level) << 3);

  if (uint32_t(code - 1u) >= 14u) // 0,15 cases
    return 0;
  // Compute interpolated points on edges
  float d = ht01 - ht00;
  Point2 left_pt = {x0, y0 + (y1 - y0) * (fabsf(d) < 1e-6f ? 0.5f : saturate((iso_level - ht00) / d))};

  d = ht10 - ht00;
  Point2 bottom_pt = {x0 + (x1 - x0) * (fabsf(d) < 1e-6f ? 0.5f : saturate((iso_level - ht00) / d)), y0};

  d = ht11 - ht10;
  Point2 right_pt = {x1, y0 + (y1 - y0) * (fabsf(d) < 1e-6f ? 0.5f : saturate((iso_level - ht10) / d))};

  d = ht11 - ht01;
  Point2 top_pt = {x0 + (x1 - x0) * (fabsf(d) < 1e-6f ? 0.5f : saturate((iso_level - ht01) / d)), y1};

  switch (code)
  {
    case 1:
    case 14:
      segments[0] = bottom_pt;
      segments[1] = left_pt;
      return 1;

    case 2:
    case 13:
      segments[0] = bottom_pt;
      segments[1] = right_pt;
      return 1;

    case 3:
    case 12:
      segments[0] = left_pt;
      segments[1] = right_pt;
      return 1;

    case 4:
    case 11:
      segments[0] = right_pt;
      segments[1] = top_pt;
      return 1;

    case 6:
    case 9:
      segments[0] = bottom_pt;
      segments[1] = top_pt;
      return 1;

    case 7:
    case 8:
      segments[0] = left_pt;
      segments[1] = top_pt;
      return 1;

    case 5:
      segments[0] = left_pt;
      if (center >= iso_level)
      {
        segments[1] = top_pt;
        segments[2] = bottom_pt;
        segments[3] = right_pt;
      }
      else
      {
        segments[1] = bottom_pt;
        segments[2] = right_pt;
        segments[3] = top_pt;
      }
      return 2;
    case 10:
      segments[0] = left_pt;
      if (center >= iso_level)
      {
        segments[1] = bottom_pt;
        segments[2] = right_pt;
        segments[3] = top_pt;
      }
      else
      {
        segments[1] = top_pt;
        segments[2] = bottom_pt;
        segments[3] = right_pt;
      }
      return 2;

    default: return 0;
  }
}

enum class ShoreMarchingResult
{
  None,
  LTRB_better,
  RTLB_better
};


void FlexGridRenderer::updateHeightInfoRecursive(const HeightmapHandler &hmap_handler, FlexGridSubdivisionAsset *subdiv,
  uint32_t level, uint32_t x, uint32_t y, FlexGridSubdivisionAsset::HeightRange *parent_height_range,
  FlexGridSubdivisionAsset::HeightError *parent_height_error, const IBBox2 &rect) const
{
  if (level >= subdiv->heightRangeChainLevels)
    return;

  const float water_level = hmap_handler.getPreparedWaterLevel();

  uint32_t hmapSize = hmap_handler.getHeightmapSizeX();
  uint32_t patch_size_hf_quads = hmapSize >> level;

  uint32_t patch_x = x * patch_size_hf_quads;
  uint32_t patch_y = y * patch_size_hf_quads;
  IPoint2 leftBottom = IPoint2(int32_t(patch_x), int32_t(patch_y));
  IPoint2 rightTop = leftBottom + IPoint2(int32_t(patch_size_hf_quads), int32_t(patch_size_hf_quads));
  IBBox2 patch_rect = IBBox2(leftBottom, rightTop);

  if (rect.width().x >= 0 && rect.width().y >= 0 && !SFlexGrid::intersectRectanglesCustom(rect, patch_rect))
    return;

  uint32_t next_level = level + 1;
  uint32_t next_x = x << 1;
  uint32_t next_y = y << 1;

  uint32_t chain_index = FlexGridSubdivisionUtils::get_chain_index(level, x, y);

  FlexGridSubdivisionAsset::HeightRange &h_range = subdiv->heightRangeChain[chain_index];
  h_range.min = eastl::numeric_limits<uint16_t>::max();
  h_range.max = 0;

  FlexGridSubdivisionAsset::HeightError *h_error = nullptr;
  if (level < subdiv->heightErrorChainLevels)
  {
    h_error = &subdiv->heightErrorChain[chain_index];
    h_error->maxError = 0.f;
    h_error->maxErrorPosition = Point3();
  }

  FlexGridSubdivisionAsset::EdgeDirection &edge_dir = subdiv->edgeDirectionChain[chain_index];
  edge_dir.level = level;
  edge_dir.x = x;
  edge_dir.y = y;
  edge_dir.p1 = 0;
  edge_dir.p2 = 0;

  BBox3 subdivBBox = subdiv->getBBox();
  uint32_t hf_step = max(patch_size_hf_quads / subdiv->patchSizeQuads, 1u);

  for (uint32_t y0 = patch_y, quadY = 0; y0 < patch_y + patch_size_hf_quads; y0 += hf_step, quadY += 1)
  {
    for (uint32_t x0 = patch_x, quadX = 0; x0 < patch_x + patch_size_hf_quads; x0 += hf_step, quadX += 1)
    {
      auto min_max_pair = eastl::minmax({HeightmapUtils::getHeight(x0, y0, hmapSize, hmap_handler),
        HeightmapUtils::getHeightSafe(x0, y0 + hf_step, hmapSize, hmap_handler),
        HeightmapUtils::getHeightSafe(x0 + hf_step, y0, hmapSize, hmap_handler),
        HeightmapUtils::getHeightSafe(x0 + hf_step, y0 + hf_step, hmapSize, hmap_handler)});

      h_range.min = eastl::min(h_range.min, min_max_pair.first);
      h_range.max = eastl::max(h_range.max, min_max_pair.second);

      if (next_level >= subdiv->heightErrorChainLevels)
        continue;

      G_ASSERT(hf_step >= 2u);
      G_ASSERT(h_error != nullptr);

      uint16_t x1 = x0 + hf_step;
      uint16_t y1 = y0 + hf_step;

      float lt = HeightmapUtils::getHeightAtPointUnpack(x0, y0, hmapSize, hmap_handler); // A, lt
      float rt = HeightmapUtils::getHeightAtPointUnpack(x1, y0, hmapSize, hmap_handler); // B, rt
      float lb = HeightmapUtils::getHeightAtPointUnpack(x0, y1, hmapSize, hmap_handler); // C, lb
      float rb = HeightmapUtils::getHeightAtPointUnpack(x1, y1, hmapSize, hmap_handler); // D, rb

      float sumError1 = 0.0f;
      float sumError2 = 0.0f;

      uint32_t quad_size_x = x1 - x0;
      uint32_t quad_size_y = y1 - y0;
      float quad_w = 1.0f / quad_size_x;
      float quad_h = 1.0f / quad_size_y;

      const Point2 c(x0 + quad_size_x * 0.5f, y0 + quad_size_y * 0.5f);
      float midActualValue = (lt + rt + lb + rb) * 0.25f;
      const bool misAlignedCenter = (c.x != floorf(c.x) || c.y != floorf(c.y));

      for (uint32_t hy = y0; hy <= y1; ++hy)
      {
        for (uint32_t hx = x0; hx <= x1; ++hx)
        {
          float true_height = HeightmapUtils::getHeightAtPointUnpack(hx, hy, hmapSize, hmap_handler);

          if (float(hy) == c.y && float(hx) == c.x)
          {
            midActualValue = true_height;
          }
          float xf = (hx - x0) * quad_w;
          float yf = (hy - y0) * quad_h;

          float interpolated1 = (xf <= yf) // LTRB
                                  ? lt * (1.0f - yf) + lb * (yf - xf) + rb * xf
                                  : lt * (1.0f - xf) + rt * (xf - yf) + rb * yf;
          float interpolated2 = (xf <= 1.0f - yf) // RTLB (default)
                                  ? lt * (1.0f - yf - xf) + lb * yf + rt * xf
                                  : rt * (1.0f - yf) + rb * (xf + yf - 1.0f) + lb * (1.0f - xf);

          float error1 = fabs(true_height - interpolated1);
          float error2 = fabs(true_height - interpolated2);

          float error = min(error1, error2);

          if (h_error && abs(error) > abs(h_error->maxError))
          {
            Point3 pos = HeightmapUtils::getPointInBbox(hx, hy, hmapSize, subdivBBox, hmap_handler);
            h_error->maxError = error;
            h_error->maxErrorPosition = pos;
          }

          if ((true_height - water_level) * (interpolated1 - water_level) < 0.0f)
            error1 += 3000.0f;
          if ((true_height - water_level) * (interpolated2 - water_level) < 0.0f)
            error2 += 3000.0f;

          sumError1 += error1;
          sumError2 += error2;
        }
      }

      float error_ltrb = sumError1;
      float error_rtlb = sumError2;

      // use marching quads to find best intersection angle (visible change of shore angle is worser than any other metric)
      ShoreMarchingResult shoreMarch = ShoreMarchingResult::None;
      Point2 waterContour[4];
      const int edges =
        march_square(x0, y0, x0 + quad_size_x, y0 + quad_size_y, lt, rt, lb, rb, midActualValue, water_level, waterContour);
      if (edges)
      {
        // debug("water edge1 %@..%@", waterContour[0], waterContour[1]);
        Point2 dir = normalize(waterContour[1] - waterContour[0]);
        float ltRbAngle = acosf(fabsf(dir.x + dir.y) * sqrtf(0.5f));
        float rtLbAngle = acosf(fabsf(dir.x - dir.y) * sqrtf(0.5f));
        if (edges == 2)
        {
          // debug("water edge2 %@..%@", waterContour[3], waterContour[2]);
          dir = normalize(waterContour[3] - waterContour[2]);
          ltRbAngle += acosf(fabsf(dir.x + dir.y) * sqrtf(0.5f));
          rtLbAngle += acosf(fabsf(dir.x - dir.y) * sqrtf(0.5f));
        }
        const bool waterContourAngleLtRbIsBetter = (ltRbAngle < rtLbAngle), ltRbIsBetter = error_ltrb < error_rtlb;

        if (waterContourAngleLtRbIsBetter)
        {
          shoreMarch = ShoreMarchingResult::LTRB_better;
        }
        else
        {
          shoreMarch = ShoreMarchingResult::RTLB_better;
        }
      }

      uint32_t quadId = quadY * 8 + quadX;
      G_ASSERT(quadId < 64);

      const uint32_t edgeDirBitPos = quadId % 32;

      const bool shouldFlip =
        (shoreMarch == ShoreMarchingResult::LTRB_better) || (shoreMarch == ShoreMarchingResult::None && error_ltrb < error_rtlb);
      if (shouldFlip)
      {
        if (quadId >= 32)
          edge_dir.p2 |= (1u << edgeDirBitPos);
        else
          edge_dir.p1 |= (1u << edgeDirBitPos);
      }
    }
  }

  updateHeightInfoRecursive(hmap_handler, subdiv, next_level, next_x, next_y, &h_range, h_error, rect);
  updateHeightInfoRecursive(hmap_handler, subdiv, next_level, next_x, next_y + 1, &h_range, h_error, rect);
  updateHeightInfoRecursive(hmap_handler, subdiv, next_level, next_x + 1, next_y, &h_range, h_error, rect);
  updateHeightInfoRecursive(hmap_handler, subdiv, next_level, next_x + 1, next_y + 1, &h_range, h_error, rect);

  if (parent_height_range != nullptr)
  {
    parent_height_range->min = eastl::min(parent_height_range->min, h_range.min);
    parent_height_range->max = eastl::max(parent_height_range->max, h_range.max);
  }

  if (h_error != nullptr && parent_height_error != nullptr)
  {
    if (abs(h_error->maxError) > abs(parent_height_error->maxError))
    {
      parent_height_error->maxErrorPosition = h_error->maxErrorPosition;
      parent_height_error->maxError = h_error->maxError;
    }
  }
}

void FlexGridRenderer::prepareSubdivision(int flex_grid_subdiv_id, const Point3 &subdiv_camera_pos, const TMatrix4 &subdiv_camera_view,
  const TMatrix4 &subdiv_camera_proj, const Frustum &view_frustum, const HeightmapHandler &hmap_handler, const Occlusion *occlusion,
  int tess_quality, bool force_low_quality, const FlexGridConfig &cfg)
{
  FlexGridConfig::Metrics metrics;

  bool limitByHeightfield = false;

  if (tess_quality <= 1)
  {
    metrics = cfg.low;
    limitByHeightfield = true;
  }
  else if (tess_quality == 2)
  {
    metrics = cfg.medium;
  }
  else if (tess_quality == 3)
  {
    metrics = cfg.high;
  }
  else if (tess_quality == 4)
  {
    metrics = cfg.ultraHigh;
  }

  const bool isOrthoProjection = fabsf(subdiv_camera_proj._44 - 1.0f) < flexGrid::EPSILON;
  if (force_low_quality || isOrthoProjection)
  {
    metrics = cfg.forceLow;
    limitByHeightfield = true;
  }

  if (convar::metrics_patch_radius > 0.0f)
  {
    metrics.maxPatchRadiusError = convar::metrics_patch_radius;
  }
  if (convar::metrics_height > 0.0f)
  {
    metrics.maxHeightError = convar::metrics_height;
  }
  if (convar::metrics_abs_height > 0.0f)
  {
    metrics.maxAbsoluteHeightError = convar::metrics_abs_height;
  }

  const FlexGridSubdivision::SubdivisionInstance &subdivision_instance =
    subdivision.prepare(&heightfieldSubdivAsset, flex_grid_subdiv_id, subdiv_camera_pos, subdiv_camera_view, subdiv_camera_proj,
      view_frustum, hmap_handler, occlusion, metrics, limitByHeightfield, cfg.minSubdivLevel, cfg);
}

void FlexGridRenderer::processSubdivisionHierarchically(const FlexGridSubdivision::Patch *subdiv_root,
  const FlexGridSubdivision::Patch *patch, InstanceData *&instance_buffer_data, uint32_t *instance_count,
  uint32_t *total_instance_count, const FlexGridConfig &cfg)
{
  if (patch == nullptr)
    return;

  if (patch->isTerminated)
  {
    // todo: uncomment after tuning
    // G_ASSERTF_RETURN(*total_instance_count < cfg.maxInstanceCount, , "total_instance_count %d is more than maximum %d",
    //  *total_instance_count, cfg.maxInstanceCount);

    if (*instance_count >= FLEXGRID_MAX_INSTANCES_PER_DRAW)
    {
      d3d::set_vs_const(FLEXGRID_INSTANCE_DATA_CB_REG, &instanceData[0], FLEXGRID_MAX_REGS);
      if (convar::flex_grid_nonindexed.get())
        d3d_err(
          d3d::draw_instanced(PRIM_TRILIST, 0, FLEXGRID_PATCH_QUADS * FLEXGRID_PATCH_QUADS * 2, FLEXGRID_MAX_INSTANCES_PER_DRAW));
      else
        d3d_err(d3d::drawind_instanced(PRIM_TRILIST, 0, patchTriangleCount, 0, FLEXGRID_MAX_INSTANCES_PER_DRAW));

      *instance_count = 0;
      instance_buffer_data = &instanceData[0];
    }

    processTerminatedPatch(subdiv_root, patch, instance_buffer_data);
    ++instance_buffer_data;
    ++(*instance_count);
    ++(*total_instance_count);
  }
  else
  {
    for (const FlexGridSubdivision::Patch *child : patch->child)
      processSubdivisionHierarchically(subdiv_root, child, instance_buffer_data, instance_count, total_instance_count, cfg);
  }
}

inline float morphRemap(float x)
{
  x = 1.f - x;

  float x2 = x * x;
  x2 = x2 * x2;

  return x2 * (x * 4.f - 5.f) + 1.f;
}

void FlexGridRenderer::processTerminatedPatch(const FlexGridSubdivision::Patch *subdiv_root, const FlexGridSubdivision::Patch *patch,
  InstanceData *instance_buffer_data)
{
  uint32_t level = patch->level;
  uint32_t x = patch->x;
  uint32_t y = patch->y;
  float levelSize = float(FlexGridSubdivisionUtils::get_level_size(level));

  // patch data
  instance_buffer_data->patchOffset = Point2(float(x), float(y)) / levelSize;
  instance_buffer_data->patchScale = 1.f / levelSize;

  const float patchMorph = patch->interjacentValue;
  instance_buffer_data->patchMorph = morphRemap(patchMorph);

  // neighbor data
  const FlexGridSubdivision::Patch *neg_x = findLastTerminatedPatch(subdiv_root, level, x - 1, y);
  const FlexGridSubdivision::Patch *neg_y = findLastTerminatedPatch(subdiv_root, level, x, y - 1);
  const FlexGridSubdivision::Patch *pos_x = findLastTerminatedPatch(subdiv_root, level, x + 1, y);
  const FlexGridSubdivision::Patch *pos_y = findLastTerminatedPatch(subdiv_root, level, x, y + 1);

  float neg_x_morph = neg_x ? neg_x->interjacentValue : patchMorph;
  float neg_y_morph = neg_y ? neg_y->interjacentValue : patchMorph;
  float pos_x_morph = pos_x ? pos_x->interjacentValue : patchMorph;
  float pos_y_morph = pos_y ? pos_y->interjacentValue : patchMorph;

  uint32_t neg_x_level = neg_x ? neg_x->level : level;
  uint32_t neg_y_level = neg_y ? neg_y->level : level;
  uint32_t pos_x_level = pos_x ? pos_x->level : level;
  uint32_t pos_y_level = pos_y ? pos_y->level : level;

  neg_x_morph = (neg_x_level < level) ? neg_x_morph : eastl::min(neg_x_morph, patchMorph);
  neg_y_morph = (neg_y_level < level) ? neg_y_morph : eastl::min(neg_y_morph, patchMorph);
  pos_x_morph = (pos_x_level < level) ? pos_x_morph : eastl::min(pos_x_morph, patchMorph);
  pos_y_morph = (pos_y_level < level) ? pos_y_morph : eastl::min(pos_y_morph, patchMorph);

  instance_buffer_data->neighborLodOffset.x = level - neg_x_level;
  instance_buffer_data->neighborLodOffset.y = level - neg_y_level;
  instance_buffer_data->neighborLodOffset.z = level - pos_x_level;
  instance_buffer_data->neighborLodOffset.w = level - pos_y_level;

  instance_buffer_data->neighborPatchMorph.x = morphRemap(neg_x_morph);
  instance_buffer_data->neighborPatchMorph.y = morphRemap(neg_y_morph);
  instance_buffer_data->neighborPatchMorph.z = morphRemap(pos_x_morph);
  instance_buffer_data->neighborPatchMorph.w = morphRemap(pos_y_morph);

  instance_buffer_data->edgesDirectionPacked1 = patch->edgeDirPacked1;
  instance_buffer_data->edgesDirectionPacked2 = patch->edgeDirPacked2;
  instance_buffer_data->parentEdgeDataOffset = patch->parentEdgeDataOffset;
  instance_buffer_data->parentEdgeDataScale = patch->parentEdgeDataScale;
}

const FlexGridSubdivision::Patch *FlexGridRenderer::findLastTerminatedPatch(const FlexGridSubdivision::Patch *subdiv_root,
  uint32_t level, uint32_t x, uint32_t y)
{
  uint32_t level_size = FlexGridSubdivisionUtils::get_level_size(level);
  if (x >= level_size || y >= level_size || subdiv_root == nullptr)
    return nullptr;

  const FlexGridSubdivision::Patch *current_patch = subdiv_root;

  uint32_t check_mask = level_size >> 1;
  for (uint32_t i = 0; i < level; ++i)
  {
    uint32_t child_index = (x & check_mask) | ((y & check_mask) << 1);
    child_index = child_index >> (level - i - 1);

    if (!current_patch->child[child_index])
      break;

    current_patch = current_patch->child[child_index];
    check_mask = check_mask >> 1;
  }
  G_ASSERTF(current_patch != nullptr, "FlexGridRenderer::findLastTerminatedPatch");

  return (current_patch->isTerminated) ? current_patch : nullptr;
}

void FlexGridRenderer::render(int flex_grid_subdiv_uid, const FlexGridConfig &cfg, bool logDebug)
{
  const FlexGridSubdivision::SubdivisionInstance &subdivision_instance = subdivision.getPreparedSubdivision(flex_grid_subdiv_uid);

  if (!shElem->setStates(0, true) || !ib || !vb)
    return;

  if (convar::flex_grid_nonindexed.get())
    d3d_err(d3d::setvsrc(0, vb_nonindexed.getBuf(), sizeof(VertexData)));
  else
  {
    d3d_err(d3d::setind(ib.getBuf()));
    d3d_err(d3d::setvsrc(0, vb.getBuf(), sizeof(VertexData)));
  }

  d3d::set_vs_constbuffer_size(FLEXGRID_INSTANCE_DATA_CB_REG + FLEXGRID_MAX_REGS);
  uint32_t instanceCount = 0;
  uint32_t totalInstanceCount = 0;

  InstanceData *instanceDataPtr = &instanceData[0];
  processSubdivisionHierarchically(subdivision_instance.root, subdivision_instance.root, instanceDataPtr, &instanceCount,
    &totalInstanceCount, cfg);

  if (instanceCount > 0)
  {
    d3d::set_vs_const(FLEXGRID_INSTANCE_DATA_CB_REG, &instanceData[0], instanceCount * FLEXGRID_REGISTERS_PER_INSTANCE);
    if (convar::flex_grid_nonindexed.get())
      d3d_err(d3d::draw_instanced(PRIM_TRILIST, 0, FLEXGRID_PATCH_QUADS * FLEXGRID_PATCH_QUADS * 2, instanceCount));
    else
      d3d_err(d3d::drawind_instanced(PRIM_TRILIST, 0, patchTriangleCount, 0, instanceCount));
  }

  d3d_err(d3d::setvsrc(0, 0, 0));
  d3d_err(d3d::setvsrc(1, 0, 0));
  d3d::set_vs_constbuffer_size(0);

  if (logDebug)
  {
    debugMaxTris = eastl::max(debugMaxTris, totalInstanceCount * patchTriangleCount);
    debugFrameCounter++;
    if (debugFrameCounter % 500 == 0)
    {
      visuallog::logmsg(String(0, "[FlexGridRenderer] max tris for last couple of secs: %d k", debugMaxTris / 1000));
      debugFrameCounter = 0;
      debugMaxTris = 0;
    }
  }
}

void FlexGridRenderer::close()
{
  if (!shmat)
    return;
  if (shElem)
    d3d::delete_vdecl(shElem->getEffectiveVDecl());
  del_it(shmat);
  shElem = NULL; // Deleted in shmat

  initialized = false;
}
