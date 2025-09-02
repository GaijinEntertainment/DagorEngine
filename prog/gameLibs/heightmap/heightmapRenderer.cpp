// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_critSec.h>
#include <heightmap/lodGrid.h>
#include <heightmap/heightmapRenderer.h>
#include <heightmap/heightmapHandler.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_info.h>
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
#include <util/dag_bitwise_cast.h>

constexpr uint32_t SUBPATCHES_DEPTH = 1;
// Current implementation supports SUBPATCHES_DEPTH not bigger than 3.

CONSOLE_BOOL_VAL("heightmap", use_hw_tesselation, true);

namespace var
{
static ShaderVariableInfo hmap_object_tess_factor("hmap_object_tess_factor", true);
}

enum
{
  MAX_HW_INSTANCING = 440,
  VDATA_OFS = 3,
  MAX_VDATA = 5
}; // bones

static LodGridVertexData vdata[MAX_VDATA];


static void generate_patch_vertices(int dim, int xofs, int yofs, LodGridVertex *vertex)
{
  for (int y = 0; y <= dim; ++y)
    for (int x = 0; x <= dim; ++x)
    {
      vertex[y * (dim + 1) + x].x = x + xofs;
      vertex[y * (dim + 1) + x].y = y + yofs;
    }
}

template <class It>
static inline int generate_patch_indices_quads(int dim, It &&indices, int startInd)
{
  size_t index = 0;
  for (int y = 0; y < dim; ++y)
    for (int x = 0; x < dim; ++x)
    {
      const int topleft = y * (dim + 1) + x + startInd;
      const int downleft = topleft + (dim + 1);
      const int downright = topleft + (dim + 1) + 1;
      const int topright = topleft + 1;
      indices[index + 0] = topleft;
      indices[index + 1] = downleft;
      indices[index + 2] = downright;
      indices[index + 3] = topright;
      index += 4;
    }
  return index;
}

bool get_hw_tesselation_usage(int hmap_tess_factorVarId, const LodGridCullData &cull_data)
{
  return (hmap_tess_factorVarId != -1) && cull_data.useHWTesselation && use_hw_tesselation.get();
}

static int get_tessellation_var_id(const char *hmap_tess_factor_name)
{
  if (d3d::get_driver_desc().caps.hasQuadTessellation)
  {
    int id = ::get_shader_variable_id(hmap_tess_factor_name, /*opt*/ true);
    if (VariableMap::isGlobVariablePresent(id))
      return id;
  }
  return -1;
}

void LodGridVertexData::close()
{
  if (!refCnt)
    return;
  if (interlocked_decrement(refCnt) > 0)
    return;
  del_d3dres(ib);
  del_d3dres(quadsIb);
  patchDim = 0;
}

bool LodGridVertexData::init(int dim)
{
  if (interlocked_increment(refCnt) > 1)
  {
    G_ASSERT(patchDim == dim);
    return true;
  }
  patchDim = dim;

  return createBuffers();
}

bool LodGridVertexData::createBuffers()
{
  recreateBuffers = false;
  verticesCnt = (patchDim + 1) * (patchDim + 1);

  const int indexSize = 2;
  indicesCnt = patchDim * patchDim * 6;
  int totalIndicesCnt = indicesCnt * 4;
  G_ASSERT(!ib);
  ib = d3d::create_ib(totalIndicesCnt * indexSize, (indexSize == 4 ? SBCF_INDEX32 : 0), "lod_grid_vdata_ib");
  d3d_err(ib);
  if (!ib)
    return false;
  if (auto lockedIndices = lock_sbuffer<uint16_t>(ib, 0, 0, VBLOCK_WRITEONLY))
  {
    faceCnt = generate_patch_indices(patchDim, lockedIndices, 0) / 3;
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt), 1);
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt * 2), 1, 0); // REGULAR_RTLB
    generate_patch_indices(patchDim, LockedBufferWithOffset(lockedIndices, indicesCnt * 3), 0, 0); // REGULAR_LTRB
  }
  else
    return false;

  if (d3d::get_driver_desc().caps.hasQuadTessellation)
  {
    quadsIndicesCnt = indicesCnt / 6 * 4;
    G_ASSERT(!quadsIb);
    quadsIb = d3d::create_ib(quadsIndicesCnt * indexSize, (indexSize == 4 ? SBCF_INDEX32 : 0), "lod_grid_vdata_quadsIb");
    d3d_err(quadsIb);
    if (!quadsIb)
      return false;
    if (auto lockedIndices = lock_sbuffer<uint16_t>(quadsIb, 0, 0, VBLOCK_WRITEONLY))
      quadsCnt = generate_patch_indices_quads(patchDim, lockedIndices, 0) / 4;
    else
      return false;
  }

  debug("heightmap will be rendered using instanceId instancing");
  ShaderGlobal::set_int(get_shader_variable_id("heightmap_use_instancing", true), 1);
  return true;
}

