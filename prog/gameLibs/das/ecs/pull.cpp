// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if BUILD_AOT_LIB
extern size_t ecs_aot_DAS_pull_AOT;
size_t pull_das_ecs_aot_lib() { return ecs_aot_DAS_pull_AOT; }
#else
size_t pull_das_ecs_aot_lib() { return 0; }
#endif
