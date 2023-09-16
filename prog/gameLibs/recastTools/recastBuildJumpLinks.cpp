#include <recastTools/recastBuildJumpLinks.h>
#include <recastTools/recastNavMeshTile.h>
#include <recastTools/recastTools.h>

#include <math/dag_TMatrix.h>

#include "commonLineSampler.h"

namespace recastbuild
{
struct JumpLink
{
  bool exist = false;
  Edge start;
  Edge link;
  Edge end;
};

template <class T>
inline void swapVal(T &a, T &b)
{
  T t = a;
  a = b;
  b = t;
}

inline bool isectSegAABB(const Point3 &sp, const Point3 &sq, const Point3 &amin, const Point3 &amax, float &tmin, float &tmax)
{
  static const float EPS = 1e-6f;

  Point3 d = sq - sp;
  tmin = 0;       // set to -FLT_MAX to get first hit on line
  tmax = FLT_MAX; // set to max distance ray can travel (for segment)

  // For all three slabs
  for (int i = 0; i < 3; i++)
  {
    if (fabsf(d[i]) < EPS)
    {
      // Ray is parallel to slab. No hit if origin not within slab
      if (sp[i] < amin[i] || sp[i] > amax[i])
        return false;
    }
    else
    {
      // Compute intersection t value of ray with near and far plane of slab
      const float ood = 1.0f / d[i];
      float t1 = (amin[i] - sp[i]) * ood;
      float t2 = (amax[i] - sp[i]) * ood;
      // Make t1 be intersection with near plane, t2 with far plane
      if (t1 > t2)
        swapVal(t1, t2);
      // Compute the intersection of slab intersections intervals
      if (t1 > tmin)
        tmin = t1;
      if (t2 < tmax)
        tmax = t2;
      // Exit with no collision as soon as slab intersection becomes empty
      if (tmin > tmax)
        return false;
    }
  }

  return true;
}

inline bool overlapRange(const float amin, const float amax, const float bmin, const float bmax)
{
  return (amin > bmax || amax < bmin) ? false : true;
}

inline float getClosestPtPtSeg(const Point3 pt, const Point3 sp, const Point3 sq)
{
  Point3 dir = sq - sp;
  Point3 diff = pt - sp;
  float t = dir * diff;
  if (t <= 0.0f)
    return 0.0f;
  const float d = dir * dir;
  if (t >= d)
    return 1.0f;
  return t / d;
}

struct PotentialSeg
{
  unsigned char mark;
  int idx;
  float umin, umax;
  float dmin, dmax;
  Point3 sp, sq;
};

int findPotentialJumpOverEdges(const rcCompactHeightfield *m_chf, const Edge &edge, const JumpLinksParams &params,
  const Tab<Edge> &m_edges, JumpLink *outSegs, const int maxOutSegs)
{
  const float widthRange = length(edge.sp - edge.sq);
  // if (widthRange < range.x)
  //  return;
  const Point3 amin = Point3(0, -params.jumpHeight * 0.5f, 0), amax = Point3(widthRange, params.jumpHeight * 0.5f, params.jumpLength);

  const float thr = cosf((180.0f - 45.0f) / 180.0f * PI);

  Point3 ax = edge.sq - edge.sp;
  ax.normalize();
  Point3 az = Point3(ax[2], 0, -ax[0]);
  az.normalize();
  Point3 ay = {0.f, 1.f, 0.f};

  TMatrix a = TMatrix::IDENT;
  a.setcol(0, ax);
  a.setcol(1, Point3(0, 1, 0));
  a.setcol(2, az);
  a.setcol(3, Point3(0.f, 0.f, 0.f));

  static const int MAX_SEGS = 64;
  PotentialSeg segs[MAX_SEGS];
  int nsegs = 0;

  const int nedges = (int)m_edges.size();
  for (int i = 0; i < nedges; ++i)
  {
    Point3 lsp = a * (m_edges[i].sp - edge.sp);
    Point3 lsq = a * (m_edges[i].sq - edge.sp);

    float tmin, tmax;
    if (isectSegAABB(lsp, lsq, amin, amax, tmin, tmax))
    {
      if (tmin > 1.0f)
        continue;
      if (tmax < 0.0f)
        continue;

      Point3 edir = m_edges[i].sq - m_edges[i].sp;
      edir.y = 0;
      edir.normalize();

      if (ax * edir > thr)
        continue;

      if (nsegs < MAX_SEGS)
      {
        segs[nsegs].umin = clamp(tmin, 0.0f, 1.0f);
        segs[nsegs].umax = clamp(tmax, 0.0f, 1.0f);
        segs[nsegs].dmin = min(lsp.z, lsq.z);
        segs[nsegs].dmax = max(lsp.z, lsq.z);
        segs[nsegs].idx = i;
        segs[nsegs].mark = 0;
        nsegs++;
      }
    }
  }

  const float eps = m_chf->cs;
  unsigned char mark = 1;
  for (int i = 0; i < nsegs; ++i)
  {
    if (segs[i].mark != 0)
      continue;
    segs[i].mark = mark;

    for (int j = i + 1; j < nsegs; ++j)
      if (overlapRange(segs[i].dmin - eps, segs[i].dmax + eps, segs[j].dmin - eps, segs[j].dmax + eps))
        segs[j].mark = mark;

    mark++;
  }

  int nout = 0;
  for (int i = 1; i < mark; ++i)
  {
    // Find destination mid point.
    float umin = 10.0f, umax = -10.0;
    Point3 ptmin = ZERO<Point3>();
    Point3 ptmax = ZERO<Point3>();

    for (int j = 0; j < nsegs; ++j)
    {
      PotentialSeg &seg = segs[j];
      if (seg.mark != i)
        continue;

      Point3 pa = lerp(m_edges[seg.idx].sp, m_edges[seg.idx].sq, seg.umin);
      Point3 pb = lerp(m_edges[seg.idx].sp, m_edges[seg.idx].sq, seg.umax);

      const float ua = getClosestPtPtSeg(pa, edge.sp, edge.sq);
      const float ub = getClosestPtPtSeg(pb, edge.sp, edge.sq);

      if (ua < umin)
      {
        ptmin = pa;
        umin = ua;
      }
      if (ua > umax)
      {
        ptmax = pa;
        umax = ua;
      }
      if (ub < umin)
      {
        ptmin = pb;
        umin = ub;
      }
      if (ub > umax)
      {
        ptmax = pb;
        umax = ub;
      }
    }

    if (umin > umax)
      continue;
    Point3 end = lerp(ptmin, ptmax, 0.5f);
    Point3 start = lerp(edge.sp, edge.sq, (umin + umax) * 0.5f);
    Point3 orig = lerp(start, end, 0.5f);

    Point3 dir = end - start;
    dir.y = 0;
    dir.normalize();
    Point3 norm = Point3(dir[2], 0, -dir[0]);

    const float width = widthRange * (umax - umin);

    if (nout < maxOutSegs)
    {
      outSegs[nout].link.sp = orig + norm * (width * 0.5f);
      outSegs[nout].link.sq = orig + norm * (-width * 0.5f);
      outSegs[nout].end.sp = ptmin;
      outSegs[nout].end.sq = ptmax;
      outSegs[nout].start = edge;
      outSegs[nout].exist = true;
      nout++;
    }
  }

  return nout;
}

JumpLink sampleEdge(const rcCompactHeightfield *m_chf, const Edge &edge, const JumpLinksParams &params, const Tab<Edge> &m_edges)
{
  static const int NSEGS = 8;
  JumpLink segs[NSEGS];

  int nsegs = findPotentialJumpOverEdges(m_chf, edge, params, m_edges, segs, NSEGS);
  int ibest = -1;
  float dbest = 0;
  for (int i = 0; i < nsegs; ++i)
  {
    const float d = lengthSq(segs[i].link.sq - segs[i].link.sp);
    if (d > dbest)
    {
      dbest = d;
      ibest = i;
    }
  }
  if (ibest == -1)
    return JumpLink();

  float lenLink = dbest;
  float lenStart = lengthSq(segs[ibest].start.sq - segs[ibest].start.sp);
  float lenEnd = lengthSq(segs[ibest].end.sq - segs[ibest].end.sp);
  float minLen = 0.1 * 0.1;
  if (lenLink < minLen || lenStart < minLen || lenEnd < minLen)
    return JumpLink();
  return segs[ibest];
}

Point3 GetCrossPlanePoint(Point3 norm, Point3 planePt, Edge &edge)
{
  float d = norm * (planePt - edge.sp);
  Point3 W = edge.sq - edge.sp;
  float e = norm * W;

  if (e != 0)
    return edge.sp + W * (d / e);
  else if (d == 0)
    return planePt;
  else
    return Point3(0.f, 0.f, 0.f);
}

Edge cutEdgeFromEdge(Edge orig, Edge toCut)
{
  Point3 origNorm = orig.sp - orig.sq;
  origNorm.normalize();
  Point3 sp = GetCrossPlanePoint(origNorm, orig.sp, toCut);
  Point3 sq = GetCrossPlanePoint(origNorm, orig.sq, toCut);
  return Edge({sp, sq});
}

bool isLineNear(const Edge &ed1, const Edge &ed2, const JumpLinksParams &params)
{
  Point3 dir1 = ed1.sp - ed1.sq;
  dir1.normalize();
  Point3 dir2 = ed2.sp - ed2.sq;
  dir2.normalize();

  if (abs(dir1 * dir2) < params.linkDegAngle)
    return false;

  Point3 m1 = lerp(ed1.sp, ed1.sq, 0.5);
  Point3 m2 = lerp(ed2.sp, ed2.sq, 0.5);
  Point3 mmdir = m1 - m2;
  float mmlen = length(mmdir);
  mmdir *= safeinv(mmlen);

  float hl1 = length(ed1.sp - ed1.sq) * 0.5;
  float hl2 = length(ed2.sp - ed2.sq) * 0.5;

  if (mmlen > min(hl1, hl2))
  {
    float angle1 = abs(dir1 * mmdir);
    float angle2 = abs(dir2 * mmdir);
    float summ = lerp(angle1, angle2, 0.5);

    if (summ < params.linkDegDist)
      return false;
  }

  return mmlen * 0.9 < (hl1 + hl2);
}

bool isLineCrossed(Edge &ed1, Edge &ed2)
{
  Point3 m1 = lerp(ed1.sp, ed1.sq, 0.5);
  Point3 m2 = lerp(ed2.sp, ed2.sq, 0.5);
  Point3 mmdir = m1 - m2;
  float mmlen = length(mmdir);
  mmdir *= safeinv(mmlen);

  Point3 dir1 = ed1.sp - ed1.sq;
  float len1 = length(dir1);
  dir1 *= safeinv(len1);

  float len2 = length(ed2.sp - ed2.sq);
  if (max(len1 * 0.5, len2 * 0.5) < mmlen)
    return false;

  Point3 hv1 = (ed1.sp - ed1.sq) * 0.5;
  Point3 hv2 = (ed2.sp - ed2.sq) * 0.6; // 0.5 + small offset
  Point3 dirToMaxMid1 = hv1 + hv2;
  dirToMaxMid1.normalize();
  Point3 dirToMaxMid2 = hv1 - hv2;
  dirToMaxMid2.normalize();
  float maxAngle = min(abs(dirToMaxMid1 * dir1), abs(dirToMaxMid2 * dir1));

  float angle = abs(mmdir * dir1);
  return angle > maxAngle;
}

Edge addPointToLine(const Edge &ed, Point3 point)
{
  G_UNUSED(point);

  // float lineLen = length(ed.sp - ed.sq);
  // float gipLen = length(ed.sp - point);
  // float higth = getClosestPtPtSeg(point, ed.sp, ed.sq);
  // float cat = sqrt(gipLen * gipLen - higth * higth);
  // float mult = cat / lineLen;
  // Point3 sp = lerp(ed.sp, point, 1.f - mult);
  // Point3 sq = lerp(ed.sq, point, mult);
  // return Edge(sp, sq);
  return Edge({ed.sp, ed.sq});
}

Edge mergeLines(const Edge &ed1, const Edge &ed2)
{
  float distansesSq[6];
  distansesSq[0] = lengthSq(ed1.sp - ed2.sp);
  distansesSq[1] = lengthSq(ed1.sp - ed2.sq);
  distansesSq[2] = lengthSq(ed1.sq - ed2.sp);
  distansesSq[3] = lengthSq(ed1.sq - ed2.sq);
  distansesSq[4] = lengthSq(ed1.sp - ed1.sq);
  distansesSq[5] = lengthSq(ed2.sp - ed2.sq);
  int maxDistIdx = 0;
  for (int i = 0; i < 6; i++)
    if (distansesSq[i] > distansesSq[maxDistIdx])
      maxDistIdx = i;

  Edge ret;
  switch (maxDistIdx)
  {
    case 0:
      ret = Edge({ed1.sp, ed2.sp});
      ret = addPointToLine(ret, ed1.sq);
      return addPointToLine(ret, ed2.sq);
    case 1:
      ret = Edge({ed1.sp, ed2.sq});
      ret = addPointToLine(ret, ed1.sq);
      return addPointToLine(ret, ed2.sp);
    case 2:
      ret = Edge({ed1.sq, ed2.sp});
      ret = addPointToLine(ret, ed1.sp);
      return addPointToLine(ret, ed2.sq);
    case 3:
      ret = Edge({ed1.sq, ed2.sq});
      ret = addPointToLine(ret, ed1.sp);
      return addPointToLine(ret, ed2.sp);
    case 4:
      ret = Edge({ed1.sp, ed1.sq});
      ret = addPointToLine(ret, ed2.sp);
      return addPointToLine(ret, ed2.sq);
  }
  ret = Edge({ed2.sp, ed2.sq});
  ret = addPointToLine(ret, ed1.sp);
  return addPointToLine(ret, ed1.sq);
}

JumpLink mergeLink(const JumpLink &lk1, const JumpLink &lk2)
{
  JumpLink ret;
  ret.exist = true;
  ret.link = mergeLines(lk1.link, lk2.link);

  Point3 mst1 = lerp(lk1.start.sp, lk1.start.sq, 0.5);
  Point3 mst2 = lerp(lk2.start.sp, lk2.start.sq, 0.5);
  Point3 mend2 = lerp(lk2.end.sp, lk2.end.sq, 0.5);

  Point3 dirSt = lk1.start.sp - lk1.start.sq;
  Point3 dirToSt = mst1 - mst2;
  Point3 dirToEnd = mst1 - mend2;
  dirSt.normalize();
  dirToSt.normalize();
  dirToEnd.normalize();
  float angle1 = abs(dirSt * dirToSt);
  float angle2 = abs(dirSt * dirToEnd);

  if (angle1 > angle2)
  {
    ret.start = mergeLines(lk1.start, lk2.start);
    ret.end = mergeLines(lk1.end, lk2.end);
  }
  else
  {
    ret.start = mergeLines(lk1.start, lk2.end);
    ret.end = mergeLines(lk1.end, lk2.start);
  }
  return ret;
}

void mergerCommonJumpLink(Tab<JumpLink> &outSegs, const JumpLinksParams &params)
{
  for (int i = 0; i < outSegs.size(); i++)
  {
    if (outSegs[i].exist)
    {
      outSegs[i].start = cutEdgeFromEdge(outSegs[i].link, outSegs[i].start);
      outSegs[i].end = cutEdgeFromEdge(outSegs[i].link, outSegs[i].end);
    }
  }

  for (int i = 0; i < outSegs.size(); i++)
  {
    if (outSegs[i].exist)
    {
      for (int j = 0; j < outSegs.size(); j++)
      {
        if (outSegs[j].exist && j != i && isLineNear(outSegs[i].link, outSegs[j].link, params))
        {
          outSegs[i] = mergeLink(outSegs[i], outSegs[j]);
          outSegs[j].exist = false;
        }
      }
    }
  }

  // clear non merged short links
  const float maxLenSq = 0.5f * 0.5f;
  for (int i = 0; i < outSegs.size(); i++)
    if (lengthSq(outSegs[i].link.sq - outSegs[i].link.sp) < maxLenSq)
      outSegs[i].exist = false;

  // clear cross links
  for (int i = 0; i < outSegs.size(); i++)
  {
    if (outSegs[i].exist)
    {
      for (int j = i + 1; j < outSegs.size(); j++)
      {
        if (outSegs[j].exist && isLineCrossed(outSegs[i].link, outSegs[j].link))
        {
          if (lengthSq(outSegs[i].link.sp - outSegs[i].link.sq) > lengthSq(outSegs[j].link.sp - outSegs[j].link.sq))
          {
            outSegs[j].exist = false;
          }
          else
          {
            outSegs[i].exist = false;
          }
        }
      }
    }
  }

  for (int i = 0; i < outSegs.size(); i++)
  {
    if (outSegs[i].exist)
    {
      outSegs[i].start = cutEdgeFromEdge(outSegs[i].link, outSegs[i].start);
      outSegs[i].end = cutEdgeFromEdge(outSegs[i].link, outSegs[i].end);
    }
  }
}

struct EdgeSampler
{
  LineSampler start;
  LineSampler end;
  LineSampler link;

