// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_restart.h>
#include <generic/dag_initOnDemand.h>
#include <gui/dag_stdGuiRender.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_simpleString.h>
#include <gui/dag_guiStartup.h>

class GuiBaseRestartProc : public SRestartProc
{
public:
  SimpleString fontsBlkFilename;

  virtual const char *procname() { return "GUIbase"; }
  GuiBaseRestartProc() : SRestartProc(RESTART_GAME | RESTART_VIDEO | RESTART_VIDEODRV | RESTART_DRIVER_VIDEO_MODE) {}

  void startup()
  {
    if (!fontsBlkFilename.empty())
    {
      DataBlock blk(fontsBlkFilename);
      StdGuiRender::init_fonts(blk);
    }

    StdGuiRender::init_render();
  }

  void shutdown()
  {
    StdGuiRender::close_render();
    StdGuiRender::close_fonts();
  }
};
static InitOnDemand<GuiBaseRestartProc> gui_base_rproc;

void startup_gui_base(const char *fonts_blk_filename)
{
  gui_base_rproc.demandInit();
  gui_base_rproc->fontsBlkFilename = fonts_blk_filename;

  add_restart_proc(gui_base_rproc);
}