void LodGridVertexData::beforeResetDevice()
{
  if (patchDim <= 0)
    return;

  recreateBuffers = true;
  del_d3dres(ib);
  del_d3dres(quadsIb);
}

void LodGridVertexData::afterResetDevice()
{
  if (recreateBuffers)
    createBuffers();
}

static int heightmap_scale_offset_varId = -1;
static int heightmap_scale_offset_c = 0;
static int heightmap_region_gvid = -1;
static int lod0_centerVarId = -1;
static int worldToHmapLod0VarId = -1;

HeightmapRenderer::HeightmapRenderer(int bits) : shmat(NULL), shElem(NULL)
{
  dimBits = clamp(bits - VDATA_OFS, 0, MAX_VDATA - 1) + VDATA_OFS;
}
void HeightmapRenderer::close()
{
  if (!shmat)
    return;
  shElem = NULL; // Deleted in shmat
  del_it(shmat);
  vdata[dimBits - VDATA_OFS].close();
}

void HeightmapRenderer::beforeResetDevice()
{
  for (int i = 0; i < MAX_VDATA; ++i)
    vdata[i].beforeResetDevice();
}

void HeightmapRenderer::afterResetDevice()
{
  for (int i = 0; i < MAX_VDATA; ++i)
    vdata[i].afterResetDevice();
}

bool HeightmapRenderer::init(const char *shader_name, const char *mat_script, const char *hmap_tess_factor_name, bool do_fatal,
  int bits)
{
  dimBits = clamp(bits - VDATA_OFS, 0, MAX_VDATA - 1) + VDATA_OFS;
  heightmap_scale_offset_varId = get_shader_variable_id("heightmap_scale_offset");
  heightmap_region_gvid = get_shader_glob_var_id("heightmap_region", true);
  heightmap_scale_offset_c = ShaderGlobal::get_int_fast(heightmap_scale_offset_varId);
  lod0_centerVarId = get_shader_variable_id("lod0_center", true);
  worldToHmapLod0VarId = get_shader_variable_id("worldToHmapLod0", true);
  hmap_tess_factorVarId = get_tessellation_var_id(hmap_tess_factor_name);

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
  if (!vdata[dimBits - VDATA_OFS].init(1 << dimBits)) // || !vdata[dimBits-VDATA_OFS].vb)
  {
    if (do_fatal)
      DAG_FATAL("can not init heightmap buffers");
    return false;
  }
  return true;
}

void HeightmapRenderer::setRenderClip(const BBox2 *clip) const
{
  Color4 c =
    clip ? Color4(clip->left(), clip->top(), clip->right(), clip->bottom()) : Color4(-(128 << 20), -(128 << 20), 128 << 20, 128 << 20);
  ShaderGlobal::set_color4_fast(heightmap_region_gvid, c);
}

void HeightmapRenderer::renderPatchesByBatches(dag::ConstSpan<LodGridPatchParams> patches, const int buffer_size, const int vDataIndex,
  int startFlipped, const bool render_quads, int primitiveCount, int startInd) const
{
  int nextStartInd = 0;
  for (int patch = 0, total_instances = patches.size(); total_instances > 0;)
  {
    int current_batch_size = min(total_instances, buffer_size);
    int toFlipped = startFlipped - patch;
    if (current_batch_size > toFlipped)
    {
      current_batch_size = toFlipped;
      startFlipped = 10000000;
      nextStartInd = render_quads ? vdata[vDataIndex].quadsIndicesCnt : vdata[vDataIndex].indicesCnt;
    }

    if (current_batch_size)
    {
      d3d::set_vs_const(heightmap_scale_offset_c, &patches[patch].params.x, current_batch_size);
      if (render_quads)
        d3d::drawind_instanced(PRIM_4_CONTROL_POINTS, startInd, primitiveCount, 0, current_batch_size);
      else
      {
        d3d::drawind_instanced(PRIM_TRILIST, startInd, primitiveCount, 0, current_batch_size);
        // no difference performance wise on Xb1 or XbSX
        // d3d::draw_instanced(PRIM_TRILIST, startInd, primitiveCount, current_batch_size);
      }
    }

    total_instances -= current_batch_size;
    patch += current_batch_size;
    startInd = nextStartInd;
  }
}

