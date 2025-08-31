// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define IMGUI_DEFINE_MATH_OPERATORS
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_geomTree.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_textMarks.h>
#include <ecs/anim/anim.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <animation/animation_data_base.h>
#include <animation/motion_matching_controller.h>
#include <animation/motion_matching.h>
#include <animation/feature_normalization.h>
#include <animation/motion_matching_metrics.h>
#include <animation/animation_sampling.h>
#include <util/dag_console.h>

template <class T>
inline void imgui_prepare_skeleton_visualization_ecs_query(ecs::EntityId, T);

static void get_current_mm_pose(GeomNodeTree &mm_animated_tree,
  eastl::bitvector<> &nodemask,
  const MotionMatchingController &controller,
  const AnimV20::AnimcharBaseComponent &animchar)
{
  const Tab<AnimV20::AnimMap> &animMap = animchar.getAnimMap();
  int animNodesCount = 0;
  for (const auto &map : animMap)
    animNodesCount = max(animNodesCount, map.animId + 1);

  AnimV20::AnimBlender::TlsContext tlsAnimCtx;
  dag::Vector<AnimV20::AnimBlender::WeightedNode> chPos(animNodesCount), chScl(animNodesCount);
  dag::Vector<AnimV20::AnimBlender::WeightedNode> chRot(animNodesCount);
  dag::Vector<AnimV20::AnimBlender::PrsResult> chPrs(animNodesCount);
  dag::Vector<AnimV20::AnimBlender::NodeWeight> wtPos(animNodesCount), wtRot(animNodesCount), wtScl(animNodesCount);
  tlsAnimCtx.chPos = chPos.data();
  tlsAnimCtx.chRot = chRot.data();
  tlsAnimCtx.chScl = chScl.data();
  tlsAnimCtx.chPrs = chPrs.data();
  tlsAnimCtx.wtPos = wtPos.data();
  tlsAnimCtx.wtRot = wtRot.data();
  tlsAnimCtx.wtScl = wtScl.data();

  controller.getPose(tlsAnimCtx, animMap);
  for (int i = 0, e = animMap.size(); i < e; i++)
  {
    vec3f pos, scl;
    quat4f rot;
    v_mat4_decompose(mm_animated_tree.getNodeTm(animMap[i].geomId), pos, rot, scl);
    int16_t animId = animMap[i].animId;
    uint16_t geomId = animMap[i].geomId.index();
    if (tlsAnimCtx.wtPos[animId].totalNum)
    {
      nodemask.set(geomId, true);
      pos = tlsAnimCtx.chPrs[animId].pos;
    }
    if (tlsAnimCtx.wtRot[animId].totalNum)
    {
      nodemask.set(geomId, true);
      rot = tlsAnimCtx.chPrs[animId].rot;
    }
    if (tlsAnimCtx.wtScl[animId].totalNum)
    {
      nodemask.set(geomId, true);
      scl = tlsAnimCtx.chPrs[animId].scl;
    }
    v_mat44_compose(mm_animated_tree.getNodeTm(animMap[i].geomId), pos, rot, scl);
  }
}

static void get_selected_anim_pose(GeomNodeTree &mm_animated_tree,
  eastl::bitvector<> &nodemask,
  int clip_idx,
  int frame_idx,
  const AnimV20::AnimcharBaseComponent &animchar,
  const MotionMatchingController &controller)
{
  float timeInSeconds = controller.getFrameTimeInSeconds(clip_idx, frame_idx, 0.f);
  const AnimationClip &clip = controller.dataBase->clips[clip_idx];
  NodeTSRFixedArray nodes(mm_animated_tree.nodeCount());
  set_identity(nodes, mm_animated_tree);
  sample_animation(timeInSeconds, clip, nodes);
  int rootId = controller.dataBase->rootNode.index();
  if (clip.inPlaceAnimation)
    nodes[0].changeBit = 0;
  else if (nodes[rootId].changeBit)
  {
    RootMotion rootTm = lerp_root_motion(clip, timeInSeconds);
    quat4f invRot = v_quat_conjugate(rootTm.rotation);
    nodes[rootId].translation = v_quat_mul_vec3(invRot, v_sub(nodes[rootId].translation, rootTm.translation));
    nodes[rootId].rotation = v_norm4(v_quat_mul_quat(invRot, nodes[rootId].rotation));
  }
  if (!controller.rootSynchronization)
  {
    mat44f animcharTm;
    animchar.getTm(animcharTm);
    quat4f animcharRotation = v_quat_from_mat43(animcharTm);
    quat4f invAnimcharRotation = v_quat_conjugate(animcharRotation);
    vec3f rootWorldPos = v_add(v_quat_mul_vec3(controller.rootRotation, nodes[rootId].translation), controller.rootPosition);
    quat4f rootWorldRot = v_quat_mul_quat(controller.rootRotation, nodes[rootId].rotation);
    nodes[rootId].translation = v_quat_mul_vec3(invAnimcharRotation, v_sub(rootWorldPos, animcharTm.col3));
    nodes[rootId].rotation = v_quat_mul_quat(invAnimcharRotation, rootWorldRot);
  }
  update_tree(nodes, mm_animated_tree);
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].changeBit != 0)
      nodemask.set(i, true);
}

