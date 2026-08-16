// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorRefreshType.h"
#include <ioSys/dag_dataBlock.h>
#include <propPanel/propPanel.h>
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
  void updateTransformParams(const CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *selectedTreeDataNode);
  CompositeEditorRefreshType onChange(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);
  CompositeEditorRefreshType onClick(CompositeEditorTreeDataNode &treeDataNode, int pcb_id);
  CompositeEditorRefreshType onDragAndDropAsset(CompositeEditorTreeDataNode &treeDataNode, int pcb_id, DagorAsset *asset);

private:
  enum
  {
    CMP_NODE_PARAM_IDX_PLACE_TYPE = 0,
    CMP_NODE_PARAM_IDX_ABOVE_HT,
    CMP_NODE_PARAM_IDX_ROT_X,
    CMP_NODE_PARAM_IDX_ROT_Y,
    CMP_NODE_PARAM_IDX_ROT_Z,
    CMP_NODE_PARAM_IDX_OFFSET_X,
    CMP_NODE_PARAM_IDX_OFFSET_Y,
    CMP_NODE_PARAM_IDX_OFFSET_Z,
    CMP_NODE_PARAM_IDX_SCALE,
    CMP_NODE_PARAM_IDX_SCALE_Y,
    CMP_NODE_PARAM_IDX_IGNORE_PARENT_INST_SEED,

    CMP_NODE_PARAM_COUNT
  };

  void fillEntityGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler);
  void fillEntitiesGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditEntities);
  void fillChildrenGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    PropPanel::IDropTargetHandler *dropTargetHandler, bool canEditChildren);
  void fillParametersGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    bool canEditParameters);

  void updateParameter(int id, real x, CompositeEditorTreeDataNode &treeDataNode);
  void fillGlobalParametersGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode);
  void fillGlobalParametersGroup(PropPanel::ContainerPropertyControl &group, const CompositeEditorTreeDataNode &treeDataNode,
    const bool useTransformationMatrix, const Point3 &position, const Point3 &rotation, const Point3 &scale);
  void shouldRefillParam(bool &refill, int index, const CompositeEditorTreeDataNode &treeDataNode, real tmValue, real def);
  void updateGlobalTransformParameters(CompositeEditorTreeDataNode &treeDataNode, const bool useTransformationMatrix,
    const Point3 &position, Point3 const &rotation, const Point3 &scale);
  void addNonDefaultParam(int index, CompositeEditorTreeDataNode &treeDataNode, real tmValue, real def);

  void fillInternal(const CompositeEditorTreeDataNode &treeDataNode, PropPanel::IDropTargetHandler *dropTargetHandler,
    bool isRootNode);
  CompositeEditorRefreshType onAddNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onRemoveNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);
  CompositeEditorRefreshType onPasteNodeParametersClicked(CompositeEditorTreeDataNode &treeDataNode);

  static void onChangeTransformationMatrixDelayed();

  int saveState(DataBlock &datablk, bool by_name = false) override;

  bool splitMinimized = false;
  bool splitRecursive = false;

  const CompositeEditorTreeDataNode *editedTreeDataNode = nullptr;
  unsigned int editedTreeDataNodeBlockId;

  static void makeUndoForPropertyEditing();

  DataBlock panelState;
  DataBlock supportedNodeParameters;

  bool parameters[CMP_NODE_PARAM_COUNT];
  int supportedNodeParameterIds[CMP_NODE_PARAM_COUNT];
  PropPanel::IconId alertId = PropPanel::IconId::Invalid;
  PropPanel::IconId deleteId = PropPanel::IconId::Invalid;
  ImVec2 captionSize;
};
