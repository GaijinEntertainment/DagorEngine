//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daEditor4/daEditor4.h>

class SqModules;
class EntityObjEditor;

bool has_in_game_editor();
void init_entity_object_editor();
void init_da_editor4();
void term_da_editor4();
void register_editor_script(SqModules *module_mgr);
bool is_editor_activated();

IDaEditor4EmbeddedComponent &get_da_editor4();
EntityObjEditor *get_entity_obj_editor();
void da_editor4_setup_scene(const char *fpath);
