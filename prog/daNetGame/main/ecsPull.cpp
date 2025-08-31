// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>

#define REG_SYS               \
  RS(keyTrackAnim)            \
  RS(replication)             \
  RS(animchar)                \
  RS(hierarchy_transform)     \
  RS(input)                   \
  RS(collres)                 \
  RS(phys_body)               \
  RS(ragdoll)                 \
  RS(animPhys)                \
  RS(physVars)                \
  RS(animCharFastPhys)        \
  RS(animCharFastPhysDebug)   \
  RS(action)                  \
  RS(rendinst)                \
  RS(particle_phys)           \
  RS(animState)               \
  RS(randomAnimStarter)       \
  RS(gravity)                 \
  RS(riGpuObjects)            \
  RS(animCharEffectors)       \
  RS(soundComponent)          \
  RS(animcharSound)           \
  RS(ecs_debug)               \
  RS(animIrq)                 \
  RS(animCharCameraTarget)    \
  RS(transformSerializer)     \
  RS(netPhysCode)             \
  RS(rendinstSound)           \
  RS(netEvents)               \
  RS(instantiateDependencies) \
  RS(deferToAct)

#define REG_SQM          \
  RS(bind_anim_events)   \
  RS(bind_app)           \
  RS(bind_ecs_utils)     \
  RS(connectivity)       \
  RS(bind_game_events)   \
  RS(bind_sceneload)     \
  RS(dasEvents)          \
  RS(bind_settings)      \
  RS(bind_sndcontrol)    \
  RS(bind_speech_events) \
  RS(bind_streaming)     \
  RS(bind_vromfs)        \
  RS(bind_level_regions) \
  RS(bind_scene_camera)  \
  RS(bind_team)


#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#undef RS
#define RS(x) extern const size_t sq_autobind_pull_##x;
REG_SQM
#undef RS

// this var is required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly visible symbols
size_t framework_primary_pulls = 0
#define RS(x) +ECS_PULL_VAR(x)
  REG_SYS
#undef RS
#define RS(x) +sq_autobind_pull_##x
    REG_SQM
#undef RS
  ;
