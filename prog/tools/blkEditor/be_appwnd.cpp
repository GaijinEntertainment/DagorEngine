// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "be_appwnd.h"
#include "be_cm.h"
#include "windows.h"
#include <commctrl.h>

#include <debug/dag_debug.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_renderTarget.h>
#include <EditorCore/ec_imguiInitialization.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndGlobal.h>
#include <gui/dag_imgui.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/blkUtil.h>
#include <osApiWrappers/dag_direct.h>
#include <propPanel/commonWindow/dialogManager.h>
#include <propPanel/control/menu.h>
#include <propPanel/control/panelWindow.h>
#include <propPanel/c_util.h>
#include <propPanel/colors.h>
#include <propPanel/constants.h>
#include <propPanel/messageQueue.h>
#include <propPanel/propPanel.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <startup/dag_restart.h>
#include <startup/dag_startupTex.h>
#include <workCycle/dag_gameScene.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <regExp/regExp.h>

using hdpi::_pxActual;
using hdpi::_pxScaled;

IEditorCoreEngine *IEditorCoreEngine::__global_instance = nullptr;

enum
{
  SCINTILLA_WIDTH = 600,
  PPANEL_WIDTH = 280,
  TOOLBAR_HEIGHT = 26,
  TOOLBAR_WIDTH = 400,
};


enum
{
  PPANEL_TYPE,
  TOOLBAR_TYPE,
  SCINTILLA_TYPE,
};

const char APP_NAME[] = "Datablock Editor";

class BlkEditorScene : public DagorGameScene
{
public:
  BlkEditorScene(BlkEditorApp &blk_editor_app, IWndManager &wnd_manager) : blkEditorApp(blk_editor_app), wndManager(wnd_manager) {}

  void actScene() override { PropPanel::process_message_queue(); }

  void drawScene() override
  {
    wndManager.updateImguiMouseCursor();

    unsigned clientWidth = 0;
    unsigned clientHeight = 0;
    wndManager.getWindowClientSize(wndManager.getMainWindow(), clientWidth, clientHeight);

    // They can be zero when toggling the application's window between minimized and maximized state.
    if (clientWidth == 0 || clientHeight == 0)
      return;

    imgui_update(clientWidth, clientHeight);
    PropPanel::after_new_frame();

    blkEditorApp.updateImgui();

    PropPanel::before_end_frame();
    imgui_endframe();

    {
      d3d::GpuAutoLock gpuLock;

      d3d::set_render_target();
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      imgui_render();
      d3d::pcwin::set_present_wnd(wndManager.getMainWindow());
    }
  }

  void sceneDeselected(DagorGameScene * /*new_scene*/) override { delete this; }

private:
  BlkEditorApp &blkEditorApp;
  IWndManager &wndManager;
};

BlkEditorApp::BlkEditorApp(IWndManager *manager, const char *open_fname) : blkEditor(this)
{
  mManager = manager;
  openFileName = open_fname;
  scintillaOldText = "";
  ignoreFoldingAndScroll = false;

  mPropPanel = NULL;
  mScintillaPanel = NULL;
  mToolPanel = NULL;

  mManager->registerWindowHandler(this);
  console = new CoolConsole();
  undoSystem = create_undo_system("CasualGuiEditorUndo", 10 << 20, this);
  console->initConsole();

  mainMenu.reset(PropPanel::create_menu());
  mainMenu->setEventHandler(this);
}


BlkEditorApp::~BlkEditorApp()
{
  delete console;
  del_it(undoSystem);

  mManager->unregisterWindowHandler(this);
}


void BlkEditorApp::init3d()
{
  dgs_all_shader_vars_optionals = true;
  mManager->init3d("auto", nullptr);

  ::startup_shaders(String(260, "%s/../commonData/guiShaders", sgg::get_exe_path_full()));
  ::startup_game(RESTART_ALL);
  ShaderGlobal::enableAutoBlockChange(true);

  set_default_file_texture_factory();
  register_loadable_tex_create_factory();
  register_tga_tex_load_factory();
}


