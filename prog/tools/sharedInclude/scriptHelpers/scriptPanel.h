// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class IWndManager;


namespace ScriptHelpers
{
bool create_helper_bar(IWndManager *manager);
void rebuild_tree_list();

void *on_wm_create_window(int type);
void removeWindows();
void updateImgui();
void save_params_state(DataBlock &blk);
void load_params_state(DataBlock &blk);
}; // namespace ScriptHelpers
