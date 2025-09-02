// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gameObjects.h"
#include <scene/dag_objsToPlace.h>
#include <daECS/core/updateStage.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/template.h>
#include <ecs/core/attributeEx.h>
#include <ecs/core/utility/ecsRecreate.h>
#include "level.h"
#include <debug/dag_debug3d.h>
#include <daEditorE/editorCommon/inGameEditor.h>
#include <math/dag_rayIntersectBox.h>
#include <math/random/dag_random.h>
#include "phys/gridCollision.h"
#include <ecs/game/generic/grid.h>
#include <ecs/render/updateStageRender.h>
#include <ioSys/dag_chainedMemIo.h>

#define ENTITIES_BBOXES_VISIBILITY_RADIUS 50.f
bool should_hide_debug();


ECS_REGISTER_RELOCATABLE_TYPE(GameObjects, nullptr);
ECS_AUTO_REGISTER_COMPONENT(GameObjects, "game_objects", nullptr, 0);

template <typename Callable>
static void all_grid_holders_ecs_query(Callable fn);

scene::TiledScene *create_ladders_scene(GameObjects &game_objects)
{
  auto insertPair = game_objects.objects.emplace("ladders", nullptr);
  if (insertPair.second) // if inserted
  {
    auto scn = new GameObjectsTiledScene();
    insertPair.first->second.reset(scn);
    scn->setPoolBBox(0, bbox3f{v_neg(V_C_HALF), V_C_HALF});
    game_objects.ladders = scn;
  }
  return insertPair.first->second.get();
}