void HeightmapRenderer::render(const LodGrid &lodGrid, const LodGridCullData &cull_data, LodGridVertexData *vData, int vDataDim) const
{
  int vDataIndex = dimBits - VDATA_OFS;
  if (!shElem || !cull_data.hasPatches())
    return;
  d3d::setvsrc_ex(0, NULL, 0, 0);
  TIME_D3D_PROFILE(heightmap);

  ShaderGlobal::set_color4(worldToHmapLod0VarId, cull_data.worldToLod0);

  const bool hwTesselationUsage = get_hw_tesselation_usage(hmap_tess_factorVarId, cull_data);
  ShaderGlobal::set_real(var::hmap_object_tess_factor, hwTesselationUsage ? 1 : 0);

  if (cull_data.hasPatches())
  {
    const int maxReqInstances = MAX_HW_INSTANCING;
    const int buffer_size = d3d::set_vs_constbuffer_size(maxReqInstances + heightmap_scale_offset_c) - heightmap_scale_offset_c;
    int dim = vDataDim >= 0 ? vDataDim : getDim();
    LodGridVertexData *vdataPtr = vData ? vData : &vdata[vDataIndex];
    d3d::set_vs_const1(heightmap_scale_offset_c - 1, dim, bitwise_cast<float>(dim + 1), cull_data.scaleX,
      bitwise_cast<float>(get_log2i(dim)));

    G_ASSERT(cull_data.lod0PatchesCount <= cull_data.startFlipped);
    const bool renderQuads = hwTesselationUsage && cull_data.lod0PatchesCount && lodGrid.lod0SubDiv >= 1;
    if (hwTesselationUsage)
    {
      if (cull_data.frustum.has_value())
        set_frustum_planes(cull_data.frustum.value());
    }

    if (renderQuads)
    {
      d3d::setind(vdataPtr->quadsIb);
      ShaderGlobal::set_real_fast(hmap_tess_factorVarId, 1 << lodGrid.lod0SubDiv);
      ShaderGlobal::set_color4_fast(lod0_centerVarId, Color4(cull_data.originPos.x, cull_data.originPos.y, 0, 0));
      if (!shElem->setStates(0, true))
        return;
      TIME_D3D_PROFILE(lod0_hw_tessellation);
      dag::ConstSpan<LodGridPatchParams> lod0Patches = make_span_const(cull_data.patches).first(cull_data.lod0PatchesCount);
      renderPatchesByBatches(lod0Patches, buffer_size, vDataIndex, cull_data.startFlipped, renderQuads,
        renderQuads ? vdataPtr->quadsCnt : vdataPtr->faceCnt);
    }

    const uint32_t renderedQuads = (renderQuads ? cull_data.lod0PatchesCount : 0);
    d3d::setind(vdataPtr->ib);
    ShaderGlobal::set_real_fast(hmap_tess_factorVarId, 1.f);
    if (!shElem->setStates(0, true))
      return;
    dag::ConstSpan<LodGridPatchParams> otherLodsPatches =
      make_span_const(cull_data.patches).last(cull_data.getCount() - renderedQuads);
    renderPatchesByBatches(otherLodsPatches, buffer_size, vDataIndex, cull_data.startFlipped - renderedQuads, false,
      vdataPtr->faceCnt);
    int startIndex = vdata[vDataIndex].indicesCnt;
    for (int i = 0; i < cull_data.additionalTriPatches.size(); ++i)
    {
      renderPatchesByBatches(make_span_const(cull_data.additionalTriPatches[i]), buffer_size, vDataIndex,
        cull_data.additionalTriPatches[i].size(), false, vdataPtr->faceCnt, startIndex);
      startIndex += vdata[vDataIndex].indicesCnt;
    }

    d3d::set_vs_constbuffer_size(0);
  }
  d3d::setvsrc_ex(1, NULL, 0, 0);
}

void HeightmapRenderer::renderOnePatch(const Point2 &left_top, const Point2 &bottom_right) const
{
  int vDataIndex = dimBits - VDATA_OFS;
  if (!shElem)
    return;
  d3d::setvsrc_ex(0, NULL, 0, 0);
  d3d::setind(vdata[vDataIndex].ib);
  if (!shElem->setStates(0, true))
    return;
  TIME_D3D_PROFILE(heightmapOnePatch);
  G_ASSERT(fabs((bottom_right.y - left_top.y) - (bottom_right.x - left_top.x)) < 0.00001);
  Point4 oneConst = Point4((bottom_right.x - left_top.x), 0, left_top.x, left_top.y);
  d3d::set_vs_const1(heightmap_scale_offset_c - 1, getDim(), bitwise_cast<float>(getDim() + 1), 0,
    bitwise_cast<float>(get_log2i(getDim())));
  d3d::set_vs_const(heightmap_scale_offset_c, &oneConst.x, 1);
  d3d::drawind(PRIM_TRILIST, 0, 2, 0);
  d3d::setvsrc_ex(1, NULL, 0, 0);
}

