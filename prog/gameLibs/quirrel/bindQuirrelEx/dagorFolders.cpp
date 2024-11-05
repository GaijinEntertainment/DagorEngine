// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>
#include <folders/folders.h>


namespace bindquirrel
{


SQInteger get_exe_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_exe_dir().c_str(), -1);
  return 1;
}

SQInteger get_game_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_game_dir().c_str(), -1);
  return 1;
}

SQInteger get_temp_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_temp_dir().c_str(), -1);
  return 1;
}

SQInteger get_gamedata_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_gamedata_dir().c_str(), -1);
  return 1;
}

SQInteger get_local_appdata_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_local_appdata_dir().c_str(), -1);
  return 1;
}

SQInteger get_common_appdata_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_common_appdata_dir().c_str(), -1);
  return 1;
}

SQInteger get_downloads_dir_wrapper(HSQUIRRELVM vm)
{
  sq_pushstring(vm, folders::get_downloads_dir().c_str(), -1);
  return 1;
}


void register_dagor_folders_module(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);
  ///@module dagor.folders
  exports //
    .SquirrelFunc("get_exe_dir", get_exe_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_game_dir", get_game_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_temp_dir", get_temp_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_gamedata_dir", get_gamedata_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_local_appdata_dir", get_local_appdata_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_common_appdata_dir", get_common_appdata_dir_wrapper, 1, ".t")
    .SquirrelFunc("get_downloads_dir", get_downloads_dir_wrapper, 1, ".t")
    /**/;

  module_mgr->addNativeModule("dagor.folders", exports);
}

} // namespace bindquirrel
