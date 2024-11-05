// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/debugCollisionVisualization.h>
#include <rendInst/rendInstCollision.h>
#include <rendInst/rendInstAccess.h>

#include <util/dag_console.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderBlock.h>
#include <memory/dag_framemem.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_frustum.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <render/debug3dSolid.h>
#include <gui/dag_stdGuiRender.h>
#include <shaders/dag_postFxRenderer.h>

#include <riGen/riGenExtra.h>
#include <riGen/riGenData.h>
#include <riGen/riUtil.h>
#include <riGen/genObjUtil.h>

#include <debug/drawShadedCollisionsFlags.h>


namespace rendinst
{

static rendinst::DrawShadedCollisionsFlags globalShadedCollisionDrawFlags = {};


static void draw_collision_info(const rendinst::CollisionInfo &coll, const Point3 &view_pos, const Color4 &color, bool draw_ri,
  bool draw_canopy, bool drawPhysOnly, bool drawTraceOnly, const float max_dist_sq = 10000000, bool shaded = false)
{
  if (lengthSq(coll.tm.getcol(3) - view_pos) > max_dist_sq)
    return;

  static Color4 convexColor = Color4(0.5f, 1.0f, 1.0f, 1.0f);
  static Color4 boxColor = Color4(0.5f, 1.0f, 1.0f, 1.0f);
  static Color4 capsuleColor = Color4(1.0f, 1.0f, 0.0f, 1.0f);
  static Color4 sphereColor = Color4(1.0f, 0.5f, 0.5f, 1.0f);

  if (coll.collRes)
  {
    auto nodes = coll.collRes->getAllNodes();
    int nodeFilter = (drawPhysOnly ? CollisionNode::PHYS_COLLIDABLE : 0) | (drawTraceOnly ? CollisionNode::TRACEABLE : 0);
    for (int i = 0, n = draw_ri ? nodes.size() : 0; i < n; ++i)
    {
      const CollisionNode &meshNode = nodes[i]; //

      if (nodeFilter && !meshNode.checkBehaviorFlags(nodeFilter))
        continue;
      switch (meshNode.type)
      {
        case COLLISION_NODE_TYPE_BOX:
        {
          draw_debug_solid_cube(meshNode.modelBBox, coll.tm, color * boxColor, shaded);
          break;
        }

        case COLLISION_NODE_TYPE_MESH:
        {
          CollisionNode *collNode = coll.collRes->getNode(i);
          if (collNode != nullptr && collNode->type == COLLISION_NODE_TYPE_MESH)
          {
            if (!drawTraceOnly && meshNode.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
            {
              draw_debug_solid_mesh(collNode->indices.data(), collNode->indices.size() / 3, &collNode->vertices.data()->x,
                elem_size(collNode->vertices), collNode->vertices.size(), coll.tm * meshNode.tm,
                color * Color4(0.5f, 1.0f, 0.5f, 1.0f), shaded, DrawSolidMeshCull::FLIP);
            }
            if (!drawPhysOnly && meshNode.checkBehaviorFlags(CollisionNode::TRACEABLE))
            {
              draw_debug_solid_mesh(collNode->indices.data(), collNode->indices.size() / 3, &collNode->vertices.data()->x,
                elem_size(collNode->vertices), collNode->vertices.size(), coll.tm * meshNode.tm,
                color * Color4(1.0f, 0.5f, 0.5f, 1.0f), shaded, DrawSolidMeshCull::FLIP);
            }
          }
          break;
        }

        case COLLISION_NODE_TYPE_CONVEX:
        {
          CollisionNode *collNode = coll.collRes->getNode(i);
          if (collNode != nullptr && collNode->type == COLLISION_NODE_TYPE_CONVEX)
          {
            draw_debug_solid_mesh(collNode->indices.data(), collNode->indices.size() / 3, &collNode->vertices.data()->x,
              elem_size(collNode->vertices), collNode->vertices.size(), coll.tm * meshNode.tm, color * convexColor, shaded,
              DrawSolidMeshCull::FLIP);
          }
          break;
        }

        case COLLISION_NODE_TYPE_SPHERE:
        {
          draw_debug_solid_sphere(meshNode.boundingSphere.c, meshNode.boundingSphere.r, coll.tm, color * sphereColor, shaded);
          break;
        }

        case COLLISION_NODE_TYPE_CAPSULE:
        {
          draw_debug_solid_capsule(meshNode.capsule, coll.tm, color * capsuleColor, shaded);
          break;
        }
      }
    }
    RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(coll.desc);
    if (draw_canopy && rgl && coll.desc.cellIdx >= 0 && coll.desc.pool >= 0 && coll.desc.pool < rgl->rtData->riProperties.size())
    {
      const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[coll.desc.pool];
      if (riProp.canopyOpacity > 0.f && rgl->rtData->riPosInst[coll.desc.pool])
      {
        RendInstGenData::Cell &cell = rgl->cells[coll.desc.cellIdx];
        RendInstGenData::CellRtData &crt = *cell.rtData;
        float cellSz = rgl->grid2world * rgl->cellSz;
        vec3f v_cell_add = crt.cellOrigin;
        vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(cellSz, crt.cellHeight, cellSz, 0));
        bool posInst = crt.rtData->riPosInst[coll.desc.pool] ? 1 : 0;
        bool palette_rotation = crt.rtData->riPaletteRotation[coll.desc.pool];
        unsigned int stride = RIGEN_STRIDE_B(posInst, crt.rtData->riZeroInstSeeds[coll.desc.pool], rgl->perInstDataDwords);

        vec3f v_pos, v_scale;
        vec4i paletteId;
        rendinst::gen::unpack_tm_pos(v_pos, v_scale,
          (int16_t *)(crt.sysMemData.get() + crt.pools[coll.desc.pool].baseOfs + coll.desc.idx * stride), v_cell_add, v_cell_mul,
          palette_rotation, &paletteId);

        bbox3f &box = crt.rtData->riResBb[coll.desc.pool];
        bbox3f treeBBox;
        if (palette_rotation)
        {
          rendinst::gen::RotationPaletteManager::Palette palette =
            rendinst::gen::get_rotation_palette_manager()->getPalette({coll.desc.layer, coll.desc.pool});
          quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(paletteId));
          mat44f tm;
          v_mat44_compose(tm, v_pos, v_rot, v_scale);

          v_bbox3_init(treeBBox, tm, box);
        }
        else
        {
          treeBBox.bmin = v_add(v_pos, v_mul(v_scale, box.bmin));
          treeBBox.bmax = v_add(v_pos, v_mul(v_scale, box.bmax));
        }

        Point3_vec4 worldBoxMin;
        Point3_vec4 worldBoxMax;
        v_st(&worldBoxMin.x, treeBBox.bmin);
        v_st(&worldBoxMax.x, treeBBox.bmax);
        BBox3 worldBox(worldBoxMin, worldBoxMax);

        BBox3 canopyBox; // synthetical tree canopy box
        getRIGenCanopyBBox(riProp, worldBox, canopyBox);

        if (riProp.canopyTriangle)
        {
          float canopyWidth = canopyBox.width().x * 0.5f;
          Point3 bottomCenter = {canopyBox.center().x, canopyBox.boxMin().y, canopyBox.center().z};
          float canopyHeight = canopyBox.boxMax().y - canopyBox.boxMin().y;
          draw_debug_solid_cone(bottomCenter, Point3(0, 1, 0), canopyWidth, canopyHeight, 8, color * sphereColor);
        }
        else
        {
          draw_debug_solid_cube(canopyBox, TMatrix::IDENT, color * boxColor);
        }
      }
    }
  }
  else if (!coll.localBBox.float_is_empty())
  {
    draw_debug_solid_cube(coll.localBBox, coll.tm, color * boxColor, shaded);
  }
}

