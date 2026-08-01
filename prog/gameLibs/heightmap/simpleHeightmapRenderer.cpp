// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/simpleHeightmapRenderer.h>
#include "lodGridVertexDataPool.h"
#include <3d/dag_resPtr.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <shaders/dag_shaders.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_atomic.h>
#include <util/dag_bitwise_cast.h>
#include <util/dag_finally.h>
#include <math/dag_adjpow2.h>
#include <math/dag_Point4.h>
#include <debug/dag_debug.h>

#define GLOBAL_VARS_LIST         \
  VAR(heightmap_parent_edges_at) \
  VAR(heightmap_has_morph)       \
  VAR(heightmap_morph)           \
  VAR(heightmap_edges)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR


static int heightmap_scale_offset_c = 0;
static UniqueBuf heightmap_edges, heightmap_morph;
static volatile int temp_buffers_ref_cnt = 0;

void SimpleHeightmapRenderer::close()
{
  if (!shmat)
    return;
  shElem = NULL; // Deleted in shmat
  del_it(shmat);
  if (interlocked_decrement(temp_buffers_ref_cnt) == 0)
  {
    heightmap_edges.close();
    heightmap_morph.close();
  }
  lod_grid_vdata[dimBits - VDATA_OFS].close();
}

bool SimpleHeightmapRenderer::init(const char *shader_name, bool do_fatal, int bits)
{
  heightmap_scale_offset_c = ShaderGlobal::get_int_fast(get_shader_variable_id("heightmap_scale_offset"));

  dimBits = clamp(bits - VDATA_OFS, 0, MAX_VDATA - 1) + VDATA_OFS;
  if (dimBits != bits)
  {
    if (do_fatal)
      DAG_FATAL("can't create hmap renderer for %d bits (should be %d..%d)", bits, VDATA_OFS, MAX_VDATA + VDATA_OFS - 1);
    return false;
  }
  shmat = new_shader_material_by_name(shader_name, shader_name);
  if (!shmat)
  {
    if (do_fatal)
      DAG_FATAL("can't create ShaderMaterial for '%s'", shader_name);
    return false;
  }
  shElem = shmat->make_elem();
  if (!shElem)
  {
    del_it(shmat);
    if (do_fatal)
      DAG_FATAL("can't create ShaderElement for ShaderMaterial '%s'", shader_name);
    return false;
  }
  // shared refs are acquired last: close() releases them only while shmat is alive
  interlocked_increment(temp_buffers_ref_cnt);
  if (!lod_grid_vdata[dimBits - VDATA_OFS].init(1 << dimBits))
  {
    close();
    if (do_fatal)
      DAG_FATAL("can not init heightmap buffers");
    return false;
  }
  return true;
}

static void render_patches_in_batches(dag::ConstSpan<LodGridPatchParams> patches, int buffer_size, int startInd, bool use_ib,
  int dim_bits)
{
  const int dim = 1 << dim_bits;
  float v[4] = {0, bitwise_cast<float>(dim_bits << 1), bitwise_cast<float>(dim - 1), 0};
  const int primitiveCount = 1 << (dim_bits + dim_bits + 1);
  for (int patch = 0, total_instances = patches.size(); patch < total_instances;)
  {
    const int current_batch_size = min(total_instances - patch, buffer_size);
    d3d::set_vs_const1(heightmap_scale_offset_c - 2, bitwise_cast<float>(patch), v[1], v[2], 0);
    d3d::set_vs_const(heightmap_scale_offset_c, &patches[patch].params.x, current_batch_size);
    if (use_ib)
      d3d::drawind_instanced(PRIM_TRILIST, startInd, primitiveCount, 0, current_batch_size);
    else
      d3d::draw_instanced(PRIM_TRILIST, 0, primitiveCount, current_batch_size);
    patch += current_batch_size;
  }
}

