// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/daScript.h>
#include <daScript/daScriptModule.h>
#include <daScript/simulate/fs_file_info.h>
#include <ecs/scripts/dasEs.h>
#include "../unitModule.h"

namespace das
{
void install_das_crash_handler() {}
} // namespace das

void require_project_specific_modules()
{
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DasEcsUnitTest)
}
