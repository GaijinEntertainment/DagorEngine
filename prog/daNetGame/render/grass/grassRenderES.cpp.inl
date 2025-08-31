// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>

#include <ioSys/dag_dataBlock.h>
#include <util/dag_convar.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>

#include <3d/dag_quadIndexBuffer.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>

#include <shaders/dag_shaderBlock.h>

#include <perfMon/dag_statDrv.h>

#include <render/toroidal_update.h>
#include <render/renderSettings.h>

#include <math/dag_hlsl_floatx.h>
#include <render/grass_eraser_consts.hlsli>

#include <render/world/global_vars.h>
#include <render/world/wrDispatcher.h>
#include <render/renderEvent.h>
#include <render/renderer.h>
#include <daSDF/worldSDF.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <ecs/rendInst/riExtra.h>
#include <ecs/render/updateStageRender.h>
#include <render/world/defaultVrsSettings.h>
#include <render/world/deformHeightmap.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

#include "grassRenderer.h"


CONSOLE_BOOL_VAL("grass", grassRender, true);
CONSOLE_BOOL_VAL("grass", grassPrepass, true);

CONSOLE_BOOL_VAL("grass", grassColorIgnoreTerrainDetail, false);

CONSOLE_BOOL_VAL("grass", grassUseSdfEraser, false);
CONSOLE_FLOAT_VAL_MINMAX("grass", grassSdfEraserHeight, 1, 0.001f, 100);
CONSOLE_BOOL_VAL("grass", grassLogSdfChanges, false);

namespace var
{
static ShaderVariableInfo grass_eraser_sdf_height("grass_eraser_sdf_height");
} // namespace var

ConVarB fast_grass_render("fast_grass.render", true, nullptr);

class RandomGrassRenderHelper final : public IRandomGrassRenderHelper
{
public:
  LandMeshRenderer &renderer;
  LandMeshManager &provider;
  ViewProjMatrixContainer oviewproj;
  TMatrix4_vec4 globTm;
  TMatrix4_vec4 projTm;
  Frustum frustum;
  LMeshRenderingMode omode;
  int oblock;

  RandomGrassRenderHelper(LandMeshRenderer &r,
    LandMeshManager &p,
    float rendInstHeightmapMaxHt,
    uint32_t eraser_count,
    Sbuffer *eraser_instance_buffer,
    ShaderElement *grass_eraser,
    PostFxRenderer &sdf_eraser) :
    renderer(r),
    provider(p),
    oviewproj(),
    omode(LMeshRenderingMode::RENDERING_LANDMESH),
    oblock(),
    rendInstHeightmapMaxHt(rendInstHeightmapMaxHt),
    eraserCount(eraser_count),
    eraserInstanceBuffer(eraser_instance_buffer),
    grassEraser(grass_eraser),
    sdfEraser(sdf_eraser)
  {}
  ~RandomGrassRenderHelper() {}
  bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &glob_tm, const TMatrix4 &proj_tm) override;
  void endRender() override;
  // void renderHeight( float min_height, float max_height );
  void renderColor() override;
  void renderMask() override;
  void renderExplosions();
  bool getMinMaxHt(const BBox2 &box, float &min_ht, float &max_ht) override;
  // bool getHeightmapAtPoint( float x, float y, float &out );
private:
  float rendInstHeightmapMaxHt;
  uint32_t eraserCount;
  Sbuffer *eraserInstanceBuffer;
  ShaderElement *grassEraser;
  PostFxRenderer &sdfEraser;
};


bool RandomGrassRenderHelper::getMinMaxHt(const BBox2 &box, float &min_ht, float &max_ht)
{
  if (provider.getHmapHandler() && provider.getHmapHandler()->heightmapHeightCulling)
  {
    const float patchSize = max(box.width().x, box.width().y);
    const float lod = provider.getHmapHandler()->heightmapHeightCulling->getLod(patchSize);
    float minHt = min_ht, maxHt = max_ht;
    provider.getHmapHandler()->heightmapHeightCulling->getMinMax(floorf(lod), box[0], patchSize, minHt, maxHt);
    min_ht = max(min_ht, minHt + ShaderGlobal::get_real(hmap_displacement_downVarId));
    max_ht = min(max_ht, maxHt + ShaderGlobal::get_real(hmap_displacement_upVarId));
    max_ht = max(max_ht, rendInstHeightmapMaxHt);
    return true;
  }
  min_ht = max(min_ht, provider.getBBox()[0].y);
  max_ht = min(max_ht, provider.getBBox()[1].y);
  max_ht = max(max_ht, rendInstHeightmapMaxHt);
  return true;
}

bool RandomGrassRenderHelper::beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &glob_tm, const TMatrix4 &proj)
{
  d3d_get_view_proj(oviewproj);
  omode = renderer.getLMeshRenderingMode();

  globTm = glob_tm;
  projTm = proj;
  frustum = Frustum{glob_tm};
  renderer.prepare(provider, center_pos, 2.f);
  renderer.setRenderInBBox(box);
  oblock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  return true;
}

