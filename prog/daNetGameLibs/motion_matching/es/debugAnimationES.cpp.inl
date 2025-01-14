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
inline void gather_data_base_ecs_query(T);

void wait_motion_matching_job();

static void draw_skeleton_links(const GeomNodeTree &tree, dag::Index16 n, const eastl::bitvector<> *node_mask, bool show_labels)
{
  Point3 nodePos = tree.getNodeWposScalar(n);
  if (!node_mask || node_mask->at(n.index()))
  {
    if (show_labels && tree.getNodeName(n))
      add_debug_text_mark(nodePos, String(0, "%s (%d)", tree.getNodeName(n), n.index()));
    draw_cached_debug_sphere(nodePos, 0.03, E3DCOLOR(0, 0, 255, 255), 10);
  }
  for (unsigned i = 0, ie = tree.getChildCount(n); i < ie; ++i)
  {
    dag::Index16 cn = tree.getChildNodeIdx(n, i);
    Point3 childPos = tree.getNodeWposScalar(cn);
    draw_cached_debug_line(nodePos, childPos, E3DCOLOR(128, 128, 255, 255));
    draw_skeleton_links(tree, cn, node_mask, show_labels);
  }
}

template <class T>
inline void imgui_visualization_skeleton_ecs_query(T);

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
  update_tree(nodes, mm_animated_tree);
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].changeBit != 0)
      nodemask.set(i, true);
}

ECS_TAG(render, dev)
ECS_NO_ORDER
ECS_REQUIRE(eastl::true_type motion_matching__enabled)
static void debug_motion_matching_skeleton_es(const ecs::UpdateStageInfoRenderDebug &,
  const MotionMatchingController &motion_matching__controller,
  AnimV20::AnimcharBaseComponent &animchar)
{
  imgui_visualization_skeleton_ecs_query(
    [&](bool mm_visualization_show_skeleton, bool mm_visualization_show_skeleton_original,
      bool mm_visualization_show_skeleton_node_labels, int mm_visualization__selectedClipIdx, int mm_visualization__selectedFrameIdx) {
      if (!mm_visualization_show_skeleton)
        return;
      wait_motion_matching_job();
      if (!motion_matching__controller.hasActiveAnimation())
        return;
      begin_draw_cached_debug_lines(false, true);
      if (mm_visualization_show_skeleton_original)
      {
        const GeomNodeTree &tree = animchar.getNodeTree();
        draw_skeleton_links(tree, dag::Index16(0), nullptr, mm_visualization_show_skeleton_node_labels);
      }
      else
      {
        const GeomNodeTree &tree = animchar.getNodeTree();
        GeomNodeTree mmAnimatedTree = tree;
        mmAnimatedTree.invalidateWtm();
        eastl::bitvector<> nodeMask(mmAnimatedTree.nodeCount());
        if (mm_visualization__selectedClipIdx < 0)
          get_current_mm_pose(mmAnimatedTree, nodeMask, motion_matching__controller, animchar);
        else
          get_selected_anim_pose(mmAnimatedTree, nodeMask, mm_visualization__selectedClipIdx, mm_visualization__selectedFrameIdx,
            motion_matching__controller);
        mmAnimatedTree.calcWtm();
        draw_skeleton_links(mmAnimatedTree, dag::Index16(0), &nodeMask, mm_visualization_show_skeleton_node_labels);
      }
      end_draw_cached_debug_lines();
    });
}

template <class T>
inline void imgui_visualization_debug_features_ecs_query(T);
template <class T>
inline void get_clip_for_debug_trajectory_ecs_query(T);

