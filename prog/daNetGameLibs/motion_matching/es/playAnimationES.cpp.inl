// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <ecs/anim/anim.h>
#include <animation/animation_data_base.h>
#include <animation/motion_matching.h>
#include <animation/motion_matching_metrics.h>
#include <animation/motion_matching_tags.h>
#include <animation/motion_matching_controller.h>
#include <animation/motion_matching_animgraph_nodes.h>
#include <animation/feature_normalization.h>
#include <ecs/delayedAct/actInThread.h>
#include <util/dag_threadPool.h>


#define BRUTE_FORCE_COMPARISON 0 // for development usage, regression testing

ECS_TAG(render)
ECS_AFTER(mm_trajectory_update)
ECS_BEFORE(mm_update_goal_features_es)
static void mm_update_root_orientation_es(const ParallelUpdateFrameDelayed &act,
  const AnimV20::AnimcharBaseComponent &animchar,
  const Point3 &mm_trajectory__linearVelocity,
  const Point3 &mm_trajectory__angularVelocity,
  MotionMatchingController &motion_matching__controller)
{
  motion_matching__controller.updateRoot(act.dt, animchar, mm_trajectory__linearVelocity, mm_trajectory__angularVelocity);
}

ECS_TAG(render)
ECS_AFTER(after_guns_update_sync, wait_motion_matching_job_es)
ECS_BEFORE(animchar_es)
static void mm_calculate_root_offset_es(const ParallelUpdateFrameDelayed &,
  const TMatrix &transform,
  MotionMatchingController &motion_matching__controller,
  bool animchar__turnDir = false)
{
  if (!motion_matching__controller.dataBase)
    return;
  mat44f animcharTm;
  v_mat44_make_from_43cu(animcharTm, transform.array);
  if (animchar__turnDir)
  {
    vec3f col0 = animcharTm.col0;
    animcharTm.col0 = animcharTm.col2;
    animcharTm.col2 = v_neg(col0);
  }
  quat4f animcharRotation = v_quat_from_mat43(animcharTm);
  quat4f invAnimcharRotation = v_quat_conjugate(animcharRotation);
  int rootId = motion_matching__controller.dataBase->rootNode.index();
  MotionMatchingController &ctrl = motion_matching__controller;
  vec3f rootWorldPos = v_add(v_quat_mul_vec3(ctrl.rootRotation, ctrl.resultAnimation.position[rootId]), ctrl.rootPosition);
  quat4f rootWorldRot = v_quat_mul_quat(ctrl.rootRotation, ctrl.resultAnimation.rotation[rootId]);
  vec3f rootLocalPos = v_quat_mul_vec3(invAnimcharRotation, v_sub(rootWorldPos, animcharTm.col3));
  quat4f rootLocalRot = v_quat_mul_quat(invAnimcharRotation, rootWorldRot);
  motion_matching__controller.rootPRS.position = rootLocalPos;
  motion_matching__controller.rootPRS.rotation = rootLocalRot;
}

ECS_TAG(render)
ECS_BEFORE(motion_matching_job_es)
static void mm_update_goal_features_es(const ParallelUpdateFrameDelayed &act,
  FrameFeatures &motion_matching__goalFeature,
  const ecs::IntList &motion_matching__goalNodesIdx,
  bool motion_matching__enabled,
  const AnimV20::AnimcharBaseComponent &animchar,
  const MotionMatchingController &motion_matching__controller,
  const ecs::Point3List &mm_trajectory__featurePositions,
  const ecs::Point3List &mm_trajectory__featureDirections)
{
  if (!motion_matching__controller.dataBase)
    return;
  const GeomNodeTree &tree = animchar.getNodeTree();
  if (!motion_matching__enabled && !motion_matching__controller.hasActiveAnimation())
  {
    // copy current positions from nodeTree to be able to calculate velocities in next frame
    dag::Span<Point3> nodePositions = motion_matching__goalFeature.get_node_positions(0);
    for (int i = 0, e = motion_matching__goalNodesIdx.size(); i < e; ++i)
      nodePositions[i] = tree.getNodeWposScalar(dag::Index16(motion_matching__goalNodesIdx[i]));
    return;
  }
  TrajectoryFeature trajectoryFeature = motion_matching__goalFeature.get_trajectory_feature(0);
  for (int i = 0, n = mm_trajectory__featurePositions.size(); i < n; i++)
  {
    trajectoryFeature.rootPositions[i] = Point2::xz(mm_trajectory__featurePositions[i]);
    trajectoryFeature.rootDirections[i] = Point2::xz(mm_trajectory__featureDirections[i]);
  }
  if (!motion_matching__controller.hasActiveAnimation())
  {
    NodeFeature nodeFeature = motion_matching__goalFeature.get_node_feature(0);
    mat44f invRoot;
    v_mat44_inverse43(invRoot, tree.getRootWtmRel());
    for (int i = 0, e = motion_matching__goalNodesIdx.size(); i < e; ++i)
    {
      dag::Index16 nodeId = dag::Index16(motion_matching__goalNodesIdx[i]);
      vec3f deltaPos = v_sub(tree.getNodeWpos(nodeId), v_ldu(&nodeFeature.nodePositions[i].x));
      v_stu_p3(&nodeFeature.nodeVelocities[i].x, v_mat44_mul_vec3v(invRoot, v_safediv(deltaPos, v_splats(act.dt))));
      v_stu_p3(&nodeFeature.nodePositions[i].x, v_mat44_mul_vec3p(invRoot, tree.getNodeWposRel(nodeId)));
    }
  }
}

