// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_wndPublic.h>
#include <libTools/util/undo.h>
#include <coolConsole/coolConsole.h>
#include <propPanel/control/menu.h>
#include <util/dag_simpleString.h>
#include "scintilla_window.h"
#include "script_panel.h"
#include <math/integer/dag_IPoint2.h>
#include <EASTL/unique_ptr.h>

namespace PropPanel
{
class ContainerPropertyControl;
class IMenu;
class PanelWindowPropertyControl;
} // namespace PropPanel

class CScintillaWindow;
class PropertyContainerControlBase;

#if !HANDLE2_DEFINED
// #ifdef STRICT
#if 1
// struct HWND__ { int unused; }
typedef struct HWND__ *HWND2;
typedef struct HINSTANCE__ *HINSTANCE2;
typedef struct HICON__ *HICON2;
typedef struct HACCEL__ *HACCEL2;
#else
typedef void *HWND2;
typedef void *HINSTANCE2;
typedef void *HICON2;
typedef void *HACCEL2;
#endif

#define HANDLE2_DEFINED 1
#endif


class BlkEditorApp : public PropPanel::ControlEventHandler,
                     public IWndManagerWindowHandler,
                     public PropPanel::IMenuEventHandler,
                     public IUndoRedoWndClient,
                     public BlkHandler,
                     public ScintillaEH
{
public:
  BlkEditorApp(IWndManager *manager, const char *open_fname = NULL);
  ~BlkEditorApp() override;

  void init();
  inline void repaint();

  bool canClose();

  // GUI
  inline PropPanel::PanelWindowPropertyControl *getPropPanel() const { return mPropPanel; }
  inline PropPanel::ContainerPropertyControl *getToolbar() const { return mToolPanel; }
  PropPanel::IMenu *getMainMenu();
  void fillToolBar();

  void updateUndoRedoMenu() override;

  // IWndManagerWindowHandler
  void *onWmCreateWindow(int type) override;
  bool onWmDestroyWindow(void *handle) override;

  // BlkHandler
  void onBlkChange() override;
  PropPanel::ContainerPropertyControl *getBlkPanel() override;

  // ScintillaEH
  void onModify() override;

  void updateImgui();

protected:
  static long __stdcall wndProc(HWND2 wnd, unsigned msg, unsigned w_param, long l_param);

  void alignControls();

  bool createNew(const char *filename);
  bool load(const char *filename);
  bool save(const char *filename);

  void changeScheme();

  void fillMenu(PropPanel::IMenu *menu);
  void updateMenu(PropPanel::IMenu *menu);

  // ControlEventHandler
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  // Menu
  int onMenuItemClick(unsigned id) override;

  void makeDefaultLayout();
  void updateWindowText();

  SimpleString getSchemeFromFn(const char *blk_fn, const char *schemes_path);
  SimpleString getSchemePathFromFn(const char *blk_fn, const char *schemes_path);

private:
  void init3d();
  void imguiBegin(const char *name, bool *open, unsigned window_flags);

  PropPanel::PanelWindowPropertyControl *mPropPanel;
  CScintillaWindow *mScintillaPanel;
  PropPanel::ContainerPropertyControl *mToolPanel;

  IWndManager *mManager;
  UndoSystem *undoSystem;
  CoolConsole *console;

  SimpleString openFileName;
  SimpleString scintillaOldText;
  ScriptPanelEditor blkEditor;
  bool ignoreFoldingAndScroll;

  eastl::unique_ptr<PropPanel::IMenu> mainMenu;
  IPoint2 scintillaWindowPos = IPoint2(0, 0);
  IPoint2 scintillaWindowSize = IPoint2(0, 0);
  bool scintillaWindowEnabled = true;
  bool scintillaWindowFocused = false;
  bool imguiWantsTextInput = false;
  bool dockPositionsInitialized = false;

  static constexpr const char *consoleWindowName = "Console";
  static constexpr const char *scintillaWindowName = "Scintilla";
  static constexpr const char *propertiesWindowName = "Properties";
};
