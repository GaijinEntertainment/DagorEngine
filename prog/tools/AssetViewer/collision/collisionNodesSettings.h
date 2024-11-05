// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/math/kdop.h>
#include <libTools/collision/exportCollisionNodeType.h>
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>

struct SelectedNodesSettings
{
  bool replaceNodes = false;
  bool isPhysCollidable = false;
  bool isTraceable = false;
  ExportCollisionNodeType type = ExportCollisionNodeType::MESH;
  String nodeName;
  SimpleString physMat;
  dag::Vector<String> refNodes;

  void readProps(const DataBlock *props);
  DataBlock *saveProps(DataBlock *props) const;
  void removeProps(DataBlock *props);
};

struct KdopSettings
{
  KdopPreset preset = KdopPreset::SET_EMPTY;
  int segmentsCountX = 0;
  int segmentsCountY = 0;
  int rotX = 0;
  int rotY = 0;
  int rotZ = 0;
  float cutOffThreshold = 0;
  SelectedNodesSettings selectedNodes;

  void readProps(const DataBlock *props);
  void saveProps(DataBlock *props) const;
  void removeProps(DataBlock *props);
};

struct ConvexVhacdSettings
{
  int depth = 0;
  int maxHulls = 0;
  int maxVerts = 0;
  int resolution = 0;
  SelectedNodesSettings selectedNodes;

  void readProps(const DataBlock *props);
  void saveProps(DataBlock *props) const;
  void removeProps(DataBlock *props);
};

struct ConvexComputerSettings
{
  float shrink = 0.0f;
  SelectedNodesSettings selectedNodes;

  void readProps(const DataBlock *props);
  void saveProps(DataBlock *props) const;
  void removeProps(DataBlock *props);
};
