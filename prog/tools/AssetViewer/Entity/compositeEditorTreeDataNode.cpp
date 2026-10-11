// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "compositeEditorTreeDataNode.h"
#include <assets/asset.h>
#include <de3_composit.h>
#include <de3_dataBlockIdHolder.h>
#include <de3_interface.h>
#include <math/dag_mathAng.h>
#include <util/dag_string.h>

static void update_point2_parameter_value(CompositeEditorTreeDataNode &node, const char *name, real value)
{
  Point2 parameter;
  if (!node.tryGetPoint2Parameter(name, parameter))
    return;
  parameter.x = value;
  node.params.setPoint2(name, parameter);
}

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

bool CompositeEditorTreeDataNode::isCompositeAsset() const
{
  const char *assetTypeName = getAssetTypeName();
  return assetTypeName != nullptr && strcmp(assetTypeName, "composit") == 0;
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

void CompositeEditorTreeDataNode::getTransformationComponents(Point3 &position, Point3 &rotation, Point3 &scale) const
{
  const TMatrix tm = getTransformationMatrix();
  position = tm.getcol(3);

  ::matrix_to_euler(tm, rotation.y, rotation.z, rotation.x);
  rotation.x = RadToDeg(rotation.x);
  rotation.y = RadToDeg(rotation.y);
  rotation.z = RadToDeg(rotation.z);

  scale = Point3(::length(tm.getcol(0)), ::length(tm.getcol(1)), ::length(tm.getcol(2)));
}

void CompositeEditorTreeDataNode::setIdentityTransformationMatrix() { params.setTm("tm", TMatrix::IDENT); }

void CompositeEditorTreeDataNode::updateTransformParamsFromMatrix()
{
  if (!getUseTransformationMatrix())
    return;

  Point3 position, rotation, scale;
  getTransformationComponents(position, rotation, scale);
  rotation = composit_rotation_params(rotation);

  update_point2_parameter_value(*this, "rot_x", rotation.x);
  update_point2_parameter_value(*this, "rot_y", rotation.y);
  update_point2_parameter_value(*this, "rot_z", rotation.z);

  update_point2_parameter_value(*this, "offset_x", position.x);
  update_point2_parameter_value(*this, "offset_y", position.y);
  update_point2_parameter_value(*this, "offset_z", position.z);

  update_point2_parameter_value(*this, "scale", scale.x);

  // yScale is the Y size divided by scale. At a zero scale composit_y_scale_param() returns the default 1, which would
  // overwrite the stored multiplier.
  if (scale.x > 0)
    update_point2_parameter_value(*this, "yScale", composit_y_scale_param(scale.x, scale.y));
}

bool CompositeEditorTreeDataNode::canTransform() const { return getUseTransformationMatrix(); }

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

bool CompositeEditorTreeDataNode::hasChildNode(unsigned dataBlockId) const
{
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i]->dataBlockId == dataBlockId)
      return true;

  return false;
}

bool CompositeEditorTreeDataNode::hasChildTreeDataNode(const CompositeEditorTreeDataNode *node) const
{
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].get() == node)
      return true;

  return false;
}

bool CompositeEditorTreeDataNode::isAncestorOfNode(unsigned dataBlockId) const
{
  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i]->dataBlockId == dataBlockId || nodes[i]->isAncestorOfNode(dataBlockId))
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
  subNode->setIdentityTransformationMatrix();
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
