// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/anim/anim.h>
#include <daECS/core/componentTypes.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_gameResources.h>
#include <anim/dag_animKeyInterp.h>
#include <math/dag_mathAng.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include "animation_data_base.h"
#include "motion_matching_tags.h"
#include "feature_normalization.h"
#include "animation_sampling.h"
#include "savgol.h"

#include <sstream>
#include <ioSys/dag_dataBlock.h>


static Tags get_tags(const OAHashNameMap<false> &tagsMap, const char *tags)
{
  if (tags == nullptr)
    return 0;
  Tags tagMask;
  std::stringstream tagsStream(tags);
  std::string tag;

  while (std::getline(tagsStream, tag, ','))
  {
    int tag_idx = tagsMap.getNameId(tag.c_str());
    if (tag_idx >= 0)
      tagMask.set(tag_idx);
  }
  return tagMask;
}

static int anim_max_time(const AnimV20::AnimData::Anim &anim)
{
  int time = max(anim.originLinVel.keyTimeLast(), anim.originAngVel.keyTimeLast());
  time = max(time, anim.pos.keyTimeLast());
  time = max(time, anim.rot.keyTimeLast());
  time = max(time, anim.scl.keyTimeLast());
  return time;
}


static dag::Index16 find_anim_channel(const eastl::string &channel_name, const AnimV20::AnimDataChan &chan)
{
  if (!chan.hasKeys())
    return dag::Index16();
  for (int i = 0, n = chan.nodeNum; i < n; i++)
    if (!strcmp(chan.nodeName[i].get(), channel_name.c_str()))
      return chan.nodeTrack[i];
  return dag::Index16();
}

constexpr int a2dtimeConversion = 2 * 256;


static int frame_duration_from_a2d_duration(int a2d_time)
{
  return a2d_time / a2dtimeConversion; // magic consts
}

static void copy_animation(const AnimV20::AnimData &anim_data, AnimationClip &clip, const GeomNodeTree &tree, const DataBlock &mask)
{
  clip.nodeMask.resize(tree.nodeCount(), 0.f);
  for (dag::Index16 nodeIdx(0), nodeCount(tree.nodeCount()); nodeIdx != nodeCount; ++nodeIdx)
  {
    const char *name = tree.getNodeName(nodeIdx);

    float weight = mask.getReal(name, -1.f /* disabled by default */);
    if (weight <= 0.f)
      continue;
    clip.nodeMask[nodeIdx.index()] = weight;
    auto pos = find_anim_channel(name, anim_data.anim.pos);
    if (pos)
    {
      clip.channelTranslation.push_back({pos, nodeIdx});
    }
    auto rot = find_anim_channel(name, anim_data.anim.rot);
    if (rot)
    {
      clip.channelRotation.push_back({rot, nodeIdx});
    }
    auto scl = find_anim_channel(name, anim_data.anim.scl);
    if (scl)
    {
      clip.channelScale.push_back({scl, nodeIdx});
    }
  }
}

static void load_and_validate_root_scale(AnimationClip &clip, dag::Index16 root_node)
{
  clip.rootScale = V_C_UNIT_1110;
  bool valid = true;
  AnimV20Math::PrsAnimSampler<AnimV20Math::DefaultConfig> sampler(clip.animation);
  for (const AnimationClip::Point3Channel &nodeScale : clip.channelScale)
  {
    sampler.seekTicks(0);
    vec3f scale = sampler.sampleSclTrack(nodeScale.first);
    if (nodeScale.second == root_node)
      clip.rootScale = scale;
    else
      valid &= v_extract_x(v_length3_sq_x(v_sub(scale, V_C_ONE))) < 0.001;
    sampler.forEachSclKey([&valid, &nodeScale, scale](decltype(sampler) &smp, float time) {
      smp.seek(time);
      vec3f scale2 = smp.sampleSclTrack(nodeScale.first);
      valid &= v_extract_x(v_length3_sq_x(v_sub(scale, scale2))) < 0.001;
    });
  }
  if (!valid)
    logerr("animation <%s> has variable scale and won't work correctly with inertial blending", clip.name);
}