void RandomGrassRenderHelper::endRender()
{
  d3d_set_view_proj(oviewproj);

  renderer.setLMeshRenderingMode(omode);
  renderer.setRenderInBBox(BBox3());
  ShaderGlobal::setBlock(oblock, ShaderGlobal::LAYER_FRAME);
}

void RandomGrassRenderHelper::renderMask()
{
  renderer.setLMeshRenderingMode(LMeshRenderingMode::GRASS_MASK);
  renderer.render(reinterpret_cast<mat44f_cref>(globTm), projTm, frustum, provider, LandMeshRenderer::RENDER_GRASS_MASK,
    ::grs_cur_view.pos, LandMeshRenderer::RENDER_FOR_GRASS);
  if (eraserCount > 0 && eraserInstanceBuffer && grassEraser)
  {
    TIME_D3D_PROFILE(grass_erasers);
    index_buffer::use_quads_16bit();
    grassEraser->setStates(0, true);
    d3d::setvsrc(0, 0, 0);

    d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, eraserCount);
  }

  if (grassUseSdfEraser)
    sdfEraser.render();
}

void RandomGrassRenderHelper::renderColor()
{
  renderer.setLMeshRenderingMode(
    grassColorIgnoreTerrainDetail.get() ? LMeshRenderingMode::GRASS_COLOR : LMeshRenderingMode::RENDERING_CLIPMAP);
  renderer.render(reinterpret_cast<mat44f_cref>(globTm), projTm, frustum, provider, LandMeshRenderer::RENDER_CLIPMAP,
    ::grs_cur_view.pos, LandMeshRenderer::RENDER_FOR_GRASS);
}
ECS_REGISTER_RELOCATABLE_TYPE(GrassRenderer, nullptr);

void GrassRenderer::renderGrassPrepassInternal(const GrassView view)
{
  TIME_D3D_PROFILE(grass_prepass);
  grass.render(view, grass.GRASS_DEPTH_PREPASS);
}


void GrassRenderer::renderGrassPrepass(const GrassView view)
{
  if (!grassRender.get())
    return;

  if (grassPrepass.get())
    renderGrassPrepassInternal(view);
}

void GrassRenderer::renderGrassVisibilityPass(const GrassView view)
{
  if (!grassRender.get() || !grassPrepass.get())
    return;

  grass.render(view, grass.GRASS_VISIBILITY_PASS);
}

void GrassRenderer::resolveGrassVisibility(const GrassView view)
{
  if (!grassRender.get() || !grassPrepass.get())
    return;

  grass.resolveVisibility(view);
}

void GrassRenderer::renderGrass(const GrassView view)
{
  if (!grassRender.get())
    return;

  if (grassPrepass.get())
    grass.render(view, grass.GRASS_AFTER_PREPASS);
  else
    grass.render(view, grass.GRASS_NO_PREPASS);
}

void GrassRenderer::renderFastGrass(const TMatrix4 &globtm, const Point3 &view_pos)
{
  fast_grass_baker::fast_grass_baker_on_render();
  if (!fast_grass_render.get())
    return;
  if (auto *lmeshManager = WRDispatcher::getLandMeshManager())
  {
    float minHt = lmeshManager->getBBox()[0].y;
    float maxHt = lmeshManager->getBBox()[1].y;
    if (rendInstHeightmap)
      maxHt = max(maxHt, rendInstHeightmap->getMaxHt());
    float waterLevel = HeightmapHeightCulling::NO_WATER_ON_LEVEL;
    if (auto hmapHandler = lmeshManager->getHmapHandler())
      waterLevel = hmapHandler->getPreparedWaterLevel();
    BBox2 clipBox = grass.getGrassWorldBox();
    fastGrass.render(globtm, view_pos, minHt, maxHt, waterLevel, &clipBox);
  }
}

