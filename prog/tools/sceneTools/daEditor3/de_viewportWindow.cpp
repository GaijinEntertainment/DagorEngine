#include "de_viewportWindow.h"
#include "de_screenshotMetaInfoLoader.h"
#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_interface.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>

DagorEdViewportWindow::DagorEdViewportWindow(TEcHandle parent, int left, int top, int w, int h) :
  ViewportWindow(parent, left, top, w, h)
{
  setDragAcceptFiles();
}

bool DagorEdViewportWindow::onDropFiles(const dag::Vector<String> &files)
{
  if (files.empty())
    return false;

  DataBlock metaInfo;
  String errorMessage;
  if (ScreenshotMetaInfoLoader::loadMetaInfo(files[0], metaInfo, errorMessage))
    ScreenshotMetaInfoLoader::applyCameraSettings(metaInfo, *this);
  else
    wingw::message_box(wingw::MBS_EXCL, "Error", "Error loading camera position from screenshot\n\"%s\"\n\n%s", files[0].c_str(),
      errorMessage.c_str());

  return true;
}
