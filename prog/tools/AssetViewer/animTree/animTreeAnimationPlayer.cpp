// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animTreeAnimationPlayer.h"
#include "animTreeUtils.h"
#include "animParamData.h"
#include "blendNodes/blendNodeType.h"
#include <math/dag_mathUtils.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <propPanel/control/container.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <ctype.h>

#include <de3_interface.h>
#include <de3_objEntity.h>
#include <de3_lodController.h>
#include <de3_animCtrl.h>

extern String a2d_last_ref_model;

AnimTreeAnimationPlayer::AnimTreeAnimationPlayer(int dyn_model_id) :
  dynModelId{dyn_model_id},
  animProgress{0.f},
  type{BlendNodeType::UNSET},
  addk{0.f},
  mulk{1.f},
  pStart{0.f},
  pEnd{1.f},
  animTime{1.f},
  firstKey{0},
  lastKey{0},
  loop{false}
{}

static int get_key_by_name(const AnimV20::AnimData *selected_a2d, const char *key_name)
{
  for (int i = 0; i < selected_a2d->dumpData.noteTrack.size(); ++i)
    if (!strcmp(key_name, selected_a2d->dumpData.noteTrack[i].name))
      return selected_a2d->dumpData.noteTrack[i].time;

  return -1;
}

static int get_key_by_text(const AnimV20::AnimData *selected_a2d, const char *text)
{
  int key = get_key_by_name(selected_a2d, text);
  if (key < 0 && isdigit(text[0]))
    key = atoi(text) * AnimV20::TIME_TicksPerSec / 30;
  else if (key < 0)
    key = 0;

  return key;
}

void AnimTreeAnimationPlayer::init(const DagorAsset &asset)
{
  if (modelName.empty())
    modelName = a2d_last_ref_model;
  reloadDynModel(asset);
}

void AnimTreeAnimationPlayer::clear()
{
  resetDynModel();
  if (selectedA2d)
  {
    ::release_game_resource((GameResource *)selectedA2d);
    selectedA2d = nullptr;
  }
  selectedA2dName.clear();
}

void AnimTreeAnimationPlayer::act(float dt)
{
  if (!entity || !ctrl || !origGeomTree || !selectedA2d)
    return;

  if (isAnimInProgress())
  {
    if (type != BlendNodeType::PARAMETRIC)
    {
      animProgress += safediv(dt, animTime);
      if (loop)
        animProgress = fmodf(animProgress, 1.f);
      int animKey = lerp(firstKey, lastKey, saturate(animProgress));
      animate(animKey);
    }
    else
    {
      animProgress += dt;
      if (loop)
        animProgress = fmodf(animProgress, 1.f);
      float param = safediv(animProgress * mulk + addk - pStart, pEnd - pStart);
      int animKey = int(param * (lastKey - firstKey) + firstKey);
      animate(animKey);
    }
  }
}

void AnimTreeAnimationPlayer::setDynModel(const char *model_name, const DagorAsset &asset)
{
  a2d_last_ref_model = modelName = model_name;
  reloadDynModel(asset);
}

void AnimTreeAnimationPlayer::resetDynModel()
{
  if (ctrl)
    ctrl->setSkeletonForRender(nullptr);
  destroy_it(entity);
  geomTree = GeomNodeTree();
  ctrl = nullptr;
  origGeomTree = nullptr;
  animNodes.clear();
}

void AnimTreeAnimationPlayer::reloadDynModel(const DagorAsset &asset)
{
  resetDynModel();
  if (modelName.empty())
    return;

  DagorAsset *modelAsset = asset.getMgr().findAsset(modelName, dynModelId);
  if (!modelAsset)
  {
    logerr("Can't find asset for load dynModel: %s", modelName);
    return;
  }

  entity = DAEDITOR3.createEntity(*modelAsset);
  entity->setSubtype(DAEDITOR3.registerEntitySubTypeId("single_ent"));
  ctrl = entity->queryInterface<ILodController>();
  origGeomTree = ctrl->getSkeleton();
  if (!origGeomTree)
    return;
  geomTree = *origGeomTree;
  ctrl->setSkeletonForRender(&geomTree);
}

