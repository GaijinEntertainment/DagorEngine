// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_startupTex.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_gameResources.h>
#include <3d/dag_texMgr.h>
#include <generic/dag_span.h>
#include <util/dag_globDef.h>

void init_shaders() {}

void init_res_factories()
{
  ::set_gameres_undefined_res_loglevel(LOGLEVEL_ERR);
  ::register_common_game_tex_factories();
  ::set_default_tex_factory(get_stub_tex_factory());
  ::register_geom_node_tree_gameres_factory();
  ::register_character_gameres_factory();
  ::register_animchar_gameres_factory();
  ::register_fast_phys_gameres_factory();
  ::register_a2d_gameres_factory();
  ::register_impostor_data_gameres_factory();

  unsigned stub_types[] = {
    (unsigned)DynModelGameResClassId, (unsigned)RendInstGameResClassId,
    (unsigned)PhysObjGameResClassId, // seems not needed (at least for Cuisine Royale
    //(unsigned)EffectGameResClassId,
  };
  ::set_default_tex_factory(get_stub_tex_factory());
  ::register_stub_gameres_factories(make_span_const(stub_types, countof(stub_types)), /* report as loaded */ false);
}
void term_res_factories() { terminate_stub_gameres_factories(); }

void init_fx() {}

void term_fx() {}
