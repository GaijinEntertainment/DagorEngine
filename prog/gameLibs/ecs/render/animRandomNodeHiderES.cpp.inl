#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/coreEvents.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_dynSceneRes.h>
#include <debug/dag_assert.h>
#include <math/dag_mathUtils.h>
#include <math/random/dag_random.h>
#include <debug/dag_debug.h>

ECS_ON_EVENT(on_appear)
static void anim_random_node_hider_es_event_handler(const ecs::Event &, AnimV20::AnimcharRendComponent &animchar_render,
  const ecs::Object &animchar_random_node_hider__nodes)
{
  DynamicRenderableSceneInstance *inst = animchar_render.getSceneInstance();
  for (auto &iter : animchar_random_node_hider__nodes)
  {
    int nodeId = inst->getNodeId(iter.first.data());
    if (nodeId >= 0)
      if (gfrnd() <= iter.second.getOr(0.f))
        inst->showNode(nodeId, false);
  }
}