void create_game_objects(Tab<ObjectsToPlace *> &&objectsToPlace)
{
  if (objectsToPlace.empty())
    return;
  G_ASSERT(!g_entity_mgr->getSingletonEntity(ECS_HASH("game_objects")));
  g_entity_mgr->createEntityAsync("game_objects", ecs::ComponentsInitializer(),
    [objs2p = eastl::move(objectsToPlace)](ecs::EntityId go_eid) {
      auto game_objects = ECS_GET_COMPONENT(GameObjects, go_eid, game_objects);
      for (ObjectsToPlace *objMap : objs2p)
      {
        G_ASSERT(objMap->typeFourCc == _MAKE4C('GmO'));
        if (game_objects) // client doesn't have this component
        {
          scene::TiledScene *indoor_walls = nullptr;
          for (auto &obj : objMap->objs)
          {
            const RoDataBlock &blk = *obj.addData.getBlockByNameEx("__asset");
            GameObjectsTiledScene *scn = nullptr;
            const int numLadderSteps = blk.getInt("ladderStepsCount", 0);
            const bool isLadders = numLadderSteps != 0 && blk.getBool("isLadder", false);
            const char *soundType = blk.getStr("soundType", nullptr);
            const bool isSounds = soundType != nullptr && *soundType;

            const char *scn_name = obj.resName.get();
            auto insertPair = game_objects->objects.emplace(isLadders ? "ladders" : isSounds ? "sounds" : scn_name, nullptr);

            if (insertPair.second) // if inserted
            {
              scn = new GameObjectsTiledScene();
              insertPair.first->second.reset(scn);
              scn->setPoolBBox(0, bbox3f{v_neg(V_C_HALF), V_C_HALF});
              if (isLadders)
                game_objects->ladders = scn;
              else if (isSounds)
              {
                game_objects->sounds = scn;
                game_objects->soundAttributes.clear();
              }
              else if (strcmp(scn_name, "indoors") == 0)
                game_objects->indoors = scn;
              else if (strcmp(scn_name, "indoor_walls") == 0)
                indoor_walls = scn;
            }
            else
              scn = insertPair.first->second.get();

            if (isLadders)
            {
              for (const TMatrix &tm : obj.tm)
              {
                mat44f m;
                v_mat44_make_from_43cu_unsafe(m, tm[0]);
                scn->allocate(m, /*pool*/ 0, /*flags*/ numLadderSteps);
              }
            }
            else if (isSounds)
            {
              const char *soundShape = blk.getStr("soundShape", "box");
              game_objects->soundAttributes.reserve(game_objects->soundAttributes.size() + obj.tm.size());
              for (const TMatrix &tm : obj.tm)
              {
                mat44f m;
                v_mat44_make_from_43cu_unsafe(m, tm[0]);
                const GameObjectSoundAttributes::id_t id = game_objects->soundAttributes.add(soundType, soundShape, tm);
                scn->allocate(m, /*pool*/ 0, /*flags*/ uint16_t(id));
              }
            }
            else
            {
              G_ASSERTF(obj.tm.size() <= 0x10000, "obj.tm.size()=%d", obj.tm.size());
              bool need_add_data = false;
              for (const TMatrix &tm : obj.tm)
              {
                mat44f m;
                v_mat44_make_from_43cu_unsafe(m, tm[0]);
                unsigned idx = &tm - obj.tm.data();
                scn->allocate(m, /*pool*/ 0, /*flags*/ idx);
                if (obj.addData.getBlock(idx)->paramCount() || obj.addData.getBlock(idx)->blockCount())
                  need_add_data = true;
              }

              if (need_add_data)
              {
                DataBlock addData;
                addData.setFrom(obj.addData);
                MemorySaveCB cwr(64 << 10);
                if (addData.saveToStream(cwr))
                {
                  addData.reset();
                  scn->addData.reset(new DataBlock);
                  MemoryLoadCB crd(cwr.takeMem(), true);
                  if (scn->addData->loadFromStream(crd, scn_name))
                    debug("GmO(%s): optimized to ROM BLK format, size=%dK, %d instances", scn_name, crd.getTargetDataSize() >> 10,
                      scn->addData->blockCount());
                  else
                    logerr("%s: failed to reload GmO(%s) addData BLK from memory stream (sz=%d)", __FUNCTION__, scn_name,
                      crd.getTargetDataSize());
                }
                else
                  logerr("%s: failed to save GmO(%s) addData BLK to memory stream", __FUNCTION__, scn_name);
              }
            }
          }
          if (!game_objects->indoors && indoor_walls) // if no native "indoors" inherit from "indoor_walls"
          {
            // NOTE: We have to clone "indoor_walls" scene instead of just pointing to it, because it will
            //       be stolen by render on client with local server, but we need "indoors" scene for game
            //       on any server, including local server (generally low memory footprint 1Kb..64Kb)
            //
            auto insertPair = game_objects->objects.emplace("indoors", nullptr);
            if (insertPair.second) // if inserted
            {
              GameObjectsTiledScene *indoors = new GameObjectsTiledScene();
              insertPair.first->second.reset(indoors);
              indoors->setPoolBBox(0, bbox3f{v_neg(V_C_HALF), V_C_HALF});
              indoors->reserve(indoor_walls->getNodesCount());
              for (scene::node_index ni : *indoor_walls)
                indoors->allocate(indoor_walls->getNode(ni), /*pool*/ 0, /*flags*/ 0);
              game_objects->indoors = indoors;
            }
          }
        }
        objMap->destroy();
      }
      if (game_objects)
      {
        g_entity_mgr->broadcastEvent(EventGameObjectsCreated(go_eid));
        g_entity_mgr->broadcastEvent(EventGameObjectsOptimize(go_eid));
      }
    });
  G_ASSERT(objectsToPlace.empty());
}

static void draw_boxes(const GameObjects &game_objects, mat44f_cref globtm, const Point3 &viewPos, float distSq, uint8_t alpha)
{
  BBox3 box(Point3(-0.5, -0.5, -0.5), Point3(0.5, 0.5, 0.5));
  vec4f pos_distscale = v_make_vec4f(viewPos.x, viewPos.y, viewPos.z, scene::defaultDisappearSq / distSq);
  for (const auto &it : game_objects.objects)
  {
    it.second->frustumCull<false, true, false>(globtm, pos_distscale, 0, 0, nullptr, [&](scene::node_index, mat44f_cref m, vec4f) {
      alignas(16) TMatrix tm;
      v_mat_43ca_from_mat44(tm.m[0], m);
      set_cached_debug_lines_wtm(tm);
      draw_cached_debug_box(box, E3DCOLOR(127, 127, 127, alpha));
    });
  }
}

