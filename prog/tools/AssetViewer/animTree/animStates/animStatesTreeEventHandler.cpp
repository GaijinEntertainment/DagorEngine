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
  DRAW_CHILDS_TREE,
};
}

AnimStatesTreeEventHandler::AnimStatesTreeEventHandler(dag::Vector<AnimStatesData> &states, dag::Vector<AnimCtrlData> &ctrls,
  dag::Vector<String> &paths, ChildsDialog &dialog) :
  states(states), ctrls(ctrls), paths(paths), dialog(dialog)
{}

void add_include_file_menu_items(PropPanel::IMenu &menu)
{
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FILEPATH, "Copy include filepath");
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FOLDER_PATH, "Copy folder path");
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_RESOLVED_FOLDER_PATH, "Copy resolved folder path");
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy include name");
  menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::REVEAL_IN_EXPLORER, "Reveal in Explorer");
}

bool AnimStatesTreeEventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_ANIM_STATES_TREE)
  {
    selLeaf = tree.getSelLeaf();
    statesTree = &tree;
    const AnimStatesData *data = find_data_by_handle(states, selLeaf);
    if (data != states.end() &&
        (data->type == AnimStatesType::ENUM || data->type == AnimStatesType::ENUM_ITEM || data->type == AnimStatesType::ENUM_ROOT ||
          data->type == AnimStatesType::STATE || data->type == AnimStatesType::CHAN || data->type == AnimStatesType::STATE_ALIAS ||
          data->type == AnimStatesType::STATE_DESC || data->type == AnimStatesType::INIT_ANIM_STATE ||
          data->type == AnimStatesType::ROOT_PROPS))
    {
      selData = data;
      PropPanel::IMenu &menu = tree_interface.createContextMenu();
      if (data->type == AnimStatesType::ENUM)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy enum name");
      else if (data->type == AnimStatesType::ENUM_ITEM)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy enum leaf name");
      else if (data->type == AnimStatesType::ENUM_ROOT || data->type == AnimStatesType::INIT_ANIM_STATE ||
               data->type == AnimStatesType::ROOT_PROPS)
        add_include_file_menu_items(menu);
      else // case when select state or chan or state_alias or state_desc leaf
      {
        G_ASSERT(data->type == AnimStatesType::STATE || data->type == AnimStatesType::CHAN ||
                 data->type == AnimStatesType::STATE_ALIAS || data->type == AnimStatesType::STATE_DESC);
        menu.addSubMenu(PropPanel::ROOT_MENU_ITEM, ContextMenu::ADD, "Add");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_STATE, "State");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_CHAN, "Channel");
        menu.addItem(ContextMenu::ADD, ContextMenu::ADD_STATE_ALIAS, "State alias");
        if (data->type == AnimStatesType::STATE)
          menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::DRAW_CHILDS_TREE, "Draw childs tree");
        if (data->type == AnimStatesType::STATE_DESC)
          add_include_file_menu_items(menu);
        else
          menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy node name");
      }
      menu.setEventHandler(this);
      return true;
    }
  }

  return false;
}

static void copy_node_name(PropPanel::ContainerPropertyControl &tree, const AnimStatesData &data)
{
  if (data.type == AnimStatesType::ENUM_ROOT || data.type == AnimStatesType::INIT_ANIM_STATE ||
      data.type == AnimStatesType::ROOT_PROPS || data.type == AnimStatesType::STATE_DESC)
    clipboard::set_clipboard_ansi_text(data.fileName);
  else
    clipboard::set_clipboard_ansi_text(tree.getCaption(data.handle));
}

int AnimStatesTreeEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::COPY_NODE_NAME: copy_node_name(*statesTree, *selData); break;
    case ContextMenu::COPY_FILEPATH: clipboard::set_clipboard_ansi_text(findFullPath()); break;
    case ContextMenu::COPY_FOLDER_PATH: clipboard::set_clipboard_ansi_text(find_folder_path(paths, selData->fileName)); break;
    case ContextMenu::COPY_RESOLVED_FOLDER_PATH: clipboard::set_clipboard_ansi_text(findResolvedFolderPath()); break;
    case ContextMenu::REVEAL_IN_EXPLORER: dag_reveal_in_explorer(findFullPath()); break;
    case ContextMenu::ADD_CHAN: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_CHAN, pluginPanel); break;
    case ContextMenu::ADD_STATE: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_STATE, pluginPanel); break;
    case ContextMenu::ADD_STATE_ALIAS: pluginEventHandler->onClick(PID_ANIM_STATES_ADD_STATE_ALIAS, pluginPanel); break;
    case ContextMenu::DRAW_CHILDS_TREE:
      dialog.fillChildsTree(selLeaf);
      if (!dialog.isVisible())
      {
        if (!dialog.hasEverBeenShown())
          dialog.positionLeftToWindow("Properties", /*use_same_height*/ true);
        dialog.show();
      }
      break;
  }
  return 0;
}

void AnimStatesTreeEventHandler::setAsset(DagorAsset *cur_asset) { asset = cur_asset; }

void AnimStatesTreeEventHandler::setPluginEventHandler(PropPanel::ContainerPropertyControl *panel,
  PropPanel::ControlEventHandler *event_handler)
{
  pluginPanel = panel;
  pluginEventHandler = event_handler;
  ctrlsTree = pluginPanel->getById(PID_ANIM_BLEND_CTRLS_TREE)->getContainer();
}

String AnimStatesTreeEventHandler::findFullPath()
{
  PropPanel::TLeafHandle includeLeaf = findIncludeLeafByStateData();
  if (!includeLeaf)
    return String("");

  return find_full_path(paths, *asset, ctrlsTree, includeLeaf);
}

String AnimStatesTreeEventHandler::findResolvedFolderPath()
{
  PropPanel::TLeafHandle includeLeaf = findIncludeLeafByStateData();
  if (!includeLeaf)
    return String("");

  return find_resolved_folder_path(paths, *asset, ctrlsTree, includeLeaf);
}

PropPanel::TLeafHandle AnimStatesTreeEventHandler::findIncludeLeafByStateData()
{
  PropPanel::TLeafHandle includeLeaf;
  if (selData->fileName == "Root")
    includeLeaf = ctrlsTree->getRootLeaf();
  else
  {
    const AnimCtrlData *ctrlData =
      eastl::find_if(ctrls.begin(), ctrls.end(), [ctrlsTree = ctrlsTree, selData = selData](const AnimCtrlData &data) {
        return data.type == ctrl_type_include && selData->fileName == ctrlsTree->getCaption(data.handle);
      });
    if (ctrlData == ctrls.end())
    {
      logerr("can't find include leaf from ctrls tree with name %s", selData->fileName);
      return nullptr;
    }
    includeLeaf = ctrlData->handle;
  }

  return includeLeaf;
}
