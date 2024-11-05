// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorTreeData.h"
#include <assets/asset.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>

CompositeEditorTreeData::CompositeEditorTreeData() { reset(); }

void CompositeEditorTreeData::reset()
{
  rootNode.reset();
  assetName.clear();
  isComposite = false;
}

void CompositeEditorTreeData::convertDataBlockToTreeData(const DataBlock &block, CompositeEditorTreeDataNode &treeDataNode)
{
  for (int blockIndex = 0; blockIndex < block.blockCount(); ++blockIndex)
  {
    const DataBlock *subBlock = block.getBlock(blockIndex);

    eastl::unique_ptr<CompositeEditorTreeDataNode> treeDataSubNode = eastl::make_unique<CompositeEditorTreeDataNode>();
    convertDataBlockToTreeData(*subBlock, *treeDataSubNode);
    treeDataNode.nodes.push_back(std::move(treeDataSubNode));
  }

  treeDataNode.params.setParamsFrom(&block);

  // This logic is based on the code in compositMgrService.cpp.
  if (treeDataNode.params.findParam("place_type") < 0)
  {
    if (treeDataNode.params.getBool("placeOnCollision", false))
    {
      treeDataNode.params.removeParam("placeOnCollision");

      if (treeDataNode.params.getBool("useCollisionNormal", false))
      {
        treeDataNode.params.removeParam("useCollisionNormal");
        treeDataNode.params.addInt("place_type", ICompositObj::Props::PT_collNorm);
      }
      else
      {
        treeDataNode.params.addInt("place_type", ICompositObj::Props::PT_coll);
      }
    }
  }

  const char *blockName = block.getBlockName();
  if (blockName && *blockName)
    treeDataNode.params.changeBlockName(blockName);
}

void CompositeEditorTreeData::setDataBlockIdsInternal(CompositeEditorTreeDataNode &treeDataNode, int &nextDataBlockId)
{
  // NOTE: the logic in CompositeEditorTreeData::setDataBlockIds and in CompositEntityPool::loadAssetData must match,
  // otherwise the data block IDs will not be in sync.
  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];
    const char *subBlockName = treeDataSubNode.params.getBlockName();

    if (strcmp(subBlockName, "colors") == 0 || strcmp(subBlockName, "ent") == 0 || strcmp(subBlockName, "spline") == 0 ||
        strcmp(subBlockName, "polygon") == 0)
      continue;

    for (int j = 0; j < treeDataSubNode.params.paramCount(); ++j)
      if (treeDataSubNode.params.getParamType(j) == DataBlock::TYPE_STRING &&
          strcmp(treeDataSubNode.params.getParamName(j), "name") == 0)
        treeDataSubNode.dataBlockId = nextDataBlockId++;

    for (int j = 0; j < treeDataSubNode.nodes.size(); j++)
      if (strcmp(treeDataSubNode.nodes[j]->params.getBlockName(), "ent") == 0)
        treeDataSubNode.nodes[j]->dataBlockId = nextDataBlockId++;

    setDataBlockIdsInternal(treeDataSubNode, nextDataBlockId);
  }
}

void CompositeEditorTreeData::setDataBlockIds()
{
  G_STATIC_ASSERT(IDataBlockIdHolder::invalid_id == 0);
  int nextDataBlockId = 1;
  setDataBlockIdsInternal(rootNode, nextDataBlockId);
}

void CompositeEditorTreeData::convertAssetToTreeData(const DagorAsset *asset, const DataBlock *block)
{
  reset();

  if (!isCompositeAsset(asset) || !block)
    return;

  assetName = asset->getName();
  isComposite = true;
  convertDataBlockToTreeData(*block, rootNode);
  setDataBlockIds();
}

void CompositeEditorTreeData::convertTreeDataToDataBlock(const CompositeEditorTreeDataNode &treeDataNode, DataBlock &block)
{
  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    const CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    if (treeDataSubNode.params.paramCount() == 0 && treeDataSubNode.nodes.empty())
      continue;

    DataBlock *subBlock = block.addNewBlock((const char *)nullptr);
    convertTreeDataToDataBlock(treeDataSubNode, *subBlock);
  }

  block.setParamsFrom(&treeDataNode.params);

  const char *blockName = treeDataNode.params.getBlockName();
  if (blockName && *blockName)
    block.changeBlockName(blockName);
}

bool CompositeEditorTreeData::isCompositeAsset(const DagorAsset *asset)
{
  if (!asset)
    return false;

  const char *type = asset->getTypeStr();
  return type && strcmp(type, "composit") == 0;
}

CompositeEditorTreeDataNode *CompositeEditorTreeData::getTreeDataNodeByDataBlockId(CompositeEditorTreeDataNode &treeDataNode,
  unsigned dataBlockId)
{
  G_ASSERT(dataBlockId != IDataBlockIdHolder::invalid_id);

  for (int nodeIndex = 0; nodeIndex < treeDataNode.nodes.size(); ++nodeIndex)
  {
    CompositeEditorTreeDataNode &treeDataSubNode = *treeDataNode.nodes[nodeIndex];

    if (treeDataSubNode.dataBlockId == dataBlockId)
      return &treeDataSubNode;

    CompositeEditorTreeDataNode *result = getTreeDataNodeByDataBlockId(treeDataSubNode, dataBlockId);
    if (result)
      return result;
  }

  return nullptr;
}

CompositeEditorTreeDataNode *CompositeEditorTreeData::getTreeDataNodeParent(CompositeEditorTreeDataNode &searchFor,
  CompositeEditorTreeDataNode &searchIn, int &nodeIndex)
{
  for (int i = 0; i < searchIn.nodes.size(); ++i)
  {
    CompositeEditorTreeDataNode *child = searchIn.nodes[i].get();
    if (child == &searchFor)
    {
      nodeIndex = i;
      return &searchIn;
    }

    CompositeEditorTreeDataNode *result = getTreeDataNodeParent(searchFor, *child, nodeIndex);
    if (result)
      return result;
  }

  return nullptr;
}
