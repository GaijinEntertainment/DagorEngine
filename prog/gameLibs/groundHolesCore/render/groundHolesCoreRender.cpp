// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/entityComponent.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/shaders.h>
#include <ecs/render/postfx_renderer.h>
#include <ecs/render/resPtr.h>
#include <groundHolesCore/render/ground_holes.hlsli>
#include <EASTL/unique_ptr.h>
#include <perfMon/dag_statDrv.h>
#include <3d/dag_resPtr.h>
#include <EASTL/vector.h>
#include <math/dag_Point4.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <heightmap/heightmapHandler.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/heightmap_holes_zones.hlsli>
#include <util/dag_convar.h>
#include <groundHolesCore/helpers.h>

#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <ecs/render/compute_shader.h>

#include <groundHolesCore/render/groundHolesCoreRender.h>

namespace ground_holes
{
#define VAR(a, opt) static ShaderVariableInfo a##VarId(#a, opt);

VAR(heightmap_holes_zones_num, true)
VAR(hmap_holes_scale_step_offset, false)
VAR(hmap_hole_ofs_size, false)
VAR(heightmap_holes_tmp_write_offset_size, false)

CONSOLE_BOOL_VAL("render", debug_ground_hole_use_cs_workaround, true); // TODO: PS4/PS5 needs it

CONSOLE_BOOL_VAL("render", debug_ground_holes_hide, false);

static void createTempResources(int tempTexSize, UniqueTexHolder &hmapHolesTmpTex, UniqueBufHolder &hmapHolesBuf)
{
  hmapHolesBuf = dag::buffers::create_persistent_cb(MAX_GROUND_HOLES * 4 * 2, "hmap_holes_matrices");
  hmapHolesTmpTex =
    dag::create_tex(NULL, tempTexSize, tempTexSize, TEXCF_RTARGET | TEXCF_UNORDERED | TEXFMT_R8, 1, "temp_heightmap_holes_tex");
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
  smpInfo.filter_mode = d3d::FilterMode::Point;
  ShaderGlobal::set_sampler(get_shader_variable_id("temp_heightmap_holes_tex_samplerstate"), d3d::request_sampler(smpInfo));
}

static void closeTempResources(UniqueTexHolder &hmapHolesTmpTex, UniqueBufHolder &hmapHolesBuf)
{
  hmapHolesBuf.close();
  hmapHolesTmpTex.close();
}

using HolesMtxTempVec = eastl::vector<Point4, framemem_allocator>;
HolesMtxTempVec getInverseMatrices(const ecs::Point4List &holes)
{
  G_ASSERT(holes.size() % 4 == 0);
  HolesMtxTempVec inverse_matrices;
  inverse_matrices.reserve(holes.size());

  for (int i = 0; i < holes.size() / 4; ++i)
  {
    TMatrix4 mat;
    mat.row[0] = holes[i * 4];
    mat.row[1] = holes[i * 4 + 1];
    mat.row[2] = holes[i * 4 + 2];
    mat.row[3] = holes[i * 4 + 3];
    mat.m[3][3] = 1;

    TMatrix4 inverse_mat = inverse43(mat);
    inverse_matrices.emplace_back(inverse_mat.row[0]);
    inverse_matrices.emplace_back(inverse_mat.row[1]);
    inverse_matrices.emplace_back(inverse_mat.row[2]);
    inverse_matrices.emplace_back(inverse_mat.row[3]);
  }
  return inverse_matrices;
}

void holes_initialize(int &hmap_holes_scale_step_offset_varId, int &hmap_holes_temp_ofs_size_varId, bool &should_render_ground_holes,
  ecs::Point4List &holes)
{
  hmap_holes_scale_step_offset_varId = hmap_holes_scale_step_offsetVarId.get_var_id();
  hmap_holes_temp_ofs_size_varId = hmap_hole_ofs_sizeVarId.get_var_id();
  holes.reserve(MAX_GROUND_HOLES);

  should_render_ground_holes = true;
}

void on_disappear(int hmap_holes_scale_step_offset_varId, UniqueTexHolder &hmapHolesTex)
{
  const Point4 forceOutOfRangeCoeffs = Point4(0, 0, 10.f, 10.f);
  ShaderGlobal::set_color4(hmap_holes_scale_step_offset_varId, forceOutOfRangeCoeffs);
  hmapHolesTex.close();
}

void render(UniqueTexHolder &hmapHolesTex, UniqueTexHolder &hmapHolesTmpTex, UniqueBufHolder &hmapHolesBuf,
  PostFxRenderer &hmapHolesProcessRenderer, PostFxRenderer &hmapHolesMipmapRenderer, ShadersECS &hmapHolesPrepareRenderer,
  bool &should_render_ground_holes, int hmap_holes_scale_step_offset_varId, int hmap_holes_temp_ofs_size_varId, ecs::Point4List &holes,
  ecs::Point3List &invalidate_bboxes, const ComputeShader &heightmap_holes_process_cs)
{
  SCOPE_RENDER_TARGET;

  int ground_holes_main_tex_size = 256;
  Point4 scaleStepOffset = getScaleOffset(ground_holes_main_tex_size);
  hmapHolesTex = dag::create_tex(NULL, ground_holes_main_tex_size, ground_holes_main_tex_size,
    TEXCF_RTARGET | TEXCF_UNORDERED | TEXFMT_L16, 0, "heightmap_holes_tex");
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
  smpInfo.filter_mode = d3d::FilterMode::Point;
  smpInfo.mip_map_mode = d3d::MipMapMode::Point;
  ShaderGlobal::set_sampler(get_shader_variable_id("heightmap_holes_tex_samplerstate"), d3d::request_sampler(smpInfo));

  int ground_holes_temp_tex_size = ground_holes_main_tex_size / 2;
  createTempResources(ground_holes_temp_tex_size, hmapHolesTmpTex, hmapHolesBuf);
  scaleStepOffset.y = getSamplingStepDist(1 / scaleStepOffset.x, ground_holes_main_tex_size);
  ShaderGlobal::set_color4(hmap_holes_scale_step_offset_varId, scaleStepOffset);
  d3d::set_depth(nullptr, DepthAccess::RW);
  int scale = (TEX_CELL_RES * ground_holes_main_tex_size) / ground_holes_temp_tex_size;
  if (!holes.empty())
  {
    HolesMtxTempVec inverseMats = getInverseMatrices(holes);

    // Updating using VBLOCK_DISCARD to avoid using MapNoOverwriteOnDynamicConstantBuffer feature on DX11
    G_ASSERT(holes.size() < MAX_GROUND_HOLES * 4);
    HolesMtxTempVec uploadBuf(MAX_GROUND_HOLES * 4 * 2, Point4(0, 0, 0, 0));
    eastl::copy(holes.begin(), holes.end(), uploadBuf.begin());
    eastl::copy(inverseMats.begin(), inverseMats.end(), uploadBuf.begin() + uploadBuf.size() / 2);
    hmapHolesBuf.getBuf()->updateDataWithLock(0, sizeof(Point4) * uploadBuf.size(), uploadBuf.data(), VBLOCK_DISCARD);
  }
  for (int y = 0; y < scale; y++)
    for (int x = 0; x < scale; x++)
    {
      d3d::set_render_target(hmapHolesTmpTex.getTex2D(), 0);
      d3d::setview(0, 0, ground_holes_temp_tex_size, ground_holes_temp_tex_size, 0, 1);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      ShaderGlobal::set_color4(hmap_holes_temp_ofs_size_varId, x, y, scale, ground_holes_temp_tex_size / TEX_CELL_RES);
      if (!holes.empty())
      {
        d3d::setvsrc(0, 0, 0);
        hmapHolesPrepareRenderer.shElem->setStates();
        d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, holes.size() / 4);
      }

      d3d::resource_barrier({hmapHolesTmpTex.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_PIXEL | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
      hmapHolesTmpTex.setVar();
      int sz = ground_holes_temp_tex_size / TEX_CELL_RES;
      if (debug_ground_hole_use_cs_workaround)
      {
        ShaderGlobal::set_color4(heightmap_holes_tmp_write_offset_sizeVarId, x * sz, y * sz, sz, sz);
        d3d::resource_barrier({hmapHolesTex.getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 1});
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_CS, 7, VALUE, 0, 0), hmapHolesTex.getTex2D());
        heightmap_holes_process_cs.dispatchThreads(sz, sz, 1);
      }
      else
      {
        d3d::resource_barrier({hmapHolesTex.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_PIXEL | RB_STAGE_PIXEL, 0, 1});
        d3d::set_render_target(hmapHolesTex.getTex2D(), 0);
        d3d::setview(x * sz, y * sz, sz, sz, 0, 1);
        hmapHolesProcessRenderer.render();
      }
    }
  should_render_ground_holes = false;
  closeTempResources(hmapHolesTmpTex, hmapHolesBuf);

