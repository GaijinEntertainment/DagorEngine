// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "plugin_dlg.h"
#include <oldEditor/de_interface.h>
#include "de_appwnd.h"
#include <oldEditor/de_cm.h>
#include <de3_dynRenderService.h>
#include <drv/3d/dag_driver.h>
#include <workCycle/dag_workCycle.h>

#include <ioSys/dag_dataBlock.h>

#include <debug/dag_debug.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

#define DIALOG_WIDTH  350
#define DIALOG_HEIGHT 768

#define HOTKEYS_BASE 600
#define CHECKS_BASE  300

enum
{
  RADIOBUTTON_COLUMN_WIDTH = 200,
  CHECKBOX_COLUMN_WIDTH = 300,
  COMBOBOX_COLUMN_WIDTH = 100,
};

//==============================================================================
// PluginShowDialog
//==============================================================================
PluginShowDialog::PluginShowDialog(void *phandle, const Tab<String> &tags, const StriMap<DeWorkspace::TabInt> &plugins) :
  DialogWindow(phandle, _pxScaled(DIALOG_WIDTH), _pxScaled(DIALOG_HEIGHT), "Plugins visibility list"),
  plugins(tmpmem),
  unLoadedPlugins(tmpmem),
  hotkeys(tmpmem),
  menuIds(tmpmem),
  tagStr(tags),
  plugTags(plugins),
  mByRadio(false)
{
  mPanel = getPanel();
  G_ASSERT(mPanel && "No panel in CamerasConfigDlg");

  mPanel->setUseFixedWidthColumns();
  mPanelLeft = mPanel->createContainer(PLUGIN_PANEL_LEFT_ID);
  mPanelLeft->setWidth(_pxScaled(RADIOBUTTON_COLUMN_WIDTH));
  mPanelCenter = mPanel->createContainer(PLUGIN_PANEL_CENTER_ID, false, _pxScaled(2));
  mPanelCenter->setWidth(_pxScaled(CHECKBOX_COLUMN_WIDTH));
  mPanelRight = mPanel->createContainer(PLUGIN_PANEL_RIGHT_ID, false, _pxScaled(2));
  mPanelRight->setWidth(_pxScaled(COMBOBOX_COLUMN_WIDTH));

  mPanelLeft->createStatic(SPACE_STATIC_ID, "");
  mPanelCenter->createStatic(PLUGIN_STATIC_ID, "Plugin");
  mPanelRight->createStatic(HOTKEY_STATIC_ID, "Hotkey");

  PropPanel::ContainerPropertyControl *rg = mPanelLeft->createRadioGroup(SELECT_TAG_RADIO_GROUP_ID, "");

  rg->createRadio(SELECT_ALL_RADIO_BUTTON_ID, "Select all");
  rg->createRadio(SELECT_NONE_RADIO_BUTTON_ID, "Select none");
  rg->createRadio(SELECT_CUSTOM_RADIO_BUTTON_ID, "Custom");
  rg->createStatic(0, "");

  for (int i = 0; i < tagStr.size(); ++i)
    rg->createRadio(SELECT_ALL_RADIO_BUTTON_ID + 3 + i, tagStr[i]);

  mPanel->setInt(SELECT_TAG_RADIO_GROUP_ID, SELECT_CUSTOM_RADIO_BUTTON_ID);
}

//==============================================================================
void PluginShowDialog::addPlugin(IGenEditorPlugin *plgn, int *hotkey, int menu_id)
{
  plugins.push_back(plgn);
  hotkeys.push_back(hotkey);
  menuIds.push_back(menu_id);

  int i = plugins.size() - 1;

  mPanelCenter->createCheckBox(CHECKS_BASE + i, plgn->getMenuCommandName(), plgn->getVisible());

  createHotkey(i);
  setHotkeys();
}

//==============================================================================
void PluginShowDialog::createHotkey(int idx, bool disable)
{
  Tab<String> items(midmem);
  for (int i = 0; i < HOTKEY_COUNT; ++i)
    items.push_back() = ::hotkey_names[i];

  mPanelRight->createCombo(HOTKEYS_BASE + idx, "", items, 0);

  if (disable)
    mPanel->setEnabledById(HOTKEYS_BASE + idx, false);
}

