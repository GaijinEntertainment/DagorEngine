// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animation_clip.h"
#include "feature_normalization.h"

void calculate_normalization(dag::Vector<AnimationClip> &clips, dag::Vector<vec3f> &avg, dag::Vector<vec3f> &std, int features)
{
  avg.assign(features, v_zero());
  std.assign(features, v_zero());
  int totalCount = 0;
  for (const AnimationClip &clip : clips)
    totalCount += clip.tickDuration;
  if (totalCount == 0)
    return;
  vec3f nInv = v_splats(1.f / totalCount);

  for (const AnimationClip &clip : clips)
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
      for (int i = 0; i < features; i++)
      {
        // divide feature on totalCount in cycle to reduce float error
        avg[i] = v_add(avg[i], v_mul(from[i], nInv));
      }

  for (const AnimationClip &clip : clips)
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
      for (int i = 0; i < features; i++)
      {
        // divide feature on totalCount in cycle to reduce float error
        vec3f dif = v_sub(from[i], avg[i]);
        std[i] = v_add(std[i], v_mul(v_mul(dif, dif), nInv));
      }

  for (int i = 0; i < features; i++)
    std[i] = v_sel(v_sqrt(std[i]), V_C_ONE, v_sub(std[i], V_C_EPS_VAL));

  G_ASSERT(!clips.empty());
  // We need to know features layout to extract span-like features, any clip is sufficient for us because offsets are same everywhere
  FrameFeatures featuresGetter;
  featuresGetter.nodeCount = clips[0].features.nodeCount;
  featuresGetter.trajectoryPointCount = clips[0].features.trajectoryPointCount;
  featuresGetter.trajectorySizeInVec4f = clips[0].features.trajectorySizeInVec4f;
  featuresGetter.data = eastl::move(std);
  auto trajectoryStd = featuresGetter.get_trajectory_feature(0);

  // Compute overall std per feature as average std across all dimensions
  float trajectoryPosStd = 0.0;
  for (Point2 p : trajectoryStd.rootPositions)
    trajectoryPosStd += (p.x + p.y) * 0.5;
  trajectoryPosStd /= trajectoryStd.rootPositions.size();
  for (Point2 &p : trajectoryStd.rootPositions)
    p.x = p.y = trajectoryPosStd;

  float trajectoryDirStd = 0.0;
  for (Point2 p : trajectoryStd.rootDirections)
    trajectoryDirStd += (p.x + p.y) * 0.5;
  trajectoryDirStd /= trajectoryStd.rootDirections.size();
  for (Point2 &p : trajectoryStd.rootDirections)
    p.x = p.y = trajectoryDirStd;

  for (Point3 &p : featuresGetter.get_node_positions(0))
  {
    float nodePosStd = (p.x + p.y + p.z) / 3.0;
    p.x = p.y = p.z = nodePosStd;
  }

  for (Point3 &p : featuresGetter.get_node_velocities(0))
  {
    float nodeVelStd = (p.x + p.y + p.z) / 3.0;
    p.x = p.y = p.z = nodeVelStd;
  }

  std = eastl::move(featuresGetter.data);

  // normalize_dataset
  for (AnimationClip &clip : clips)
  {
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
    {
      normalize_feature(make_span(from, features), avg, std);
    }
  }
}

void normalize_feature(dag::Span<vec4f> features, const dag::Vector<vec4f> &avg, const dag::Vector<vec4f> &std)
{
  for (int i = 0, n = features.size(); i < n; i++)
    features[i] = v_safediv(v_sub(features[i], avg[i]), std[i]);
}

void denormalize_feature(dag::Span<vec4f> features, const dag::Vector<vec4f> &avg, const dag::Vector<vec4f> &std)
{
  for (int i = 0, n = features.size(); i < n; i++)
    features[i] = v_add(v_mul(features[i], std[i]), avg[i]);
}