void GrassRenderer::updateSdfEraser()
{
  if (!grassUseSdfEraser)
    return;

  if (!sdfBoxesToUpdate.empty())
  {
    BBox2 box = sdfBoxesToUpdate.back();
    sdfBoxesToUpdate.pop_back();
    if (grassLogSdfChanges)
      debug("invalidating grass due to SDF changes: (%f %f) .. (%f %f) area %f", B2D(box), box.width().x * box.width().y);
    grass.invalidateBoxes(make_span(&box, 1));
  }

  if (!sdfBoxesToUpdate.empty())
    return;

  auto *worldRenderer = get_world_renderer();
  if (!worldRenderer)
    return;

  auto *worldSdf = worldRenderer->getWorldSDF();
  if (!worldSdf)
    return;

  int numClips = worldSdf->getClipsCount();
  if (numClips != lastSdfBoxes.size())
  {
    lastSdfBoxes.clear();
    lastSdfBoxes.resize(numClips);
  }

  dag::Vector<BBox2, framemem_allocator> boxesToUpdate;

  for (int i = 0; i < lastSdfBoxes.size(); i++)
  {
    BBox3 box = worldSdf->getClipBox(i);
    auto &lastBox = lastSdfBoxes[i];
    if (box == lastBox)
      continue;

    BBox2 oldBox = BBox2(Point2::xz(lastBox.lim[0]), Point2::xz(lastBox.lim[1]));
    lastBox = box;
    BBox2 newBox = BBox2(Point2::xz(box.lim[0]), Point2::xz(box.lim[1]));
    if (newBox.isempty())
      continue;

    if (oldBox & newBox)
    {
      // subtract oldBox from newBox
      if (newBox[0].y < oldBox[0].y)
        boxesToUpdate.push_back(BBox2(newBox[0].x, newBox[0].y, newBox[1].x, oldBox[0].y));
      if (newBox[0].x < oldBox[0].x)
        boxesToUpdate.push_back(BBox2(newBox[0].x, max(oldBox[0].y, newBox[0].y), oldBox[0].x, min(oldBox[1].y, newBox[1].y)));
      if (newBox[1].x > oldBox[1].x)
        boxesToUpdate.push_back(BBox2(oldBox[1].x, max(oldBox[0].y, newBox[0].y), newBox[1].x, min(oldBox[1].y, newBox[1].y)));
      if (newBox[1].y > oldBox[1].y)
        boxesToUpdate.push_back(BBox2(newBox[0].x, oldBox[1].y, newBox[1].x, newBox[1].y));
    }
    else
      boxesToUpdate.push_back(newBox);
  }

  if (boxesToUpdate.empty())
    return;

  if (grassLogSdfChanges)
  {
    debug("invalidating grass due to SDF changes (%d boxes):", boxesToUpdate.size());
    for (const auto &box : boxesToUpdate)
      debug("  (%f %f) .. (%f %f) area %f", B2D(box), box.width().x * box.width().y);
  }

  BBox2 totalBox;
  for (const auto &box : boxesToUpdate)
    totalBox += box;

  for (int y = 0; y < sdfUpdatePeriod; y++)
    for (int x = 0; x < sdfUpdatePeriod; x++)
    {
      BBox2 subbox(mul(Point2(x / float(sdfUpdatePeriod), y / (float(sdfUpdatePeriod))), totalBox.width()) + totalBox[0],
        mul(Point2((x + 1) / float(sdfUpdatePeriod), (y + 1) / (float(sdfUpdatePeriod))), totalBox.width()) + totalBox[0]);

      BBox2 unionBox;
      for (const auto &box : boxesToUpdate)
        unionBox += BBox2(max(box[0], subbox[0]), min(box[1], subbox[1]));

      if (!unionBox.isempty())
        sdfBoxesToUpdate.push_back(unionBox);
    }
}

static inline float get_rendinst_heightmap_max_ht(const RendInstHeightmap *hmap) { return hmap ? hmap->getMaxHt() : -10000.0f; }