ECS_TAG(render, dev)
ECS_NO_ORDER
static void debug_motion_matching_skeleton_es(const ecs::UpdateStageInfoAct &,
  ecs::EntityId mm_imguiAnimcharEid,
  bool mm_visualization_show_skeleton,
  bool mm_visualization_show_skeleton_original,
  int mm_visualization__selectedClipIdx,
  int mm_visualization__selectedFrameIdx,
  ecs::Point3List &mm_visualization__skeletonNodeWpos,
  ecs::IntList &mm_visualization__skeletonNodeFilter)
{
  if (!mm_visualization_show_skeleton)
    return;
  mm_visualization__skeletonNodeWpos.clear();
  imgui_prepare_skeleton_visualization_ecs_query(mm_imguiAnimcharEid,
    [&](const MotionMatchingController &motion_matching__controller, const AnimV20::AnimcharBaseComponent &animchar) {
      if (!motion_matching__controller.hasActiveAnimation())
        return;
      GeomNodeTree copiedTree = animchar.getNodeTree();
      mm_visualization__skeletonNodeFilter.clear();
      if (!mm_visualization_show_skeleton_original)
      {
        copiedTree.invalidateWtm();
        eastl::bitvector<> nodeMask(copiedTree.nodeCount());
        if (mm_visualization__selectedClipIdx < 0)
          get_current_mm_pose(copiedTree, nodeMask, motion_matching__controller, animchar);
        else
          get_selected_anim_pose(copiedTree, nodeMask, mm_visualization__selectedClipIdx, mm_visualization__selectedFrameIdx, animchar,
            motion_matching__controller);
        copiedTree.calcWtm();
        for (int i = 0, n = nodeMask.size(); i < n; ++i)
          if (nodeMask[i])
            mm_visualization__skeletonNodeFilter.push_back(i);
      }
      mm_visualization__skeletonNodeWpos.resize(copiedTree.nodeCount());
      for (dag::Index16 i(0), n(copiedTree.nodeCount()); i < n; ++i)
        mm_visualization__skeletonNodeWpos[i.index()] = copiedTree.getNodeWposScalar(i);
    });
}

template <class T>
inline void enable_mm_on_player_ecs_query(T);
template <class T>
inline void disable_mm_on_player_ecs_query(T);

