// Copyright (C) Gaijin Games KFT.  All rights reserved.

extern size_t framework_primary_pulls;
extern size_t framework_render_pulls;
extern size_t framework_input_pulls;
extern size_t framework_es_pulls;
extern size_t daNetGame_client_DAS_pull_AOT;
extern size_t daNetGame_server_DAS_pull_AOT;
extern size_t pull_das_ecs_aot_lib();
extern size_t pull_das_math_aot_lib();

size_t get_framework_pulls()
{
  return framework_primary_pulls + framework_render_pulls + framework_input_pulls + framework_es_pulls +
         daNetGame_client_DAS_pull_AOT + daNetGame_server_DAS_pull_AOT + pull_das_ecs_aot_lib() + pull_das_math_aot_lib();
}
