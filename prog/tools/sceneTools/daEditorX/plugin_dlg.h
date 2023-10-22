#ifndef __GAIJIN_DAGORED_PLUGIN_DLG_H__
#define __GAIJIN_DAGORED_PLUGIN_DLG_H__
#pragma once

#include <oldEditor/de_workspace.h>

#include <propPanel2/comWnd/dialog_window.h>

#include <libTools/containers/dag_StrMap.h>


class IGenEditorPlugin;

enum
{
  SELECT_TAG_RADIO_GROUP_ID = 800,
  CHECK_COMBO_GROUP_ID,
  HOTKEY_STATIC_ID,

  SELECT_ALL_RADIO_BUTTON_ID,
  SELECT_NONE_RADIO_BUTTON_ID,
  SELECT_CUSTOM_RADIO_BUTTON_ID,
};

//==============================================================================
class PluginShowDialog : public CDialogWindow
{
public:
  PluginShowDialog(void *phandle, const Tab<String> &tags, const StriMap<DeWorkspace::TabInt> &plugins);
  ~PluginShowDialog();

  void addPlugin(IGenEditorPlugin *plgn, int *hotkey, int menu_id);

  virtual void onChange(int pcb_id, PropPanel2 *panel);
  virtual void onClick(int pcb_id, PropPanel2 *panel);

  void autoSize();

private:
  void createHotkey(int idx, bool disable = false);
  void setHotkeys();
  void unsetHotkey(int hotkey);
  void resizeTable();
  void resizeLine(int index);

  int needRehideIdx;
  Tab<IGenEditorPlugin *> plugins;
  Tab<String> unLoadedPlugins;
  Tab<int *> hotkeys;
  Tab<int> menuIds;
  bool centerWnd;

  const Tab<String> &tagStr;
  const StriMap<DeWorkspace::TabInt> &plugTags;

  PropertyContainerControlBase *mPanel, *mCheckComboPanel;

  bool mByRadio;
};


#endif