// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/landMask.h>
#include <math/dag_bounds2.h>
#include <ioSys/dag_dataBlock.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_overrideStates.h>
#include <render/toroidal_update_regions.h>

#define DEFAULT_LAND_TEX_SIZE   512
#define DEFAULT_MAX_LAND_HEIGHT 2500.f
#define DEFAULT_MIN_LAND_HEIGHT -100.f
#define DEFAULT_MAX_SLOPE       1.5f

enum
{
  ST_OK = 0,
  ST_DISCARDED = 1,
  ST_NOT_INITIALIZED = 2,
};

enum
{
  SHV_VIEWPORT,
  SHV_CELL_INV_SCALE,
  SHV_RANDOM_GRASS_HEIGHTMAP_PARAMS,
  SHV_HEIGHT_TO_WORLD,
  SHV_CELL_TO_HEIGHT, // same as SHV_HEIGHT_TO_WORLD, legacy, remove me after 21.03.2022

  SHV_COUNT
};

static int world_to_grassVarId = -1;
static const Color4 defaultGrassBiomes(0.1 / 255., 1.1 / 255., 2.1 / 255, 0); // by default red == 0, green = 1, blue == 2, and we
                                                                              // don't use direct biome indices when writing grass mask


LandMask::LandMask(const DataBlock &level_blk, int tex_align, bool needGrass) :
  texAlign(tex_align), worldSize(1), squareCenter(0, 0), status(ST_NOT_INITIALIZED), worldAlign(1), maxSlope(DEFAULT_MAX_SLOPE)
{
  filterRenderer = new PostFxRenderer();
  filterRenderer->init("random_grass_filter");

  shaders::OverrideState state;
  state.set(shaders::OverrideState::FLIP_CULL);

  state.colorWr = WRITEMASK_RED0 | WRITEMASK_GREEN0 | WRITEMASK_BLUE0;
  landColorOverride = shaders::overrides::create(state);

  state.colorWr = WRITEMASK_RED1 | WRITEMASK_ALPHA0;
  grassMaskAndTypeOverride = shaders::overrides::create(state);

  state.colorWr = WRITEMASK_ALPHA0;
  grassMaskOnlyOverride = shaders::overrides::create(state);

  state.set(shaders::OverrideState::FLIP_CULL | shaders::OverrideState::Z_FUNC);
  state.colorWr = 0;
  state.zFunc = CMPF_GREATEREQUAL;
  flipCullDepthOnlyOverride = shaders::overrides::create(state);

  // shader vars
  shaderVars.resize(SHV_COUNT);

  shaderVars[SHV_CELL_INV_SCALE] = ::get_shader_variable_id("grass_inv_scale");
  shaderVars[SHV_VIEWPORT] = ::get_shader_variable_id("random_grass_tex_viewport");
  shaderVars[SHV_RANDOM_GRASS_HEIGHTMAP_PARAMS] = ::get_shader_variable_id("random_grass_heightmap_params");
  shaderVars[SHV_HEIGHT_TO_WORLD] = ::get_shader_variable_id("land_mask_height_to_world");
  shaderVars[SHV_CELL_TO_HEIGHT] = ::get_shader_variable_id("cell_to_height", true);

  world_to_grassVarId = ::get_shader_variable_id("world_to_grass", true);

  // level params
  loadParams(level_blk);

  // textures

  unsigned flags = TEXCF_RTARGET;
  landHeightTex = UniqueTexHolder(dag::create_tex(NULL, landTexSize, landTexSize, TEXFMT_DEPTH16 | flags, 1, "landHeightTex"),
    "random_grass_heightmap");

  if (d3d::get_texformat_usage(TEXFMT_DEPTH16) & d3d::USAGE_FILTER)
    landHeightTex.getTex2D()->texfilter(TEXFILTER_LINEAR);

  if (needGrass)
  {
    // these textures are needed only when we have grass!
    grassTypeTex = dag::create_tex(NULL, landTexSize, landTexSize, flags | TEXFMT_L8, 1, "land_grass_type_tex");
    grassTypeTex.getTex2D()->texfilter(TEXFILTER_POINT);

    landColorTex = UniqueTexHolder(
      dag::create_tex(NULL, landTexSize, landTexSize, flags | TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, 1, "landColorTex"),
      "grass_land_color_mask");
  }

  torHelper.texSize = landTexSize;
  torHelper.curOrigin = IPoint2(-1000000, 100000);

  invalidRegions.clear();
  quadRegions.clear();

  // clear height
#if _TARGET_PC
  d3d::GpuAutoLock gpuLock;

  Driver3dRenderTarget prevRt;
  d3d::get_render_target(prevRt);

  d3d::set_render_target(nullptr, 0);
  d3d_err(d3d::set_depth(landHeightTex.getTex2D(), DepthAccess::RW));
  d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
  d3d::resource_barrier({landHeightTex.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  d3d::set_render_target(prevRt);
#endif
}

LandMask::~LandMask()
{
  delete filterRenderer;

  landHeightTex.close();
  landColorTex.close();
  grassTypeTex.close();
  ShaderGlobal::set_color4(get_shader_variable_id("grass_mask_biomes", true), defaultGrassBiomes);
}

void LandMask::loadParams(const DataBlock &level_blk)
{
  landTexSize = level_blk.getInt("texSize", DEFAULT_LAND_TEX_SIZE);
  landTexSize = ((landTexSize - 1) / texAlign + 1) * texAlign;

  minLandHeight = level_blk.getReal("minLandHeight", DEFAULT_MIN_LAND_HEIGHT);
  maxLandHeight = level_blk.getReal("maxLandHeight", DEFAULT_MAX_LAND_HEIGHT);

  // params for heightmap filtering
  maxSlope = level_blk.getReal("max_slope", DEFAULT_MAX_SLOPE);

  // grass mask
  Color4 grass_mask_biomes = defaultGrassBiomes;
  grass_mask_biomes.a = level_blk.getBool("useGrassMaskBiomes", false) ? 1 : 0;
  // +0.1 to avoid floor differences in hw
  grass_mask_biomes.r = (level_blk.getInt("grassMaskRedBiomeId", defaultGrassBiomes.r * 255) + 0.1) / 255.;
  grass_mask_biomes.g = (level_blk.getInt("grassMaskGreenBiomeId", defaultGrassBiomes.g * 255) + 0.1) / 255.;
  grass_mask_biomes.b = (level_blk.getInt("grassMaskBlueBiomeId", defaultGrassBiomes.b * 255) + 0.1) / 255.;
  ShaderGlobal::set_color4(get_shader_variable_id("grass_mask_biomes", true), grass_mask_biomes);
}

void LandMask::invalidate() // can be called from another thread
{
  invalidRegions.clear();
  quadRegions.clear();

  if (status != ST_NOT_INITIALIZED)
    status = ST_DISCARDED;
}

BBox2 LandMask::getRenderBbox() const
{
  if (status == ST_NOT_INITIALIZED)
    return BBox2();

  BBox2 render_bbox;
  Point2 center2((float)squareCenter.x * worldAlign, (float)squareCenter.y * worldAlign);
  float radius2 = (float)worldSize / 2.f;
  render_bbox[0] = center2 - Point2(radius2, radius2);
  render_bbox[1] = center2 + Point2(radius2, radius2);
  return render_bbox;
}

bool LandMask::isInitialized() const { return status != ST_NOT_INITIALIZED; }

void LandMask::addRegionToRender(const ToroidalQuadRegion &reg) { append_items(quadRegions, 1, &reg); }

void LandMask::renderRegion(const Point3 &center_pos, const ToroidalQuadRegion &reg, ILandMaskRenderHelper &render_helper,
  const TMatrix &view)
{
  float texelSize = worldSize / landTexSize;

  BBox2 box(point2(reg.texelsFrom) * texelSize, point2(reg.texelsFrom + reg.wd) * texelSize);

  BBox3 box3(Point3::xVy(box[0], minLandHeight), Point3::xVy(box[1], maxLandHeight));
  TMatrix4 proj = matrix_ortho_off_center_lh(box3[0].x, box3[1].x, box3[1].z, box3[0].z, box3[0].y, box3[1].y);

  TMatrix4 viewProj = TMatrix4(view) * proj;
  render_helper.beginRender(Point3::xVz(box3.center(), center_pos.y), box3, viewProj);

  d3d::settm(TM_PROJ, &proj);
  ShaderGlobal::set_color4(shaderVars[SHV_CELL_TO_HEIGHT], maxLandHeight - minLandHeight, 0, 0, 0);

  {
    // height only
    shaders::overrides::set(flipCullDepthOnlyOverride);
    d3d::set_render_target({landHeightTex.getTex2D(), 0}, DepthAccess::RW, {{NULL, 0}});
    d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
    d3d::clearview(CLEAR_ZBUFFER, 0x00000000, 0.f, 0);

    render_helper.renderHeight(minLandHeight, maxLandHeight);
    shaders::overrides::reset();
    d3d::resource_barrier({landHeightTex.getBaseTex(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  if (landColorTex.getTex2D())
  {
    // Render color, mask
    d3d::set_render_target({NULL, 0}, DepthAccess::RW, {{landColorTex.getTex2D(), 0}, {grassTypeTex.getTex2D(), 0}});
    d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0.f, 0);
    shaders::overrides::set(landColorOverride);

    // render combined color and mask
    render_helper.renderColor();

    // only grass mask part
    shaders::overrides::reset();
    shaders::overrides::set(grassMaskAndTypeOverride);

    render_helper.renderMask();
    shaders::overrides::reset();
    shaders::overrides::set(grassMaskOnlyOverride);

    render_helper.renderExplosions(); // just remove mask, do not change mask type
    shaders::overrides::reset();

    // filter based on slope
    ShaderGlobal::set_color4(shaderVars[SHV_VIEWPORT], reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y);

    // params for heightmap filtering
    ShaderGlobal::set_color4_fast(shaderVars[SHV_RANDOM_GRASS_HEIGHTMAP_PARAMS],
      Color4(1.f / landTexSize, (maxSlope * worldSize / landTexSize) / (maxLandHeight - minLandHeight), 0, 0));

    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    filterRenderer->render();

    d3d::resource_barrier({landColorTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({grassTypeTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }

  render_helper.endRender();
  // Restore.
  shaders::overrides::reset();
}

void LandMask::beforeRender(const Point3 &center_pos, ILandMaskRenderHelper &render_helper, bool force_update)
{
  TIME_D3D_PROFILE(LandMask_beforeRender);

  if (!render_helper.isValid())
    return;

  force_update |= status != ST_OK;
  float texelSize = worldSize / landTexSize;

  if (force_update)
    torHelper.curOrigin = IPoint2(-1000000, 100000);

  bool move = false;
  int newSquareCenterX = (int)floorf(center_pos.x / worldAlign + 0.5f);
  int newSquareCenterZ = (int)floorf(center_pos.z / worldAlign + 0.5f);
  move = force_update || newSquareCenterX != squareCenter.x || newSquareCenterZ != squareCenter.y;
  if (move)
  {
    if (!force_update)
    {
      if (abs(newSquareCenterX - squareCenter.x) > abs(newSquareCenterZ - squareCenter.y))
        newSquareCenterZ = squareCenter.y;
      else
        newSquareCenterX = squareCenter.x;
    }

    squareCenter.x = newSquareCenterX;
    squareCenter.y = newSquareCenterZ;

    status = ST_OK;

    regions.clear();
    IPoint2 newTexelOrigin = ipoint2(Point2::xy(squareCenter) * worldAlign / texelSize);

    ToroidalGatherCallback cb(regions);
    toroidal_update(newTexelOrigin, torHelper, 0.33f * landTexSize, cb);

    squareCenter.x = (int)roundf(torHelper.curOrigin.x * texelSize / worldAlign);
    squareCenter.y = (int)roundf(torHelper.curOrigin.y * texelSize / worldAlign);

    BBox2 bbox2 = getRenderBbox();
    Point2 width = bbox2.width();
    ShaderGlobal::set_color4(world_to_grassVarId,
      Color4(-1 / width.x, -1 / width.y, 1 + bbox2[0].x / width.x, 1 + bbox2[0].y / width.y));

    for (int i = 0; i < regions.size(); ++i)
    {
      const ToroidalQuadRegion &reg = regions[i];
      addRegionToRender(reg);
    }
  }

  // clip invalidated regions and choose one to render
  toroidal_clip_regions(torHelper, invalidRegions);

  int invalid_reg_id = get_closest_region_and_split(torHelper, invalidRegions);
  if (invalid_reg_id >= 0)
  {
    // update chosen invalid region.
    ToroidalQuadRegion quad(transform_point_to_viewport(invalidRegions[invalid_reg_id][0], torHelper),
      invalidRegions[invalid_reg_id].width() + IPoint2(1, 1), invalidRegions[invalid_reg_id][0]);

    addRegionToRender(quad);

    // and remove it from update list
    erase_items(invalidRegions, invalid_reg_id, 1);
  }

  // render
  {
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;

    TMatrix view;
    view.setcol(0, 1, 0, 0);
    view.setcol(1, 0, 0, 1);
    view.setcol(2, 0, 1, 0);
    view.setcol(3, 0, 0, 0);

    d3d::settm(TM_VIEW, view);

    d3d::set_render_target();

    // render regions and remove them from render list
    for (int i = 0; i < quadRegions.size(); ++i)
    {
      const ToroidalQuadRegion &reg = quadRegions[i];
      renderRegion(center_pos, reg, render_helper, view);
    }
    quadRegions.clear();
  }

  Point2 ofs = point2((torHelper.mainOrigin - torHelper.curOrigin) % torHelper.texSize) / torHelper.texSize;
  ShaderGlobal::set_color4_fast(shaderVars[SHV_CELL_INV_SCALE], -1.f / worldSize, ofs.x, ofs.y, landTexSize);

  ShaderGlobal::set_color4_fast(shaderVars[SHV_HEIGHT_TO_WORLD], maxLandHeight - minLandHeight, minLandHeight, 0.0f, 0.0f);
  // legacy, this variable doesn't exist, remove me after 21.03.2022
  ShaderGlobal::set_color4_fast(shaderVars[SHV_CELL_TO_HEIGHT], maxLandHeight - minLandHeight, minLandHeight, 0.0f, 0.0f);
}

void LandMask::invalidateBox(const BBox2 &box)
{
  float texelSize = worldSize / landTexSize;

  IBBox2 ibox(ipoint2(floor(box[0] / texelSize)), ipoint2(ceil(box[1] / texelSize)));

  toroidal_clip_region(torHelper, ibox);
  if (ibox.isEmpty())
    return;

  add_non_intersected_box(invalidRegions, ibox);
}