void BlkEditorApp::init()
{
  DEBUG_CP();
  const HINSTANCE inst = ::GetModuleHandle(NULL);

  String imgPath(512, "%s%s", sgg::get_exe_path_full(), "../commonData/icons/");
  ::dd_simplify_fname_c(imgPath.str());
  PropPanel::p2util::set_icon_path(imgPath.str());
  PropPanel::p2util::set_main_parent_handle(mManager->getMainWindow());

  // accel
  mManager->addAccelerator(CM_SAVE, ImGuiMod_Ctrl | ImGuiKey_S);
  mManager->addAccelerator(CM_UNDO, ImGuiMod_Ctrl | ImGuiKey_Z);
  mManager->addAccelerator(CM_REDO, ImGuiMod_Ctrl | ImGuiKey_Y);
  mManager->addAccelerator(CM_NEW, ImGuiMod_Ctrl | ImGuiKey_N);
  mManager->addAccelerator(CM_LOAD, ImGuiMod_Ctrl | ImGuiKey_L);

  fillMenu(getMainMenu());
  updateMenu(getMainMenu());
  getMainMenu()->setEventHandler(this);

  updateWindowText();
  mManager->show();
  makeDefaultLayout();

  init3d();
  editor_core_initialize_imgui(nullptr); // The path is not set to avoid saving the settings.
  dagor_select_game_scene(new BlkEditorScene(*this, *mManager));

  // Needed because of the Scintilla child window.
  const HWND mainWindow = (HWND)mManager->getMainWindow();
  SetWindowLong(mainWindow, GWL_STYLE, GetWindowLong(mainWindow, GWL_STYLE) | WS_CLIPCHILDREN);
}


void BlkEditorApp::updateWindowText()
{
  String caption(512, "%s %s", APP_NAME, openFileName.str());
  mManager->setMainWindowCaption(caption);
}


bool BlkEditorApp::canClose()
{
  SimpleString scintillaText(blkEditor.getBlkText());
  if (strcmp(scintillaText.str(), scintillaOldText.str()) != 0)
  {
    int ret = wingw::message_box(wingw::MBS_YESNOCANCEL, "Exit", "Save changes?");
    if (ret == wingw::MB_ID_YES)
      onMenuItemClick(CM_SAVE);
    if (ret == wingw::MB_ID_CANCEL)
      return false;
  }
  return true;
}


PropPanel::IMenu *BlkEditorApp::getMainMenu() { return mainMenu.get(); }


PropPanel::ContainerPropertyControl *BlkEditorApp::getBlkPanel() { return mPropPanel; }


void BlkEditorApp::makeDefaultLayout()
{
  mManager->reset();
  mManager->setWindowType(nullptr, PPANEL_TYPE);
  mManager->setWindowType(nullptr, SCINTILLA_TYPE);
  mManager->setWindowType(nullptr, TOOLBAR_TYPE);
}


void *BlkEditorApp::onWmCreateWindow(int type)
{
  switch (type)
  {
    case SCINTILLA_TYPE:
    {
      if (mScintillaPanel)
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_SCINTILLA, false);
      mScintillaPanel = new CScintillaWindow(this, mManager->getMainWindow(), 0, 0, _pxActual(100), _pxActual(100), "Scintilla");
      mScintillaPanel->setText(blkEditor.getBlkText());
      return mScintillaPanel;
    }
    break;

    case PPANEL_TYPE:
    {
      if (mPropPanel)
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_PPANEL, false);
      mPropPanel =
        new PropPanel::PanelWindowPropertyControl(0, nullptr, nullptr, 0, 0, _pxActual(0), _pxActual(0), propertiesWindowName);
      blkEditor.createPanel();

      return mPropPanel;
    }
    break;

    case TOOLBAR_TYPE:
    {
      if (mToolPanel)
        return NULL;

      getMainMenu()->setEnabledById(CM_WINDOW_TOOLBAR, false);

      mToolPanel = new PropPanel::ContainerPropertyControl(0, this, nullptr, 0, 0, _pxActual(0), _pxActual(0));
      fillToolBar();

      return mToolPanel;
    }
    break;

    default: return NULL;
  }
}


