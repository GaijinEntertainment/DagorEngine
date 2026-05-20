// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/c_common.h>
#include <propPanel/c_window_event_handler.h>

struct ObjPropDialogNode
{
  bool isScriptNode() const { return nodeData != nullptr; }

  String name;
  String script;
  const void *nodeData = nullptr; // type: ObjectPropertyEditor::Data::NodeData
  dag::Vector<ObjPropDialogNode> children;
};

class ObjPropDialog : public PropPanel::DialogWindow
{
public:
  ObjPropDialog(const char *caption, hdpi::Px width, hdpi::Px height, const ObjPropDialogNode &root_node);

  const ObjPropDialogNode &getRootDialogNode() const { return rootNode; }

  int showDialog() override;

private:
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  void fillTree(PropPanel::TLeafHandle tree_node, const ObjPropDialogNode &dialog_node);
  void applyChanges(dag::ConstSpan<PropPanel::TLeafHandle> script_nodes);
  void updateSaveButtonState();
  void updateTreeImages(PropPanel::TLeafHandle tree_node);

  ObjPropDialogNode *getObjPropDialogNode(PropPanel::TLeafHandle tree_node);
  const ObjPropDialogNode *getObjPropDialogNode(PropPanel::TLeafHandle tree_node) const;
  dag::Vector<PropPanel::TLeafHandle> getSelectedScriptNodes() const;
  String getCommonScriptLines(dag::ConstSpan<PropPanel::TLeafHandle> script_nodes) const;

  PropPanel::ContainerPropertyControl *tree = nullptr;
  ObjPropDialogNode rootNode;
  dag::Vector<PropPanel::TLeafHandle> prevSel;
  bool changed = false, shouldSave = false;

  static float lastUsedSplitRatio;
};
