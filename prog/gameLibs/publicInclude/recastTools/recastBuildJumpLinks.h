//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include "recastBuildEdges.h"
#include <pathFinder/pathFinder.h>

namespace recastnavmesh
{
struct OffMeshConnectionsStorage;
}

namespace recastbuild
{
struct JumpLinksParams
{
  bool enabled;
  float jumpHeight; // up + down
  float jumpLength;
  float width;
  float agentHeight;
  float agentMinSpace;

  float deltaHeightThreshold;

  float linkDegAngle;
  float linkDegDist;

  float agentRadius;

  // height difference from edge to link when should appear two jumplinks instead of one
  // 0 - disabled
  float complexJumpTheshold;
  bool crossObstaclesWithJumplinks;
  bool enableCustomJumplinks;
};

struct JumpLinkObstacle
{
  BBox3 box;
  float yAngle;
};


struct CustomJumpLink
{
  Point3 from;
  Point3 to;
};

void build_jumplinks_connstorage(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const JumpLinksParams &jlkParams, const rcCompactHeightfield *chf, const rcHeightfield *solid);
void cross_obstacles_with_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const BBox3 &box, const JumpLinksParams &jlkParams, const Tab<JumpLinkObstacle> &obstacles);
void add_custom_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const Tab<CustomJumpLink> &custom_jump_links);
void disable_jumplinks_around_obstacle(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage,
  const Tab<JumpLinkObstacle> &obstacles);
} // namespace recastbuild
