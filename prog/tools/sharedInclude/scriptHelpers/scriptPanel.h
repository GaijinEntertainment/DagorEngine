// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepGui/wndCommon.h>

class IWndManager;


namespace ScriptHelpers
{
bool create_helper_bar(IWndManager *manager);
void rebuild_tree_list();

void *on_wm_create_window(int type);
void removeWindows();
void updateImgui();
}; // namespace ScriptHelpers
