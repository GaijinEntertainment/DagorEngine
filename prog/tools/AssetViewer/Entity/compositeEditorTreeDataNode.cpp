// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorTreeDataNode.h"
#include <assets/asset.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_interface.h>
#include <util/dag_string.h>

CompositeEditorTreeDataNode::CompositeEditorTreeDataNode() { reset(); }

void CompositeEditorTreeDataNode::reset()
{
  params.reset();
  nodes.clear();
  dataBlockId = IDataBlockIdHolder::invalid_id;
}

const char *CompositeEditorTreeDataNode::getAssetName(const char *defaultValue) const { return params.getStr("name", defaultValue); }

const char *CompositeEditorTreeDataNode::getAssetTypeName() const
{
  const int paramIndex = params.findParam("name");
  if (paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_STRING)
  {
    const char *name = params.getStr(paramIndex);
    if (name == nullptr || *name == 0)
      return nullptr;

    DagorAsset *asset = DAEDITOR3.getGenObjAssetByName(name);
    if (asset)
      return asset->getTypeStr();
  }

  return nullptr;
}

const char *CompositeEditorTreeDataNode::getName() const
{
  const int paramIndex = params.findParam("name");
  if (paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_STRING)
  {
    const char *name = params.getStr(paramIndex);
    if (name == nullptr || *name == 0)
      return "--";
    return name;
  }

  if (isEntBlock())
    return "--";

  if (hasEntBlock())
    return "random";

  return params.getBlockName();
}

float CompositeEditorTreeDataNode::getWeight() const
{
  const int paramIndex = params.findParam("weight");
  if (paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_REAL)
    return params.getReal(paramIndex);

  return 1.0f;
}

bool CompositeEditorTreeDataNode::getUseTransformationMatrix() const
{
  const int paramIndex = params.findParam("tm");
  return paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_MATRIX;
}

TMatrix CompositeEditorTreeDataNode::getTransformationMatrix() const
{
  const int paramIndex = params.findParam("tm");
  if (paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_MATRIX)
    return params.getTm(paramIndex);

  return TMatrix::IDENT;
}

bool CompositeEditorTreeDataNode::canTransform() const
{
  Point2 p2;
  return getUseTransformationMatrix() && !tryGetPoint2Parameter("offset_x", p2) && !tryGetPoint2Parameter("offset_y", p2) &&
         !tryGetPoint2Parameter("offset_z", p2) && !tryGetPoint2Parameter("rot_x", p2) && !tryGetPoint2Parameter("rot_y", p2) &&
         !tryGetPoint2Parameter("rot_z", p2) && !tryGetPoint2Parameter("scale", p2) && !tryGetPoint2Parameter("xScale", p2) &&
         !tryGetPoint2Parameter("yScale", p2);
}

bool CompositeEditorTreeDataNode::tryGetPoint2Parameter(const char *name, Point2 &value) const
{
  const int paramIndex = params.findParam(name);
  if (paramIndex >= 0 && params.getParamType(paramIndex) == DataBlock::TYPE_POINT2)
  {
    value = params.getPoint2(paramIndex);
    return true;
  }

  return false;
}

bool CompositeEditorTreeDataNode::isEntBlock() const
{
  const char *blockName = params.getBlockName();
  return blockName != nullptr && strcmp(blockName, "ent") == 0;
}

bool CompositeEditorTreeDataNode::hasEntBlock() const
{
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i]->isEntBlock())
      return true;

  return false;
}

bool CompositeEditorTreeDataNode::hasNameParameter() const { return params.findParam("name") >= 0; }

bool CompositeEditorTreeDataNode::canEditAssetName(bool isRootNode) const { return !isRootNode && !hasEntBlock(); }

bool CompositeEditorTreeDataNode::canEditChildren() const { return !isEntBlock(); }

bool CompositeEditorTreeDataNode::canEditRandomEntities(bool isRootNode) const
{
  return !isRootNode && !hasNameParameter() && !isEntBlock();
}

void CompositeEditorTreeDataNode::insertEntBlock(int index)
{
  eastl::unique_ptr<CompositeEditorTreeDataNode> subNode = eastl::make_unique<CompositeEditorTreeDataNode>();
  subNode->params.changeBlockName("ent");
  subNode->params.addStr("name", "");
  subNode->params.addReal("weight", 1.0f);
  if (index >= 0)
    nodes.insert(nodes.begin() + index, std::move(subNode));
  else
    nodes.push_back(std::move(subNode));
}

void CompositeEditorTreeDataNode::insertNodeBlock(int index)
{
  eastl::unique_ptr<CompositeEditorTreeDataNode> subNode = eastl::make_unique<CompositeEditorTreeDataNode>();
  subNode->params.changeBlockName("node");
  if (index >= 0)
    nodes.insert(nodes.begin() + index, std::move(subNode));
  else
    nodes.push_back(std::move(subNode));
}

void CompositeEditorTreeDataNode::convertSingleRandomEntityNodeToRegularNode(bool isRootNode)
{
  if (nodes.size() != 1 || !canEditRandomEntities(isRootNode) || !nodes[0]->isEntBlock())
    return;

  const String assetName(nodes[0]->getAssetName());
  if (assetName.empty())
    return;

  nodes.clear();
  insertNodeBlock(-1);
  nodes[0]->params.setStr("name", assetName);
}
