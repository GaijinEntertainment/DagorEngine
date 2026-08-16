// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorTreeDataNode.h"
#include <EASTL/hash_map.h>
#include <util/dag_string.h>
#include <propPanel/commonWindow/treeviewPanel.h>
#include <propPanel/control/treeInterface.h>
#include <propPanel/imguiHelper.h>

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
  long onWcKeyDown(WindowBase *source, unsigned v_key) override;

  PropPanel::IconId getImageId(CompositeEditorTreeDataNode &treeDataNode) const;

  void fillInternal(CompositeEditorTreeDataNode &treeDataNode, PropPanel::TLeafHandle parent,
    CompositeEditorTreeDataNode *treeDataNodeToSelect, const dag::Vector<CompositeEditorTreeDataNode *> &dataBlocksToMultiSelect,
    eastl::hash_map<const void *, bool> &closedNodes);

  void getClosedNodes(eastl::hash_map<const void *, bool> &closedNodes);

  eastl::optional<ImU32> getBorderColor() const override;
  bool treeNodeStart(const NodeStartData &data) override;
  void treeNodeRender(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data, bool show_arrow) override;
  void treeNodeEnd(PropPanel::ImguiHelper::TreeNodeWithSpecialHoverBehaviorEndData &end_data) override;
  void treeItemRenderEx(const ItemRenderData &data) override;

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
};
