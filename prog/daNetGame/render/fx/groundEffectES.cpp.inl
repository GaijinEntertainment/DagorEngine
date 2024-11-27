// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>

#include <landMesh/biomeQuery.h>
#include <math/integer/dag_IBBox2.h>
#include <landMesh/lmeshManager.h>
#include <dag_noise/dag_uint_noise.h>

#include <render/toroidal_update.h>
#include "render/fx/fx.h"
#include "render/fx/effectManager.h"
#include "camera/sceneCam.h"
#include "effectEntity.h"

namespace dacoll
{
extern LandMeshManager *get_lmesh();
}

#define MOD(a, b) (((a) % (b) + (b)) % (b)) // Euclidean mod (0 <= r < b for all a).

struct GroundEffectTransform
{
  Point3 offset;
  Point3 rot;
  Point3 scale;
  __forceinline bool operator==(const GroundEffectTransform &tm) const
  {
    return offset == tm.offset && rot == tm.rot && scale == tm.scale;
  }
  __forceinline bool operator!=(const GroundEffectTransform &tm) const { return !operator==(tm); }
};

__forceinline float unoise2D(int x, int y, uint32_t seed, float min, float max)
{
  return min + double(uint_noise2D(x, y, seed)) / double(UINT_MAX) * (max - min);
}

template <typename Callable>
inline void ground_fx_stop_ecs_query(ecs::EntityId, Callable c);

class GroundEffectManager
{
  struct GroundEffectParams
  {
    Point2 gridWorldOrigin = {0, 0};
    float gridCellSize = 1.0f;
    float visRadius = 0.0f;
    float randomOffsetScale = 0.0f;
    float biomeWeightForActiveThr = 0.25f;
    float fxRadius = 1.0f;
    GroundEffectTransform tmParams;
    GroundEffectTransform randTmParams;
  };

  struct GroundEffectQuery
  {
    int queryId;
    int cellX;
    int cellY;
  };

public:
  GroundEffectManager() = default;
  GroundEffectManager(GroundEffectManager &&) = default;

  ~GroundEffectManager() { killEffects(0, fxPool.size()); }

  bool update(const Point3 &camPos)
  {
    if (DAGOR_LIKELY(biomeGroupId >= 0))
      ; // We good
    else if (biome_query::get_num_biome_groups())
    {
      biomeGroupId = biome_query::get_biome_group_id(biomeGroupName.c_str());
      if (biomeGroupId < 0)
      {
        logerr("Can't create ground effect, invalid biome group name: %s", biomeGroupName.c_str());
        return false;
      }
    }
    else if (biomeIds.empty())
      return true; // In this case we don't use biome ids => wait until biome groups will be initialized.

    ToroidalGatherCallback::RegionTab regions;
    ToroidalGatherCallback cb(regions);

    IPoint2 newAreaOrigin = calcAreaOriginForCam(camPos);

    // In order to guarante that toroidalHelper.curOrigin will move to newAreaOrigin
    // we should call `toroidal_update` twice. For more: toroidal_update.h
    toroidal_update(newAreaOrigin, toroidalHelper, 100000, cb);
    toroidal_update(newAreaOrigin, toroidalHelper, 100000, cb);

    for (int i = 0; i < regions.size(); ++i)
    {
      const IPoint2 &wb = regions[i].wd;
      const IPoint2 &texelsFrom = regions[i].texelsFrom;
      for (int x = texelsFrom.x; x < texelsFrom.x + wb.x; ++x)
        for (int y = texelsFrom.y; y < texelsFrom.y + wb.y; ++y)
          updateCell(x, y);
    }

    processQueryQueue();
    return true;
  }

