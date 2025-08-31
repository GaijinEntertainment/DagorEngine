// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "hmlPlugin.h"

#include <EditorCore/ec_IEditorCore.h>
#include <math/dag_mathUtils.h>

#include <libTools/util/makeBindump.h>
#include <coolConsole/coolConsole.h>
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>

using editorcore_extapi::dagTools;

// Grid spline point.
struct SplineGridPoint
{
  uint16_t splineId;
  uint16_t pointId;

  SplineGridPoint(uint16_t spl, uint16_t pnt) : splineId(spl), pointId(pnt) {}
};

struct SplinesGrid
{
  uint16_t width;
  uint16_t height;
  Point2 offset;
  Point2 cellSize;
  Tab<SplineGridPoint> **grid;

public:
  SplinesGrid() : width(0), height(0), grid(NULL) {}

  ~SplinesGrid()
  {
    for (int i = 0; i != width * height; ++i)
      delete grid[i];

    delete[] grid;
  }

  void write(mkbindump::BinDumpSaveCB &cwr)
  {
    bool packed = dagor_target_code_store_packed(cwr.getTarget());
    cwr.beginTaggedBlock(packed ? (HmapLandPlugin::preferZstdPacking ? _MAKE4C('spgz') : _MAKE4C('spgZ')) : _MAKE4C('splg'));
    if (packed)
    {
      mkbindump::BinDumpSaveCB cwr_t(2 << 10, cwr);
      writeData(cwr_t);

      MemoryLoadCB mcrd(cwr_t.getMem(), false);
      if (HmapLandPlugin::preferZstdPacking)
        zstd_compress_data(cwr.getRawWriter(), mcrd, cwr_t.getSize());
      else
        lzma_compress_data(cwr.getRawWriter(), 9, mcrd, cwr_t.getSize());
    }
    else
      writeData(cwr);
    cwr.align8();
    cwr.endBlock();
  }
  void writeData(mkbindump::BinDumpSaveCB &cwr)
  {
    int recVal = 0;
    Point4 offsetMul(offset.x, offset.y, 1.f / cellSize.x, 1.f / cellSize.y);

    cwr.writeInt32e(width);
    cwr.writeInt32e(height);
    cwr.write32ex(&offsetMul, sizeof(Point4));

    for (uint16_t i = 0; i < width; ++i)
    {
      for (uint16_t j = 0; j < height; ++j)
      {
        cwr.writeInt32e(recVal);
        recVal += grid[j * width + i]->size();
      }
    }

    cwr.writeInt32e(recVal);
    for (uint16_t i = 0; i != width; ++i)
      for (uint16_t j = 0; j != height; ++j)
        for (uint16_t num = 0; num != grid[j * width + i]->size(); ++num)
          cwr.write16ex(&grid[j * width + i]->operator[](num), sizeof(SplineGridPoint));
  }

  void init(int w, int h, const Point2 &offs, const Point2 &cellSz)
  {
    width = w;
    height = h;
    offset = offs;
    cellSize = cellSz;

    grid = new Tab<SplineGridPoint> *[width * height];
    for (int i = 0; i != width * height; ++i)
      grid[i] = new Tab<SplineGridPoint>(tmpmem);
  }

  void addPoint(const Point3 &pos, uint16_t splineId, uint16_t pointId)
  {
    Point2 nrmPt = (Point2::xz(pos) - offset);
    int x = clamp(int(nrmPt.x / cellSize.x), 0, int(width - 1));
    int y = clamp(int(nrmPt.y / cellSize.y), 0, int(height - 1));
    grid[y * width + x]->push_back(SplineGridPoint(splineId, pointId));
  }
};

// Spline point for path (represents accurate position).
struct IntersectionSplinePoint
{
  uint16_t splineId;
  float pos;

  IntersectionSplinePoint(uint16_t splId, float p) : splineId(splId), pos(p) {}
};

struct IntersectionEdge
{
  uint16_t interId;
  uint8_t pointId;
  float distance;
  uint8_t fromPointId;
};

