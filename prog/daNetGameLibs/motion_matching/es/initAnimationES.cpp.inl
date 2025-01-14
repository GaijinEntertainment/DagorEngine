// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_animPostBlendCtrl.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <ecs/anim/anim.h>
#include <startup/dag_globalSettings.h>
#include <animation/animation_data_base.h>
#include <animation/motion_matching_controller.h>
#include <animation/motion_matching_animgraph_nodes.h>
#include <ioSys/dag_dataBlock.h>

ECS_BROADCAST_EVENT_TYPE(InvalidateAnimationDataBase);
ECS_REGISTER_EVENT(InvalidateAnimationDataBase);
ECS_UNICAST_EVENT_TYPE(AnimationDataBaseAssigned);
ECS_REGISTER_EVENT(AnimationDataBaseAssigned);

static void load_tags(OAHashNameMap<false> &tags_map, const ecs::StringList &available_tags)
{
  tags_map.clear();
  for (const ecs::string &tag : available_tags)
  {
    if (tags_map.nameCount() >= Tags::kSize)
    {
      logerr("MM: tags limit was reached");
      return;
    }
    tags_map.addNameId(tag.c_str());
  }
}

static void resolve_tags_for_presets(const ecs::StringList &preset_names, AnimationDataBase &dataBase)
{
  G_ASSERT(preset_names.size() == dataBase.tagsPresets.size());
  for (int i = 0; i < preset_names.size(); ++i)
  {
    // todo: use separate string for tags instead of preset name, this way we can specify any combination of tags per preset.
    dataBase.tagsPresets[i].requiredTagIdx = dataBase.tags.getNameId(preset_names[i].c_str());
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_mm_limit_per_frame_es(const ecs::Event &, int &main_database__perFrameLimit)
{
  main_database__perFrameLimit = dgs_get_settings()->getInt("motionMatchingPerFrameLimit", -1);
}

ECS_REGISTER_BOXED_TYPE(MotionMatchingController, nullptr);

AnimationRootMotionConfig load_root_motion_config(const ecs::string &root_node,
  const ecs::StringList &direction_nodes,
  const ecs::FloatList &direction_weights,
  const ecs::StringList &center_of_mass_nodes,
  const ecs::Point4List &center_of_mass_params);

void load_animations(AnimationDataBase &dataBase,
  const AnimationRootMotionConfig &rootMotionConfig,
  const DataBlock &node_masks,
  const ecs::StringList &clips_path);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag deadEntity)
static void disable_motion_matching_es(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar)
{
  animchar.setMotionMatchingController(nullptr);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(MotionMatchingController motion_matching__controller)
static void on_motion_matching_controller_removed_es(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar)
{
  animchar.setMotionMatchingController(nullptr);
}

static void load_post_blend_controller_weight_overrides(
  AnimationDataBase &dataBase, const ecs::Object &pbc_overrides, const AnimV20::AnimationGraph *anim_graph)
{
  if (!anim_graph)
    return;
  dataBase.pbcWeightOverrides.clear();
  for (const auto &name_weight : pbc_overrides)
  {
    bool found = false;
    for (int pbcIdx = 0, pbcCnt = anim_graph->getPBCCount(); pbcIdx < pbcCnt; ++pbcIdx)
    {
      const char *pbcName = anim_graph->getPBCName(pbcIdx);
      if (pbcName && name_weight.first == pbcName)
      {
        found = true;
        dataBase.pbcWeightOverrides.emplace_back(pbcIdx, name_weight.second.get<float>());
        break;
      }
    }
    if (!found)
      logerr("Motion Matching: failed to find '%s' post blend controller", name_weight.first);
  }
}

static void load_foot_locker_nodes(AnimationDataBase &dataBase, const ecs::StringList &node_names)
{
  dataBase.footLockerNodes.clear();
  const GeomNodeTree &tree = *dataBase.getReferenceSkeleton();
  for (const ecs::string &node_name : node_names)
  {
    dag::Index16 nodeId = tree.findNodeIndex(node_name.c_str());
    if (nodeId)
      dataBase.footLockerNodes.push_back(nodeId);
    else
      logerr("Motion Matching: can't find foot locker node '%s'", node_name);
  }
}

template <class T>
inline void get_data_base_ecs_query(T);

static bool animation_data_base_entity_is_creating = false;

ECS_TAG(render)
ECS_ON_EVENT(on_appear, InvalidateAnimationDataBase)
static void init_animations_es(const ecs::Event &,
  ecs::EntityId eid,
  AnimV20::AnimcharBaseComponent &animchar,
  MotionMatchingController &motion_matching__controller,
  bool &motion_matching__enabled,
  int &motion_matching__presetIdx,
  ecs::EntityId &motion_matching__dataBaseEid,
  const ecs::string &motion_matching__dataBaseTemplateName,
  ecs::IntList &motion_matching__goalNodesIdx,
  FrameFeatures &motion_matching__goalFeature)
{
#if DAGOR_DBGLEVEL > 0
  if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("disable_motion_matching", false))
    return;
#endif
  const char *dbTemplateName = motion_matching__dataBaseTemplateName.c_str();
  if (!g_entity_mgr->getSingletonEntity(ECS_HASH_SLOW(dbTemplateName)) && !animation_data_base_entity_is_creating)
  {
    animation_data_base_entity_is_creating = true;
    g_entity_mgr->createEntityAsync(dbTemplateName, {}, {}, [](ecs::EntityId) {
      g_entity_mgr->broadcastEvent(InvalidateAnimationDataBase());
      animation_data_base_entity_is_creating = false;
    });
  }

  get_data_base_ecs_query(
    [&](ecs::EntityId eid, bool &main_database__loaded, const ecs::StringList &data_bases_paths,
      const ecs::StringList &main_database__availableTags, const ecs::StringList &main_database__presetsTagsName,
      const ecs::string &main_database__root_node, const ecs::string &main_database__root_motion_a2d_node,
      const ecs::StringList &main_database__direction_nodes, const ecs::FloatList &main_database__direction_weights,
      const ecs::StringList &main_database__center_of_mass_nodes, const ecs::Point4List &main_database__center_of_mass_params,
      const ecs::string &main_database__nodeMasksPath, const ecs::Object &main_database__pbcWeightOverrides,
      const ecs::string &main_database__footLockerCtrlName, const ecs::StringList main_database__footLockerNodes,
      AnimationDataBase &dataBase) {
      // current data base entity can be with daeditor_selected subtemplate
      if (!find_sub_template_name(g_entity_mgr->getEntityTemplateName(eid), dbTemplateName))
        return;
      if (!main_database__loaded)
      {
        AnimationRootMotionConfig rootMotionConfig = load_root_motion_config(main_database__root_node, main_database__direction_nodes,
          main_database__direction_weights, main_database__center_of_mass_nodes, main_database__center_of_mass_params);
        DataBlock nodeMasks(main_database__nodeMasksPath.c_str());
        dataBase.rootMotionA2dNode = main_database__root_motion_a2d_node;
        dataBase.setReferenceSkeleton(animchar.getOriginalNodeTree());
        load_foot_locker_nodes(dataBase, main_database__footLockerNodes);
        load_tags(dataBase.tags, main_database__availableTags);
        resolve_tags_for_presets(main_database__presetsTagsName, dataBase);
        load_motion_matching_animgraph_nodes(dataBase, animchar.getAnimGraph());
        load_animations(dataBase, rootMotionConfig, nodeMasks, data_bases_paths);
        load_post_blend_controller_weight_overrides(dataBase, main_database__pbcWeightOverrides, animchar.getAnimGraph());
        if (!main_database__footLockerCtrlName.empty())
        {
          String footLockerParamName(0, "$%s", main_database__footLockerCtrlName.c_str());
          dataBase.footLockerParamId =
            animchar.getAnimGraph()->getParamId(footLockerParamName, AnimV20::AnimCommonStateHolder::PT_InlinePtr);
          int footLockerNumLegs =
            animchar.getAnimState()->getInlinePtrMaxSz(dataBase.footLockerParamId) / sizeof(AnimV20::FootLockerIKCtrl::LegData);
          if (footLockerNumLegs != dataBase.footLockerNodes.size())
          {
            logerr("Motion Matching: FootLockerIKCtrl has different number of legs");
            dataBase.footLockerParamId = -1;
          }
        }
        main_database__loaded = true;
      }
      if (dataBase.getReferenceSkeleton() != animchar.getOriginalNodeTree())
      {
        logerr("Motion Matching: single animation database cannot be used with different skeletons");
        return;
      }
      if (dataBase.referenceAnimGraph != animchar.getAnimGraph())
      {
        logerr("Motion Matching: single animation database cannot be used with different AnimTrees/AnimGraphs");
        return;
      }
      motion_matching__dataBaseEid = eid;
      motion_matching__controller.setup(dataBase, animchar);
      motion_matching__controller.useTagsFromAnimGraph = dataBase.hasAnimGraphTags();
    });
  if (!motion_matching__controller.dataBase) // database is not ready, will try again on next InvalidateAnimationDataBase event
    return;
  motion_matching__enabled = true;
  const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
  const GeomNodeTree &tree = *animchar.getOriginalNodeTree();

  motion_matching__goalNodesIdx.clear();
  for (const eastl::string &nodeName : dataBase.nodesName)
    motion_matching__goalNodesIdx.push_back((int)tree.findNodeIndex(nodeName.c_str()));

  animchar.setMotionMatchingController(&motion_matching__controller);

  motion_matching__goalFeature.init(1, dataBase.nodeCount, dataBase.trajectorySize);

  int skeletonNodeCount = motion_matching__controller.nodeCount;
  if (motion_matching__controller.offset.position.empty()) // on_appear event, no need to reset bones on db invalidation
  {
    motion_matching__controller.offset.set_bone_count(skeletonNodeCount);
    motion_matching__controller.currentAnimation.set_bone_count(skeletonNodeCount);
    motion_matching__controller.resultAnimation.set_bone_count(skeletonNodeCount);
    motion_matching__controller.perNodeWeights.resize(skeletonNodeCount, 0.0f);
    for (int i = 0; i < skeletonNodeCount; ++i)
      motion_matching__controller.currentAnimation.position[i] = tree.getNodeTm(dag::Index16(i)).col3;
  }
  motion_matching__controller.currentClipInfo = {};
  motion_matching__presetIdx = -1;
  g_entity_mgr->sendEventImmediate(eid, AnimationDataBaseAssigned());
}
