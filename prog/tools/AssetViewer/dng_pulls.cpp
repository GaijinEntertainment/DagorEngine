// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daScript/misc/platform.h>
#include <daScript/daScriptModule.h>
#include <package_physobj/pullDas.h>
#include <folders/folders.h>
#include <dag/dag_vector.h>


const char *get_dir(const char *location) { return folders::get_path(location, ""); }

extern size_t pull_mem_leak_detector;
extern size_t get_framework_pulls();
size_t framework_pulls = get_framework_pulls() + pull_mem_leak_detector;

void pull_game_das()
{
  NEED_MODULES_PACKAGE_PHYSOBJ()
  NEED_MODULE(AssetViewerModule)
}