static void get_ri_collision(int layer, mat44f_cref globtm, const Point3 &view_pos, Tab<rendinst::CollisionInfo> &out_collisions,
  const float visibility_max_dist = 500.0f)
{
  TIME_D3D_PROFILE(get_ri_collision);

  rendinst::RendInstDesc desc;
  desc.layer = layer;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);

  if (!rgl)
    return;

  vec3f curViewPos = v_ldu(&view_pos.x);

  ScopedLockRead lock(rgl->rtData->riRwCs);

  for (int i = 0; i < rgl->cells.size(); ++i)
  {
    const RendInstGenData::Cell &cell = rgl->cells[i];
    const RendInstGenData::CellRtData *crt_ptr = cell.isReady();
    if (!crt_ptr)
      continue;
    const RendInstGenData::CellRtData &crt = *crt_ptr;
    for (int riIdx = 0; riIdx < crt.pools.size(); ++riIdx)
    {
      bool posInst = crt.rtData->riPosInst[riIdx] ? 1 : 0;
      unsigned int stride = RIGEN_STRIDE_B(posInst, crt.rtData->riZeroInstSeeds[riIdx], rgl->perInstDataDwords);

      const bbox3f &box = crt.rtData->riResBb[riIdx];
      vec4f lbbc2 = v_add(box.bmax, box.bmin);
      vec4f lbbe2 = v_sub(box.bmax, box.bmin);

      if (!crt.rtData->riCollRes[riIdx].collRes)
        continue;
      if (crt.pools[riIdx].total - crt.pools[riIdx].avail < 1)
        continue;

      for (int j = 0; j < crt.pools[riIdx].total; ++j)
      {
        rendinst::RendInstDesc riDesc(i, j, riIdx, 0, layer);

        mat44f tm;
        if (!riutil::get_rendinst_matrix(riDesc, rgl, (int16_t *)(crt.sysMemData.get() + crt.pools[riIdx].baseOfs + j * stride), &cell,
              tm))
          continue;

        vec4f distVec = v_sub(tm.col3, curViewPos);
        float dist = v_extract_x(v_length3_x(distVec));
        if (dist > visibility_max_dist)
          continue;

        mat44f clipTm;
        v_mat44_mul43(clipTm, globtm, tm);
        if (!v_is_visible_extent_fast(lbbc2, lbbe2, clipTm))
          continue;

        rendinst::CollisionInfo &info = out_collisions.emplace_back(riDesc);
        info.handle = crt.rtData->riCollRes[riIdx].handle;
        info.collRes = crt.rtData->riCollRes[riIdx].collRes;
        v_stu_bbox3(info.localBBox, box);
        v_mat_43cu_from_mat44(info.tm.array, tm);
      }
    }
  }
}

