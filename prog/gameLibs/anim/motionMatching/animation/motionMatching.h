// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include "animationDataBase.h"


struct MatchingResult
{
  int clip, frame;
  float metrica;
};

using MotionMatchingSearchFeatures = dag::Vector<vec4f, framemem_allocator>;
void prepare_features_for_mm_search(const AnimationDataBase &data_base, const FrameFeatures &denormalized_features_in,
  MotionMatchingSearchFeatures &normalized_search_features_out);

MatchingResult motion_matching(const AnimationDataBase &dataBase, const MatchingResult &current_state,
  const AnimationFilterTags &current_tags, const FeatureWeights &weights, const MotionMatchingSearchFeatures &current_feature);

MatchingResult motion_matching_brute_force(const AnimationDataBase &dataBase, const MatchingResult &current_state,
  const AnimationFilterTags &current_tags, const FeatureWeights &weights, const MotionMatchingSearchFeatures &current_feature);

float calculate_trajectory_features_cost(const AnimationDataBase &dataBase, int clip_idx, int frame_idx, const FeatureWeights &weights,
  const MotionMatchingSearchFeatures &features);
float calculate_features_cost(const AnimationDataBase &dataBase, int clip_idx, int frame_idx, const FeatureWeights &weights,
  const MotionMatchingSearchFeatures &features);
void calculate_features_cost_detailed(const AnimationDataBase &dataBase, int clip_idx, int frame_idx, const FeatureWeights &weights,
  const MotionMatchingSearchFeatures &features, float &total_cost, float &trajectory_cost, float &pose_cost);
