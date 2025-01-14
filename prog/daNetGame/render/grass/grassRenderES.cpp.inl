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
#include <render/world/lmesh_modes.h>
#include <render/world/wrDispatcher.h>
#include <render/renderEvent.h>
#include <render/daBfg/ecs/frameGraphNode.h>
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
CONSOLE_BOOL_VAL("grass", grassPrepassEarly, true);

CONSOLE_BOOL_VAL("grass", grassColorIgnoreTerrainDetail, false);

class RandomGrassRenderHelper final : public IRandomGrassRenderHelper
{
public:
  LandMeshRenderer &renderer;
  LandMeshManager &provider;
  ViewProjMatrixContainer oviewproj;
  TMatrix4_vec4 globTm;
  Frustum frustum;
  LMeshRenderingMode omode;
  int oblock;

  RandomGrassRenderHelper(LandMeshRenderer &r,
    LandMeshManager &p,
    float rendInstHeightmapMaxHt,
    uint32_t eraser_count,
    Sbuffer *eraser_instance_buffer,
    ShaderElement *grass_eraser) :
    renderer(r),
    provider(p),
    oviewproj(),
    omode(LMeshRenderingMode::RENDERING_LANDMESH),
    oblock(),
    rendInstHeightmapMaxHt(rendInstHeightmapMaxHt),
    eraserCount(eraser_count),
    eraserInstanceBuffer(eraser_instance_buffer),
    grassEraser(grass_eraser)
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

bool RandomGrassRenderHelper::beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &glob_tm, const TMatrix4 &)
{
  d3d_get_view_proj(oviewproj);
  omode = lmesh_rendering_mode;

  globTm = glob_tm;
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

  set_lmesh_rendering_mode(omode);
  renderer.setRenderInBBox(BBox3());
  ShaderGlobal::setBlock(oblock, ShaderGlobal::LAYER_FRAME);
}

void RandomGrassRenderHelper::renderMask()
{
  set_lmesh_rendering_mode(LMeshRenderingMode::GRASS_MASK);
  renderer.render(reinterpret_cast<mat44f_cref>(globTm), frustum, provider, LandMeshRenderer::RENDER_GRASS_MASK, ::grs_cur_view.pos,
    LandMeshRenderer::RENDER_FOR_GRASS);
  if (eraserCount > 0 && eraserInstanceBuffer && grassEraser)
  {
    TIME_D3D_PROFILE(grass_erasers);
    index_buffer::use_quads_16bit();
    grassEraser->setStates(0, true);
    d3d::setvsrc(0, 0, 0);

    d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, eraserCount);
  }
}

void RandomGrassRenderHelper::renderColor()
{
  set_lmesh_rendering_mode(
    grassColorIgnoreTerrainDetail.get() ? LMeshRenderingMode::GRASS_COLOR : LMeshRenderingMode::RENDERING_CLIPMAP);
  renderer.render(reinterpret_cast<mat44f_cref>(globTm), frustum, provider, LandMeshRenderer::RENDER_CLIPMAP, ::grs_cur_view.pos,
    LandMeshRenderer::RENDER_FOR_GRASS);
}
ECS_REGISTER_RELOCATABLE_TYPE(GrassRenderer, nullptr);

void GrassRenderer::renderGrassPrepassInternal()
{
  TIME_D3D_PROFILE(grass_prepass);
  grass.render(grass.GRASS_DEPTH_PREPASS);
}


void GrassRenderer::renderGrassPrepass()
{
  if (!grassRender.get())
    return;
  if (grassPrepass.get() && grassPrepassEarly.get())
  {
    renderGrassPrepassInternal();
  }
}
void GrassRenderer::renderGrass()
{
  if (!grassRender.get())
    return;
  if (grassPrepass.get())
  {
    if (!grassPrepassEarly.get())
      renderGrassPrepassInternal();
    grass.render(grass.GRASS_AFTER_PREPASS);
  }
  else
    grass.render(grass.GRASS_NO_PREPASS);
}

