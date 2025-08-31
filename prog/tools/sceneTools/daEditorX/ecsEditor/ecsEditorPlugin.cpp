// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "ecsEditorPlugin.h"
#include "ecsEditorCm.h"

#include <de3_interface.h>
#include <EditorCore/ec_IEditorCore.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <libTools/util/strUtil.h>
#include <oldEditor/de_workspace.h>
#include <osApiWrappers/dag_direct.h>
#include <winGuiWrapper/wgw_dialogs.h>

using editorcore_extapi::dagRender;

static const int MAX_RECENT_COUNT = CM_ECS_EDITOR_RECENT_FILE_END - CM_ECS_EDITOR_RECENT_FILE_START;

void ECSEditorPlugin::loadSettings(const DataBlock &global_settings, const DataBlock &per_app_settings)
{
  const DataBlock *recentBlk = per_app_settings.getBlockByName("recent");
  if (!recentBlk)
    return;

  for (int i = 0; i < recentBlk->paramCount(); ++i)
    if (recentBlk->getParamType(i) == DataBlock::TYPE_STRING && strcmp(recentBlk->getParamName(i), "file") == 0)
    {
      const char *path = recentBlk->getStr(i);
      if (path != nullptr && *path != 0)
      {
        recentCustomPaths.emplace_back(String(path));
        if (recentCustomPaths.size() == MAX_RECENT_COUNT)
          break;
      }
    }
}

void ECSEditorPlugin::saveSettings(DataBlock &global_settings, DataBlock &per_app_settings)
{
  DataBlock *recentBlk = per_app_settings.addNewBlock("recent");
  for (const String &path : recentCustomPaths)
    recentBlk->addStr("file", path.c_str());
}

bool ECSEditorPlugin::begin(int toolbar_id, unsigned menu_id)
{
  PropPanel::ContainerPropertyControl *toolbar = DAGORED2->getCustomPanel(toolbar_id);
  G_ASSERT(toolbar);
  objEd->initUi(toolbar_id);

  menuId = menu_id;
  updateMenu();

  return true;
}

bool ECSEditorPlugin::end()
{
  objEd->closeUi();
  return true;
}

void ECSEditorPlugin::setVisible(bool vis) { isVisible = vis; }

void ECSEditorPlugin::loadObjects(const DataBlock &blk, const DataBlock &local_data, const char *base_path)
{
  objEd->load(getCurrentSceneBlkPath());
  objEd->updateToolbarButtons();
}

void ECSEditorPlugin::saveObjects(DataBlock &blk, DataBlock &local_data, const char *base_path)
{
  objEd->saveObjects(getCurrentSceneBlkPath());
}

void ECSEditorPlugin::clearObjects() { objEd.reset(new ECSObjectEditor()); }

void ECSEditorPlugin::actObjects(float dt)
{
  if (!isVisible)
    return;

  if (DAGORED2->curPlugin() == this)
  {
    objEd->update(dt);

    if (viewportMessageLifeTimeInSeconds >= 0.0f)
    {
      viewportMessageLifeTimeInSeconds -= dt;
      if (viewportMessageLifeTimeInSeconds < 0.0f)
        viewportMessage.clear();
    }
  }
}

void ECSEditorPlugin::beforeRenderObjects(IGenViewportWnd *vp)
{
  if (!isVisible || DAGORED2->curPlugin() != this)
    return;

  objEd->beforeRender();
}

void ECSEditorPlugin::renderObjects()
{
  if (!isVisible || DAGORED2->curPlugin() != this)
    return;

  objEd->render();
}

void ECSEditorPlugin::renderTransObjects()
{
  if (!isVisible || DAGORED2->curPlugin() != this)
    return;

  objEd->renderTrans();
}