ECS_TAG(render, dev)
ECS_NO_ORDER
ECS_REQUIRE(eastl::true_type motion_matching__enabled)
static void debug_motion_matching_es(const ecs::UpdateStageInfoRenderDebug &,
  ecs::EntityId eid,
  const AnimV20::AnimcharBaseComponent &animchar,
  const MotionMatchingController &motion_matching__controller,
  const ecs::Point3List &mm_trajectory__featurePositions,
  const ecs::Point3List &mm_trajectory__featureDirections,
  const FrameFeatures &motion_matching__goalFeature)
{
  bool debugNodes = false, debugTrajectory = false;
  imgui_visualization_debug_features_ecs_query([&](bool mm_visualization_show_feature_nodes, bool mm_visualization_show_trajectory) {
    debugNodes = mm_visualization_show_feature_nodes;
    debugTrajectory = mm_visualization_show_trajectory;
  });

  if (!(debugNodes || debugTrajectory))
    return;
  wait_motion_matching_job();
  if (!motion_matching__controller.hasActiveAnimation())
    return;
  const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
  int cur_clip = motion_matching__controller.getCurrentClip();
  int cur_frame = motion_matching__controller.getCurrentFrame();
  bool curClipOverride = false;
  get_clip_for_debug_trajectory_ecs_query(
    [&](ecs::EntityId mm_imguiAnimcharEid, int mm_visualization__selectedClipIdx, int mm_visualization__selectedFrameIdx) {
      if (mm_visualization__selectedClipIdx < 0 || eid != mm_imguiAnimcharEid)
        return;
      G_ASSERT(mm_visualization__selectedFrameIdx >= 0);
      cur_clip = mm_visualization__selectedClipIdx;
      cur_frame = mm_visualization__selectedFrameIdx;
      curClipOverride = true;
    });

  // create temporal copy for denormalized features
  auto clipBegin = dataBase.clips[cur_clip].features.data.data() + cur_frame * dataBase.featuresSize;

  // need to create copy for denormalization
  FrameFeatures clipCopy = motion_matching__goalFeature;
  clipCopy.data.assign(clipBegin, clipBegin + dataBase.featuresSize);
  int clipFeaturesOffset = dataBase.clips[cur_clip].featuresNormalizationGroup * dataBase.featuresSize;
  denormalize_feature(make_span(clipCopy.data), dataBase.featuresAvg.data() + clipFeaturesOffset,
    dataBase.featuresStd.data() + clipFeaturesOffset);
  FrameFeature clipFeature = clipCopy.get_feature(0);
  ConstFrameFeature goalFeature = motion_matching__goalFeature.get_feature(0);

  // It is next frame tm because physics and animchars are updated after MM update but before debug
  // rendering. Not sure that it is a real issue because it affects only debug visualization
  TMatrix animcharTm;
  animchar.getTm(animcharTm);

  // Can differ from gameplay animchar transform when synchronization is disabled because root node
  // will be moved according trajectory from animations
  TMatrix mmRootTm;
  mat44f tmpTm;
  v_mat44_from_quat(tmpTm, motion_matching__controller.rootRotation, motion_matching__controller.rootPosition);
  v_mat_43cu_from_mat44(mmRootTm.array, tmpTm);

  constexpr float VELOCITY_SCALE = 0.15f;
  constexpr float DIRECTION_SCALE = 0.35f;
  begin_draw_cached_debug_lines(false, true);

  for (int i = 0, n = dataBase.nodeCount; debugNodes && i < n; i++)
  {
    Point3 p = mmRootTm * goalFeature.nodePositions[i];
    Point3 v = mmRootTm % goalFeature.nodeVelocities[i];
    draw_cached_debug_sphere(p, 0.15f, E3DCOLOR(0xFFFFFFFF), 10);
    draw_debug_line(p, p + v * VELOCITY_SCALE, E3DCOLOR(0xFF00FFFF));
  }

  for (int i = -1, n = dataBase.predictionTimes.size(); debugTrajectory && i < n; i++)
  {
    Point3 predictedP = i < 0 ? animcharTm.col[3] : mmRootTm * mm_trajectory__featurePositions[i];
    Point3 predictedV = i < 0 ? -animcharTm.col[2] : mmRootTm % mm_trajectory__featureDirections[i];
    Point3 animatedP = i < 0 ? mmRootTm.col[3] : mmRootTm * Point3::x0y(clipFeature.rootPositions[i]);
    Point3 animatedV = i < 0 ? -mmRootTm.col[2] : mmRootTm % Point3::x0y(clipFeature.rootDirections[i]);

    auto predictedColor = i < 0 ? E3DCOLOR_MAKE(50, 150, 255, 255) : E3DCOLOR_MAKE(0, 0, 255, 255);
    auto animatedColor = curClipOverride ? E3DCOLOR_MAKE(177, 177, 177, 255) : E3DCOLOR_MAKE(0, 255, 0, 255);
    draw_cached_debug_sphere(predictedP, 0.03f, predictedColor, 10);
    draw_cached_debug_sphere(animatedP, 0.03f, animatedColor, 10);
    draw_debug_line(predictedP, predictedP + predictedV * DIRECTION_SCALE, predictedColor);
    draw_debug_line(animatedP, animatedP + animatedV * DIRECTION_SCALE, animatedColor);
  }
  end_draw_cached_debug_lines();
}
ECS_BROADCAST_EVENT_TYPE(InvalidateAnimationDataBase);
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
  ecs::EntityId selectedEntity;
  get_current_imgui_character_ecs_query(
    [&selectedEntity](ecs::EntityId mm_imguiAnimcharEid) { selectedEntity = mm_imguiAnimcharEid; });
  if (!selectedEntity)
    ImGui::Text("No entity is selected.\nMake sure that `motion matching` imgui window is open.");
  process_current_character_ecs_query(selectedEntity,
    [](const AnimV20::AnimcharBaseComponent &animchar, MotionMatchingController &motion_matching__controller,
      const FrameFeatures &motion_matching__goalFeature, int motion_matching__presetIdx) {
      if (!motion_matching__controller.hasActiveAnimation())
      {
        ImGui::Text("No active animation");
        return;
      }
      const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
      dag::Vector<vec4f, framemem_allocator> normalizedFeatures(dataBase.featuresSize * dataBase.normalizationGroupsCount);
      for (int i = 0; i < dataBase.normalizationGroupsCount; ++i)
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


      int featureSize = dataBase.featuresSize;
      int trajectorySize = dataBase.trajectorySize;


      const vec4f *featureWeightsPtr = currentWeights.featureWeights.data();


      ImDrawList *draw_list = ImGui::GetWindowDrawList();
      ImVec2 metricsVisualizationPos = ImGui::GetCursorScreenPos();
      float metricsVisualizationOffset = 200.f;
      for (int clip_idx = 0, clip_count = dataBase.clips.size(); clip_idx < clip_count; clip_idx++)
      {
        const AnimationClip &clip = dataBase.clips[clip_idx];
        String clipRecord(0, "%s (%d)", clip.name.c_str(), clip.tickDuration);
        metricsVisualizationOffset = max(metricsVisualizationOffset, ImGui::CalcTextSize(clipRecord.c_str()).x);
      }

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
      const float windowContentRegionWidth = ImGui::GetContentRegionAvail().x;
      const int framesInOneLine = (windowContentRegionWidth - metricsVisualizationOffset) / size.x;


      String tooltipMessage;
      ImVec2 mousePos = ImGui::GetIO().MousePos;
      int clipOffset = timeParamId >= 0 ? 1 : 0;
      int curClipOffset = 1;
      int curFrameOffset = 0;
      for (int i = 0, clip_count = dataBase.clips.size(); i < clip_count; ++i)
      {
        int clip_idx = sortRecent ? recentClips[i] : i;
        const AnimationClip &nextClip = dataBase.clips[clip_idx];
        if (filterByTags)
        {
          bool availableByTags = false;
          for (const AnimationInterval &interval : nextClip.intervals)
            if (motion_matching__controller.currentTags.isMatch(interval.tags))
            {
              availableByTags = true;
              break;
            }
          if (!availableByTags)
            continue;
        }
        ImGui::Text("%s (%d)", nextClip.name.c_str(), nextClip.tickDuration);


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
          float pose = result - trajectory;
          nextFeaturePtr += featureSize;

          ImVec2 p = ImVec2(pos.x + frameOffset * size.x, pos.y + clipOffset * ImGui::GetTextLineHeightWithSpacing());
          float trajectoryValue = trajectory * metricScale;
          float poseValue = pose * metricScale;
          auto trajectoryColor = ImGui::ColorConvertFloat4ToU32(ImVec4(trajectoryValue, 1.f - trajectoryValue, 0.f, 1.f));
          auto poseColor = ImGui::ColorConvertFloat4ToU32(ImVec4(poseValue, 0.f, 1.f - poseValue, 1.f));
          draw_list->AddRectFilled(ImVec2(p.x, p.y + size.y * 0.0f), ImVec2(p.x + size.x, p.y + size.y * 0.5f), trajectoryColor);
          draw_list->AddRectFilled(ImVec2(p.x, p.y + size.y * 0.5f), ImVec2(p.x + size.x, p.y + size.y * 1.0f), poseColor);
          if (p.x <= mousePos.x && mousePos.x < p.x + size.x && p.y <= mousePos.y && mousePos.y < p.y + size.y)
          {
            tooltipMessage.printf(0,
              "frame: %d\n"
              "cost: %f\n"
              "- trajectory: %f\n",
              frame, result, trajectory);
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
              tooltipMessage.aprintf(0, "velocity: %@\n", clipNodeFeatures.nodeVelocities[i]);
            }
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
        }
        clipOffset++;
      }

      ImVec2 p = ImVec2(pos.x + curFrameOffset * size.x, pos.y + curClipOffset * ImGui::GetTextLineHeightWithSpacing());
      auto currentColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1.f));
      draw_list->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + size.x * 2.f, p.y + size.y), currentColor);

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
