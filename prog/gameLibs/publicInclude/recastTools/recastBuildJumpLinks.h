//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
enum
{
  JUMPLINKS_TYPEGEN_ORIGINAL,
  JUMPLINKS_TYPEGEN_NEW2024,
};

struct JumpLinksParams
{
  bool enabled;
  int typeGen;

  float jumpoffMinHeight;
  float jumpoffMaxHeight;
  float jumpoffMinLinkLength;

  float edgeMappingAngle;
  float edgeMergeAngle;
  float edgeMergeDist;

  float jumpHeight; // up + down
  float jumpLength;
  float width;
  float agentHeight;
  float agentMinSpace;

  float deltaHeightThreshold;
  float maxObstructionAngle;

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

bool check_jump_max_height(const rcHeightfield *solid, const Point3 &sp, const Point3 &sq, const float &agentHeight,
  const float &agentMinSpace, float &maxHeight);

void build_jumplinks_connstorage(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, dtNavMesh *navmesh,
  const Tab<Edge> &edges, const JumpLinksParams &jlkParams, const rcCompactHeightfield *chf, const rcHeightfield *solid,
  const BBox3 &tileBox);
void cross_obstacles_with_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const rcPolyMeshDetail &dmesh,
  const BBox3 &box, const JumpLinksParams &jlkParams, float cell_height, const Tab<JumpLinkObstacle> &obstacles);
void add_custom_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const rcPolyMeshDetail &dmesh, float cell_height,
  const Tab<CustomJumpLink> &custom_jump_links, BBox3 box);
void disable_jumplinks_around_obstacle(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage,
  const Tab<JumpLinkObstacle> &obstacles);
} // namespace recastbuild