float rendinst_debug_min_size = 0.0f;

static void get_ri_extra_collision(Tab<CollisionInfo> &out_collisions, mat44f_cref globtm, const Point3 &view_pos,
  const float visibility_max_dist = 500.0f)
{
  TIME_D3D_PROFILE(get_ri_extra_collision);

  Frustum frustum;
  frustum.construct(globtm);
  Point3_vec4 vpos = view_pos;
  vec3f curViewPos = v_ldu(&vpos.x);
  shrink_frustum_zfar(frustum, curViewPos, v_splats(visibility_max_dist));
  vec3f vMinSize = v_splats(rendinst_debug_min_size);

  for (int i = 0; i < rendinst::riExtra.size(); ++i)
  {
    rendinst::RiExtraPool &riPool = rendinst::riExtra[i];
    if (riPool.collRes == nullptr)
      continue;

    int pool_vis = frustum.testBox(riPool.fullWabb.bmin, riPool.fullWabb.bmax);
    if (pool_vis == frustum.OUTSIDE)
      continue;

    for (int j = 0; j < riPool.riTm.size(); ++j)
    {
      vec3f radius = v_splat_w(v_abs(riPool.riXYZR[j]));
      if (v_test_vec_x_le(radius, vMinSize))
        continue;

      if (!frustum.testSphereB(riPool.riXYZR[j], radius))
        continue;
      if (!riPool.isValid(j))
        continue;

      if (pool_vis != frustum.INSIDE)
      {
        const float *src_mf = (const float *)&riPool.riTm[j];
        Point3_vec4 pos(src_mf[3], src_mf[7], src_mf[11]);
        unsigned dist2_i = bitwise_cast<unsigned, float>(lengthSq(pos - vpos) * rendinst::riExtraCullDistSqMul);
        if (dist2_i > riPool.distSqLOD_i[rendinst::RiExtraPool::MAX_LODS - 1])
          continue;
      }

      rendinst::RendInstDesc riDesc(-1, j, i, 0, -1);
      rendinst::CollisionInfo info(riDesc);
      info.handle = riPool.collHandle;
      info.collRes = riPool.collRes;
      v_stu_bbox3(info.localBBox, riPool.lbb);

      mat44f mat44;
      v_mat43_transpose_to_mat44(mat44, riPool.riTm[j]);
      v_mat_43cu_from_mat44(info.tm.array, mat44);

      out_collisions.push_back(info);
    }
  }
}

