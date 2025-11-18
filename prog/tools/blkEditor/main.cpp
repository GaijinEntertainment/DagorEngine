// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "be_appwnd.h"

#include <EditorCore/ec_input.h>
#include <EditorCore/ec_mainWindow.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <debug/dag_debug.h>
#include <startup/dag_globalSettings.h>

#define __UNLIMITED_BASE_PATH 1
#include <startup/dag_winMain.inc.cpp>

class AppManager : public IWndManagerEventHandler
{
public:
  void onInit(IWndManager *manager) override
  {
    G_ASSERT(manager);

    const char *open_fname = NULL;

    for (int i = 1; i < __argc; ++i)
    {
      if (__argv[i][0] != '/' && __argv[i][0] != '-')
      {
        open_fname = __argv[i];
        break;
      }
    }

    app = new BlkEditorApp(manager, open_fname);
    ::dgs_pre_shutdown_handler = &clearBusy;
    app->init();
  }

  bool onClose() override
  {
    G_ASSERT(app);
    return app->canClose();
    return true;
  }

  void onDestroy() override { del_it(app); }

  static void clearBusy() { ec_set_busy(false); }

private:
  BlkEditorApp *app;
};


void DagorWinMainInit(int, bool) {}

int DagorWinMain(int nCmdShow, bool /*debugmode*/)
{
  AppManager appManager;

  EditorMainWindow mainWindow(appManager);
  mainWindow.run("Datablock Editor", "guiEditorIcon");

  return 0;
}
