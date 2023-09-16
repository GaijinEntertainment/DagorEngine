#include <ecs/anim/anim.h>
#include <ecs/core/entityManager.h>
#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>

namespace ecs
{
AnimIrqHandler::~AnimIrqHandler()
{
  if (AnimV20::AnimcharBaseComponent *animChar =
        g_entity_mgr->getNullableRW<AnimV20::AnimcharBaseComponent>(eid, ECS_HASH("animchar"))) // animchar still exist (might be
                                                                                                // destroyed already)?
    animChar->unregisterIrqHandler(-1, this);                                                   // for all irq's
}
} // namespace ecs

void anim::dump_animchar_state(AnimV20::AnimcharBaseComponent &animchar)
{
  animchar.createDebugBlenderContext(/*dump_all_nodes*/ true);
  const DataBlock *debugBlk = animchar.getDebugBlenderState(/*dump_tm*/ true);
  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 128 << 10, 4 << 10);
  debugBlk->saveToTextStream(cwr);
  cwr.write(ZERO_PTR<char>(), sizeof(char)); // '\0'
  debug("%s", cwr.data());
  animchar.destroyDebugBlenderContext();
}