static void draw_collision_ui(const CollisionInfo &coll, mat44f_cref globtm, const Point3 &view_pos,
  const float ui_max_dist_sq = 20.0f * 20.0f)
{
  if (lengthSq(coll.tm.getcol(3) - view_pos) > ui_max_dist_sq)
    return;

  const char *riName = nullptr;
  uint16_t idx = -1;
  if (coll.desc.isRiExtra())
  {
    riName = rendinst::riExtraMap.getName(coll.desc.pool);
    idx = coll.desc.idx;
  }
  else
  {
    RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(coll.desc);
    riName = rgl->rtData->riResName[coll.desc.pool];
    idx = coll.desc.idx;
  }

  if (riName == nullptr)
    return;

  mat44f locTm;
  v_mat44_make_from_43cu_unsafe(locTm, coll.tm.array);
  mat44f gtm;
  v_mat44_mul43(gtm, globtm, locTm);

  Point3_vec4 bbMid = coll.localBBox.center();
  vec3f avg = v_ldu(&bbMid.x);
  avg = v_mat44_mul_vec3p(gtm, avg);
  avg = v_div(avg, v_splat_w(avg));
  avg = v_madd(avg, V_C_HALF, V_C_HALF);
  Point2 center = Point2(v_extract_x(avg) * StdGuiRender::screen_width(), (1 - v_extract_y(avg)) * StdGuiRender::screen_height());

  StdGuiRender::set_font(0, 0);
  StdGuiRender::goto_xy(center);
  String str(128, "%s: %d", riName, idx);
  StdGuiRender::set_color(E3DCOLOR(255, 255, 255, 255));
  StdGuiRender::draw_str(str.str());
}

static shaders::UniqueOverrideStateId debugOverride;
static shaders::UniqueOverrideStateId debugWireOverride;
static PostFxRenderer debugRiClearDepth;
static PostFxRenderer debugRiClearDepthColor;
static shaders::UniqueOverrideStateId debugRiClearDepthStateId;

