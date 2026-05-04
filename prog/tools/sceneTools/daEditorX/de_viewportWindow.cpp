// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_viewportWindow.h"
#include "de_appwnd.h"
#include "de_screenshotMetaInfoLoader.h"
#include "de3_dynRenderService.h"

#include <EditorCore/ec_brush.h>
#include <EditorCore/ec_cm.h>
#include <image/dag_loadImage.h>
#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_interface.h>
#include <util/dag_string.h>
#include <util/dag_delayedAction.h>
#include <winGuiWrapper/wgw_dialogs.h>

int DagorEdViewportWindow::onMenuItemClick(unsigned id)
{
  switch (id)
  {
    case CM_CAMERAS_FREE:
    case CM_CAMERAS_FPS:
    case CM_CAMERAS_TPS:
    case CM_CAMERAS_CAR:
      DagorEdAppWindow *appWindow = (DagorEdAppWindow *)DAGORED2;
      static_cast<PropPanel::IMenuEventHandler *>(appWindow)->onMenuItemClick(id);
      break;
  }

  return ViewportWindow::onMenuItemClick(id);
}

bool DagorEdViewportWindow::onDropFiles(const dag::Vector<String> &files)
{
  if (files.empty())
    return false;

  DataBlock metaInfo;
  String errorMessage;
  if (load_meta_info_from_image(files[0], metaInfo, errorMessage))
    ScreenshotMetaInfoLoader::applyCameraSettings(metaInfo, *this);
  else
  {
    String f = files[0];
    delayed_call([f, errorMessage]() {
      wingw::message_box(wingw::MBS_EXCL, "Error", "Error loading camera position from screenshot\n\"%s\"\n\n%s", f.c_str(),
        errorMessage.c_str());
    });
  }

  return true;
}

bool DagorEdViewportWindow::canStartInteractionWithViewport()
{
  if (!ViewportWindow::canStartInteractionWithViewport())
    return false;

  if (DAGORED2->isGizmoOperationStarted())
    return false;

  if (DAGORED2->isBrushPainting()) // This just tells if the brush is active.
  {
    const Brush *brush = DAGORED2->getBrush();
    if (brush && brush->isDrawingInProgress())
      return false;
  }

  return true;
}

BaseTexture *DagorEdViewportWindow::getDepthBuffer()
{
  if (auto *srv = EDITORCORE->queryEditorInterface<IDynRenderService>())
  {
    return srv->getDepthBuffer();
  }

  return nullptr;
}

void DagorEdViewportWindow::fillStatSettingsDialog(ViewportWindowStatSettingsDialog &dialog, bool include_camera_distance)
{
  ViewportWindow::fillStatSettingsDialog(dialog, false);
}
