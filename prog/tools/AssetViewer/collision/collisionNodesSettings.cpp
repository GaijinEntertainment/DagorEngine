// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collisionNodesSettings.h"
#include <util/dag_lookup.h>

void SelectedNodesSettings::readProps(const DataBlock *props)
{
  const char *collision = props->getStr("collision", nullptr);
  type = get_export_type_by_name(collision);
  replaceNodes = props->getBool("replaceNodes", false);
  isPhysCollidable = props->getBool("isPhysCollidable", false);
  isTraceable = props->getBool("isTraceable", false);
  nodeName = props->getBlockName();
  physMat = props->getStr("phmat", "default");
  if (const DataBlock *refNodesBlock = props->getBlockByName("refNodes"))
  {
    for (int i = 0; i < refNodesBlock->blockCount(); ++i)
    {
      refNodes.push_back() = refNodesBlock->getBlock(i)->getBlockName();
    }
  }
}

DataBlock *SelectedNodesSettings::saveProps(DataBlock *props) const
{
  DataBlock *node = props->addBlock(nodeName);
  node->setStr("collision", get_export_type_name(type));
  node->setStr("phmat", physMat.str());
  node->setBool("replaceNodes", replaceNodes);
  node->setBool("isPhysCollidable", isPhysCollidable);
  node->setBool("isTraceable", isTraceable);
  DataBlock *refNodesBlock = node->addBlock("refNodes");
  for (const auto &refNode : refNodes)
  {
    refNodesBlock->addBlock(refNode);
  }
  return node;
}

void SelectedNodesSettings::removeProps(DataBlock *props)
{
  props->removeParam("collision");
  props->removeParam("phmat");
  props->removeParam("replaceNodes");
  props->removeParam("isPhysCollidable");
  props->removeParam("isTraceable");
  props->removeBlock("refNodes");
}

void KdopSettings::readProps(const DataBlock *props)
{
  selectedNodes.readProps(props);
  preset = static_cast<KdopPreset>(props->getInt("kdopPreset", -1));
  segmentsCountX = props->getInt("kdopSegmentsX", -1);
  segmentsCountY = props->getInt("kdopSegmentsY", -1);
  rotX = props->getInt("kdopRotX", 0);
  rotY = props->getInt("kdopRotY", 0);
  rotZ = props->getInt("kdopRotZ", 0);
  cutOffThreshold = props->getReal("cutOffThreshold", 0.0f);
}

void KdopSettings::saveProps(DataBlock *props) const
{
  DataBlock *node = selectedNodes.saveProps(props);
  node->setInt("kdopPreset", preset);
  node->setInt("kdopSegmentsX", segmentsCountX);
  node->setInt("kdopSegmentsY", segmentsCountY);
  node->setInt("kdopRotX", rotX);
  node->setInt("kdopRotY", rotY);
  node->setInt("kdopRotZ", rotZ);
  node->setReal("cutOffThreshold", cutOffThreshold);
}

void KdopSettings::removeProps(DataBlock *props)
{
  selectedNodes.removeProps(props);
  props->removeParam("kdopPreset");
  props->removeParam("kdopSegmentsX");
  props->removeParam("kdopSegmentsY");
  props->removeParam("kdopRotX");
  props->removeParam("kdopRotY");
  props->removeParam("kdopRotZ");
  props->removeParam("cutOffThreshold");
}

void ConvexVhacdSettings::readProps(const DataBlock *props)
{
  selectedNodes.readProps(props);
  depth = props->getInt("convexDepth", -1);
  maxHulls = props->getInt("maxConvexHulls", -1);
  maxVerts = props->getInt("maxConvexVerts", -1);
  resolution = props->getInt("convexResolution", -1);
}

void ConvexVhacdSettings::saveProps(DataBlock *props) const
{
  DataBlock *node = selectedNodes.saveProps(props);
  node->setInt("convexDepth", depth);
  node->setInt("maxConvexHulls", maxHulls);
  node->setInt("maxConvexVerts", maxVerts);
  node->setInt("convexResolution", resolution);
}

void ConvexVhacdSettings::removeProps(DataBlock *props)
{
  selectedNodes.removeProps(props);
  props->removeParam("convexDepth");
  props->removeParam("maxConvexHulls");
  props->removeParam("maxConvexVerts");
  props->removeParam("convexResolution");
}

void ConvexComputerSettings::readProps(const DataBlock *props)
{
  selectedNodes.readProps(props);
  shrink = props->getReal("shrink");
}

void ConvexComputerSettings::saveProps(DataBlock *props) const
{
  DataBlock *node = selectedNodes.saveProps(props);
  node->setReal("shrink", shrink);
}

void ConvexComputerSettings::removeProps(DataBlock *props)
{
  selectedNodes.removeProps(props);
  props->removeParam("shrink");
}
