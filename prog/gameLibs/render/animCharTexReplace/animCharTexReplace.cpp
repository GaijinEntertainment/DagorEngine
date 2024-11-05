// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <animChar/dag_animCharacter2.h>
#include <shaders/dag_dynSceneRes.h>

#include "render/animCharTexReplace.h"
#include <gameRes/dag_stdGameResId.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>

AnimCharTexReplace::AnimCharTexReplace(AnimV20::AnimcharRendComponent &ac_rend, const char *animchar_name)
{
  acRend = &ac_rend;
  animcharName = animchar_name;
  dagor_set_sm_tex_load_ctx_type(AnimCharGameResClassId);
  dagor_set_sm_tex_load_ctx_name(animcharName);
  acRend->getVisualResource()->gatherUsedTex(usedTexIds);
}

void AnimCharTexReplace::replaceTex(const char *from, const char *to, TEXTUREID to_id)
{
  TEXTUREID from_id = get_managed_texture_id(from);
  if (from_id == BAD_TEXTUREID || !usedTexIds.has(from_id))
  {
    logwarn("missing or unused src tex <%s> id=%d for tex replace in animChar=%s", from, from_id, animcharName);
    return;
  }
  if (to_id == BAD_TEXTUREID)
  {
    logerr("missing dest tex <%s> id=%d for tex replace in animChar=%s", to, to_id, animcharName);
    return;
  }
  if (!skinnedRes)
    skinnedRes = acRend->getVisualResource()->clone();

  // debug("animChar %s: replace tex (%s)=%d -> (%s)=%d", animcharName, from, from_id, to, to_id);
  skinnedRes->duplicateMaterial(from_id, oldMat, newMat);
  skinnedRes->replaceTexture(from_id, to_id);
}

void AnimCharTexReplace::replaceTex(const char *from, const char *to)
{
  String tmp1, tmp2;
  from = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(from, tmp1);
  to = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(to, tmp2);

  TEXTUREID to_id = get_managed_texture_id(to);
  replaceTex(from, to, to_id);
}

void AnimCharTexReplace::replaceTexByShaderVar(const char *from, const char *to, const char *shader_var_name)
{
  String tmp1, tmp2;
  from = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(from, tmp1);
  to = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(to, tmp2);

  TEXTUREID from_id = get_managed_texture_id(from);
  TEXTUREID to_id = get_managed_texture_id(to);
  int toParam = get_shader_variable_id(shader_var_name);
  if (from_id == BAD_TEXTUREID || !usedTexIds.has(from_id))
  {
    logwarn("missing or unused src tex <%s> id=%d for tex replace in animChar=%s", from, from_id, animcharName);
    return;
  }
  if (to_id == BAD_TEXTUREID)
  {
    logerr("missing dest tex <%s> id=%d for tex replace in animChar=%s", to, to_id, animcharName);
    return;
  }
  if (!skinnedRes)
    skinnedRes = acRend->getVisualResource()->clone();

  skinnedRes->duplicateMaterial(from_id, oldMat, newMat);

  for (int m = 0; m < newMat.size(); m++)
    if (newMat[m]->replaceTexture(from_id, from_id))
      newMat[m]->set_texture_param(toParam, to_id);
}

void AnimCharTexReplace::apply(const AnimV20::AnimcharBaseComponent &ac_base, bool create_instance)
{
  dagor_reset_sm_tex_load_ctx();
  if (skinnedRes == acRend->getVisualResource() || skinnedRes == nullptr)
    return;
  skinnedRes->finalizeDuplicateMaterial(make_span(newMat));
  skinnedRes->updateShaderElems();
  acRend->setVisualResource(skinnedRes, create_instance, ac_base.getNodeTree());
}
