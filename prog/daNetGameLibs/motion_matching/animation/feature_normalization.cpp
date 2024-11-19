// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animation_clip.h"
#include "feature_normalization.h"
#include <memory/dag_framemem.h>

void calculate_normalization(
  dag::Vector<AnimationClip> &clips, dag::Vector<vec3f> &avg, dag::Vector<vec3f> &std, int features, int normalization_groups)
{
  avg.assign(features * normalization_groups, v_zero());
  std.assign(features * normalization_groups, v_zero());
  dag::Vector<int, framemem_allocator> totalCountPerGroup(normalization_groups, 0);
  dag::Vector<vec3f, framemem_allocator> nInvArray(normalization_groups, v_zero());

  for (const AnimationClip &clip : clips)
    totalCountPerGroup[clip.featuresNormalizationGroup] += clip.tickDuration;

  for (int i = 0; i < normalization_groups; ++i)
    if (totalCountPerGroup[i] > 0)
      nInvArray[i] = v_splats(1.f / totalCountPerGroup[i]);

  for (const AnimationClip &clip : clips)
  {
    int featuresOffset = clip.featuresNormalizationGroup * features;
    vec3f nInv = nInvArray[clip.featuresNormalizationGroup];
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
      for (int i = 0; i < features; i++)
      {
        // divide feature on totalCount in cycle to reduce float error
        avg[featuresOffset + i] = v_add(avg[featuresOffset + i], v_mul(from[i], nInv));
      }
  }

  for (const AnimationClip &clip : clips)
  {
    int featuresOffset = clip.featuresNormalizationGroup * features;
    vec3f nInv = nInvArray[clip.featuresNormalizationGroup];
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
      for (int i = 0; i < features; i++)
      {
        // divide feature on totalCount in cycle to reduce float error
        vec3f dif = v_sub(from[i], avg[featuresOffset + i]);
        std[featuresOffset + i] = v_add(std[featuresOffset + i], v_mul(v_mul(dif, dif), nInv));
      }
  }

  for (int i = 0, n = std.size(); i < n; i++)
    std[i] = v_sel(v_sqrt(std[i]), V_C_ONE, v_sub(std[i], V_C_EPS_VAL));

  G_ASSERT(!clips.empty());
  // We need to know features layout to extract span-like features, any clip is sufficient for us because offsets are same everywhere
  FrameFeatures featuresGetter;
  featuresGetter.nodeCount = clips[0].features.nodeCount;
  featuresGetter.trajectoryPointCount = clips[0].features.trajectoryPointCount;
  featuresGetter.trajectorySizeInVec4f = clips[0].features.trajectorySizeInVec4f;
  featuresGetter.data = eastl::move(std);
  for (int group = 0; group < normalization_groups; ++group)
  {
    auto trajectoryStd = featuresGetter.get_trajectory_feature(group);

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

    for (Point3 &p : featuresGetter.get_node_positions(group))
    {
      float nodePosStd = (p.x + p.y + p.z) / 3.0;
      p.x = p.y = p.z = nodePosStd;
    }

    for (Point3 &p : featuresGetter.get_node_velocities(group))
    {
      float nodeVelStd = (p.x + p.y + p.z) / 3.0;
      p.x = p.y = p.z = nodeVelStd;
    }
  }

  std = eastl::move(featuresGetter.data);

  // normalize_dataset
  for (AnimationClip &clip : clips)
  {
    int featuresOffset = clip.featuresNormalizationGroup * features;
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features)
    {
      normalize_feature(make_span(from, features), avg.data() + featuresOffset, std.data() + featuresOffset);
    }
  }
}

void normalize_feature(dag::Span<vec4f> dst_features, const vec4f *src_features, const vec4f *avg, const vec4f *std)
{
  for (int i = 0, n = dst_features.size(); i < n; i++)
    dst_features[i] = v_safediv(v_sub(src_features[i], avg[i]), std[i]);
}

void denormalize_feature(dag::Span<vec4f> features, const vec4f *avg, const vec4f *std)
{
  for (int i = 0, n = features.size(); i < n; i++)
    features[i] = v_add(v_mul(features[i], std[i]), avg[i]);
}
