// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorRefreshType.h"
#include <ioSys/dag_dataBlock.h>
#include <propPanel/control/container.h>
#include <propPanel/control/dragAndDropHandler.h>

class CompositeEditorTreeData;
class CompositeEditorTreeDataNode;
class DagorAsset;

class CompositeEditorPanel : public PropPanel::ContainerPropertyControl
{
public:
  CompositeEditorPanel(PropPanel::ControlEventHandler *event_handler, int x, int y, unsigned w, unsigned h);

  void fill(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler);
  void updateTransformParams(const CompositeEditorTreeData &treeData, const CompositeEditorTreeDataNode *selectedTreeDataNode);
  CompositeEditorRefreshType onChange(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);
  CompositeEditorRefreshType onClick(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);
  CompositeEditorRefreshType onDragAndDropAsset(CompositeEditorTreeDataNode &treeDataNode, int pcb_id, DagorAsset *asset);

private:
  void fillEntityGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler);
  void fillEntitiesGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditEntities);
  void fillChildrenGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditChildren);
  void fillParametersGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    bool canEditParameters);
  void fillInternal(const CompositeEditorTreeDataNode &treeDataNode, PropPanel::IDropTargetHandler *dropTargetHandler,
    bool isRootNode);
  CompositeEditorRefreshType onAddNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onRemoveNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onCopyNodeParametersClicked(const CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onPasteNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);

  static void makeUndoForPropertyEditing();

  DataBlock panelState;
  DataBlock supportedNodeParameters;
};
