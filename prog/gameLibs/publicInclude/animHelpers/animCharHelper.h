//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <anim/dag_animDecl.h>
#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>
#include <util/dag_oaHashNameMap.h>

class AnimCharHelper
{
  AnimV20::IAnimCharacter2 *animChar;

  Ptr<AnimV20::AnimBlendCtrl_Fifo3> root;
  Tab<AnimV20::IAnimBlendNode *> bnls;
  NameMap animNames;

public:
  AnimCharHelper();
  ~AnimCharHelper();

  void init(AnimV20::IAnimCharacter2 *anim_char, const char *root_name);
  int registerAnim(const char *name);
  void playAnim(int anim_id, bool seek);
};
