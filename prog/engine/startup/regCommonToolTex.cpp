// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <3d/dag_texMgr.h>

void register_common_tool_tex_factories()
{
  set_default_file_texture_factory();

  register_dds_tex_create_factory();

  register_jpeg_tex_load_factory();
  register_tga_tex_load_factory();
  register_psd_tex_load_factory();
  register_svg_tex_load_factory();
  register_any_tex_load_factory();

  register_loadable_tex_create_factory();
}
