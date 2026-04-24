// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildJumpLinks.h>
#include <recastTools/recastNavMeshTile.h>
#include <recastTools/recastTools.h>

namespace recastbuild
{
void build_jumplinks_v1(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const JumpLinksParams &jlk_params, const rcCompactHeightfield *chf, const rcHeightfield *solid);
void build_jumplinks_v2(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, dtNavMesh *navmesh, const Tab<Edge> &edges,
  const JumpLinksParams &jlk_params, const rcCompactHeightfield *chf, const rcHeightfield *solid, const BBox3 &tileBox);

void build_jumplinks_connstorage(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, dtNavMesh *navmesh,
  const Tab<Edge> &edges, const JumpLinksParams &jlk_params, const rcCompactHeightfield *chf, const rcHeightfield *solid,
  const BBox3 &tile_box)
{
  if (!jlk_params.enabled)
    return;

  switch (jlk_params.typeGen)
  {
    case JUMPLINKS_TYPEGEN_ORIGINAL: build_jumplinks_v1(out_conn_storage, edges, jlk_params, chf, solid); break;
    case JUMPLINKS_TYPEGEN_NEW2024: build_jumplinks_v2(out_conn_storage, navmesh, edges, jlk_params, chf, solid, tile_box); break;
  }
}

bool remove_off_mesh_connection(recastnavmesh::OffMeshConnectionsStorage &storage, int idx)
{
  if (storage.offMeshCon.size() <= idx)
  {
    logerr("can't remove offmesh connection idx %d from storage. Storage count = %d", idx, storage.offMeshCon.size());
    return false;
  }

  storage.offMeshCon.erase(storage.offMeshCon.begin() + idx);

  return true;
}
} // namespace recastbuild

static Point3 cast_endpoint_to_edge(const Point3 &p, const Point3 &opposite_p, float cell_height, const rcPolyMeshDetail &dmesh)
{
  Point3 res = Point3::ZERO;
  float t, bestDstSq = eastl::numeric_limits<float>::max();
  for (int m = 0; m < dmesh.nmeshes; ++m)
  {
    int startVertex = dmesh.meshes[m * 4 + 0];
    // int numVertices = dmesh.meshes[m*4+1];
    int startTri = dmesh.meshes[m * 4 + 2];
    int numTri = dmesh.meshes[m * 4 + 3];
    for (int i = startTri; i < startTri + numTri; ++i)
    {
      for (int k = 0; k < 3; ++k)
      {
        int n = k == 2 ? 0 : k + 1;
        int p1i = startVertex + dmesh.tris[i * 4 + k];
        int p2i = startVertex + dmesh.tris[i * 4 + n];
        Point3 p1(&dmesh.verts[p1i * 3], Point3::CtorPtrMark::CTOR_FROM_PTR);
        Point3 p2(&dmesh.verts[p2i * 3], Point3::CtorPtrMark::CTOR_FROM_PTR);
        Point3 candidate = closest_pt_on_seg(p, p1, p2, t);
        auto dstSq = (p - candidate).lengthSq();
        if (dstSq < bestDstSq)
        {
          bestDstSq = dstSq;
          res = candidate;
        }
      }
    }
  }

  // ensure it's not 'over' the polygon or else different math kicks in and being above or below becomes important
  Point3 vec = opposite_p - res;
  vec.normalize();
  float saveY = res.y;
  res += vec * 0.02;
  res.y = saveY;

  res.y -= cell_height * 0.5; // poly edges will be lowered by cell_height
  return res;
}

void recastbuild::cross_obstacles_with_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage,
  const rcPolyMeshDetail &dmesh, const BBox3 &box, const JumpLinksParams &jlkParams, float cell_height,
  const Tab<JumpLinkObstacle> &obstacles)
{
  float serachRadius = 0.2;
  for (auto &obstacle : obstacles)
  {
    Point3 center = obstacle.box.center();
    center.y = obstacle.box.lim[0].y + 0.1f;
    if (box & center)
    {
      Point3 dims = obstacle.box.lim[1] - obstacle.box.lim[0];
      Point3 widthVec = dims.x < dims.z ? Point3(dims.x, 0, 0) : Point3(0, 0, dims.z);
      Quat q(Point3(0, 1, 0), obstacle.yAngle);
      widthVec = q * widthVec;
      Point3 widthNorm = normalize(widthVec);
      Point3 p1 = center + (widthVec * 0.5f + widthNorm * (jlkParams.agentRadius + serachRadius));
      Point3 p2 = center - (widthVec * 0.5f + widthNorm * (jlkParams.agentRadius + serachRadius));
      if (box & p1)
      {
        p1 = cast_endpoint_to_edge(p1, p2, cell_height, dmesh);
        if (box & p2)
          p2 = cast_endpoint_to_edge(p2, p1, cell_height, dmesh);
        else
          p2.y = p1.y;
        recastnavmesh::add_off_mesh_connection(out_conn_storage, &p1.x, &p2.x, serachRadius, 1, 0, pathfinder::POLYAREA_OBSTACLE);
      }
      else if (box & p2)
      {
        p2 = cast_endpoint_to_edge(p2, p1, cell_height, dmesh);
        if (box & p1)
          p1 = cast_endpoint_to_edge(p1, p2, cell_height, dmesh);
        else
          p1.y = p2.y;
        recastnavmesh::add_off_mesh_connection(out_conn_storage, &p2.x, &p1.x, serachRadius, 1, 0, pathfinder::POLYAREA_OBSTACLE);
      }
      else // Should be impossible
        logerr("Obstacle center is in tile, but none of the jumplink points are");
    }
  }
}


void recastbuild::add_custom_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const rcPolyMeshDetail &dmesh,
  float cell_height, const Tab<CustomJumpLink> &custom_jump_links, BBox3 box)
{
  for (auto [jumpLinkStart, jumpLinkEnd] : custom_jump_links)
  {
    bool startInBox = box & jumpLinkStart;
    bool endInBox = box & jumpLinkEnd;
    if (!startInBox && !endInBox)
      continue;

    if (startInBox)
      jumpLinkStart = cast_endpoint_to_edge(jumpLinkStart, jumpLinkEnd, cell_height, dmesh);
    if (endInBox)
      jumpLinkEnd = cast_endpoint_to_edge(jumpLinkEnd, jumpLinkStart, cell_height, dmesh);
    recastnavmesh::add_off_mesh_connection(out_conn_storage, &jumpLinkStart.x, &jumpLinkEnd.x, 0.5f, 1);
  }
}

void recastbuild::disable_jumplinks_around_obstacle(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage,
  const Tab<JumpLinkObstacle> &obstacles)
{
  auto verts = out_conn_storage.offMeshCon.get<recastnavmesh::OffMeshCon::Verts>();
  for (auto &obstacle : obstacles)
  {
    Quat q = inverse(Quat(Point3(0, 1, 0), obstacle.yAngle));
    Point3 center = obstacle.box.center();
    for (int i = 0; i < out_conn_storage.offMeshCon.size(); i++)
    {
      for (auto &p : make_span_const(&verts[i].first, 2))
      {
        Point3 rPoint = q * (p - center) + center;
        if (obstacle.box & rPoint)
        {
          if (remove_off_mesh_connection(out_conn_storage, i))
            i--;
          break;
        }
      }
    }
  }
}