void AnimTreeAnimationPlayer::setValuesFromCommonNode(PropPanel::ContainerPropertyControl *panel,
  const dag::Vector<AnimParamData> &params)
{
  if (const auto timeParam = find_param_by_name(params, "time"); timeParam != params.end())
    animTime = panel->getFloat(timeParam->pid);
  else
    animTime = 1.f;
  const auto firstKeyParam = find_param_by_name(params, "key_start");
  const auto lastKeyParam = find_param_by_name(params, "key_end");
  if (firstKeyParam != params.end() && lastKeyParam != params.end())
  {
    firstKey = get_key_by_text(selectedA2d, panel->getText(firstKeyParam->pid));
    lastKey = get_key_by_text(selectedA2d, panel->getText(lastKeyParam->pid));
  }
  else
  {
    const int lastTrackIdx = selectedA2d->dumpData.noteTrack.size() - 1;
    firstKey = 0;
    lastKey = selectedA2d->dumpData.noteTrack[lastTrackIdx].time;
  }
}

void AnimTreeAnimationPlayer::setValuesFormStillNode(PropPanel::ContainerPropertyControl *panel,
  const dag::Vector<AnimParamData> &params)
{
  const auto keyParam = find_param_by_name(params, "key");
  const auto startParam = find_param_by_name(params, "start");
  if (startParam != params.end())
  {
    firstKey = panel->getInt(startParam->pid);
    if (firstKey < 0 && keyParam != params.end())
      firstKey = get_key_by_text(selectedA2d, panel->getText(keyParam->pid));
  }
  loop = false;
  lastKey = firstKey;
  animProgress = 1.f;
}

void AnimTreeAnimationPlayer::setValuesFormParametricNode(PropPanel::ContainerPropertyControl *panel,
  const dag::Vector<AnimParamData> &params)
{
  loop = get_bool_param_by_name_optional(params, panel, "looping");
  addk = get_float_param_by_name_optional(params, panel, "addk");
  mulk = get_float_param_by_name_optional(params, panel, "mulk");
  pStart = get_float_param_by_name_optional(params, panel, "p_start");
  pEnd = get_float_param_by_name_optional(params, panel, "p_end");
  SimpleString keyStart = get_str_param_by_name_optional(params, panel, "key_start");
  SimpleString keyEnd = get_str_param_by_name_optional(params, panel, "key_end");
  firstKey = get_key_by_text(selectedA2d, keyStart.c_str());
  lastKey = get_key_by_text(selectedA2d, keyEnd.c_str());
}

void AnimTreeAnimationPlayer::loadA2dResource(PropPanel::ContainerPropertyControl *panel, const dag::Vector<AnimParamData> &params,
  PropPanel::TLeafHandle leaf)
{
  if (selectedA2dName.empty() || !entity || !ctrl || !origGeomTree)
    return;

  if (selectedA2d)
  {
    ::release_game_resource((GameResource *)selectedA2d);
    selectedA2d = nullptr;
  }
  selectedA2d = (AnimV20::AnimData *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(selectedA2dName), Anim2DataGameResClassId);
  animNodes.clear();
  animProgress = 0.f;
  if (!selectedA2d)
    return;

  for (dag::Index16 ni(0), idxEnd(geomTree.nodeCount()); ni != idxEnd; ++ni)
    if (const char *nodeName = geomTree.getNodeName(ni))
      if (SimpleNodeAnim anim; anim.init(selectedA2d, nodeName))
        animNodes.emplace_back(AnimNodeData{nodeName, geomTree.findNodeIndex(nodeName), anim});

  PropPanel::ContainerPropertyControl *tree = panel->getById(PID_ANIM_BLEND_NODES_TREE)->getContainer();
  if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_A2D)
  {
    const int lastTrackIdx = selectedA2d->dumpData.noteTrack.size() - 1;
    firstKey = 0;
    lastKey = selectedA2d->dumpData.noteTrack[lastTrackIdx].time;
    animTime = 1.f;
    loop = false;
    type = BlendNodeType::UNSET;
  }
  else if (tree->getUserData(leaf) == &ANIM_BLEND_NODE_LEAF)
  {
    type = static_cast<BlendNodeType>(panel->getInt(PID_NODES_SETTINGS_NODE_TYPE_COMBO_SELECT));
    switch (type)
    {
      case BlendNodeType::SINGLE:
        setValuesFromCommonNode(panel, params);
        loop = false;
        break;
      case BlendNodeType::CONTINUOUS:
        setValuesFromCommonNode(panel, params);
        loop = true;
        break;
      case BlendNodeType::STILL: setValuesFormStillNode(panel, params); break;
      case BlendNodeType::PARAMETRIC: setValuesFormParametricNode(panel, params); break;
      default: break;
    }
  }

  animate(firstKey);
}

