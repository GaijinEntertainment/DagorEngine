// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

#include <EASTL/unique_ptr.h>

struct IGraphTexGenService;
class GraphEditorPlg;
class LandscapePreviewScene;

class LandscapePreviewPanel final : public PropPanel::ControlEventHandler
{
public:
  LandscapePreviewPanel(GraphEditorPlg &plugin, IGraphTexGenService &ctx);
  ~LandscapePreviewPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }
  LandscapePreviewScene *getScene() { return scene.get(); }

  void updateImgui();

private:
  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;
  IGraphTexGenService &texGenService;

  eastl::unique_ptr<LandscapePreviewScene> scene;
  bool controllingWithRmb = false;
};
