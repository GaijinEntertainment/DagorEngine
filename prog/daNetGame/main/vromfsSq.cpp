// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/vromfs.h"

#include <quirrel/bindQuirrelEx/autoBind.h>


static uint32_t get_updated_game_version_sq() { return get_updated_game_version().value; }


SQ_DEF_AUTO_BINDING_MODULE_EX(bind_vromfs, "vromfs", sq::VM_INTERNAL_UI)
{
  Sqrat::Table tbl(vm);

  tbl //
    .Func("get_vromfs_dump_version", get_vromfs_dump_version)
    .Func("get_updated_game_version", get_updated_game_version_sq)
    /**/;
  return tbl;
}