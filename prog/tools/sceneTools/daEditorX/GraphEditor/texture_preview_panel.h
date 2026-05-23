// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>
#include <math/dag_Point2.h>

struct IGraphTexGenService;
class GraphEditorPlg;

class TexturePreviewPanel final : public PropPanel::ControlEventHandler
{
public:
  TexturePreviewPanel(GraphEditorPlg &plugin, IGraphTexGenService &ctx);
  ~TexturePreviewPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  void updateImgui();

private:
  PropPanel::PanelWindowPropertyControl *panelWindow;
  GraphEditorPlg &plugin;
  IGraphTexGenService &texGenService;

  float viewScale = 1.0f;
  Point2 viewCenter = {0.5f, 0.5f};
  bool isPanning = false;
};