struct PathFinderInfo
{
  IntersectionEdge prevEdge;
  bool navigated;
  float distToDst;

  void reset()
  {
    prevEdge.distance = FLT_MAX;
    prevEdge.interId = 0xffff;
    prevEdge.pointId = 0xff;
    navigated = false;
  }
};

// Spline intersection point. Tab of accurate spline positions, and connections.
struct SplineIntersection
{
  Tab<IntersectionSplinePoint> points;
  Point2 position;
  Tab<IntersectionEdge> edges;
  PathFinderInfo pfInfo;

  SplineIntersection() : points(tmpmem), edges(tmpmem) {}

  void addEdge(uint16_t interId, uint8_t pointId, float dist, uint8_t from)
  {
    IntersectionEdge &edge = edges.push_back();
    edge.interId = interId;
    edge.pointId = pointId;
    edge.distance = dist;
    edge.fromPointId = from;
  }

  void resetPathFinder(const Point2 &dest)
  {
    pfInfo.reset();
    pfInfo.distToDst = (dest - position).length();
  }
};

// Represents exact point in intersection.
struct IntersectionNodePoint
{
  uint16_t nodeId;
  uint8_t nodePointId;

  IntersectionNodePoint() : nodeId(0xffff), nodePointId(0xff) {}
  IntersectionNodePoint(uint16_t nId, uint8_t nPt) : nodeId(nId), nodePointId(nPt) {}

  bool isValid() const { return nodeId != 0xffff && nodePointId != 0xff; }

  bool operator==(const IntersectionNodePoint &rhs) const { return rhs.nodeId == nodeId && rhs.nodePointId == nodePointId; }

  bool operator!=(const IntersectionNodePoint &rhs) const { return !(*this == rhs); }
};

// Point in intersection to which we should proceed and distance to this point.
struct IntersectionTableRecord
{
  IntersectionNodePoint nextPoint;
  float dist;
};

// Exact position and spline parametric position of this point.
struct SplineSegmentPoint
{
  Point3 in;
  Point3 pos;
  Point3 out;
  float splinePos;

  SplineSegmentPoint(SplinePointObject *p, float splPos) :
    in(p->getBezierIn()), pos(p->getPt()), out(p->getBezierOut()), splinePos(splPos)
  {}
};

// All spline segments, used to find intersections.
struct SplineSegments
{
  Tab<SplineSegmentPoint> points;
  bool haveIntersections;

  SplineSegments() : points(tmpmem), haveIntersections(false) {}
};

// IntersectionNodePoint and intersection which is represented by it.
struct SplineIntersectionNode
{
  IntersectionNodePoint point;
  SplineIntersection *inter = nullptr;
  SplineIntersectionNode() = default;

  SplineIntersectionNode(const IntersectionNodePoint &pt, SplineIntersection *intr) : point(pt), inter(intr) {}
};

struct SplineData
{
  String name;
  SplineSegments segments;
  Tab<SplineIntersectionNode> pathNodePoints;
  BBox2 box;

  SplineData() : pathNodePoints(tmpmem) {}
};


class SplineExporter
{
  Tab<SplineData> splineData;
  SplinesGrid grid;
  float distBetween;
  Tab<SplineIntersection> intersections;

  void fillSplineData(const Tab<SplineObject *> &splines);
  void findIntersections();
  void addNodePoint(const IntersectionNodePoint &pt);
  void makeConnections();

  bool findWay(uint16_t from, uint16_t to);

  void savePaths(mkbindump::BinDumpSaveCB &cwr);

public:
  SplineExporter() : splineData(tmpmem), distBetween(20.f), intersections(tmpmem) {}

  void exportSplines(mkbindump::BinDumpSaveCB &cwr, const Tab<SplineObject *> &splines, const HeightMapStorage &heightMap,
    float gridCellSize, const Point2 &heightMapOffset);
};


