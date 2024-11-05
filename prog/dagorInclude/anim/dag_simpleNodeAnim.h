//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animChannels.h>


class SimpleNodeAnim
{
public:
  SimpleNodeAnim() {} //-V730

  bool init(AnimV20::AnimData *a, const char *node_name);

  void calcAnimTm(TMatrix &tm, int t, int d_keys_no_blend = -1);

  bool isValid() const { return anim.get() && (pos || rot || scl); }

  bool setTargetNode(const char *node_name);

  void calcTimeMinMax(int &t_min, int &t_max);

protected:
  Ptr<AnimV20::AnimData> anim;
  const AnimV20::AnimChanPoint3 *pos;
  const AnimV20::AnimChanQuat *rot;
  const AnimV20::AnimChanPoint3 *scl;
};