static void load_animation_tags(const AnimationDataBase &dataBase,
  const AnimV20::AnimData &anim_data,
  AnimationClip &clip,
  const DataBlock *clip_block,
  const char *default_tags)
{
  Tags currentTags = get_tags(dataBase.tags, clip_block->getStr("tags", default_tags));
  if (const DataBlock *tagRanges = clip_block->getBlockByName("tag_ranges"))
  {
    for (uint32_t i = 0, ie = tagRanges->blockCount(); i < ie; ++i)
    {
      const DataBlock *range = tagRanges->getBlock(i);
      uint32_t from = range->getInt("from");
      uint32_t to = range->getInt("to");
      Tags tags = get_tags(dataBase.tags, range->getStr("tags"));
      if (clip.intervals.empty() || clip.intervals.back().to == from)
        clip.intervals.emplace_back(AnimationInterval{from, to, tags});
      else
        logerr("MM: wrong tag_ranges in '%s' clip", clip_block->getBlockName());
    }
    if (clip.intervals.empty() || clip.intervals.back().to != clip.tickDuration)
      logerr("MM: wrong tag_ranges in '%s' clip", clip_block->getBlockName());
    return;
  }
  uint32_t from = 0;
  for (const auto &track : anim_data.dumpData.noteTrack)
  {
    if (strcmp(track.name.get(), "start") && strcmp(track.name.get(), "end"))
    {
      Tags newTags = get_tags(dataBase.tags, track.name);
      if (newTags.any())
      {
        uint32_t to = frame_duration_from_a2d_duration(track.time);
        clip.intervals.emplace_back(AnimationInterval{from, to, currentTags});
        currentTags ^= newTags;
        from = to;
      }
    }
  }
  uint32_t to = clip.tickDuration;
  clip.intervals.emplace_back(AnimationInterval{from, to, currentTags});
}

static quat4f get_default_rotation(const GeomNodeTree &tree, dag::Index16 node_id)
{
  const mat44f &tm = tree.getNodeWtmRel(node_id);
  vec3f translation;
  vec3f scale;
  quat4f rotation;
  v_mat4_decompose(tm, translation, rotation, scale);
  return rotation;
}

static void load_animation_root_motion_bones(const AnimationRootMotionConfig &config, AnimationDataBase &dataBase)
{
  const GeomNodeTree &tree = *dataBase.getReferenceSkeleton();
  dataBase.rootNode = tree.findNodeIndex(config.rootNodeName.c_str());
  G_ASSERTF(dataBase.rootNode, "Could not find node <%s>", config.rootNodeName.c_str());
  if (!dataBase.rootNode)
    dataBase.rootNode = dag::Index16(0);
  for (const auto &itr : config.rootMotionDirections)
  {
    if (itr.second <= 0)
      continue;
    auto nodeId = tree.findNodeIndex(itr.first.c_str());
    G_ASSERTF_CONTINUE(nodeId, "Could not find node <%s>", itr.first.c_str());
    dataBase.rootRotations[nodeId].weight = itr.second;
    dataBase.rootRotations[nodeId].defaultRotation = get_default_rotation(tree, nodeId);
  }
  for (const auto &itr : config.rootMotionCenterOfMasses)
  {
    if (itr.second.w <= 0)
      continue;
    auto nodeId = tree.findNodeIndex(itr.first.c_str());
    G_ASSERTF_CONTINUE(nodeId, "Could not find node <%s>", itr.first.c_str());
    dataBase.nodeCenterOfMasses[nodeId] = v_make_vec4f(itr.second.x, itr.second.y, itr.second.z, itr.second.w);
  }
}

static void calculate_root_motion_for_inplace_animation(AnimationClip &clip, const DataBlock &params)
{
  float velocityDirection = params.getReal("velocity_direction", 0);
  float velocityMagnitude = params.getReal("velocity_magnitude", 0);
  float rotationDelta = params.getReal("rotation_delta", 0);
  float velAngle = DegToRad(velocityDirection);
  vec3f velocity = v_make_vec4f(-sin(velAngle) * velocityMagnitude, 0, -cos(velAngle) * velocityMagnitude, 0);

  clip.rootMotion[0] = {v_zero(), V_C_UNIT_0001};
  for (int tick = 1; tick <= clip.tickDuration; tick++)
  {
    vec3f pos = v_mul(velocity, v_splats(clip.duration * tick / clip.tickDuration));
    float rotAngle = DegToRad(rotationDelta) * tick / clip.tickDuration;
    quat4f rot = v_quat_from_unit_vec_ang(V_C_UNIT_0100, v_splats(rotAngle));
    clip.rootMotion[tick] = {pos, rot};
  }
}