void SplineExporter::fillSplineData(const Tab<SplineObject *> &splines)
{
  if (splines.empty())
  {
    clear_and_shrink(splineData);
    return;
  }
  else
  {
    splineData.resize(splines.size());
    Point2 offset = Point2(1.f, 1.f) * distBetween;
    for (int i = 0; i < splines.size(); ++i)
    {
      SplineData &splData = splineData[i];
      SplineObject *o = splines[i];
      BBox3 box = o->getSplineBox();
      splData.name = o->getName();
      splData.box = BBox2(Point2::xz(box.lim[0]) - offset, Point2::xz(box.lim[1]) + offset);
      const BezierSpline3d &bezSpl = o->getBezierSpline();
      float len = 0.f;
      int pointId = 0;
      for (int j = 0; j < o->points.size(); ++j)
      {
        SplinePointObject *p = o->points[j];
        if (p)
        {
          splData.segments.points.push_back(SplineSegmentPoint(p, len));
          grid.addPoint(p->getPt(), uint16_t(i), uint16_t(pointId));
          pointId++;
        }
        if (j != o->points.size() - 1)
          len += bezSpl.segs[j].len;
      }
    }
  }
}

void SplineExporter::findIntersections()
{
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  con.addMessage(ILogWriter::REMARK, "[SPL] Generating intersections.");

  int startIntersections = dagTools->getTimeMsec();
  for (uint16_t i = 0; i < splineData.size(); ++i)
  {
    SplineData &splSrc = splineData[i];
    for (uint16_t j = i; j < splineData.size(); ++j) // start at i because we want self intersections as well
    {
      SplineData &splDst = splineData[j];
      if (!(splSrc.box & splDst.box))
        continue; // No intersections... NEXT
      for (int k = 0; k < (int)splSrc.segments.points.size() - 1; ++k)
      {
        const SplineSegmentPoint &pnt0 = splSrc.segments.points[k];
        const SplineSegmentPoint &pnt1 = splSrc.segments.points[k + 1];
        Point3 p0 = pnt0.pos;
        Point3 p1 = pnt1.pos;

        for (int l = 0; l < (int)splDst.segments.points.size() - 1; ++l)
        {
          const SplineSegmentPoint &pnt2 = splDst.segments.points[l];
          const SplineSegmentPoint &pnt3 = splDst.segments.points[l + 1];
          Point3 p2 = pnt2.pos;
          Point3 p3 = pnt3.pos;

          if (i == j && (pnt2.splinePos - pnt1.splinePos) < 2.f * distBetween)
            continue; // Skip all points on same spline, when they are too close to each other.

          float mua, mub;
          lineLineIntersect(p0, p1, p2, p3, mua, mub);
          mua = clamp(mua, 0.f, 1.f);
          mub = clamp(mub, 0.f, 1.f);
          Point3 p10 = p1 - p0;
          Point3 p32 = p3 - p2;
          Point3 pOnFirst = p0 + p10 * mua;
          Point3 pOnSecond = p2 + p32 * mub;

          float firstPos = pnt0.splinePos + (pnt1.splinePos - pnt0.splinePos) * mua;
          float secondPos = pnt2.splinePos + (pnt3.splinePos - pnt2.splinePos) * mub;

          Point2 ptMid = Point2::xz(pOnFirst + pOnSecond) * 0.5f;

          if (lengthSq(pOnFirst - pOnSecond) >= sqr(distBetween))
            continue;
          bool haveIntersections = false;
          for (int m = 0; m < intersections.size(); ++m)
          {
            Point2 pt = intersections[m].position;
            if (lengthSq(pt - ptMid) >= sqr(distBetween))
              continue;
            bool foundFirst = false;
            bool foundSecond = false;
            for (int n = 0; n < intersections[m].points.size(); ++n)
            {
              const IntersectionSplinePoint &splinePoint = intersections[m].points[n];
              if (splinePoint.splineId == i && rabs(splinePoint.pos - firstPos) < distBetween)
                foundFirst = true;
              if (splinePoint.splineId == j && rabs(splinePoint.pos - secondPos) < distBetween)
                foundSecond = true;
              if (foundFirst && foundSecond)
                break;
            }
            if (!foundFirst)
              intersections[m].points.push_back(IntersectionSplinePoint(i, firstPos));
            if (!foundSecond)
              intersections[m].points.push_back(IntersectionSplinePoint(j, secondPos));
            haveIntersections = true;
          }
          if (!haveIntersections)
          {
            SplineIntersection &intersection = intersections.push_back();
            intersection.points.push_back(IntersectionSplinePoint(i, firstPos));
            intersection.points.push_back(IntersectionSplinePoint(j, secondPos));
            intersection.position = ptMid;
          }
          splSrc.segments.haveIntersections = splDst.segments.haveIntersections = true;
        } // second spline segments
      } // first spline segments
    } // for second splines
  } // for first splines

  // First and last points on each spline which have intersections.
  for (int i = 0; i < splineData.size(); ++i)
  {
    const SplineData &splData = splineData[i];
    const SplineSegments &splSeg = splData.segments;
    if (!splSeg.haveIntersections || splSeg.points.empty())
      continue;
    bool firstPoint = false;
    bool lastPoint = false;
    float lastPos = splSeg.points.back().splinePos;
    for (int j = 0; j < intersections.size(); ++j)
    {
      SplineIntersection &intersection = intersections[j];
      for (int k = 0; k < intersection.points.size(); ++k)
      {
        if (intersection.points[k].splineId != i)
          continue;
        if (intersection.points[k].pos < distBetween)
        {
          firstPoint = true;
          intersection.points[k].pos = 0.f;
        }
        if (intersection.points[k].pos > lastPos - distBetween)
        {
          lastPoint = true;
          intersection.points[k].pos = lastPos;
        }
        if (firstPoint && lastPoint)
          break;
      }
      if (firstPoint && lastPoint)
        break;
    }

    if (!firstPoint)
    {
      SplineIntersection &intersection = intersections.push_back();
      intersection.points.push_back(IntersectionSplinePoint(i, 0.f));
      intersection.position = Point2::xz(splSeg.points[0].pos);
    }
    if (!lastPoint)
    {
      SplineIntersection &intersection = intersections.push_back();
      intersection.points.push_back(IntersectionSplinePoint(i, lastPos));
      intersection.position = Point2::xz(splSeg.points.back().pos);
    }
  }

  // For each spline fill list of intersections on it.
  for (int i = 0; i < intersections.size(); ++i)
  {
    const SplineIntersection &inter = intersections[i];
    for (int j = 0; j < inter.points.size(); ++j)
      addNodePoint(IntersectionNodePoint(i, j));
  }

  con.addMessage(ILogWriter::REMARK, "[SPL] Found %d intersections in %g seconds.", intersections.size(),
    (dagTools->getTimeMsec() - startIntersections) / 1000.f);
  con.endLog();
}

