// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorTreeDataNode.h"
#include <EASTL/hash_map.h>
#include <util/dag_string.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/treeInterface.h>

#include <ska_hash_map/flat_hash_map2.hpp>

class CompositeEditorTreeData;

class CompositeEditorTree : public PropPanel::TreeBaseWindow, public PropPanel::ITreeRenderEx
{
public:
  CompositeEditorTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
    const char caption[]);

  void fill(CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *treeDataNodeToSelect,
    const dag::Vector<CompositeEditorTreeDataNode *> &dataBlocksToMultiSelect, bool keepNodeExpansionState);
  void selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect, bool multiSelect);
  void deselectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToDeselect);

  void updateImgui(float control_height = 0.0f) override;

private:
  enum class NodeState : uint8_t
  {
    Missing,
    ContainsMissing,
    Disabled,
    ParentDisabled,
  };

  long onWcKeyDown(WindowBase *source, unsigned v_key) override;

  PropPanel::IconId getImageId(CompositeEditorTreeDataNode &treeDataNode) const;

  void fillInternal(CompositeEditorTreeDataNode &treeDataNode, PropPanel::TLeafHandle parent,
    CompositeEditorTreeDataNode *treeDataNodeToSelect, const dag::Vector<CompositeEditorTreeDataNode *> &dataBlocksToMultiSelect,
    eastl::hash_map<const void *, bool> &closedNodes, bool parentDisabled = false);

  bool collectMissingStates(const CompositeEditorTreeDataNode &treeDataNode);
  void getClosedNodes(eastl::hash_map<const void *, bool> &closedNodes);

  eastl::optional<E3DCOLOR> getBorderColor() const override;
  void drawTreeNode(PropPanel::TreeNodeDrawContext &ctx) override;

  PropPanel::IconId animCharImageId;
  PropPanel::IconId compositImageId;
  PropPanel::IconId dynModelImageId;
  PropPanel::IconId efxImageId;
  PropPanel::IconId folderImageId;
  PropPanel::IconId fxImageId;
  PropPanel::IconId gameObjImageId;
  PropPanel::IconId prefabImageId;
  PropPanel::IconId rendinstImageId;
  PropPanel::IconId alertImageId;

  String headerTooltip;
  int headerTooltipDepth = 0;

  ska::flat_hash_map<const void *, NodeState> nodeStates;
};