  int mipLevels = hmapHolesTex.getTex2D()->level_count();
  for (int i = 1; i < mipLevels; ++i)
  {
    int sz = ground_holes_main_tex_size >> i;
    d3d::resource_barrier(
      {hmapHolesTex.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_SOURCE_STAGE_PIXEL | RB_STAGE_PIXEL, unsigned(i - 1), 1});
    hmapHolesTex.getTex2D()->texmiplevel(i - 1, i - 1);
    d3d::set_render_target(hmapHolesTex.getTex2D(), i);
    d3d::setview(0, 0, sz, sz, 0, 1);
    hmapHolesMipmapRenderer.render();
  }
  d3d::resource_barrier({hmapHolesTex.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_PIXEL | RB_STAGE_ALL_SHADERS, 0, 0});
  hmapHolesTex.getTex2D()->texmiplevel(0, mipLevels - 1);

  G_UNUSED(invalidate_bboxes);
}

void spawn_hole(ecs::Point4List &holes, const TMatrix &transform, bool roundShape, bool shapeIntersection)
{
  holes.push_back(Point4(transform.m[0][0], transform.m[0][1], transform.m[0][2], 0.0));
  holes.push_back(Point4(transform.m[1][0], transform.m[1][1], transform.m[1][2], 0.0));
  holes.push_back(Point4(transform.m[2][0], transform.m[2][1], transform.m[2][2], 0.0));
  holes.push_back(Point4(transform.m[3][0], transform.m[3][1], transform.m[3][2], 1.0));
  if (roundShape) // using element [3][3] for flags
    holes.back().w = -(1 + shapeIntersection);
  else
    holes.back().w = 1 + shapeIntersection;

  const LandMeshManager *lmeshMgr = getLmeshMgr();
  if (lmeshMgr)
  {
    HeightmapHandler *hmap = lmeshMgr->getHmapHandler();
    if (hmap)
      hmap->addTessRect(transform, shapeIntersection);
  }
}