void drawDebugCollisions(DrawCollisionsFlags flags, mat44f_cref globtm, const Point3 &view_pos, bool reverse_depth,
  float max_coll_dist_sq, float max_label_dist_sq)
{
  bool drawRendinstExtra = bool(flags & DrawCollisionsFlag::RendInstExtra);
  bool drawRendinst = bool(flags & DrawCollisionsFlag::RendInst);
  bool drawAnyRendinst = drawRendinstExtra || drawRendinst;
  bool drawRendinstCanopy = bool(flags & DrawCollisionsFlag::RendInstCanopy);


  bool drawPhysOnly = bool(flags & DrawCollisionsFlag::PhysOnly);
  bool drawTraceOnly = bool(flags & DrawCollisionsFlag::TraceOnly);

  bool drawShadedAlone = bool(globalShadedCollisionDrawFlags & DrawShadedCollisionsFlag::Alone);
  bool drawShadedWithVis = bool(globalShadedCollisionDrawFlags & DrawShadedCollisionsFlag::WithVis);
  bool drawShadedWireframe = bool(globalShadedCollisionDrawFlags & DrawShadedCollisionsFlag::Wireframe);
  bool drawShadedFacingHighlight = bool(globalShadedCollisionDrawFlags & DrawShadedCollisionsFlag::FaceOrientation);
  bool drawShaded = drawShadedAlone || drawShadedWithVis || drawShadedFacingHighlight;

  if (!drawAnyRendinst && !drawRendinstCanopy && !drawShaded)
    return;

  static int lastCollisionCount = 0;

  Tab<rendinst::CollisionInfo> collisions(framemem_ptr());
  collisions.reserve(lastCollisionCount);

  if (drawRendinstExtra || drawRendinstCanopy || drawShaded)
    get_ri_extra_collision(collisions, globtm, view_pos);

  if (drawRendinst || drawRendinstCanopy || drawShaded)
    FOR_EACH_RG_LAYER_DO (rgl)
      get_ri_collision(_layer, globtm, view_pos, collisions);

  lastCollisionCount = collisions.size();

  if (collisions.size() == 0)
    return;

  Color4 surfaceColor = Color4(1, 1, 1, 1);
  Color4 linesColor = Color4(1, 1, 1, 1);
  Point2 alpha = Point2(1.0f, 0.5f);

  if (flags & DrawCollisionsFlag::Opacity)
  {
    surfaceColor.a = alpha.y;
    linesColor.a = alpha.x;
  }
  else
  {
    surfaceColor *= alpha.y;
    surfaceColor.a = 1.0f;
    linesColor *= alpha.x;
    linesColor.a = 1.0f;
  }

  int lastBlockId = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);


  static shaders::OverrideStateId g_zfunc_less_state_id;
  static shaders::OverrideStateId g_zfunc_greater_state_id;

  if (!g_zfunc_less_state_id)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_FUNC);
    state.zFunc = CMPF_LESS;
    g_zfunc_less_state_id = shaders::overrides::create(state);
    state.zFunc = CMPF_GREATER;
    g_zfunc_greater_state_id = shaders::overrides::create(state);
  }

  if (flags & DrawCollisionsFlag::Invisible)
  {
    TIME_D3D_PROFILE(DRAW_COLLISIONS_INVISIBLE);

    shaders::overrides::set(reverse_depth ? g_zfunc_less_state_id : g_zfunc_greater_state_id);
    Color4 backSurfaceColor(surfaceColor.r, surfaceColor.g, surfaceColor.b, surfaceColor.a * 0.2);
    for (int i = 0; i < collisions.size(); ++i)
      draw_collision_info(collisions[i], view_pos, backSurfaceColor, drawAnyRendinst, drawRendinstCanopy, drawPhysOnly, drawTraceOnly,
        max_coll_dist_sq);
    shaders::overrides::reset();
  }

  d3d::settm(TM_WORLD, TMatrix::IDENT);
  if (!debugOverride)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_BIAS);
    state.zBias = 0.00001f;
    debugOverride.reset(shaders::overrides::create(state));
    state.zBias = 0.00005f;
    debugWireOverride.reset(shaders::overrides::create(state));
  }

  // Vulkan does not support clearview as a command, and inside of native renderpass, which is the main scene, it will split the
  // pass with an error.
  if (drawShaded && d3d::get_driver_code().is(d3d::vulkan) && (drawShadedAlone || drawShadedWithVis))
  {
    shaders::overrides::set(debugRiClearDepthStateId);
    if (drawShadedAlone)
      debugRiClearDepthColor.render();
    else if (drawShadedWithVis)
      debugRiClearDepth.render();
    shaders::overrides::reset();
  }

  // Using set_master_state to avoid "override already set" errors because draw_debug_solid_mesh also sets an override.
  shaders::overrides::set_master_state(shaders::overrides::get(debugOverride));

  if (drawShaded)
  {
    TIME_D3D_PROFILE(draw_collision_shaded)

    if (!d3d::get_driver_code().is(d3d::vulkan))
    {
      if (drawShadedAlone)
        d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0x00000000, 0, 0);
      else if (drawShadedWithVis)
        d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0x00000000, 0, 0);
    }

    static const int debug_ri_face_orientationVarId = get_shader_variable_id("debug_ri_face_orientation", false);
    ShaderGlobal::set_int(debug_ri_face_orientationVarId, drawShadedFacingHighlight ? 1 : 0);

    static int wireframeVarId = get_shader_glob_var_id("debug_ri_wireframe", false);
    ShaderGlobal::set_int(wireframeVarId, 0);
    for (auto &coll : collisions)
      draw_collision_info(coll, view_pos, surfaceColor, true, false, drawPhysOnly, drawTraceOnly, max_coll_dist_sq, true);

    shaders::overrides::reset_master_state();
    if (drawShadedWireframe)
    {
      shaders::overrides::set_master_state(shaders::overrides::get(debugWireOverride));
      d3d::setwire(true);
      max_coll_dist_sq = eastl::min(max_coll_dist_sq, 200.f * 200.f); // no need WIREFRAME on far distance
      ShaderGlobal::set_int(wireframeVarId, 1);
      for (auto &coll : collisions)
        draw_collision_info(coll, view_pos, surfaceColor, true, false, drawPhysOnly, drawTraceOnly, max_coll_dist_sq, true);
      d3d::setwire(false);
      shaders::overrides::reset_master_state();
    }

    {
      TIME_D3D_PROFILE(draw_collision_ui);

      StdGuiRender::ScopeStarter strt;
      for (auto &coll : collisions)
        draw_collision_ui(coll, globtm, view_pos, max_label_dist_sq);
    }

    ShaderGlobal::set_int(debug_ri_face_orientationVarId, 0);
    ShaderGlobal::setBlock(lastBlockId, ShaderGlobal::LAYER_FRAME);
    return;
  }

  {
    TIME_D3D_PROFILE(draw_collision_info);
    for (int i = 0; i < collisions.size(); ++i)
      draw_collision_info(collisions[i], view_pos, surfaceColor, drawAnyRendinst, drawRendinstCanopy, drawPhysOnly, drawTraceOnly,
        max_coll_dist_sq);
  }

  shaders::overrides::reset_master_state();
  if (flags & DrawCollisionsFlag::Wireframe)
  {
    TIME_D3D_PROFILE(DRAW_COLLISIONS_WIREFRAME);

    shaders::overrides::set_master_state(shaders::overrides::get(debugWireOverride));
    d3d::setwire(true);
    max_coll_dist_sq = eastl::min(max_coll_dist_sq, 20.f * 20.f); // no need WIREFRAME on far distance
    for (int i = 0; i < collisions.size(); ++i)
      draw_collision_info(collisions[i], view_pos, linesColor, drawAnyRendinst, drawRendinstCanopy, drawPhysOnly, drawTraceOnly,
        max_coll_dist_sq);
    d3d::setwire(false);
    shaders::overrides::reset_master_state();
  }

  {
    TIME_D3D_PROFILE(draw_collision_ui);

    StdGuiRender::ScopeStarter strt;
    for (auto &coll : collisions)
      draw_collision_ui(coll, globtm, view_pos, max_label_dist_sq);
  }

  ShaderGlobal::setBlock(lastBlockId, ShaderGlobal::LAYER_FRAME);
}

