// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <daECS/core/coreEvents.h>
#include <math/dag_mathAng.h>

#include "jetpackFx.h"
#include "render/fx/fx.h"
#include "render/fx/effectManager.h"


ECS_REGISTER_RELOCATABLE_TYPE(JetpackExhaust, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(JetpackExhaust, "jetpack_exhaust", nullptr, 0, "animchar");

bool JetpackExhaust::onLoaded(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  const ecs::Object &jetpackExhausts = mgr.get<ecs::Object>(eid, ECS_HASH("jetpack_exhaust__effects"));
  const AnimV20::AnimcharBaseComponent &animchar = mgr.get<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"));
  const GeomNodeTree &tree = animchar.getNodeTree();

  for (const auto &exhaust : jetpackExhausts)
  {
    ExhaustNode &ex = exhaustFx.push_back();
    ex.nodeId = tree.findNodeIndex(exhaust.first.c_str());
    ex.fxId = acesfx::get_type_by_name(exhaust.second.get<ecs::string>().c_str());
    ex.fx = nullptr;
  }
  return true;
}

ECS_TAG(render)
ECS_BEFORE(remove_unowned_items)
static void jetpack_fx_es(const ecs::UpdateStageInfoAct &,
  JetpackExhaust &jetpack_exhaust,
  const AnimV20::AnimcharBaseComponent &animchar,
  bool jetpack__active,
  Point3 jetpack_exhaust__modAngles = Point3(0.f, 0.f, 0.f),
  Point3 jetpack_exhaust__nodeOffset = Point3(0.f, 0.f, 0.f))
{
  const GeomNodeTree &gnTree = animchar.getNodeTree();
  for (JetpackExhaust::ExhaustNode &node : jetpack_exhaust.exhaustFx)
  {
    TMatrix nodeWtm = TMatrix::IDENT;
    gnTree.getNodeWtmScalar(node.nodeId, nodeWtm);
    nodeWtm.setcol(3, nodeWtm * jetpack_exhaust__nodeOffset);
    if (jetpack_exhaust__modAngles.lengthSq() > 1e-5f)
    {
      Quat modQuat;
      euler_to_quat(DegToRad(jetpack_exhaust__modAngles.x), DegToRad(jetpack_exhaust__modAngles.y),
        DegToRad(jetpack_exhaust__modAngles.z), modQuat);
      nodeWtm = nodeWtm * makeTM(modQuat);
    }
    if (jetpack__active && !node.fx)
    {
      if (AcesEffect *fx = acesfx::start_effect(node.fxId, TMatrix::IDENT, nodeWtm, false))
      {
        node.fx = fx;
        fx->lock();
      }
    }
    if (node.fx)
    {
      if (!jetpack__active)
        acesfx::stop_effect(node.fx);
      if (jetpack__active)
        node.fx->setFxTm(nodeWtm);
    }
  }
}


ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void clear_jetpack_fx_es_event_handler(const ecs::Event &, JetpackExhaust &jetpack_exhaust)
{
  for (JetpackExhaust::ExhaustNode &node : jetpack_exhaust.exhaustFx)
    if (node.fx)
      acesfx::stop_effect(node.fx);
}