void HeightmapRenderer::renderEmpty() const
{
  int vDataIndex = dimBits - VDATA_OFS;
  if (!shElem)
    return;
  d3d::setvsrc_ex(0, NULL, 0, 0);
  d3d::setind(vdata[vDataIndex].ib);
  if (!shElem->setStates(0, true))
    return;
  TIME_D3D_PROFILE(heightmapOnePatch);
  Point4 oneConst = Point4(0, 0, -1000000, -1000000);
  d3d::set_vs_const1(heightmap_scale_offset_c - 1, getDim(), bitwise_cast<float>(getDim() + 1), 0,
    bitwise_cast<float>(get_log2i(getDim())));
  d3d::set_vs_const(heightmap_scale_offset_c, &oneConst.x, 1);
  d3d::drawind(PRIM_TRILIST, 0, 1, 0);
}

struct GridCullingContext
{
  const BBox2 *clip;
  const Frustum *frustum;
  const Occlusion *use_occlusion;
  float hMin, hMax;
};

static inline bool cull_node_by_clip(const Point3_vec4 &pos, const Point3_vec4 &posRB, const GridCullingContext &ctx)
{
  if (ctx.clip)
  {
    if (posRB.x <= ctx.clip->left() || pos.x >= ctx.clip->right() || posRB.z <= ctx.clip->top() || pos.z >= ctx.clip->bottom())
      return false;
  }
  return true;
}

static inline bool cull_node(const Point3_vec4 &pos, const Point3_vec4 &posRB, const GridCullingContext &ctx, float waterLevel,
  const Point3 *viewPos)
{
  if (ctx.frustum)
  {
    vec4f bmin = v_ld(&pos.x), bmax = v_ld(&posRB.x);
    vec4f center2 = v_add(bmax, bmin);
    vec4f extent2 = v_sub(bmax, bmin);
    if (!ctx.frustum->testBoxExtentB(center2, extent2))
      return false;
    if (ctx.use_occlusion && ctx.use_occlusion->isOccludedBoxExtent2(center2, extent2))
      return false;
  }

  bool isWaterOnLevel = waterLevel > HeightmapHeightCulling::NO_WATER_ON_LEVEL;
  if (isWaterOnLevel && viewPos)
  {
    static constexpr float SWIMMING_MAX_CAMERA_HEIGHT_ABOVE_WATER = 0.2f;
    bool isPlayerSwimming = viewPos->y < waterLevel + SWIMMING_MAX_CAMERA_HEIGHT_ABOVE_WATER;
    if (!isPlayerSwimming)
    {
      Point3 closestPatchPoint =
        Point3(clamp(viewPos->x, pos.x, posRB.x), clamp(viewPos->y, pos.y, posRB.y), clamp(viewPos->z, pos.z, posRB.z));
      Point3 patchToEye = *viewPos - closestPatchPoint;
      float distSquared = patchToEye.lengthSq();

      // Threshold incidence angle is approximately 73 degrees:
      static constexpr float THRESHOLD_INCIDENCE_ANGLE_SINE_SQUARED = 0.3f * 0.3f;
      static constexpr float THRESHOLD_DIST_SQUARED_BY_SINE_SQUARED = 20.f * 20.f * THRESHOLD_INCIDENCE_ANGLE_SINE_SQUARED;
      static constexpr float CAMERA_HEIGHT_FACTOR = 1.f / 3.f;

      // Choosing height difference between camera and the heighest point of the patch guarantees the lowest sine of incidence angle,
      // and, therefore, lowest incidence angle. That means that we only cull away the patch if all of the points are not visible:
      float minHeightDifference = max(viewPos->y - posRB.y, 0.f);
      // Angle of incidence with water level surface is greater than the threshold angle and the point is further from the camera than
      // threshold distance:
      if (THRESHOLD_INCIDENCE_ANGLE_SINE_SQUARED * distSquared >
            max(minHeightDifference * minHeightDifference, THRESHOLD_DIST_SQUARED_BY_SINE_SQUARED)
          // Point under water is below certain depth, which is increasing with camera position height:
          && closestPatchPoint.y < waterLevel - patchToEye.y * CAMERA_HEIGHT_FACTOR)
        return false;
    }
  }

  return true;
}