bool BlkEditorApp::onWmDestroyWindow(void *handle)
{
  if (mScintillaPanel && mScintillaPanel->getParentWindowHandle() == handle)
  {
    del_it(mScintillaPanel);
    mainMenu->setEnabledById(CM_WINDOW_SCINTILLA, true);
    return true;
  }

  if (mPropPanel && mPropPanel->getParentWindowHandle() == handle)
  {
    blkEditor.destroyPanel();
    del_it(mPropPanel);
    mainMenu->setEnabledById(CM_WINDOW_PPANEL, true);
    return true;
  }

  if (mToolPanel && mToolPanel->getParentWindowHandle() == handle)
  {
    del_it(mToolPanel);
    mainMenu->setEnabledById(CM_WINDOW_TOOLBAR, true);
    return true;
  }

  return false;
}


bool BlkEditorApp::createNew(const char *filename) { return true; }


bool BlkEditorApp::load(const char *filename) { return true; }


bool BlkEditorApp::save(const char *filename) { return true; }


void BlkEditorApp::repaint() {}


void BlkEditorApp::fillToolBar()
{
  PropPanel::ContainerPropertyControl *tool = mToolPanel->createToolbarPanel(CM_TOOLS, "");

  tool->setEventHandler(this);

  tool->createButton(CM_NEW, "New (Ctrl+N)");
  tool->setButtonPictures(CM_NEW, "show_panel");
  tool->createButton(CM_LOAD, "Load (Ctrl+L)");
  tool->setButtonPictures(CM_LOAD, "obj_lib");
  tool->createButton(CM_SAVE, "Save (Ctrl+S)");
  tool->setButtonPictures(CM_SAVE, "save_mission");
  tool->createButton(CM_SAVE_AS, "Save as...");
  tool->setButtonPictures(CM_SAVE_AS, "save_mission_as");
  tool->createButton(CM_CHANGE_SCHEME, "Change scheme");
  tool->setButtonPictures(CM_CHANGE_SCHEME, "asset_composit");


  tool->createSeparator();

  // copy
  // cut
  // paste

  tool->createButton(CM_CONSOLE, "Show/hide console");
  tool->setButtonPictures(CM_CONSOLE, "console");
}


void BlkEditorApp::fillMenu(PropPanel::IMenu *menu)
{
  using PropPanel::ROOT_MENU_ITEM;
  menu->clearMenu(ROOT_MENU_ITEM);

  // fill file menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_FILE_MENU, "File");
  menu->addItem(CM_FILE_MENU, CM_NEW, "New \tCrtl+N");
  menu->addItem(CM_FILE_MENU, CM_LOAD, "Load \tCrtl+L");
  menu->addItem(CM_FILE_MENU, CM_SAVE, "Save \tCrtl+S");
  menu->addItem(CM_FILE_MENU, CM_SAVE_AS, "Save as ...");
  menu->addItem(CM_FILE_MENU, CM_CHANGE_SCHEME, "Change scheme");


  // Edit
  menu->addSubMenu(ROOT_MENU_ITEM, CM_EDIT, "Edit");
  menu->addItem(CM_EDIT, CM_UNDO, "Undo\tCtrl+Z");
  menu->addItem(CM_EDIT, CM_REDO, "Redo\tCtrl+Y");
  menu->setEnabledById(CM_UNDO, false);
  menu->setEnabledById(CM_REDO, false);
  updateUndoRedoMenu();

  // fill view menu
  menu->addSubMenu(ROOT_MENU_ITEM, CM_WINDOW, "View");
  menu->addItem(CM_WINDOW, CM_WINDOW_TOOLBAR, "Toolbar");
  menu->addItem(CM_WINDOW, CM_WINDOW_SCINTILLA, "Scintilla");
  menu->addItem(CM_WINDOW, CM_WINDOW_PPANEL, "Properties");

  menu->setEnabledById(CM_WINDOW_TOOLBAR, false);
  menu->setEnabledById(CM_WINDOW_SCINTILLA, false);
  menu->setEnabledById(CM_WINDOW_PPANEL, false);

  menu->addItem(ROOT_MENU_ITEM, CM_EXIT, "Exit");
}