static void blend_add_anim(mat44f &dest_tm, const mat44f &base_tm, const TMatrix &add_tm_s)
{
  vec4f p0, r0, s0;
  vec4f p1, r1, s1;
  mat44f add_tm;
  v_mat44_make_from_43cu(add_tm, add_tm_s.m[0]);
  v_mat4_decompose(base_tm, p0, r0, s0);
  v_mat4_decompose(add_tm, p1, r1, s1);
  v_mat44_compose(dest_tm, v_add(p0, p1), v_quat_mul_quat(r0, r1), v_mul(s0, s1));
}

void AnimTreeAnimationPlayer::animate(int key)
{
  if (!entity || !ctrl || !origGeomTree || !selectedA2d)
    return;

  for (AnimNodeData &animNode : animNodes)
  {
    if (!animNode.skelIdx)
      continue;
    TMatrix tm;
    animNode.anim.calcAnimTm(tm, key);
    mat44f &nodeTm = geomTree.getNodeTm(animNode.skelIdx);
    if (selectedA2d->isAdditive())
      blend_add_anim(nodeTm, origGeomTree->getNodeTm(animNode.skelIdx), tm);
    else
    {
      if (lengthSq(tm.getcol(3)) < 1e-4)
        v_stu_p3(&tm.col[3].x, nodeTm.col3);
      geomTree.setNodeTmScalar(animNode.skelIdx, tm);
    }
  }

  geomTree.invalidateWtm();
  geomTree.calcWtm();
  entity->setTm(TMatrix::IDENT);
}

void AnimTreeAnimationPlayer::animNodesFieldChanged(PropPanel::ContainerPropertyControl *panel,
  const dag::Vector<AnimParamData> &params, const AnimParamData &param)
{
  if (param.name == "time")
    animTime = panel->getFloat(param.pid);
  else if (param.name == "addk")
    addk = panel->getFloat(param.pid);
  else if (param.name == "p_start")
    pStart = panel->getFloat(param.pid);
  else if (param.name == "p_end")
    pEnd = panel->getFloat(param.pid);
  else if (selectedA2d && param.name == "key_start")
  {
    // If start:i > 0 we need ignore change this field
    int startValue = get_int_param_by_name_optional(params, panel, "start", /*default_value*/ -1);
    if (startValue < 0)
      firstKey = get_key_by_text(selectedA2d, panel->getText(param.pid));
  }
  else if (selectedA2d && param.name == "key_end")
  {
    // If end:i > 0 we need ignore change this field
    int endValue = get_int_param_by_name_optional(params, panel, "end", /*default_value*/ -1);
    if (endValue < 0)
      lastKey = get_key_by_text(selectedA2d, panel->getText(param.pid));
  }
  else if (selectedA2d && param.name == "start")
  {
    int startValue = panel->getInt(param.pid);
    firstKey = startValue > 0 ? startValue : get_key_by_text(selectedA2d, get_str_param_by_name_optional(params, panel, "key_start"));
  }
  else if (selectedA2d && param.name == "end")
  {
    int endValue = panel->getInt(param.pid);
    lastKey = endValue > 0 ? endValue : get_key_by_text(selectedA2d, get_str_param_by_name_optional(params, panel, "key_end"));
  }
  else if (param.name == "looping")
    loop = panel->getBool(param.pid);
}

void AnimTreeAnimationPlayer::changePlayingAnimation()
{
  if (isAnimInProgress())
    animProgress = -1.f;
  else
    animProgress = 0.f;
}

void AnimTreeAnimationPlayer::setSelectedA2dName(const char *name) { selectedA2dName = DagorAsset::fpath2asset(name); }

bool AnimTreeAnimationPlayer::getSelectionBox(BBox3 &box) const
{
  if (!entity)
    return false;
  box = entity->getBbox();
  return true;
}
