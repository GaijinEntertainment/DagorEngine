// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gamePhys/phys/animatedPhys.h>
#include <gamePhys/phys/physVars.h>
#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animBlend.h>

inline void AnimatedPhys::addRemap(int animNid, int physNid, bool pullable)
{
#ifdef _DEBUG_TAB_
  G_ASSERT(animNid >= -1 && animNid < SHRT_MAX);
  G_ASSERT((unsigned)physNid < remappedVars.size());
#else
  G_FAST_ASSERT(animNid >= -1 && animNid < SHRT_MAX);
  G_FAST_ASSERT((unsigned)physNid < remappedVars.size());
#endif
  remappedVars[physNid] = int16_t(animNid);
  if (DAGOR_UNLIKELY(pullable))
    pullBitmap.set(physNid, true);
}

void AnimatedPhys::init(const AnimV20::AnimcharBaseComponent &anim_char, const PhysVars &phys_vars)
{
  const AnimV20::AnimationGraph *animGraph = anim_char.getAnimGraph();
  if (!animGraph || !phys_vars.getVarsCount())
    return;
  G_FAST_ASSERT(anim_char.getAnimState());
  remappedVars.clear();
  pullBitmap.clear();
  remappedVars.resize(phys_vars.getVarsCount(), -1);
  phys_vars.iter(
    [&](const char *name, int var_id) { addRemap(animGraph->getParamId(name), var_id, phys_vars.isVarPullable(var_id)); });
}

void AnimatedPhys::appendVar(const char *var_name, const AnimV20::AnimcharBaseComponent &anim_char, const PhysVars &phys_vars)
{
  const AnimV20::AnimationGraph *animGraph = anim_char.getAnimGraph();
  if (!animGraph)
    return;
  G_FAST_ASSERT(anim_char.getAnimState());
  int physNid = phys_vars.getVarId(var_name);
  if (physNid >= 0)
  {
    bool append = true;
    if (physNid >= remappedVars.size())
    {
      append = physNid == remappedVars.size();
      remappedVars.resize(physNid + 1, -1);
    }
    if (append)
      addRemap(animGraph->getParamId(var_name), physNid, phys_vars.isVarPullable(physNid));
    else
      init(anim_char, phys_vars);
  }
}

void AnimatedPhys::update(AnimV20::AnimcharBaseComponent &anim_char, PhysVars &phys_vars)
{
  AnimV20::IAnimStateHolder *animState = anim_char.getAnimState();
  if (DAGOR_UNLIKELY(remappedVars.size() != phys_vars.getVarsCount()))
    init(anim_char, phys_vars);
  if (pullBitmap.empty())
  {
    for (int i = 0, n = remappedVars.size(); i < n; ++i)
      if (remappedVars[i] >= 0)
        animState->setParam(remappedVars[i], phys_vars.getVarUnsafe(i));
  }
  else
  {
    for (int i = 0, n = remappedVars.size(); i < n; ++i)
      if (remappedVars[i] >= 0)
      {
        if (!pullBitmap.test(i, false))
          animState->setParam(remappedVars[i], phys_vars.getVarUnsafe(i));
        else
          phys_vars.setVar(i, animState->getParam(remappedVars[i]));
      }
  }
}