static bool enable_motion_matching_cheat(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("motion_matching", "toggle_on_player", 1, 4)
  {
    bool mmWasEnabled = false;
    const char *mmComponentName = argc > 1 ? argv[1] : "motion_matching_animchar_component";
    disable_mm_on_player_ecs_query([&](ECS_REQUIRE(ecs::EntityId watchedByPlr)
                                       ECS_REQUIRE(MotionMatchingController motion_matching__controller) ecs::EntityId eid) {
      mmWasEnabled = true;
      if (!find_sub_template_name(g_entity_mgr->getEntityTemplateName(eid), mmComponentName))
        console::print("Failed to disable MM for player. MM component '%s' is not a subtemplate", mmComponentName);
      else
        remove_sub_template_async(eid, mmComponentName);
    });

    if (mmWasEnabled)
      return found;

    enable_mm_on_player_ecs_query([&](ECS_REQUIRE(ecs::EntityId watchedByPlr) ecs::EntityId eid) {
      ecs::ComponentsInitializer comps;
      if (argc > 2)
        ECS_INIT(comps, motion_matching__dataBaseTemplateName, eastl::string(argv[2]));
      add_sub_template_async(eid, mmComponentName, eastl::move(comps));
    });
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(enable_motion_matching_cheat);

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <util/dag_string.h>

template <class T>
inline void process_current_character_ecs_query(ecs::EntityId, T);
template <class T>
inline void get_current_imgui_character_ecs_query(T);
template <class T>
inline void update_clip_for_debug_trajectory_ecs_query(T);

static void motion_matching_weights()
{
  if (ImGui::IsWindowCollapsed())
    return;
  ecs::EntityId selectedEntity;
  IPoint2 prevTransition(-1, -1);
  get_current_imgui_character_ecs_query([&selectedEntity, &prevTransition](ecs::EntityId mm_imguiAnimcharEid,
                                          int mm_visualization__prevTransitionClipIdx, int mm_visualization__prevTransitionFrameIdx) {
    selectedEntity = mm_imguiAnimcharEid;
    prevTransition.x = mm_visualization__prevTransitionClipIdx;
    prevTransition.y = mm_visualization__prevTransitionFrameIdx;
  });
  if (!selectedEntity)
    ImGui::Text("No entity is selected.\nMake sure that `motion matching` imgui window is open.");
  process_current_character_ecs_query(selectedEntity,
    [prevTransition](const AnimV20::AnimcharBaseComponent &animchar, MotionMatchingController &motion_matching__controller,
      const FrameFeatures &motion_matching__goalFeature, int motion_matching__presetIdx) {
      if (!motion_matching__controller.hasActiveAnimation())
      {
        ImGui::Text("No active animation");
        return;
      }
      const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
      int normalizationGroupsCount = dataBase.featuresNormalizationGroups.nameCount();
      dag::Vector<vec4f, framemem_allocator> normalizedFeatures(dataBase.featuresSize * normalizationGroupsCount);
      for (int i = 0; i < normalizationGroupsCount; ++i)
      {
        int offset = i * dataBase.featuresSize;
        normalize_feature(make_span(normalizedFeatures.data() + offset, dataBase.featuresSize),
          motion_matching__goalFeature.data.data(), dataBase.featuresAvg.data() + offset, dataBase.featuresStd.data() + offset);
      }

      if (dataBase.tagsPresets.size() <= (uint32_t)motion_matching__presetIdx)
        return;
      const FeatureWeights &currentWeights = dataBase.tagsPresets[motion_matching__presetIdx].weights;

      int cur_clip = -1;
      int cur_frame = -1;
      float curMetric = 0.0;
      if (motion_matching__controller.hasActiveAnimation())
      {
        cur_clip = motion_matching__controller.getCurrentClip();
        cur_frame = motion_matching__controller.getCurrentFrame();
        int featureSize = dataBase.featuresSize;
        const vec4f *frameFeature = dataBase.clips[cur_clip].features.data.data() + cur_frame * featureSize;
        int featuresOffset = dataBase.clips[cur_clip].featuresNormalizationGroup * featureSize;
        curMetric = feature_distance_metric(normalizedFeatures.data() + featuresOffset, frameFeature,
          currentWeights.featureWeights.data(), featureSize);
      }


      MatchingResult currentState = {cur_clip, cur_frame, FLT_MAX};
      MatchingResult result =
        motion_matching(dataBase, currentState, motion_matching__controller.currentTags, currentWeights, normalizedFeatures);

      String tags;
      for (int tag_idx = 0, num_tags = dataBase.getTagsCount(); tag_idx < num_tags; ++tag_idx)
      {
        if (motion_matching__controller.currentTags.isTagRequired(tag_idx))
          tags.aprintf(0, "%s ", dataBase.getTagName(tag_idx));
        else if (motion_matching__controller.currentTags.isTagExcluded(tag_idx))
          tags.aprintf(0, "!%s ", dataBase.getTagName(tag_idx));
      }

      ImGui::Text("        [clip, frame]: metric\n"
                  "current [%4d, %5d]: %f\n"
                  "matched [%4d, %5d]: %f\n"
                  "MM blending weight: %f",
        cur_clip, cur_frame, curMetric, result.clip, result.frame, result.metrica, motion_matching__controller.motionMatchingWeight);

      ImGui::Text("goal tags:");
      if (dataBase.hasAvailableAnimations(motion_matching__controller.currentTags))
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0.1, 1), "%s", tags.c_str());
      }
      else
      {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1, 0.1, 0.1, 1), "%s", tags.c_str());
        ImGui::SameLine();
        ImGui::Text("(no available animations)");
      }
      static float metricScale = 1.f / 500.f;
      String valueTempStr(0, "%.04f", metricScale);
      ImGui::SetNextItemWidth(ImGui::CalcTextSize(valueTempStr).x + ImGui::GetStyle().FramePadding.x * 2.0f);
      ImGui::DragFloat("Metric scale", &metricScale, 0.001f, 0.0f, 10.0f, "%.04f", ImGuiSliderFlags_Logarithmic);
      static bool filterByTags = true;
      static bool sortRecent = true;
      static int sortThreshold = 5;
      static dag::Vector<int> recentClips;
      ImGui::Checkbox("Filter by tags", &filterByTags);
      ImGui::SameLine();
      ImGui::Checkbox("Sort by recent", &sortRecent);
      if (sortRecent && !dataBase.clips.empty())
      {
        valueTempStr.printf(0, "%d", sortThreshold);
        ImGui::SetNextItemWidth(ImGui::CalcTextSize(valueTempStr).x + ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::InputScalar("Number of clips to lock", ImGuiDataType_S32, &sortThreshold);
        if (recentClips.size() != dataBase.clips.size())
        {
          recentClips.resize(dataBase.clips.size());
          for (int i = 0, ie = recentClips.size(); i < ie; ++i)
            recentClips[i] = i;
        }
        int oldPlace = find_value_idx(recentClips, cur_clip);
        if (oldPlace >= sortThreshold)
        {
          for (int i = oldPlace; i > 0; --i)
            recentClips[i] = recentClips[i - 1];
          recentClips[0] = cur_clip;
        }
      }

      auto filteredByTags = [&](const AnimationClip &clip) {
        if (!filterByTags)
          return false;
        bool availableByTags = false;
        for (const AnimationInterval &interval : clip.intervals)
          if (motion_matching__controller.currentTags.isMatch(interval.tags))
          {
            availableByTags = true;
            break;
          }
        return !availableByTags;
      };

      int featureSize = dataBase.featuresSize;
      int trajectorySize = dataBase.trajectorySize;


      const vec4f *featureWeightsPtr = currentWeights.featureWeights.data();


      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      float metricsVisualizationOffset = 200.f;
      float metricsMinSize = 120.f; // 40 frames
      ImGui::Dummy(ImVec2(metricsVisualizationOffset + metricsMinSize, 1));
      ImVec2 metricsVisualizationPos = ImGui::GetCursorScreenPos();

      for (int clip_idx = 0, clip_count = dataBase.clips.size(); clip_idx < clip_count; clip_idx++)
      {
        const AnimationClip &clip = dataBase.clips[clip_idx];
        if (filteredByTags(clip))
          continue;
        String clipRecord(0, "%s (%d)", clip.name.c_str(), clip.tickDuration);
        int metricsPadding = 5;
        metricsVisualizationOffset = max(metricsVisualizationOffset, ImGui::CalcTextSize(clipRecord.c_str()).x + metricsPadding);
      }
      const float windowContentRegionWidth = ImGui::GetContentRegionAvail().x;
      if (windowContentRegionWidth - metricsVisualizationOffset < metricsMinSize)
        metricsVisualizationOffset = windowContentRegionWidth - metricsMinSize;
      metricsVisualizationPos.x += metricsVisualizationOffset;
      ImVec2 pos = metricsVisualizationPos;
      ImVec2 size = ImVec2(3, 14);

      int timeParamId = dataBase.clips[cur_clip].animTreeTimer;
      if (timeParamId >= 0)
      {
        float time = animchar.getAnimState()->getParam(timeParamId);
        ImGui::Text("%s (%f)", animchar.getAnimGraph()->getParamName(timeParamId), time);
        int fakeFrame = fmodf(time / dataBase.clips[cur_clip].animTreeTimerCycle, 1.0f) * dataBase.clips[cur_clip].tickDuration;
        ImVec2 markPos = ImVec2(pos.x + fakeFrame * size.x, pos.y);
        draw_list->AddRectFilled(markPos, markPos + ImVec2(size.x * 2.f, size.y), 0xFFFFFFFF);
      }
      const int framesInOneLine = (windowContentRegionWidth - metricsVisualizationOffset) / size.x;


      String tooltipMessage;
      ImVec2 mousePos = ImGui::GetIO().MousePos;
      int clipOffset = timeParamId >= 0 ? 1 : 0;
      int curClipOffset = -1;
      int curFrameOffset = 0;
      for (int i = 0, clip_count = dataBase.clips.size(); i < clip_count; ++i)
      {
        int clip_idx = sortRecent ? recentClips[i] : i;
        const AnimationClip &nextClip = dataBase.clips[clip_idx];
        if (filteredByTags(nextClip))
          continue;

        String clipRecord(0, "%s (%d)", nextClip.name.c_str(), nextClip.tickDuration);
        int textSize = ImGui::CalcTextSize(clipRecord.c_str()).x;
        if (metricsVisualizationOffset < textSize)
        {
          float averageSymbolSize = (float)textSize / clipRecord.length();
          int truncatedSymbols = ceilf((textSize - metricsVisualizationOffset) / averageSymbolSize) + 3;
          clipRecord.printf(0, "%.*s... (%d)", max(1, (int)nextClip.name.length() - truncatedSymbols), nextClip.name.c_str(),
            nextClip.tickDuration);
        }
        ImGui::TextUnformatted(clipRecord.c_str());
        if (metricsVisualizationOffset < textSize && ImGui::IsItemHovered())
          tooltipMessage.aprintf(0, "%s", nextClip.name.c_str());

        int featuresOffset = nextClip.featuresNormalizationGroup * featureSize;
        const vec4f *goalFeaturePtr = normalizedFeatures.data() + featuresOffset;
        const vec3f *nextFeaturePtr = nextClip.features.data.data();
        for (int frame = 0, frameOffset = 0; frame < nextClip.tickDuration; frame++, frameOffset++)
        {
          if (frameOffset >= framesInOneLine)
          {
            frameOffset = 0;
            clipOffset++;
            ImGui::Text("--- (%d-%d)", frame, min(frame + framesInOneLine, nextClip.tickDuration - 1));
          }
          float result = feature_distance_metric(goalFeaturePtr, nextFeaturePtr, featureWeightsPtr, featureSize);
          float trajectory = feature_distance_metric(goalFeaturePtr, nextFeaturePtr, featureWeightsPtr, trajectorySize);
          float costBias = nextClip.featuresCostBias;
          float pose = result - trajectory;
          nextFeaturePtr += featureSize;

          ImVec2 p = ImVec2(pos.x + frameOffset * size.x, pos.y + clipOffset * ImGui::GetTextLineHeightWithSpacing());
          float trajectoryValue = trajectory * metricScale;
          float poseValue = pose * metricScale;
          auto trajectoryColor = ImGui::ColorConvertFloat4ToU32(ImVec4(trajectoryValue, 1.f - trajectoryValue, 0.f, 1.f));
          auto poseColor = ImGui::ColorConvertFloat4ToU32(ImVec4(poseValue, 0.f, 1.f - poseValue, 1.f));
          draw_list->AddRectFilled(ImVec2(p.x, p.y + size.y * 0.0f), ImVec2(p.x + size.x, p.y + size.y * 0.5f), trajectoryColor);
          draw_list->AddRectFilled(ImVec2(p.x, p.y + size.y * 0.5f), ImVec2(p.x + size.x, p.y + size.y * 1.0f), poseColor);
          if (ImGui::IsWindowHovered() && p.x <= mousePos.x && mousePos.x < p.x + size.x && p.y <= mousePos.y &&
              mousePos.y < p.y + size.y)
          {
            tooltipMessage.printf(0,
              "frame: %d\n"
              "cost: %f\n"
              "- trajectory: %f\n",
              frame, result + costBias, trajectory);
            FrameFeatures goalFeaturesCopy = motion_matching__goalFeature;
            goalFeaturesCopy.data.assign(goalFeaturePtr, goalFeaturePtr + featureSize);
            TrajectoryFeature goalTrajectory = goalFeaturesCopy.get_trajectory_feature(0);
            ConstTrajectoryFeature clipTrajectory = nextClip.features.get_trajectory_feature(frame);
            for (int i = 0, ie = goalTrajectory.rootPositions.size(); i < ie; ++i)
            {
              float posWeight = currentWeights.rootPositions[i];
              float posCost = lengthSq((goalTrajectory.rootPositions[i] - clipTrajectory.rootPositions[i]) * posWeight);
              float dirWeight = currentWeights.rootDirections[i];
              float dirCost = lengthSq((goalTrajectory.rootDirections[i] - clipTrajectory.rootDirections[i]) * dirWeight);
              tooltipMessage.aprintf(0, "  - pt%d pos: %f, dir: %f\n", i, posCost, dirCost);
            }
            tooltipMessage.aprintf(0, "- pose: %f\n", pose);
            NodeFeature goalNodeFeatures = goalFeaturesCopy.get_node_feature(0);
            ConstNodeFeature clipNodeFeatures = nextClip.features.get_node_feature(frame);
            for (int i = 0, ie = goalNodeFeatures.nodePositions.size(); i < ie; ++i)
            {
              float posWeight = currentWeights.nodePositions[i];
              float posCost = lengthSq((goalNodeFeatures.nodePositions[i] - clipNodeFeatures.nodePositions[i]) * posWeight);
              float velWeight = currentWeights.nodeVelocities[i];
              float velCost = lengthSq((goalNodeFeatures.nodeVelocities[i] - clipNodeFeatures.nodeVelocities[i]) * velWeight);
              tooltipMessage.aprintf(0, "  - %s pos: %f, vel: %f\n", dataBase.nodesName[i], posCost, velCost);
              tooltipMessage.aprintf(0, "velocity: %@\n", dataBase.getNodeVelocityFeature(clip_idx, frame, i));
            }
            if (costBias != 0.0f)
              tooltipMessage.aprintf(0, "- costBias: %f\n", costBias);
            tooltipMessage.aprintf(0, "legs lock state: \n");
            for (int i = 0, numLegs = dataBase.footLockerNodes.size(); i < numLegs; i++)
              tooltipMessage.aprintf(0, " - leg %@: %@\n", i, dataBase.clips[clip_idx].footLockerStates.needLock(i, frame, numLegs));
            update_clip_for_debug_trajectory_ecs_query(
              [clip_idx, frame](int &mm_visualization__selectedClipIdx, int &mm_visualization__selectedFrameIdx) {
                mm_visualization__selectedClipIdx = clip_idx;
                mm_visualization__selectedFrameIdx = frame;
              });
          }
          if (cur_clip == clip_idx && cur_frame == frame)
          {
            curClipOffset = clipOffset;
            curFrameOffset = frameOffset;
          }
          if (clip_idx == prevTransition.x && frame == prevTransition.y)
          {
            ImU32 prevTransitionColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 0.25f, 1.f, 1.f));
            draw_list->AddRectFilled(p, p + size, prevTransitionColor);
          }
        }
        clipOffset++;
      }

      if (curClipOffset >= 0)
      {
        ImVec2 p = ImVec2(pos.x + curFrameOffset * size.x, pos.y + curClipOffset * ImGui::GetTextLineHeightWithSpacing());
        auto currentColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
        draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + size.x * 2.f, p.y + size.y), currentColor);
      }

      if (tooltipMessage.length())
      {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltipMessage);
        ImGui::EndTooltip();
      }
      else
      {
        update_clip_for_debug_trajectory_ecs_query(
          [](int &mm_visualization__selectedClipIdx, int &mm_visualization__selectedFrameIdx) {
            mm_visualization__selectedClipIdx = -1;
            mm_visualization__selectedFrameIdx = -1;
          });
      }
    });
}

REGISTER_IMGUI_WINDOW("Anim", "Motion Matching metrics", motion_matching_weights);
