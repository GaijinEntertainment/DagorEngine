// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "nodeMaskTreeEventHandler.h"
#include "nodeMaskType.h"
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
};
}

NodeMaskTreeEventHandler::NodeMaskTreeEventHandler(dag::Vector<NodeMaskData> &node_masks, dag::Vector<String> &paths) :
  nodeMasks(node_masks), paths(paths)
{}

bool NodeMaskTreeEventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_NODE_MASKS_TREE)
  {
    selLeaf = tree.getSelLeaf();
    masksTree = &tree;
    const NodeMaskData *data = find_data_by_handle(nodeMasks, selLeaf);
    if (data != nodeMasks.end())
    {
      PropPanel::IMenu &menu = tree_interface.createContextMenu();
      if (data->type == NodeMaskType::MASK)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy node mask name");
      else if (data->type == NodeMaskType::LEAF)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy node name");
      else if (data->type == NodeMaskType::INCLUDE)
      {
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FILEPATH, "Copy include filepath");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FOLDER_PATH, "Copy folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_RESOLVED_FOLDER_PATH, "Copy resolved folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy include name");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::REVEAL_IN_EXPLORER, "Reveal in Explorer");
      }
      menu.setEventHandler(this);
      return true;
    }
  }

  return false;
}

int NodeMaskTreeEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::COPY_NODE_NAME: clipboard::set_clipboard_ansi_text(masksTree->getCaption(selLeaf)); break;
    case ContextMenu::COPY_FILEPATH: clipboard::set_clipboard_ansi_text(find_full_path(paths, *asset, masksTree, selLeaf)); break;
    case ContextMenu::COPY_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_folder_path(paths, masksTree->getCaption(selLeaf)));
      break;
    case ContextMenu::COPY_RESOLVED_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_resolved_folder_path(paths, *asset, masksTree, selLeaf));
      break;
    case ContextMenu::REVEAL_IN_EXPLORER: dag_reveal_in_explorer(find_full_path(paths, *asset, masksTree, selLeaf)); break;
  }
  return 0;
}

void NodeMaskTreeEventHandler::setAsset(DagorAsset *cur_asset) { asset = cur_asset; }