static inline uint32_t bit_pack_ipoint2_to_uint(const IPoint2 &pos)
{
  int16_t pp[2]{(int16_t)pos.x, (int16_t)pos.y};
  uint32_t packed_pos;
  memcpy(&packed_pos, &pp[0], sizeof(uint32_t));
  return packed_pos;
}

static inline IPoint2 bit_unpack_uint_to_ipoint2(const uint32_t &input)
{
  int16_t pp[2];
  memcpy(&pp[0], &input, sizeof(uint32_t));
  return IPoint2(pp[0], pp[1]);
}

static inline uint32_t mask_0_LSb(int bits_count) { return ~((1 << bits_count) - 1); }

static inline uint32_t edge_tess_encode(int lod_difference)
{
  return min(lod_difference, 15); // fits range to 4bit [0, 15] range.
}
static inline uint32_t uint_pack_edges_tess(int edgeTessOut[4])
{
  // Pack 4 x 4bit = 16 bit of information in a integer and then static_cast it as float. Shader unpacks edge tesselation.
  // Note: float has 23 bit of mantissa + 1 implicit bit. Thus, up to 24 bits can be packed using this trick.
  int packed_edge_tess = edgeTessOut[0] | (edgeTessOut[1] << 4) | (edgeTessOut[2] << 8) | (edgeTessOut[3] << 12);
  return packed_edge_tess;
}

