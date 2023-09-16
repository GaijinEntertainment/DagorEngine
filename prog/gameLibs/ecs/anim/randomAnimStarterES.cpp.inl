#include <ecs/core/entitySystem.h>
#include <math/random/dag_random.h>
#include <ecs/anim/anim.h>
#include <daECS/core/coreEvents.h>
#include <anim/dag_animBlendCtrl.h>

ECS_TAG(gameClient)
ECS_REQUIRE(eastl::true_type randomAnimStart)
ECS_ON_EVENT(on_appear)
static void amimchar_entities_with_random_starter_es_event_handler(const ecs::Event &, AnimV20::AnimcharBaseComponent &animchar)
{
  if (AnimV20::AnimationGraph *animGraph = animchar.getAnimGraph())
    for (int i = 0; i < animGraph->getAnimNodeCount(); i++)
    {
      AnimV20::IAnimBlendNode *node = animGraph->getBlendNodePtr(i);
      if (!node || !node->isSubOf(AnimV20::AnimBlendNodeLeafCID))
        continue;
      AnimV20::AnimBlendNodeLeaf *animBlendNode = (AnimV20::AnimBlendNodeLeaf *)node;
      if (animBlendNode->isSubOf(AnimV20::AnimBlendNodeContinuousLeafCID))
      {
        AnimV20::AnimBlendNodeContinuousLeaf *continuousNode = (AnimV20::AnimBlendNodeContinuousLeaf *)animBlendNode;
        if (!continuousNode->isStartOffsetEnabled())
          continue;
        AnimV20::IAnimStateHolder *stateHolder = animchar.getAnimState();
        real duration = continuousNode->getDuration(*stateHolder);
        continuousNode->seekToSyncTime(*stateHolder, rnd_float(0.0, duration));
        continuousNode->enableRewind(false);
      }
    }
}