static bool load_root_motion_from_a2d_node(AnimationClip &clip, const eastl::string &a2d_node_name)
{
  if (a2d_node_name.empty())
    return false;
  auto rootMotionPos = find_anim_channel(a2d_node_name, clip.animation->anim.pos);
  auto rootMotionRot = find_anim_channel(a2d_node_name, clip.animation->anim.rot);
  if (!rootMotionPos || !rootMotionRot)
  {
    if (rootMotionPos)
      logerr("MM: can't find root rotation channel for '%s' node in '%s' clip (but position was found)", a2d_node_name, clip.name);
    if (rootMotionRot)
      logerr("MM: can't find root position channel for '%s' node in '%s' clip (but rotation was found)", a2d_node_name, clip.name);

    // NOTE: Not sure if it is good to logerr about missing node, maybe it was intentional like with
    // old inplace animation. Maybe in future it can be reworked to specify root motion a2d node name for
    // specific subsets of animations and logerr only when such node missing in those animations.
    if (!clip.inPlaceAnimation && !rootMotionPos && !rootMotionRot)
      logerr("MM: missed root motion node '%s' (in '%s' clip)", a2d_node_name, clip.name);
    return false;
  }

  AnimV20Math::PrsAnimNodeSampler<AnimV20Math::DefaultConfig> sampler(clip.animation, rootMotionRot);
  for (int tick = 0; tick <= clip.tickDuration; tick++)
  {
    sampler.seek(clip.duration * tick / clip.tickDuration);
    clip.rootMotion[tick] = {sampler.samplePos(), sampler.sampleRot()};
  }
  return true;
}

static void load_root_motion(const AnimationDataBase &dataBase, AnimationClip &clip, const DataBlock &clip_block)
{
  G_ASSERT_RETURN(clip.tickDuration, );
  int rootMotionSize = clip.tickDuration + 1;
  clip.rootMotion.resize(rootMotionSize);

  const DataBlock *inPlaceAnimParams = clip_block.getBlockByName("in_place_animation");
  clip.inPlaceAnimation = inPlaceAnimParams != nullptr;
  if (inPlaceAnimParams)
  {
    calculate_root_motion_for_inplace_animation(clip, *inPlaceAnimParams);
    return;
  }

  if (load_root_motion_from_a2d_node(clip, dataBase.rootMotionA2dNode))
    return;

  dag::Vector<vec3f, framemem_allocator> rootPositions(rootMotionSize, V_C_UNIT_0001);
  dag::Vector<vec3f, framemem_allocator> rootDirections(rootMotionSize, V_C_UNIT_0001);

  GeomNodeTree tree(*dataBase.getReferenceSkeleton()); // copy of tree for invalidate and calc Wtm
  mat44f m;
  v_mat44_ident(m);
  tree.setRootTm(m);
  NodeTSRFixedArray nodes(tree.nodeCount());

  for (int t = 0; t <= clip.tickDuration; t++)
  {
    float timeInSeconds = clip.duration * t / clip.tickDuration;
    set_identity(nodes, tree);
    sample_animation(timeInSeconds, clip, nodes);
    update_tree(nodes, tree);
    rootPositions[t] = get_root_translation(dataBase, tree);
    rootDirections[t] = get_root_direction(dataBase, tree);
  }
  if (clip_block.paramExists("root_direction"))
  {
    const Point3 rootDirectionOverride = clip_block.getPoint3("root_direction");
    vec3f rootDirectionVec = v_ldu_p3(&rootDirectionOverride.x);
    for (int t = 0; t <= clip.tickDuration; t++)
      rootDirections[t] = rootDirectionVec;
  }

  const DataBlock &rootMotionSmoothing = *clip_block.getBlockByNameEx("root_motion_smoothing");
  if (rootMotionSmoothing.getBool("enabled", true))
  {
    int polyOrder = rootMotionSmoothing.getInt("poly_order", 3);
    int posWindow = rootMotionSmoothing.getInt("position_filter_radius", 15);
    int dirWindow = rootMotionSmoothing.getInt("direction_filter_radius", 30);
    dag::Vector<vec3f, framemem_allocator> tempResult(rootMotionSize, V_C_UNIT_0001);
    if (rootMotionSize > posWindow * 2)
    {
      dag::Vector<vec4f> posWeights = savgol_weigths(posWindow, polyOrder);
      savgol_filter(posWeights.data(), rootPositions.data(), tempResult.data(), posWindow, rootMotionSize);
      eastl::swap(rootPositions, tempResult);
    }
    if (rootMotionSize > dirWindow * 2)
    {
      dag::Vector<vec4f> dirWeights = savgol_weigths(dirWindow, polyOrder);
      savgol_filter(dirWeights.data(), rootDirections.data(), tempResult.data(), dirWindow, rootMotionSize);
      eastl::swap(rootDirections, tempResult);
    }
  }

  for (int t = 0; t < rootMotionSize; t++)
  {
    rootDirections[t] = v_norm3_safe(rootDirections[t], FORWARD_DIRECTION); // normalize after savgol_filter
    quat4f q = v_quat_from_unit_arc(FORWARD_DIRECTION, rootDirections[t]);
    clip.rootMotion[t] = {rootPositions[t], q};
  }
}


