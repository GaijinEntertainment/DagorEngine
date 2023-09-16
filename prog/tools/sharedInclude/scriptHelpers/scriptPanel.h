#ifndef __GAIJIN_SCRIPTHELPERS_SCRIPTPANEL_H__
#define __GAIJIN_SCRIPTHELPERS_SCRIPTPANEL_H__
#pragma once

#include <sepGui/wndCommon.h>

class IWndManager;
class IWndEmbeddedWindow;


namespace ScriptHelpers
{
bool create_helper_bar(IWndManager *manager, void *htree, void *hpropbar);
void rebuild_tree_list();

IWndEmbeddedWindow *on_wm_create_window(void *handle, int type);
bool on_wm_destroy_window(void *handle);
}; // namespace ScriptHelpers


#endif