  EdgeSampler(const rcCompactHeightfield *m_chf, const Edge &link_in, const Edge &start_in, const Edge &end_in,
    const float ground_range)
  {
    int nsamples = (int)ceilf(length(link_in.sp - link_in.sq) / m_chf->cs);
    link = LineSampler(link_in, nsamples);
    start = LineSampler(start_in, nsamples);
    end = LineSampler(end_in, nsamples);

    const float range = m_chf->cs * 2.f;
    start.checkGround(m_chf, range, ground_range);
    end.checkGround(m_chf, range, ground_range);

    for (int i = 0; i < nsamples; ++i)
    {
      link.points[i].ClearBitState();
      if (start.points[i].haveBit(NAVMESH_CHECKED) && end.points[i].haveBit(NAVMESH_CHECKED))
        link.points[i].AddBit(NAVMESH_CHECKED);
    }
  }


  void sampleTrace(const rcHeightfield *m_solid, const float jumpHeight, const float agentRadius)
  {
    for (int i = 0; i < (int)link.points.size(); ++i)
    {
      if (!link.points[i].haveBit(NAVMESH_CHECKED))
      {
        link.points[i].height = lerp(start.points[i].height, end.points[i].height, 0.5f);
        continue;
      }

      const float u = (float)i / (float)((int)link.points.size() - 1);
      Point3 spt = lerp(start.edge.sp, start.edge.sq, u);
      Point3 ept = lerp(end.edge.sp, end.edge.sq, u);

      spt.y = start.points[i].height;
      ept.y = end.points[i].height;

      Point3 traceDir = ept - spt;
      traceDir.y = 0;
      traceDir.normalize();

      Point3 agentOffset = traceDir * agentRadius;
      Point3 agentSpt = spt - agentOffset;
      Point3 agentEpt = ept + agentOffset;

      float height = recastcoll::get_line_max_height(m_solid, agentSpt, agentEpt, 1.f);
      float maxHeight = min(start.points[i].height, end.points[i].height) + jumpHeight;

      if (height < maxHeight && height > min(spt.y, ept.y) - m_solid->ch)
      {
        link.points[i].height = height;
        link.points[i].AddBit(HEIGHT_CHECKED);
      }
      else
        link.points[i].height = lerp(start.points[i].height, end.points[i].height, 0.5f);
    }
  }