void ECSEditorPlugin::handleViewportPaint(IGenViewportWnd *wnd)
{
  if (!isVisible || DAGORED2->curPlugin() != this)
    return;

  IGenEventHandlerWrapper::handleViewportPaint(wnd);

  dagRender->setTextFont(0);

  int width, height;
  wnd->getViewportSize(width, height);

  const String path = getCurrentSceneBlkPath();
  const char *name = get_file_name(path);
  const Point2 textSize = dagRender->getTextBBox(name).size();
  const Point2 textPos = Point2(hdpi::_pxS(10), height - textSize.y);
  const E3DCOLOR bgColor = E3DCOLOR(0, 0, 0, 255);

  dagRender->renderText(textPos.x + 1, textPos.y, bgColor, name);
  dagRender->renderText(textPos.x, textPos.y + 1, bgColor, name);
  dagRender->renderText(textPos.x - 1, textPos.y, bgColor, name);
  dagRender->renderText(textPos.x, textPos.y - 1, bgColor, name);
  dagRender->renderText(textPos.x, textPos.y, E3DCOLOR(255, 255, 100, 255), name);

  if (!viewportMessage.empty())
  {
    int w, h;
    wnd->getViewportSize(w, h);

    E3DCOLOR textColor = COLOR_BLACK;
    if (viewportMessageLifeTimeInSeconds >= 0.0f)
      textColor.a = real2uchar(viewportMessageLifeTimeInSeconds / VIEWPORT_MESSAGE_AUTO_HIDE_DELAY_IN_SECONDS);

    StdGuiRender::set_font(0);
    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_box(hdpi::_pxS(0), h - hdpi::_pxS(30), w, h);
    StdGuiRender::set_color(textColor);
    StdGuiRender::draw_strf_to(hdpi::_pxS(10), h - hdpi::_pxS(10), viewportMessage);
  }
}

void ECSEditorPlugin::updateImgui()
{
  if (DAGORED2->curPlugin() == this)
    objEd->updateImgui();
}

void *ECSEditorPlugin::queryInterfacePtr(unsigned huid)
{
  RETURN_INTERFACE(huid, IOnExportNotify);
  return nullptr;
}

void ECSEditorPlugin::handleViewportAcceleratorCommand(unsigned id) { objEd->onClick(id, nullptr); }

void ECSEditorPlugin::registerMenuAccelerators()
{
  IWndManager &wndManager = *DAGORED2->getWndManager();
  objEd->registerViewportAccelerators(wndManager);
}

String ECSEditorPlugin::getDefaultSceneBlkPath() const
{
  static dag::Vector<String> sceneFolders;

  if (sceneFolders.empty())
  {
    DataBlock appBlk(String(260, "%s/application.blk", DAGORED2->getWorkspace().getAppDir()));
    const DataBlock *sceneFoldersBlk =
      appBlk.getBlockByNameEx("projectDefaults")->getBlockByNameEx("ecsEditor")->getBlockByName("sceneFolders");
    if (sceneFoldersBlk)
      for (int i = 0; i < sceneFoldersBlk->paramCount(); ++i)
        if (strcmp(sceneFoldersBlk->getParamName(i), "folder") == 0 && sceneFoldersBlk->getParamType(i) == DataBlock::TYPE_STRING)
          sceneFolders.push_back(String(sceneFoldersBlk->getStr(i)));

    if (sceneFolders.empty())
      sceneFolders.push_back(String("prog/gamebase/gamedata/scenes"));
  }

  String firstFullPath;
  for (const String &sceneFolder : sceneFolders)
  {
    String name = DAGORED2->getProjectFileName();
    remove_trailing_string(name, ".level.blk");
    String relativePath(DAGOR_MAX_PATH, "%s/%s.blk", sceneFolder.c_str(), name.c_str());
    String fullPath = make_full_path(DAGORED2->getWorkspace().getAppDir(), relativePath);
    if (dd_file_exists(fullPath))
      return fullPath;

    if (firstFullPath.empty())
      firstFullPath = fullPath;
  }

  return firstFullPath;
}

String ECSEditorPlugin::getCurrentSceneBlkPath() const
{
  return customSceneBlkPath.empty() ? getDefaultSceneBlkPath() : customSceneBlkPath;
}

bool ECSEditorPlugin::onPluginMenuClick(unsigned id)
{
  if (id == CM_ECS_EDITOR_LOAD_FROM_DEFAULT_LOCATION)
  {
    loadSceneManually(nullptr);
    return true;
  }
  else if (id == CM_ECS_EDITOR_LOAD_FROM_CUSTOM_LOCATION)
  {
    String directory, fileName;
    split_path(getCurrentSceneBlkPath(), directory, fileName);

    String path = wingw::file_open_dlg(nullptr, "Load scene file", "BLK files|*.blk", "blk", directory, fileName);
    if (!path.empty())
      loadSceneManually(path);

    return true;
  }
  else if (id >= CM_ECS_EDITOR_RECENT_FILE_START && id < CM_ECS_EDITOR_RECENT_FILE_END)
  {
    const int index = id - CM_ECS_EDITOR_RECENT_FILE_START;
    G_ASSERT(index >= 0);
    G_ASSERT(index < MAX_RECENT_COUNT);
    if (index < recentCustomPaths.size())
      loadSceneManually(recentCustomPaths[index]);

    return true;
  }

  return false;
}

