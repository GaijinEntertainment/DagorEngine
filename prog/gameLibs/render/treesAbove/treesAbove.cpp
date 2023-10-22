#include <render/treesAbove.h>

#include <math/dag_Point2.h>
#include <math/dag_mathUtils.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds2.h>
#include <render/scopeRenderTarget.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/visibility.h>

#include <ioSys/dag_dataBlock.h>

#include <3d/dag_drv3d.h>
#include <util/dag_convar.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_renderScene.h>

#include <perfMon/dag_statDrv.h>

#include <render/toroidal_update.h>

#define MAX_INVALID_TREES_PER_FRAME 10U

TreesAbove::TreesAbove(const DataBlock &settings)
{
  world_to_trees_tex_ofsVarId = get_shader_glob_var_id("world_to_trees_tex_ofs");
  world_to_trees_tex_mulVarId = get_shader_glob_var_id("world_to_trees_tex_mul");
  gbuffer_for_treesaboveVarId = get_shader_variable_id("gbuffer_for_treesabove", true);

  initTrees2d(settings);
}

TreesAbove::~TreesAbove() { closeTrees2d(); }

void TreesAbove::invalidateTrees2d(bool force)
{
  G_UNUSED(force);
  // if (force)
  trees2dHelper.curOrigin = IPoint2(-1024 * 1024, 1024 * 1024);
}

void TreesAbove::invalidateTrees2d(const BBox3 &box, const TMatrix &tm)
{
  mat44f mat;
  v_mat44_make_from_43cu_unsafe(mat, tm.m[0]);

  bbox3f bbox;
  v_bbox3_init(bbox, mat, v_ldu_bbox3(box));
  invalidBoxes.push_back(bbox);
}


