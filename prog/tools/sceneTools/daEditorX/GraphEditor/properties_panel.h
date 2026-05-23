// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

#include <EASTL/string.h>
#include <EASTL/vector_map.h>

#include <graphEditor/graph_data.h>

class DataBlock;
class GraphEditorPlg;

// Context-sensitive property grid. With no node selected it shows graph-level fields
// (source path, output dirs, heightmap parameters); with one node selected it walks the
// node's descriptor in base_nodes.blk and creates a control per `property {}` block.
// Edits write back into GraphData and ask IGraphTexGenService to regenerate.
class PropertiesPanel final : public PropPanel::ControlEventHandler
{
public:
  explicit PropertiesPanel(GraphEditorPlg &plugin);
  ~PropertiesPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  // Called every frame after GraphPanel updates. Diffs against lastRenderedNodeId /
  // lastRenderedSourcePath to decide whether a control rebuild is required; the no-change
  // path is just a few comparisons + panelWindow->updateImgui().
  void updateImgui();

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

private:
  enum class Mode
  {
    None,
    Graph,
    Node,
  };

  void rebuildForGraph();
  void rebuildForNode(int node_id);

  const GraphData::Node *findNodeById(int id) const;
  const DataBlock *findPropertyDesc(const DataBlock *node_desc, const char *prop_name) const;

  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
  GraphEditorPlg &plugin;

  Mode currentMode = Mode::None;
  int lastRenderedNodeId = -1;
  eastl::string lastRenderedSourcePath;

  // Per-pid -> property name (for node mode). Wiped on every rebuild. PIDs are allocated
  // sequentially starting at PID_NODE_PROP_BASE.
  eastl::vector_map<int, eastl::string> pidToPropertyName;
};