static void copy_pose_features(FrameFeatures &goal_feature, const MotionMatchingController &controller)
{
  const AnimationDataBase &dataBase = *controller.dataBase;
  NodeFeature dstFeature = goal_feature.get_node_feature(0);
  // when we play animation
  if (controller.hasActiveAnimation())
  {
    int clip = controller.getCurrentClip();
    int frame = controller.getCurrentFrame();

    ConstNodeFeature srcFeature = dataBase.clips[clip].features.get_node_feature(frame);

    for (int i = 0; i < dataBase.nodeCount; i++)
    {
      dstFeature.nodePositions[i] = srcFeature.nodePositions[i];
      dstFeature.nodeVelocities[i] = srcFeature.nodeVelocities[i];
    }
    int nodeFeaturesOffset =
      dataBase.clips[clip].featuresNormalizationGroup * dataBase.featuresSize + goal_feature.trajectorySizeInVec4f;
    denormalize_feature(goal_feature.get_node_features_raw(0), dataBase.featuresAvg.data() + nodeFeaturesOffset,
      dataBase.featuresStd.data() + nodeFeaturesOffset);
  }
}

ECS_TAG(render)
ECS_BEFORE(motion_matching_job_es)
static void update_tag_changes_es(const ParallelUpdateFrameDelayed &,
  MotionMatchingController &motion_matching__controller,
  AnimV20::AnimcharBaseComponent &animchar,
  int &motion_matching__presetIdx,
  float &motion_matching__presetBlendTimeLeft,
  float &mm_trajectory__linearVelocityViscosity,
  float &mm_trajectory__angularVelocityViscosity,
  bool &motion_matching__enabled)
{
  if (!motion_matching__controller.dataBase)
    return;
  const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
  AnimationFilterTags &currentTags = motion_matching__controller.currentTags;
  if (motion_matching__controller.useTagsFromAnimGraph)
  {
    AnimV20::IAnimStateHolder &animState = *animchar.getAnimState();
    currentTags = get_mm_tags_from_animgraph(dataBase, animState);
    reset_animgraph_tags(dataBase, animState);
  }
  if (currentTags.wasChanged || motion_matching__presetIdx < 0)
  {
    currentTags.wasChanged = false;
    if (motion_matching__controller.useTagsFromAnimGraph)
      motion_matching__enabled = currentTags.selectedMask.any() && dataBase.hasAvailableAnimations(currentTags);
    if (!motion_matching__enabled && motion_matching__presetIdx >= 0)
      return;
    int prevPresetIdx = motion_matching__presetIdx;
    motion_matching__presetIdx = 0;
    for (int i = 0; i < dataBase.tagsPresets.size(); ++i)
    {
      int tagIdx = dataBase.tagsPresets[i].requiredTagIdx;
      if (tagIdx >= 0 && currentTags.isTagRequired(tagIdx))
      {
        motion_matching__presetIdx = i;
        break;
      }
    }
    if (motion_matching__presetIdx == prevPresetIdx)
      return;

    if (dataBase.tagsPresets.empty())
    {
      G_ASSERTF_ONCE(1, "need at least one motion matching preset");
      return;
    }
    const TagPreset &preset = dataBase.tagsPresets[motion_matching__presetIdx];
    const TagPreset &prevPreset = dataBase.tagsPresets[max(prevPresetIdx, 0)];
    mm_trajectory__linearVelocityViscosity = preset.linearVelocityViscosity;
    mm_trajectory__angularVelocityViscosity = preset.angularVelocityViscosity;
    motion_matching__presetBlendTimeLeft = max(preset.presetBlendTime, prevPreset.presetBlendTime);
    motion_matching__controller.transitionBlendTime = max(motion_matching__presetBlendTimeLeft, preset.animationBlendTime);
  }
}