ECS_TAG(server, dev, render)
static void game_objects_es(const UpdateStageInfoRenderDebug &info, const GameObjects &game_objects)
{
  if (!get_da_editor4().isActive())
    return;
  begin_draw_cached_debug_lines(false, false);
  draw_boxes(game_objects, info.globtm, info.viewItm.getcol(3), 80 * 80, 0x1f);
  end_draw_cached_debug_lines();

  begin_draw_cached_debug_lines(true, false);
  draw_boxes(game_objects, info.globtm, info.viewItm.getcol(3), 80 * 80, 0x80);
  end_draw_cached_debug_lines();

  set_cached_debug_lines_wtm(TMatrix::IDENT);
}

static void game_objects_bspheres_common(const UpdateStageInfoRenderDebug &info)
{
  if (should_hide_debug())
    return;
  begin_draw_cached_debug_lines_ex();
  BSphere3 bsph(info.viewItm.getcol(3), ENTITIES_BBOXES_VISIBILITY_RADIUS);
  for_each_grid_holder([&](const GridHolder &grid_holder) {
    for_each_entity_in_grid(&grid_holder, bsph, GridEntCheck::POS, [](ecs::EntityId, vec3f wbsph) {
      float radius = v_extract_w(wbsph);
      if (radius > 0.1f)
      {
        Point3_vec4 pos;
        v_st(&pos.x, wbsph);
        draw_cached_debug_sphere(pos, radius, E3DCOLOR_MAKE(200, 0, 200, 255));
      }
    });
  });
  end_draw_cached_debug_lines_ex();
}

ECS_NO_ORDER
ECS_TAG(dev, render)
ECS_REQUIRE(ecs::Tag collision_debug)
static void game_objects_bspheres_es(const UpdateStageInfoRenderDebug &info) { game_objects_bspheres_common(info); }


ECS_TAG(server)
static void game_objects_events_es_event_handler(const EventGameObjectsCreated &,
  GameObjects &game_objects,
  const int *game_objects__seed,
  const ecs::Object *game_objects__syncCreation,
  const ecs::Object &game_objects__createChance)
{
  int seed = game_objects__seed ? *game_objects__seed : get_rnd_seed();
  debug("game obj seed: %d", seed);
  int count = 0;
  for (const auto &instances : game_objects.objects)
  {
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(instances.first))
    {
      logwarn("missing template <%s> for game_object", instances.first);
      continue;
    }
    const GameObjectsTiledScene &scene = *instances.second;
    eastl::string templName = add_sub_template_name<eastl::string>(instances.first.c_str(), "game_object");
    const ecs::HashedConstString instanceHash = ECS_HASH_SLOW(instances.first.c_str());
    float chance = game_objects__createChance.getMemberOr(instanceHash, 1.f);
    int nid_id = -1, nid_pid = -1, nid_tm = -1;
    if (scene.addData)
    {
      nid_id = scene.addData->getNameId("hierarchy_unresolved_id");
      nid_pid = scene.addData->getNameId("hierarchy_unresolved_parent_id");
      nid_tm = scene.addData->getNameId("hierarchy_transform");
    }
    for (scene::node_index ni : scene)
    {
      if (_frnd(seed) > chance)
        continue;
      alignas(16) TMatrix tm;
      v_mat_43ca_from_mat44(tm.m[0], scene.getNode(ni));
      ecs::ComponentsInitializer attrs;
      ECS_INIT(attrs, transform, tm);
      ECS_INIT(attrs, initialTransform, tm);
      if (nid_id >= 0 || nid_pid >= 0)
      {
        static constexpr int ID_OFFSET = 100000;
        unsigned idx = scene::get_node_flags(scene.getNode(ni));
        const DataBlock &b = *scene.addData->getBlock(idx);
        if (int c_id = b.getIntByNameId(nid_id, -1); c_id != -1)
          ECS_INIT(attrs, hierarchy_unresolved_id, c_id + ID_OFFSET);
        if (int p_id = b.getIntByNameId(nid_pid, -1); p_id != -1)
        {
          ECS_INIT(attrs, hierarchy_unresolved_parent_id, p_id + ID_OFFSET);
          ECS_INIT(attrs, hierarchy_transform, b.getTmByNameId(nid_tm, TMatrix::IDENT));
        }
      }

      if (!(game_objects__syncCreation && game_objects__syncCreation->getMemberOr(instanceHash, false)))
        g_entity_mgr->createEntityAsync(templName.c_str(), eastl::move(attrs));
      else
        g_entity_mgr->createEntitySync(templName.c_str(), eastl::move(attrs));
      count++;
    }
  }
  g_entity_mgr->broadcastEvent(EventGameObjectsEntitiesScheduled(count));
}