  void setParameters(const char *fx_name,
    const char *biome_group_name,
    dag::Vector<int> &&biome_ids,
    float grid_cell_size,
    Point2 grid_world_origin,
    float vis_radius,
    float biome_weight_for_active_thr,
    float fx_radius,
    const GroundEffectTransform &tm_params,
    const GroundEffectTransform &rand_tm_params,
    ecs::EntityManager &manager,
    ecs::EntityId eid)
  {
    bool needUpdateAllCells = fx_name != fxType || tm_params != params.tmParams || rand_tm_params != params.randTmParams ||
                              biome_group_name != biomeGroupName || biome_ids != biomeIds || grid_cell_size != params.gridCellSize ||
                              grid_world_origin != params.gridWorldOrigin || vis_radius != params.visRadius ||
                              biome_weight_for_active_thr != params.biomeWeightForActiveThr || fx_radius != params.fxRadius;

    if (!needUpdateAllCells)
      return;

    fxType = fx_name;
    fxHash = ECS_HASH_SLOW(fx_name).hash;
    biomeGroupName = biome_group_name;
    biomeIds = biome_ids;
    params.gridCellSize = grid_cell_size;
    params.gridWorldOrigin = grid_world_origin;
    params.visRadius = vis_radius;
    params.biomeWeightForActiveThr = biome_weight_for_active_thr;
    params.fxRadius = fx_radius;
    params.tmParams = tm_params;
    params.randTmParams = rand_tm_params;
    if (params.fxRadius <= 0.0f)
      params.fxRadius = params.gridCellSize * 0.5f;

    int fxAreaSize = calcAreaSize(vis_radius, grid_cell_size);
    int newFxPoolSize = fxAreaSize * fxAreaSize;
    killEffects(newFxPoolSize, fxPool.size());
    fxPool.resize(newFxPoolSize, ecs::INVALID_ENTITY_ID);

    toroidalHelper.texSize = fxAreaSize;
    toroidalHelper.curOrigin = IPoint2{-100000, -100000};
    toroidalHelper.mainOrigin = IPoint2{0, 0};

    const bool useBiomes = (biome_group_name && *biome_group_name != '\0') || biome_ids.size() > 0;
    if (!useBiomes)
      logerr("ground_effect__biome_group_name or ground_effect__biome_ids "
             "should be set for template ground_effect\n"
             "ground_effect__fx_name='%s' for %@<%@>",
        fx_name, (ecs::entity_id_t)eid, manager.getEntityTemplateName(eid));
  }

  bool isValid() { return !fxType.empty(); }

protected:
  void killEffects(int fromId, int toId)
  {
    for (int fxId = fromId; fxId < toId; ++fxId)
      g_entity_mgr->destroyEntity(fxPool[fxId]);
  }

  void updateCell(int x, int y)
  {
    Point3 fxPos;
    if (!getPosForCell(x, y, fxPos))
      return;
    GroundEffectQuery query;
    query.cellX = x;
    query.cellY = y;
    query.queryId = biome_query::query(fxPos, params.fxRadius);
    queryQueue.push_back(query);
  }

  void processQueryQueue()
  {
    auto newEnd = eastl::remove_if(queryQueue.begin(), queryQueue.end(), [&](GroundEffectQuery &query) {
      // There is a limit for queries at one frame,
      // so some queries could be not started.
      if (query.queryId < 0)
      {
        Point3 fxPos;
        if (!getPosForCell(query.cellX, query.cellY, fxPos))
          return true;
        query.queryId = biome_query::query(fxPos, params.fxRadius);
        return false;
      }
      BiomeQueryResult qResult;
      GpuReadbackResultState state = biome_query::get_query_result(query.queryId, qResult);
      if (state == GpuReadbackResultState::SUCCEEDED)
      {
        float weight = 0.0f;
        bool firstGroupMatch = qResult.mostFrequentBiomeGroupIndex == biomeGroupId;
        bool secondGroupMatch = qResult.secondMostFrequentBiomeGroupIndex == biomeGroupId;
        for (int biomeId : biomeIds)
        {
          firstGroupMatch |= qResult.mostFrequentBiomeGroupIndex == biomeId;
          secondGroupMatch |= qResult.secondMostFrequentBiomeGroupIndex == biomeId;
        }
        if (firstGroupMatch)
          weight += qResult.mostFrequentBiomeGroupWeight;
        if (secondGroupMatch)
          weight += qResult.secondMostFrequentBiomeGroupWeight;

        updateCellProcessed(query.cellX, query.cellY, weight > params.biomeWeightForActiveThr, weight);
      }
      else if (state != GpuReadbackResultState::IN_PROGRESS)
      {
        updateCellProcessed(query.cellX, query.cellY, false, 0.0f);
      }
      return state != GpuReadbackResultState::IN_PROGRESS;
    });
    queryQueue.resize(newEnd - queryQueue.begin());
  }

