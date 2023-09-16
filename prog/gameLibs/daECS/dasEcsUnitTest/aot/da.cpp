#include <daScript/daScript.h>
#include <daScript/daScriptModule.h>
#include <daScript/simulate/fs_file_info.h>
#include <ecs/scripts/dasEs.h>
#include "../unitModule.h"

void require_project_specific_modules()
{
  NEED_MODULE(DagorFiles)
  NEED_MODULE(DasEcsUnitTest)
}