ECS_TAG(gameClient)
static void game_objects_client_events_es_event_handler(const EventGameObjectsOptimize &, GameObjects &game_objects)
{
  if (has_in_game_editor()) // we keep objects for visibility
    return;
  for (auto instances = game_objects.objects.begin(); instances != game_objects.objects.end();)
  {
    if (game_objects.needSceneInGame(instances->second.get()))
    {
      instances++;
      continue;
    }
    bool to_remove = instances->first == "loot_box" || instances->first == "loot_crate"; // hardcoded loot boxes
    if (to_remove || !g_entity_mgr->getTemplateDB().getTemplateByName(instances->first))
    {
      debug("keep game_obj %s", instances->first.c_str());
      instances++;
      continue;
    }
    debug("erase game_obj %s", instances->first.c_str());
    instances = game_objects.objects.erase(instances);
  }
}

ECS_TAG(server)
static void game_objects_events_es_event_handler(const EventGameObjectsOptimize &, GameObjects &game_objects)
{
  if (has_in_game_editor()) // we keep objects for visibility
    return;
  for (auto instances = game_objects.objects.begin(); instances != game_objects.objects.end();)
  {
    if (game_objects.needSceneInGame(instances->second.get()))
    {
      instances++;
      continue;
    }
    if (!g_entity_mgr->getTemplateDB().getTemplateByName(instances->first))
    {
      debug("keep game_obj %s", instances->first.c_str());
      instances++;
      continue;
    }
    debug("erase game_obj %s", instances->first.c_str());
    instances = game_objects.objects.erase(instances);
  }
}

void auto_rearrange_game_object_scene(scene::TiledScene &s, uint32_t split, const float max_tile_size)
{
  if (split <= 1)
    return;
  bbox3f box;
  v_bbox3_init_empty(box);
  for (auto ni : s)
    v_bbox3_add_pt(box, s.getNode(ni).col3);
  vec3f size = v_bbox3_size(box);
  size = v_max(size, v_perm_zwxy(size));
  float sz = v_extract_x(size);
  const float newTileSize = min(max_tile_size, sz / split);
  s.rearrange(newTileSize);
}

void traceray_ladders(const Point3 &from, const Point3 &to, const GameObjInstCB &game_obj_inst_cb)
{
  const GameObjects *game_objects = ECS_GET_SINGLETON_COMPONENT(GameObjects, game_objects);
  if (!game_objects || !game_objects->ladders)
    return;

  bbox3f traceBox;
  traceBox.bmin = traceBox.bmax = v_ldu(&from.x);
  v_bbox3_add_pt(traceBox, v_ldu(&to.x));
  v_bbox3_extend(traceBox, v_splats(0.3f));
  Point3 dir = to - from;
  vec3f t = v_splats(length(dir));
  dir *= safeinv(v_extract_x(t));
  game_objects->ladders->boxCull</*use_flags*/ false, /*use_pools*/ true>(traceBox, 0, 0, [&](scene::node_index, mat44f_cref m) {
    // we can't use orthonormalized_inverse, as matrix is not normalized (although it is ortho).
    // if we would store additional data, that would be quite easy
    mat44f im;
    v_mat44_inverse43(im, m);
    vec3f _from = v_mat44_mul_vec3p(im, v_ldu(&from.x));
    vec3f _dir = v_mat44_mul_vec3v(im, v_ldu(&dir.x));
    bbox3f box;
    v_bbox3_init_ident(box);
    alignas(16) TMatrix tmres;
    if (v_ray_box_intersection(_from, _dir, t /*rw*/, box))
    {
      v_mat_43ca_from_mat44(tmres.m[0], m);
      game_obj_inst_cb(tmres, /*numSteps*/ scene::get_node_flags(m));
    }
  });
}

static void ladder_optimize_es_event_handler(const EventGameObjectsOptimize &, GameObjects &game_objects)
{
  if (!game_objects.ladders || (game_objects.ladders->getNodesAliveCount() < 8 && game_objects.ladders->getTileSize() > 0.f))
    return;
  auto_rearrange_game_object_scene(*game_objects.ladders);
}