void SplineExporter::addNodePoint(const IntersectionNodePoint &pt)
{
  SplineIntersection &inter = intersections[pt.nodeId];
  const IntersectionSplinePoint &pathSplPt = inter.points[pt.nodePointId];
  int splineId = inter.points[pt.nodePointId].splineId;
  SplineData &splData = splineData[splineId];

  int i;
  for (i = 0; i < splData.pathNodePoints.size(); ++i)
  {
    const IntersectionNodePoint &point = splData.pathNodePoints[i].point;
    if (point == pt)
    {
      G_ASSERT(false);
      return;
    }
    const IntersectionSplinePoint &curPathSplPt = intersections[point.nodeId].points[point.nodePointId];
    if (pathSplPt.pos < curPathSplPt.pos)
      break;
  }
  SplineIntersectionNode initWith = SplineIntersectionNode(pt, &inter);
  insert_items(splData.pathNodePoints, i, 1, &initWith);
}

void SplineExporter::makeConnections()
{
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  con.addMessage(ILogWriter::REMARK, "[SPL] Generating connections.");
  int startConnections = dagTools->getTimeMsec();

  for (int i = 0; i < splineData.size(); ++i)
  {
    SplineData &splData = splineData[i];
    for (int j = 0; j < (int)splData.pathNodePoints.size() - 1; ++j)
    {
      const SplineIntersectionNode &splPathNodeSrc = splData.pathNodePoints[j];
      const IntersectionNodePoint &srcPt = splPathNodeSrc.point;
      SplineIntersection *interSrc = splPathNodeSrc.inter;
      const IntersectionSplinePoint &splPtSrc = interSrc->points[srcPt.nodePointId];
      for (int k = j + 1; k < splData.pathNodePoints.size(); ++k)
      {
        const SplineIntersectionNode &splPathNodeDst = splData.pathNodePoints[k];
        const IntersectionNodePoint &dstPt = splPathNodeDst.point;
        SplineIntersection *interDst = splPathNodeDst.inter;
        const IntersectionSplinePoint &splPtDst = interDst->points[dstPt.nodePointId];
        G_ASSERT(splPtSrc.splineId == splPtDst.splineId);
        float dist = rabs(splPtSrc.pos - splPtDst.pos);

        interSrc->addEdge(dstPt.nodeId, dstPt.nodePointId, dist, srcPt.nodePointId);
        interDst->addEdge(srcPt.nodeId, srcPt.nodePointId, dist, dstPt.nodePointId);
      }
    }
  }

  con.addMessage(ILogWriter::REMARK, "[SPL] Generated connections in %g seconds.",
    (dagTools->getTimeMsec() - startConnections) / 1000.f);
  con.endLog();
}

