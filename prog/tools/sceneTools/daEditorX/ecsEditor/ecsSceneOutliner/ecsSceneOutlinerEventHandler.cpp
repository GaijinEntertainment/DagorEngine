// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <de3_interface.h>
#include <EditorCore/ec_confirmation_dialog.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <oldEditor/de_interface.h>
#include <oldEditor/de_workspace.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>
#include "ecsSceneOutlinerEventHandler.h"
#include "ecsSceneOutlinerUndoRedo.h"

namespace
{
bool resolveToMountedPath(const String &abs_path, String &mounted_path, String &relative_path)
{
  for (const String &mount : DAGORED2->getWorkspace().getMountPoints())
  {
    if (mount.empty())
    {
      continue;
    }

    bool prefixed = mount[0] == '%';
    if (const char *mountedPath =
          dd_get_named_mount_path(prefixed ? mount.c_str() + 1 : mount, prefixed ? mount.size() - 1 : mount.size()))
    {
      if (eastl::string_view{abs_path.c_str()}.starts_with(mountedPath))
      {
        relative_path = abs_path.c_str() + strlen(mountedPath) + 1;
        mounted_path.printf(0, "%s/%s", mount, relative_path.c_str());
        return true;
      }
    }
  }

  return false;
}
} // namespace

ECSSceneOutlinerPanel::EventHandler::EventHandler(ECSSceneOutlinerPanel &owner) noexcept : owner(owner) {}

bool ECSSceneOutlinerPanel::EventHandler::onTreeContextMenu(PropPanel::ContainerPropertyControl &tree, int pcb_id,
  PropPanel::ITreeInterface &tree_interface)
{
  if (pcb_id == PID_SCENE_TREE)
  {
    PropPanel::IMenu &menu = tree_interface.createContextMenu();

    menu.addItem(PropPanel::ROOT_MENU_ITEM, PID_CM_DELETE, "Delete");
    menu.setEnabledById(PID_CM_DELETE, owner.canSelectedLeafsBeDeleted());

    dag::Vector<PropPanel::TLeafHandle> selectedLeafs;
    tree.getSelectedLeafs(selectedLeafs, true, true);
    if (selectedLeafs.empty())
    {
      return false;
    }

    const auto *userData = reinterpret_cast<LeafUserData *>(tree.getUserData(selectedLeafs[0]));
    if (selectedLeafs.size() == 1 && userData->type == LeafType::SCENE && userData->sid != ecs::Scene::C_INVALID_SCENE_ID)
    {
      menu.addItem(PropPanel::ROOT_MENU_ITEM, PID_CM_ADD_EXISTING_IMPORT, "Add existing");
      menu.addItem(PropPanel::ROOT_MENU_ITEM, PID_CM_ADD_NEW_IMPORT, "Add new");
    }

    menu.setEventHandler(this);
    return !menu.isEmpty();
  }

  return false;
}

int ECSSceneOutlinerPanel::EventHandler::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case PID_CM_DELETE:
    {
      owner.deleteSelectedLeafs();
      break;
    }
    case PID_CM_ADD_EXISTING_IMPORT:
    {
      const String path = wingw::file_open_dlg(nullptr, "Select exisintg scene file", "Objects BLK|*.blk", "blk",
        DAGORED2->getWorkspace().getSceneDir());
      if (!path.empty())
      {
        addImport(path);
      }
      break;
    }
    case PID_CM_ADD_NEW_IMPORT:
    {
      const String path =
        wingw::file_save_dlg(nullptr, "Select scene file", "Objects BLK|*.blk", "blk", DAGORED2->getWorkspace().getSceneDir());

      if (path.empty())
      {
        break;
      }

      if (!dd_file_exist(path))
      {
        if (ConfirmationDialog("Scene file does not exist", "Are you sure you want to create new file?").showDialog() ==
            PropPanel::DIALOG_ID_OK)
        {
          DataBlock().saveToTextFile(path);
        }
        else
        {
          break;
        }
      }

      addImport(path);
      break;
    }
  }

  return id;
}

void ECSSceneOutlinerPanel::EventHandler::addImport(const String &path)
{
  String mountedPath;
  String relativePath;
  if (!resolveToMountedPath(path, mountedPath, relativePath))
  {
    DAEDITOR3.conError("Failed to resolve %s", path);
    return;
  }

  DataBlock block(mountedPath);
  if (block.isValid())
  {
    const PropPanel::TLeafHandle parentLeaf = owner.tree->getSelLeaf();
    const ecs::Scene::SceneId parentId = reinterpret_cast<LeafUserData *>(owner.tree->getUserData(parentLeaf))->sid;
    owner.importRequested = ecs::g_scenes->addImportScene(parentId, block, relativePath);
  }
  else
  {
    DAEDITOR3.conError("Failed to load scene blk %s", mountedPath);
  }
}