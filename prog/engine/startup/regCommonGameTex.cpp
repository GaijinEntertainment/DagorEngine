// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <3d/dag_texMgr.h>

void register_common_game_tex_factories()
{
  set_default_file_texture_factory();
  register_dds_tex_create_factory();
}