bool SplineExporter::findWay(uint16_t from, uint16_t to)
{
  // Init.
  for (int i = 0; i < intersections.size(); ++i)
    intersections[i].resetPathFinder(intersections[to].position);
  SplineIntersection &frm = intersections[from];

  frm.pfInfo.navigated = true;
  frm.pfInfo.prevEdge.distance = 0.f;
  frm.pfInfo.prevEdge.interId = from;
  frm.pfInfo.prevEdge.pointId = 0;

  Tab<uint16_t> openNodes(tmpmem);
  openNodes.push_back(from);
  while (!openNodes.empty())
  {
    int shortest = -1;
    float minDist = FLT_MAX;
    for (int i = 0; i < openNodes.size(); ++i)
    {
      uint16_t nodeId = openNodes[i];
      G_ASSERT(nodeId < intersections.size());
      SplineIntersection &node = intersections[nodeId];
      float dist = node.pfInfo.distToDst + node.pfInfo.prevEdge.distance;
      if (dist < minDist)
      {
        shortest = i;
        minDist = dist;
      }
    }
    uint16_t nodeId = openNodes[shortest];
    if (nodeId == to)
      return true;
    erase_items(openNodes, shortest, 1);
    SplineIntersection &node = intersections[nodeId];
    node.pfInfo.navigated = true;
    for (int i = 0; i < node.edges.size(); ++i)
    {
      const IntersectionEdge &edge = node.edges[i];
      G_ASSERT(edge.interId < intersections.size());
      SplineIntersection &nextNode = intersections[edge.interId];
      if (nextNode.pfInfo.navigated)
        continue;
      IntersectionEdge &pfEdge = nextNode.pfInfo.prevEdge;
      float distance = node.pfInfo.prevEdge.distance + edge.distance;
      bool proceedToNode = false;
      if (find_value_idx(openNodes, edge.interId) < 0)
      {
        openNodes.push_back(edge.interId);
        proceedToNode = true;
      }
      else
        proceedToNode = distance < pfEdge.distance;

      if (proceedToNode)
      {
        pfEdge.interId = nodeId;
        pfEdge.pointId = edge.fromPointId;
        pfEdge.distance = distance;
        pfEdge.fromPointId = edge.pointId;
      }
    }
  }
  return false;
}

