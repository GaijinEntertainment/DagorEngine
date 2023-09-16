//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gameRes/dag_gameResources.h>

#include <util/dag_string.h>
#include <generic/dag_tab.h>

//! returns list of accesible resources
void get_loaded_game_resource_names(Tab<String> &names);

//! returns name of resource type
const char *get_resource_type_name(unsigned id);

//! returns list of GRPs needed to load all specified resources
//! (optionally, with all referenced resources)
void get_used_gameres_packs(dag::ConstSpan<const char *> needed_res, bool check_ref, Tab<String> &out_grp_filenames);