static void update_node_weights(MotionMatchingController &controller, bool mm_enabled, float blend_time, float dt)
{
  const AnimationClip *curentClip = nullptr;
  if (mm_enabled && controller.hasActiveAnimation())
    curentClip = &controller.dataBase->clips[controller.getCurrentClip()];
  float weightDelta = dt / blend_time;
  controller.motionMatchingWeight = 0.0f;
  dag::Vector<float> &nodeWeights = controller.perNodeWeights;
  for (int i = 0, e = nodeWeights.size(); i < e; ++i)
  {
    float targetWeight = curentClip ? curentClip->nodeMask[i] : 0.0f;
    if (nodeWeights[i] < targetWeight)
      nodeWeights[i] = min(nodeWeights[i] + weightDelta, targetWeight);
    else
      nodeWeights[i] = max(nodeWeights[i] - weightDelta, targetWeight);
    controller.motionMatchingWeight = max(controller.motionMatchingWeight, nodeWeights[i]);
  }
}

template <typename T>
ECS_REQUIRE(eastl::true_type animchar__visible)
void motion_matching_ecs_query(T ECS_CAN_PARALLEL_FOR(c, 1));

static class MotionMatchingJob final : public cpujobs::IJob
{
public:
  float dt = 0.;
  uint32_t tpqpos = 0;
  int maxMotionMatchingPerFrame = 0;
  void doJob() override
  {
    TIME_PROFILE(motion_matching_job);
    int processedMotionMatchingCount = 0;

    motion_matching_ecs_query(
      [this, &processedMotionMatchingCount](MotionMatchingController &motion_matching__controller,
        float &motion_matching__updateProgress, float &motion_matching__metricaTolerance, float &motion_matching__presetBlendTimeLeft,
        float motion_matching__blendTimeToAnimtree, float motion_matching__distanceFactor, FrameFeatures &motion_matching__goalFeature,
        int motion_matching__presetIdx, bool motion_matching__enabled) {
        G_ASSERT_RETURN(!motion_matching__enabled || motion_matching__controller.dataBase, ); // don't enable until database is loaded
        update_node_weights(motion_matching__controller, motion_matching__enabled, motion_matching__blendTimeToAnimtree, dt);
        if (!motion_matching__enabled && motion_matching__controller.motionMatchingWeight <= 0.f)
        {
          // Fully animated by animTree, no need to keep previously matched animation
          motion_matching__controller.currentClipInfo = {};
          return;
        }
        bool animFinished = motion_matching__controller.willCurrentAnimationEnd(dt);

        const AnimationDataBase &dataBase = *motion_matching__controller.dataBase;
        const TagPreset &currentPreset = dataBase.tagsPresets[motion_matching__presetIdx];
        if (motion_matching__presetBlendTimeLeft > 0)
        {
          motion_matching__presetBlendTimeLeft -= dt;
          if (motion_matching__presetBlendTimeLeft <= 0)
            motion_matching__controller.transitionBlendTime = currentPreset.animationBlendTime;
        }

        const auto &clips = dataBase.clips;
        motion_matching__updateProgress += dt * TICKS_PER_SECOND;
        if (motion_matching__updateProgress >= motion_matching__distanceFactor || animFinished)
        {
          copy_pose_features(motion_matching__goalFeature, motion_matching__controller);
          dag::Span<vec4f> currentFeature;
          dag::Vector<vec4f, framemem_allocator> normalizedFeatures(dataBase.featuresSize * dataBase.normalizationGroupsCount);
          for (int i = 0; i < dataBase.normalizationGroupsCount; ++i)
          {
            int offset = i * dataBase.featuresSize;
            normalize_feature(make_span(normalizedFeatures.data() + offset, dataBase.featuresSize),
              motion_matching__goalFeature.data.data(), dataBase.featuresAvg.data() + offset, dataBase.featuresStd.data() + offset);
          }

          const FeatureWeights &currentWeights = currentPreset.weights;

          int featuresSizeof = dataBase.featuresSize;
          MatchingResult currentState = {-1, -1, FLT_MAX};
          bool performSearch = true;
          if (motion_matching__controller.hasActiveAnimation() && !animFinished)
          {
            int cur_clip = motion_matching__controller.getCurrentClip();
            int cur_frame = motion_matching__controller.getCurrentFrame();
            const vec4f *frameFeature = clips[cur_clip].features.data.data() + cur_frame * featuresSizeof;
            int featuresOffset = clips[cur_clip].featuresNormalizationGroup * dataBase.featuresSize;
            currentFeature = make_span(normalizedFeatures.data() + featuresOffset, dataBase.featuresSize);
            int curInterval = clips[cur_clip].getInterval(cur_frame);
            bool suitableTags = motion_matching__controller.currentTags.isMatch(clips[cur_clip].intervals[curInterval].tags);
            float currentMetric = suitableTags ? feature_distance_metric(currentFeature.data(), frameFeature,
                                                   currentWeights.featureWeights.data(), featuresSizeof)
                                               : FLT_MAX;
            currentState.clip = cur_clip;
            currentState.frame = cur_frame;
            currentState.metrica = currentMetric;

            // Current animation is good enough, we can't find better because of additional cost for transition
            if (motion_matching__metricaTolerance > currentMetric)
              performSearch = false;
          }
          if (maxMotionMatchingPerFrame > 0 && performSearch &&
              interlocked_increment(processedMotionMatchingCount) > maxMotionMatchingPerFrame)
          {
            performSearch = false;
            if (animFinished)
            {
              // todo: we should prioritize animchars with finished animations. This way we maybe won't exceed this limit.
              logwarn("Motion Matching: Limit of searches per frame was exceeded (%d/%d)", processedMotionMatchingCount,
                maxMotionMatchingPerFrame);
              performSearch = true;
            }
          }

          if (!DISABLE_MOTION_MATCHING && performSearch)
          {
            motion_matching__updateProgress -= (int)motion_matching__updateProgress;
            TIME_PROFILE(motion_matching);
            MatchingResult bestIndex =
              motion_matching(dataBase, currentState, motion_matching__controller.currentTags, currentWeights, normalizedFeatures);

#if BRUTE_FORCE_COMPARISON
            MatchingResult bestIndex2 = motion_matching_brute_force(dataBase, currentState, motion_matching__controller.currentTags,
              currentWeights, normalizedFeatures);

            G_ASSERT(is_same(bestIndex, bestIndex2));
#endif
            if (bestIndex.metrica + motion_matching__metricaTolerance < currentState.metrica)
            {
              bool needTransition = motion_matching__controller.hasActiveAnimation();
              motion_matching__metricaTolerance = currentPreset.metricaToleranceMax;
              motion_matching__controller.playAnimation(bestIndex.clip, bestIndex.frame, needTransition);
            }
          }
        }
        // Call update after MM search to finish transition faster
        motion_matching__controller.updateAnimationProgress(dt);
      });
  }
} motion_matching_job;