void SplineExporter::savePaths(mkbindump::BinDumpSaveCB &cwr)
{
  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  int startTable = dagTools->getTimeMsec();

  int size = intersections.size();

  cwr.beginTaggedBlock(_MAKE4C('stbl'));

  cwr.writeInt32e(size);
  for (int i = 0; i < size; ++i)
  {
    const SplineIntersection &inter = intersections[i];
    cwr.writeInt32e(inter.points.size());
    for (int j = 0; j < inter.points.size(); ++j)
    {
      cwr.writeReal(inter.points[j].pos);
      cwr.writeInt16e(inter.points[j].splineId);
      cwr.writeInt16e(0);
    }
  }

  SmallTab<IntersectionTableRecord, TmpmemAlloc> table;
  if (size < 360) // Less than 1 mb
  {
    con.addMessage(ILogWriter::REMARK, "[SPL] Calculating path table.");
    clear_and_resize(table, size * size);
    for (int i = 0; i < size; ++i)
      for (int j = 0; j < size; ++j)
        table[i + j * size] = IntersectionTableRecord();

    for (int i = 0; i < size; ++i)
      for (int j = 0; j < size; ++j)
        G_ASSERT(!table[i + j * size].nextPoint.isValid());

    for (int i = 0; i < size; ++i)
    {
      const SplineIntersection &firstInter = intersections[i];
      for (int j = 0; j < size; ++j)
      {
        if (i == j)
        {
          table[i + j * size].dist = FLT_MAX;
          continue;
        }
        const SplineIntersection &secondInter = intersections[j];
        if (!findWay(i, j))
        {
          table[i + j * size].dist = FLT_MAX;
          continue;
        }
        float totalDist = secondInter.pfInfo.prevEdge.distance;
        int nodeId = j;
        int step = 0;
        while (nodeId != i)
        {
          const SplineIntersection &inter = intersections[nodeId];
          const IntersectionEdge &edge = inter.pfInfo.prevEdge;
          const IntersectionEdge &prevEdge = intersections[edge.interId].pfInfo.prevEdge;
          IntersectionTableRecord &rec = table[edge.interId + j * size];
          bool overwrite = true;
          if (rec.nextPoint.isValid())
          {
            if (rec.nextPoint.nodeId != nodeId)
            {
              const PathFinderInfo &pfInfo = intersections[rec.nextPoint.nodeId].pfInfo;
              if (pfInfo.navigated && rec.dist < totalDist - intersections[pfInfo.prevEdge.interId].pfInfo.prevEdge.distance)
              {
                nodeId = rec.nextPoint.nodeId;
                step++;
                continue;
              }
            }
          }
          if (overwrite)
          {
            rec.dist = totalDist - prevEdge.distance;
            rec.nextPoint.nodeId = nodeId;
            rec.nextPoint.nodePointId = edge.fromPointId;
          }
          nodeId = edge.interId;
          step++;
        }
      }
    }
    cwr.beginTaggedBlock(_MAKE4C('tabl'));
    cwr.writeInt32e(table.size());
    for (int i = 0; i < table.size(); ++i)
    {
      const IntersectionTableRecord &rec = table[i];
      cwr.writeInt32e(rec.dist);
      cwr.writeInt16e(rec.nextPoint.nodeId);
      cwr.writeInt8e(rec.nextPoint.nodePointId);
      cwr.writeInt8e(0);
    }
    con.addMessage(ILogWriter::REMARK, "[SPL] Filled table in %g seconds.", (dagTools->getTimeMsec() - startTable) / 1000.f);
  }
  else
  {
    cwr.beginTaggedBlock(_MAKE4C('ncon'));
    for (int i = 0; i < size; ++i)
    {
      const SplineIntersection &inter = intersections[i];
      int edgesCount = inter.edges.size();
      cwr.write32ex(&inter.position, sizeof(Point2));
      cwr.writeInt32e(edgesCount);
      for (int j = 0; j < edgesCount; ++j)
      {
        const IntersectionEdge &edge = inter.edges[j];
        cwr.writeReal(edge.distance);
        cwr.writeInt16e(edge.interId);
        cwr.writeInt8e(edge.pointId);
        cwr.writeInt8e(0);
        cwr.writeInt8e(edge.fromPointId);
        cwr.writeInt8e(0);
        cwr.writeInt8e(0);
        cwr.writeInt8e(0);
      }
    }
  }
  cwr.align8();
  cwr.endBlock();


  for (int i = 0; i < splineData.size(); ++i)
  {
    const SplineData &splData = splineData[i];
    cwr.writeInt32e(splData.pathNodePoints.size());
    for (int j = 0; j < splData.pathNodePoints.size(); ++j)
    {
      const IntersectionNodePoint &pathNodePt = splData.pathNodePoints[j].point;
      cwr.writeInt16e(pathNodePt.nodeId);
      cwr.writeInt8e(pathNodePt.nodePointId);
      cwr.writeInt8e(0);
    }
  }

  cwr.align8();
  cwr.endBlock();

  con.endLog();
}

