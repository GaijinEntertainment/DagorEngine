#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/phys/particlePhysSys.h>

#include <memory/dag_framemem.h>
#include <ioSys/dag_dataBlock.h>

#include <ecs/anim/anim.h>

#include <util/dag_convar.h>

CONSOLE_BOOL_VAL("phys", debug_particle_phys, false);

ECS_TAG(render)
static __forceinline void particle_phys_es_event_handler(const ecs::EventEntityCreated &, const ecs::string &particle_phys__blk,
  const AnimV20::AnimcharBaseComponent &animchar, daphys::ParticlePhysSystem &particle_phys)
{
  char blkName[DAGOR_MAX_PATH];
  strncpy(blkName, particle_phys__blk.c_str(), sizeof(blkName) - 1);
  blkName[sizeof(blkName) - 1] = '\0';
  char *pColon = strchr(blkName, ':');
  const char *blockName = NULL;
  if (pColon)
  {
    *pColon = '\0'; // cut block name off
    blockName = pColon + 1;
  }
  const DataBlock blk(blkName, framemem_ptr());
  particle_phys.loadFromBlk(blockName ? blk.getBlockByName(blockName) : &blk, animchar.getOriginalNodeTree());
}

ECS_NO_ORDER
ECS_TAG(render, dev)
inline void particle_phys_debug_es(const ecs::UpdateStageInfoRenderDebug &, const TMatrix &transform,
  const daphys::ParticlePhysSystem &particle_phys)
{
  if (debug_particle_phys.get())
    particle_phys.renderDebug(transform);
}


ECS_REGISTER_RELOCATABLE_TYPE(daphys::ParticlePhysSystem, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(daphys::ParticlePhysSystem, "particle_phys", nullptr, 0, "animchar");
ECS_DEF_PULL_VAR(particle_phys);
