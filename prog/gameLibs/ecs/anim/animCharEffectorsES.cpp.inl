// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <anim/dag_animStateHolder.h>
#include <anim/dag_animPostBlendCtrl.h>
#include <ecs/anim/animCharEffectors.h>
#include <ecs/anim/animcharUpdateEvent.h>

ECS_REGISTER_TYPE(EffectorData, nullptr);

static void reset_ik_effector(AnimV20::AnimcharBaseComponent &animchar, int eff_id)
{
  if (eff_id >= 0)
    animchar.getAnimState()->paramEffector(eff_id).type = AnimV20::AnimCommonStateHolder::EffectorVar::T_useGeomNode;
}

static void reset_ik_attnode(AnimV20::AnimcharBaseComponent &animchar, int wtm_id)
{
  if (wtm_id >= 0)
    AnimV20::AttachGeomNodeCtrl::getAttachDesc(*animchar.getAnimState(), wtm_id).w = 0;
}

ECS_ON_EVENT(on_appear, InvalidateEffectorData)
static void animchar_effectors_init_es_event_handler(const ecs::Event &, ecs::EntityId eid,
  const AnimV20::AnimcharBaseComponent &animchar, const ecs::Array &animchar_effectors__effectorsList,
  ecs::Object &animchar_effectors__effectorsState)
{
  animchar_effectors__effectorsState.clear();
  const AnimV20::AnimationGraph *animGraph = animchar.getAnimGraph();
  if (!animGraph)
    return;
  for (const auto &it : animchar_effectors__effectorsList)
  {
    const ecs::Object &obj = it.get<ecs::Object>();

    const ecs::string &effName = obj[ECS_HASH("effectorName")].get<ecs::string>();
    int effId = animGraph->getParamId(effName.c_str(), AnimV20::IAnimStateHolder::PT_Effector);
    if (DAGOR_UNLIKELY(effId < 0))
    {
      logerr("Effector name '%s' not found in animgraph of animchar '%s' entity '%s'", effName.c_str(),
        animchar.getCreateInfo()->resName.c_str(), g_entity_mgr->getEntityTemplateName(eid));
      continue;
    }

    const ecs::string &effWtmName = obj[ECS_HASH("effectorWtmName")].get<ecs::string>();
    int effWtmId = animGraph->getParamId(effWtmName.c_str(), AnimV20::IAnimStateHolder::PT_InlinePtr);

    animchar_effectors__effectorsState.addMember(ECS_HASH_SLOW(effName.c_str()), EffectorData(effId, effWtmId));
  }
}

ECS_TAG(render)
ECS_AFTER(before_animchar_update_sync)
ECS_BEFORE(animchar__updater_es) // must be before animchar update (but actually it should be 'just before')
ECS_REQUIRE(eastl::true_type animchar__updatable)
ECS_REQUIRE(eastl::true_type animchar__visible = true)
ECS_REQUIRE_NOT(ecs::Tag animchar__actOnDemand)
static void animchar_effectors_update_es(const UpdateAnimcharEvent &, AnimV20::AnimcharBaseComponent &animchar,
  const TMatrix &transform, const ecs::Object &animchar_effectors__effectorsState)
{
  AnimV20::IAnimStateHolder *animState = animchar.getAnimState();
  for (auto &it : animchar_effectors__effectorsState)
  {
    const EffectorData &eff = it.second.get<EffectorData>();
    if (eff.weight > 0.f)
    {
      if (eff.effId >= 0)
        animState->paramEffector(eff.effId).set(eff.position.x, eff.position.y, eff.position.z, true);
      if (eff.effWtmId >= 0)
      {
        AnimV20::AttachGeomNodeCtrl::AttachDesc &att = AnimV20::AttachGeomNodeCtrl::getAttachDesc(*animState, eff.effWtmId);
        att.wtm = eff.wtm;
        att.wtm.setcol(3, att.wtm.getcol(3) - (transform.getcol(3) - GeomNodeTree::calc_optimal_wofs(transform.getcol(3))));
        att.w = eff.weight;
      }
    }
    else
    {
      reset_ik_effector(animchar, eff.effId);
      reset_ik_attnode(animchar, eff.effWtmId);
    }
  }
}
