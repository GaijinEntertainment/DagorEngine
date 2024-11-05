// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorTreeDataNode.h"
#include <EASTL/hash_map.h>
#include <propPanel/commonWindow/treeviewPanel.h>

class CompositeEditorTreeData;

class CompositeEditorTree : public PropPanel::TreeBaseWindow
{
public:
  CompositeEditorTree(PropPanel::ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h,
    const char caption[]);

  void fill(CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *treeDataNodeToSelect, bool keepNodeExpansionState);
  void selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect);

private:
  virtual long onWcKeyDown(WindowBase *source, unsigned v_key) override;

  TEXTUREID getImageId(CompositeEditorTreeDataNode &treeDataNode) const;

  void fillInternal(CompositeEditorTreeDataNode &treeDataNode, PropPanel::TLeafHandle parent,
    CompositeEditorTreeDataNode *treeDataNodeToSelect, eastl::hash_map<const void *, bool> &closedNodes);

  void getClosedNodes(eastl::hash_map<const void *, bool> &closedNodes);

  TEXTUREID animCharImageId;
  TEXTUREID compositImageId;
  TEXTUREID dynModelImageId;
  TEXTUREID efxImageId;
  TEXTUREID folderImageId;
  TEXTUREID fxImageId;
  TEXTUREID gameObjImageId;
  TEXTUREID prefabImageId;
  TEXTUREID rendinstImageId;
};
