// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <anim/dag_simpleNodeAnim.h>
#include <anim/dag_animKeyInterp.h>
#include <math/dag_quatInterp.h>
#include <debug/dag_debug.h>
#include <limits.h>

bool SimpleNodeAnim::init(AnimV20::AnimData *a, const char *node_name)
{
  anim = a;
  return setTargetNode(node_name);
}

bool SimpleNodeAnim::setTargetNode(const char *node_name)
{
  if (!anim.get())
    return false;

  prs = anim->getPrsAnim(node_name);

  return isValid();
}

void SimpleNodeAnim::calcTimeMinMax(int &t_min, int &t_max)
{
  if (!prs.trackId)
  {
    t_min = INT_MAX;
    t_max = INT_MIN;
    return;
  }

  t_min = prs.keyTimeFirst();
  t_max = prs.keyTimeLast();
}

void SimpleNodeAnim::calcAnimTm(TMatrix &tm, int t)
{
  vec3f p, s;
  quat4f r;

  AnimV20Math::PrsAnimNodeSampler<AnimV20Math::OneShotConfig> sampler(prs, t);
  sampler.sampleTransform(&p, &r, &s);

  tm = AnimV20Math::makeTM((Point3 &)p, (Quat &)r, (Point3 &)s);
}
