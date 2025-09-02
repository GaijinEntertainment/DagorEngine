// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de_viewportWindow.h"
#include "de_screenshotMetaInfoLoader.h"
#include "de3_dynRenderService.h"

#include <EditorCore/ec_brush.h>
#include <ioSys/dag_dataBlock.h>
#include <oldEditor/de_interface.h>
#include <util/dag_string.h>
#include <winGuiWrapper/wgw_dialogs.h>

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