void BlkEditorApp::updateMenu(PropPanel::IMenu *menu) {}


void BlkEditorApp::updateUndoRedoMenu() {}


void BlkEditorApp::onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel)
{
  if (onMenuItemClick(pcb_id))
    return;

  switch (pcb_id)
  {
    case CM_CONSOLE: console->showConsole(true); break;
  }
}


void BlkEditorApp::onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) {}


int BlkEditorApp::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case CM_EXIT: mManager->close(); return 1;

    case CM_NEW:
      if (canClose())
      {
        String path = wingw::file_open_dlg(NULL, "Select script for datablock",
          "Scripts (*.nut)|*.nut|"
          "All files(*.*)|*.*",
          "nut", blkEditor.getScriptsDir());

        if (path.length() > 0)
        {
          ignoreFoldingAndScroll = true;
          openFileName = "";
          String simple_path = ::make_path_relative(path.str(), blkEditor.getScriptsDir());
          String blk_text(512, "__scheme:t=\"%s\"\n", simple_path.str());
          blkEditor.destroyPanel();
          blkEditor.setBlkText(blk_text.str(), blk_text.length());
          updateWindowText();
          scintillaOldText = "";
        }
      }
      return 1;

    case CM_LOAD:
      if (canClose())
      {
        String path = wingw::file_open_dlg(NULL, "Open datablock...",
          "Datablocks (*.blk)|*.blk|"
          "All files(*.*)|*.*",
          "blk", sgg::get_exe_path_full(), openFileName.str());
        if (path.length() > 0)
        {
          ignoreFoldingAndScroll = true;
          openFileName = path;
          blkEditor.load(path.str());
          scintillaOldText = blkEditor.getBlkText();
          if (!blkEditor.hasScript())
          {
            SimpleString schemeName =
              getSchemeFromFn(::dd_get_fname(path), getSchemePathFromFn(::dd_get_fname(path), "../commonData/scripts/"));
            if (!schemeName.empty())
            {
              String schemeFullPath(512, "%s%s", sgg::get_exe_path_full(), schemeName.str());
              ::dd_simplify_fname_c(schemeFullPath.str());
              if (::dd_file_exist(schemeFullPath.str()))
              {
                blkEditor.setScriptPath(schemeFullPath.str());
                onBlkChange();
              }
            }
            else
              changeScheme();
          }
          updateWindowText();
        }
      }
      return 1;

    case CM_SAVE:
      if (openFileName.length() > 0)
      {
        blkEditor.save(openFileName.str());
        scintillaOldText = blkEditor.getBlkText();
      }
      else
        onMenuItemClick(CM_SAVE_AS);
      return 1;

    case CM_SAVE_AS:
    {
      String path = wingw::file_save_dlg(NULL, "Save datablock as...",
        "Datablocks (*.blk)|*.blk|"
        "All files(*.*)|*.*",
        "blk", sgg::get_exe_path_full(), openFileName.str());
      if (path.length() > 0)
      {
        blkEditor.save(path.str());
        openFileName = path;
        updateWindowText();
        scintillaOldText = blkEditor.getBlkText();
      }
    }
      return 1;

    case CM_CHANGE_SCHEME:
    {
      changeScheme();
    }
      return 1;

    case CM_WINDOW_SCINTILLA: mManager->setWindowType(nullptr, SCINTILLA_TYPE); return 1;

    case CM_WINDOW_PPANEL: mManager->setWindowType(nullptr, PPANEL_TYPE); return 1;

    case CM_WINDOW_TOOLBAR: mManager->setWindowType(nullptr, TOOLBAR_TYPE); return 1;

    default: return 0;
  }
}