  void buildLinkLines(int minWeight, float maxDelta, Tab<JumpLink> &ret)
  {
    Tab<Edge> links(tmpmem);
    links.reserve(5);
    link.findJumpLinkLines(minWeight, maxDelta, links);

    for (const auto &edge : links)
    {
      JumpLink jlk;
      jlk.link = edge;
      jlk.start = cutEdgeFromEdge(jlk.link, start.edge);
      jlk.end = cutEdgeFromEdge(jlk.link, end.edge);
      ret.push_back(jlk);
    }
  }
};

static void add_off_mesh_connection(recastnavmesh::OffMeshConnectionsStorage &storage, const float *spos, const float *epos, float rad,
  unsigned char bidir, unsigned short flags = pathfinder::POLYFLAG_JUMP, unsigned short area = pathfinder::POLYAREA_JUMP)
{
  int &count = storage.m_offMeshConCount;
  if (count >= MAX_OFFMESH_CONNECTIONS)
  {
    logerr("m_offMeshConCount %d >= MAX_OFFMESH_CONNECTIONS %d", count, MAX_OFFMESH_CONNECTIONS);
    return;
  }

  float *v = &storage.m_offMeshConVerts[count * 3 * 2];
  storage.m_offMeshConRads[count] = rad;
  storage.m_offMeshConDirs[count] = bidir;
  storage.m_offMeshConAreas[count] = area;
  storage.m_offMeshConFlags[count] = flags;
  storage.m_offMeshConId[count] = 1000 + count;
  v[0] = spos[0];
  v[1] = spos[1];
  v[2] = spos[2];

  v[3] = epos[0];
  v[4] = epos[1];
  v[5] = epos[2];
  storage.m_offMeshConCount++;
}

static bool remove_off_mesh_connection(recastnavmesh::OffMeshConnectionsStorage &storage, int idx)
{
  if (storage.m_offMeshConCount <= idx)
  {
    logerr("can't remove offmesh connection idx %d from storage. Storage count = %d", idx, storage.m_offMeshConCount);
    return false;
  }

  storage.m_offMeshConCount--;
  if (storage.m_offMeshConCount == idx)
    return true; // Job done

  float *vTo = &storage.m_offMeshConVerts[idx * 3 * 2];
  float *vFrom = &storage.m_offMeshConVerts[storage.m_offMeshConCount * 3 * 2];

  storage.m_offMeshConRads[idx] = storage.m_offMeshConRads[storage.m_offMeshConCount];
  storage.m_offMeshConDirs[idx] = storage.m_offMeshConDirs[storage.m_offMeshConCount];
  storage.m_offMeshConAreas[idx] = storage.m_offMeshConAreas[storage.m_offMeshConCount];
  storage.m_offMeshConFlags[idx] = storage.m_offMeshConFlags[storage.m_offMeshConCount];
  storage.m_offMeshConId[idx] = storage.m_offMeshConId[storage.m_offMeshConCount];
  vTo[0] = vFrom[0];
  vTo[1] = vFrom[1];
  vTo[2] = vFrom[2];

  vTo[3] = vFrom[3];
  vTo[4] = vFrom[4];
  vTo[5] = vFrom[5];

  return true;
}

void buildConnectionsOnLink(const JumpLink &link, float weigth, float complex_jump_theshold,
  recastnavmesh::OffMeshConnectionsStorage &conn_storage)
{
  float len = length(link.link.sq - link.link.sp);
  float ctf = len / weigth;
  int count = (int)round(ctf);
  if (!count)
    return;

  float fullu = weigth / len;
  float halfu = fullu * 0.5;
  float startu = (ctf - float(count)) * halfu;
  float addu = halfu + startu;

  if (complex_jump_theshold <= 0.0f)
  {
    for (int i = 0; i < count; i++)
    {
      float u = i * fullu + addu;
      Point3 sp = lerp(link.start.sp, link.start.sq, u);
      Point3 sq = lerp(link.end.sp, link.end.sq, u);
      add_off_mesh_connection(conn_storage, &sp.x, &sq.x, weigth * 0.5, 1);
    }
    return;
  }

  for (int i = 0; i < count; i++)
  {
    float u = i * fullu + addu;
    Point3 sp = lerp(link.start.sp, link.start.sq, u);
    Point3 sq = lerp(link.end.sp, link.end.sq, u);
    //
    if (fabsf(sp.y - link.link.sp.y) > complex_jump_theshold || fabsf(sq.y - link.link.sq.y) > complex_jump_theshold)
    {
      const float top = max(link.link.sp.y, link.link.sq.y);
      // split link
      Point3 linkCenter = Point3::xVz((sp + sq) * 0.5, top);
      add_off_mesh_connection(conn_storage, &sp.x, &linkCenter.x, weigth * 0.5, 1);
      add_off_mesh_connection(conn_storage, &linkCenter.x, &sq.x, weigth * 0.5, 1);
    }
    else
    {
      add_off_mesh_connection(conn_storage, &sp.x, &sq.x, weigth * 0.5, 1);
    }
  }
}

void buildJumpLink(const rcHeightfield *m_solid, const rcCompactHeightfield *m_chf, const JumpLink &jampLink,
  const JumpLinksParams &params, recastnavmesh::OffMeshConnectionsStorage &conn_storage)
{
  if (!jampLink.exist)
    return;

  EdgeSampler es(m_chf, jampLink.link, jampLink.start, jampLink.end, params.width);
  es.sampleTrace(m_solid, params.jumpHeight * 0.5f, params.agentRadius);

  Tab<JumpLink> links(tmpmem);
  links.reserve(5);
  es.buildLinkLines(4, params.deltaHeightThreshold, links);

  for (const auto &link : links)
    buildConnectionsOnLink(link, params.width, params.complexJumpTheshold, conn_storage);
}
} // namespace recastbuild