void GrassRenderer::generateGrassPerCamera(const TMatrix &itm)
{
  auto *lmeshManager = WRDispatcher::getLandMeshManager();
  if ((!grassRender.get() && !fast_grass_render.get()) || !lmeshManager)
    return;
  if (rendInstHeightmap)
  {
    rendInstHeightmap->updateToroidalTextureRegions(globalFrameBlockId);

    // This call can initiate RI visibility job (culling) which will be executed in other thread. We can do it before
    // `updateToroidalTextureRegions` but then we need to do it as early as possible to not waste time on wait.
    // I decided to do it here because most likely texture update will be delayed anyway until required lods
    // for RI are loaded.
    rendInstHeightmap->updatePos(itm.getcol(3));
  }

  if (grassEraserBuffer.getBuf() && grassErasersModified && grassErasersActualSize > 0)
  {
    grassEraserBuffer.getBuf()->updateDataWithLock(0, sizeof(grassErasers[0]) * grassErasersActualSize, grassErasers.begin(),
      VBLOCK_DISCARD);
    grassErasersModified = false;
    d3d::resource_barrier({grassEraserBuffer.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
  }

  bool needInvalidate = grassColorIgnoreTerrainDetail.pullValueChange();
  if (grassUseSdfEraser.pullValueChange() || (grassUseSdfEraser && grassSdfEraserHeight.pullValueChange()))
  {
    needInvalidate = true;
    ShaderGlobal::set_real(var::grass_eraser_sdf_height, grassSdfEraserHeight);
  }
  if (needInvalidate)
  {
    grass.invalidate();
    sdfBoxesToUpdate.clear();
  }

  updateSdfEraser();

  auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  if (lmeshRenderer)
  {
    const float rendInstHeightmapMaxHt = get_rendinst_heightmap_max_ht(rendInstHeightmap.get());

    RandomGrassRenderHelper rhlp(*lmeshRenderer, *lmeshManager, rendInstHeightmapMaxHt, min(grassErasersActualSize, MAX_GRASS_ERASERS),
      grassEraserBuffer.getBuf(), grassEraserShader.shader, grassSdfEraser);

    grass.generatePerCamera(itm.getcol(3), rhlp);
  }
}

void GrassRenderer::generateGrassPerView(
  const GrassView view, const Frustum &frustum, const TMatrix &itm, const Driver3dPerspective &perspective)
{
  auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  auto *lmeshManager = WRDispatcher::getLandMeshManager();
  if ((!grassRender.get() && !fast_grass_render.get()) || !lmeshManager)
    return;

  if (::grs_draw_wire)
    d3d::setwire(0);

  const float rendInstHeightmapMaxHt = get_rendinst_heightmap_max_ht(rendInstHeightmap.get());

  RandomGrassRenderHelper rhlp(*lmeshRenderer, *lmeshManager, rendInstHeightmapMaxHt, min(grassErasersActualSize, MAX_GRASS_ERASERS),
    grassEraserBuffer.getBuf(), grassEraserShader.shader, grassSdfEraser);

  grass.generatePerView(view, frustum, itm.getcol(3), itm.getcol(2), rhlp);

  if (grassify)
  {
    grassify->generate(view, itm.getcol(3), itm, perspective, grass.getMaskTex(), rhlp, grass.base);
  }

  if (::grs_draw_wire)
    d3d::setwire(1);
}

template <typename Callable>
static void get_grass_render_ecs_query(Callable c);
template <typename Callable>
static void init_grass_render_ecs_query(Callable c);
template <typename Callable>
static void grass_erasers_ecs_query(Callable c);
template <typename Callable>
static void fast_grass_types_ecs_query(Callable c);

ECS_TAG(render)
static void grass_render_update_es(const UpdateStageInfoBeforeRender &, GrassRenderer &grass_render)
{
  if (grass_render.fastGrassChanged)
    grass_render.initOrUpdateFastGrass();
}

ECS_TRACK(fast_grass__slice_step,
  fast_grass__fade_start,
  fast_grass__fade_end,
  fast_grass__step_scale,
  fast_grass__height_variance_scale,
  fast_grass__smoothness_fade_start,
  fast_grass__smoothness_fade_end,
  fast_grass__normal_fade_start,
  fast_grass__normal_fade_end,
  fast_grass__placed_fade_start,
  fast_grass__placed_fade_end,
  fast_grass__ao_max_strength,
  fast_grass__ao_curve,
  fast_grass__num_samples,
  fast_grass__max_samples)
ECS_REQUIRE(float fast_grass__slice_step,
  float fast_grass__fade_start,
  float fast_grass__fade_end,
  float fast_grass__step_scale,
  float fast_grass__height_variance_scale,
  float fast_grass__smoothness_fade_start,
  float fast_grass__smoothness_fade_end,
  float fast_grass__normal_fade_start,
  float fast_grass__normal_fade_end,
  float fast_grass__placed_fade_start,
  float fast_grass__placed_fade_end,
  float fast_grass__ao_max_strength,
  float fast_grass__ao_curve,
  int fast_grass__num_samples,
  int fast_grass__max_samples)
static void track_fast_grass_settings_es(const ecs::Event &)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.fastGrassChanged = true; });
}

// split because of ECS limit of 15 tracked components
ECS_TRACK(fast_grass__hmap_range, fast_grass__hmap_cell_size, fast_grass__clipmap_resolution, fast_grass__clipmap_cascades)
ECS_REQUIRE(
  float fast_grass__hmap_range, float fast_grass__hmap_cell_size, int fast_grass__clipmap_resolution, int fast_grass__clipmap_cascades)
static void track_fast_grass_settings2_es(const ecs::Event &)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.fastGrassChanged = true; });
}

ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(fast_grass__impostor,
  dagdp__biomes,
  fast_grass__height,
  fast_grass__weight_to_height_mul,
  fast_grass__height_variance,
  fast_grass__stiffness,
  fast_grass__color_mask_r_from,
  fast_grass__color_mask_r_to,
  fast_grass__color_mask_g_from,
  fast_grass__color_mask_g_to,
  fast_grass__color_mask_b_from,
  fast_grass__color_mask_b_to)
ECS_REQUIRE(ecs::Tag fast_grass_type,
  const ecs::string &fast_grass__impostor,
  const ecs::List<int> &dagdp__biomes,
  float fast_grass__height,
  float fast_grass__weight_to_height_mul,
  float fast_grass__height_variance,
  float fast_grass__stiffness,
  E3DCOLOR fast_grass__color_mask_r_from,
  E3DCOLOR fast_grass__color_mask_r_to,
  E3DCOLOR fast_grass__color_mask_g_from,
  E3DCOLOR fast_grass__color_mask_g_to,
  E3DCOLOR fast_grass__color_mask_b_from,
  E3DCOLOR fast_grass__color_mask_b_to)
static void track_fast_grass_types_es(const ecs::Event &)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.fastGrassChanged = true; });
}

