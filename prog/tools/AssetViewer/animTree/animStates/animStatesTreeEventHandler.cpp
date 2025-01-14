// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animStatesTreeEventHandler.h"
#include "animStatesType.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <osApiWrappers/dag_clipboard.h>
#include <assets/assetUtils.h>

namespace ContextMenu
{
enum
{
  COPY_NODE_NAME = 0,
  COPY_FILEPATH,
  COPY_FOLDER_PATH,
  COPY_RESOLVED_FOLDER_PATH,
  REVEAL_IN_EXPLORER,
  ADD,
  ADD_STATE,
  ADD_CHAN,
  ADD_STATE_ALIAS,
};
}

AnimStatesTreeEventHandler::AnimStatesTreeEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<String> &paths) :
  states(states), paths(paths)
{}

bool AnimStatesTreeEventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_ANIM_STATES_TREE)
  {
    selLeaf = tree.getSelLeaf();
    statesTree = &tree;
    const AnimStatesData *data = find_data_by_handle(states, selLeaf);
    if (data != states.end() &&
        (data->type == AnimStatesType::ENUM || data->type == AnimStatesType::ENUM_ITEM || data->type == AnimStatesType::INCLUDE_ROOT ||
          data->type == AnimStatesType::STATE || data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS ||
          data->type == AnimStatesType::STATE_DESC))
    {
      PropPanel::IMenu &menu = tree_interface.createContextMenu();
      if (data->type == AnimStatesType::ENUM)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy enum name");
      else if (data->type == AnimStatesType::ENUM_ITEM)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy enum leaf name");
      else if (data->type == AnimStatesType::INCLUDE_ROOT)
      {
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FILEPATH, "Copy include filepath");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FOLDER_PATH, "Copy folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_RESOLVED_FOLDER_PATH, "Copy resolved folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy include name");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::REVEAL_IN_EXPLORER, "Reveal in Explorer");
      }
      else // case when select state or chan or state_alias or state_desc leaf
      {
        G_ASSERT(data->type == AnimStatesType::STATE || data->type == AnimStatesType::CHAN ||
                 data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE_DESC);
        menu.addSubMenu(PropPanel::ROOT_MENU_ITEM, ContextMenu::ADD, "Add");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_STATE, "State");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_CHAN, "Channel");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_STATE_ALIAS, "State alias");
      }
      menu.setEventHandler(this);
      return true;
    }
  }

  return false;
}

int AnimStatesTreeEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::COPY_NODE_NAME: clipboard::set_clipboard_ansi_text(statesTree->getCaption(selLeaf)); break;
    case ContextMenu::COPY_FILEPATH: clipboard::set_clipboard_ansi_text(find_full_path(paths, *asset, statesTree, selLeaf)); break;
    case ContextMenu::COPY_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_folder_path(paths, statesTree->getCaption(selLeaf)));
      break;
    case ContextMenu::COPY_RESOLVED_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_resolved_folder_path(paths, *asset, statesTree, selLeaf));
      break;
    case ContextMenu::REVEAL_IN_EXPLORER: dag_reveal_in_explorer(find_full_path(paths, *asset, statesTree, selLeaf)); break;
    case ContextMenu::ADD_CHAN: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_CHAN, pluginPanel); break;
    case ContextMenu::ADD_STATE: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_STATE, pluginPanel); break;
    case ContextMenu::ADD_STATE_ALIAS: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_STATE_ALIAS, pluginPanel); break;
  }
  return 0;
}

void AnimStatesTreeEventHandler::setAsset(DagorAsset *cur_asset) { asset = cur_asset; }

void AnimStatesTreeEventHandler::setPluginEventHandler(PropPanel::ContainerPropertyControl *panel,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
}