void close_debug_collision_visualization()
{
  shaders::overrides::destroy(debugWireOverride);
  shaders::overrides::destroy(debugOverride);
#if DAGOR_DBGLEVEL > 0
  close_debug_solid();
#endif
  debugRiClearDepth.clear();
  debugRiClearDepthColor.clear();
  shaders::overrides::destroy(debugRiClearDepthStateId);
}

void init_debug_collision_visualization()
{
#if DAGOR_DBGLEVEL > 0
  init_debug_solid();
#endif
  if (d3d::get_driver_code().is(d3d::vulkan))
  {
    debugRiClearDepth.init("debug_ri_clear_depth");
    debugRiClearDepthColor.init("debug_ri_clear_depth_color");

    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_FUNC);
    state.zFunc = CMPF_ALWAYS;
    debugRiClearDepthStateId = shaders::overrides::create(state);
  }
}


} // namespace rendinst

static bool console_debug_shaded_collision_console_handler(const char *argv[], int argc)
{
  int found = 0;
  static const auto debug_ri_diffVarId = ::get_shader_variable_id("debug_ri_diff");
  using Flags = rendinst::DrawShadedCollisionsFlag;
  CONSOLE_CHECK_NAME("shaded_collision", "none", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::WithVis;
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::Alone;
    ShaderGlobal::set_int(debug_ri_diffVarId, 0);
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "alone", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::WithVis;
    rendinst::globalShadedCollisionDrawFlags ^= Flags::Alone;
    ShaderGlobal::set_int(debug_ri_diffVarId, 0);
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "with_visual", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::Alone;
    rendinst::globalShadedCollisionDrawFlags ^= Flags::WithVis;
    ShaderGlobal::set_int(debug_ri_diffVarId, 0);
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "alone_diff", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::WithVis;
    rendinst::globalShadedCollisionDrawFlags |= Flags::Alone;
    ShaderGlobal::set_int(debug_ri_diffVarId, 1);
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "with_visual_diff", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags &= ~Flags::Alone;
    rendinst::globalShadedCollisionDrawFlags |= Flags::WithVis;
    ShaderGlobal::set_int(debug_ri_diffVarId, 1);
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "wireframe", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags ^= Flags::Wireframe;
    return true;
  }
  CONSOLE_CHECK_NAME("shaded_collision", "face_orientation", 1, 1)
  {
    rendinst::globalShadedCollisionDrawFlags ^= Flags::FaceOrientation;
    return true;
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(console_debug_shaded_collision_console_handler);