int ECSEditorPlugin::getRecentFileIndex(const char *path) const
{
  if (path == nullptr || *path == 0)
    return -1;

  for (int i = 0; i < recentCustomPaths.size(); ++i)
    if (stricmp(recentCustomPaths[i].str(), path) == 0)
      return i;

  return -1;
}

void ECSEditorPlugin::addToRecentList(const char *path)
{
  if (path == nullptr || *path == 0)
    return;

  const int index = getRecentFileIndex(path);
  if (index == 0)
    return;

  if (index > 0)
    recentCustomPaths.erase(recentCustomPaths.begin() + index);
  recentCustomPaths.insert(recentCustomPaths.begin(), String(path));

  if (recentCustomPaths.size() > MAX_RECENT_COUNT)
    recentCustomPaths.erase(recentCustomPaths.begin() + MAX_RECENT_COUNT, recentCustomPaths.end());
}

void ECSEditorPlugin::updateMenu()
{
  G_ASSERT(menuId != 0);

  PropPanel::IMenu *menu = DAGORED2->getMainMenu();
  menu->clearMenu(menuId);

  menu->addItem(menuId, CM_ECS_EDITOR_CURRENT_SCENE_TITLE,
    customSceneBlkPath.empty() ? "Current scene (default):" : "Current scene (custom path):");
  menu->setEnabledById(CM_ECS_EDITOR_CURRENT_SCENE_TITLE, false);
  menu->addItem(menuId, CM_ECS_EDITOR_CURRENT_SCENE_PATH, getCurrentSceneBlkPath());
  menu->setEnabledById(CM_ECS_EDITOR_CURRENT_SCENE_PATH, false);
  menu->addSeparator(menuId);

  menu->addItem(menuId, CM_ECS_EDITOR_LOAD_FROM_DEFAULT_LOCATION, "Load scene from default path");
  menu->addItem(menuId, CM_ECS_EDITOR_LOAD_FROM_CUSTOM_LOCATION, "Load scene from custom path...");

  if (recentCustomPaths.size() > 0)
  {
    menu->addSeparator(menuId);
    menu->addItem(menuId, CM_ECS_EDITOR_RECENT_FILES_TITLE, "Recent custom paths:");
    menu->setEnabledById(CM_ECS_EDITOR_RECENT_FILES_TITLE, false);

    const int currentRecentIndex = getRecentFileIndex(customSceneBlkPath);
    G_ASSERT(recentCustomPaths.size() < MAX_RECENT_COUNT);
    for (int i = 0; i < recentCustomPaths.size(); ++i)
    {
      menu->addItem(menuId, CM_ECS_EDITOR_RECENT_FILE_START + i, recentCustomPaths[i]);
      if (i == currentRecentIndex)
        menu->setCheckById(CM_ECS_EDITOR_RECENT_FILE_START + i);
    }
  }
}

void ECSEditorPlugin::loadSceneManually(const char *custom_path)
{
  const String path(custom_path ? custom_path : getDefaultSceneBlkPath());
  if (!dd_file_exist(path))
  {
    wingw::message_box(wingw::MBS_EXCL, "Scene does not exist",
      String(0, "The file does not exist, cannot load scene.\n\nPath: \"%s\"", path.c_str()));
    return;
  }

  UndoSystem *undoSystem = IEditorCoreEngine::get()->getUndoSystem();
  const bool hasPotentiallyUnsavedChanged = undoSystem && (undoSystem->can_undo() || undoSystem->can_redo() || undoSystem->isDirty());
  if (hasPotentiallyUnsavedChanged && wingw::message_box(wingw::MBS_QUEST | wingw::MBS_OKCANCEL, "Confirm scene opening",
                                        "Load the selected scene?\n\n(Unsaved changes will be lost.)") != wingw::MB_ID_OK)
    return;

  if (custom_path)
  {
    customSceneBlkPath = path;
    G_ASSERT(!customSceneBlkPath.empty());
    addToRecentList(customSceneBlkPath);
  }
  else
  {
    customSceneBlkPath.clear();
  }

  objEd->removeAllObjects(false);
  objEd->load(getCurrentSceneBlkPath());
  objEd->updateToolbarButtons();
  updateMenu();
}

void ECSEditorPlugin::setViewportMessage(const char *message, bool auto_hide)
{
  viewportMessage = message;
  viewportMessageLifeTimeInSeconds = auto_hide ? VIEWPORT_MESSAGE_AUTO_HIDE_DELAY_IN_SECONDS : -1.0f;
}

void ECSEditorPlugin::onBeforeExport(unsigned target_code) { objEd->onBeforeExport(); }

void ECSEditorPlugin::onAfterExport(unsigned target_code) { objEd->onAfterExport(); }
