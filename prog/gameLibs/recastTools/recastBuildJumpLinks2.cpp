// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildJumpLinks.h>
#include <recastTools/recastNavMeshTile.h>
#include <detourNavMeshQuery.h>
#include <pathFinder/pathFinder.h>
#include <recastTools/recastTools.h>

#include "commonLineSampler.h"

namespace recastbuild
{
struct Jumplink
{
  Edge start;
  Edge end;
  Edge obstructionLine;
  bool merged = false;
};

void add_off_mesh_connection(recastnavmesh::OffMeshConnectionsStorage &storage, const float *spos, const float *epos, float rad,
  unsigned char bidir, unsigned short flags = pathfinder::POLYFLAG_JUMP, unsigned short area = pathfinder::POLYAREA_JUMP);

static bool find_nearest_navmesh_point(const dtNavMeshQuery *nav_query, Point3 &pos, float horz_extents, float vert_extend)
{
  if (!nav_query)
    return false;
  dtQueryFilter filter;
  filter.setIncludeFlags(pathfinder::POLYFLAG_GROUND);
  const Point3 extents(horz_extents, vert_extend, horz_extents);
  Point3 resPos;
  dtPolyRef nearestPoly;
  if (dtStatusSucceed(nav_query->findNearestPoly(&pos.x, &extents.x, &filter, &nearestPoly, &resPos.x)) && nearestPoly != 0)
  {
    pos = resPos;
    return true;
  }
  return false;
}

static float distance_pt_to_edge_sq(const Point3 &pt, const Edge &edge)
{
  const Point3 dir = edge.sq - edge.sp;
  const float t = dot(pt - edge.sp, dir) / lengthSq(dir);
  const Point3 closestPt = lerp(edge.sp, edge.sq, t);
  return lengthSq(pt - closestPt);
}

static float distance_pt_to_edge(const Point3 &pt, const Edge &edge) { return sqrt(distance_pt_to_edge_sq(pt, edge)); }

static Point3 constrain_point_to_edge(const Point3 &pt, const Edge &edge)
{
  const Point3 dir = edge.sq - edge.sp;
  const float t = dot(pt - edge.sp, dir) / lengthSq(dir);
  return lerp(edge.sp, edge.sq, clamp(t, 0.0f, 1.0f));
}

static float distance_between_edges_sq(const Edge &edge1, const Edge &edge2, Point3 *edge1_pt = nullptr, Point3 *edge2_pt = nullptr)
{
  const Point3 edge1Dir = edge1.sq - edge1.sp;
  const Point3 edge2Dir = edge2.sq - edge2.sp;

  if (abs(dot(edge1Dir, edge2Dir)) >= 1.0f - FLT_EPSILON)
  {
    float dist1 = lengthSq(edge1.sp - constrain_point_to_edge(edge1.sp, edge2));
    float dist2 = lengthSq(edge1.sq - constrain_point_to_edge(edge1.sq, edge2));
    return min(dist1, dist2);
  }

  const float lineDirSqrMag = lengthSq(edge2Dir);

  const Point3 inPlaneA = edge1.sp - (safediv(dot(edge1.sp - edge2.sp, edge2Dir), lineDirSqrMag) * edge2Dir); //-V778
  const Point3 inPlaneB = edge1.sq - (safediv(dot(edge1.sq - edge2.sp, edge2Dir), lineDirSqrMag) * edge2Dir); //-V778
  const Point3 inPlaneBA = inPlaneB - inPlaneA;

  float t = safediv(dot(edge2.sp - inPlaneA, inPlaneBA), dot(inPlaneBA, inPlaneBA));
  t = (inPlaneA != inPlaneB) ? t : 0.0f;

  const Point3 segABtoLineCD = lerp(edge1.sp, edge1.sq, clamp(t, 0.0f, 1.0f));

  const Point3 segCDtoSegAB = constrain_point_to_edge(segABtoLineCD, edge2);
  const Point3 segABtoSegCD = constrain_point_to_edge(segCDtoSegAB, edge1);

  if (edge1_pt != nullptr && edge2_pt != nullptr)
  {
    *edge1_pt = segABtoSegCD;
    *edge2_pt = segCDtoSegAB;
  }

  return lengthSq(segCDtoSegAB - segABtoLineCD);
}

enum EdgeClipResult
{
  EDGE_CLIP_RESULT_INSIDE,
  EDGE_CLIP_RESULT_OUTSIDE,
  EDGE_CLIP_RESULT_INTERSECT
};
static Point3 clip_edge_by_plane(const Edge &edge, const Point3 &plane_point, const Point3 &plane_normal, EdgeClipResult &clip_result)
{
  const float planeConst = dot(plane_normal, plane_point);

  const float d1 = dot(plane_normal, edge.sp) - planeConst;
  const float d2 = dot(plane_normal, edge.sq) - planeConst;
  if (abs(d1 - d2) < 0.001f)
  {
    clip_result = d1 > 0.0f ? EDGE_CLIP_RESULT_INSIDE : EDGE_CLIP_RESULT_OUTSIDE;
    return edge.sq;
  }

  if (d1 * d2 > 0)
  {
    if (d1 > 0)
    {
      clip_result = EDGE_CLIP_RESULT_INSIDE;
      return edge.sp;
    }
    else
    {
      clip_result = EDGE_CLIP_RESULT_OUTSIDE;
      return edge.sq;
    }
  }

  const float t = d1 / (d1 - d2);
  const Point3 intersection = edge.sp + t * (edge.sq - edge.sp);
  clip_result = EDGE_CLIP_RESULT_INTERSECT;

  return intersection;
}

static bool clip_edge_by_mapping_angle(const Edge &base_edge, float mapping_angle, const Edge &edge_to_clip, Edge &clipped_edge)
{
  Point3 clippingPlanePoint = base_edge.sp;
  Point3 clippingPlaneNormal = normalize(cross(Point3(0.0f, 1.0f, 0.0f), (base_edge.sq - base_edge.sp)));
  clippingPlaneNormal = Quat(Point3(0.0f, 1.0f, 0.0f), -DegToRad(mapping_angle)) * clippingPlaneNormal;

  EdgeClipResult clipResult;
  Point3 clippedPoint1 = clip_edge_by_plane(edge_to_clip, clippingPlanePoint, clippingPlaneNormal, clipResult);
  if (clipResult == EDGE_CLIP_RESULT_OUTSIDE)
    return false;
  if (clipResult == EDGE_CLIP_RESULT_INSIDE)
    clippedPoint1 = edge_to_clip.sq;

  clippingPlanePoint = base_edge.sq;
  clippingPlaneNormal = normalize(cross(Point3(0.0f, 1.0f, 0.0f), (base_edge.sq - base_edge.sp)));
  clippingPlaneNormal = Quat(Point3(0.0f, 1.0f, 0.0f), DegToRad(mapping_angle)) * clippingPlaneNormal;

  Point3 clippedPoint2 = clip_edge_by_plane(edge_to_clip, clippingPlanePoint, clippingPlaneNormal, clipResult);
  if (clipResult == EDGE_CLIP_RESULT_OUTSIDE)
    return false;
  if (clipResult == EDGE_CLIP_RESULT_INSIDE)
    clippedPoint2 = edge_to_clip.sp;

  clipped_edge = Edge{clippedPoint1, clippedPoint2};

  return true;
}

static bool can_merge_edges(const Edge &edge1, const Edge &edge2, float merge_angle, float merge_dist)
{
  const Point3 edge1Dir = normalize(edge1.sq - edge1.sp);
  const Point3 edge2Dir = normalize(edge2.sq - edge2.sp);

  if (abs(dot(edge1Dir, edge2Dir)) < cos(DegToRad(merge_angle)))
    return false;

  if (distance_between_edges_sq(edge1, edge2) > sqr(merge_dist))
    return false;

  return true;
}

static Edge merge_edges(const Edge &edge1, const Edge &edge2)
{
  float maxDistSq = 0;
  int maxDistIdx = 0;
  Edge edges[6] = {edge1, edge2, {edge1.sp, edge2.sp}, {edge1.sp, edge2.sq}, {edge1.sq, edge2.sp}, {edge1.sq, edge2.sq}};

  for (int i = 0; i < 6; i++)
  {
    float distSq = (edges[i].sq - edges[i].sp).lengthSq();
    if (distSq > maxDistSq)
    {
      maxDistSq = distSq;
      maxDistIdx = i;
    }
  }

  return edges[maxDistIdx];
}

static bool merge_jumplinks(const Jumplink &jumplink1, const Jumplink &jumplink2, float merge_angle, float merge_dist,
  Jumplink &out_jumplink)
{
  if (can_merge_edges(jumplink1.start, jumplink2.start, merge_angle, merge_dist) &&
      can_merge_edges(jumplink1.end, jumplink2.end, merge_angle, merge_dist))
  {
    out_jumplink = Jumplink{merge_edges(jumplink1.start, jumplink2.start), merge_edges(jumplink1.end, jumplink2.end)};
    return true;
  }
  if (can_merge_edges(jumplink1.start, jumplink2.end, merge_angle, merge_dist) &&
      can_merge_edges(jumplink1.end, jumplink2.start, merge_angle, merge_dist))
  {
    out_jumplink = Jumplink{merge_edges(jumplink1.start, jumplink2.end), merge_edges(jumplink1.end, jumplink2.start)};
    return true;
  }
  return false;
}

static Tab<Jumplink> merge_all_jumplinks(Tab<Jumplink> &jumplinks, float merge_angle, float merge_dist)
{
  Tab<Jumplink> mergedJumplinks(tmpmem);
  mergedJumplinks.reserve(jumplinks.size() / 2);

  for (int i = 0; i < jumplinks.size(); i++)
  {
    if (jumplinks[i].merged)
      continue;
    Jumplink mergedJumplink = jumplinks[i];
    bool merged = false;
    for (int j = i + 1; j < jumplinks.size(); j++)
    {
      if (i == j || jumplinks[j].merged)
      {
        continue;
      }
      if (merge_jumplinks(mergedJumplink, jumplinks[j], merge_angle, merge_dist, mergedJumplink))
      {
        if (dot(mergedJumplink.start.sq - mergedJumplink.start.sp, mergedJumplink.end.sq - mergedJumplink.end.sp) < 0)
          mergedJumplink.end = Edge{mergedJumplink.end.sq, mergedJumplink.end.sp};
        jumplinks[j].merged = true;
        merged = true;
      }
    }
    jumplinks[i].merged = merged;
    mergedJumplinks.push_back(mergedJumplink);
  }

  return mergedJumplinks;
}

static float get_jump_down_trajectory_height(float x) { return -x * x * x * 3.0f; }

static bool check_jumpoff_fall(const rcHeightfield *solid, const JumpLinksParams &jlk_params, const Point3 &start_pos,
  const Point3 &dir, Point3 &fall_pos)
{
  const float *orig = solid->bmin;
  const float cellSize = solid->cs;
  const float cellHeight = solid->ch;
  const int hfWidth = solid->width;
  const int hfHeight = solid->height;

  const float agentHeight = jlk_params.agentHeight;
  const float allowedSpace = jlk_params.agentHeight * 0.1f;

  float prevAgentY = start_pos.y;
  float prevCellFloor = start_pos.y;
  bool res = false;

  // 1. check for jump down trajectory
  const float maxWalkForwardSamples = ceilf(jlk_params.agentRadius * 2.f / cellSize);
  bool isFalling = false;
  bool foundObstruction = false;
  for (int i = 0; !foundObstruction; i++)
  {
    isFalling = isFalling || i > maxWalkForwardSamples;
    const float dist = cellSize * i;
    const float trajHeight = isFalling ? get_jump_down_trajectory_height(dist) : 0.0f;
    if (trajHeight < -jlk_params.jumpoffMaxHeight)
      break;
    const Point3 pt = start_pos + dir * dist;
    const float agentY = pt.y + trajHeight;
    const int ix = (int)floorf((pt.x - orig[0]) / cellSize);
    const int iz = (int)floorf((pt.z - orig[2]) / cellSize);
    if (ix < 0 || iz < 0 || ix >= hfWidth || iz >= hfHeight)
      break;

    const rcSpan *span = solid->spans[ix + iz * hfWidth];
    float prevFloor = -FLT_MAX;
    for (;;)
    {
      float ymin = span ? orig[1] + span->smin * cellHeight : FLT_MAX;
      float ymax = span ? orig[1] + span->smax * cellHeight : FLT_MAX;

      // skip floors below startPos
      if (agentY + allowedSpace > ymax)
      {
        prevFloor = ymax;
        span = span ? span->next : nullptr; //-V547
        continue;
      }
      // stop on ceiling above agent
      if (ymin != FLT_MAX && agentY + agentHeight < ymin)
        break;

      if (isFalling)
      {
        // obstruction on the way
        if (!(agentY + allowedSpace >= prevFloor && agentY + agentHeight < ymin))
        {
          foundObstruction = true;
          break;
        }
        // found floor to jump to
        if (ymin > prevAgentY + agentHeight && prevFloor > agentY - allowedSpace && prevFloor < prevAgentY + allowedSpace)
        {
          fall_pos = Point3::xVz(pt, prevFloor);
          res = start_pos.y - prevFloor > jlk_params.jumpoffMinHeight;
          foundObstruction = true;
          break;
        }
      }

      if (!isFalling)
      {
        // obstruction on the way to edge
        if (!(agentY + allowedSpace >= prevFloor && agentY + agentHeight < ymin))
          foundObstruction = true;

        // found cliff, start falling
        else if (prevFloor < prevCellFloor - cellHeight)
          isFalling = true;

        prevCellFloor = prevFloor;
        break;
      }

      if (!span)
        break;

      prevFloor = ymax;
      span = span->next;
    }
    prevAgentY = agentY;
  }

  return res;
}

static bool find_jumpoff_point(const rcHeightfield *solid, const dtNavMeshQuery *nav_query, const JumpLinksParams &jlk_params,
  const Point3 &start_pos, const Point3 &jump_dir, Point3 &fall_pos)
{
  Point3 pos = start_pos;
  const bool isStartPointOnNavmesh = find_nearest_navmesh_point(nav_query, pos, jlk_params.agentRadius, solid->ch * 2);
  if (!isStartPointOnNavmesh)
    return false;

  const bool canJump = check_jumpoff_fall(solid, jlk_params, pos, jump_dir, fall_pos);
  if (!canJump)
    return false;

  pos = fall_pos;
  bool isEndPointOnNavmesh = find_nearest_navmesh_point(nav_query, pos, jlk_params.agentRadius, solid->ch * 2);
  if (!isEndPointOnNavmesh)
    return false;

  return true;
}

static const float MAX_JUMPOFF_LINK_SPREAD_ANGLE = DegToRad(70.0f);
static void sample_edge_to_navmesh_points(const Edge edge, const rcHeightfield *solid, const dtNavMeshQuery *nav_query,
  const JumpLinksParams &jlk_params, Tab<Jumplink> &out_jumplinks)
{
  int samples = (int)ceilf(length(Point3(edge.sp.x - edge.sq.x, 0.f, edge.sp.z - edge.sq.z)) / solid->cs);
  const Point3 edgeDir = edge.sq - edge.sp;
  const Point3 jumpDir = normalize(Point3{edgeDir.z, 0.0f, -edgeDir.x});
  const float edgeLength = edgeDir.length();

  float u = 0.0f;
  float prevU = 0.0f;
  float linkStartU = 0.0f;
  float currentLinkLength = 0.0f;
  Edge fromEdge{}, toEdge{};
  fromEdge.sp = edge.sp;

  for (int i = 0; i < samples; i++)
  {
    prevU = u;
    u = (float)i / (float)(samples - 1);
    const Point3 startPoint = lerp(edge.sp, edge.sq, u);
    currentLinkLength = (prevU - linkStartU) * edgeLength;

    // 1. check jumpoff trajectory slices
    Point3 jumpTo = Point3::ZERO;
    const bool canJump = find_jumpoff_point(solid, nav_query, jlk_params, startPoint, jumpDir, jumpTo);
    if (!canJump)
    {
      if (toEdge.sp != Point3::ZERO && currentLinkLength > jlk_params.jumpoffMinLinkLength)
      {
        const Edge fromEdge{lerp(edge.sp, edge.sq, linkStartU), lerp(edge.sp, edge.sq, prevU)};
        out_jumplinks.push_back(Jumplink{fromEdge, toEdge});
      }
      linkStartU = (float)(i + 1) / (float)(samples - 1);
      continue;
    }

    // 2. check and build link intervals

    // new link interval init
    if (u == linkStartU)
    {
      toEdge.sp = toEdge.sq = jumpTo;
      continue;
    }

    // check for link continuity
    const float dotProduct = dot(normalize(toEdge.sq - toEdge.sp), normalize(jumpTo - toEdge.sq));
    if (lengthSq(toEdge.sq - toEdge.sp) > 0.0f && dotProduct < cosf(MAX_JUMPOFF_LINK_SPREAD_ANGLE))
    {
      if (toEdge.sp != Point3::ZERO && currentLinkLength > jlk_params.jumpoffMinLinkLength)
      {
        const Edge fromEdge{lerp(edge.sp, edge.sq, linkStartU), lerp(edge.sp, edge.sq, prevU)};
        out_jumplinks.push_back(Jumplink{fromEdge, toEdge});
      }
      toEdge.sp = toEdge.sq = jumpTo;
      linkStartU = u;
      continue;
    }

    toEdge.sq = jumpTo;
  }

  currentLinkLength = (u - linkStartU) * edgeLength;
  if (toEdge.sp != Point3::ZERO && currentLinkLength > jlk_params.jumpoffMinLinkLength)
  {
    const Edge fromEdge{lerp(edge.sp, edge.sq, linkStartU), lerp(edge.sp, edge.sq, u)};
    out_jumplinks.push_back(Jumplink{fromEdge, toEdge});
  }
}

static void build_edge_to_poly_links(const Tab<Edge> &edges, const rcHeightfield *solid, const dtNavMeshQuery *nav_query,
  const JumpLinksParams &jlk_params, Tab<Jumplink> &jumplinks)
{
  for (int i = 0; i < edges.size(); ++i)
  {
    const Edge edge = edges[i];
    sample_edge_to_navmesh_points(edge, solid, nav_query, jlk_params, jumplinks);
  }
}

bool check_jump_between_points(const dtNavMeshQuery *nav_query, const rcHeightfield *solid, const rcCompactHeightfield *chf,
  const JumpLinksParams &jlk_params, Point3 &start_point, Point3 &end_point, float &height)
{
  Point3 pos = start_point;
  if (!find_nearest_navmesh_point(nav_query, pos, jlk_params.agentRadius, chf->ch * 2))
    return false;

  pos = end_point;
  if (!find_nearest_navmesh_point(nav_query, pos, jlk_params.agentRadius, chf->ch * 2))
    return false;

  Point3 dirXZ = start_point - end_point;
  dirXZ.y = 0;
  bool isEnoughDistance = lengthSq(dirXZ) < sqr(jlk_params.jumpLength);
  if (!isEnoughDistance)
    return false;

  bool isStartOnGround = recastcoll::get_compact_heightfield_height(chf, start_point, 0.01f, chf->ch * 4, start_point.y);
  if (!isStartOnGround)
    return false;

  bool isEndOnGround = recastcoll::get_compact_heightfield_height(chf, end_point, 0.01f, chf->ch * 4, end_point.y);
  if (!isEndOnGround)
    return false;

  return check_jump_max_height(solid, start_point, end_point, jlk_params.agentHeight, jlk_params.agentMinSpace, height);
}

static void sample_mapping_areas(const Edge edge, const Tab<Edge> &mapped_edges, const rcHeightfield *solid,
  const rcCompactHeightfield *chf, dtNavMeshQuery *nav_query, const JumpLinksParams &jlk_params, Tab<Jumplink> &out_jumplinks,
  bool &found_obstacles)
{
  found_obstacles = false;
  Point2 samplingDir = Point2(edge.sq.x - edge.sp.x, edge.sq.z - edge.sp.z);
  int samples = (int)ceilf(length(samplingDir) / solid->cs);
  float relativeAgentRadius = jlk_params.agentRadius / samplingDir.length();
  samplingDir.normalize();
  const Point3 edgeDir = edge.sq - edge.sp;
  for (int i = 0; i < mapped_edges.size(); ++i)
  {
    const Edge mappedEdge = mapped_edges[i];
    const Point3 mappedEdgeDir = mappedEdge.sq - mappedEdge.sp;
    float startValidU = 0.0f;
    float lastValidU = 0.0f;
    for (int i = 0; i < samples; i++)
    {
      const float u = (float)i / (float)(samples - 1);
      Point3 startPoint = lerp(edge.sp, edge.sq, u);
      Point3 endPoint = lerp(mappedEdge.sp, mappedEdge.sq, u);
      Point3 dir = endPoint - startPoint;
      dir.y = 0;
      dir.normalize();
      startPoint -= dir * jlk_params.agentRadius;
      endPoint += dir * jlk_params.agentRadius;

      float height = 0.0f;
      const bool canJump = check_jump_between_points(nav_query, solid, chf, jlk_params, startPoint, endPoint, height);

      if (!canJump)
      {
        found_obstacles = true;
        if (lastValidU - startValidU > relativeAgentRadius)
        {
          const Edge fromEdge{edge.sp + edgeDir * startValidU, edge.sp + edgeDir * lastValidU};
          const Edge toEdge{mappedEdge.sp + mappedEdgeDir * startValidU, mappedEdge.sp + mappedEdgeDir * lastValidU};
          out_jumplinks.push_back(Jumplink{fromEdge, toEdge});
        }
        i += ceil(jlk_params.agentRadius / solid->cs);
        if (i >= samples)
          return;
        startValidU = lastValidU = (float)(i + 1) / (float)(samples - 1);
        continue;
      }

      lastValidU = u;
    }
    if (1.0f - startValidU > relativeAgentRadius)
    {
      const Edge fromEdge{edge.sp + edgeDir * startValidU, edge.sq};
      const Edge toEdge{mappedEdge.sp + mappedEdgeDir * startValidU, mappedEdge.sq};
      out_jumplinks.push_back(Jumplink{fromEdge, toEdge});
    }
  }
}

static const int MAX_MAPPING_CYCLES = 15;
static void build_edge_to_edge_links(const Tab<Edge> &edges, const rcHeightfield *solid, const rcCompactHeightfield *chf,
  dtNavMeshQuery *nav_query, const JumpLinksParams &jlk_params, Tab<Jumplink> &jumplinks)
{
  for (int i = 0; i < edges.size(); ++i)
  {
    const Edge edge1 = edges[i];
    for (int j = i + 1; j < edges.size(); ++j)
    {
      const Edge edge2 = edges[j];

      // skip connected edges
      if (edge1.sq == edge2.sp)
      {
        continue;
      }

      if (dot(edge1.sp - edge1.sq, edge2.sp - edge2.sq) >= 0)
      {
        continue;
      }

      float minHeightBetweenEdges = min(min(abs(edge1.sq.y - edge2.sp.y), abs(edge1.sq.y - edge2.sq.y)),
        min(abs(edge1.sp.y - edge2.sp.y), abs(edge1.sp.y - edge2.sq.y)));
      if (minHeightBetweenEdges > jlk_params.jumpHeight)
      {
        continue;
      }

      // clip and filter edges
      const Point3 edge1Dir = edge1.sq - edge1.sp;
      const Point3 edge2Dir = edge2.sq - edge2.sp;
      const Point3 edge1NearPlaneNormal = normalize(Point3{edge1Dir.z, 0.0f, -edge1Dir.x});
      const Point3 edge1FarPlaneNormal = -edge1NearPlaneNormal;
      const Point3 edge2NearPlaneNormal = normalize(Point3{edge2Dir.z, 0.0f, -edge2Dir.x});
      const Point3 edge2FarPlaneNormal = -edge2NearPlaneNormal;

      const Point3 nearPlaneOffset1 = edge1NearPlaneNormal * jlk_params.agentRadius;
      const Point3 nearPlaneOffset2 = edge2NearPlaneNormal * jlk_params.agentRadius;
      EdgeClipResult clipResult1, clipResult2;
      const Point3 edge1NearPlaneClippedPoint =
        clip_edge_by_plane(edge1, edge2.sp + nearPlaneOffset2, edge2NearPlaneNormal, clipResult1);
      const Point3 edge2NearPlaneClippedPoint =
        clip_edge_by_plane(edge2, edge1.sp + nearPlaneOffset1, edge1NearPlaneNormal, clipResult2);
      if (clipResult1 == EDGE_CLIP_RESULT_OUTSIDE || clipResult2 == EDGE_CLIP_RESULT_OUTSIDE)
      {
        continue;
      }

      Edge clippedEdge1 = edge1, clippedEdge2 = edge2;
      if (edge1NearPlaneClippedPoint != edge1.sp)
      {
        clippedEdge1 = dot(edge1NearPlaneClippedPoint - clippedEdge1.sp, edge2NearPlaneNormal) > 0
                         ? Edge{edge1NearPlaneClippedPoint, clippedEdge1.sq}
                         : Edge{clippedEdge1.sp, edge1NearPlaneClippedPoint};
      }
      if (edge2NearPlaneClippedPoint != edge2.sp)
      {
        clippedEdge2 = dot(edge2NearPlaneClippedPoint - clippedEdge2.sp, edge1NearPlaneNormal) > 0
                         ? Edge{edge2NearPlaneClippedPoint, clippedEdge2.sq}
                         : Edge{clippedEdge2.sp, edge2NearPlaneClippedPoint};
      }

      const Point3 farPlaneOffset1 = edge1NearPlaneNormal * jlk_params.jumpLength;
      const Point3 farPlaneOffset2 = edge2NearPlaneNormal * jlk_params.jumpLength;
      Point3 edge1FarPlaneClippedPoint =
        clip_edge_by_plane(clippedEdge1, edge2.sp + farPlaneOffset2, edge2FarPlaneNormal, clipResult1);
      Point3 edge2FarPlaneClippedPoint =
        clip_edge_by_plane(clippedEdge2, edge1.sp + farPlaneOffset1, edge1FarPlaneNormal, clipResult2);
      if (clipResult1 == EDGE_CLIP_RESULT_OUTSIDE || clipResult2 == EDGE_CLIP_RESULT_OUTSIDE)
      {
        continue;
      }
      if (edge1FarPlaneClippedPoint != clippedEdge1.sp)
      {
        clippedEdge1 = dot(edge1FarPlaneClippedPoint - edge1.sp, edge2FarPlaneNormal) > 0
                         ? Edge{edge1FarPlaneClippedPoint, clippedEdge1.sq}
                         : Edge{clippedEdge1.sp, edge1FarPlaneClippedPoint};
      }
      if (edge2FarPlaneClippedPoint != clippedEdge2.sp)
      {
        clippedEdge2 = dot(edge2FarPlaneClippedPoint - clippedEdge2.sp, edge1FarPlaneNormal) > 0
                         ? Edge{edge2FarPlaneClippedPoint, clippedEdge2.sq}
                         : Edge{clippedEdge2.sp, edge2FarPlaneClippedPoint};
      }

      Edge proccessedEdge1, proccessedEdge2;
      const float edgeMappingAngle = 90.0f;
      if (!clip_edge_by_mapping_angle(clippedEdge2, edgeMappingAngle, clippedEdge1, proccessedEdge1))
      {
        continue;
      }
      if (!clip_edge_by_mapping_angle(clippedEdge1, edgeMappingAngle, clippedEdge2, proccessedEdge2))
      {
        continue;
      }

      if (lengthSq(proccessedEdge1.sp - proccessedEdge1.sq) < sqr(jlk_params.agentRadius * 2))
      {
        continue;
      }

      // swap so start and end points are opposite each other
      const Point3 tempPt = proccessedEdge2.sp;
      proccessedEdge2.sp = proccessedEdge2.sq;
      proccessedEdge2.sq = tempPt;

      // find smaller mapping edge
      const Edge mappingEdge = Edge{proccessedEdge1.sp, proccessedEdge2.sp};
      const float edge1ToMappingDist = distance_pt_to_edge(proccessedEdge1.sq, mappingEdge);
      const float edge2ToMappingDist = distance_pt_to_edge(proccessedEdge2.sq, mappingEdge);
      const Edge &smallerMappingEdge = edge1ToMappingDist < edge2ToMappingDist ? proccessedEdge1 : proccessedEdge2;
      const Edge &largerMappingEdge = edge1ToMappingDist < edge2ToMappingDist ? proccessedEdge2 : proccessedEdge1;

      // mapping areas
      Tab<Edge> mappedEdges;
      Point3 lastLeftMappedPoint = largerMappingEdge.sp;
      Point3 lastRightMappedPoint = largerMappingEdge.sq;
      for (int i = 0;; i++)
      {
        if (i > MAX_MAPPING_CYCLES)
        {
          break;
        }
        const Edge leftMappingConnection = {smallerMappingEdge.sp, lastLeftMappedPoint};
        Point3 mappingDir = leftMappingConnection.sq - leftMappingConnection.sp;
        Point3 planeNormal = {mappingDir.z, 0, -mappingDir.x};
        EdgeClipResult clipResult;
        Point3 mappedPoint = clip_edge_by_plane(largerMappingEdge, smallerMappingEdge.sq, planeNormal, clipResult);
        if (clipResult == EDGE_CLIP_RESULT_INSIDE)
          mappedPoint = largerMappingEdge.sq;

        Edge mappedEdge = {lastLeftMappedPoint, mappedPoint};
        mappedEdges.push_back(mappedEdge);
        lastLeftMappedPoint = mappedPoint;

        if (i == 0 && (mappedEdge.sq - mappedEdge.sp).lengthSq() > (largerMappingEdge.sq - largerMappingEdge.sp).lengthSq() * 0.95f)
          break;

        if ((largerMappingEdge.sp - lastLeftMappedPoint).lengthSq() > (largerMappingEdge.sp - lastRightMappedPoint).lengthSq())
          break;

        const Edge rightMappingConnection = {smallerMappingEdge.sq, lastRightMappedPoint};
        mappingDir = rightMappingConnection.sq - rightMappingConnection.sp;
        planeNormal = {-mappingDir.z, 0, mappingDir.x};
        mappedPoint = clip_edge_by_plane(largerMappingEdge, smallerMappingEdge.sp, planeNormal, clipResult);
        if (clipResult == EDGE_CLIP_RESULT_INSIDE)
          mappedPoint = largerMappingEdge.sq;

        mappedEdge = {mappedPoint, lastRightMappedPoint};
        mappedEdges.push_back(mappedEdge);
        lastRightMappedPoint = mappedPoint;

        if ((largerMappingEdge.sp - lastLeftMappedPoint).lengthSq() > (largerMappingEdge.sp - lastRightMappedPoint).lengthSq())
          break;
      }

      // sample mapping areas
      bool foundObstacles = false;
      Tab<Jumplink> sampledJumplinks(tmpmem);
      sampledJumplinks.reserve(5);
      sample_mapping_areas(smallerMappingEdge, mappedEdges, solid, chf, nav_query, jlk_params, sampledJumplinks, foundObstacles);

      if (sampledJumplinks.size() == 0)
      {
        continue;
      }

      // merge or split jumplinks, based on sampled mapping areas
      if (!foundObstacles)
      {
        jumplinks.push_back(Jumplink{smallerMappingEdge, largerMappingEdge});
      }
      else
      {
        for (int i = 0; i < sampledJumplinks.size(); i++)
        {
          jumplinks.push_back(sampledJumplinks[i]);
        }
      }
    }
  }
}

static void split_jumplink_by_obstruction_heights(Jumplink &link, const rcHeightfield *solid, const JumpLinksParams &jlk_params,
  Tab<Jumplink> &ret)
{
  const bool isStartEdgeSmaller = lengthSq(link.start.sq - link.start.sp) < lengthSq(link.end.sq - link.end.sp);
  const Edge smallerEdge = isStartEdgeSmaller ? link.start : link.end;
  const Edge largerEdge = isStartEdgeSmaller ? link.end : link.start;
  const Edge middleLine = Edge{lerp(smallerEdge.sp, largerEdge.sp, 0.5f), lerp(smallerEdge.sq, largerEdge.sq, 0.5f)};
  int samples = (int)ceilf(length(middleLine.sq - middleLine.sp) / solid->cs);
  LineSampler lineSampler = LineSampler(middleLine, samples);
  for (int j = 0; j < samples; j++)
  {
    const float u = (float)j / (float)(samples - 1);
    const Point3 start = lerp(smallerEdge.sp, smallerEdge.sq, u);
    const Point3 end = lerp(largerEdge.sp, largerEdge.sq, u);
    float height = recastcoll::get_line_max_height(solid, start, end, jlk_params.agentMinSpace);
    lineSampler.points[j].height = height;
    lineSampler.points[j].AddBit(HEIGHT_CHECKED);
  }
  const int minWidthCells = 4;
  Tab<Edge> links(tmpmem);
  links.reserve(5);
  lineSampler.findJumpLinkLines(minWidthCells, jlk_params.deltaHeightThreshold, links);
  for (const auto &edge : links)
  {
    if (jlk_params.maxObstructionAngle > 0)
    {
      const Point3 edgeDir = edge.sq - edge.sp;
      const float deltaHeight = abs(edgeDir.y);
      if (deltaHeight > 0)
      {
        const float edgeLength = length(edgeDir);
        if (safe_atan2(deltaHeight, edgeLength) > jlk_params.maxObstructionAngle)
        {
          continue;
        }
      }
    }
    Jumplink jlk;
    float middleLineXZLength = length(Point2::xz(middleLine.sq - middleLine.sp));
    float startU = safediv(length(Point2::xz(edge.sp - middleLine.sp)), middleLineXZLength);
    float endU = safediv(length(Point2::xz(edge.sq - middleLine.sp)), middleLineXZLength);
    jlk.start = Edge{lerp(smallerEdge.sp, smallerEdge.sq, startU), lerp(smallerEdge.sp, smallerEdge.sq, endU)};
    jlk.end = Edge{lerp(largerEdge.sp, largerEdge.sq, startU), lerp(largerEdge.sp, largerEdge.sq, endU)};
    jlk.obstructionLine = edge;
    ret.push_back(jlk);
  }
}

static bool is_point_in_tile(const Point3 pt, const BBox3 &tile_box)
{
  Point3 dir = pt - tile_box.center();
  Point3 tileBoxHalfSize = tile_box.width() * 0.5f;
  return abs(dot(dir, Point3(1, 0, 0))) <= tileBoxHalfSize.x && abs(dot(dir, Point3(0, 0, 1))) <= tileBoxHalfSize.z;
}

static void build_offmesh_connection(recastnavmesh::OffMeshConnectionsStorage &conn_storage, const Point3 &start, const Point3 &end,
  float radius, float jump_height, float obstruction_height, const BBox3 &tileBox, bool isComplex = false)
{
  const bool bidir = max(abs(start.y - obstruction_height), abs(end.y - obstruction_height)) < jump_height;
  Point3 jumplinkStart = start;
  Point3 jumplinkEnd = end;
  if ((!bidir && start.y < end.y) || (bidir && !is_point_in_tile(start, tileBox)))
  {
    jumplinkStart = end;
    jumplinkEnd = start;
  }

  if (!isComplex)
  {
    add_off_mesh_connection(conn_storage, &jumplinkStart.x, &jumplinkEnd.x, radius, bidir);
  }
  else
  {
    Point3 linkCenter = Point3::xVz((start + end) * 0.5, obstruction_height);
    add_off_mesh_connection(conn_storage, &jumplinkStart.x, &linkCenter.x, radius, bidir);
    add_off_mesh_connection(conn_storage, &linkCenter.x, &jumplinkEnd.x, radius, bidir);
  }
}

static void build_jumplink_connections(const Jumplink &link, const JumpLinksParams &jlk_params, const BBox3 &tile_box,
  recastnavmesh::OffMeshConnectionsStorage &conn_storage)
{
  const Edge smallerEdge = lengthSq(link.start.sq - link.start.sp) < lengthSq(link.end.sq - link.end.sp) ? link.start : link.end;
  const float len = length(smallerEdge.sq - smallerEdge.sp);
  const float ctf = len / jlk_params.width;
  int count = (int)round(ctf);
  count = count < 1 ? 1 : count;

  const float fullu = jlk_params.width / len;
  const float halfu = fullu * 0.5;
  const float startu = (ctf - float(count)) * halfu;
  const float addu = halfu + startu;
  const float edgeOffset = jlk_params.agentRadius;


  for (int i = 0; i < count; i++)
  {
    const float u = count == 1 ? 0.5f : i * fullu + addu;
    Point3 sp = lerp(link.start.sp, link.start.sq, u);
    Point3 sq = lerp(link.end.sp, link.end.sq, u);
    const Point3 dir = normalize(Point3(sq.x - sp.x, 0.0f, sq.z - sp.z));
    sp -= dir * edgeOffset;
    sq += dir * edgeOffset;
    float height = lerp(link.obstructionLine.sp.y, link.obstructionLine.sq.y, u);
    bool isComplex = jlk_params.complexJumpTheshold > 0 &&
                     (fabsf(sp.y - height) > jlk_params.complexJumpTheshold || fabsf(sq.y - height) > jlk_params.complexJumpTheshold);
    build_offmesh_connection(conn_storage, sp, sq, jlk_params.width * 0.5f, jlk_params.jumpHeight, height, tile_box, isComplex);
  }
}

void build_jumplinks_v2(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, dtNavMesh *navmesh, const Tab<Edge> &edges,
  const JumpLinksParams &jlk_params, const rcCompactHeightfield *chf, const rcHeightfield *solid, const BBox3 &tile_box)
{
  dtNavMeshQuery *navQuery = dtAllocNavMeshQuery();
  if (navQuery && dtStatusFailed(navQuery->init(navmesh, 4096)))
  {
    dtFreeNavMeshQuery(navQuery);
    navQuery = nullptr;
    logerr("Build Jumplinks: could not initialize navQuery");
    return;
  }

  Tab<Jumplink> jumplinks(tmpmem);
  jumplinks.reserve(edges.size());

  build_edge_to_poly_links(edges, solid, navQuery, jlk_params, jumplinks);

  build_edge_to_edge_links(edges, solid, chf, navQuery, jlk_params, jumplinks);

  if (jumplinks.size() == 0)
    return;

  Tab<Jumplink> mergedJumplinks = merge_all_jumplinks(jumplinks, jlk_params.edgeMergeAngle, jlk_params.edgeMergeDist);

  for (int i = 0; i < mergedJumplinks.size(); i++)
  {
    Jumplink link = mergedJumplinks[i];
    Tab<Jumplink> splittedLinks(tmpmem);
    splittedLinks.reserve(5);
    split_jumplink_by_obstruction_heights(link, solid, jlk_params, splittedLinks);
    for (const auto &splittedLink : splittedLinks)
      build_jumplink_connections(splittedLink, jlk_params, tile_box, out_conn_storage);
  }

  dtFreeNavMeshQuery(navQuery);
  navQuery = nullptr;
}
} // namespace recastbuild
