//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/daEditorE.h>

class SqModules;
class EntityObjEditor;

bool has_in_game_editor();
void init_entity_object_editor();
void init_da_editor4();
void term_da_editor4();
void register_editor_script(SqModules *module_mgr);
bool is_editor_activated();
bool is_editor_in_reload();
void start_editor_reload();
void finish_editor_reload();
bool is_editor_free_camera_enabled();

IDaEditor4EmbeddedComponent &get_da_editor4();
EntityObjEditor *get_entity_obj_editor();
void da_editor4_setup_scene(const char *fpath);
