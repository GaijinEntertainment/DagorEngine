// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/componentTypes.h>
#include <daECS/core/dataComponent.h>
#include <daECS/core/entityManager.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include "animation_data_base.h"

#define EXTRA_LOG 0 // for development usage

#if EXTRA_LOG
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_Point4.h>
#endif

bool AnimationDataBase::hasAvailableAnimations(const AnimationFilterTags &filter_tags) const
{
  for (const AnimationClip &clip : clips)
    for (const AnimationInterval &interval : clip.intervals)
      if (filter_tags.isMatch(interval.tags))
        return true;
  return false;
}

void AnimationDataBase::changePBCWeightOverride(int pbc_id, float weight)
{
  bool found = false;
  for (eastl::pair<int, float> &pair : pbcWeightOverrides)
    if (pair.first == pbc_id)
    {
      found = true;
      pair.second = weight;
      break;
    }
  if (!found)
    pbcWeightOverrides.emplace_back(pbc_id, weight);
}

void AnimationDataBase::requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
{
  for (const ecs::string &path : res_cb.get<ecs::StringList>(ECS_HASH("data_bases_paths")))
  {
    DataBlock source(path.c_str());
    const DataBlock *clipsBlock = source.getBlockByName("clips");
    if (!clipsBlock)
      continue; // there is informative error catch in actual animations loading
    for (int i = 0, n = clipsBlock->blockCount(); i < n; i++)
    {
      const char *clip_name = clipsBlock->getBlock(i)->getBlockName();
#if DAGOR_DBGLEVEL > 0
      // Do not preload missing animations because it will fail entity creation.
      // There will be logerr about missing animations on MM database loading.
      if (get_game_resource_pack_id_by_resource((GameResHandle)clip_name) < 0)
        continue;
#endif
      res_cb(clip_name, Anim2DataGameResClassId);
    }
  }
}
void calculate_acceleration_struct(dag::Vector<AnimationClip> &clips, int features)
{
#if EXTRA_LOG
  dag::Vector<vec4f> globalMax1(features, V_C_MIN_VAL);
  dag::Vector<vec4f> globalMin1(features, V_C_MAX_VAL);
  dag::Vector<vec4f> globalMax2(features, V_C_MIN_VAL);
  dag::Vector<vec4f> globalMin2(features, V_C_MAX_VAL);
#endif
  int totalCount = 0;

  for (AnimationClip &clip : clips)
  {
    int frames = clip.features.data.size() / features;
    int boundsSmallN = ((frames + BOUNDS_SMALL_SIZE - 1) / BOUNDS_SMALL_SIZE) * features;
    int boundsLargeN = ((frames + BOUNDS_LARGE_SIZE - 1) / BOUNDS_LARGE_SIZE) * features;
    clip.boundsSmallMax.assign(boundsSmallN, V_C_MIN_VAL);
    clip.boundsSmallMin.assign(boundsSmallN, V_C_MAX_VAL);
    clip.boundsLargeMax.assign(boundsLargeN, V_C_MIN_VAL);
    clip.boundsLargeMin.assign(boundsLargeN, V_C_MAX_VAL);

    int frame = 0;
    for (auto from = clip.features.data.begin(), to = clip.features.data.end(); from < to; from += features, frame++)
    {
      int smallIdx = (frame / BOUNDS_SMALL_SIZE) * features;
      int largeIdx = (frame / BOUNDS_LARGE_SIZE) * features;
      totalCount++;
      for (int i = 0, small_i = smallIdx, large_i = largeIdx; i < features; i++, small_i++, large_i++)
      {
        clip.boundsSmallMax[small_i] = v_max(clip.boundsSmallMax[small_i], from[i]);
        clip.boundsSmallMin[small_i] = v_min(clip.boundsSmallMin[small_i], from[i]);
        clip.boundsLargeMax[large_i] = v_max(clip.boundsLargeMax[large_i], from[i]);
        clip.boundsLargeMin[large_i] = v_min(clip.boundsLargeMin[large_i], from[i]);
#if EXTRA_LOG
        globalMax1[i] = v_max(globalMax1[i], clip.boundsSmallMax[small_i]);
        globalMin1[i] = v_min(globalMin1[i], clip.boundsSmallMin[small_i]);
        globalMax2[i] = v_max(globalMax2[i], clip.boundsLargeMax[large_i]);
        globalMin2[i] = v_min(globalMin2[i], clip.boundsLargeMin[large_i]);
#endif
      }
    }
  }
  debug("[MM] total frame count %d, total clips %d", totalCount, clips.size());
#if EXTRA_LOG
  auto log = [](const char *name, const dag::Vector<vec4f> &v) {
    for (vec4f elem : v)
    {
      debug("[MM] %s %@", name, elem);
    }
  };
  log("maxSmall", globalMax1);
  log("minSmall", globalMin1);
  log("maxLarge", globalMax2);
  log("minLarge", globalMin2);
#endif
}


ECS_REGISTER_BOXED_TYPE(AnimationDataBase, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(FrameFeatures, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(AnimationFilterTags, nullptr);
