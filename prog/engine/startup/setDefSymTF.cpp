// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <3d/dag_texMgr.h>
#include <3d/fileTexFactory.h>

void set_default_sym_texture_factory() { set_default_tex_factory(&SymbolicTextureFactory::self); }