static void load_animation_features(const AnimationDataBase &dataBase, AnimationClip &clip, const DataBlock &clip_block)
{
  GeomNodeTree tree(*dataBase.getReferenceSkeleton()); // copy of tree for invalidate and calc Wtm

  mat44f m;
  v_mat44_ident(m);
  tree.setRootTm(m);
  NodeTSRFixedArray nodes(tree.nodeCount());
  eastl::fixed_vector<dag::Index16, 8, false> nodeIndexes(dataBase.nodesName.size());
  for (int i = 0, n = dataBase.nodesName.size(); i < n; i++)
    nodeIndexes[i] = tree.findNodeIndex(dataBase.nodesName[i].c_str());

  // need to copy size/count to features for wrappers
  clip.features.init(clip.tickDuration, dataBase.nodeCount, dataBase.trajectorySize);
  dag::Vector<RootMotion, framemem_allocator> invRootMotions(clip.tickDuration);

  for (int t = 0; t < clip.tickDuration; t++)
  {
    float timeInSeconds = clip.duration * t / clip.tickDuration;
    set_identity(nodes, tree);
    sample_animation(timeInSeconds, clip, nodes);
    update_tree(nodes, tree);

    auto nodePositions = clip.features.get_node_positions(t);

    for (int i = 0, n = nodeIndexes.size(); i < n; i++)
    {
      if (auto nodeIdx = nodeIndexes[i])
      {
        nodePositions[i] = tree.getNodeWposRelScalar(nodeIdx);
      }
      else
      {
        logerr("no node %s in tree", dataBase.nodesName[i].c_str());
        nodePositions[i] = Point3(0, 0, 0);
      }
    }
  }

  for (int t = 0; t < clip.tickDuration; t++)
    invRootMotions[t] = clip.rootMotion[t].inv();

  for (int t = 0; t + 1 < clip.tickDuration; t++)
  {
    NodeFeature curFeature = clip.features.get_node_feature(t);
    NodeFeature nextFeature = clip.features.get_node_feature(t + 1);
    for (int i = 0, n = dataBase.nodesName.size(); i < n; i++)
    {
      // Calc velocity in world space and rotate to local frame[t] space
      Point3 vel = (nextFeature.nodePositions[i] - curFeature.nodePositions[i]) * TICKS_PER_SECOND;
      v_stu_p3(&curFeature.nodeVelocities[i].x, invRootMotions[t].transformDirection(v_ldu(&vel.x)));
    }
  }
  // copy last velocity from prev last
  if (clip.tickDuration > 1)
  {
    NodeFeature lastFeature = clip.features.get_node_feature(clip.tickDuration - 1);
    NodeFeature prevFeature = clip.features.get_node_feature(clip.tickDuration - 2);
    for (int i = 0, n = dataBase.nodesName.size(); i < n; i++)
    {
      lastFeature.nodeVelocities[i] = prevFeature.nodeVelocities[i];
    }
  }

  // translate nodes to local space
  {
    for (int t = 0; t < clip.tickDuration; t++)
    {
      auto nodePositions = clip.features.get_node_positions(t);
      for (int i = 0, n = dataBase.nodesName.size(); i < n; i++)
      {
        vec3f pos = v_ldu_p3(&nodePositions[i].x);
        pos = invRootMotions[t].transformPosition(pos);
        v_stu_p3(&nodePositions[i].x, pos);
      }
    }
  }

  const DataBlock *extrapolateTrajectory = clip_block.getBlockByName("extrapolate_trajectory");
  float fakeTrajectoryMultiplier = clip_block.getReal("fake_trajectory_multiplier", 1.0f);
  for (int i = 0; i < clip.tickDuration; i++)
  {
    const RootMotion &invTm = invRootMotions[i];
    TrajectoryFeature feature = clip.features.get_trajectory_feature(i);
    int k = 0;
    for (float timeDelta : dataBase.predictionTimes)
    {
      float predictedTime = i / TICKS_PER_SECOND + timeDelta * fakeTrajectoryMultiplier;
      float sampleTime = clamp(predictedTime, 0.f, clip.duration);
      if (predictedTime != sampleTime && clip.looped)
      {
        sampleTime = fmodf(predictedTime, clip.duration);
        if (sampleTime < 0)
          sampleTime += clip.duration;
      }
      RootMotion trajectorySample = lerp_root_motion(clip, sampleTime);
      vec3f trajectoryWorldPos = trajectorySample.translation;
      vec3f trajectoryWorldDir = trajectorySample.transformDirection(FORWARD_DIRECTION);
      if (predictedTime != sampleTime && clip.looped)
      {
        int cycles = roundf((predictedTime - sampleTime) / clip.duration);
        vec3f posDelta = v_sub(clip.rootMotion.back().translation, clip.rootMotion[0].translation);
        quat4f dirDelta = v_quat_mul_quat(clip.rootMotion.back().rotation, invRootMotions[0].rotation);
        if (cycles < 0)
        {
          cycles = -cycles;
          dirDelta = v_quat_conjugate(dirDelta);
          posDelta = v_quat_mul_vec3(dirDelta, v_neg(posDelta));
        }
        for (; cycles > 0; --cycles)
        {
          trajectoryWorldPos = v_add(v_quat_mul_vec3(dirDelta, trajectoryWorldPos), posDelta);
          trajectoryWorldDir = v_quat_mul_vec3(dirDelta, trajectoryWorldDir);
        }
      }
      else if (predictedTime != sampleTime && extrapolateTrajectory)
      {
        Point3 velocity;
        bool useBorderFrames = extrapolateTrajectory->getBool("use_border_frames", false);
        if (predictedTime < 0.f)
        {
          if (useBorderFrames)
          {
            vec3f deltaPos = v_sub(clip.rootMotion[1].translation, clip.rootMotion[0].translation);
            v_stu_p3(&velocity.x, v_mul(deltaPos, V_TICKS_PER_SECOND));
          }
          else
            velocity = extrapolateTrajectory->getPoint3("start_velocity", Point3::ZERO);
        }
        else
        {
          if (useBorderFrames)
          {
            int lastIdx = clip.rootMotion.size() - 1;
            vec3f deltaPos = v_sub(clip.rootMotion[lastIdx].translation, clip.rootMotion[lastIdx - 1].translation);
            v_stu_p3(&velocity.x, v_mul(deltaPos, V_TICKS_PER_SECOND));
          }
          else
            velocity = extrapolateTrajectory->getPoint3("end_velocity", Point3::ZERO);
        }
        float dt = predictedTime - sampleTime;
        trajectoryWorldPos = v_madd(v_ldu(&velocity.x), v_splats(dt), trajectoryWorldPos);
      }
      vec3f localP = invTm.transformPosition(trajectoryWorldPos);
      vec3f localV = invTm.transformDirection(trajectoryWorldDir);

      feature.rootPositions[k] = Point2(v_extract_x(localP), v_extract_z(localP));
      feature.rootDirections[k] = Point2(v_extract_x(localV), v_extract_z(localV));
      k++;
    }
  }
}