void GrassRenderer::initOrUpdateFastGrass()
{
  dag::Vector<FastGrassRenderer::GrassTypeDesc, framemem_allocator> grassTypes;

  if (auto eid = g_entity_mgr->getSingletonEntity(ECS_HASH("fast_grass_settings")); eid != ecs::INVALID_ENTITY_ID)
  {
#define GET_PARAM(N, C) fastGrass.N = g_entity_mgr->getOr(eid, ECS_HASH("fast_grass__" #C), fastGrass.N)
    GET_PARAM(sliceStep, slice_step);
    GET_PARAM(fadeStart, fade_start);
    GET_PARAM(fadeEnd, fade_end);
    GET_PARAM(stepScale, step_scale);
    GET_PARAM(heightVarianceScale, height_variance_scale);
    GET_PARAM(smoothnessFadeStart, smoothness_fade_start);
    GET_PARAM(smoothnessFadeEnd, smoothness_fade_end);
    GET_PARAM(normalFadeStart, normal_fade_start);
    GET_PARAM(normalFadeEnd, normal_fade_end);
    GET_PARAM(placedFadeStart, placed_fade_start);
    GET_PARAM(placedFadeEnd, placed_fade_end);
    GET_PARAM(aoMaxStrength, ao_max_strength);
    GET_PARAM(aoCurve, ao_curve);
    GET_PARAM(numSamples, num_samples);
    GET_PARAM(maxSamples, max_samples);
    GET_PARAM(hmapRange, hmap_range);
    GET_PARAM(hmapCellSize, hmap_cell_size);
    GET_PARAM(precompResolution, clipmap_resolution);
    GET_PARAM(precompCascades, clipmap_cascades);
#undef GET_PARAM
  }

  fast_grass_types_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag fast_grass_type) const ecs::string &dagdp__name, const ecs::List<int> &dagdp__biomes,
      const ecs::string &fast_grass__impostor, float fast_grass__height, float fast_grass__weight_to_height_mul,
      float fast_grass__height_variance, float fast_grass__stiffness, E3DCOLOR fast_grass__color_mask_r_from,
      E3DCOLOR fast_grass__color_mask_r_to, E3DCOLOR fast_grass__color_mask_g_from, E3DCOLOR fast_grass__color_mask_g_to,
      E3DCOLOR fast_grass__color_mask_b_from, E3DCOLOR fast_grass__color_mask_b_to) {
      auto &gd = grassTypes.push_back();
      gd.name = dagdp__name;
      gd.impostorName = fast_grass__impostor;
      for (const auto biomeId : dagdp__biomes)
        gd.biomes.push_back(biomeId);
      gd.height = fast_grass__height;
      gd.w_to_height_mul = fast_grass__weight_to_height_mul;
      gd.heightVariance = fast_grass__height_variance;
      gd.stiffness = fast_grass__stiffness;
      gd.colors[0] = fast_grass__color_mask_r_from;
      gd.colors[1] = fast_grass__color_mask_r_to;
      gd.colors[2] = fast_grass__color_mask_g_from;
      gd.colors[3] = fast_grass__color_mask_g_to;
      gd.colors[4] = fast_grass__color_mask_b_from;
      gd.colors[5] = fast_grass__color_mask_b_to;
    });

  fastGrass.initOrUpdate(make_span(grassTypes));
  fastGrassChanged = false;
}

void GrassRenderer::initGrass(const DataBlock &grass_settings)
{
  const DataBlock *grCfg = ::dgs_get_settings()->getBlockByNameEx("graphics");

  if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    grassPrepass.set(grCfg->getBool("grassPrepass", grassPrepass.get()));
  else
    grassPrepass.set(false);
  if (!grCfg->getBool("grassPrepassEarly", true))
    logerr("Sorry, the late grass prepass feature was removed. Go away.");

  grass.base.varNotification = grassEditVarNotification.get();
  grass.init(grass_settings, renderer_has_feature(CAMERA_IN_CAMERA));
  grassEraserBuffer = dag::buffers::create_persistent_cb(MAX_GRASS_ERASERS, "grass_eraser_instances");
  grassEraserShader.init("grass_eraser", nullptr, 0, "grass_eraser");
  grassErasers.resize(MAX_GRASS_ERASERS);
  grassErasersActualSize = 0;
  grassErasersIndexToAdd = 0;
  grassErasersModified = false;

  grassSdfEraser.init("grass_sdf_eraser", true);
  sdfUpdatePeriod = max(1, grass_settings.getInt("sdf_eraser_update_period", grCfg->getInt("sdf_eraser_update_period", 2)));
  float eraserHeight = grass_settings.getReal("grass_eraser_sdf_height", -1);
  bool sdfEraserOn = (eraserHeight > 0);
  grassUseSdfEraser.set(sdfEraserOn);
  if (sdfEraserOn)
  {
    grassSdfEraserHeight.set(eraserHeight);
    ShaderGlobal::set_real(var::grass_eraser_sdf_height, eraserHeight);
  }

  initOrUpdateFastGrass();
  fast_grass_baker::init_fast_grass_baker();

  grassify.reset();
  const auto grassify_settings = grass_settings.getBlockByName("grassify");
  if (grassify_settings)
    grassify = eastl::make_unique<Grassify>(*grassify_settings, grass.getGrassDistance());

  grass_erasers_ecs_query([this](const ecs::Point4List &grass_erasers__spots) {
    setGrassErasers(grass_erasers__spots.size(), grass_erasers__spots.data());
  });
}

