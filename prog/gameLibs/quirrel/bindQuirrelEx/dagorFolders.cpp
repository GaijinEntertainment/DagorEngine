// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <sqmodules/sqmodules.h>
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
    .SquirrelFuncDeclString(get_exe_dir_wrapper, "get_exe_dir(): string")
    .SquirrelFuncDeclString(get_game_dir_wrapper, "get_game_dir(): string")
    .SquirrelFuncDeclString(get_temp_dir_wrapper, "get_temp_dir(): string")
    .SquirrelFuncDeclString(get_gamedata_dir_wrapper, "get_gamedata_dir(): string")
    .SquirrelFuncDeclString(get_local_appdata_dir_wrapper, "get_local_appdata_dir(): string")
    .SquirrelFuncDeclString(get_common_appdata_dir_wrapper, "get_common_appdata_dir(): string")
    .SquirrelFuncDeclString(get_downloads_dir_wrapper, "get_downloads_dir(): string")
    /**/;

  module_mgr->addNativeModule("dagor.folders", exports);
}

} // namespace bindquirrel
