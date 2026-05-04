// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <ecs/render/updateStageRender.h>
#include <generic/dag_enumerate.h>

#include <shaders/dag_dynSceneRes.h>


template <typename Callable>
inline void gather_collimator_moa_lens_info_ecs_query(ecs::EntityManager &manager, ecs::EntityId, Callable c);
namespace
{
struct LensRenderInfo
{
  int relemId = -1;
  int rigidNo = -1;

  operator bool() const { return relemId != -1 && rigidNo != -1; }
};

const char *DYNAMIC_COLLIMATOR_MOA_SHADER_NAME = "dynamic_collimator_moa";
} // namespace

static LensRenderInfo get_lens_render_info(ecs::EntityManager &manager, ecs::EntityId gun_mod_eid)
{
  LensRenderInfo lensRenderInfo;

  gather_collimator_moa_lens_info_ecs_query(manager, gun_mod_eid,
    [&lensRenderInfo](ECS_REQUIRE(ecs::Tag gunScope) const AnimV20::AnimcharRendComponent &animchar_render,
      const ecs::string &animchar__res) {
      const DynamicRenderableSceneInstance *sceneInstance = animchar_render.getSceneInstance();
      if (!sceneInstance || !sceneInstance->getLodsResource())
        return;

      const DynamicRenderableSceneResource *sceneRes = sceneInstance->getLodsResource()->lods[0].scene;
      if (!sceneRes)
        return;

      int resRigidNo = -1;
      int resElemId = -1;

      dag::ConstSpan<DynamicRenderableSceneResource::RigidObject> rigids = sceneRes->getRigidsConst();
      for (auto [rigidNo, rigid] : enumerate(rigids))
      {
        dag::ConstSpan<ShaderMesh::RElem> relems = rigid.mesh->getMesh()->getAllElems();
        for (auto [relemId, relem] : enumerate(relems))
        {
          if (strcmp(relem.mat->getShaderClassName(), DYNAMIC_COLLIMATOR_MOA_SHADER_NAME) == 0)
          {
            if (resElemId == -1)
            {
              resRigidNo = rigidNo;
              resElemId = relemId;
            }
            else
            {
              const char *selectedNodeName = sceneRes->getNames().node.getNameSlow(rigids[resRigidNo].nodeId);
              const char *excessiveNodeName = sceneRes->getNames().node.getNameSlow(rigid.nodeId);

              // clang-format off
              logerr("[CollimatorMOA] %s material in human cockpit is not supported for multiple nodes."
                "\nWhat to check:"
                "\n1.Artists: check the asset %s. The code encountered two different nodes with %s shader."
                "\n  First Node: '%s',"
                "\n  Second Node: '%s'."
                "\n  >>The shader must be used only for ONE node<<"
                "\n2.Programmers: if the asset is fine, this case must be investigated."
                "\n  Somehow we've got collision in ShaderMesh::RElem."
                "\n   First [RigidId:%d ElemId:%d],"
                "\n   Second [RigidId:%d ElemId:%d]",
                DYNAMIC_COLLIMATOR_MOA_SHADER_NAME,
                animchar__res, DYNAMIC_COLLIMATOR_MOA_SHADER_NAME,
                selectedNodeName,
                excessiveNodeName,
                resRigidNo, resElemId,
                rigidNo, relemId
              );
              // clang-format on
            }
          }
        }
      }

      lensRenderInfo.relemId = resElemId;
      lensRenderInfo.rigidNo = resRigidNo;
    });

  return lensRenderInfo;
}

ECS_TAG(render)
static void collimator_moa_update_lens_eids_es(const UpdateStageInfoBeforeRender &, ecs::EntityManager &manager,
  const ecs::EntityId collimator_moa_render__gun_mod_eid, int &collimator_moa_render__rigid_id, int &collimator_moa_render__relem_id,
  bool &collimator_moa_render__relem_update_required)
{
  if (!collimator_moa_render__relem_update_required)
    return;

  collimator_moa_render__rigid_id = -1;
  collimator_moa_render__relem_id = -1;

  if (collimator_moa_render__gun_mod_eid)
  {
    const LensRenderInfo rInfo = get_lens_render_info(manager, collimator_moa_render__gun_mod_eid);
    if (rInfo)
    {
      collimator_moa_render__rigid_id = rInfo.rigidNo;
      collimator_moa_render__relem_id = rInfo.relemId;
    }
  }

  collimator_moa_render__relem_update_required = false;
}
