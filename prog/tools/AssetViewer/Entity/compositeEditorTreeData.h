// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "compositeEditorTreeDataNode.h"
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

  static CompositeEditorTreeDataNode *getTreeDataNodeParent(CompositeEditorTreeDataNode &searchFor,
    CompositeEditorTreeDataNode &searchIn, int &nodeIndex);

  CompositeEditorTreeDataNode rootNode;
  String assetName;
  bool isComposite;

private:
  static void setDataBlockIdsInternal(CompositeEditorTreeDataNode &treeDataNode, int &nextDataBlockId);
};