  void updateCellProcessed(int x, int y, bool active, float spawnRate)
  {
    unsigned id = getCellId(x, y);
    ground_fx_stop_ecs_query(fxPool[id], [](TheEffect &effect) {
      for (auto &fx : effect.getEffects())
        acesfx::kill_effect(fx.fx);
    });
    g_entity_mgr->destroyEntity(fxPool[id]);
    if (active)
    {
      ecs::ComponentsInitializer init;
      init[ECS_HASH("transform")] = getEmitterTm(x, y);
      init[ECS_HASH("effect__spawnRate")] = spawnRate;
      fxPool[id] = g_entity_mgr->createEntityAsync(fxType.c_str(), eastl::move(init));
    }
  }

  unsigned getCellId(int x, int y)
  {
    unsigned locX = MOD(x, toroidalHelper.texSize);
    unsigned locY = MOD(y, toroidalHelper.texSize);
    return locX * toroidalHelper.texSize + locY;
  }

  bool getPosForCell(int x, int y, Point3 &outPos)
  {
    Point2 flatPos = Point2((float(x) + 0.5f) * params.gridCellSize + params.gridWorldOrigin.x,
      (float(y) + 0.5f) * params.gridCellSize + params.gridWorldOrigin.y);
    Point3 offset = getOffset(x, y);
    flatPos += Point2(offset.x, offset.z);
    float height;
    LandMeshManager *lmeshMgr = dacoll::get_lmesh();
    if (!lmeshMgr || !lmeshMgr->getHeight(flatPos, height, nullptr))
      return false;
    outPos = Point3(flatPos.x, height + offset.y, flatPos.y);
    return true;
  }

  int calcAreaSize(float visRadius, float cellSize) { return int(visRadius * 2.0f / cellSize) + 1; }

  IPoint2 calcAreaOriginForCam(const Point3 &pos)
  {
    Point2 localFlatCenter = Point2(pos.x, pos.z) - params.gridWorldOrigin;
    return ipoint2(floor(localFlatCenter / params.gridCellSize));
  }

  Point3 getOffset(int x, int y)
  {
    float offsetX = params.tmParams.offset.x + params.randTmParams.offset.x * unoise2D(x, y, _MAKE4C('OFSX') + fxHash, -1.0f, 1.0f);
    float offsetY = params.tmParams.offset.y + params.randTmParams.offset.y * unoise2D(x, y, _MAKE4C('OFSY') + fxHash, -1.0f, 1.0f);
    float offsetZ = params.tmParams.offset.z + params.randTmParams.offset.z * unoise2D(x, y, _MAKE4C('OFSZ') + fxHash, -1.0f, 1.0f);
    return Point3(offsetX, offsetY, offsetZ);
  }

  TMatrix getEmitterTm(int x, int y)
  {
    float rotX = params.tmParams.rot.x + params.randTmParams.rot.x * unoise2D(x, y, _MAKE4C('ROTX') + fxHash, -1.0f, 1.0f);
    float rotY = params.tmParams.rot.y + params.randTmParams.rot.y * unoise2D(x, y, _MAKE4C('ROTY') + fxHash, -1.0f, 1.0f);
    float rotZ = params.tmParams.rot.z + params.randTmParams.rot.z * unoise2D(x, y, _MAKE4C('ROTZ') + fxHash, -1.0f, 1.0f);
    float scaleX = params.tmParams.scale.x + params.randTmParams.scale.x * unoise2D(x, y, _MAKE4C('SCLX') + fxHash, -1.0f, 1.0f);
    float scaleY = params.tmParams.scale.y + params.randTmParams.scale.y * unoise2D(x, y, _MAKE4C('SCLY') + fxHash, -1.0f, 1.0f);
    float scaleZ = params.tmParams.scale.z + params.randTmParams.scale.z * unoise2D(x, y, _MAKE4C('SCLZ') + fxHash, -1.0f, 1.0f);
    TMatrix result = rotxTM(rotX) * rotyTM(rotY) * rotzTM(rotZ);
    result.setcol(0, result.getcol(0) * scaleX);
    result.setcol(1, result.getcol(1) * scaleY);
    result.setcol(2, result.getcol(2) * scaleZ);
    Point3 pos;
    if (!getPosForCell(x, y, pos))
      return result;
    result.setcol(3, pos);
    return result;
  }

