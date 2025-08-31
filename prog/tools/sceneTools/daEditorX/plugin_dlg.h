// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_workspace.h>

#include <propPanel/commonWindow/dialogWindow.h>

#include <libTools/containers/dag_StrMap.h>


class IGenEditorPlugin;

enum
{
  SELECT_TAG_RADIO_GROUP_ID = 800,
  PLUGIN_PANEL_LEFT_ID,
  PLUGIN_PANEL_CENTER_ID,
  PLUGIN_PANEL_RIGHT_ID,
  SPACE_STATIC_ID,
  PLUGIN_STATIC_ID,
  HOTKEY_STATIC_ID,

  SELECT_ALL_RADIO_BUTTON_ID,
  SELECT_NONE_RADIO_BUTTON_ID,
  SELECT_CUSTOM_RADIO_BUTTON_ID,
};

//==============================================================================
class PluginShowDialog : public PropPanel::DialogWindow
{
public:
  PluginShowDialog(void *phandle, const Tab<String> &tags, const StriMap<DeWorkspace::TabInt> &plugins);
  ~PluginShowDialog() override;

  void addPlugin(IGenEditorPlugin *plgn, int *hotkey, int menu_id);

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  void createHotkey(int idx, bool disable = false);
  void setHotkeys();
  void unsetHotkey(int hotkey);

  int needRehideIdx;
  Tab<IGenEditorPlugin *> plugins;
  Tab<String> unLoadedPlugins;
  Tab<int *> hotkeys;
  Tab<int> menuIds;
  bool centerWnd;

  const Tab<String> &tagStr;
  const StriMap<DeWorkspace::TabInt> &plugTags;

  PropPanel::ContainerPropertyControl *mPanel, *mPanelLeft, *mPanelCenter, *mPanelRight;

  bool mByRadio;
};