template <typename T>
void motion_matching_per_frame_limit_ecs_query(T);

ECS_TAG(render)
ECS_BEFORE(before_net_phys_sync)
static __forceinline void motion_matching_job_es(const ParallelUpdateFrameDelayed &act)
{
  int perFrameLimit = 0;
  motion_matching_per_frame_limit_ecs_query([&perFrameLimit](int main_database__perFrameLimit) {
    if (perFrameLimit == -1 || main_database__perFrameLimit == -1)
      perFrameLimit = -1;
    else
      perFrameLimit += main_database__perFrameLimit;
  });
  if (perFrameLimit == 0)
    return;
  motion_matching_job.dt = act.dt;
  motion_matching_job.maxMotionMatchingPerFrame = perFrameLimit;
  if (threadpool::get_num_workers() >= 4) // It's already does p-for query within (which might grab unrelated jobs)
    threadpool::add(&motion_matching_job, threadpool::PRIO_NORMAL, motion_matching_job.tpqpos);
  else
    motion_matching_job.doJob();
}

void wait_motion_matching_job()
{
  if (!interlocked_acquire_load(motion_matching_job.done))
  {
    TIME_PROFILE(wait_motion_matching_job_done);
    threadpool::barrier_active_wait_for_job(&motion_matching_job, threadpool::PRIO_NORMAL, motion_matching_job.tpqpos);
  }
}

ECS_TAG(render)
ECS_BEFORE(animchar_es)
ECS_AFTER(after_net_phys_sync)
static __forceinline void wait_motion_matching_job_es(const ParallelUpdateFrameDelayed &, int main_database__perFrameLimit)
{
  if (main_database__perFrameLimit == 0)
    return;
  wait_motion_matching_job();
}
