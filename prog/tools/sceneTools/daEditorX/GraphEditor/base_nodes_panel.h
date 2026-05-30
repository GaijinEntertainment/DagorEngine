// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_common.h> // TLeafHandle
#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/dragAndDropHandler.h>
#include <propPanel/control/panelWindow.h>

#include <EASTL/string.h>
#include <EASTL/vector_map.h>

class GraphEditorPlg;

class BaseNodesPanel final : public PropPanel::ControlEventHandler, public PropPanel::ITreeDragHandler
{
public:
  explicit BaseNodesPanel(GraphEditorPlg &plugin);
  ~BaseNodesPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  void updateImgui();

  // Tears down the existing panel contents and re-builds the reload button + categorised tree
  // against the plugin's current baseNodesBlk. Called by GraphEditorPlg::reloadBaseNodes after
  // the descriptor registry has been refreshed; safe to call repeatedly.
  void refresh();

  void onBeginDrag(PropPanel::TLeafHandle leaf) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  void buildPanelContents();
  void populateTree(PropPanel::ContainerPropertyControl *tree);

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;

  // Maps tree leaf handles to the descriptor's stable templateUid. The drag payload is the
  // uid, not the visible name -- so a template renamed in base_nodes.blk between sessions
  // still resolves on drop. Populated only for child leaves; category roots are
  // intentionally absent so dragging them produces no payload.
  eastl::vector_map<PropPanel::TLeafHandle, eastl::string> leafToTemplateUid;
};
