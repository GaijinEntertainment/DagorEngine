// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ctrlTreeEventHandler.h"
#include "../animTreeUtils.h"
#include "../animTreePanelPids.h"

#include <propPanel/control/container.h>
#include <propPanel/control/menu.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <assets/assetUtils.h>

namespace ContextMenu
{
enum
{
  DRAW_CHILDS_TREE = 0,
  COPY_CONTROLLER_NAME,
  COPY_FILEPATH,
  COPY_FOLDER_PATH,
  COPY_RESOLVED_FOLDER_PATH,
  REVEAL_IN_EXPLORER,
};
}

CtrlTreeEventHandler::CtrlTreeEventHandler(dag::Vector<AnimCtrlData> &controllers, dag::Vector<String> &paths, ChildsDialog &dialog) :
  dialog(dialog), controllers(controllers), paths(paths)
{}

bool CtrlTreeEventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_ANIM_BLEND_CTRLS_TREE)
  {
    selLeaf = tree.getSelLeaf();
    ctrlsTree = &tree;
    const AnimCtrlData *data = find_data_by_handle(controllers, selLeaf);
    if (data != controllers.end())
    {
      PropPanel::IMenu &menu = tree_interface.createContextMenu();
      if (!is_include_ctrl(*data))
      {
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_CONTROLLER_NAME, "Copy controller name");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::DRAW_CHILDS_TREE, "Draw childs tree");
      }
      else
      {
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FILEPATH, "Copy include filepath");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_FOLDER_PATH, "Copy folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_RESOLVED_FOLDER_PATH, "Copy resolved folder path");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::COPY_CONTROLLER_NAME, "Copy include name");
        menu.addItem(PropPanel::ROOT_MENU_ITEM, ContextMenu::REVEAL_IN_EXPLORER, "Reveal in Explorer");
      }
      menu.setEventHandler(this);
      return true;
    }
  }

  return false;
}

int CtrlTreeEventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case ContextMenu::DRAW_CHILDS_TREE:
      dialog.fillChildsTree(selLeaf);
      if (!dialog.isVisible())
      {
        if (!dialog.hasEverBeenShown())
          dialog.positionLeftToWindow("Properties", /*use_same_height*/ true);
        dialog.show();
      }
      break;

    case ContextMenu::COPY_CONTROLLER_NAME: clipboard::set_clipboard_ansi_text(ctrlsTree->getCaption(selLeaf)); break;
    case ContextMenu::COPY_FILEPATH: clipboard::set_clipboard_ansi_text(find_full_path(paths, *asset, ctrlsTree, selLeaf)); break;
    case ContextMenu::COPY_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_folder_path(paths, ctrlsTree->getCaption(selLeaf)));
      break;
    case ContextMenu::COPY_RESOLVED_FOLDER_PATH:
      clipboard::set_clipboard_ansi_text(find_resolved_folder_path(paths, *asset, ctrlsTree, selLeaf));
      break;
    case ContextMenu::REVEAL_IN_EXPLORER: dag_reveal_in_explorer(find_full_path(paths, *asset, ctrlsTree, selLeaf)); break;
  }
  return 0;
}

void CtrlTreeEventHandler::setAsset(DagorAsset *cur_asset) { asset = cur_asset; }