void get_invalidation_bbox(ecs::Point3List &bboxes, const TMatrix &transform, bool shapeIntersection)
{
  Point3 center = transform.getcol(3);
  Point3 cornersOfs =
    Point3(abs(transform.getcol(0).x) + abs(transform.getcol(2).x) + (shapeIntersection ? abs(transform.getcol(1).x) : 0.f),
      0, // placeholder
      abs(transform.getcol(0).z) + abs(transform.getcol(2).z) + (shapeIntersection ? abs(transform.getcol(1).z) : 0.f));
  BBox3 invalidate_box(center - cornersOfs, center + cornersOfs);

  Point2 heightMinMax = getApproxHeightmapMinMax(invalidate_box);
  invalidate_box.lim[0].y = heightMinMax.x;
  invalidate_box.lim[1].y = heightMinMax.y;

  bboxes.emplace_back(invalidate_box.boxMin());
  bboxes.emplace_back(invalidate_box.boxMax());
}

void convar_helper(bool &should_render_ground_holes)
{
  if (debug_ground_holes_hide.pullValueChange() || debug_ground_hole_use_cs_workaround.pullValueChange())
  {
    should_render_ground_holes = true;
  }
}


void zones_before_render(UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones, Tab<Point3_vec4> &bboxes)
{
  if (!should_update_ground_holes_zones)
    return;
  if (!heightmap_holes_zones_numVarId)
    return;
  if (!hmapHolesZonesBuf.getBuf())
    hmapHolesZonesBuf = dag::buffers::create_persistent_cb(
      dag::buffers::cb_array_reg_count<Point3_vec4>(MAX_HEIGHTMAP_HOLES_ZONES * 2), "heightmap_holes_zones_cb");

  G_ASSERT(bboxes.size() % 2 == 0);
  G_ASSERTF_ONCE(bboxes.size() <= MAX_HEIGHTMAP_HOLES_ZONES * 2, "Ground holes zones above limit! Current: %d, Limit: %d",
    bboxes.size() / 2, MAX_HEIGHTMAP_HOLES_ZONES);

  ShaderGlobal::set_int(heightmap_holes_zones_numVarId, bboxes.size() / 2);
  if (!bboxes.empty())
    hmapHolesZonesBuf.getBuf()->updateDataWithLock(0, sizeof(Point3_vec4) * bboxes.size(), bboxes.data(), VBLOCK_DISCARD);

  should_update_ground_holes_zones = false;
}

void zones_after_device_reset(UniqueBufHolder &hmapHolesZonesBuf, bool &should_update_ground_holes_zones)
{
  should_update_ground_holes_zones = true;
  ShaderGlobal::set_int(heightmap_holes_zones_numVarId, 0);
  hmapHolesZonesBuf.close();
}

void zones_manager_on_disappear(UniqueBufHolder &hmapHolesZonesBuf)
{
  ShaderGlobal::set_int(heightmap_holes_zones_numVarId, 0);
  hmapHolesZonesBuf.close();
}

bool get_debug_hide() { return debug_ground_holes_hide; }
}; // namespace ground_holes