// Copyright (C) Gaijin Games KFT.  All rights reserved.

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

void anim::dump_animchar_state(AnimV20::AnimcharBaseComponent &animchar, const bool json_format)
{
  animchar.createDebugBlenderContext(/*dump_all_nodes*/ true);
  const DataBlock *debugBlk = animchar.getDebugBlenderState(/*dump_tm*/ true);
  DynamicMemGeneralSaveCB cwr(framemem_ptr(), 128 << 10, 4 << 10);
  if (json_format)
    dblk::export_to_json_text_stream(*debugBlk, cwr, true);
  else
    debugBlk->saveToTextStream(cwr);
  cwr.write(ZERO_PTR<char>(), sizeof(char)); // '\0'
  debug("--- animchar blend-state dump ---");
  debug("%s", cwr.data());
  animchar.destroyDebugBlenderContext();
}
