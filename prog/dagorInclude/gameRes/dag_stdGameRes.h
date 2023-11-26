//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gameRes/dag_stdGameResId.h>
#include <generic/dag_tabFwd.h>
class DataBlock;


void register_a2d_gameres_factory();
void register_dynmodel_gameres_factory(bool use_united_vdata = false, bool allow_united_vdata_rebuild = true);
void register_rendinst_gameres_factory(bool use_united_vdata_streaming = true);
void register_rndgrass_gameres_factory();
void register_geom_node_tree_gameres_factory();
void register_character_gameres_factory();
void register_fast_phys_gameres_factory();
void register_phys_sys_gameres_factory();
void register_phys_obj_gameres_factory();
void register_ragdoll_gameres_factory();
void register_effect_gameres_factory();
void register_isl_light_gameres_factory();
void register_animchar_gameres_factory(bool warn_about_missing_anim = true);
void register_cloud_gameres_factory();
void register_material_gameres_factory();
void register_lshader_gameres_factory();
void register_impostor_data_gameres_factory();

class ParamScriptsPool *get_effect_scripts_pool();


extern DataBlock gameres_rendinst_desc;
extern DataBlock gameres_dynmodel_desc;

void gameres_append_desc(DataBlock &desc, const char *desc_fn, const char *pkg_folder, bool allow_override = false);
void gameres_patch_desc(DataBlock &desc, const char *patch_desc_fn, const char *pkg_folder, const char *desc_fn);
void gameres_final_optimize_desc(DataBlock &desc, const char *label);

void register_stub_gameres_factories(dag::ConstSpan<unsigned> stubbed_types);
void terminate_stub_gameres_factories();
