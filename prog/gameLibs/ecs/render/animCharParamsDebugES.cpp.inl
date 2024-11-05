// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/anim/anim.h>
#include <anim/dag_animBlend.h>
#include <daECS/core/updateStage.h>
#include <debug/dag_textMarks.h>
#include <math/dag_geomTree.h>

ECS_NO_ORDER
ECS_TAG(render, dev)
inline void debug_draw_animchar_es(const ecs::UpdateStageInfoRenderDebug &, AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Array &animchar_debug__varsList, const TMatrix &transform)
{
  int i = 0;
  for (auto &var : animchar_debug__varsList)
  {
    int paramId = animchar.getAnimGraph()->getParamId(var.getStr());
    if (paramId < 0)
      continue;
    String str(128, "%s = %f", var.getStr(), animchar.getAnimState()->getParam(paramId));
    add_debug_text_mark(transform.getcol(3) + Point3(0.f, 1.f, 0.f), str.str(), -1, i++);
    debug(str.str());
  }
}

ECS_NO_ORDER
ECS_TAG(render, dev)
inline void debug_draw_animchar_on_node_es(const ecs::UpdateStageInfoRenderDebug &, AnimV20::AnimcharBaseComponent &animchar,
  const ecs::Array &animchar_debug__varsList, const ecs::string &animchar_debug__nodeName)
{
  auto idx = animchar.getNodeTree().findNodeIndex(animchar_debug__nodeName.c_str());
  if (!idx)
    return;
  TMatrix wtm;
  animchar.getNodeTree().getNodeWtmScalar(idx, wtm);
  int i = 0;
  for (auto &var : animchar_debug__varsList)
  {
    int paramId = animchar.getAnimGraph()->getParamId(var.getStr());
    if (paramId < 0)
      continue;
    String str(128, "%s = %f", var.getStr(), animchar.getAnimState()->getParam(paramId));
    add_debug_text_mark(wtm * Point3(0.f, 0.f, 0.f), str.str(), -1, i++);
    debug(str.str());
  }
}