static Point3 cast_endpoint_to_edge(const Point3 &p, const Tab<recastbuild::Edge> edges)
{
  if (edges.empty())
    return p;
  Point3 res = Point3::ZERO;
  float t, bestDstSq = eastl::numeric_limits<float>::max();
  for (const recastbuild::Edge &e : edges)
  {
    Point3 candidate = closest_pt_on_seg(p, e.sp, e.sq, t);
    if (t > .0 && t < 1.0)
    {
      auto dst = (p - candidate).lengthSq();
      if (dst < bestDstSq)
      {
        bestDstSq = dst;
        res = candidate;
      }
    }
  }
  res.y += 0.1f;
  return res;
}


void recastbuild::build_jumplinks_connstorage(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const JumpLinksParams &jlkParams, const rcCompactHeightfield *chf, const rcHeightfield *solid)
{
  if (!jlkParams.enabled)
    return;

  const int nedges = (int)edges.size();

  Tab<JumpLink> outSegs(tmpmem);
  outSegs.reserve(nedges);

  for (int i = 0; i < nedges; ++i)
  {
    JumpLink link = sampleEdge(chf, edges[i], jlkParams, edges);
    if (link.exist)
      outSegs.push_back(link);
  }

  mergerCommonJumpLink(outSegs, jlkParams);

  for (const auto &ed : outSegs)
    buildJumpLink(solid, chf, ed, jlkParams, out_conn_storage);
}