static void load_anim_tree_timer(const AnimationDataBase &dataBase, AnimationClip &clip, const DataBlock &clip_block)
{
  G_ASSERT(dataBase.referenceAnimGraph);
  clip.animTreeTimer = -1;
  clip.animTreeTimerCycle = 0.f;
  if (const DataBlock *timer = clip_block.getBlockByName("set_anim_tree_timer"))
  {
    // We can allow timers for non looped animations too, but currently it is not needed and not implemented
    G_ASSERT_RETURN(clip.looped, );
    const char *timerName = timer->getStr("timer_name", nullptr);
    if (timerName)
      clip.animTreeTimer = dataBase.referenceAnimGraph->getParamId(timerName, AnimV20::AnimCommonStateHolder::PT_TimeParam);
    clip.animTreeTimerCycle = timer->getReal("timer_cycle", 1.0f);
  }
}

void validate_clips_duplication(const AnimationDataBase &data_base, const char *clip_name)
{
  for (const AnimationClip &clip : data_base.clips)
    if (clip.name == clip_name)
    {
      char buf[2 << 10];
      logerr("MM: duplicated animation%s", dgs_get_fatal_context(buf, sizeof(buf)));
    }
}

static void load_animations(AnimationDataBase &dataBase, const DataBlock &node_masks, const eastl::string &path)
{
  FATAL_CONTEXT_AUTO_SCOPE(path.c_str());
  DataBlock source(path.c_str());
  const DataBlock *clipsBlock = source.getBlockByName("clips");
  if (!clipsBlock)
  {
    logerr("MM: no clips block in %s", path.c_str());
    return;
  }
  else if (source.blockCount() > 1 || source.paramCount() > 0)
  {
    logerr("MM: only single 'clips' block is supported (%s)", path.c_str());
  }
  const char *defaultMaskName = clipsBlock->getStr("default_animation_mask", nullptr);
  const char *defaultTags = clipsBlock->getStr("default_tags", nullptr);
  int featuresNormalizationGroup = 0;
  if (clipsBlock->getBool("separate_features_normalization", false))
  {
    featuresNormalizationGroup = dataBase.normalizationGroupsCount;
    dataBase.normalizationGroupsCount++;
  }
  auto &clips = dataBase.clips;
  clips.reserve(clips.size() + clipsBlock->blockCount());

  for (int i = 0, n = clipsBlock->blockCount(); i < n; i++)
  {
    const DataBlock *clipBlock = clipsBlock->getBlock(i);
    const char *clip_name = clipBlock->getBlockName();
    FATAL_CONTEXT_AUTO_SCOPE(clip_name);
#if DAGOR_DBGLEVEL > 0
    validate_clips_duplication(dataBase, clip_name);
#endif
    AnimV20::AnimData *animData = (AnimV20::AnimData *)get_game_resource_ex(((GameResHandle)(clip_name)), Anim2DataGameResClassId);
    if (animData)
    {
      AnimationClip &clip = clips.emplace_back();
      clip.name = clip_name;
      int a2dTicks = anim_max_time(animData->anim);
      if (clipBlock->paramExists("endLabel"))
        a2dTicks = min(a2dTicks, animData->getLabelTime(clipBlock->getStr("endLabel")));
      clip.tickDuration = a2dTicks * TICKS_PER_SECOND / AnimV20::TIME_TicksPerSec; // in ticks(30 per second)
      // one-frame, zero time animation
      clip.tickDuration = clip.tickDuration < 1 ? 1 : clip.tickDuration;
      clip.duration = clip.tickDuration / TICKS_PER_SECOND;
      clip.nextClipName = clipBlock->getStr("nextClip", "");
      clip.looped = clip.name == clip.nextClipName;
      clip.playSpeedMultiplierRange = clipBlock->getPoint2("play_speed_multiplier_range", Point2::ONE);
      clip.featuresNormalizationGroup = featuresNormalizationGroup;

      const char *maskName = clipBlock->getStr("animation_mask", defaultMaskName);
      const DataBlock *nodeMask = node_masks.getBlockByName(maskName);
      if (!nodeMask)
      {
        if (maskName)
          logerr("can't find node mask with '%s' name for '%s' clip", maskName, clip_name);
        else
          logerr("empty animation_mask for %s, add animation_mask in animation block or default_animation_mask in clips block",
            clip_name);
        continue;
      }
      clip.animation = animData;
      copy_animation(*animData, clip, *dataBase.getReferenceSkeleton(), *nodeMask);
      load_and_validate_root_scale(clip, dataBase.rootNode);
      load_root_motion(dataBase, clip, *clipBlock);
      load_foot_locker_states(dataBase, clip.footLockerStates, clip, *clipBlock);
      load_animation_features(dataBase, clip, *clipBlock);
      load_animation_tags(dataBase, *animData, clip, clipBlock, defaultTags);
      load_anim_tree_timer(dataBase, clip, *clipBlock);
    }
    else
    {
      logerr("MM: can't find '%s' animation", clip_name);
    }
  }
}

