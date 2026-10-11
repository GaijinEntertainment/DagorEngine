// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorTreeDataNode.h"
#include <EASTL/functional.h>
#include <util/dag_string.h>

class DagorAsset;
class DataBlock;

class CompositeEditorTreeData
{
public:
  CompositeEditorTreeData();

  void reset();
  void convertAssetToTreeData(const DagorAsset *asset, const DataBlock *block);
  void setDataBlockIds();

  static void convertTreeDataToDataBlock(const CompositeEditorTreeDataNode &treeDataNode, DataBlock &block);
  static void convertDataBlockToTreeData(const DataBlock &block, CompositeEditorTreeDataNode &treeDataNode);
  static bool isCompositeAsset(const DagorAsset *asset);
  static CompositeEditorTreeDataNode *getTreeDataNodeByDataBlockId(CompositeEditorTreeDataNode &treeDataNode, unsigned dataBlockId);

  static CompositeEditorTreeDataNode *getTreeDataNodeParent(const CompositeEditorTreeDataNode &searchFor,
    CompositeEditorTreeDataNode &searchIn, int &nodeIndex);

  // Visits every node in pre-order DFS, calling callback(node, order) at each visit.
  // order starts at 0 and increments by one per visited node.
  static void traverseDepthFirst(CompositeEditorTreeDataNode &root,
    const eastl::function<void(CompositeEditorTreeDataNode &, int)> &callback);

  CompositeEditorTreeDataNode rootNode;
  String assetName;
  bool isComposite;

private:
  static bool isSavedToDataBlock(const CompositeEditorTreeDataNode &treeDataNode);
  static bool hasParamInSubtree(const CompositeEditorTreeDataNode &treeDataNode);
  static void invalidateDataBlockIds(CompositeEditorTreeDataNode &treeDataNode);
  static void setDataBlockIdsInternal(CompositeEditorTreeDataNode &treeDataNode, int &nextDataBlockId);
};
