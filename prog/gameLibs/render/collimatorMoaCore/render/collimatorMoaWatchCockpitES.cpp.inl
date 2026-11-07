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
      const ecs::string &animchar__res, const ecs::string *gunmod__lensNode = nullptr) {
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

      if (resElemId == -1 && gunmod__lensNode)
      {
        const int lensNodeId = sceneRes->getNames().node.getNameId(gunmod__lensNode->c_str());
        for (auto [rigidNo, rigid] : enumerate(rigids))
        {
          if (rigid.nodeId != lensNodeId)
            continue;

          const int relemsCount = rigid.mesh->getMesh()->getAllElems().size();
          if (relemsCount == 1)
          {
            resRigidNo = rigidNo;
            resElemId = 0;
          }
          else
          {
            // clang-format off
            logerr("[CollimatorMOA] lens node '%s' (gunmod__lensNode) in the asset %s has %d render elements, only 1 is supported."
              "\nWhat to check:"
              "\n1.Artists: the lens node must have exactly ONE material"
              "\n  and its child nodes must not be collapsed into it (mark them animated_node:b=yes)."
              "\n2.Or apply the %s material to the lens.",
              gunmod__lensNode->c_str(), animchar__res, relemsCount, DYNAMIC_COLLIMATOR_MOA_SHADER_NAME);
            // clang-format on
          }
          break;
        }
      }

      lensRenderInfo.relemId = resElemId;
      lensRenderInfo.rigidNo = resRigidNo;
    });

  return lensRenderInfo;
}

static dag::Index16 find_collimator_moa_node(const GeomNodeTree &node_tree, const ecs::string &node_name)
{
  if (node_name.empty())
    return {};
  return node_tree.findNodeIndex(node_name.c_str());
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE_NOT(ecs::Tag collimatorMoaUseLensOrientation)
static void collimator_moa_cache_lens_axes_es(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid,
  const AnimV20::AnimcharBaseComponent &animchar, const ecs::string &gunmod__lensNode, const ecs::string &gunmod__camNode,
  Point3 &gunmod__collimator_moa_local_forward, Point3 &gunmod__collimator_moa_local_right)
{
  const GeomNodeTree &nodeTree = animchar.getNodeTree();
  const dag::Index16 lensNode = find_collimator_moa_node(nodeTree, gunmod__lensNode);
  const dag::Index16 camNode = find_collimator_moa_node(nodeTree, gunmod__camNode);
  if (!lensNode || !camNode)
  {
    logerr(
      "[CollimatorMOA] <%s>: gunmod__lensNode '%s' or gunmod__camNode '%s' is not in the node tree, the lens axes are not computed",
      manager.getEntityTemplateName(eid), gunmod__lensNode.c_str(), gunmod__camNode.c_str());
    return;
  }

  TMatrix lensWtm, camWtm;
  nodeTree.getNodeWtmRelScalar(lensNode, lensWtm);
  nodeTree.getNodeWtmRelScalar(camNode, camWtm);
  const TMatrix lensItm = inverse(lensWtm);
  const Point3 localForward = normalize(lensItm % (lensWtm.getcol(3) - camWtm.getcol(3)));
  const Point3 lensLocalRight(-1.f, 0.f, 0.f);

  gunmod__collimator_moa_local_forward = localForward;
  gunmod__collimator_moa_local_right = normalize(lensLocalRight - localForward * (lensLocalRight * localForward));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag collimatorMoaUseLensOrientation)
static void collimator_moa_cache_lens_axes_legacy_es(const ecs::Event &, Point3 &gunmod__collimator_moa_local_forward,
  Point3 &gunmod__collimator_moa_local_right)
{
  gunmod__collimator_moa_local_forward = Point3(0.f, -1.f, 0.f);
  gunmod__collimator_moa_local_right = Point3(-1.f, 0.f, 0.f);
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