  ToroidalHelper toroidalHelper;

  eastl::string fxType;
  int fxHash = -1;
  eastl::vector<ecs::EntityId> fxPool;

  GroundEffectParams params;

  eastl::string biomeGroupName; // Need to store since biomes aren't initialized when the entity is created.
  int biomeGroupId = -1;
  dag::Vector<GroundEffectQuery> queryQueue;
  dag::Vector<int> biomeIds;
};

ECS_DECLARE_RELOCATABLE_TYPE(GroundEffectManager);
ECS_REGISTER_RELOCATABLE_TYPE(GroundEffectManager, nullptr);
ECS_AUTO_REGISTER_COMPONENT(GroundEffectManager, "ground_effect__manager", nullptr, 0);


template <typename Callable>
void get_camera_position_ecs_query(ecs::EntityId, Callable);

ECS_AFTER(camera_animator_update_es)
ECS_TAG(render)
static void update_ground_effects_es(const ecs::UpdateStageInfoAct &, ecs::EntityId eid, GroundEffectManager &ground_effect__manager)
{
  Point3 camPos(0, 0, 0);
  get_camera_position_ecs_query(get_cur_cam_entity(), [&](const TMatrix &transform) { camPos = transform.getcol(3); });
  if (!ground_effect__manager.isValid() || !ground_effect__manager.update(camPos))
    g_entity_mgr->destroyEntity(eid);
}

ECS_TAG(render)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear)
static void update_ground_effect_params_es_event_handler(const ecs::Event &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  GroundEffectManager &ground_effect__manager,
  const ecs::string &ground_effect__fx_name,
  const ecs::string &ground_effect__biome_group_name,
  const ecs::Array &ground_effect__biome_ids,
  float ground_effect__grid_cell_size,
  Point2 ground_effect__grid_world_origin,
  float ground_effect__vis_radius,
  float ground_effect__biome_weight_for_active_thr,
  float ground_effect__fx_radius,
  Point2 ground_effect__rot_x,
  Point2 ground_effect__rot_y,
  Point2 ground_effect__rot_z,
  Point2 ground_effect__offset_x,
  Point2 ground_effect__offset_y,
  Point2 ground_effect__offset_z,
  Point2 ground_effect__scale_x,
  Point2 ground_effect__scale_y,
  Point2 ground_effect__scale_z)
{
  dag::Vector<int> biomeIds;
  if (uint32_t bsz = ground_effect__biome_ids.size())
  {
    biomeIds.resize_noinit(bsz);
    for (int i = 0; i < bsz; ++i)
      biomeIds[i] = ground_effect__biome_ids[i].get<int>();
  }
  GroundEffectTransform tmParams[2];
  for (int i = 0; i < 2; ++i)
  {
    tmParams[i].offset = Point3(ground_effect__offset_x[i], ground_effect__offset_y[i], ground_effect__offset_z[i]);
    tmParams[i].rot = Point3(DegToRad(ground_effect__rot_x[i]), DegToRad(ground_effect__rot_y[i]), DegToRad(ground_effect__rot_z[i]));
    tmParams[i].scale = Point3(ground_effect__scale_x[i], ground_effect__scale_y[i], ground_effect__scale_z[i]);
  };
  ground_effect__manager.setParameters(ground_effect__fx_name.c_str(), ground_effect__biome_group_name.c_str(), eastl::move(biomeIds),
    ground_effect__grid_cell_size, ground_effect__grid_world_origin, ground_effect__vis_radius,
    ground_effect__biome_weight_for_active_thr, ground_effect__fx_radius, tmParams[0], tmParams[1], manager, eid);
}