static void updade_trajectory_features_from_next_clip(
  AnimationClip &clip, const AnimationClip &next_clip, const AnimationDataBase &dataBase)
{
  for (int t = 0, te = dataBase.predictionTimes.size(); t < te; ++t)
  {
    float timeDelta = dataBase.predictionTimes[t];
    int frameDelta = timeDelta * TICKS_PER_SECOND;
    for (int frame = max(clip.tickDuration - frameDelta + 1, 0); frame < clip.tickDuration; ++frame)
    {
      TrajectoryFeature feature = clip.features.get_trajectory_feature(frame);
      RootMotion invRootMotion = clip.rootMotion[frame].inv();
      int nextClipFrame = min(frame + frameDelta - clip.tickDuration, next_clip.tickDuration);
      quat4f invNextClipRotation = v_quat_conjugate(next_clip.rootMotion[0].rotation);
      quat4f nextToCurRotation = v_quat_mul_quat(clip.rootMotion[clip.tickDuration].rotation, invNextClipRotation);
      vec3f posDelta = v_sub(next_clip.rootMotion[nextClipFrame].translation, next_clip.rootMotion[0].translation);
      vec3f trajectoryWorldPos = v_add(clip.rootMotion[clip.tickDuration].translation, v_quat_mul_vec3(nextToCurRotation, posDelta));
      vec3f trajectoryWorldDir =
        v_quat_mul_vec3(nextToCurRotation, next_clip.rootMotion[nextClipFrame].transformDirection(FORWARD_DIRECTION));

      vec3f localP = invRootMotion.transformPosition(trajectoryWorldPos);
      vec3f localV = invRootMotion.transformDirection(trajectoryWorldDir);

      feature.rootPositions[t] = Point2(v_extract_x(localP), v_extract_z(localP));
      feature.rootDirections[t] = Point2(v_extract_x(localV), v_extract_z(localV));
    }
  }
}