//==============================================================================
void PluginShowDialog::setHotkeys()
{
  for (int i = 0; i < hotkeys.size(); ++i)
    for (int j = 0; j < HOTKEY_COUNT; ++j)
      if (::hotkey_values[j] == *hotkeys[i])
        mPanel->setInt(HOTKEYS_BASE + i, j);
}


//==============================================================================
void PluginShowDialog::unsetHotkey(int hotkey)
{
  if (hotkey == ::hotkey_values[0])
    return;

  for (int i = 0; i < hotkeys.size(); ++i)
    if (*hotkeys[i] == hotkey)
      *hotkeys[i] = ::hotkey_values[0];
}


//==============================================================================
PluginShowDialog::~PluginShowDialog()
{
  clear_and_shrink(unLoadedPlugins);
  clear_and_shrink(plugins);
}

void PluginShowDialog::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (pcb_id == SELECT_TAG_RADIO_GROUP_ID)
  {
    Tab<bool> toCheck(tmpmem);
    bool doSet = true;
    toCheck.resize(plugins.size());

    int state = panel->getInt(SELECT_TAG_RADIO_GROUP_ID);

    if (state == SELECT_ALL_RADIO_BUTTON_ID)
    {
      toCheck.assign(toCheck.size(), true);
    }
    else if (state == SELECT_NONE_RADIO_BUTTON_ID)
    {
      toCheck.assign(toCheck.size(), false);
    }
    else if (state == SELECT_CUSTOM_RADIO_BUTTON_ID)
    {
      doSet = false;
    }
    else
    {
      toCheck.assign(toCheck.size(), false);

      String plugName;
      DeWorkspace::TabInt tagIdx;

      for (bool ok = plugTags.getFirst(plugName, tagIdx); ok; ok = plugTags.getNext(plugName, tagIdx))
      {
        for (int ti = 0; ti < tagIdx.size(); ++ti)
        {
          int buttonID = panel->getInt(SELECT_TAG_RADIO_GROUP_ID);
          if (tagIdx[ti] == buttonID - SELECT_ALL_RADIO_BUTTON_ID - 3)
          {
            for (int ci = 0; ci < toCheck.size(); ++ci)
            {
              if (!stricmp(plugName, plugins[ci]->getMenuCommandName()))
              {
                toCheck[ci] = true;
                break;
              }
            }

            break;
          }
        }
      }
    }

    if (doSet)
    {
      for (int ci = 0; ci < toCheck.size(); ++ci)
      {
        mPanel->setBool(CHECKS_BASE + ci, toCheck[ci]);

        mByRadio = true;
        onClick(CHECKS_BASE + ci, panel);
      }
    }
  }
  if (pcb_id >= CHECKS_BASE && pcb_id < CHECKS_BASE + plugins.size())
  {
    const int idx = pcb_id - CHECKS_BASE;
    ;

    plugins[idx]->setVisible(panel->getBool(pcb_id));
    panel->setBool(pcb_id, plugins[idx]->getVisible());
  }
  if (pcb_id >= HOTKEYS_BASE && pcb_id < HOTKEYS_BASE + plugins.size())
  {
    const int idx = pcb_id - HOTKEYS_BASE;
    const int hotkey = ::hotkey_values[panel->getInt(pcb_id)];

    unsetHotkey(hotkey);
    *hotkeys[idx] = hotkey;

    setHotkeys();
  }
}

void PluginShowDialog::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  needRehideIdx = -1;
  if (pcb_id >= CHECKS_BASE && pcb_id < CHECKS_BASE + plugins.size())
  {
    needRehideIdx = pcb_id - CHECKS_BASE;
    bool state = mPanel->getBool(pcb_id);
    plugins[needRehideIdx]->setVisible(state);
    mPanel->setBool(pcb_id, plugins[needRehideIdx]->getVisible());

    if (!mByRadio)
      mPanel->setInt(SELECT_TAG_RADIO_GROUP_ID, SELECT_CUSTOM_RADIO_BUTTON_ID);
    mByRadio = false;
  }
}
