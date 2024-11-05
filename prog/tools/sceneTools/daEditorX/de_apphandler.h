// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "de_appwnd.h"

#include <EditorCore/ec_genapp_ehfilter.h>


class DagorEdAppEventHandler : public GenericEditorAppWindow::AppEventHandler
{
public:
  bool showCompass;

  DagorEdAppEventHandler(GenericEditorAppWindow &m);
  ~DagorEdAppEventHandler();

  virtual void handleViewportPaint(IGenViewportWnd *wnd);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
};
