#pragma once
#include "compositeEditorRefreshType.h"
#include <ioSys/dag_dataBlock.h>
#include <propPanel2/comWnd/panel_window.h>

class CompositeEditorTreeData;
class CompositeEditorTreeDataNode;

class CompositeEditorPanel : public CPanelWindow
{
public:
  CompositeEditorPanel(ControlEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]);

  void fill(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode);
  void updateTransformParams(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode);
  CompositeEditorRefreshType onChange(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);
  CompositeEditorRefreshType onClick(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);

private:
  void fillEntityGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode);
  void fillEntitiesGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode, bool canEditEntities);
  void fillChildrenGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode, bool canEditChildren);
  void fillParametersGroup(PropertyContainerControlBase &group, const CompositeEditorTreeDataNode &treeDataNode,
    bool canEditParameters);
  void fillInternal(const CompositeEditorTreeDataNode &treeDataNode, bool isRootNode);
  CompositeEditorRefreshType onAddNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onRemoveNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onCopyNodeParametersClicked(const CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onPasteNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);

  static void makeUndoForPropertyEditing();

  DataBlock panelState;
  DataBlock supportedNodeParameters;
};
