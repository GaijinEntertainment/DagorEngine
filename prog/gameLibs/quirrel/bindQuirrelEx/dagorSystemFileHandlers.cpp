// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fileDropHandler.h"
#include "screenshotMetaInfoLoader.h"
#include <quirrel/sqModules/sqModules.h>
#include <quirrel/bindQuirrelEx/bindQuirrelEx.h>

namespace bindquirrel
{

void register_dagor_system_file_handlers(SqModules *module_mgr)
{
  HSQUIRRELVM vm = module_mgr->getVM();
  Sqrat::Table exports(vm);

  ///@module dagor.system.file_handlers
  exports //
    .Func("get_meta_info_from_screenshot", get_meta_info_from_screenshot)
    ///@param name s : path of the screenshot file
    ///@return DataBlock : datablock with screenshot meta info

    .Func("init_file_drop_handler", init_file_drop_handler)
    /**/;

  module_mgr->addNativeModule("dagor.system.file_handlers", exports);
}

void cleanup_dagor_system_file_handlers() { release_file_drop_handler(); }

} // namespace bindquirrel
