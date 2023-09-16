//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "anim/dag_animDecl.h"
#include <generic/dag_tab.h>
#include <3d/dag_texIdSet.h>

class DynamicRenderableSceneLodsResource;
class ShaderMaterial;

class AnimCharTexReplace
{
public:
  AnimCharTexReplace(AnimV20::AnimcharRendComponent &ac_rend, const char *animchar_name);
  void replaceTex(const char *from, const char *to, TEXTUREID to_id);
  void replaceTex(const char *from, const char *to);
  void replaceTexByShaderVar(const char *from, const char *to, const char *shader_var_name);
  void apply(const AnimV20::AnimcharBaseComponent &ac_base, bool create_instance);

private:
  AnimV20::AnimcharRendComponent *acRend = nullptr;
  const char *animcharName = nullptr;
  DynamicRenderableSceneLodsResource *skinnedRes = nullptr;
  Tab<ShaderMaterial *> oldMat;
  Tab<ShaderMaterial *> newMat;
  TextureIdSet usedTexIds;
};