static void transform_pose_to_root_motion_space(NodeTSRFixedArray &pose, RootMotion root_tm, int root_node_id)
{
  quat4f invRot = v_quat_conjugate(root_tm.rotation);
  if (pose[root_node_id].changeBit & AnimV20::AnimBlender::RM_POS_B)
    pose[root_node_id].translation = v_quat_mul_vec3(invRot, v_sub(pose[root_node_id].translation, root_tm.translation));
  if (pose[root_node_id].changeBit & AnimV20::AnimBlender::RM_ROT_B)
    pose[root_node_id].rotation = v_norm4(v_quat_mul_quat(invRot, pose[root_node_id].rotation));
}

static void validate_next_clip_pose(const AnimationClip &prev_clip, const AnimationClip &next_clip, const AnimationDataBase &dataBase)
{
  const GeomNodeTree &tree = *dataBase.getReferenceSkeleton();
  int rootId = dataBase.rootNode.index();
  NodeTSRFixedArray lastPose(tree.nodeCount());
  set_identity(lastPose, tree);
  sample_animation(prev_clip.duration, prev_clip, lastPose);
  if (!prev_clip.inPlaceAnimation)
    transform_pose_to_root_motion_space(lastPose, lerp_root_motion(prev_clip, prev_clip.duration), rootId);
  NodeTSRFixedArray nextFirstPose(tree.nodeCount());
  set_identity(nextFirstPose, tree);
  sample_animation(0, next_clip, nextFirstPose);
  if (!next_clip.inPlaceAnimation)
    transform_pose_to_root_motion_space(nextFirstPose, lerp_root_motion(next_clip, 0), rootId);

  for (int i = 0, ie = lastPose.size(); i < ie; ++i)
  {
    if (prev_clip.nodeMask[i] == 0 || next_clip.nodeMask[i] == 0)
      continue;
    if (lastPose[i].changeBit != nextFirstPose[i].changeBit)
    {
      logerr("MM: mismatched channels in 'nextClip' animation. %s -> %s (%s) %c%c%c != %c%c%c", prev_clip.name, next_clip.name,
        tree.getNodeName(dag::Index16(i)), lastPose[i].changeBit & AnimV20::AnimBlender::RM_POS_B ? 'P' : '-',
        lastPose[i].changeBit & AnimV20::AnimBlender::RM_ROT_B ? 'R' : '-',
        lastPose[i].changeBit & AnimV20::AnimBlender::RM_SCL_B ? 'S' : '-',
        nextFirstPose[i].changeBit & AnimV20::AnimBlender::RM_POS_B ? 'P' : '-',
        nextFirstPose[i].changeBit & AnimV20::AnimBlender::RM_ROT_B ? 'R' : '-',
        nextFirstPose[i].changeBit & AnimV20::AnimBlender::RM_SCL_B ? 'S' : '-');
      continue;
    }
    vec4f maxDiff = v_splats(0.01);
    vec4f maxRelDiff = v_splats(0.1);
    if ((lastPose[i].changeBit & AnimV20::AnimBlender::RM_POS_B) &&
        v_test_all_bits_zeros(v_cmp_relative_equal(lastPose[i].translation, nextFirstPose[i].translation, maxDiff, maxRelDiff)))
      logerr("MM: different pos channel in 'nextClip' animation. %s -> %s (%s) " FMT_P3 " != " FMT_P3, prev_clip.name, next_clip.name,
        tree.getNodeName(dag::Index16(i)), V3D(lastPose[i].translation), V3D(nextFirstPose[i].translation));
    if ((lastPose[i].changeBit & AnimV20::AnimBlender::RM_ROT_B) &&
        v_test_all_bits_zeros(v_or(v_cmp_relative_equal(lastPose[i].rotation, nextFirstPose[i].rotation, maxDiff, maxRelDiff),
          v_cmp_relative_equal(lastPose[i].rotation, v_quat_conjugate(nextFirstPose[i].rotation), maxDiff, maxRelDiff))))
      logerr("MM: different rot channel in 'nextClip' animation. %s -> %s (%s) " FMT_P4 " != " FMT_P4, prev_clip.name, next_clip.name,
        tree.getNodeName(dag::Index16(i)), V4D(lastPose[i].rotation), V4D(nextFirstPose[i].rotation));
    if ((lastPose[i].changeBit & AnimV20::AnimBlender::RM_SCL_B) &&
        v_test_all_bits_zeros(v_cmp_relative_equal(lastPose[i].scale, nextFirstPose[i].scale, maxDiff, maxRelDiff)))
      logerr("MM: different scl channel in 'nextClip' animation. %s -> %s (%s) " FMT_P3 " != " FMT_P3, prev_clip.name, next_clip.name,
        tree.getNodeName(dag::Index16(i)), V3D(lastPose[i].scale), V3D(nextFirstPose[i].scale));
  }
}