void GrassRenderer::initGrassRendinstClipmap()
{
  if (grassify)
    grassify->initGrassifyRendinst();

  // Note: technically the scene can have dynamically created objects that generate heightmap (currently there isn't any)
  if (!RendInstHeightmap::has_heightmap_objects_on_scene())
    return;

  auto *lmeshManager = WRDispatcher::getLandMeshManager();
  if (!lmeshManager)
    return;

  G_ASSERT_RETURN(lmeshManager->getHmapHandler(), );

  const int texelDensityMul = 8;
  BBox3 worldBox = lmeshManager->getHmapHandler()->getWorldBox();
  rendInstHeightmap =
    eastl::make_unique<RendInstHeightmap>(eastl::min(get_bigger_pow2((int)grass.getGrassDistance() * 2 * texelDensityMul), 2048),
      grass.getGrassDistance() * 2.0f, worldBox.lim[0].y, worldBox.lim[1].y);
}

void GrassRenderer::invalidateGrass(bool regenerate)
{
  grass.invalidate(regenerate);
  sdfBoxesToUpdate.clear();
}

void GrassRenderer::invalidateGrassBoxes(const dag::ConstSpan<BBox2> &boxes) { grass.invalidateBoxes(boxes); }

void GrassRenderer::invalidateGrassBoxes(const dag::ConstSpan<BBox3> &boxes) { grass.invalidateBoxes(boxes); }

void GrassRenderer::toggleCameraInCamera(const bool v)
{
  const bool inited = grass.base.toggleCameraInCameraView(v);
  if (!inited)
    logerr("grassRenderer: failed to toggle camcam ctx view to %d", v);
}

void render_grass_prepass(const GrassView view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderGrassPrepass(view); });
}

void render_grass_visibility_pass(const GrassView view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderGrassVisibilityPass(view); });
}

void resolve_grass_visibility(const GrassView view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.resolveGrassVisibility(view); });
}

void render_grass(const GrassView view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderGrass(view); });
}

void render_fast_grass(const TMatrix4 &globtm, const Point3 &view_pos)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderFastGrass(globtm, view_pos); });
}