void cull_lod_grid(const LodGrid &lodGrid, int maxLod, float originPosX, float originPosY, float scaleX, float scaleY, float alignX,
  float alignY, float hMin, float hMax, const Frustum *frustum, const BBox2 *clip, LodGridCullData &cull_data,
  const Occlusion *use_occlusion, float &out_lod0_area_radius, int hmap_tess_factorVarId, int dim, bool fight_t_junctions,
  const HeightmapHeightCulling *heightCulling, const HMapTesselationData *hmap_tdata, BBox2 *innerLodsRegion, float waterLevel,
  const Point3 *viewPos)
{
  G_UNREFERENCED(fight_t_junctions);
  G_ASSERT(scaleX == scaleY);
  if (alignX < 0)
    alignX = dim * scaleX;
  if (alignY < 0)
    alignY = dim * scaleY;

  cull_data.originPos = Point2(floorf(originPosX / alignX + 0.5f) * alignX, floorf(originPosY / alignY + 0.5f) * alignY);
  int patchesX = floorf(originPosX / alignX + 0.5f), patchesY = floorf(originPosY / alignY + 0.5f);
  cull_data.scaleX = scaleX;

  GridCullingContext ctx;

  if (!frustum)
    heightCulling = NULL;

  if (innerLodsRegion)
    innerLodsRegion->setempty();

  ctx.clip = clip;
  ctx.frustum = frustum;
  ctx.use_occlusion = use_occlusion;
  ctx.hMin = hMin;
  ctx.hMax = hMax;
  cull_data.eraseAll();
  int numLods = min<int>(lodGrid.lodsCount, maxLod);
  int lod0SubDiv = !get_hw_tesselation_usage(hmap_tess_factorVarId, cull_data) ? lodGrid.lod0SubDiv : 0;
  int flipLod = ((patchesX + patchesY) & 1) ? get_log2w(dim) : 100000;
  cull_data.startFlipped = 1000000;
  out_lod0_area_radius = 0.f;
  alignas(16) struct Area
  {
    int minX = -1000000, minY = -1000000, maxX = 1000000, maxY = 1000000;
    void nextLod()
    {
      minX >>= 1;
      minY >>= 1;
      maxX = maxX >> 1;
      maxY = maxY >> 1;
    }
  } area;
  if (clip || frustum)
  {
    vec4f worldBboxXZ;
    if (frustum)
    {
      bbox3f frustumBox;
      frustum->calcFrustumBBox(frustumBox);
      worldBboxXZ = v_perm_xzac(frustumBox.bmin, frustumBox.bmax);
    }

    if (clip)
    {
      vec4f clipBox = v_ldu(&clip->lim[0].x);
      if (!frustum)
        worldBboxXZ = clipBox;
      else
        worldBboxXZ = v_perm_xycd(v_max(worldBboxXZ, clipBox), v_min(worldBboxXZ, clipBox));
    }

    const float patchSize0 = scaleX * dim;
    vec4f startCull = v_make_vec4f(cull_data.originPos.x, cull_data.originPos.y, 0, 0);
    vec4f regionV = v_div(v_sub(worldBboxXZ, v_perm_xyxy(startCull)), v_splats(patchSize0));
    vec4i regionI = v_cvt_floori(regionV);
    v_sti(&area, regionI);
  }

  struct IPoint2Hasher
  {
    typedef ska::power_of_two_hash_policy hash_policy;
    auto operator()(const IPoint2 &p) const { return wyhash64(p.x, p.y); }
  };
  // Using integer coordinates to resolve edge tesselation. 1 unit is equal to LOD 0's gridSize.
  // Because LOD 0 can use subdivision (for software tesselation), for LOD > 0 coordinates should shift left by `lod0SubDiv`.
  ska::flat_hash_map<IPoint2, int, IPoint2Hasher, eastl::equal_to<IPoint2>, framemem_allocator> posLOD_map;
  for (int nodeLodSize = 1, lod = 0; lod < numLods; ++lod, nodeLodSize <<= 1)
  {
    if (lod == flipLod)
      cull_data.startFlipped = cull_data.getCount();
    int lodRad = lod == (numLods - 1) ? lodGrid.lastLodRad : lodGrid.lodStep;
    int subdiv = lod == 0 ? lod0SubDiv : 0;
    float gridSize = nodeLodSize * scaleX / (1 << subdiv);
    float patchSize = gridSize * dim;
    int rad1 = (lodRad << 1) << subdiv;
    int prevRad1 = lod == 0 ? -1000000 : (lodGrid.lodStep);
    int prevRad2 = prevRad1 - 1;
    int heightLod = 0;
    float lodArea = patchSize * rad1;

    if (lod == 0)
    {
      const int mnX = max(-rad1, area.minX << subdiv), mxX = min(rad1, (area.maxX + 1) << subdiv),
                mnY = max(-rad1, area.minY << subdiv), mxY = min(rad1, (area.maxY + 1) << subdiv);
      out_lod0_area_radius = lodArea;
      int cullStep = 0;
      if (heightCulling) // no need to calculate individual minmax for all patches
      {
        float chunkSize = heightCulling->getChunkSize();
        cullStep = clamp((int)floorf(chunkSize / patchSize), 1, 2 * rad1);
        heightLod = heightCulling->getLod(patchSize * cullStep);
      }
      else
      {
        cullStep = 2 * rad1;
      }

      if (mxX - mnX > 0 && mxY - mnY > 0)
        cull_data.worldToLod0 = Point4(cull_data.originPos.x + patchSize * mnX, cull_data.originPos.y + patchSize * mnY,
          1.0f / (patchSize * (mxX - mnX)), 1.0f / (patchSize * (mxY - mnY)));
      else
        cull_data.worldToLod0 = Point4::ZERO;

      // todo: front to back generating
      for (int y = mnY; y < mxY; y += cullStep)
      {
        for (int x = mnX; x < mxX; x += cullStep)
        {
          if (heightCulling)
            heightCulling->getMinMax(heightLod, cull_data.originPos + Point2(x, y) * patchSize, patchSize * cullStep, ctx.hMin,
              ctx.hMax);

          int mcY = min(y + cullStep, rad1);
          int mcX = min(x + cullStep, rad1);
          for (int chunkY = y; chunkY < mcY; chunkY++)
          {
            for (int chunkX = x; chunkX < mcX; chunkX++)
            {
              Point2 origin = cull_data.originPos + Point2(chunkX, chunkY) * patchSize;

              Point3_vec4 pos(origin.x, ctx.hMin, origin.y);
              Point3_vec4 posRB(pos.x + patchSize, ctx.hMax, pos.z + patchSize);

              if (innerLodsRegion && lod != numLods - 1)
                (*innerLodsRegion) += BBox2(Point2(pos.x, pos.z), Point2(posRB.x, posRB.z));

              // Water stretches last LOD edges to fill far water.
              // pos and posRB are extented to cull far water correctly.
              if (lod == numLods - 1)
              {
                if (x == mnX && chunkX == x)
                  pos.x -= lodGrid.lastLodExtension;
                if (y == mnY && chunkY == y)
                  pos.z -= lodGrid.lastLodExtension;
                if ((x + cullStep) >= mxX && chunkX == (mcX - 1))
                  posRB.x += lodGrid.lastLodExtension;
                if ((y + cullStep) >= mxY && chunkY == (mcY - 1))
                  posRB.z += lodGrid.lastLodExtension;
              }

              if (cull_node_by_clip(pos, posRB, ctx) && cull_node(pos, posRB, ctx, waterLevel, viewPos))
              {
                IPoint2 patchPos = IPoint2(chunkX, chunkY);
                cull_data.patches.push_back(
                  LodGridPatchParams(gridSize, bit_pack_ipoint2_to_uint(IPoint2(chunkX, chunkY)), origin.x, origin.y));
                // Add patchPos-LOD pair to hash map.
                // If software tesselation is used, LOD 0 subpatches have effective LOD of `-subdiv`.
                posLOD_map[patchPos] = -subdiv;
              }
            }
          }
        }
      }
      cull_data.lod0PatchesCount = cull_data.getCount();
    }
    else // other lods
    {
      const int mnX = max(-rad1, area.minX), mxX = min(rad1, area.maxX + 1), mnY = max(-rad1, area.minY),
                mxY = min(rad1, area.maxY + 1);
      if (heightCulling)
        heightLod = heightCulling->getLod(patchSize);

      // todo: front to back generating
      for (int y = mnY; y < mxY; y++)
      {
        for (int x = mnX; x < mxX; x++)
        {
          if (x >= -prevRad1 && x <= prevRad2 && y >= -prevRad1 && y <= prevRad2)
            continue;

          Point2 origin = cull_data.originPos + Point2(x, y) * patchSize;

          Point3_vec4 pos(origin.x, ctx.hMin, origin.y);
          Point3_vec4 posRB(pos.x + patchSize, ctx.hMax, pos.z + patchSize);

          if (lod == numLods - 1)
          {
            if (x == mnX)
              pos.x -= lodGrid.lastLodExtension;
            if (y == mnY)
              pos.z -= lodGrid.lastLodExtension;
            if (x == (mxX - 1))
              posRB.x += lodGrid.lastLodExtension;
            if (y == (mxY - 1))
              posRB.z += lodGrid.lastLodExtension;
          }

          // Note: We can also cull at subpatches. In such case, make sure BBox is extended by `lodGrid.lastLodExtension` at edges.
          if (!cull_node_by_clip(pos, posRB, ctx))
            continue;

          if (heightCulling)
          {
            heightCulling->getMinMax(heightLod, origin, patchSize, ctx.hMin, ctx.hMax);
            pos.y = ctx.hMin;
            posRB.y = ctx.hMax;
          }

          // Note: At `if (lod == 0)` branch, BBox2 innerLodsRegion filling is before `cull_node_by_clip` (fix-me?)
          if (innerLodsRegion && lod != numLods - 1)
            (*innerLodsRegion) += BBox2(Point2(pos.x, pos.z), Point2(posRB.x, posRB.z));

          if (!cull_node(pos, posRB, ctx, waterLevel, viewPos))
            continue;

          if (BBox2 cellRoot(origin, origin + Point2(patchSize, patchSize));
              SUBPATCHES_DEPTH > 0 && hmap_tdata && hmap_tdata->testRegionTesselated(cellRoot))
          {
            struct SubpatchQuad
            {
              BBox2 cell;
              IPoint2 pos;
              int depth;
            };

            // tessCellSize is equal of bigger than LOD 0 gridSize (without subdivision).
            float tessCellSize = max(scaleX, hmap_tdata->getTessCellSize());
            Tab<SubpatchQuad> subpatchesStack(framemem_ptr());
            subpatchesStack.push_back({cellRoot, IPoint2(x, y) << (lod + lod0SubDiv), 1});
            do
            {
              SubpatchQuad subpatch = subpatchesStack.back();
              subpatchesStack.pop_back();

              int patchLOD = lod - subpatch.depth;
              int patchStep = 1 << (patchLOD + lod0SubDiv);
              IPoint2 patchPos = subpatch.pos;
              Point2 widthStep = subpatch.cell.width() * 0.5f;
              BBox2 newCell(subpatch.cell.lim[0], subpatch.cell.lim[0]);
              for (int j = 0; j < 2; ++j, newCell[0].y += widthStep.y, patchPos.y += patchStep)
              {
                patchPos.x = subpatch.pos.x;
                newCell[0].x = subpatch.cell.lim[0].x;
                for (int i = 0; i < 2; ++i, newCell[0].x += widthStep.x, patchPos.x += patchStep)
                {
                  newCell[1] = newCell[0] + widthStep;
                  float newGridSize = gridSize * widthStep.x / cellRoot.width().x;
                  if (subpatch.depth < SUBPATCHES_DEPTH && newGridSize > tessCellSize && hmap_tdata->testRegionTesselated(newCell))
                  {
                    subpatchesStack.push_back({newCell, patchPos, subpatch.depth + 1});
                  }
                  else
                  {
                    cull_data.patches.push_back(LodGridPatchParams(newGridSize,
                      bit_pack_ipoint2_to_uint(patchPos >> (patchLOD + lod0SubDiv)), newCell[0].x, newCell[0].y));
                    posLOD_map[patchPos] = patchLOD;
                  }
                }
              }
            } while (subpatchesStack.size());
          }
          else
          {
            int patchStep = 1 << (lod + lod0SubDiv);
            IPoint2 patchPos = IPoint2(x, y) << (lod + lod0SubDiv);
            cull_data.patches.push_back(LodGridPatchParams(gridSize, bit_pack_ipoint2_to_uint(IPoint2(x, y)), origin.x, origin.y));
            posLOD_map[patchPos] = lod;
          }
        }
      }
    }
    if (clip)
    {
      if (non_empty_boxes_inclusive(*clip, BBox2(Point2(cull_data.originPos.x - lodArea, cull_data.originPos.y - lodArea),
                                             Point2(cull_data.originPos.x + lodArea, cull_data.originPos.y + lodArea))))
        break;
    }
    area.nextLod();
  }

  // Get heightmap area (without edges stretch), `area_bbox`
  int lastLod = numLods - 1;
  int lastLodSubdiv = lastLod == 0 ? lod0SubDiv : 0;
  int lastLodRad = (lodGrid.lastLodRad << 1) << lastLodSubdiv;
  int lastPatchLod = lastLod - lastLodSubdiv;
  IBBox2 area_bbox(IPoint2(-lastLodRad, -lastLodRad) << (lastPatchLod + lod0SubDiv),
    (IPoint2(lastLodRad, lastLodRad) << (lastPatchLod + lod0SubDiv)) - IPoint2::ONE);

  // For each patch find edge tesselation transitioning.
  TIME_PROFILE(edge_tessellation_search)
  // For each patch we want to retrieve it's effective LOD, `patchLOD`. Where, `patchLOD = log2f(gridSize / scaleX)`.
  float subDivFactor = float(1 << lod0SubDiv); // - Multiply by 2^lod0SubDiv, to avoid negative log2 results (from LOD 0 subdivision).
  float scaleXFactor = subDivFactor / scaleX;  // - Turn float division into float multiplication.
  for (auto &patch : cull_data.patches)
  {
    float gridSize = patch.size;
    int patchLOD = get_log2i_of_pow2(round(gridSize * scaleXFactor)) - lod0SubDiv; // - Use `get_log2i_of_pow2` instead of `log2f`.
                                                                                   // Then, remove `lod0SubDiv` bias which was added
                                                                                   // above.
    int patchStep = 1 << (patchLOD + lod0SubDiv);
    IPoint2 patchPos = bit_unpack_uint_to_ipoint2(patch.tess) << (patchLOD + lod0SubDiv);

    auto getNeighbourLOD = [&](IPoint2 pos, int baseLOD, int maxLOD) -> int {
      for (int lod = baseLOD; lod <= maxLOD; ++lod)
      {
        // If we cannot find a subpatch, we can get ancestors position by masking least significant bits.
        uint32_t mask = mask_0_LSb(lod + lod0SubDiv);
        IPoint2 posRounded = IPoint2(pos.x & mask, pos.y & mask);
        if (auto search = posLOD_map.find(posRounded); search != posLOD_map.end())
          return search->second >= lod ? min(search->second, maxLOD) : -1;
      }
      return -1;
    };

    int edgeTessOut[4] = {0 /*left*/, 0 /*right*/, 0 /*top*/, 0 /*bottom*/};
    for (int i = 0; i != 4; ++i)
    {
      IPoint2 ofs = i / 2 == 0 ? IPoint2(patchStep, 0) : IPoint2(0, patchStep);
      ofs *= i % 2 == 0 ? -1 : 1;

      IPoint2 nextPatchPos = patchPos + ofs;
      if (area_bbox & nextPatchPos)
      {
        int neightbourLOD = getNeighbourLOD(nextPatchPos, patchLOD, patchLOD + 4);
        if (neightbourLOD != -1)
          edgeTessOut[i] = edge_tess_encode(neightbourLOD - patchLOD);
      }
      else
      {
        edgeTessOut[i] = edge_tess_encode(1 + lastLodSubdiv); // for last LOD edges stretch
      }
    }

    patch.params.y = bitwise_cast<float>(uint_pack_edges_tess(edgeTessOut));
  }
}
