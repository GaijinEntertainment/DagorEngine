// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_lockTexture.h>
#include <camera/sceneCam.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <ecs/delayedAct/actInThread.h>
#include <ecs/render/resPtr.h>
#include <ecs/render/updateStageRender.h>
#include <gamePhys/collision/collisionLib.h>
#include <physMap/physMap.h>
#include <render/renderEvent.h>
#include <render/toroidal_update.h>
#include <util/dag_threadPool.h>
#include <util/dag_treeBitmap.h>

#define PHYSMAP_PATCH_VARS VAR(physmap_patch_origin_invscale_size)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
PHYSMAP_PATCH_VARS
#undef VAR

static ToroidalHelper toroidal_helper;

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(ecs::Tag create_physmap_patch_data)
static void exec_physmap_patch_data_creation_request_es_event_handler(const ecs::UpdateStageInfoAct &, const ecs::EntityId &eid)
{
  if (!g_entity_mgr->getTemplateDB().getTemplateByName("physmap_patch_data"))
  {
    g_entity_mgr->destroyEntity(eid);
    return;
  }

  const PhysMap *physMap = dacoll::get_lmesh_phys_map();
  if (physMap)
  {
    g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("physmap_patch_data"));
    g_entity_mgr->destroyEntity(eid);
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_physmap_patch_es_event_handler(
  const ecs::Event &, UniqueTexHolder &physmap_patch_tex, int physmap_patch_tex_size, float &physmap_patch_invscale)
{
  const PhysMap *physMap = dacoll::get_lmesh_phys_map();

  physmap_patch_tex.close();
  physmap_patch_tex = dag::create_tex(nullptr, physmap_patch_tex_size, physmap_patch_tex_size, TEXFMT_R8UI, 1, "physmap_patch_tex");
  physmap_patch_tex->disableSampler();
  physmap_patch_invscale = physMap->invScale;

  toroidal_helper.texSize = physmap_patch_tex_size;
  toroidal_helper.curOrigin = IPoint2{-100000, -100000};
  toroidal_helper.mainOrigin = IPoint2::ZERO;
}

static struct GatherPhysmapPatchUpdatedRegionsJob final : public cpujobs::IJob
{
  uint32_t tpqpos = 0;
  int patchTexSize;
  Point2 camPosXZ;
  const PhysMap *physMap;
  ToroidalGatherCallback::RegionTab regionsToUpdate;
  Tab<Tab<int>> regionsTexelData;
  Point2 patchOrigin;
  bool patchNeedsUpdate = false;

  void doJob() override
  {
    TIME_PROFILE(gather_physmap_patch_updated_regions);

    IPoint2 newOriginInPhysmapCoords = ipoint2(floor((camPosXZ - physMap->worldOffset) * physMap->invScale));

    regionsToUpdate.clear();
    ToroidalGatherCallback cb(regionsToUpdate);
    toroidal_update(newOriginInPhysmapCoords, toroidal_helper, 100000, cb);

    patchOrigin = (floor(camPosXZ * physMap->invScale) - patchTexSize / 2 * Point2::ONE -
                    (newOriginInPhysmapCoords - toroidal_helper.mainOrigin)) *
                  physMap->scale;

    regionsTexelData.clear();
    regionsTexelData.resize(regionsToUpdate.size());
    for (int regionNo = 0; regionNo < regionsToUpdate.size(); ++regionNo)
    {
      const IPoint2 &wd = regionsToUpdate[regionNo].wd;
      const IPoint2 &texelsFrom = regionsToUpdate[regionNo].texelsFrom;
      if (texelsFrom.x < 0 || texelsFrom.y < 0 || texelsFrom.x + wd.x >= physMap->size || texelsFrom.y + wd.y >= physMap->size)
        return;
      regionsTexelData[regionNo].resize(wd.x * wd.y);
      for (int j = 0; j < wd.y; ++j)
        for (int i = 0; i < wd.x; ++i)
          regionsTexelData[regionNo][j * wd.x + i] =
            physMap->materials[physMap->parent->get(texelsFrom.x + i, texelsFrom.y + j, physMap->size)];
    }
    patchNeedsUpdate = true;
  };
} gather_physmap_patch_updated_regions_job;

ECS_TAG(render)
static void gather_physmap_patch_updated_regions_es(const ParallelUpdateFrameDelayed &,
  int physmap_patch_tex_size,
  float physmap_patch_update_distance_squared,
  Point2 &physmap_patch_last_update_pos)
{
  vec3f camPosXZ = v_perm_xzxz(v_ldu(&(get_cam_itm().getcol(3)).x));
  vec3f lastUpdatePos = v_ldu_half(&physmap_patch_last_update_pos.x);
  if (v_extract_x(v_length2_sq_x(v_sub(lastUpdatePos, camPosXZ))) < physmap_patch_update_distance_squared)
    return;

  const PhysMap *physMap = dacoll::get_lmesh_phys_map();
  if (!physMap)
    return;

  gather_physmap_patch_updated_regions_job.patchTexSize = physmap_patch_tex_size;
  v_stu_half(&physmap_patch_last_update_pos, camPosXZ);
  v_stu_half(&gather_physmap_patch_updated_regions_job.camPosXZ, camPosXZ);
  gather_physmap_patch_updated_regions_job.physMap = physMap;

  using namespace threadpool;
  auto addf = get_current_worker_id() < 0 ? AddFlags::WakeOnAdd : AddFlags::None;
  add(&gather_physmap_patch_updated_regions_job, PRIO_LOW, gather_physmap_patch_updated_regions_job.tpqpos, addf);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void update_physmap_patch_tex_es(
  const UpdateStageInfoBeforeRender &, UniqueTexHolder &physmap_patch_tex, int physmap_patch_tex_size, float physmap_patch_invscale)
{
  if (!interlocked_acquire_load(gather_physmap_patch_updated_regions_job.done)) // Wait job from prev frame
  {
    TIME_PROFILE(wait_gather_physmap_patch_updated_regions_job_done);
    auto &job = gather_physmap_patch_updated_regions_job;
    threadpool::barrier_active_wait_for_job(&job, threadpool::PRIO_LOW, job.tpqpos);
  }

  if (!gather_physmap_patch_updated_regions_job.patchNeedsUpdate)
    return;

  TIME_D3D_PROFILE(update_physmap_patch_tex);
  if (auto data = lock_texture<uint8_t>(physmap_patch_tex.getTex2D(), 0, TEXLOCK_WRITE))
  {
    const Tab<Tab<int>> &regionsTexelData = gather_physmap_patch_updated_regions_job.regionsTexelData;
    for (int regionNo = 0; regionNo < gather_physmap_patch_updated_regions_job.regionsToUpdate.size(); ++regionNo)
    {
      const IPoint2 &lt = gather_physmap_patch_updated_regions_job.regionsToUpdate[regionNo].lt;
      const IPoint2 &wd = gather_physmap_patch_updated_regions_job.regionsToUpdate[regionNo].wd;
      for (int j = 0; j < wd.y; ++j)
        for (int i = 0; i < wd.x; ++i)
          data.at(lt.x + i, lt.y + j) = regionsTexelData[regionNo][j * wd.x + i];
    }
  }
  ShaderGlobal::set_color4(physmap_patch_origin_invscale_sizeVarId, gather_physmap_patch_updated_regions_job.patchOrigin,
    physmap_patch_invscale, physmap_patch_tex_size);
  gather_physmap_patch_updated_regions_job.patchNeedsUpdate = false;
}
