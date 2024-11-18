// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include "motion_matching.h"
#include "motion_matching_metrics.h"
#include "motion_matching_tags.h"

// No need to jump on last frames in animation or specific interval with tags because it ends
// very soon and can cause infinite cycling between 2 last frames of different animations
constexpr int IGNORE_LAST_FRAMES = 5;

constexpr int IGNORE_SURROUNDING_FRAMES = 10;
static inline bool is_transition_allowed(int cur_clip_idx, int target_clip_idx, int cur_frame, int target_frame)
{
  return cur_clip_idx != target_clip_idx || abs(target_frame - cur_frame) > IGNORE_SURROUNDING_FRAMES;
}

MatchingResult motion_matching(const AnimationDataBase &dataBase,
  MatchingResult current_state,
  bool use_brute_force,
  const AnimationFilterTags &current_tags,
  const FeatureWeights &weights,
  dag::ConstSpan<FrameFeaturesData::value_type> current_feature)
{
  int featureSize = dataBase.featuresSize;

  G_ASSERT(featureSize == weights.featureWeights.size());
  G_ASSERT(featureSize * dataBase.normalizationGroupsCount == current_feature.size());
  const vec4f *featureWeightsPtr = weights.featureWeights.data();

  int cur_clip = current_state.clip;
  int cur_frame = current_state.frame;

  vec4f bestResult = v_splats(current_state.metrica);
  int best_clip = cur_clip;
  int best_frame = cur_frame;

  for (int next_clip = 0, clip_count = dataBase.clips.size(); next_clip < clip_count; next_clip++)
  {
    const AnimationClip &nextClip = dataBase.clips[next_clip];
    const auto &smallBoundsMin = nextClip.boundsSmallMin;
    const auto &smallBoundsMax = nextClip.boundsSmallMax;
    const auto &largeBoundsMin = nextClip.boundsLargeMin;
    const auto &largeBoundsMax = nextClip.boundsLargeMax;
    int featuresOffset = nextClip.featuresNormalizationGroup * featureSize;
    const vec3f *goalFeaturePtr = current_feature.data() + featuresOffset;

    G_ASSERT(nextClip.features.data.size() == nextClip.tickDuration * featureSize);

    for (const AnimationInterval &interval : nextClip.intervals)
    {
      if (!current_tags.isMatch(interval.tags))
        continue;

      const vec3f *nextFeaturePtr = nextClip.features.data.data() + interval.from * featureSize;

      int i = interval.from;
      int searchEnd = interval.to == nextClip.tickDuration && nextClip.looped ? interval.to : interval.to - IGNORE_LAST_FRAMES;
      if (next_clip == cur_clip && nextClip.looped)
      {
        i = max(i, cur_frame + IGNORE_SURROUNDING_FRAMES - nextClip.tickDuration);
        searchEnd = min(searchEnd, cur_frame + nextClip.tickDuration - IGNORE_SURROUNDING_FRAMES);
      }
      if (dataBase.playOnlyFromStartTag >= 0 && interval.tags.test(dataBase.playOnlyFromStartTag))
        searchEnd = interval.from + 1;

      if (use_brute_force)
      {
        while (i < searchEnd)
        {
          if (is_transition_allowed(cur_clip, next_clip, cur_frame, i) &&
              feature_metric_need_update(bestResult, goalFeaturePtr, nextFeaturePtr, featureWeightsPtr, featureSize))
          {
            best_clip = next_clip;
            best_frame = i;
          }
          i++;
          nextFeaturePtr += featureSize;
        }
      }
      else
      {
        while (i < searchEnd)
        {

          int largeBoundIdx = i / BOUNDS_LARGE_SIZE;
          int largeBoundIdxNext = (largeBoundIdx + 1) * BOUNDS_LARGE_SIZE;
          const vec4f *minBounds = largeBoundsMin.data() + largeBoundIdx * featureSize;
          const vec4f *maxBounds = largeBoundsMax.data() + largeBoundIdx * featureSize;
          if (!feature_bounded_metric_need_update(bestResult, goalFeaturePtr, minBounds, maxBounds, featureWeightsPtr, featureSize))
          {
            i = largeBoundIdxNext;
            nextFeaturePtr = nextClip.features.data.data() + i * featureSize;
            continue;
          }
          while (i < largeBoundIdxNext && i < searchEnd)
          {
            int smallBoundIdx = i / BOUNDS_SMALL_SIZE;
            int smallBoundIdxNext = (smallBoundIdx + 1) * BOUNDS_SMALL_SIZE;
            const vec4f *minBounds = smallBoundsMin.data() + smallBoundIdx * featureSize;
            const vec4f *maxBounds = smallBoundsMax.data() + smallBoundIdx * featureSize;
            if (!feature_bounded_metric_need_update(bestResult, goalFeaturePtr, minBounds, maxBounds, featureWeightsPtr, featureSize))
            {
              i = smallBoundIdxNext;
              nextFeaturePtr = nextClip.features.data.data() + i * featureSize;
              continue;
            }

            if (is_transition_allowed(cur_clip, next_clip, cur_frame, i) &&
                feature_metric_need_update(bestResult, goalFeaturePtr, nextFeaturePtr, featureWeightsPtr, featureSize))
            {
              best_clip = next_clip;
              best_frame = i;
            }
            i++;
            nextFeaturePtr += featureSize;
          }
        }
      }
    }
  }

  return MatchingResult{best_clip, best_frame, v_extract_x(bestResult)};
}
