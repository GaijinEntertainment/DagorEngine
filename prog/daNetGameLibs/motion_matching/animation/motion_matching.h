// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "animation_data_base.h"


struct MatchingResult
{
  int clip, frame;
  float metrica;
};

inline bool is_same(const MatchingResult &a, const MatchingResult &b) { return a.clip == b.clip && a.frame == b.frame; }

MatchingResult motion_matching(const AnimationDataBase &dataBase,
  MatchingResult current_state,
  const AnimationFilterTags &current_tags,
  const FeatureWeights &weights,
  dag::ConstSpan<FrameFeaturesData::value_type> current_feature);

MatchingResult motion_matching_brute_force(const AnimationDataBase &dataBase,
  MatchingResult current_state,
  const AnimationFilterTags &current_tags,
  const FeatureWeights &weights,
  dag::ConstSpan<FrameFeaturesData::value_type> current_feature);
