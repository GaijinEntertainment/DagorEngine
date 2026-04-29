// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// hotkey_index: the hotkey index to edit. Use -1 to add a new hotkey.
void ec_create_single_hotkey_editor(const char *editor_command_id, int hotkey_index);

void ec_update_imgui_single_hotkey_editor();
