// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animHelpers/animCharHelper.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlend.h>
#include <anim/dag_animBlendCtrl.h>
#include <debug/dag_log.h>

AnimCharHelper::AnimCharHelper() : animChar(NULL) {}

AnimCharHelper::~AnimCharHelper() = default;

void AnimCharHelper::init(AnimV20::IAnimCharacter2 *anim_char, const char *root_name)
{
  animChar = anim_char;
  if (!animChar)
    return;

  AnimV20::AnimationGraph *ag = animChar->getAnimGraph();
  AnimV20::IAnimBlendNode *n = ag->getBlendNodePtr(root_name);
  root = n && n->isSubOf(AnimV20::AnimBlendCtrl_Fifo3CID) ? (AnimV20::AnimBlendCtrl_Fifo3 *)n : nullptr;
  if (!root)
    logerr("animChar %s: no Fifo3 blend node <%s>", animChar->getCreateInfo()->resName.c_str(), root_name);
  clear_and_shrink(bnls);
  animNames.reset();
}

int AnimCharHelper::registerAnim(const char *name)
{
  if (!animChar)
    return -1;
  int nameId = animNames.getNameId(name);
  if (nameId >= 0)
    return nameId; // Already registered

  AnimV20::AnimationGraph *ag = animChar->getAnimGraph();
  AnimV20::IAnimBlendNode *node = ag->getBlendNodePtr(name);
  if (!node)
    return -1;
  nameId = animNames.addNameId(name);
  G_ASSERT(nameId == bnls.size());
  bnls.push_back(node);
  return nameId;
}

void AnimCharHelper::playAnim(int anim_id)
{
  if (anim_id < 0 || !animChar || !root)
    return;

  bnls[anim_id]->seek(*animChar->getAnimState(), 0.f);
  bnls[anim_id]->resume(*animChar->getAnimState(), true);
  root->enqueueState(*animChar->getAnimState(), bnls[anim_id], 0.05f);
}