void BlkEditorApp::onBlkChange()
{
  if (!mScintillaPanel)
    return;

  DataBlock folding;
  int pos = 0;
  if (!ignoreFoldingAndScroll)
  {
    mScintillaPanel->getFoldingInfo(folding);
    pos = mScintillaPanel->getScrollPos();
  }

  mScintillaPanel->setText(blkEditor.getBlkText());

  if (!ignoreFoldingAndScroll)
  {
    mScintillaPanel->setScrollPos(pos);
    mScintillaPanel->setFoldingInfo(folding);
  }
  else
    ignoreFoldingAndScroll = false;
}


void BlkEditorApp::onModify()
{
  if (mScintillaPanel)
  {
    SimpleString text;
    mScintillaPanel->getText(text);

    DataBlock panel_state;
    if (mPropPanel)
      mPropPanel->saveState(panel_state);

    blkEditor.setBlkText(text.str(), text.length());

    if (mPropPanel)
      mPropPanel->loadState(panel_state);
  }
}


void BlkEditorApp::changeScheme()
{
  String path = wingw::file_open_dlg(NULL, "Select script for datablock",
    "Scripts (*.nut)|*.nut|"
    "All files(*.*)|*.*",
    "nut", blkEditor.getScriptsDir(), blkEditor.getScriptPath());
  if (::dd_file_exist(path.str()))
  {
    blkEditor.setScriptPath(path.str());
    onBlkChange();
  }
}


bool test_fname(const char *fn, dag::ConstSpan<RegExp *> patterns, dag::ConstSpan<RegExp *> excludes)
{
  for (int i = 0; i < patterns.size(); ++i)
    if (patterns[i]->test(fn))
    {
      for (int j = 0; j < excludes.size(); ++j)
        if (excludes[j]->test(fn))
          return false;
      return true;
    }
  return false;
}


SimpleString parse_config(const DataBlock &configBlk, const char *blk_fn, bool default_schemes)
{
  Tab<RegExp *> tests(tmpmem);
  Tab<RegExp *> excludes(tmpmem);
  int scheme_name_id = configBlk.getNameId("scheme");
  int test_name_id = configBlk.getNameId("test");
  int exclude_name_id = configBlk.getNameId("exclude");

  for (int i = 0; i < configBlk.blockCount(); ++i)
  {
    const DataBlock *schemeBlk = configBlk.getBlock(i);
    if ((default_schemes && schemeBlk->getBlockNameId() != scheme_name_id) ||
        (!default_schemes && schemeBlk->getBlockNameId() == scheme_name_id))
      continue;

    for (int j = 0; j < schemeBlk->paramCount(); ++j)
    {
      int param_id = schemeBlk->getParamNameId(j);
      if (param_id == test_name_id || param_id == exclude_name_id)
      {
        SimpleString paramVal(schemeBlk->getStr(j));
        RegExp *re = new RegExp;
        if (!re->compile(paramVal.str(), "i"))
        {
          debug("Wrong regExp \"%s\"", paramVal.str());
          delete re;
        }
        else
        {
          if (param_id == test_name_id)
            tests.push_back(re);
          if (param_id == exclude_name_id)
            excludes.push_back(re);
        }
      }
    }

    if (test_fname(blk_fn, tests, excludes))
    {
      clear_all_ptr_items(tests);
      clear_all_ptr_items(excludes);

      if (default_schemes)
        return SimpleString(schemeBlk->getStr("scheme", ""));
      else
        return SimpleString(schemeBlk->getStr("schemeFolder", ""));
    }
  }

  clear_all_ptr_items(tests);
  clear_all_ptr_items(excludes);
  return SimpleString("");
}