void SplineExporter::exportSplines(mkbindump::BinDumpSaveCB &cwr, const Tab<SplineObject *> &splines,
  const HeightMapStorage &heightMap, float gridCellSize, const Point2 &heightMapOffset)
{
  if (splines.empty())
    return;

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();

  const uint16_t width = clamp(heightMap.getMapSizeX() * gridCellSize / 128.f, 0.f, 512.f);
  const uint16_t height = clamp(heightMap.getMapSizeY() * gridCellSize / 128.f, 0.f, 512.f);
  Point2 cellSize(heightMap.getMapSizeX() * gridCellSize / float(width), heightMap.getMapSizeY() * gridCellSize / float(height));
  con.addMessage(ILogWriter::REMARK, "[SPL] width = %d, height = %d, cellSize = (%f, %f)", width, height, cellSize.x, cellSize.y);

  grid.init(width, height, heightMapOffset, cellSize);
  fillSplineData(splines);

  // Splines themselves.
  cwr.beginTaggedBlock(_MAKE4C('hspl'));
  cwr.writeInt32e(splineData.size());

  int splineId = 0;
  for (int i = 0; i < splineData.size(); ++i)
  {
    const SplineData &splData = splineData[i];
    cwr.writeDwString(splData.name.str());

    int numPoints = splData.segments.points.size();
    cwr.writeInt32e(numPoints);
    for (int j = 0; j < numPoints; ++j)
    {
      const SplineSegmentPoint &pt = splData.segments.points[j];
      cwr.write32ex(&pt.in, sizeof(Point3));
      cwr.write32ex(&pt.pos, sizeof(Point3));
      cwr.write32ex(&pt.out, sizeof(Point3));
    }
  }
  cwr.align8();
  cwr.endBlock();

  // Splines grid.
  grid.write(cwr);

  // Pathfinding info
  {
    con.addMessage(ILogWriter::REMARK, "[SPL] Generating pathfinding info.");
    int startPathFinding = dagTools->getTimeMsec();

    findIntersections();
    makeConnections();
    savePaths(cwr);

    con.addMessage(ILogWriter::REMARK, "[SPL] Generated pathfinding info in %g seconds.",
      (dagTools->getTimeMsec() - startPathFinding) / 1000.f);
  }

  con.endLog();
}


void HmapLandPlugin::exportSplines(mkbindump::BinDumpSaveCB &cwr)
{
  Tab<SplineObject *> splines(tmpmem);
  for (int i = 0; i < objEd.objectCount(); i++)
  {
    SplineObject *o = RTTI_cast<SplineObject>(objEd.getObject(i));
    if (o && EditLayerProps::layerProps[o->getEditLayerIdx()].exp && o->getProps().exportable && !o->points.empty())
      splines.push_back(o);
  }

  CoolConsole &con = DAGORED2->getConsole();
  con.startLog();
  con.addMessage(ILogWriter::REMARK, "[SPL] spline objects %d", splines.size());
  con.endLog();

  SplineExporter exporter;
  exporter.exportSplines(cwr, splines, heightMap, gridCellSize, heightMapOffset);
}
