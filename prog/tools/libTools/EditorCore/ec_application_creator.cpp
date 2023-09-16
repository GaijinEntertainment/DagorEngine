#include <EditorCore/ec_application_creator.h>
#include <EditorCore/ec_workspace.h>

#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>

#include <stdio.h>
#include <windows.h>

#include <osApiWrappers/dag_direct.h>
#include <debug/dag_debug.h>

#include <winGuiWrapper/wgw_dialogs.h>
#include <sepGui/wndGlobal.h>


enum
{
  PID_APP_GRP,
  PID_APP_NAME,
  PID_APP_FOLDER,
  PID_APP_TEXT,
  PID_APP_PATH,
  PID_ALREADY_EXISTS,
  PID_CREATE_GRP,
  PID_OBJLIB,
  PID_RES_BASE,
  PID_RES_BASE_BIN,
  PID_LEVELS,
  PID_HEIGHTMAP,
  PID_SCRIPTS,
  PID_SHADERS,
  PID_CONFIG,
  PID_PHYS_ATTACK,

  APP_DIALOG_WIDTH = 300,
  APP_DIALOG_HEIGHT = 460,
};


//==================================================================================================
static bool createFolder(const char *path)
{
  if (!::dd_dir_exist(path))
    if (!::dd_mkdir(path))
    {
      wingw::message_box(wingw::MBS_HAND, "Disk error", "Couldn't create folder \"%s\"", path);
      return false;
    }

  return true;
}


//==================================================================================================
static bool copyFolder(const char *dest_folder, const char *folder)
{
  Tab<String> ignore(tmpmem);
  ignore.push_back() = "CVS";

  const String src = ::make_full_path(sgg::get_exe_path_full(), String("../common/samples/") + folder);
  const String dest = ::make_full_path(dest_folder, folder);

  if (!::dag_copy_folder_content(src, dest, ignore))
  {
    wingw::message_box(wingw::MBS_HAND, "Disk error", "Couldn't copy folder \"%s\" content into the folder \"%s\".", (const char *)src,
      (const char *)dest);

    return false;
  }

  return true;
}


//==================================================================================================
static bool createFolderContent(const char *app_folder, const char *folder, PropertyContainerControlBase &panel, int pid)
{
  if (panel.getBool(pid))
  {
    String path = ::make_full_path(app_folder, folder);

    if (!createFolder(path))
      return false;

    if (!copyFolder(app_folder, folder))
      return false;
  }

  return true;
}


//==================================================================================================
static bool copyFile(const char *dest_folder, const char *file)
{
  const String src = ::make_full_path(sgg::get_exe_path(), String("../common/samples/") + file);
  const String dest = ::make_full_path(dest_folder, file);

  if (!::dag_copy_file(src, dest))
  {
    wingw::message_box(wingw::MBS_HAND, "Disk error", "Couldn't copy file \"%s\" into the file \"%s\".", (const char *)src,
      (const char *)dest);
    return false;
  }

  return true;
}


//==================================================================================================