DataBlock load_grass_settings(ecs::EntityId eid)
{
  const ecs::Array &entityGrassTypes = g_entity_mgr->get<ecs::Array>(eid, ECS_HASH("grass_types"));
  if (entityGrassTypes.size() == 0)
    return DataBlock();

  DataBlock grassBlock;
  grassBlock.setReal("grass_grid_size", g_entity_mgr->getOr(eid, ECS_HASH("grass_grid_size"), 0.5f));
  grassBlock.setReal("grass_distance", g_entity_mgr->getOr(eid, ECS_HASH("grass_distance"), 100.0f));
  grassBlock.setInt("grassMaskResolution", g_entity_mgr->getOr(eid, ECS_HASH("grassMaskResolution"), 512));
  grassBlock.setReal("hor_size_mul", g_entity_mgr->getOr(eid, ECS_HASH("hor_size_mul"), 1.0f));
  grassBlock.setReal("grassFarRange", g_entity_mgr->getOr(eid, ECS_HASH("grassFarRange"), -1.0f));
  grassBlock.setInt("grassFarMaskResolution", g_entity_mgr->getOr(eid, ECS_HASH("grassFarMaskResolution"), 512));

  if (g_entity_mgr->getOr(eid, ECS_HASH("grassify"), false))
  {
    // its blk params are deprecated, only its existence is checked
    DataBlock &grassifyBlock = *grassBlock.addNewBlock("grassify");
    grassifyBlock.setInt("grassifyMaskResolution", g_entity_mgr->getOr(eid, ECS_HASH("grassifyMaskResolution"), 512));
  }

  DataBlock *grassTypesBlock = grassBlock.addNewBlock("grass_types");

  for (auto &grassType : entityGrassTypes)
  {
    const ecs::Object &obj = grassType.get<ecs::Object>();
    DataBlock *typeBlock = grassTypesBlock->addNewBlock(obj[ECS_HASH("name")].get<ecs::string>().c_str());
    if (!obj[ECS_HASH("diffuse")].isNull())
      typeBlock->setStr("diffuse", obj[ECS_HASH("diffuse")].getStr());
    if (!obj[ECS_HASH("normal")].isNull())
      typeBlock->setStr("normal", obj[ECS_HASH("normal")].getStr());
    if (!obj[ECS_HASH("height")].isNull())
      typeBlock->setReal("height", obj[ECS_HASH("height")].get<float>());
    if (!obj[ECS_HASH("size_lod_mul")].isNull())
      typeBlock->setReal("size_lod_mul", obj[ECS_HASH("size_lod_mul")].get<float>());
    if (!obj[ECS_HASH("size_lod_wide_mul")].isNull())
      typeBlock->setReal("size_lod_wide_mul", obj[ECS_HASH("size_lod_wide_mul")].get<float>());
    if (!obj[ECS_HASH("ht_rnd_add")].isNull())
      typeBlock->setReal("ht_rnd_add", obj[ECS_HASH("ht_rnd_add")].get<float>());
    if (!obj[ECS_HASH("hor_size")].isNull())
      typeBlock->setReal("hor_size", obj[ECS_HASH("hor_size")].get<float>());
    if (!obj[ECS_HASH("hor_size_rnd_add")].isNull())
      typeBlock->setReal("hor_size_rnd_add", obj[ECS_HASH("hor_size_rnd_add")].get<float>());
    if (!obj[ECS_HASH("height_rnd_add")].isNull())
      typeBlock->setReal("height_rnd_add", obj[ECS_HASH("height_rnd_add")].get<float>());
    if (!obj[ECS_HASH("height_from_weight_mul")].isNull())
      typeBlock->setReal("height_from_weight_mul", obj[ECS_HASH("height_from_weight_mul")].get<float>());
    if (!obj[ECS_HASH("height_from_weight_add")].isNull())
      typeBlock->setReal("height_from_weight_add", obj[ECS_HASH("height_from_weight_add")].get<float>());
    if (!obj[ECS_HASH("density_from_weight_mul")].isNull())
      typeBlock->setReal("density_from_weight_mul", obj[ECS_HASH("density_from_weight_mul")].get<float>());
    if (!obj[ECS_HASH("density_from_weight_add")].isNull())
      typeBlock->setReal("density_from_weight_add", obj[ECS_HASH("density_from_weight_add")].get<float>());
    if (!obj[ECS_HASH("vertical_angle_add")].isNull())
      typeBlock->setReal("vertical_angle_add", obj[ECS_HASH("vertical_angle_add")].get<float>());
    if (!obj[ECS_HASH("vertical_angle_mul")].isNull())
      typeBlock->setReal("vertical_angle_mul", obj[ECS_HASH("vertical_angle_mul")].get<float>());
    if (!obj[ECS_HASH("color_mask_r_from")].isNull())
      typeBlock->setE3dcolor("color_mask_r_from", obj[ECS_HASH("color_mask_r_from")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("color_mask_r_to")].isNull())
      typeBlock->setE3dcolor("color_mask_r_to", obj[ECS_HASH("color_mask_r_to")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("color_mask_g_from")].isNull())
      typeBlock->setE3dcolor("color_mask_g_from", obj[ECS_HASH("color_mask_g_from")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("color_mask_g_to")].isNull())
      typeBlock->setE3dcolor("color_mask_g_to", obj[ECS_HASH("color_mask_g_to")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("color_mask_b_from")].isNull())
      typeBlock->setE3dcolor("color_mask_b_from", obj[ECS_HASH("color_mask_b_from")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("color_mask_b_to")].isNull())
      typeBlock->setE3dcolor("color_mask_b_to", obj[ECS_HASH("color_mask_b_to")].get<E3DCOLOR>());
    if (!obj[ECS_HASH("tile_tc_x")].isNull())
      typeBlock->setReal("tile_tc_x", obj[ECS_HASH("tile_tc_x")].get<float>());
    if (!obj[ECS_HASH("stiffness")].isNull())
      typeBlock->setReal("stiffness", obj[ECS_HASH("stiffness")].get<float>());
    if (!obj[ECS_HASH("horizontal_grass")].isNull())
      typeBlock->setBool("horizontal_grass", obj[ECS_HASH("horizontal_grass")].get<bool>());
    if (!obj[ECS_HASH("underwater")].isNull())
      typeBlock->setBool("underwater", obj[ECS_HASH("underwater")].get<bool>());
    if (!obj[ECS_HASH("variations")].isNull())
      typeBlock->setInt("variations", obj[ECS_HASH("variations")].get<int>());
    if (!obj[ECS_HASH("porosity")].isNull())
      typeBlock->setReal("porosity", obj[ECS_HASH("porosity")].get<float>());
  }

  DataBlock *decalsBlock = grassBlock.addNewBlock("decals");
  const ecs::Array &entityDecals = g_entity_mgr->get<ecs::Array>(eid, ECS_HASH("decals"));
  for (auto &decal : entityDecals)
  {
    const ecs::Object &obj = decal.get<ecs::Object>();
    DataBlock *decalBlock = decalsBlock->addNewBlock(obj[ECS_HASH("name")].get<ecs::string>().c_str());
    decalBlock->setInt("id", obj[ECS_HASH("id")].get<int>());
    const ecs::Array &decalWeights = obj[ECS_HASH("weights")].get<ecs::Array>();
    for (auto &weight : decalWeights)
    {
      const ecs::Object weightObject = weight.get<ecs::Object>();
      decalBlock->setReal(weightObject[ECS_HASH("type")].get<ecs::string>().c_str(), weightObject[ECS_HASH("weight")].get<float>());
    }
  }
  return grassBlock;
}