void SimpleHeightmapRenderer::render(const LodGridCullData &cull_data, const ShaderElement *shElem, int dim_bits)
{
  if (!shElem || !cull_data.hasPatches())
    return;
  const uint32_t morphCount = cull_data.patches.size() - cull_data.morph_at;
  const uint32_t morphNoEdgesCount = cull_data.patches.size() - cull_data.morph_no_edges_at;
  const uint32_t morphEdgesCount = morphCount - morphNoEdgesCount;

  if (!cull_data.edgesData.empty())
  {
    const uint32_t curSize = heightmap_edges ? heightmap_edges.getBuf()->getNumElements() : 0;
    if (cull_data.edgesData.size() > curSize * 4)
    {
      heightmap_edges.close();
      heightmap_edges = dag::create_sbuffer(sizeof(uint32_t), (cull_data.edgesData.size() + 3) >> 2,
        SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "heightmap_edges_",
        RESTAG_LAND);
    }
    const uint32_t maxSz = 2 << 20;
    if (cull_data.edgesData.size() > maxSz)
    {
      debug("edges sz is too big %d", cull_data.edgesData.size());
      debug_dump_stack();
    }
    heightmap_edges.getBuf()->updateData(0, min<uint32_t>(maxSz, cull_data.edgesData.size()), cull_data.edgesData.data(),
      VBLOCK_DISCARD | VBLOCK_WRITEONLY);
    ShaderGlobal::set_buffer(heightmap_edgesVarId, heightmap_edges.getBufId());
  }
  else
    ShaderGlobal::set_buffer(heightmap_edgesVarId, BAD_TEXTUREID);
  if (!cull_data.morphData.empty())
  {
    const uint32_t curMSize = heightmap_morph ? heightmap_morph.getBuf()->getNumElements() : 0;
    if (cull_data.morphData.size() > curMSize)
    {
      heightmap_morph.close();
      heightmap_morph = dag::create_sbuffer(sizeof(uint32_t), (cull_data.morphData.size() + 1023) & ~1023,
        SBCF_CPU_ACCESS_WRITE | SBCF_DYNAMIC | SBCF_FRAMEMEM | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "heightmap_morph_",
        RESTAG_LAND);
    }
    G_ASSERT(cull_data.morphData.size() == morphCount);
    heightmap_morph.getBuf()->updateData(0, cull_data.morphData.size() * 4, cull_data.morphData.data(),
      VBLOCK_DISCARD | VBLOCK_WRITEONLY);
    ShaderGlobal::set_buffer(heightmap_morphVarId, heightmap_morph.getBufId());
  }
  else
    ShaderGlobal::set_buffer(heightmap_morphVarId, BAD_TEXTUREID);

  const uint32_t dim = 1 << dim_bits;
  uint32_t bitEdgesOffset = dim;
  bitEdgesOffset = bitEdgesOffset * bitEdgesOffset;

  G_ASSERTF(cull_data.edgesData.empty() ||
              cull_data.edgesData.size() ==
                (bitEdgesOffset / 8 * (cull_data.morph_no_edges_at - cull_data.edges_at)) + morphEdgesCount * bitEdgesOffset / 32,
    "edgesData.size() = %d (%d + %d) edges %d morph_no_edges %d morph %d total %d", cull_data.edgesData.size(),
    (bitEdgesOffset / 8 * (cull_data.morph_no_edges_at - cull_data.edges_at)), morphEdgesCount * bitEdgesOffset / 32,
    cull_data.edges_at, cull_data.morph_no_edges_at, morphCount, cull_data.patches.size());

  ShaderGlobal::set_int4(heightmap_parent_edges_atVarId, (cull_data.morph_no_edges_at - cull_data.edges_at) * bitEdgesOffset,
    cull_data.morph_at, cull_data.morph_no_edges_at, cull_data.edges_at);
  ShaderGlobal::set_int(heightmap_has_morphVarId, cull_data.exact_edges ? 1 : 0);

  const bool use_ib = !cull_data.exact_edges;
  d3d::setvsrc_ex(0, NULL, 0, 0);
  TIME_D3D_PROFILE(heightmap);

  const int buffer_size =
    d3d::set_vs_constbuffer_register_count(MAX_HW_INSTANCING + heightmap_scale_offset_c) - heightmap_scale_offset_c;
  d3d::set_vs_const1(heightmap_scale_offset_c - 1, dim, bitwise_cast<float>(dim + 1), cull_data.scaleX, bitwise_cast<float>(dim_bits));

  if (!shElem->setStates(0, true))
    return;
  if (use_ib)
  {
    const int vDataIndex = dim_bits - VDATA_OFS;
    G_ASSERT(vDataIndex >= 0 && vDataIndex < MAX_VDATA && lod_grid_vdata[vDataIndex].ib);
    d3d::setind(lod_grid_vdata[vDataIndex].ib);
  }
  render_patches_in_batches(make_span_const(cull_data.patches), buffer_size, 0, use_ib, dim_bits);
  const int indicesCnt = 6 << (dim_bits + dim_bits);
  int startIndex = indicesCnt;
  for (int i = 0; i < cull_data.additionalTriPatches.size(); ++i)
  {
    render_patches_in_batches(make_span_const(cull_data.additionalTriPatches[i]), buffer_size, startIndex, use_ib, dim_bits);
    startIndex += indicesCnt;
  }

  d3d::set_vs_constbuffer_register_count(0);

  ShaderGlobal::set_int(heightmap_has_morphVarId, 0);

  d3d::setind(nullptr);
}

void SimpleHeightmapRenderer::renderOnePatch(const Point2 &left_top, float world_size) const
{
  int vDataIndex = dimBits - VDATA_OFS;
  if (!shElem)
    return;
  d3d::setvsrc_ex(0, NULL, 0, 0);
  d3d::setind(lod_grid_vdata[vDataIndex].ib);
  d3d::set_vs_constbuffer_register_count(522);
  FINALLY([]() { d3d::set_vs_constbuffer_register_count(0); });
  if (!shElem->setStates(0, true))
    return;
  TIME_D3D_PROFILE(heightmapOnePatch);
  Point4 oneConst = Point4(world_size, 0, left_top.x, left_top.y);
  d3d::set_vs_const1(heightmap_scale_offset_c - 2, 0, bitwise_cast<float>(2 * get_log2i(getDim())), bitwise_cast<float>(getDim() - 1),
    0);
  d3d::set_vs_const1(heightmap_scale_offset_c - 1, getDim(), bitwise_cast<float>(getDim() + 1), 0,
    bitwise_cast<float>(get_log2i(getDim())));
  d3d::set_vs_const(heightmap_scale_offset_c, &oneConst.x, 1);
  d3d::drawind(PRIM_TRILIST, 0, 2, 0);
}