bool get_config(const char *schemes_path, DataBlock *config_blk)
{
  String schemesFullPath(512, "%s%s", sgg::get_exe_path_full(), schemes_path);
  String configPath(255, "%sconfig.blk", schemesFullPath.str());
  ::dd_simplify_fname_c(configPath.str());
  if (!::dd_file_exist(configPath.str()))
    return false;
  config_blk->load(configPath);
  return true;
}


SimpleString BlkEditorApp::getSchemeFromFn(const char *blk_fn, const char *schemes_path)
{
  DataBlock configBlk;
  if (!get_config(schemes_path, &configBlk))
    return SimpleString("");
  SimpleString res = parse_config(configBlk, blk_fn, true);
  if (res.empty())
    return SimpleString("");
  String schemeFileName(512, "%s%s.nut", schemes_path, res.str());
  return SimpleString(schemeFileName.str());
}


SimpleString BlkEditorApp::getSchemePathFromFn(const char *blk_fn, const char *schemes_path)
{
  DataBlock configBlk;
  if (!get_config(schemes_path, &configBlk))
    return SimpleString("");
  SimpleString result = parse_config(configBlk, blk_fn, false);
  if (result.empty())
    result = schemes_path;
  return result;
}


void BlkEditorApp::imguiBegin(const char *name, bool *open, unsigned window_flags)
{
  ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

  // Change the tab title color for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();

  ImGui::Begin(name, open, window_flags | ImGuiWindowFlags_NoFocusOnAppearing);

  PropPanel::popDialogTitleBarColorOverrides();
}