void TreesAbove::renderInvalidBboxes(const TMatrix &view_itm, float minZ, float maxZ)
{
  if (!trees2d.getTex2D())
    return;

  TIME_D3D_PROFILE(trees2d_bbox_invalidate);

  const float offsetZ = 32.f;
  maxZ += offsetZ;

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;

  d3d::set_render_target(trees2d.getTex2D(), 0);
  d3d::set_depth(trees2dDepth.getTex2D(), 0, DepthAccess::RW); // to be removed

  const float fullDistance = 2 * trees2dDist;
  const float texelSize = (fullDistance / trees2dHelper.texSize);
  const uint32_t updatesNum = min(invalidBoxes.size(), MAX_INVALID_TREES_PER_FRAME);
  for (int i = 0; i < updatesNum; ++i)
  {
    bbox3f bbox = invalidBoxes[i];
    Point4 bMin = as_point4(&bbox.bmax);
    Point4 bMax = as_point4(&bbox.bmin);
    const BBox2 box2d = {Point2(bMax.x, bMax.z), Point2(bMin.x, bMin.z)};
    dag::ConstSpan<BBox2> span = {&box2d, 1};

    ToroidalGatherCallback::RegionTab tab;
    ToroidalGatherCallback cb(tab);
    toroidal_invalidate_boxes(trees2dHelper, texelSize, span, cb);

    for (auto reg : tab)
      renderRegion(reg, texelSize, minZ, maxZ, view_itm);
  }

  d3d::resource_barrier({trees2d.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  d3d::resource_barrier({trees2dDepth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  invalidBoxes.erase(invalidBoxes.begin(), invalidBoxes.begin() + updatesNum);
  if (invalidBoxes.empty())
    invalidBoxes.shrink_to_fit();
}

void TreesAbove::renderRegion(ToroidalQuadRegion &reg, float texelSize, float minZ, float maxZ, const TMatrix &view_itm)
{
  d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
  BBox2 cBox(texelSize * point2(reg.texelsFrom), texelSize * point2(reg.texelsFrom + reg.wd));
  TMatrix4 proj = matrix_ortho_off_center_lh(cBox[0].x, cBox[1].x, cBox[1].y, cBox[0].y, maxZ, minZ);
  d3d::settm(TM_PROJ, &proj);

  TMatrix4 view;
  view.setcol(0, 1, 0, 0, 0);
  view.setcol(1, 0, 0, 1, 0);
  view.setcol(2, 0, 1, 0, 0);
  view.setcol(3, 0, 0, 0, 1);
  d3d::settm(TM_VIEW, &view);

  TMatrix4 globtm_ = view * proj;
  mat44f globtm;
  v_mat44_make_from_44cu(globtm, globtm_.m[0]);

  d3d::clearview(CLEAR_ZBUFFER | CLEAR_TARGET, 0, 0.0f, 0);
  rendinst::prepareRIGenVisibility(Frustum(globtm), Point3::xVy(cBox.center(), minZ), rendinst_trees_visibility, false, NULL);
  uint32_t immediateConsts[] = {0u, 0u, ((uint32_t)float_to_half(-1.f) << 16) | (uint32_t)float_to_half(0.f)};
  d3d::set_immediate_const(STAGE_PS, immediateConsts, countof(immediateConsts));
  rendinst::render::renderRIGen(rendinst::RenderPass::Normal, rendinst_trees_visibility, view_itm, rendinst::LayerFlag::NotExtra,
    rendinst::OptimizeDepthPass::No);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);
}

void TreesAbove::prepareTrees2d(const Point3 &origin, const TMatrix &view_itm, float minZ, float maxZ)
{
  if (!trees2d.getTex2D())
    return;
  const float offsetZ = 32.f;
  maxZ += offsetZ;
  Point2 alignedOrigin = Point2::xz(origin);
  const float fullDistance = 2 * trees2dDist;
  const float texelSize = (fullDistance / trees2dHelper.texSize);
  static constexpr int TEXEL_ALIGN = 4;
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);

  ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 1);
  static constexpr int THRESHOLD = TEXEL_ALIGN * 4;
  IPoint2 move = abs(trees2dHelper.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD)
  {
    TIME_D3D_PROFILE(trees2d);
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * trees2dHelper.texSize;
    if (max(move.x, move.y) < fullUpdateThresholdTexels) // if distance travelled is too big, there is no need to update movement in
                                                         // two steps
    {
      if (move.x < move.y)
        newTexelsOrigin.x = trees2dHelper.curOrigin.x;
      else
        newTexelsOrigin.y = trees2dHelper.curOrigin.y;
    }
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    ToroidalGatherCallback::RegionTab tab;
    ToroidalGatherCallback cb(tab);
    toroidal_update(newTexelsOrigin, trees2dHelper, fullUpdateThresholdTexels, cb);

    Point2 ofs = point2((trees2dHelper.mainOrigin - trees2dHelper.curOrigin) % trees2dHelper.texSize) / trees2dHelper.texSize;

    alignedOrigin = point2(trees2dHelper.curOrigin) * texelSize;
    d3d::set_render_target(trees2d.getTex2D(), 0);
    d3d::set_depth(trees2dDepth.getTex2D(), 0, DepthAccess::RW); // to be removed

    for (auto &reg : tab)
      renderRegion(reg, texelSize, minZ, maxZ, view_itm);

    d3d::resource_barrier({trees2d.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({trees2dDepth.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    ShaderGlobal::set_color4(world_to_trees_tex_ofsVarId, minZ - maxZ, maxZ, ofs.x, ofs.y);
    ShaderGlobal::set_color4(world_to_trees_tex_mulVarId, 1.0f / fullDistance, -alignedOrigin.x / fullDistance + 0.5,
      -alignedOrigin.y / fullDistance + 0.5, trees2dHelper.texSize);
  }
  ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 0);
}

void TreesAbove::renderAlbedo()
{
  if (voxelize_trees_albedo.getMat())
  {
    TIME_D3D_PROFILE(voxelize_trees);
    voxelize_trees_albedo.render();
  }
}

void TreesAbove::renderAlbedoLit()
{
  TIME_D3D_PROFILE(voxelize_trees);
  voxelize_trees_lit.render();
}

void TreesAbove::renderAlbedo(int w, int h, int maxRes)
{
  if (voxelize_trees_albedo.getMat())
  {
    d3d::setview(0, 0, min(w, maxRes * 2), min(h, maxRes * 2), 0, 1);
    renderAlbedo();
  }
}

void TreesAbove::renderAlbedoLit(int w, int h, const IPoint3 &res)
{
  if (voxelize_trees_lit.getMat() && min(res.z, res.x) >= 1)
  {
    d3d::setview(0, 0, min(res.x, w), min(res.z, h), 0, 1);
    renderAlbedoLit();
  }
}

void TreesAbove::initTrees2d(const DataBlock &settings)
{
  closeTrees2d();
  const int trees2dDRes = clamp(settings.getInt("trees2dRes", 320 * 2), 0, 2048) & (~1); // it has to be bigger than gi distance. I.e.
                                                                                         // 256m
  if (trees2dDRes > 0)
  {
    float treesTexelSize = settings.getReal("treesTexel", 1.); // todo: use gi 25d voxel size
    trees2dDist = ((float)trees2dDRes / 2.0f) * treesTexelSize;
    debug("init trees2d map rex %d^2, %f", trees2dDRes, treesTexelSize);
    // trees2d.set(d3d::create_tex(NULL, treesRes, treesRes, TEXCF_RTARGET|TEXFMT_L8, 1, "trees2d"), "trees2d");
    trees2d.set(d3d::create_tex(NULL, trees2dDRes, trees2dDRes, TEXCF_RTARGET | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, 1, "trees2d"),
      "trees2d");
    trees2dDepth.set(d3d::create_tex(NULL, trees2dDRes, trees2dDRes, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "trees2d_depth"),
      "trees2d_depth");    // to be removed, we should copy from back buffer
    trees2dDepth.setVar(); // to be removed, we should copy from back buffer

    trees2d.setVar();
    trees2d.getTex2D()->texaddr(TEXADDR_WRAP);
    trees2dDepth.getTex2D()->texaddr(TEXADDR_WRAP);
    trees2d.getTex2D()->texfilter(TEXFILTER_POINT);
    trees2dDepth.getTex2D()->texfilter(TEXFILTER_POINT);
    trees2dHelper.curOrigin = IPoint2(-1000000, 1000000);
    trees2dHelper.texSize = trees2dDRes;
    rendinst_trees_visibility = rendinst::createRIGenVisibility(midmem);
    rendinst::setRIGenVisibilityMinLod(rendinst_trees_visibility, 0, 4);
    voxelize_trees_albedo.init("voxelize_trees_albedo");
    voxelize_trees_lit.init("voxelize_trees_lit");
  }
  else
  {
    ShaderGlobal::set_color4(world_to_trees_tex_mulVarId, 0, 0, 0, 0);
    ShaderGlobal::set_color4(world_to_trees_tex_ofsVarId, 0, 0, 0, 0);
  }
}

void TreesAbove::closeTrees2d()
{
  rendinst::destroyRIGenVisibility(rendinst_trees_visibility);
  rendinst_trees_visibility = nullptr;
  trees2d.close();
  trees2dDepth.close();
  voxelize_trees_albedo.clear();
  voxelize_trees_lit.clear();
}