void init_grass(const DataBlock *grass_settings_from_level_fallback)
{
  ecs::EntityId grassSettingsEid = g_entity_mgr->getSingletonEntity(ECS_HASH("grass_settings"));
  DataBlock loadedGrassSettings;
  if (grassSettingsEid != ecs::INVALID_ENTITY_ID)
  {
    loadedGrassSettings = load_grass_settings(grassSettingsEid);
  }
  else
  {
    logwarn("No grass entity found! Loading grass_render from level blk fallback.");
  }

  const DataBlock *grassSettings = loadedGrassSettings.isEmpty() ? grass_settings_from_level_fallback : &loadedGrassSettings;

  if (!grassSettings)
    return;
  g_entity_mgr->createEntitySync("grass_render");

  init_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    grass_render.initGrass(*grassSettings);
    grass_render.invalidateGrass(true);
  });
}

ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
ECS_REQUIRE(const GrassRenderer &grass_render)
void init_grass_render_es(const ecs::Event &evt, dafg::NodeHandle &grass_node)
{
  if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    return;

  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!changedFeatures->isFeatureChanged(FeatureRenderFlags::CAMERA_IN_CAMERA))
      return;
    else
      init_grass_render_ecs_query(
        [&](GrassRenderer &grass_render) { grass_render.toggleCameraInCamera(changedFeatures->hasFeature(CAMERA_IN_CAMERA)); });
  }

  auto decoNs = dafg::root() / "opaque" / "decorations";
  grass_node = decoNs.registerNode("fullres_grass_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("grass_generated_token").optional();

    shaders::OverrideState st;
    st.set(shaders::OverrideState::Z_FUNC);
    st.zFunc = CMPF_EQUAL;

    use_camera_in_camera(registry);
    registry.requestState().allowWireframe().setFrameBlock("global_frame").enableOverride(st);

    render_to_gbuffer(registry).vrsRate(VRS_RATE_TEXTURE_NAME);

    if (grass_supports_visibility_prepass())
      registry.read("grass_visibility_resolved").blob<OrderingToken>();

    // For grass billboards
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

    return [](const dafg::multiplexing::Index &multiplexing_index) {
      const camera_in_camera::ApplyMasterState camcam{multiplexing_index};

      const GrassView gv = multiplexing_index.subCamera == 0 ? GrassView::Main : GrassView::CameraInCamera;
      render_grass(gv);
    };
  });
}

ECS_ON_EVENT(EventRendinstsLoaded)
void init_grass_ri_clipmap_es(const ecs::Event &, GrassRenderer &grass_render) { grass_render.initGrassRendinstClipmap(); }

ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__bare_minimum, render_settings__grassQuality)
static void grass_quiality_es(const ecs::Event &, bool render_settings__bare_minimum, const ecs::string &render_settings__grassQuality)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    const GrassQuality newQuality =
      render_settings__bare_minimum ? GrassQuality::GRASS_QUALITY_LOW : str_to_grass_quality(render_settings__grassQuality.c_str());
    grass_render.grass.setQuality(newQuality);
  });
}


ECS_TRACK(render_settings__anisotropy)
ECS_REQUIRE(int render_settings__anisotropy)
static void track_anisotropy_change_es(const ecs::Event &, GrassRenderer &grass_render) { grass_render.grass.applyAnisotropy(); }

void grass_prepare(const CameraParams &main_view, const eastl::optional<CameraParams> &camcam_view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    grass_render.generateGrassPerCamera(main_view.viewItm);

    grass_render.generateGrassPerView(GrassView::Main, main_view.noJitterFrustum, main_view.viewItm, main_view.noJitterPersp);
    if (camcam_view.has_value())
      grass_render.generateGrassPerView(GrassView::CameraInCamera, camcam_view->noJitterFrustum, camcam_view->viewItm,
        camcam_view->noJitterPersp);
  });
}

void grass_prepare_per_camera(const CameraParams &main_view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.generateGrassPerCamera(main_view.viewItm); });
}

void grass_prepare_per_view(const CameraParams &view, const bool is_main_view)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    grass_render.generateGrassPerView(is_main_view ? GrassView::Main : GrassView::CameraInCamera, view.noJitterFrustum, view.viewItm,
      view.noJitterPersp);
  });
}

void grass_invalidate()
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.invalidateGrass(false); });
}

void grass_invalidate(const dag::ConstSpan<BBox3> &boxes)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.invalidateGrassBoxes(boxes); });
}

#include <drv/3d/dag_resetDevice.h>

static void reset_grass_render(bool full_reset)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    if (!full_reset)
    {
      grass_render.invalidateGrass(false);
      return;
    }

    if (grass_render.rendInstHeightmap)
      grass_render.rendInstHeightmap->driverReset();

    grass_render.grass.driverReset();
    grass_render.grassErasersModified = true;
  });
}

REGISTER_D3D_AFTER_RESET_FUNC(reset_grass_render);