#include <vr/vrHands.h>

#include <drivers/dag_vr.h>

#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_stdGameResId.h>
#include <render/dynmodelRenderer.h>

#include <gamePhys/phys/animatedPhys.h>
#include <gamePhys/phys/physVars.h>


namespace vr
{

VrHands::~VrHands() { clear(); }

void VrHands::clear()
{
  for (VrInput::Hands side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    if (animChar[side])
      del_it(animChar[side]);
    del_it(animPhys[side]);
    del_it(physVars[side]);
  }
}

void VrHands::initAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component)
{
  del_it(animPhys[side]);
  del_it(physVars[side]);

  physVars[side] = new PhysVars;

  animPhys[side] = new AnimatedPhys();
  animPhys[side]->init(animchar_component, *physVars[side]);

  indexFingerAnimVarId[side] = addAnimPhysVar(animchar_component, side, "index_finger_var", 0.f);
  fingersAnimVarId[side] = addAnimPhysVar(animchar_component, side, "fingers_var", 0.f);
  thumbAnimVarId[side] = addAnimPhysVar(animchar_component, side, "thumb_var", 0.f);
}

void VrHands::updateAnimPhys(VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component)
{
  if (!physVars[side])
    return;

  const OneHandState &state = handsState[side];

  const float POINTING_POSE_GRIP_THRESHOLD = 0.85f;
  const bool shouldPoint =
    !float_nonzero(state.indexFinger) && float_nonzero(state.thumb) && state.squeeze > POINTING_POSE_GRIP_THRESHOLD;
  setAnimPhysVarVal(side, indexFingerAnimVarId[side], shouldPoint ? 1.f : state.indexFinger * 0.5f);
  setAnimPhysVarVal(side, thumbAnimVarId[side], shouldPoint ? 1.f : state.thumb * 0.5f);
  setAnimPhysVarVal(side, fingersAnimVarId[side], state.squeeze);

  animPhys[side]->update(animchar_component, *physVars[side]);
}

int VrHands::addAnimPhysVar(AnimV20::AnimcharBaseComponent &animchar_component, VrInput::Hands side, const char *name, float def_val)
{
  if (!physVars[side])
    return -1;

  const int varId = physVars[side]->registerVar(name, def_val);
  if (animPhys[side])
    animPhys[side]->appendVar(name, animchar_component, *physVars[side]);
  return varId;
}

void VrHands::setAnimPhysVarVal(VrInput::Hands side, int var_id, float val)
{
  if (!physVars[side])
    return;

  physVars[side]->setVar(var_id, val);
}

void VrHands::initHand(const char *model_name, VrInput::Hands side)
{
  String animcharResName(model_name);
  animcharResName += "_char";
  GameResHandle animcharHandle = GAMERES_HANDLE_FROM_STRING(animcharResName);

  AnimCharV20::IAnimCharacter2 *resAnimChar =
    (AnimCharV20::IAnimCharacter2 *)::get_one_game_resource_ex(animcharHandle, CharacterGameResClassId);

  if (resAnimChar)
  {
    animChar[side] = resAnimChar->clone();
    ::release_game_resource((GameResource *)resAnimChar);
  }
}

void VrHands::init(const char *model_name_l, const char *model_name_r, const char *index_tip_l, const char *index_tip_r)
{
  clear();

  const char *modelNames[VrInput::Hands::Total] = {model_name_l, model_name_r};
  const char *indexFingerTipNames[VrInput::Hands::Total] = {index_tip_l, index_tip_r};
  for (auto hand : {VrInput::Hands::Left, VrInput::Hands::Right})
    if (modelNames[hand])
    {
      initHand(modelNames[hand], hand);
      if (auto ac = animChar[hand])
      {
        initAnimPhys(hand, ac->baseComp());
        nodeMaps[hand].init(ac->getNodeTree(), ac->getVisualResource()->getNames().node);
        indexFingerTipsIndices[hand] = ac->getNodeTree().findNodeIndex(indexFingerTipNames[hand]);
      }
    }
}

void VrHands::init(AnimV20::AnimcharBaseComponent &left_char, AnimV20::AnimcharBaseComponent &right_char)
{
  clear();
  initAnimPhys(VrInput::Hands::Left, left_char);
  initAnimPhys(VrInput::Hands::Right, right_char);
}

void VrHands::update(float dt, const TMatrix &local_ref_space_tm, HandsState &&hands)
{
  handsState = eastl::move(hands);

  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    if (handsState[side].isActive && animChar[side])
    {
      auto &ac = animChar[side]->baseComp();
      TMatrix handTm;
      updateHand(local_ref_space_tm, side, ac, handTm);
      mat44f tm4; // construct matrix manually to avoid losing precision/perfomance to orthonormalization
      v_mat44_make_from_43cu_unsafe(tm4, handTm.array);
      ac.setTm(tm4, true);
      ac.act(dt, true);
      animChar[side]->copyNodes();
    }
  }
}


void VrHands::setState(HandsState &&hands) { handsState = eastl::move(hands); }


void VrHands::updateHand(const TMatrix &local_ref_space_tm, VrInput::Hands side, AnimV20::AnimcharBaseComponent &animchar_component,
  TMatrix &tm)
{
  updateAnimPhys(side, animchar_component);

  const TMatrix grip = local_ref_space_tm * handsState[side].grip;
  tm = local_ref_space_tm * handsState[side].aim;
  tm.setcol(3, grip.getcol(3));
}


void VrHands::beforeRender(const Point3 &cam_pos)
{
  vec4f camPos = v_ldu(&cam_pos.x);
  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
  {
    if (handsState[side].isActive && animChar[side])
    {
      animChar[side]->beforeRender();
      if (auto scene = animChar[side]->getSceneInstance())
      {
        scene->savePrevNodeWtm();
        scene->setOrigin(cam_pos);
        auto &nodeMap = nodeMaps[side];
        auto &tree = animChar[side]->getNodeTree();
        for (unsigned int i = 0; i < nodeMap.size(); ++i)
        {
          mat44f wtm = tree.getNodeWtmRel(nodeMap[i].nodeIdx);
          vec4f ofs = v_sub(animChar[side]->getFinalWtm().wofs, camPos);
          wtm.col3 = v_add(wtm.col3, ofs);
          scene->setNodeWtmRelToOrigin(nodeMap[i].id, wtm);
        }

        scene->setLod(0);
        scene->beforeRender();
      }
    }
  }
}


void VrHands::render(TexStreamingContext texCtx)
{
  for (auto side : {VrInput::Hands::Left, VrInput::Hands::Right})
    if (handsState[side].isActive && animChar[side])
      if (auto scene = animChar[side]->getSceneInstance())
      {
        G_ASSERT_RETURN(dynrend::can_render(scene), );
        dynrend::render_one_instance(scene, dynrend::RenderMode::Opaque, texCtx, nullptr, nullptr);
      }
}


TMatrix VrHands::getPalmTm(VrInput::Hands hand) const
{
  TMatrix tm = TMatrix::IDENT;
  if (handsState[hand].isActive && animChar[hand])
    animChar[hand]->baseComp().getTm(tm);

  return tm;
}


TMatrix VrHands::getIndexFingertipWtm(VrInput::Hands hand) const
{
  TMatrix tm = TMatrix::IDENT;
  if (handsState[hand].isActive && animChar[hand])
    animChar[hand]->getNodeTree().getNodeWtmScalar(indexFingerTipsIndices[hand], tm);

  return tm;
}


} // namespace vr
