// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "motion_matching_animgraph_nodes.h"
#include <anim/dag_animBlendCtrl.h>

constexpr uint32_t MM_TAG_NEGATION_BIT = 1 << 31;

void load_motion_matching_animgraph_nodes(AnimationDataBase &data_base, AnimV20::AnimationGraph *anim_graph)
{
  data_base.animGraphNodeTagsRemap.clear();
  G_ASSERT(anim_graph);
  data_base.referenceAnimGraph = anim_graph;
  for (int i = 0, ie = anim_graph->getAnimNodeCount(); i < ie; ++i)
  {
    AnimV20::IAnimBlendNode *node = anim_graph->getBlendNodePtr(i);
    if (!node->isSubOf(AnimV20::AnimBlendCtrl_SetMotionMatchingTagCID))
      continue;
    AnimV20::AnimBlendCtrl_SetMotionMatchingTag *mmTagNode = static_cast<AnimV20::AnimBlendCtrl_SetMotionMatchingTag *>(node);
    const char *tagName = mmTagNode->getTagName();
    bool tagNegation = false;
    if (tagName[0] == '!')
    {
      tagName++;
      tagNegation = true;
    }
    int tagIdx = data_base.getTagIdx(tagName);
    if (tagIdx < 0)
    {
      logerr("MM: unknown tag '%s' in animTree node '%s', add this tag to `main_database__availableTags` "
             "ecs component if you need it",
        tagName, anim_graph->getBlendNodeName(mmTagNode));
      continue;
    }
    if (tagIdx >= AnimV20::AnimBlendCtrl_SetMotionMatchingTag::MAX_TAGS_COUNT)
    {
      logerr("MM: max number of tags in animGraph reached");
      continue;
    }
    mmTagNode->setTagIdx(data_base.animGraphNodeTagsRemap.size());
    data_base.animGraphNodeTagsRemap.push_back((tagNegation ? MM_TAG_NEGATION_BIT : 0) | tagIdx);
  }
  data_base.animGraphTagsParamId = anim_graph->getParamId("motion_matching_tags");
}

AnimationFilterTags get_mm_tags_from_animgraph(const AnimationDataBase &data_base, const AnimV20::IAnimStateHolder &st)
{
  AnimationFilterTags combinedTags;
  const uint32_t *activeAnimGraphTags = static_cast<const uint32_t *>(st.getInlinePtr(data_base.animGraphTagsParamId));
  for (uint32_t nodeIdBase = 0, nodesCount = data_base.animGraphNodeTagsRemap.size(); nodeIdBase < nodesCount; nodeIdBase += 32)
  {
    uint32_t mask = *activeAnimGraphTags++;
    while (mask)
    {
      uint32_t nodeIdOffset = __bsf(mask);
      uint32_t tagIdx = data_base.animGraphNodeTagsRemap[nodeIdBase + nodeIdOffset];
      if (tagIdx & MM_TAG_NEGATION_BIT)
        combinedTags.excludeTag(tagIdx & ~MM_TAG_NEGATION_BIT);
      else
        combinedTags.requireTag(tagIdx);
      mask ^= 1 << nodeIdOffset;
    }
  }
  return combinedTags;
}

void reset_animgraph_tags(const AnimationDataBase &data_base, AnimV20::IAnimStateHolder &st)
{
  if (data_base.animGraphNodeTagsRemap.empty())
    return;
  size_t tagsMaskSize = (data_base.animGraphNodeTagsRemap.size() + 7) / 8;
  memset(st.getInlinePtr(data_base.animGraphTagsParamId), 0, tagsMaskSize);
}