void recastbuild::cross_obstacles_with_jumplinks(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage, const Tab<Edge> &edges,
  const BBox3 &box, const JumpLinksParams &jlkParams, const Tab<JumpLinkObstacle> &obstacles)
{
  float serachRadius = 0.5;
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
      Point3 p1 = center + (widthVec * 0.5f + widthNorm * (jlkParams.agentRadius + serachRadius * 0.5f));
      Point3 p2 = center - (widthVec * 0.5f + widthNorm * (jlkParams.agentRadius + serachRadius * 0.5f));
      if (box & p1)
      {
        p1.y = cast_endpoint_to_edge(p1, edges).y;
        p2.y = cast_endpoint_to_edge(p2, edges).y;
        recastbuild::add_off_mesh_connection(out_conn_storage, &p1.x, &p2.x, serachRadius, 1, 0, pathfinder::POLYAREA_OBSTACLE);
      }
      else if (box & p2)
      {
        p1.y = cast_endpoint_to_edge(p1, edges).y;
        p2.y = cast_endpoint_to_edge(p2, edges).y;
        recastbuild::add_off_mesh_connection(out_conn_storage, &p2.x, &p1.x, serachRadius, 1, 0, pathfinder::POLYAREA_OBSTACLE);
      }
      else // Should be impossible
        logerr("Obstacle center is in tile, but none of the jumplink points are");
    }
  }
}

void recastbuild::disable_jumplinks_around_obstacle(recastnavmesh::OffMeshConnectionsStorage &out_conn_storage,
  const Tab<JumpLinkObstacle> &obstacles)
{
  for (auto &obstacle : obstacles)
  {
    Quat q = inverse(Quat(Point3(0, 1, 0), obstacle.yAngle));
    Point3 center = obstacle.box.center();
    for (int i = 0; i < out_conn_storage.m_offMeshConCount; i++)
    {
      float *v = &out_conn_storage.m_offMeshConVerts[i * 3 * 2];
      Point3 points[2] = {Point3(v[0], v[1], v[2]), Point3(v[3], v[4], v[5])};

      for (auto &p : points)
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
