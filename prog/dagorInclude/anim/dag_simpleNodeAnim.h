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

  void calcAnimTm(TMatrix &tm, int t);

  bool isValid() const { return anim.get() && prs.valid(); }

  bool setTargetNode(const char *node_name);

  void calcTimeMinMax(int &t_min, int &t_max);

protected:
  Ptr<AnimV20::AnimData> anim;
  AnimV20::PrsAnimNodeRef prs;
};