void BlkEditorApp::updateImgui()
{
  PropPanel::render_menu(*getMainMenu());

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  const ImGuiWindowFlags rootWindowFlags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav;
  ImGui::Begin("Root", nullptr, rootWindowFlags);
  ImGui::PopStyleVar(3);

  if (mToolPanel)
  {
    PropPanel::pushToolBarColorOverrides();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, PropPanel::Constants::TOOLBAR_WINDOW_PADDING);

    ImGui::BeginChild("Toolbar", ImVec2(0, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border,
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

    ImGui::PopStyleVar();
    PropPanel::popToolBarColorOverrides();

    mToolPanel->updateImgui();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0.0f));
    ImGui::EndChild();
    ImGui::PopStyleVar();
  }

  ImGui::BeginChild("DockHolder", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);

  ImGuiID rootDockSpaceId = ImGui::GetID("RootDockSpace");

  if (!dockPositionsInitialized)
  {
    dockPositionsInitialized = true;

    ImGui::DockBuilderRemoveNode(rootDockSpaceId);
    ImGui::DockBuilderAddNode(rootDockSpaceId, ImGuiDockNodeFlags_CentralNode);
    ImGui::DockBuilderSetNodeSize(rootDockSpaceId, ImGui::GetMainViewport()->Size);

    ImGuiID dockSpaceProperties;
    ImGuiID dockSpaceScintilla = ImGui::DockBuilderSplitNode(rootDockSpaceId, ImGuiDir_Right, 0.85f, nullptr, &dockSpaceProperties);
    ImGui::DockBuilderDockWindow(scintillaWindowName, dockSpaceScintilla);
    ImGui::DockBuilderDockWindow(propertiesWindowName, dockSpaceProperties);

    ImGui::DockBuilderFinish(rootDockSpaceId);
  }

  // Change the close button's color for the daEditorX Classic style.
  PropPanel::pushDialogTitleBarColorOverrides();
  ImGui::DockSpace(rootDockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
  PropPanel::popDialogTitleBarColorOverrides();

  imguiBegin(scintillaWindowName, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  const IPoint2 newScintillaWindowPos(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
  const IPoint2 newScintillaWindowSize(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
  ImGui::End();

  if (mScintillaPanel && (newScintillaWindowPos != scintillaWindowPos || newScintillaWindowSize != scintillaWindowSize))
  {
    scintillaWindowPos = newScintillaWindowPos;
    scintillaWindowSize = newScintillaWindowSize;
    mScintillaPanel->moveWindow(newScintillaWindowPos.x - viewport->Pos.x, newScintillaWindowPos.y - viewport->Pos.y,
      scintillaWindowSize.x, scintillaWindowSize.y);
  }

  ImVec2 propPanelPos = viewport->Pos;
  ImVec2 propPanelSize = viewport->Size;

  if (mPropPanel)
  {
    mPropPanel->beforeImguiBegin();
    imguiBegin(mPropPanel->getStringCaption(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
    propPanelPos = ImGui::GetCursorScreenPos();
    propPanelSize = ImGui::GetContentRegionAvail();
    mPropPanel->updateImgui();
    ImGui::End();
  }

  if (console && console->isVisible())
  {
    // Create a new viewport for the console window by default to prevent it getting clipped by the Scintilla editor's window.
    ImGuiWindowClass windowClass;
    windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
    ImGui::SetNextWindowClass(&windowClass);

    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    const bool canClose = console->isCanClose() && !console->isProgress();
    bool open = true;
    imguiBegin(consoleWindowName, canClose ? &open : nullptr, ImGuiWindowFlags_None);
    console->updateImgui();
    ImGui::End();

    if (!open)
      console->hide();
  }

  // ImGui::BeginPopupModal() uses ImGuiViewport::GetCenter() to set the initial size of the modals. Ensure that they
  // will not end up behind the native Scintilla editor window by limiting them to the property panel's area.
  const ImVec2 originalPos = viewport->Pos;
  const ImVec2 originalSize = viewport->Size;
  viewport->Pos = propPanelPos;
  viewport->Size = propPanelSize;
  PropPanel::render_dialogs();
  viewport->Pos = originalPos;
  viewport->Size = originalSize;

  ImGui::EndChild();
  ImGui::End();

  // Try to reconcile the focus between the Scintilla editor and the ImGui-based property panel.
  const bool newScintillaWindowFocused = mScintillaPanel && mScintillaPanel->isWindowFocused();
  if (newScintillaWindowFocused != scintillaWindowFocused)
  {
    if (newScintillaWindowFocused && !scintillaWindowFocused)
      if (ImGuiWindow *scintillaWindow = ImGui::FindWindowByName(scintillaWindowName))
        ImGui::FocusWindow(scintillaWindow);

    scintillaWindowFocused = newScintillaWindowFocused;
  }
  else
  {
    const bool newImguiWantsTextInput = ImGui::GetIO().WantTextInput;
    if (newImguiWantsTextInput != imguiWantsTextInput)
    {
      if (newImguiWantsTextInput && !imguiWantsTextInput)
        SetFocus((HWND)mManager->getMainWindow()); // Unfocus the Scintilla editor window.

      imguiWantsTextInput = newImguiWantsTextInput;
    }
  }

  // If an ImGui modal is open then simulate the modal behavior by disabling the Scintilla editor.
  const bool imguiHasOpenModal = ImGui::GetTopMostPopupModal() != nullptr;
  const bool newScintillaWindowEnabled = !imguiHasOpenModal;
  if (newScintillaWindowEnabled != scintillaWindowEnabled)
  {
    mScintillaPanel->enableWindow(newScintillaWindowEnabled);
    if (!newScintillaWindowEnabled)
      SetFocus((HWND)mManager->getMainWindow()); // Unfocus the Scintilla editor window.

    scintillaWindowEnabled = newScintillaWindowEnabled;
  }
}
