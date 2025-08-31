// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entitySystem.h>

extern size_t pull_das_daFrameGraph_aot_lib();

#define REG_SYS_DEV RS(animCharDbgRender)

#define REG_SYS              \
  RS(sphere_cam)             \
  RS(free_cam)               \
  RS(menu_cam)               \
  RS(camera_view_es)         \
  RS(node_cam)               \
  RS(animchar_camera_target) \
  RS(animRandomNodeHider)    \
  RS(animCharNodeDebug)      \
  RS(animCharParamsDebug)    \
  RS(animCharHider)          \
  RS(tonemap)                \
  RS(shaderVars)             \
  RS(renderAnimCharIcon)     \
  RS(resPtr)                 \
  RS(frameGraphNode)         \
  RS(RTPool)                 \
  RS(postfxRenderer)         \
  RS(computeShader)          \
  RS(shaders)                \
  RS(lightning)              \
  RS(decals)                 \
  RS(samplerHandle)          \
  RS(skiesSettings)          \
  RS(camouflageOverrideParams)

#define REG_SQM RS(bind_screencap)

#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#if DAGOR_DBGLEVEL > 0
REG_SYS_DEV
#endif
#undef RS
#define RS(x) extern const size_t sq_autobind_pull_##x;
REG_SQM
#undef RS

#define RS(x) +ECS_PULL_VAR(x)
size_t framework_render_pulls = 0 REG_SYS
#if DAGOR_DBGLEVEL > 0
                                  REG_SYS_DEV
#endif
#undef RS
#define RS(x) +sq_autobind_pull_##x
                                    REG_SQM
#undef RS
                                + pull_das_daFrameGraph_aot_lib();

#include <util/dag_console.h>
PULL_CONSOLE_PROC(camera_console_handler)
PULL_CONSOLE_PROC(rendinst_impostor_console_handler)