void GrassRenderer::generateGrass(const TMatrix &itm, const Driver3dPerspective &perspective)
{
  auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  auto *lmeshManager = WRDispatcher::getLandMeshManager();
  if (!grassRender.get() || !lmeshManager)
    return;
  float rendInstHeightmapMaxHt = -10000.0f;
  if (rendInstHeightmap)
  {
    rendInstHeightmap->updateToroidalTextureRegions(globalFrameBlockId);
    rendInstHeightmapMaxHt = rendInstHeightmap->getMaxHt();

    // This call can initiate RI visibility job (culling) which will be executed in other thread. We can do it before
    // `updateToroidalTextureRegions` but then we need to do it as early as possible to not waste time on wait.
    // I decided to do it here because most likely texture update will be delayed anyway until required lods
    // for RI are loaded.
    rendInstHeightmap->updatePos(itm.getcol(3));
  }

  if (::grs_draw_wire)
    d3d::setwire(0);

  if (grassEraserBuffer.getBuf() && grassErasersModified && grassErasersActualSize > 0)
  {
    grassEraserBuffer.getBuf()->updateDataWithLock(0, sizeof(grassErasers[0]) * grassErasersActualSize, grassErasers.begin(),
      VBLOCK_DISCARD);
    grassErasersModified = false;
    d3d::resource_barrier({grassEraserBuffer.getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
  }

  if (grassColorIgnoreTerrainDetail.pullValueChange())
    grass.invalidate();

  RandomGrassRenderHelper rhlp(*lmeshRenderer, *lmeshManager, rendInstHeightmapMaxHt, min(grassErasersActualSize, MAX_GRASS_ERASERS),
    grassEraserBuffer.getBuf(), grassEraserShader.shader);

  grass.generate(itm.getcol(3), itm.getcol(2), rhlp);

  if (grassify)
  {
    grassify->generate(itm.getcol(3), itm, perspective, grass.getMaskTex(), rhlp, grass.base);
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

void GrassRenderer::initGrass(const DataBlock &grass_settings)
{
  const DataBlock *grCfg = ::dgs_get_settings()->getBlockByNameEx("graphics");

  if (!renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    grassPrepass.set(grCfg->getBool("grassPrepass", grassPrepass.get()));
  else
    grassPrepass.set(false);
  grassPrepassEarly.set(grCfg->getBool("grassPrepassEarly", grassPrepassEarly.get()));

  grass.base.varNotification = grassEditVarNotification.get();
  grass.init(grass_settings);
  grassEraserBuffer = dag::buffers::create_persistent_cb(MAX_GRASS_ERASERS, "grass_eraser_instances");
  grassEraserShader.init("grass_eraser", nullptr, 0, "grass_eraser");
  grassErasers.resize(MAX_GRASS_ERASERS);
  grassErasersActualSize = 0;
  grassErasersIndexToAdd = 0;
  grassErasersModified = false;

  grassify.reset();
  const auto grassify_settings = grass_settings.getBlockByName("grassify");
  if (grassify_settings)
    grassify = eastl::make_unique<Grassify>(*grassify_settings, grass.getMaskResolution(), grass.getGrassDistance());

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

void GrassRenderer::invalidateGrass(bool regenerate) { grass.invalidate(regenerate); }

void GrassRenderer::invalidateGrassBoxes(const dag::ConstSpan<BBox2> &boxes) { grass.invalidateBoxes(boxes); }

void GrassRenderer::invalidateGrassBoxes(const dag::ConstSpan<BBox3> &boxes) { grass.invalidateBoxes(boxes); }

void render_grass_prepass()
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderGrassPrepass(); });
}

void render_grass()
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.renderGrass(); });
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

  if (g_entity_mgr->getOr(eid, ECS_HASH("grassify"), false))
  {
    // its blk params are deprecated, only its existence is checked
    grassBlock.addNewBlock("grassify");
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

  init_grass_render_ecs_query([&](GrassRenderer &grass_render, dabfg::NodeHandle &grass_node) {
    grass_render.initGrass(*grassSettings);
    grass_render.invalidateGrass(true);

    if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
      return;

    auto decoNs = dabfg::root() / "opaque" / "decorations";
    grass_node = decoNs.registerNode("fullres_grass_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      auto state = registry.requestState().allowWireframe().setFrameBlock("global_frame");

      auto pass = render_to_gbuffer(registry);

      use_default_vrs(pass, state);

      // For grass billboards
      registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
      registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();

      return []() {
        extern void render_grass();
        render_grass();
      };
    });
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

void grass_prepare(const TMatrix &itm, const Driver3dPerspective &perspective)
{
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) { grass_render.generateGrass(itm, perspective); });
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

static void reset_grass_render_es(const AfterDeviceReset &event)
{
  if (!event.get<0>())
    return;
  get_grass_render_ecs_query([&](GrassRenderer &grass_render) {
    if (grass_render.rendInstHeightmap)
      grass_render.rendInstHeightmap->driverReset();

    grass_render.grass.driverReset();
    grass_render.grassErasersModified = true;
  });
}
