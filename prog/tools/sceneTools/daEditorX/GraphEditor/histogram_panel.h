// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>
#include <math/dag_color.h>

#include <EASTL/vector.h>

struct IGraphTexGenService;
class GraphEditorPlg;

class HistogramPanel final : public PropPanel::ControlEventHandler
{
public:
  HistogramPanel(GraphEditorPlg &plugin, IGraphTexGenService &ctx);
  ~HistogramPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  void updateImgui();

private:
  void drawHistogramRect(const char *label, const eastl::vector<Color4> &hist, const int *channelIndices, int numChannels,
    const ImU32 *colors, float histW, float histH);

  PropPanel::PanelWindowPropertyControl *panelWindow;
  GraphEditorPlg &plugin;
  IGraphTexGenService &texGenService;
};
