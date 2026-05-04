// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <billboardDecals/billboardDecals.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderEvent.h>

static constexpr int MAX_HOLES_IN_IB = 16384;

using BillboardDecalsPtr = eastl::unique_ptr<BillboardDecals>;

ECS_DECLARE_RELOCATABLE_TYPE(BillboardDecalsPtr);
ECS_REGISTER_RELOCATABLE_TYPE(BillboardDecalsPtr, nullptr);

template <typename Callable>
inline void get_billboard_manager_ecs_query(ecs::EntityManager &manager, Callable c);


void add_billboard_decal(const Point3 &pos, const Point3 &norm, int texture_idx, float size)
{
  get_billboard_manager_ecs_query(*g_entity_mgr, [&](BillboardDecalsPtr &billboard_decals__mgr) {
    if (billboard_decals__mgr)
    {
      uint32_t decalMatrixId = DecalsMatrices::INVALID_MATRIX_ID;
      int32_t hole_id = billboard_decals__mgr->addHole(pos, norm, Point3::ZERO, size, texture_idx, decalMatrixId);
      G_UNUSED(hole_id);
    }
    return ecs::QueryCbResult::Stop;
  });
}

void erase_billboard_decals(const Point3 &world_pos, float radius)
{
  get_billboard_manager_ecs_query(*g_entity_mgr, [&](BillboardDecalsPtr &billboard_decals__mgr) {
    if (billboard_decals__mgr)
    {
      billboard_decals__mgr->clearInSphere(world_pos, radius);
    }
    return ecs::QueryCbResult::Stop;
  });
}

void erase_all_billboard_decals()
{
  get_billboard_manager_ecs_query(*g_entity_mgr, [&](BillboardDecalsPtr &billboard_decals__mgr) {
    if (billboard_decals__mgr)
    {
      billboard_decals__mgr->clear();
    }
    return ecs::QueryCbResult::Stop;
  });
}

ECS_TAG(render)
ECS_ON_EVENT(AfterDeviceReset)
static void bullet_holes_after_device_reset_es(const ecs::Event &, BillboardDecalsPtr &billboard_decals__mgr)
{
  if (billboard_decals__mgr)
    billboard_decals__mgr->afterReset();
}

ECS_TAG(render)
static void bullet_holes_before_render_es(const UpdateStageInfoBeforeRender &evt, BillboardDecalsPtr &billboard_decals__mgr)
{
  if (billboard_decals__mgr)
    billboard_decals__mgr->prepareRender(evt.mainCullingFrustum, evt.camPos);
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderDecals)
static inline void bullet_holes_render_es(const ecs::Event &, BillboardDecalsPtr &billboard_decals__mgr)
{
  if (billboard_decals__mgr)
    billboard_decals__mgr->renderBillboards();
}


ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void bullet_holes_on_level_loaded_es(const ecs::Event &,
  BillboardDecalsPtr &billboard_decals__mgr,
  int billboard_decals__maxDecals,
  const ecs::string &billboard_decals__diffuseTextureArray,
  const ecs::string &billboard_decals__normalsTextureArray,
  float billboard_decals__depthHardDist,
  float billboard_decals__depthSoftnessDist)
{

  unsigned maxRegularBulletHoles = min((unsigned)billboard_decals__maxDecals, (unsigned)MAX_HOLES_IN_IB);
  billboard_decals__mgr.reset(
    BillboardDecals::create(billboard_decals__depthHardDist, billboard_decals__depthSoftnessDist, maxRegularBulletHoles));

  SharedTexHolder diffuseTextureArray =
    dag::get_tex_gameres(billboard_decals__diffuseTextureArray.c_str(), "billboard_decals_diff_tex");
  SharedTexHolder normalTextureArray =
    dag::get_tex_gameres(billboard_decals__normalsTextureArray.c_str(), "billboard_decals_bump_tex");
  billboard_decals__mgr->init_textures(eastl::move(diffuseTextureArray), eastl::move(normalTextureArray));
}