static void resolve_next_clips(dag::Vector<AnimationClip> &clips, const AnimationDataBase &dataBase)
{
  for (int i = 0, n = clips.size(); i < n; i++)
  {
    AnimationClip &clip = clips[i];
    clip.nextClip = -1;
    auto it = eastl::find_if(clips.begin(), clips.end(), [&](const AnimationClip &other) { return other.name == clip.nextClipName; });
    if (it != clips.end())
    {
      validate_next_clip_pose(clip, *it, dataBase);
      clip.nextClip = it - clips.begin();
      if (!clip.looped)
        updade_trajectory_features_from_next_clip(clip, *it, dataBase);
    }
  }
}

AnimationRootMotionConfig load_root_motion_config(const ecs::string &root_node,
  const ecs::StringList &direction_nodes,
  const ecs::FloatList &direction_weights,
  const ecs::StringList &center_of_mass_nodes,
  const ecs::Point4List &center_of_mass_params)
{
  AnimationRootMotionConfig config;
  config.rootNodeName = root_node;
  G_ASSERT(direction_nodes.size() == direction_weights.size());
  G_ASSERT(center_of_mass_nodes.size() == center_of_mass_params.size());
  for (int i = 0; i < direction_nodes.size(); ++i)
    config.rootMotionDirections[direction_nodes[i]] = direction_weights[i];
  for (int i = 0; i < center_of_mass_nodes.size(); ++i)
    config.rootMotionCenterOfMasses[center_of_mass_nodes[i]] = center_of_mass_params[i];
  return config;
}

void load_animations(AnimationDataBase &dataBase,
  const AnimationRootMotionConfig &rootMotionConfig,
  const DataBlock &node_masks,
  const ecs::StringList &clips_path)
{
  G_ASSERT_RETURN(dataBase.getReferenceSkeleton(), );
  auto refTime = ref_time_ticks();
  dataBase.clips.clear();

  load_animation_root_motion_bones(rootMotionConfig, dataBase);
  dataBase.defaultCenterOfMass = extract_center_of_mass(dataBase, *dataBase.getReferenceSkeleton());

  dataBase.normalizationGroupsCount = 1;
  for (const ecs::string &path : clips_path)
    load_animations(dataBase, node_masks, path);
  dataBase.playOnlyFromStartTag = dataBase.tags.getNameId("play_only_from_start");
  resolve_next_clips(dataBase.clips, dataBase);
  calculate_normalization(dataBase.clips, dataBase.featuresAvg, dataBase.featuresStd, dataBase.featuresSize,
    dataBase.normalizationGroupsCount);
  calculate_acceleration_struct(dataBase.clips, dataBase.featuresSize);

  debug("[MM] loading motion matching data base in %d ms", get_time_usec(refTime) / 1000);
}