ApplicationCreator::ApplicationCreator(void *phandle, EditorWorkspace &w) :

  CDialogWindow(phandle, APP_DIALOG_WIDTH, APP_DIALOG_HEIGHT, "Create application"), wsp(w)
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ApplicationCreator");

  PropertyContainerControlBase *_grp = _panel->createGroupBox(PID_APP_GRP, "Application settings");

  G_ASSERT(_grp);


  _grp->createEditBox(PID_APP_NAME, "Application name:");
  _grp->createFileEditBox(PID_APP_FOLDER, "Folder where application will be created:");
  _grp->setBool(PID_APP_FOLDER, true); // select directories

  _grp->createStatic(PID_APP_TEXT, "Full application path:");
  _grp->createStatic(PID_APP_PATH, "");
  _grp->createStatic(PID_ALREADY_EXISTS, "");

  PropertyContainerControlBase *_grp_samples = _panel->createGroupBox(PID_CREATE_GRP, "Create samples");
  G_ASSERT(_grp_samples);

  String path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/library");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_OBJLIB, "Object library", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/resource");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_RES_BASE, "Resources database", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/game/res");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_RES_BASE_BIN, "Resources database binaries", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/levels");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_LEVELS, "Sample levels", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/heightmap");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_HEIGHTMAP, "Sample heightmap scripts", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/scripts");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_SCRIPTS, "Sripts scheme", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/compile_shaders.bat");
  if (::dd_file_exist(path))
  {
    path = ::make_full_path(sgg::get_exe_path(), "../common/samples/game/compiledShaders");
    if (::dd_dir_exist(path))
    {
      _grp_samples->createCheckBox(PID_SHADERS, "Compile shaders routine", true);
    }
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/game/config");
  if (::dd_dir_exist(path))
  {
    _grp_samples->createCheckBox(PID_CONFIG, "Game configuration files", true);
  }

  path = ::make_full_path(sgg::get_exe_path(), "../common/samples/develop/phys_attacks.scheme.blk");
  if (::dd_file_exist(path))
  {
    _grp_samples->createCheckBox(PID_PHYS_ATTACK, "phys_attacks.scheme.blk", true);
  }
}

//==================================================================================================

bool ApplicationCreator::onOk()
{
  PropertyContainerControlBase *_panel = getPanel();
  G_ASSERT(_panel && "No panel in ApplicationCreator");

  SimpleString app(_panel->getText(PID_APP_PATH));

  if (app.empty())
  {
    wingw::message_box(0, "Application error", "Path is empty");
    return false;
  }

  String appFolder(app);
  ::location_from_path(appFolder);

  if (::dd_dir_exist(appFolder))
  {
    wingw::message_box(0, "Application error", "Application folder is exists");
    return false;
  }

  String path;
  path.reserve(512);

  if (!createFolder(appFolder))
    return false;

  if (!createFolderContent(appFolder, "develop/library", *_panel, PID_OBJLIB))
    return false;

  if (!createFolderContent(appFolder, "develop/resource", *_panel, PID_RES_BASE))
    return false;

  if (!createFolderContent(appFolder, "game/res", *_panel, PID_RES_BASE_BIN))
    return false;

  if (!createFolderContent(appFolder, "develop/levels", *_panel, PID_LEVELS))
    return false;

  if (!createFolderContent(appFolder, "develop/heightmap", *_panel, PID_HEIGHTMAP))
    return false;

  if (!createFolderContent(appFolder, "develop/scripts", *_panel, PID_SCRIPTS))
    return false;

  if (!createFolderContent(appFolder, "game/compiledShaders", *_panel, PID_SHADERS))
    return false;

  if (_panel->getBool(PID_SHADERS))
    if (!copyFile(appFolder, "develop/compile_shaders.bat"))
      return false;

  if (!createFolderContent(appFolder, "game/config", *_panel, PID_CONFIG))
    return false;

  if (_panel->getBool(PID_PHYS_ATTACK))
    if (!copyFile(appFolder, "develop/phys_attacks.scheme.blk"))
      return false;

  if (!copyFile(appFolder, "application.blk"))
    return false;

  wsp.setAppPath(app);
  wsp.setName(_panel->getText(PID_APP_NAME).str());

  return true;
}

//==================================================================================================

void ApplicationCreator::onChange(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case PID_APP_FOLDER:
    case PID_APP_NAME: correctAppPath(*panel); break;
  }
}


//==================================================================================================

void ApplicationCreator::correctAppPath(PropertyContainerControlBase &panel)
{
  String appName(panel.getText(PID_APP_NAME));
  String appFolder(panel.getText(PID_APP_FOLDER));

  String path(appFolder);
  if (appName.length())
    path = ::make_full_path(path, appName);

  const char *alreadyExistsStr = "Already exists";
  const char *blankStr = "";

  if (::dd_dir_exist(path))
  {
    panel.setText(PID_ALREADY_EXISTS, alreadyExistsStr);
  }
  else
  {
    panel.setText(PID_ALREADY_EXISTS, blankStr);
  }

  path = ::make_full_path(path, "application.blk");

  panel.setText(PID_APP_PATH, path.str());
}
