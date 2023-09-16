#pragma once
#include "compositeEditorTreeDataNode.h"
#include <EASTL/hash_map.h>
#include <propPanel2/comWnd/treeview_panel.h>

class CompositeEditorTreeData;

class CompositeEditorTree : public TreeBaseWindow
{
public:
  CompositeEditorTree(ITreeViewEventHandler *event_handler, void *phandle, int x, int y, unsigned w, unsigned h, const char caption[]);

  void fill(CompositeEditorTreeData &treeData, CompositeEditorTreeDataNode *treeDataNodeToSelect, bool keepNodeExpansionState);
  void selectByTreeDataNode(const CompositeEditorTreeDataNode *treeDataNodeToSelect);

private:
  virtual long onWcKeyDown(WindowBase *source, unsigned v_key) override;

  int addIconImage(const char *fileName);
  int getImageIndex(CompositeEditorTreeDataNode &treeDataNode) const;

  void fillInternal(CompositeEditorTreeDataNode &treeDataNode, TLeafHandle parent, CompositeEditorTreeDataNode *treeDataNodeToSelect,
    eastl::hash_map<const void *, bool> &closedNodes);

  void getClosedNodes(eastl::hash_map<const void *, bool> &closedNodes);

  int animCharImageIndex;
  int compositImageIndex;
  int dynModelImageIndex;
  int efxImageIndex;
  int folderImageIndex;
  int fxImageIndex;
  int gameObjImageIndex;
  int prefabImageIndex;
  int rendinstImageIndex;

  int gameObjStateImageIndex;
};
