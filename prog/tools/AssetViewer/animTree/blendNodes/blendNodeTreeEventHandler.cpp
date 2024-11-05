// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "blendNodeTreeEventHandler.h"
#include "blendNodeType.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <osApiWrappers/dag_clipboard.h>
#include <ioSys/dag_dataBlock.h>
#include <assets/asset.h>
#include <assets/assetUtils.h>

namespace ContextMenu
{
enum
{
  COPY_NODE_NAME = 0,
  COPY_A2D_NAME,
  COPY_FILEPATH,
  COPY_FOLDER_PATH,
  COPY_RESOLVED_FOLDER_PATH,
  REVEAL_IN_EXPLORER,
};
}

BlendNodeTreeEventHandler::BlendNodeTreeEventHandler(dag::Vector<BlendNodeData> &blend_nodes, dag::Vector<String> &paths) :
  blendNodes(blend_nodes), paths(paths)
{}

bool BlendNodeTreeEventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_ANIM_BLEND_NODES_TREE)
  {
    selLeaf = tree.getSelLeaf();
    nodesTree = &tree;
    const BlendNodeData *data = find_data_by_handle(blendNodes, selLeaf);
    if (data != blendNodes.end())
    {
      PropPanel::IMenu &menu = tree_interface.createContextMenu();
      if (data->type >= BlendNodeType::SINGLE && data->type < BlendNodeType::SIZE)
      {
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy blend node name");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_A2D_NAME, "Copy a2d name");
      }
      else if (data->type == BlendNodeType::A2D)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy a2d name");
      else if (data->type == BlendNodeType::IRQ)
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_NODE_NAME, "Copy irq name");
      else if (data->type == BlendNodeType::INCLUDE)
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

int BlendNodeTreeEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::COPY_NODE_NAME: clipboard::set_clipboard_ansi_text(nodesTree->getCaption(selLeaf)); break;
    case ContextMenu::COPY_A2D_NAME: clipboard::set_clipboard_ansi_text(getA2dName()); break;
    case ContextMenu::COPY_FILEPATH: clipboard::set_clipboard_ansi_text(find_full_path(paths, *asset, nodesTree, selLeaf)); break;
    case ContextMenu::COPY_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_folder_path(paths, nodesTree->getCaption(selLeaf)));
      break;
    case ContextMenu::COPY_RESOLVED_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_resolved_folder_path(paths, *asset, nodesTree, selLeaf));
      break;
    case ContextMenu::REVEAL_IN_EXPLORER: dag_reveal_in_explorer(find_full_path(paths, *asset, nodesTree, selLeaf)); break;
  }
  return 0;
}

void BlendNodeTreeEventHandler::setAsset(DagorAsset *cur_asset) { asset = cur_asset; }

const char *BlendNodeTreeEventHandler::getA2dName()
{
  PropPanel::TLeafHandle includeLeaf = nodesTree->getParentLeaf(selLeaf);
  String fullPath;
  DataBlock props = get_props_from_include_leaf(paths, *asset, nodesTree, includeLeaf, fullPath);
  if (DataBlock *settings = find_a2d_node_settings(props, nodesTree->getCaption(selLeaf)))
    return settings->getStr("a2d", "");

  return "";
}
