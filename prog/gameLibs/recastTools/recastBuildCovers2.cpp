// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildCovers.h>
#include <recastTools/recastTools.h>

#include <EASTL/vector.h>
#include <EASTL/sort.h>

#ifdef _MSC_VER
#pragma warning(disable : 4701)
#endif

namespace
{
const float COVERS_MAX_HEIGHT = 1.80f;
const float COVERS_MIN_HEIGHT = 0.42f;
const float COVERS_POS_RADIUS = 0.30f;
const float COVERS_STANDARD_WIDTH = 1.00f;
const float COVERS_WALL_OFFSET = 0.50f;

static void covtrace(...) {}
// template <typename... Args>
// __forceinline void covtrace(Args... args)
// {
//    debug(args...);
// }
} // namespace

namespace recastbuild
{
class CoversArr
{
public:
  struct CoverPos
  {
    Point3 pt;
    float r = 0.0f;
    float h = 0.0f;

    Point3 along;
    int edge = -1;

    int next = -1;
    int intersections = 0;

    bool removed = false;

    int coverIdx = -1;
    float coverScore = 0.0f;

    static float calcScore(bool isHalfCrouch, bool isHalfCrawl, bool isAround, bool isWalkout, bool isExtra, float hiddenCoef)
    {
      if (isHalfCrouch) // crouch to stand
        return 3.0f;
      if (isHalfCrawl) // crawl to crouch/stand
        return 2.5f;
      if (isWalkout && !isAround) // full walkout (not around trees/columns/corners)
        return 2.0f;
      if (isWalkout && isAround) // full walkout (around trees/columns/corners)
        return min(1.0f, hiddenCoef);
      if (isExtra) // extra cover (in case not enough other ones)
        return -1.0f;
      return 1.5f; // full stand or crouch cover
    }
  };

  eastl::vector<CoverPos> arr;
  eastl::vector<eastl::pair<Point3, Point3>> edges;
  eastl::vector<int> indx;
  int indx_px1 = 0;
  int indx_pz1 = 0;
  int indx_px2 = 0;
  int indx_pz2 = 0;
  int indx_xs = 0;
  int indx_zs = 0;

  float calcEdgeLenSq(int idx) const
  {
    if (idx < 0)
      return 0.0f;
    const auto &edge = edges[idx];
    return lengthSq(edge.first - edge.second);
  }

  void reservePositions(size_t sz) { arr.reserve(sz); }
  int addNewEdge(const Point3 &edge_start, const Point3 &edge_end)
  {
    edges.push_back({edge_start, edge_end});
    return (int)edges.size() - 1;
  }

  void addPosition(const Point3 &pt, float radius, float height, const Point3 &along, int edge);
  void addPosition(const Point3 &pt, float radius, float height, const Point3 &along, int edge, const BBox3 &limit_box);
  void addEdgePositions(const Point3 &edge_start, const Point3 &edge_end, float radius, float height, float off_dist,
    const BBox3 &limit_box);
  void mergePositions(float mergeCoef);

public:
  void wipeRemoved();
  void rebuildIndex();
  void gatherAround(eastl::vector<int> &out_idxs, const Point3 &pt, float radius, float height);
  int countAround(const Point3 &pt, int except_idx, float radius, float height, int max_count, float max_other_height);

public:
  static bool isIntersecting(const CoverPos &a, const CoverPos &b, float coef = 1.0f);
  static bool isInsideXZLimits(const Point3 &pt, const BBox3 &limit_box, float radius);
  static int getCell(float v);
};

void CoversArr::addPosition(const Point3 &pt, float radius, float height, const Point3 &along, int edge)
{
  CoverPos &pos = arr.push_back();
  pos.pt = pt;
  pos.r = radius;
  pos.h = height;
  pos.along = along;
  pos.edge = edge;
}

void CoversArr::addPosition(const Point3 &pt, float radius, float height, const Point3 &along, int edge, const BBox3 &limit_box)
{
  if (isInsideXZLimits(pt, limit_box, radius))
    addPosition(pt, radius, height, along, edge);
}

void CoversArr::addEdgePositions(const Point3 &edge_start, const Point3 &edge_end, float radius, float height, float off_dist,
  const BBox3 &limit_box)
{
  const Point3 delta = edge_end - edge_start;
  const Point3 along = normalize(Point3::x0z(delta));
  const Point3 norm = Point3(along.z, 0.0f, -along.x);

  const float covPosLen = radius * 2.0f;
  const float covPosRadius = radius;
  const float covPosHeight = height;
  const float covPosDistFrom = off_dist;
  const float covPosDistCorner = sqrtf(covPosDistFrom * covPosDistFrom * 0.5f);

  const int edgeLen = length(delta);
  const int numPositions = max(1, (int)ceilf(edgeLen / covPosLen));

  const Point3 posStep = delta / (float)numPositions;
  const Point3 startPt = edge_start + posStep * 0.5f;

  const int edge = addNewEdge(edge_start, edge_end);

  for (int posIdx = 0; posIdx < numPositions; ++posIdx)
  {
    Point3 posPt = startPt + posStep * posIdx;

    addPosition(posPt + norm * covPosDistFrom, covPosRadius, covPosHeight, along, edge, limit_box);
    addPosition(posPt - norm * covPosDistFrom, covPosRadius, covPosHeight, along, edge, limit_box);
  }

  addPosition(edge_start - norm * covPosDistCorner - along * covPosDistCorner, covPosRadius, covPosHeight, along, edge, limit_box);
  addPosition(edge_end - norm * covPosDistCorner + along * covPosDistCorner, covPosRadius, covPosHeight, along, edge, limit_box);
}

bool CoversArr::isIntersecting(const CoverPos &a, const CoverPos &b, float coef)
{
  if (a.pt.y > b.pt.y + b.h)
    return false;
  if (b.pt.y > a.pt.y + a.h)
    return false;

  const float dx = b.pt.x - a.pt.x;
  const float dz = b.pt.z - a.pt.z;
  const float dd = dx * dx + dz * dz;
  const float rsum = (a.r + b.r) * coef;
  if (dd > rsum * rsum)
    return false;

  return true;
}

bool CoversArr::isInsideXZLimits(const Point3 &pt, const BBox3 &limit_box, float radius)
{
  if (pt.x < limit_box.boxMin().x - radius || pt.x > limit_box.boxMax().x + radius)
    return false;
  if (pt.z < limit_box.boxMin().z - radius || pt.z > limit_box.boxMax().z + radius)
    return false;
  return true;
}

int CoversArr::getCell(float v)
{
  const float CELL_SIZE = 2.0f;
  return (int)floorf(v * (1.0f / CELL_SIZE));
}

void CoversArr::wipeRemoved()
{
  for (int i = 0; i < (int)arr.size(); ++i)
  {
    if (arr[i].removed)
    {
      arr[i] = arr.back();
      arr.pop_back();
      --i;
    }
  }
}

void CoversArr::rebuildIndex()
{
  for (int i = 0; i < (int)arr.size(); ++i)
  {
    CoverPos &pos = arr[i];
    pos.next = -1;
  }

  indx_px1 = 0;
  indx_pz1 = 0;
  indx_px2 = 0;
  indx_pz2 = 0;
  bool first = true;
  for (int i = 0; i < (int)arr.size(); ++i)
  {
    CoverPos &pos = arr[i];
    if (pos.removed)
      continue;
    const int px = getCell(pos.pt.x);
    const int pz = getCell(pos.pt.z);
    if (first)
    {
      first = false;
      indx_px1 = px;
      indx_pz1 = pz;
      indx_px2 = px;
      indx_pz2 = pz;
      continue;
    }
    indx_px1 = min(indx_px1, px);
    indx_pz1 = min(indx_pz1, pz);
    indx_px2 = max(indx_px2, px);
    indx_pz2 = max(indx_pz2, pz);
  }

  indx_xs = indx_px2 - indx_px1 + 1;
  indx_zs = indx_pz2 - indx_pz1 + 1;

  indx.clear();
  indx.resize(indx_xs * indx_zs, -1);
  for (int i = 0; i < (int)arr.size(); ++i)
  {
    CoverPos &pos = arr[i];
    if (pos.removed)
      continue;
    const int px = getCell(pos.pt.x) - indx_px1;
    const int pz = getCell(pos.pt.z) - indx_pz1;
    int &cell = indx[px + pz * indx_xs];
    pos.next = cell;
    cell = i;
  }
}

void CoversArr::gatherAround(eastl::vector<int> &out_idxs, const Point3 &pt, float radius, float height)
{
  CoverPos test;
  test.pt = pt;
  test.h = height;
  test.r = radius;

  const int px = getCell(pt.x) - indx_px1;
  const int pz = getCell(pt.z) - indx_pz1;
  const int cz1 = clamp(pz - 1, 0, indx_zs - 1);
  const int cz2 = clamp(pz + 1, 0, indx_zs - 1);
  for (int cz = cz1; cz <= cz2; ++cz)
  {
    const int cx1 = clamp(px - 1, 0, indx_xs - 1);
    const int cx2 = clamp(px + 1, 0, indx_xs - 1);
    for (int cx = cx1; cx <= cx2; ++cx)
    {
      int idx = indx[cx + cz * indx_xs];
      while (idx >= 0)
      {
        CoverPos &pos2 = arr[idx];
        if (!pos2.removed && isIntersecting(test, pos2))
          out_idxs.push_back(idx);
        idx = pos2.next;
      }
    }
  }
}

int CoversArr::countAround(const Point3 &pt, int except_idx, float radius, float height, int max_count, float max_other_height)
{
  CoverPos test;
  test.pt = pt;
  test.h = height;
  test.r = radius;

  int count = 0;
  const int px = getCell(pt.x) - indx_px1;
  const int pz = getCell(pt.z) - indx_pz1;
  const int cz1 = clamp(pz - 1, 0, indx_zs - 1);
  const int cz2 = clamp(pz + 1, 0, indx_zs - 1);
  for (int cz = cz1; cz <= cz2; ++cz)
  {
    const int cx1 = clamp(px - 1, 0, indx_xs - 1);
    const int cx2 = clamp(px + 1, 0, indx_xs - 1);
    for (int cx = cx1; cx <= cx2; ++cx)
    {
      int idx = indx[cx + cz * indx_xs];
      while (idx >= 0)
      {
        const CoverPos &pos = arr[idx];
        if (idx != except_idx && !pos.removed)
        {
          CoverPos posCopy = pos;
          if (posCopy.h > max_other_height)
            posCopy.h = max_other_height;
          if (isIntersecting(test, posCopy))
          {
            if (++count >= max_count)
              return count;
          }
        }
        idx = pos.next;
      }
    }
  }
  return count;
}

void CoversArr::mergePositions(float mergeCoef)
{
  wipeRemoved();

  if (arr.empty())
    return;

  // covtrace("CoversArr: start count = %d", (int)arr.size());

  eastl::vector<int> idxs;

  bool merging = true;
  while (merging)
  {
    merging = false;

    for (int i = 0; i < (int)arr.size(); ++i)
    {
      CoverPos &pos = arr[i];
      pos.intersections = 0;
    }

    rebuildIndex();

    for (int i = 0; i < (int)arr.size(); ++i)
    {
      CoverPos &pos = arr[i];
      const int px = getCell(pos.pt.x) - indx_px1;
      const int pz = getCell(pos.pt.z) - indx_pz1;
      const int cz1 = clamp(pz - 1, 0, indx_zs - 1);
      const int cz2 = clamp(pz + 1, 0, indx_zs - 1);
      for (int cz = cz1; cz <= cz2; ++cz)
      {
        const int cx1 = clamp(px - 1, 0, indx_xs - 1);
        const int cx2 = clamp(px + 1, 0, indx_xs - 1);
        for (int cx = cx1; cx <= cx2; ++cx)
        {
          int idx = indx[cx + cz * indx_xs];
          while (idx >= 0)
          {
            CoverPos &pos2 = arr[idx];
            if (idx != i && isIntersecting(pos, pos2))
              pos.intersections += 1;
            idx = pos2.next;
          }
        }
      }
    }

    idxs.clear();
    idxs.resize(arr.size(), -1);
    for (int i = 0; i < (int)arr.size(); ++i)
      idxs[i] = i;

    eastl::sort(idxs.begin(), idxs.end(), [&](auto &a, auto &b) { return arr[a].intersections > arr[b].intersections; });

    for (int ci = 0; ci < (int)idxs.size(); ++ci)
    {
      const int cidx = idxs[ci];
      CoverPos &pos = arr[cidx];
      if (pos.removed)
        continue;

      const int px = getCell(pos.pt.x) - indx_px1;
      const int pz = getCell(pos.pt.z) - indx_pz1;
      const int cz1 = clamp(pz - 1, 0, indx_zs - 1);
      const int cz2 = clamp(pz + 1, 0, indx_zs - 1);
      for (int cz = cz1; cz <= cz2; ++cz)
      {
        const int cx1 = clamp(px - 1, 0, indx_xs - 1);
        const int cx2 = clamp(px + 1, 0, indx_xs - 1);
        for (int cx = cx1; cx <= cx2; ++cx)
        {
          int prev = -1;
          int idx = indx[cx + cz * indx_xs];
          while (idx >= 0)
          {
            CoverPos &pos2 = arr[idx];
            if (idx != cidx && !pos2.removed && isIntersecting(pos, pos2, mergeCoef))
            {
              merging = true;

              pos.pt += pos2.pt;
              pos.pt *= 0.5f;
              pos.r = (pos.r + pos2.r) * 0.5f;
              pos.h = (pos.h + pos2.h) * 0.5f;
              if (pos.edge != pos2.edge && pos.edge != -1 && pos2.edge != -1)
              {
                const auto &e1 = edges[pos.edge];
                const auto &e2 = edges[pos2.edge];
                const Point3 d1 = normalize(e1.second - e1.first);
                const Point3 d2 = normalize(e2.second - e2.first);
                const float epsParallel = 0.99f;
                const float kParallel = dot(d1, d2);
                if (kParallel < epsParallel)
                {
                  if (kParallel <= -epsParallel)
                    pos2.along = -pos2.along;
                  else
                  {
                    const float dff = lengthSq(e1.first - e2.first);
                    const float dfs = lengthSq(e1.first - e2.second);
                    const float dss = lengthSq(e1.second - e2.second);
                    const float dsf = lengthSq(e1.second - e2.first);
                    if ((dff < dfs && dff < dss && dff < dsf) || (dss < dfs && dss < dff && dss < dsf))
                      pos2.along = -pos2.along;
                  }
                }
                pos.along = normalize(pos.along + pos2.along);
              }
              pos2.removed = true;
              if (prev < 0)
                indx[cx + cz * indx_xs] = pos2.next;
              else
                arr[prev].next = pos2.next;
              pos2.next = -1;
            }
            prev = idx;
            idx = pos2.next;
          }
        }
      }
    }

    wipeRemoved();
  } // merging

  for (int i = 0; i < (int)arr.size(); ++i)
  {
    CoverPos &pos = arr[i];
    if (pos.removed)
    {
      covtrace("CoversArr: !!! found not wiped !!!");
      continue;
    }
    for (int j = i + 1; j < (int)arr.size(); ++j)
    {
      CoverPos &pos2 = arr[j];
      if (i != j && !pos2.removed && isIntersecting(pos, pos2, mergeCoef))
      {
        covtrace("CoversArr: !!! found not merged !!!");
        covtrace("CoversArr: pos1 idx=%d x=%f y=%f z=%f r=%f h=%f", i, pos.pt.x, pos.pt.y, pos.pt.z, pos.r, pos.h);
        covtrace("CoversArr: pos2 idx=%d x=%f y=%f z=%f r=%f h=%f", j, pos2.pt.x, pos2.pt.y, pos2.pt.z, pos2.r, pos2.h);
        const float adx = fabsf(pos.pt.x - pos2.pt.x);
        const float adz = fabsf(pos.pt.z - pos2.pt.z);
        const float dist = sqrtf(adx * adx + adz * adz);
        covtrace("CoversArr: adx=%f adz=%f dist=%f r1=%f r2=%f", adx, adz, dist, pos.r, pos2.r);
      }
    }
  }

  // covtrace("CoversArr: merged count = %d", (int)arr.size());
}


int trace_covers2_place(const rcCompactHeightfield *chf, float min_height, RenderEdges *edges_out, Point3 &in_out_pos, float radius)
{
  int result = 1;

  const float maxOver = 1.00f;
  const float maxUnder = 0.70f;
  const float maxHtDiff = 0.30f;
  const float minHeight = min_height;

  const auto &pt = in_out_pos;
  const float range = radius + chf->cs;
  const float radiusSq = sqr(radius);

  const int ix0 = clamp((int)floorf((pt.x - range - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz0 = clamp((int)floorf((pt.z - range - chf->bmin[2]) / chf->cs), 0, chf->height - 1);
  const int ix1 = clamp((int)floorf((pt.x + range - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz1 = clamp((int)floorf((pt.z + range - chf->bmin[2]) / chf->cs), 0, chf->height - 1);

  float min_py = FLT_MAX;
  float max_py = -FLT_MAX;
  float min_py2 = FLT_MAX;

  Tab<eastl::pair<float, float>> cols(tmpmem);
  cols.reserve(32);
  for (int iz = iz0; iz <= iz1; ++iz)
  {
    for (int ix = ix0; ix <= ix1; ++ix)
    {
      const float px = (float)ix * chf->cs + chf->bmin[0];
      const float pz = (float)iz * chf->cs + chf->bmin[2];
      const float dx = pt.x - px;
      const float dz = pt.z - pz;
      const float dd = dx * dx + dz * dz;
      if (dd > radiusSq)
        continue;

      bool found = false;
      const rcCompactCell &c = chf->cells[ix + iz * chf->width];
      for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
      {
        const rcCompactSpan &s = chf->spans[i];
        if (chf->areas[i] == RC_NULL_AREA)
          continue;

        const float py = chf->bmin[1] + (int)s.y * chf->ch;
        const float ph = (int)s.h * chf->ch;

        if (py > pt.y - maxUnder && py < pt.y + maxOver && ph >= minHeight && py + ph > pt.y)
        {
          min_py = min(min_py, py);
          max_py = max(max_py, py);
          min_py2 = min(min_py2, py + ph);

          cols.push_back({py, ph});
          found = true;

          // FOR DEBUG
          // if (edges_out)
          //  edges_out->push_back({Point3(ph, -1024.5f, chf->cs), Point3(px, py, pz)});
          break;
        }
      }
      if (!found)
      {
        result = 0;
        break;
      }
    }
    if (!result)
      break;
  }

  const float htDiff = max_py - min_py;
  if (htDiff > maxHtDiff)
    result = -1;

  if (min_py2 - max_py < minHeight)
    result = 0;

  // FOR DEBUG
  // const int num_cols = (int)cols.size();
  // if (edges_out && result != 1 && num_cols > 0)
  //  edges_out->resize(edges_out->size() - num_cols);
  G_UNUSED(edges_out);

  if (result == 1)
    in_out_pos.y = min_py;

  return result;
}


struct CoversTracer
{
  const rcCompactHeightfield *chf;
  const rcHeightfield *solid;
  float ks, kh;
  int dxdir[2];
  int dzdir[2];

  int hh;            // ground hole height
  int sh1, sh2, sh3; // shoot heights
  int eh1, eh2, eh3; // expose heights
  int bodyLen, covDist, shotDist, nearDist;

  int goodShootSum, maxSeenCrawl, maxSeenCrouch;

  IPoint3 start;
  int startSpan;
  int startY, startH;

  struct Range
  {
    unsigned int dtop : 8, dbot : 8;
    int dytop : 8, dybot : 8;

    void setEmpty()
    {
      dtop = dbot = 1;
      dytop = -128;
      dybot = 127;
    }

    void setTop(int dist, int dy)
    {
      if (correctPos(dist, dy))
      {
        dtop = dist;
        dytop = dy;
      }
    }

    void setBot(int dist, int dy)
    {
      if (correctPos(dist, dy))
      {
        dbot = dist;
        dybot = dy;
      }
    }

    bool isEmpty() const { return (dtop != 0) && (dbot != 0) && (int)dytop * (int)dbot <= (int)dybot * (int)dtop; }
    bool isSteep() const { return isEmpty() || ((dbot != 0) && dybot > dbot * 5); }

    bool inRange(int dist, int dy) const { return inRangeTop(dist, dy) && inRangeBot(dist, dy); }
    bool inRangeTop(int dist, int dy) const { return !dtop || dy * (int)dtop <= (int)dytop * dist; }
    bool inRangeBot(int dist, int dy) const { return !dbot || dy * (int)dbot >= (int)dybot * dist; }

    bool inRangeSpan(int dist, int dy, int dh) const
    {
      const int dy2 = dy + dh;
      if (!inRangeBot(dist, dy2))
        return false;
      if (!inRangeTop(dist, dy))
        return false;
      return true;
    }

    float debugAngleTop(float coef) const { return !dtop ? 90.0f : RadToDeg(atan2f((float)dytop, coef * (float)dtop)); }
    float debugAngleBot(float coef) const { return !dbot ? -90.0f : RadToDeg(atan2f((float)dybot, coef * (float)dbot)); }

    static bool correctPos(int &dist, int &dy)
    {
      while (dy < -128 || dy > 127)
      {
        dy >>= 1;
        dist >>= 1;
        if (!dist)
          return false;
      }
      return true;
    }

    bool clipRangeTop(int dist, int dy)
    {
      if (!inRangeTop(dist, dy))
        return false;
      if (!correctPos(dist, dy))
        return false;
      dtop = dist;
      dytop = dy;
      return true;
    }

    bool clipRangeBot(int dist, int dy)
    {
      if (!inRangeBot(dist, dy))
        return false;
      if (!correctPos(dist, dy))
        return false;
      dbot = dist;
      dybot = dy;
      return true;
    }

    int clipRange(int dist, int dy_high, int dy_low)
    {
      if (!inRangeTop(dist, dy_low))
        return 0;
      if (!inRangeBot(dist, dy_high))
        return 0;
      if (!inRangeTop(dist, dy_high))
      {
        if (!inRangeBot(dist, dy_low))
          return -1; // need remove
        if (correctPos(dist, dy_low))
        {
          dtop = dist;
          dytop = dy_low;
        }
        return 0;
      }
      if (!inRangeBot(dist, dy_low))
      {
        if (correctPos(dist, dy_high))
        {
          dbot = dist;
          dybot = dy_high;
        }
        return 0;
      }
      return 1; // need split
    }
  };

  struct Ranges
  {
    static bool hasB(int n) { return (n & 4) != 0; }
    static int addB(int n, bool high) { return n | 4 | (high ? 2 : 0); }
    static int delB(int n) { return n & ~4; }
    static bool isLowA(int n) { return (n & 1) == 0; }
    static bool isLowB(int n) { return (n & 2) == 0; }
    static bool isHighA(int n) { return (n & 1) != 0; }
    static bool isHighB(int n) { return (n & 2) != 0; }
    static int setHighA(int n) { return n | 1; }
    static int setHighB(int n) { return n | 2; }
    static int resetHighA(int n) { return n & ~1; }
    static int resetHighB(int n) { return n & ~2; }

    static bool isEmpty(const Range *r, int n) { return r[0].isEmpty() && (!hasB(n) || r[1].isEmpty()); }

    static bool inRanges(int dist, int dy, bool high, const Range *r, int n)
    {
      if (isHighA(n) == high && r[0].inRange(dist, dy))
        return true;
      if (hasB(n) && isHighB(n) == high && r[1].inRange(dist, dy))
        return true;
      return false;
    }

    static int clipRangesTop(Range *r, int n, int dist, int dy, bool high)
    {
      if (hasB(n) && isHighB(n) == high)
      {
        if (r[1].clipRangeTop(dist, dy))
          if (r[1].isEmpty())
            n = delB(n);
      }
      if (isHighA(n) == high)
      {
        if (r[0].clipRangeTop(dist, dy))
          if (hasB(n) && r[0].isEmpty())
          {
            n = isHighB(n) ? setHighA(0) : 0;
            r[0] = r[1];
          }
      }
      return n;
    }

    static int clipRangesBot(Range *r, int n, int dist, int dy, bool high)
    {
      if (hasB(n) && isHighB(n) == high)
      {
        if (r[1].clipRangeBot(dist, dy))
          if (r[1].isEmpty())
            n = delB(n);
      }
      if (isHighA(n) == high)
      {
        if (r[0].clipRangeBot(dist, dy))
          if (hasB(n) && r[0].isEmpty())
          {
            n = isHighB(n) ? setHighA(0) : 0;
            r[0] = r[1];
          }
      }
      return n;
    }

    static int splitRanges(Range *r, int n, int dist, int dy_high, int dy_low)
    {
      int action0 = r[0].clipRange(dist, dy_high, dy_low);
      if (action0 == -1)
      {
        if (!hasB(n))
        {
          r[0].setEmpty();
          return 0;
        }
        n = isHighB(n) ? setHighA(0) : 0;
        r[0] = r[1];
        action0 = r[0].clipRange(dist, dy_high, dy_low);
      }
      else if (hasB(n) && action0 != 1)
      {
        int action1 = r[1].clipRange(dist, dy_high, dy_low);
        if (action1 == 1) // split (can't have more than 2 ranges, leave bottom only)
          r[1].setTop(dist, dy_low);
        else if (action1 == -1)
          n = delB(n);
      }
      if (action0 == 1) // split (high B and low A)
      {
        n = addB(resetHighA(n), true);
        r[1].dtop = r[0].dtop;
        r[1].dytop = r[0].dytop;
        r[1].setBot(dist, dy_high);
        r[0].setTop(dist, dy_low);
      }
      return n;
    }

    static int lowerRanges(int n)
    {
      n = resetHighA(resetHighB(n));
      return n;
    }

    static bool inRangesLow(const Range *r, int n, int dist, int dy, int dh)
    {
      if (isLowA(n) && r[0].inRangeSpan(dist, dy, dh))
        return true;
      if (hasB(n) && isLowB(n) && r[1].inRangeSpan(dist, dy, dh))
        return true;
      return false;
    }
  };

  struct State
  {
    unsigned int px : 16, pz : 16;
    unsigned int span : 24, aim1n : 3, aim2n : 3, stop : 1;
    unsigned int spanY : 16, spanH : 16, highY : 16, highH : 16;
    unsigned int shot1 : 10, shot2 : 10, shot3 : 10, cover : 1, wall : 1;
    unsigned int over1 : 10, over2 : 10, aim3n : 3;
    // +24 bytes
    Range aim1[2];
    Range aim2[2];
    Range aim3[2];
    // +24 bytes
    // 48 bytes total

    void applyShot1(int dy, int hh)
    {
      ++shot1;
      if (dy >= hh)
        ++over1;
    }
    void applyShot2(int dy, int hh)
    {
      ++shot2;
      if (dy >= hh)
        ++over2;
    }
    void applyShot3() { ++shot3; }

    bool inRangesLow(int dist, int dist3, int dy1, int dy2, int dy3, int dh) const
    {
      if (Ranges::inRangesLow(aim1, aim1n, dist, dy1, dh))
        return true;
      if (Ranges::inRangesLow(aim2, aim2n, dist, dy2, dh))
        return true;
      if (Ranges::inRangesLow(aim3, aim3n, dist3, dy3, dh))
        return true;
      return false;
    }

    void clipTopEdge(int dist, int ny, bool high, int by1, int by2, int by3, int dist3)
    {
      aim1n = Ranges::clipRangesTop(aim1, aim1n, dist, ny - by1, high);
      aim2n = Ranges::clipRangesTop(aim2, aim2n, dist, ny - by2, high);
      aim3n = Ranges::clipRangesTop(aim3, aim3n, dist3, ny - by3, high);
    }
    void clipBotEdge(int dist, int ny, bool high, int by1, int by2, int by3, int dist3)
    {
      aim1n = Ranges::clipRangesBot(aim1, aim1n, dist, ny - by1, high);
      aim2n = Ranges::clipRangesBot(aim2, aim2n, dist, ny - by2, high);
      aim3n = Ranges::clipRangesBot(aim3, aim3n, dist3, ny - by3, high);
    }
    void splitRanges(int dist, int dy_high, int dy_low, int by1, int by2, int by3, int dist3)
    {
      aim1n = Ranges::splitRanges(aim1, aim1n, dist, dy_high - by1, dy_low - by1);
      aim2n = Ranges::splitRanges(aim2, aim2n, dist, dy_high - by2, dy_low - by2);
      aim3n = Ranges::splitRanges(aim3, aim3n, dist3, dy_high - by3, dy_low - by3);
    }
    void lowerRanges()
    {
      aim1n = Ranges::lowerRanges(aim1n);
      aim2n = Ranges::lowerRanges(aim2n);
      aim3n = Ranges::lowerRanges(aim3n);
    }
  };

  void debug_print_ranges(const State &state)
  {
    auto printRanges = [&](const Range *r, int n, const char *name) {
      const float coef = chf->cs * kh;
      if (!Ranges::hasB(n))
      {
        covtrace("!!! AIM RANGES: %s %c(%.2f %.2f)", name, Ranges::isHighA(n) ? 'H' : 'L', r[0].debugAngleTop(coef),
          r[0].debugAngleBot(coef));
      }
      else
      {
        covtrace("!!! AIM RANGES: %s %c(%.2f %.2f) %c(%.2f %.2f)", name, Ranges::isHighA(n) ? 'H' : 'L', r[0].debugAngleTop(coef),
          r[0].debugAngleBot(coef), Ranges::isHighB(n) ? 'H' : 'L', r[1].debugAngleTop(coef), r[1].debugAngleBot(coef));
      }
    };
    printRanges(state.aim1, state.aim1n, "aim1");
    printRanges(state.aim2, state.aim2n, "aim2");
    printRanges(state.aim3, state.aim3n, "aim3");
  };

  static const int CACHE_SIZE = 7;
  State states[CACHE_SIZE + 1];
  int ncached;

  CoversTracer(const rcCompactHeightfield *_chf, const rcHeightfield *_solid, float shoot_ht_stand, float shoot_ht_crouch, //-V730
    float shoot_ht_crawl, float expose_ht_stand, float expose_ht_crouch, float expose_ht_crawl, float crawl_body_length,
    float cover_max_dist, float shoot_min_dist, float shoot_near_dist, float ground_hole_ht, float good_shootable_sumdist,
    float max_sumdist_crawling, float max_sumdist_crouched)
  {
    chf = _chf;
    solid = _solid;

    G_ASSERT(chf->cs == solid->cs);
    G_ASSERT(chf->ch == solid->ch);
    G_ASSERT(chf->width == solid->width);
    G_ASSERT(chf->height == solid->height);
    G_ASSERT(chf->bmin[0] == solid->bmin[0]);
    G_ASSERT(chf->bmin[1] == solid->bmin[1]);
    G_ASSERT(chf->bmin[2] == solid->bmin[2]);

    ks = 65536.0f / chf->cs; // 16-bit fixed-point conv.coef for X and Z
    kh = 1.0f / chf->ch;     // whole number (not fixed-point) conv.coef for Y

    dxdir[0] = rcGetDirForOffset(-1, 0);
    dxdir[1] = rcGetDirForOffset(1, 0);
    dzdir[0] = rcGetDirForOffset(0, -1);
    dzdir[1] = rcGetDirForOffset(0, 1);

    hh = (int)ceilf(ground_hole_ht * kh);

    sh1 = (int)ceilf(shoot_ht_stand * kh);
    sh2 = (int)ceilf(shoot_ht_crouch * kh);
    sh3 = (int)ceilf(shoot_ht_crawl * kh);

    eh1 = (int)ceilf(expose_ht_stand * kh);
    eh2 = (int)ceilf(expose_ht_crouch * kh);
    eh3 = (int)ceilf(expose_ht_crawl * kh);

    bodyLen = (int)floorf(crawl_body_length * ks);
    covDist = (int)floorf(cover_max_dist * ks);
    shotDist = (int)floorf(shoot_min_dist * ks);
    nearDist = (int)floorf(shoot_near_dist * ks);

    goodShootSum = (int)ceilf(good_shootable_sumdist / chf->cs);
    maxSeenCrawl = (int)ceilf(max_sumdist_crawling / chf->cs);
    maxSeenCrouch = (int)ceilf(max_sumdist_crouched / chf->cs);

    startSpan = -1;
  }

  bool init_position(const Point3 &_start, const BBox3 &_limits, bool debug_this_pos)
  {
    startSpan = -1;
    ncached = 0;

    if (_start.x < _limits.lim[0].x || _start.x > _limits.lim[1].x)
      return false;
    if (_start.z < _limits.lim[0].z || _start.z > _limits.lim[1].z)
      return false;

    const float *bmin = chf->bmin;
    start.x = (int)floorf((_start.x - bmin[0]) * ks);
    start.y = (int)floorf((_start.y - bmin[1]) * kh);
    start.z = (int)floorf((_start.z - bmin[2]) * ks);

    const int px = start.x >> 16;
    if (px < 0 || px >= chf->width)
      return false;
    const int pz = start.z >> 16;
    if (pz < 0 || pz >= chf->height)
      return false;

    // if (debug_this_pos)
    // {
    //   covtrace("!!! CHF   cs=%f ch=%f w=%d h=%d bmin(%f,%f,%f) bmax(%f,%f,%f)",
    //     chf->cs, chf->ch, chf->width, chf->height,
    //     chf->bmin[0], chf->bmin[1], chf->bmin[2],
    //     chf->bmax[0], chf->bmax[1], chf->bmax[2]);
    //   covtrace("!!! SOLID cs=%f ch=%f w=%d h=%d bmin(%f,%f,%f) bmax(%f,%f,%f)",
    //     solid->cs, solid->ch, solid->width, solid->height,
    //     solid->bmin[0], solid->bmin[1], solid->bmin[2],
    //     solid->bmax[0], solid->bmax[1], solid->bmax[2]);
    // }

    if (debug_this_pos)
      covtrace("!!! START (%f,%f,%f) => (%d,%d,%d)", _start.x, _start.y, _start.z, start.x, start.y, start.z);

    const auto &cell = chf->cells[px + pz * chf->width];
    const int py = start.y;

    if (debug_this_pos)
      covtrace("!!! START cell spans = %d", cell.count);

    int idx = cell.index;
    int cnt = cell.count;
    while (cnt-- > 0)
    {
      const auto &span = chf->spans[idx++];
      if (debug_this_pos)
        covtrace("!!! START ?SPAN %d y=%d h=%d py=%d", idx - 1, span.y, span.h, py);
      if ((int)span.y <= py + 1 && (int)span.y + (int)span.h > py + 1)
      {
        startSpan = idx - 1;
        startY = span.y;
        startH = span.h;

        if (startH < eh1)
        {
          if (debug_this_pos)
            covtrace("!!! START TOO_LOW SPAN %d startY=%d startH=%d", startSpan, startY, startH);
          return false;
        }

        if (debug_this_pos)
          covtrace("!!! START SPAN %d startY=%d startH=%d", startSpan, startY, startH);
        return true;
      }
    }

    if (debug_this_pos)
      covtrace("!!! START SPAN NOT FOUND");
    return false;
  }

  enum ECover
  {
    NO_START,              // not initialized
    NO_DIRECTION,          // bad direction
    NO_COVER_BAD_ATTACK,   // not covered, bad attack direction
    NO_COVER_GOOD_ATTACK,  // not covered, good attack direction
    INEFFECTIVE_COVER,     // not effective cover
    COVER_CROUCH_TO_STAND, // hidden in crouched, stand to shoot
    COVER_CRAWL_TO_CROUCH, // hidden in crawling, must crouch to shoot
    COVER_CRAWL_TO_STAND,  // hidden in crawling, must stand to shoot
    COVER_FULL_WALL,       // hidden always
    COVER_FULL_STAND,      // hidden if standing
    COVER_FULL_CROUCH,     // hidden if crouched
    COVER_FULL_CRAWL,      // hidden if crawling
  };
  static const char *get_cover_name(ECover cover)
  {
    switch (cover)
    {
      case CoversTracer::NO_START: return "NO_START";
      case CoversTracer::NO_DIRECTION: return "NO_DIRECTION";
      case CoversTracer::NO_COVER_BAD_ATTACK: return "NO_COVER_BAD_ATTACK";
      case CoversTracer::NO_COVER_GOOD_ATTACK: return "NO_COVER_GOOD_ATTACK";
      case CoversTracer::INEFFECTIVE_COVER: return "INEFFECTIVE_COVER";
      case CoversTracer::COVER_CROUCH_TO_STAND: return "COVER_CROUCH_TO_STAND";
      case CoversTracer::COVER_CRAWL_TO_CROUCH: return "COVER_CRAWL_TO_CROUCH";
      case CoversTracer::COVER_CRAWL_TO_STAND: return "COVER_CRAWL_TO_STAND";
      case CoversTracer::COVER_FULL_WALL: return "COVER_FULL_WALL";
      case CoversTracer::COVER_FULL_STAND: return "COVER_FULL_STAND";
      case CoversTracer::COVER_FULL_CROUCH: return "COVER_FULL_CROUCH";
      case CoversTracer::COVER_FULL_CRAWL: return "COVER_FULL_CRAWL";
    }
    return "???";
  }

  static bool is_cover(ECover cover) { return cover >= COVER_CROUCH_TO_STAND; }
  static bool is_cover_for_crawl(ECover cover) { return cover >= COVER_CROUCH_TO_STAND; }
  static bool is_cover_for_stand(ECover cover) { return cover == COVER_FULL_WALL || cover == COVER_FULL_STAND; }
  static bool is_cover_for_crouch(ECover cover)
  {
    switch (cover)
    {
      case CoversTracer::COVER_CROUCH_TO_STAND:
      case CoversTracer::COVER_FULL_WALL:
      case CoversTracer::COVER_FULL_STAND:
      case CoversTracer::COVER_FULL_CROUCH: return true;
      default: break;
    }
    return false;
  }
  static bool is_good_for_stand_attack(ECover cover, const Range &cover_aim1)
  {
    switch (cover)
    {
      case CoversTracer::NO_COVER_GOOD_ATTACK: return true;
      case CoversTracer::COVER_CROUCH_TO_STAND:
      case CoversTracer::COVER_CRAWL_TO_CROUCH:
      case CoversTracer::COVER_CRAWL_TO_STAND: return cover_aim1.inRange(10, 0);
      default: break;
    }
    return false;
  }

  ECover determine_cover(const State &state, bool at_max_dist) const
  {
    if (!state.cover)
    {
      if (state.shot1 >= goodShootSum)
        return NO_COVER_GOOD_ATTACK;
      return NO_COVER_BAD_ATTACK;
    }

    int shot1 = state.shot1;
    int shot2 = state.shot2;
    int shot3 = state.shot3;
    if (at_max_dist)
    {
      if (state.aim1[0].inRange(100, 0))
        shot1 += goodShootSum;
      if (state.aim2[0].inRange(100, 0))
        shot2 += goodShootSum;
    }

    if (shot3 <= maxSeenCrawl)
    {
      if (shot2 <= maxSeenCrouch)
      {
        if (shot1 - state.over1 / 2 >= goodShootSum)
          return COVER_CROUCH_TO_STAND;
      }
      else
      {
        if (shot2 - state.over2 / 2 >= goodShootSum)
          return COVER_CRAWL_TO_CROUCH;
      }

      if (shot1 - state.over1 / 2 >= goodShootSum)
        return COVER_CRAWL_TO_STAND;
    }

    if (state.wall)
      return COVER_FULL_WALL;

    if (shot3 <= 0)
    {
      if (shot2 <= 0)
      {
        const bool standFullPossible = (lastCoverAim1.isEmpty() || !lastCoverAim1.inRangeBot(10, 1));
        if (shot1 <= 0 && standFullPossible)
          return COVER_FULL_STAND;
        return COVER_FULL_CROUCH;
      }
      return COVER_FULL_CRAWL;
    }

    return INEFFECTIVE_COVER;
  }

  Range lastCoverAim1;
  Range lastCoverAim2;
  Range lastCoverAim3;
  int lastCoverDist = 0;
  int lastCoverShot1 = 0;
  int lastCoverShot2 = 0;
  int lastCoverShot3 = 0;
  int lastShot1 = 0;
  int lastShot2 = 0;
  int lastShot3 = 0;
  int lastOver1 = 0;
  int lastOver2 = 0;

  bool is_span_aimable(int span_y, int span_h, int prev_y, int prev_h, int start_y1, int start_y2) const
  {
    const int span_yh = span_y + span_h;
    const int prev_yh = prev_y + prev_h;
    if (span_yh <= prev_y || prev_yh <= span_y)
      return false;
    if (span_y <= start_y1 && span_yh >= start_y1)
      return true;
    if (span_y <= start_y2 && span_yh >= start_y2)
      return true;
    if (span_yh - prev_y >= hh)
      return true;
    return false;
  }

  ECover trace_normalized(float dir_x, float dir_z, float &trace_dist, bool debug_this_trace)
  {
    const bool debugThisTrace = debug_this_trace;

    lastCoverAim1.setEmpty();
    lastCoverAim2.setEmpty();
    lastCoverAim3.setEmpty();
    lastCoverDist = 0;
    lastCoverShot1 = 0;
    lastCoverShot2 = 0;
    lastCoverShot3 = 0;
    lastShot1 = 0;
    lastShot2 = 0;
    lastShot3 = 0;
    lastOver1 = 0;
    lastOver2 = 0;

    if (startSpan < 0)
      return NO_START;

    if (debugThisTrace)
      covtrace("!!! START span=%d startY=%d startH=%d / dir_x=%f dir_z=%f trace_dist=%f", startSpan, startY, startH, dir_x, dir_z,
        trace_dist);

    const int fdist = (int)ceilf(trace_dist * ks);

    struct IXZ
    {
      int x, z;
      void operator+=(const IXZ &t)
      {
        x += t.x;
        z += t.z;
      }
    };
    int xdist, zdist, xdadd, zdadd, xdir, zdir, xcnt, zcnt;
    IXZ xpos, zpos, xstep, zstep;

    const float EPS = 0.00001f;

    if (abs(dir_x) < EPS)
      xcnt = 0;
    else
    {
      const bool neg_x = dir_x < 0.0f;
      const int next_x = (start.x & ~0xFFFFUL) + (neg_x ? -1 : 0x10000);
      const int offs_x = next_x - start.x;

      const float div_dx = 1.0f / dir_x;
      const float kslope = dir_z * div_dx;

      xpos.x = next_x;
      xpos.z = start.z + (int)floorf(offs_x * kslope);
      xstep.x = neg_x ? -0x10000 : 0x10000;
      xstep.z = (int)floorf(xstep.x * kslope);
      xdist = (int)floorf(div_dx * offs_x);
      xdadd = (int)floorf(fabsf(div_dx) * 65536.0f);
      xdir = neg_x ? dxdir[0] : dxdir[1];
      xcnt = 1 + ((fdist > xdist) ? ((fdist - xdist + 32768) / xdadd) : 0);
    }

    if (abs(dir_z) < EPS)
    {
      zcnt = 0;
      if (!xcnt)
      {
        trace_dist = 0.0f;
        return NO_DIRECTION;
      }
    }
    else
    {
      const bool neg_z = dir_z < 0.0f;
      const int next_z = (start.z & ~0xFFFFUL) + (neg_z ? -1 : 0x10000);
      const int offs_z = next_z - start.z;

      const float div_dz = 1.0f / dir_z;
      const float kslope = dir_x * div_dz;

      zpos.z = next_z;
      zpos.x = start.x + (int)floorf(offs_z * kslope);
      zstep.z = neg_z ? -0x10000 : 0x10000;
      zstep.x = (int)floorf(zstep.z * kslope);
      zdist = (int)floorf(div_dz * offs_z);
      zdadd = (int)floorf(fabsf(div_dz) * 65536.0f);
      zdir = neg_z ? dzdir[0] : dzdir[1];
      zcnt = 1 + ((fdist > zdist) ? ((fdist - zdist + 32768) / zdadd) : 0);
    }

    if (debugThisTrace)
    {
      covtrace("!!! TRACE xcnt=%d xdir=%d xdist=%d(%f) xdadd=%d(%f) xpos.x=%d(%f) xpos.z=%d(%f) xstep.x=%d(%f) xstep.z=%d(%f)",
        xcnt,                                                                                                                  //-V614
        xdir, xdist, xdist / 65536.0f, xdadd, xdadd / 65536.0f, xpos.x, xpos.x / 65536.0f, xpos.z, xpos.z / 65536.0f, xstep.x, //-V614
        xstep.x / 65536.0f, xstep.z, xstep.z / 65536.0f);                                                                      //-V614

      covtrace("!!! TRACE zcnt=%d zdir=%d zdist=%d(%f) zdadd=%d(%f) zpos.x=%d(%f) zpos.z=%d(%f) zstep.x=%d(%f) zstep.z=%d(%f)",
        zcnt,                                                                                                                  //-V614
        zdir, zdist, zdist / 65536.0f, zdadd, zdadd / 65536.0f, zpos.x, zpos.x / 65536.0f, zpos.z, zpos.z / 65536.0f, zstep.x, //-V614
        zstep.x / 65536.0f, zstep.z, zstep.z / 65536.0f);                                                                      //-V614
    }

    int highY = 0;
    int highH = 0;
    int lastY = startY;
    int lastH = startH;

    int stateIdx = 0;
    int px, pz, dir, dist;

    for (;;)
    {
      if (xcnt && (!zcnt || xdist < zdist))
      {
        px = xpos.x >> 16;
        pz = xpos.z >> 16;
        dir = xdir;
        dist = xdist;
        xpos += xstep;
        xdist += xdadd;
        --xcnt;

        if (debugThisTrace)
          covtrace("!!! EARLY X-NEXT px=%d pz=%d dir=%d", px, pz, dir);
      }
      else
      {
        px = zpos.x >> 16;
        pz = zpos.z >> 16;
        dir = zdir;
        dist = zdist;
        zpos += zstep;
        zdist += zdadd;
        --zcnt;

        if (debugThisTrace)
          covtrace("!!! EARLY Z-NEXT px=%d pz=%d dir=%d", px, pz, dir);
      }

      State &state = states[stateIdx];
      if (stateIdx >= ncached || state.px != px || state.pz != pz)
      {
        if (debugThisTrace)
          covtrace("!!! EARLY OFF-CACHED at %d px=%d pz=%d dir=%d", stateIdx, px, pz, dir);

        ncached = stateIdx;
        if (stateIdx > 0)
        {
          if (debugThisTrace)
            covtrace("!!! ~ CONTINUE from cached %d for %d to px=%d pz=%d dir=%d", stateIdx - 1, stateIdx, px, pz, dir);

          memcpy(&state, &states[stateIdx - 1], sizeof(State));

          highY = state.highY;
          highH = state.highH;
          lastY = state.spanY;
          lastH = state.spanH;

          if (debugThisTrace)
            covtrace("!!! ~ CONTINUE ~~~ lastY=%d lastH=%d highY=%d highH=%d", lastY, lastH, highY, highH);
        }
        else
        {
          if (debugThisTrace)
            covtrace("!!! ~ BEGIN from startSpan=%d at %d to px=%d pz=%d dir=%d", startSpan, stateIdx, px, pz, dir);

          memset(&state, 0, sizeof(State));
          state.span = startSpan;
        }
        break;
      }

      if (debugThisTrace)
        covtrace("!!! EARLY USE cached %d ~ px=%d pz=%d dir=%d", stateIdx, px, pz, dir);

      if (state.stop)
      {
        if (debugThisTrace)
          covtrace("!!! FINISHED CACHED STOP cover=%d shot1=%d shot2=%d shot3=%d ~ px=%d pz=%d dir=%d", state.cover ? 1 : 0,
            state.shot1, state.shot2, state.shot3, px, pz, dir);

        lastShot1 = state.shot1;
        lastShot2 = state.shot2;
        lastShot3 = state.shot3;
        lastOver1 = state.over1;
        lastOver2 = state.over2;

        trace_dist = -1.0f;
        return determine_cover(state, false);
      }
      if (!xcnt && !zcnt)
      {
        if (debugThisTrace)
          covtrace("!!! FINISHED CACHED ZERO cover=%d shot1=%d shot2=%d shot3=%d ~ px=%d pz=%d dir=%d", state.cover ? 1 : 0,
            state.shot1, state.shot2, state.shot3, px, pz, dir);

        lastShot1 = state.shot1;
        lastShot2 = state.shot2;
        lastShot3 = state.shot3;
        lastOver1 = state.over1;
        lastOver2 = state.over2;

        trace_dist = -1.0f;
        return determine_cover(state, true);
      }

      ++stateIdx;
    }

    const int w = chf->width;
    const int h = chf->height;

    const int startY1 = startY + sh1;
    const int startY2 = startY + sh2;
    const int startY3 = startY + sh3;

    if (debugThisTrace)
      covtrace("!!! IN-TRACING w=%d h=%d", w, h);

    const int SKY_HIGH = 1 << 13; // 8192

    const bool canStand = startH >= eh1;
    for (;;)
    {
      if ((unsigned)px >= w || (unsigned)pz >= h)
      {
        xcnt = 0;
        zcnt = 0; // force at_max_dist

        if (debugThisTrace)
          covtrace("!!! OUT px=%d pz=%d dir=%d w=%d h=%d", px, pz, dir, w, h);
        break;
      }

      State &state = states[stateIdx];

      state.px = px;
      state.pz = pz;

      const rcSpan *solidSpan = nullptr;
      const auto &cell = chf->cells[px + pz * w];
      int nspan = (state.span != 0xFFFFFF) ? rcGetCon(chf->spans[state.span], dir) : RC_NOT_CONNECTED;
      if (nspan != RC_NOT_CONNECTED && cell.index + nspan >= chf->spanCount) // Note: bug in recast?
        nspan = RC_NOT_CONNECTED;
      int lastYH = lastY + lastH;
      int nY, nH;

      if (debugThisTrace)
        covtrace("!!! px=%d pz=%d dir=%d wasSpan=%d nspan=%d", px, pz, dir, (state.span == 0xFFFFFF) ? -1 : state.span,
          (nspan == RC_NOT_CONNECTED) ? -1 : nspan);

      if (nspan != RC_NOT_CONNECTED)
      {
        state.span = cell.index + nspan;
        const auto &span = chf->spans[state.span];
        nY = span.y;
        nH = span.h;

        if (state.span > cell.index)
        {
          const auto &lowerSpan = chf->spans[state.span - 1];
          if ((int)lowerSpan.y + (int)lowerSpan.h - lastY >= hh) // TODO could it happen at all?
          {
            --state.span;
            nY = lowerSpan.y;
            nH = lowerSpan.h;

            if (debugThisTrace)
              covtrace("!!! TOOK lower neightbour span=%d (%d %d) instead of %d (%d %d) lastY=%d lastH=%d hh=%d", state.span, nY, nH,
                state.span + 1, span.y, span.h, lastY, lastH, hh);
          }
        }

        if (nY < lastYH && lastY < nY + nH)
        {
          if (debugThisTrace)
            covtrace("!!! NEXT CHF neightbour SPAN %d (cell index=%d count=%d) nY=%d nH=%d lastY=%d lastH=%d", state.span, cell.index,
              cell.count, nY, nH, lastY, lastH);
        }
        else
        {
          nspan = RC_NOT_CONNECTED;

          if (debugThisTrace)
            covtrace("!!! BAD CHF neightbour SPAN %d (cell index=%d count=%d) nY=%d nH=%d lastY=%d lastH=%d", state.span, cell.index,
              cell.count, nY, nH, lastY, lastH);
        }
      }
      if (nspan == RC_NOT_CONNECTED)
      {
        bool found = false;

        int skip1 = 0;
        const rcSpan *skip2 = nullptr;

        if (state.span == 0xFFFFFF)
        {
          if (debugThisTrace)
            covtrace("!!! NOT_CONN CHF ?CELL px=%d pz=%d index=%d count=%d", px, pz, cell.index, cell.count);

          int idx = cell.index;
          int cnt = cell.count;
          while (cnt-- > 0)
          {
            const auto &span = chf->spans[idx++];
            if ((int)span.y >= lastYH)
            {
              skip1 = idx - 1 - cell.index;
              break;
            }

            if (debugThisTrace)
              covtrace("!!! NOT_CONN CHF ?SPAN %d y=%d h=%d lastY=%d lastH=%d", idx - 1, span.y, span.h, lastY, lastH);

            if (is_span_aimable(span.y, span.h, lastY, lastH, startY1, startY2))
            {
              found = true;
              state.span = idx - 1;
              nY = span.y;
              nH = span.h;

              if (debugThisTrace)
                covtrace("!!! NEXT CHF scanned SPAN %d (cell index=%d, count=%d) nY=%d nH=%d lastY=%d lastH=%d", state.span,
                  cell.index, cell.count, nY, nH, lastY, lastH);
              break;
            }
          }
        }

        if (!found)
        {
          const rcSpan *span = solid->spans[px + pz * w];
          while (span)
          {
            const int span_y = span->smax;
            if (span_y >= lastYH)
            {
              if (debugThisTrace)
              {
                const int span_h = span->next ? (span->next->smin - span_y) : 255;
                covtrace("!!! NOT_CONN SOLID !SPAN y=%d >= lastYH=%d h=%d lastY=%d lastH=%d", span_y, lastYH, span_h, lastY, lastH);
              }
              skip2 = span;
              break;
            }
            const int span_h = span->next ? (span->next->smin - span_y) : 255;

            if (debugThisTrace)
              covtrace("!!! NOT_CONN SOLID ?SPAN y=%d h=%d lastY=%d lastH=%d startY1=%d startY2=%d", span_y, span_h, lastY, lastH,
                startY1, startY2);

            if (is_span_aimable(span_y, span_h, lastY, lastH, startY1, startY2))
            {
              found = true;
              solidSpan = span;
              state.span = 0xFFFFFF;
              nY = span_y;
              nH = span_h;

              if (debugThisTrace)
                covtrace("!!! NEXT SOLID scanned SPAN nY=%d nH=%d lastY=%d lastH=%d", nY, nH, lastY, lastH);
              break;
            }
            span = span->next;
          }

          if (!found && highH > 0)
          {
            const int highYH = highY + highH;

            if (state.span == 0xFFFFFF)
            {
              if (debugThisTrace)
                covtrace("!!! NOT_CONN CHF(high) ?CELL px=%d pz=%d index=%d count=%d", px, pz, cell.index, cell.count);

              int idx = cell.index + skip1;
              int cnt = cell.count - skip1;
              while (cnt-- > 0)
              {
                const auto &span = chf->spans[idx++];
                if ((int)span.y >= highYH)
                  break;

                if (debugThisTrace)
                  covtrace("!!! NOT_CONN CHF(high) ?SPAN %d y=%d h=%d lastY=%d lastH=%d", idx - 1, span.y, span.h, lastY, lastH);

                if (is_span_aimable(span.y, span.h, highY, highH, startY1, startY2))
                {
                  found = true;
                  state.span = idx - 1;
                  nY = span.y;
                  nH = span.h;

                  lastY = highY;
                  lastH = highH;
                  lastYH = lastY + lastH;
                  highH = 0;

                  state.lowerRanges();

                  if (debugThisTrace)
                    covtrace("!!! NEXT CHF high-to-last SPAN %d nY=%d nH=%d lastY=%d lastH=%d", idx - 1, nY, nH, lastY, lastH);
                  break;
                }
              }
            }

            if (!found)
            {
              const rcSpan *span = skip2 ? skip2 : solid->spans[px + pz * w];
              while (span)
              {
                const int span_y = span->smax;
                if (span_y >= highYH)
                  break;
                const int span_h = span->next ? (span->next->smin - span_y) : 255;

                if (debugThisTrace)
                  covtrace("!!! NOT_CONN SOLID(high) ?SPAN nY=%d nH=%d lastY=%d lastH=%d", span_y, span_h, lastY, lastH);

                if (is_span_aimable(span_y, span_h, highY, highH, startY1, startY2))
                {
                  found = true;
                  solidSpan = span;
                  state.span = 0xFFFFFF;
                  nY = span_y;
                  nH = span_h;

                  lastY = highY;
                  lastH = highH;
                  lastYH = lastY + lastH;
                  highH = 0;

                  state.lowerRanges();

                  if (debugThisTrace)
                    covtrace("!!! NEXT SOLID high-to-last SPAN nY=%d nH=%d lastY=%d lastH=%d", nY, nH, lastY, lastH);
                  break;
                }
                span = span->next;
              }
            }
          }

          if (!found)
          {
            if (dist <= covDist && solid->spans[px + pz * w] != nullptr)
            {
              lastCoverAim1.setEmpty();
              lastCoverAim2.setEmpty();
              lastCoverAim3.setEmpty();
              lastCoverDist = dist;
              lastCoverShot1 = state.shot1;
              lastCoverShot2 = state.shot2;
              lastCoverShot3 = state.shot3;

              state.cover = true;
              state.wall = true;
            }

            state.stop = true;
            nY = nH = 0;

            if (debugThisTrace)
              covtrace("!!! NOT_CONNECTED WALL(stop) at (%f,%f,%f)", chf->bmin[0] + (px << 16) / ks, chf->bmin[1] + lastY / kh,
                chf->bmin[2] + (pz << 16) / ks);
          }
        }
      }

      const int atDist = dist >> 16;
      const int atDist3 = (dist + bodyLen) >> 16;
      G_ASSERT(atDist <= 255);
      G_ASSERT(atDist3 <= 255);

      int hY = 0;
      int hH = 0;

      if (!state.stop)
      {
        if (!solidSpan)
        {
          int cidx = state.span - cell.index + 1;
          if (cidx < cell.count)
          {
            const auto &upperSpan = chf->spans[state.span + 1];
            hY = upperSpan.y;
            hH = upperSpan.h;

            // if (debugThisTrace)
            //   covtrace("!!! HIGH CHF: hY=%d hH=%d highY=%d highH=%d", hY, hH, highY, highH);

            const bool notSeenSpanUnderHigh =
              highH > 0 && hY + hH <= highY && !state.inRangesLow(atDist, atDist3, hY - startY1, hY - startY2, hY - startY3, hH);

            if ((hH < hh || notSeenSpanUnderHigh) && ++cidx < cell.count)
            {
              if (debugThisTrace)
              {
                debug_print_ranges(state);
                covtrace("!!! HIGHER CHF: was hY=%d hH=%d hh=%d highH=%d notSeenUnder=%d", hY, hH, hh, highH,
                  notSeenSpanUnderHigh ? 1 : 0);
              }

              const auto &upperSpan2 = chf->spans[state.span + 2];
              hY = upperSpan2.y;
              hH = upperSpan2.h;

              if (debugThisTrace)
                covtrace("!!! HIGHER CHF: new hY=%d hH=%d", hY, hH);
            }
          }
        }
        else
        {
          const rcSpan *upperSpan = solidSpan->next;
          if (upperSpan)
          {
            hY = upperSpan->smax;
            hH = upperSpan->next ? (upperSpan->next->smin - hY) : 255;

            // if (debugThisTrace)
            //   covtrace("!!! HIGH SOLID: hY=%d hH=%d highY=%d highH=%d", hY, hH, highY, highH);

            const bool notSeenSpanUnderHigh =
              highH > 0 && hY + hH <= highY && !state.inRangesLow(atDist, atDist3, hY - startY1, hY - startY2, hY - startY3, hH);

            if ((hH < hh || notSeenSpanUnderHigh) && upperSpan->next)
            {
              if (debugThisTrace)
              {
                debug_print_ranges(state);
                covtrace("!!! HIGHER SOLID: was hY=%d hH=%d hh=%d highH=%d notSeenUnder=%d", hY, hH, hh, highH,
                  notSeenSpanUnderHigh ? 1 : 0);
              }

              upperSpan = upperSpan->next;
              hY = upperSpan->smax;
              hH = upperSpan->next ? (upperSpan->next->smin - hY) : 255;

              if (debugThisTrace)
                covtrace("!!! HIGHER SOLID: new hY=%d hH=%d", hY, hH);
            }
          }
        }

        // if (debugThisTrace)
        //   covtrace("!!! TRY HIGH: hY=%d hH=%d highY=%d highH=%d", hY, hH, highY, highH);

        if (hY + hH >= SKY_HIGH)
        {
          hH = SKY_HIGH - hY;
          if (hH < eh1)
          {
            hY = 0;
            hH = 0;
          }
        }

        if (hY >= SKY_HIGH || hY + hH >= SKY_HIGH)
        {
          // if (debugThisTrace)
          //   covtrace("!!! TRY HIGH: too high, reset hY=%d hH=%d", hY, hH);

          hY = 0;
          hH = 0;
        }

        if (hH > 0)
        {
          const int hYH = hY + hH;
          if (!(lastYH - hY >= sh2) && !(highH > 0 && hY < highY + highH && highY < hYH))
          {
            // if (debugThisTrace)
            //   covtrace("!!! TRY HIGH: clamped! lastY=%d lastH=%d lastYH=%d highY=%d highH=%d hY=%d hH=%d hYH=%d",
            //     lastY, lastH, lastYH, highY, highH, hY, hH, hYH);

            hY = 0;
            hH = 0;
          }
        }

        if (highH > 0 || hH > 0)
        {
          if (debugThisTrace)
            covtrace("!!! HIGH %s: highY=%d highH=%d hY=%d hH=%d", (hH > 0) ? (highH <= 0 ? "started" : "continued") : "ended", highY,
              highH, hY, hH);
        }
      }

      state.highY = hY;
      state.highH = hH;
      state.spanY = nY;
      state.spanH = nH;

      if (!state.stop)
      {
        // cont1: Ln      highH <= 0 hH <= 0           => 2 pts
        // split: Lhn     highH <= 0 hH >  0           => 4 pts
        // join1: HLn     highH >  0 hH <= 0           => 4 pts
        // join2: Hh/HLn  highH >  0 hH >  0 && Hn     => 6 pts
        // cont2: Hh/Ln   highH >  0 hH >  0 && not Hn => 4 pts

        const int nYH = nY + nH;
        const int hYH = hY + hH;

        if (highH <= 0)
        {
          if (hH <= 0) // cont1
          {
            state.clipTopEdge(atDist, nYH, false, startY1, startY2, startY3, atDist3);
            state.clipBotEdge(atDist, nY, false, startY1, startY2, startY3, atDist3);

            if (debugThisTrace)
            {
              covtrace("!!! RANGES CONT1: lastY=%d lastH=%d nY=%d nH=%d", lastY, lastH, nY, nH);
              debug_print_ranges(state);
            }
          }
          else // split
          {
            state.clipTopEdge(atDist, hYH, false, startY1, startY2, startY3, atDist3);
            state.clipBotEdge(atDist, nY, false, startY1, startY2, startY3, atDist3);
            state.splitRanges(atDist, hY, nYH, startY1, startY2, startY3, atDist3);

            if (debugThisTrace)
            {
              covtrace("!!! RANGES SPLIT: lastY=%d lastH=%d hY=%d hH=%d nY=%d nH=%d", lastY, lastH, hY, hH, nY, nH);
              debug_print_ranges(state);
            }
          }
        }
        else
        {
          const int highYH = highY + highH;
          if (hH <= 0) // join1
          {
            state.lowerRanges();
            state.clipTopEdge(atDist, nYH, false, startY1, startY2, startY3, atDist3);
            state.clipBotEdge(atDist, nY, false, startY1, startY2, startY3, atDist3);

            if (debugThisTrace)
            {
              covtrace("!!! RANGES JOIN1: highY=%d highH=%d lastY=%d lastH=%d nY=%d nH=%d", highY, highH, lastY, lastH, nY, nH);
              debug_print_ranges(state);
            }
          }
          else
          {
            if (highY < nYH && nY < highYH) // join2
            {
              state.lowerRanges();
              state.clipTopEdge(atDist, hYH, false, startY1, startY2, startY3, atDist3);
              state.clipBotEdge(atDist, nY, false, startY1, startY2, startY3, atDist3);
              state.splitRanges(atDist, hY, nYH, startY1, startY2, startY3, atDist3);

              if (debugThisTrace)
              {
                covtrace("!!! RANGES JOIN2: highY=%d highH=%d lastY=%d lastH=%d hY=%d hH=%d nY=%d nH=%d", highY, highH, lastY, lastH,
                  hY, hH, nY, nH);
                debug_print_ranges(state);
              }
            }
            else // cont2
            {
              state.clipTopEdge(atDist, hYH, true, startY1, startY2, startY3, atDist3);
              state.clipBotEdge(atDist, hY, true, startY1, startY2, startY3, atDist3);
              state.clipTopEdge(atDist, nYH, false, startY1, startY2, startY3, atDist3);
              state.clipBotEdge(atDist, nY, false, startY1, startY2, startY3, atDist3);

              if (debugThisTrace)
              {
                covtrace("!!! RANGES CONT2: highY=%d highH=%d lastY=%d lastH=%d hY=%d hH=%d nY=%d nH=%d", highY, highH, lastY, lastH,
                  hY, hH, nY, nH);
                debug_print_ranges(state);
              }
            }
          }
        }

        if (Ranges::isEmpty(state.aim1, state.aim1n) && Ranges::isEmpty(state.aim2, state.aim2n))
        {
          state.stop = true;

          if (debugThisTrace)
            covtrace("!!! NO AIMING RANGE(stop) at (%f,%f,%f)", chf->bmin[0] + (px << 16) / ks, chf->bmin[1] + lastY / kh,
              chf->bmin[2] + (pz << 16) / ks);
        }

        if (dist <= covDist && (state.aim2[0].dybot > 0 || state.aim3[0].dybot > 0))
        {
          if (!state.cover || (state.aim2[0].dybot > lastCoverAim2.dybot) || (state.aim3[0].dybot > lastCoverAim3.dybot))
          {
            lastCoverAim1 = state.aim1[0];
            lastCoverAim2 = state.aim2[0];
            lastCoverAim3 = state.aim3[0];
            lastCoverDist = dist;
            lastCoverShot1 = state.shot1;
            lastCoverShot2 = state.shot2;
            lastCoverShot3 = state.shot3;

            state.cover = true;
          }
        }
      }

      if (!state.stop && dist >= shotDist)
      {
        if (debugThisTrace)
          covtrace("!!! WAS SHOTS: shot1=%d shot2=%d shot3=%d over1=%d over2=%d max2=%d max3=%d", state.shot1, state.shot2,
            state.shot3, state.over1, state.over2, maxSeenCrouch, maxSeenCrawl);

        for (int floor = 0; floor < 2; ++floor)
        {
          const int enemyH = floor == 0 ? nH : hH;
          if (enemyH >= eh3)
          {
            const bool isHigh = (floor > 0);
            const int enemyY = (floor == 0) ? nY : hY;
            if (enemyY >= startY1 && dist < nearDist)
              continue;

            const int aim1n = state.aim1n;
            const int aim2n = state.aim2n;
            const int aim3n = state.aim3n;

            int testY = enemyY + sh3;
            if (canStand && Ranges::inRanges(atDist, testY - startY1, isHigh, state.aim1, aim1n))
              state.applyShot1(testY - startY1, hh);
            if (Ranges::inRanges(atDist, testY - startY2, isHigh, state.aim2, aim2n))
              state.applyShot2(testY - startY2, hh);
            if (Ranges::inRanges(atDist3, testY - startY3, isHigh, state.aim3, aim3n))
              state.applyShot3();

            if (enemyH >= eh2)
            {
              testY = enemyY + sh2;
              if (canStand && Ranges::inRanges(atDist, testY - startY1, isHigh, state.aim1, aim1n))
                state.applyShot1(testY - startY1, hh);
              if (Ranges::inRanges(atDist, testY - startY2, isHigh, state.aim2, aim2n))
                state.applyShot2(testY - startY2, hh);
              if (Ranges::inRanges(atDist3, testY - startY3, isHigh, state.aim3, aim3n))
                state.applyShot3();

              if (enemyH >= eh1)
              {
                testY = enemyY + sh1;
                if (canStand && Ranges::inRanges(atDist, testY - startY1, isHigh, state.aim1, aim1n))
                  state.applyShot1(testY - startY1, hh);
                if (Ranges::inRanges(atDist, testY - startY2, isHigh, state.aim2, aim2n))
                  state.applyShot2(testY - startY2, hh);
                if (Ranges::inRanges(atDist3, testY - startY3, isHigh, state.aim3, aim3n))
                  state.applyShot3();
              }
            }
          }

          if (hH <= 0)
            break;
        }

        if (debugThisTrace)
          covtrace("!!! NEW SHOTS: shot1=%d shot2=%d shot3=%d over1=%d over2=%d max2=%d max3=%d", state.shot1, state.shot2,
            state.shot3, state.over1, state.over2, maxSeenCrouch, maxSeenCrawl);
      }

      if (stateIdx < CACHE_SIZE)
        ncached = stateIdx + 1;

      if (state.stop)
      {
        if (debugThisTrace)
          covtrace("!!! STOP px=%d pz=%d dir=%d", px, pz, dir);
        break;
      }
      if (!xcnt && !zcnt)
      {
        if (debugThisTrace)
          covtrace("!!! ZERO after px=%d pz=%d dir=%d", px, pz, dir);
        break;
      }

      if (xcnt && (!zcnt || xdist < zdist))
      {
        px = xpos.x >> 16;
        pz = xpos.z >> 16;
        dir = xdir;
        dist = xdist;
        xpos += xstep;
        xdist += xdadd;
        --xcnt;

        if (debugThisTrace)
          covtrace("!!! X-NEXT px=%d pz=%d dir=%d", px, pz, dir);
      }
      else
      {
        px = zpos.x >> 16;
        pz = zpos.z >> 16;
        dir = zdir;
        dist = zdist;
        zpos += zstep;
        zdist += zdadd;
        --zcnt;

        if (debugThisTrace)
          covtrace("!!! Z-NEXT px=%d pz=%d dir=%d", px, pz, dir);
      }

      if (stateIdx < CACHE_SIZE)
      {
        ++stateIdx;
        memcpy(&states[stateIdx], &state, sizeof(State));

        if (debugThisTrace)
          covtrace("!!! COPY-&-CACHE %d to %d", stateIdx - 1, stateIdx);
      }

      highY = hY;
      highH = hH;
      lastY = nY;
      lastH = nH;
    }

    const State &state = states[stateIdx];

    if (debugThisTrace)
    {
      covtrace(
        "!!! FINISHED TRACING cover=%d shot1=%d shot2=%d shot3=%d over1=%d over2=%d good=%d max2=%d max3=%d ~ px=%d pz=%d dir=%d",
        state.cover ? 1 : 0, state.shot1, state.shot2, state.shot3, state.over1, state.over2, goodShootSum, maxSeenCrouch,
        maxSeenCrawl, px, pz, dir);
    }

    lastShot1 = state.shot1;
    lastShot2 = state.shot2;
    lastShot3 = state.shot3;
    lastOver1 = state.over1;
    lastOver2 = state.over2;

    trace_dist = lastCoverDist / ks;
    return determine_cover(state, !xcnt && !zcnt);
  }
};


static void build_covers_v2_reduce(Tab<covers::Cover> &out_covers, CoversArr &covers_arr, const BBox3 &tile_box,
  const CoversParams &cov_params)
{
  G_UNUSED(cov_params);

  const float MAX_COVERS_PER_X_M_SQ = 20.0f;
  const float SQR_X_M_SQ = 32.0f * 32.0f;

  const Point3 tileBoxSize = tile_box.width();
  const float tileBoxSq = tileBoxSize.x * tileBoxSize.z;
  if (tileBoxSq <= 0.0f)
    return;
  const float tileBoxSqXMSq = tileBoxSq / SQR_X_M_SQ;
  const float countPerMSq = (float)out_covers.size() / tileBoxSqXMSq;
  if (countPerMSq < MAX_COVERS_PER_X_M_SQ)
    return;

  int totalCovers = 0;
  eastl::vector<int> idxs;
  for (int posIdx = 0; posIdx < (int)covers_arr.arr.size(); ++posIdx)
  {
    auto &coverPos = covers_arr.arr[posIdx];
    if (coverPos.removed || coverPos.coverIdx < 0)
      continue;
    if (coverPos.coverScore <= 1.0f)
    {
      ++totalCovers;
      idxs.push_back(posIdx);
    }
  }

  const int leaveCovers = (int)ceilf(tileBoxSqXMSq * MAX_COVERS_PER_X_M_SQ);
  const int lesserCovers = (int)idxs.size();

  if (totalCovers <= leaveCovers)
    return;
  if (lesserCovers <= 0)
    return;

  eastl::sort(idxs.begin(), idxs.end(), [&](auto &a, auto &b) { return covers_arr.arr[a].coverScore > covers_arr.arr[b].coverScore; });

  const int removeCovers = min(lesserCovers, totalCovers - leaveCovers);
  for (int i = 0; i < removeCovers; ++i)
  {
    const int posIdx = idxs[lesserCovers - 1 - i];
    auto &coverPos = covers_arr.arr[posIdx];
    auto &cover = out_covers[coverPos.coverIdx];

    coverPos.coverIdx = -1; // detach from cover
    cover.hLeft = -1.0f;    // cover to be removed
  }

  for (int i = 0; i < (int)out_covers.size(); ++i)
    if (out_covers[i].hLeft < 0.0f)
    {
      out_covers[i] = out_covers.back();
      out_covers.pop_back();
      --i;
    }

  covers_arr.arr.clear(); // not valid anymore (.coverIdx indexes may be broken)
}


static void build_covers_v2_join(Tab<covers::Cover> &out_covers, CoversArr &covers_arr, const CoversParams &cov_params)
{
  G_UNUSED(cov_params);

  // Join rows of alike covers
  const float COVERS_JOIN_DIR_HALF_ANGLE_DEG = 30.0f;
  const float COVERS_JOIN_DIR_FULL_ANGLE_DEG = 45.0f;
  const float COVERS_JOIN_DIR_SHOT_ANGLE_DEG = 20.0f;
  const float COVERS_JOIN_MAX_GROUND_DH_M = 0.30f;
  const float COVERS_JOIN_MAX_HEIGHT_DIFF = 0.20f;
  const float COVERS_JOIN_MAX_DIST_BETWEEN = COVERS_STANDARD_WIDTH * 1.25f;
  const float COVERS_JOIN_MAX_ALIGN_FORWARD = 0.25f;
  const float COVERS_JOIN_MAX_ALIGN_BACKWARD = 0.75f;
  const float COVERS_JOIN_MAX_COVER_LENGTH = COVERS_STANDARD_WIDTH * 8.0f;

  const float maxAngleHalfCos = cosf(DegToRad(COVERS_JOIN_DIR_HALF_ANGLE_DEG));
  const float maxAngleFullCos = cosf(DegToRad(COVERS_JOIN_DIR_FULL_ANGLE_DEG));
  const float maxAngleShotCos = cosf(DegToRad(COVERS_JOIN_DIR_SHOT_ANGLE_DEG));
  const float maxLengthSq = sqr(COVERS_JOIN_MAX_COVER_LENGTH);

  covers_arr.rebuildIndex();

  eastl::vector<int> idxs;
  for (int posIdx = 0; posIdx < (int)covers_arr.arr.size(); ++posIdx)
  {
    auto &coverPos = covers_arr.arr[posIdx];
    if (coverPos.removed || coverPos.coverIdx < 0)
      continue;

    auto &cover = out_covers[coverPos.coverIdx];
    const bool isHalfCover =
      !cover.hasLeftPos && !cover.hasRightPos && cover.shootLeft != ZERO<Point3>() && cover.shootRight != ZERO<Point3>();

    const float maxAngleCos = isHalfCover ? maxAngleHalfCos : maxAngleFullCos;

    bool joined = false;
    bool joining = true;
    while (joining)
    {
      joining = false;

      const Point3 dirRight(cover.dir.z, 0.0f, -cover.dir.x);
      const Point3 offsRight = dirRight * COVERS_STANDARD_WIDTH * 0.5f;
      const Point3 &ptLeft = cover.groundLeft - cover.dir * COVERS_WALL_OFFSET + offsRight;
      const Point3 &ptRight = cover.groundRight - cover.dir * COVERS_WALL_OFFSET - offsRight;
      const Point3 &ptCenter = (ptLeft + ptRight) * 0.5f;
      const float minHeight = min(cover.hLeft, cover.hRight);

      if (lengthSq(cover.groundLeft - cover.groundRight) >= maxLengthSq)
        break;

      idxs.clear();
      covers_arr.gatherAround(idxs, ptLeft, COVERS_STANDARD_WIDTH * 0.9f, minHeight);
      covers_arr.gatherAround(idxs, ptRight, COVERS_STANDARD_WIDTH * 0.9f, minHeight);

      int bestL = -1;
      float distL = 0.0f;
      float forwL = 0.0f;
      Point3 newDirL;
      Point3 posL;
      int bestR = -1;
      float distR = 0.0f;
      float forwR = 0.0f;
      Point3 newDirR;
      Point3 posR;
      for (int i = 0; i < (int)idxs.size(); ++i)
      {
        const int posNearIdx = idxs[i];
        auto &coverPos2 = covers_arr.arr[posNearIdx];
        if (coverPos2.removed || coverPos2.coverIdx < 0)
          continue;

        auto &cover2 = out_covers[coverPos2.coverIdx];
        const bool isHalfCover2 =
          !cover2.hasLeftPos && !cover2.hasRightPos && cover2.shootLeft != ZERO<Point3>() && cover2.shootRight != ZERO<Point3>();
        if (isHalfCover != isHalfCover2)
          continue;

        const float minHeight2 = min(cover2.hLeft, cover2.hRight);

        const float angleCos = dot(cover.dir, cover2.dir);
        if (angleCos < maxAngleCos)
          continue;
        if (abs(minHeight - minHeight2) > COVERS_JOIN_MAX_HEIGHT_DIFF)
          continue;

        Point3 newDir = cover.dir + cover2.dir;
        newDir.normalize();

        const Point3 pos2 = (cover2.groundLeft + cover2.groundRight) * 0.5f - cover2.dir * COVERS_WALL_OFFSET;

        const float distLeft = newDir.x * (ptLeft.z - pos2.z) + newDir.z * (pos2.x - ptLeft.x);
        const float distRight = newDir.x * (ptRight.z - pos2.z) + newDir.z * (pos2.x - ptRight.x);

        if (distLeft > -0.001f && distRight < 0.001f)
          continue;

        const bool isLeft = (distLeft <= -0.001f);
        const float absDist = isLeft ? -distLeft : distRight;
        if (absDist > COVERS_JOIN_MAX_DIST_BETWEEN)
          continue;

        if (isLeft && cover.hasLeftPos && cover2.hasLeftPos)
        {
          cover.hasLeftPos = false;
          cover.shootLeft = ZERO<Point3>();
        }
        if (!isLeft && cover.hasRightPos && cover2.hasRightPos)
        {
          cover.hasRightPos = false;
          cover.shootRight = ZERO<Point3>();
        }

        float distForw = newDir.x * (ptCenter.x - pos2.x) + newDir.z * (ptCenter.z - pos2.z);
        if (distForw < -COVERS_JOIN_MAX_ALIGN_FORWARD || distForw > COVERS_JOIN_MAX_ALIGN_BACKWARD)
          continue;

        bool keepMain = false;
        const float distForwMain = cover.dir.x * (ptCenter.x - pos2.x) + cover.dir.z * (ptCenter.z - pos2.z);
        if (distForwMain < -COVERS_JOIN_MAX_ALIGN_FORWARD || distForwMain > COVERS_JOIN_MAX_ALIGN_BACKWARD)
        {
          distForw = 0.0f;
          newDir = cover.dir;
          keepMain = true;
        }

        const float minH = min(isLeft ? cover.groundLeft.y : cover.groundRight.y, pos2.y);
        const float maxH = max(isLeft ? cover.groundLeft.y : cover.groundRight.y, pos2.y);
        if ((maxH - minH) / absDist > COVERS_JOIN_MAX_GROUND_DH_M)
          continue;

        Point3 newDirRight(newDir.z, 0.0f, -newDir.x);
        const float awayCos = dot(newDirRight, cover2.dir);
        const bool shootJoinOK = angleCos >= maxAngleShotCos;

        if (isLeft)
        {
          if ((shootJoinOK || (!cover.hasLeftPos && !cover2.hasRightPos)) && (bestL < 0 || absDist < distL))
          {
            float coef = (awayCos > 0.0f) ? (1.0f - awayCos) : 1.0f;
            coef *= coef;
            bestL = posNearIdx;
            distL = absDist;
            forwL = distForw;
            newDirL = newDir;
            posL = keepMain ? (ptLeft + dirRight * distLeft * coef) : lerp(ptLeft, pos2, coef);
          }
        }
        else
        {
          if ((shootJoinOK || (!cover.hasRightPos && !cover2.hasLeftPos)) && (bestR < 0 || absDist < distR))
          {
            float coef = (awayCos < 0.0f) ? (1.0f + awayCos) : 1.0f;
            coef *= coef;
            bestR = posNearIdx;
            distR = absDist;
            forwR = distForw;
            newDirR = newDir;
            posR = keepMain ? (ptRight + dirRight * distRight * coef) : lerp(ptRight, pos2, coef);
          }
        }
      }

      if (bestL >= 0 && (bestR < 0 || distL < distR))
      {
        auto &coverPosL = covers_arr.arr[bestL];
        auto &coverL = out_covers[coverPosL.coverIdx];
        const float minHeightL = min(coverL.hLeft, coverL.hRight);
        coverPosL.coverIdx = -1; // detach from cover
        coverL.hLeft = -1.0f;    // cover to be removed
        joined = true;
        joining = true;

        Point3 offsRightL(newDirL.z, 0.0f, -newDirL.x);
        offsRightL *= COVERS_STANDARD_WIDTH * 0.5f;

        cover.groundLeft = posL + newDirL * (forwL * 0.5f + COVERS_WALL_OFFSET) - offsRightL;
        cover.groundRight = ptRight + newDirL * (-forwL * 0.5f + COVERS_WALL_OFFSET) + offsRightL;

        Point3 delta = cover.groundRight - cover.groundLeft;
        cover.dir.x = -delta.z;
        cover.dir.y = 0.0f;
        cover.dir.z = delta.x;
        cover.dir.normalize();

        cover.hLeft = (minHeight + minHeightL) * 0.5f;
        cover.hRight = cover.hLeft;

        cover.hasLeftPos = coverL.hasLeftPos;
        cover.shootLeft = coverL.shootLeft;

        cover.visibleBox += coverL.visibleBox;
      }
      else if (bestR >= 0 && (bestL < 0 || distR < distL))
      {
        auto &coverPosR = covers_arr.arr[bestR];
        auto &coverR = out_covers[coverPosR.coverIdx];
        const float minHeightR = min(coverR.hLeft, coverR.hRight);
        coverPosR.coverIdx = -1; // detach from cover
        coverR.hLeft = -1.0f;    // cover to be removed
        joined = true;
        joining = true;

        Point3 offsRightR(newDirR.z, 0.0f, -newDirR.x);
        offsRightR *= COVERS_STANDARD_WIDTH * 0.5f;

        cover.groundLeft = ptLeft + newDirR * (-forwR * 0.5f + COVERS_WALL_OFFSET) - offsRightR;
        cover.groundRight = posR + newDirR * (forwR * 0.5f + COVERS_WALL_OFFSET) + offsRightR;

        Point3 delta = cover.groundRight - cover.groundLeft;
        cover.dir.x = -delta.z;
        cover.dir.y = 0.0f;
        cover.dir.z = delta.x;
        cover.dir.normalize();

        cover.hLeft = (minHeight + minHeightR) * 0.5f;
        cover.hRight = cover.hLeft;

        cover.hasRightPos = coverR.hasRightPos;
        cover.shootRight = coverR.shootRight;

        cover.visibleBox += coverR.visibleBox;
      }
    }

    if (joined)
      coverPos.coverIdx = -1;
  }

  idxs.clear();
  idxs.resize(out_covers.size(), -1);
  for (int posIdx = 0; posIdx < (int)covers_arr.arr.size(); ++posIdx)
  {
    auto &coverPos = covers_arr.arr[posIdx];
    if (coverPos.removed || coverPos.coverIdx < 0)
      continue;
    idxs[coverPos.coverIdx] = posIdx;
  }

  for (int i = 0; i < (int)out_covers.size(); ++i)
    if (out_covers[i].hLeft < 0.0f)
    {
      const int lastPosIdx = idxs.back();
      if (lastPosIdx >= 0)
        covers_arr.arr[lastPosIdx].coverIdx = i;
      idxs[i] = lastPosIdx;
      out_covers[i] = out_covers.back();
      out_covers.pop_back();
      idxs.pop_back();
      --i;
    }

  const float MIN_LONG_COVER_LEN = COVERS_STANDARD_WIDTH + COVERS_POS_RADIUS * 2.5f;
  const float MIN_LONG_COVER_LEN_SQ = sqr(MIN_LONG_COVER_LEN);

  for (int i = 0; i < (int)out_covers.size(); ++i)
  {
    auto &cover = out_covers[i];
    Point3 offsRight = cover.groundRight - cover.groundLeft;
    if (lengthSq(offsRight) < MIN_LONG_COVER_LEN_SQ)
    {
      const Point3 midPoint = (cover.groundLeft + cover.groundRight) * 0.5f;
      normalize(offsRight);
      offsRight *= COVERS_STANDARD_WIDTH * 0.5f;
      cover.groundLeft = midPoint - offsRight;
      cover.groundRight = midPoint + offsRight;
    }
  }
}


void build_covers_v2_positions(CoversArr &out_covers_arr, const BBox3 &tile_box, const Tab<Edge> &edges,
  const CoversParams &cov_params, const rcCompactHeightfield *chf, RenderEdges *edges_out)
{
  const float COVERS_POS_EDGE_OFF_DIST1 = 0.20f;
  const float COVERS_POS_EDGE_OFF_DIST2 = 0.60f;
  const float COVERS_MERGE_COEF = 0.75f;

  const int num_edges = (int)edges.size();
  for (int i = 0; i < num_edges; ++i)
  {
    const Edge &edge = edges[i];
    out_covers_arr.addEdgePositions(edge.sp, edge.sq, COVERS_POS_RADIUS, COVERS_MAX_HEIGHT, COVERS_POS_EDGE_OFF_DIST1, tile_box);
    out_covers_arr.addEdgePositions(edge.sp, edge.sq, COVERS_POS_RADIUS, COVERS_MAX_HEIGHT, COVERS_POS_EDGE_OFF_DIST2, tile_box);
  }

  out_covers_arr.mergePositions(COVERS_MERGE_COEF);

  for (int i = 0; i < (int)out_covers_arr.arr.size(); ++i)
  {
    auto &coverPos = out_covers_arr.arr[i];
    Point3 pt = coverPos.pt;

    int result = trace_covers2_place(chf, COVERS_MIN_HEIGHT, edges_out, pt, coverPos.r);

    if (result != 1)
    {
      pt = coverPos.pt - coverPos.along * chf->cs;
      for (int j = 1; j <= 2; ++j)
      {
        const Point3 offs = coverPos.along * chf->cs * j;
        pt = coverPos.pt - offs;
        result = trace_covers2_place(chf, COVERS_MIN_HEIGHT, edges_out, pt, coverPos.r);
        if (result == 1)
          break;
        pt = coverPos.pt + offs;
        result = trace_covers2_place(chf, COVERS_MIN_HEIGHT, edges_out, pt, coverPos.r);
        if (result == 1)
          break;
      }
    }

    if (result != 1)
    {
      Point3 norm(coverPos.along.z, 0.0f, -coverPos.along.x);
      for (int j = 1; j <= 8; ++j)
      {
        const Point3 offs = norm * chf->cs * j;
        pt = coverPos.pt + offs;
        result = trace_covers2_place(chf, COVERS_MIN_HEIGHT, edges_out, pt, coverPos.r);
        if (result == 1)
          break;
        pt = coverPos.pt - offs;
        result = trace_covers2_place(chf, COVERS_MIN_HEIGHT, edges_out, pt, coverPos.r);
        if (result == 1)
          break;
      }
    }

    if (result != 1)
    {
      // FOR DEBUG
      // const Point3 disp1(-0.05f, 0.0f, 0.05f);
      // const Point3 disp2(0.05f, 0.0f, 0.05f);
      // const Point3 disp3 = coverPos.along * chf->cs * 2.0f;
      // edges_out->push_back({pt + disp1, pt + disp2});
      // edges_out->push_back({pt + disp2, pt - disp1});
      // edges_out->push_back({pt - disp1, pt - disp2});
      // edges_out->push_back({pt - disp2, pt + disp1});
      // edges_out->push_back({pt, pt + disp3});

      coverPos.removed = true;
      continue;
    }

    coverPos.pt = pt;
    coverPos.removed = false;
  }

  out_covers_arr.mergePositions(COVERS_MERGE_COEF);

  if (cov_params.testPos != ZERO<Point3>())
  {
    for (int i = 0; i < (int)out_covers_arr.arr.size(); ++i)
    {
      auto &coverPos = out_covers_arr.arr[i];
      if (lengthSq(coverPos.pt - cov_params.testPos) > sqr(5.0f))
        coverPos.removed = true;
    }

    out_covers_arr.wipeRemoved();
    out_covers_arr.rebuildIndex();
  }
}


void build_covers_v2_debug_voxels(const Point3 &pt, float radius, const rcCompactHeightfield *chf, RenderEdges *out_render_edges)
{
  const int ix0 = clamp((int)floorf((pt.x - radius - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz0 = clamp((int)floorf((pt.z - radius - chf->bmin[2]) / chf->cs), 0, chf->height - 1);
  const int ix1 = clamp((int)floorf((pt.x + radius - chf->bmin[0]) / chf->cs), 0, chf->width - 1);
  const int iz1 = clamp((int)floorf((pt.z + radius - chf->bmin[2]) / chf->cs), 0, chf->height - 1);

  const float radiusSq = sqr(radius);

  for (int iz = iz0; iz <= iz1; ++iz)
  {
    for (int ix = ix0; ix <= ix1; ++ix)
    {
      const float px = (float)ix * chf->cs + chf->bmin[0];
      const float pz = (float)iz * chf->cs + chf->bmin[2];

      const float dx = pt.x - px;
      const float dz = pt.z - pz;
      const float dd = dx * dx + dz * dz;
      if (dd > radiusSq)
        continue;

      float py = chf->bmin[1];
      const rcCompactCell &c = chf->cells[ix + iz * chf->width];
      for (int i = (int)c.index, ni = (int)(c.index + c.count); i < ni; ++i)
      {
        const rcCompactSpan &s = chf->spans[i];
        const bool nullArea = (chf->areas[i] == RC_NULL_AREA);

        const float py2 = chf->bmin[1] + (int)s.y * chf->ch;
        const float ph2 = (int)s.h * chf->ch;

        const float ph = py2 - py;
        const float ps = chf->cs * (nullArea ? 0.8f : 1.0f);

        out_render_edges->push_back({Point3(ph, -1024.5f, ps), Point3(px, py, pz)});

        py = py2 + ph2;
      }

      const float ph = 0.0f;
      const float ps = chf->cs;
      out_render_edges->push_back({Point3(ph, -1024.5f, ps), Point3(px, py, pz)});
    }
  }
}


void build_covers_v2(Tab<covers::Cover> &out_covers, const BBox3 &tile_box, const Tab<Edge> &edges, const CoversParams &cov_params,
  const rcCompactHeightfield *chf, const rcHeightfield *solid, const rcCompactHeightfield *trace_chf, const rcHeightfield *trace_solid,
  RenderEdges *out_render_edges)
{
  G_UNUSED(out_covers);
  G_UNUSED(edges);
  G_UNUSED(cov_params);
  G_UNUSED(solid);
  G_UNUSED(trace_chf);

  RenderEdges *edges_out = out_render_edges;

  const float SHOOT_HEIGHT_PREC = chf->ch * 0.3f;
  const float SHOOT_HEIGHT_STAND = 1.30f - SHOOT_HEIGHT_PREC;
  const float SHOOT_HEIGHT_CROUCH = 0.80f - SHOOT_HEIGHT_PREC;
  const float SHOOT_HEIGHT_CRAWL = 0.30f - SHOOT_HEIGHT_PREC;

  const float EXPOSE_HEIGHT_STAND = 1.45f;
  const float EXPOSE_HEIGHT_CROUCH = 1.00f;
  const float EXPOSE_HEIGHT_CRAWL = 0.45f;

  const float COVER_HEIGHT_STAND = 1.65f;
  const float COVER_HEIGHT_CROUCH = 1.10f;
  const float COVER_HEIGHT_CRAWL = 0.40f;

  const float CRAWL_BODY_LEN = 0.50f;
  const float COVER_MAX_DIST = 2.00f;
  const float SHOOT_MIN_DIST = 2.50f;
  const float SHOOT_NEAR_DIST = 3.20f;
  const float GROUND_HOLE_HT = 0.30f;

  const float GOOD_SHOOTABLE_SUMDIST = 6.25f;
  const float MAX_SUMDIST_CROUCH = 0.625f;
  const float MAX_SUMDIST_CRAWL = 0.625f;

  const int NUM_TRACES_AROUND = 128; // +X +Z -X -Z +X
  const float TRACE_DIST = 15.0f;

  const float SHOOT_WINDOW_MIN_DEG = 20.0f;
  const float SHOOT_WINDOW_MID_DEG = 33.0f;
  const float SHOOT_WINDOW_MAX_DEG = 45.0f;
  const float SHOOT_AROUND_MIN_DEG = 26.0f;
  const float SHOOT_SAFETY_MIN_DEG = 110.0f;
  const float SHOOT_SAFETY_MIN_DEG_CROUCH = 90.0f;
  const float SHOOT_SAFETY_MIN_DEG_CRAWL = 70.0f;
  const int SHOOT_WINMIN_DIRS = (int)floorf((SHOOT_WINDOW_MIN_DEG * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_WINMID_DIRS = (int)floorf((SHOOT_WINDOW_MID_DEG * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_WINMAX_DIRS = (int)floorf((SHOOT_WINDOW_MAX_DEG * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_AROUND_DIRS = (int)floorf((SHOOT_AROUND_MIN_DEG * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_SAFETY_DIRS = (int)floorf((SHOOT_SAFETY_MIN_DEG * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_SAFETY_DIRS2 = (int)floorf((SHOOT_SAFETY_MIN_DEG_CROUCH * NUM_TRACES_AROUND) / 360.0f);
  const int SHOOT_SAFETY_DIRS3 = (int)floorf((SHOOT_SAFETY_MIN_DEG_CRAWL * NUM_TRACES_AROUND) / 360.0f);

  const float MAX_COVER_DIR_TO_ALONG_DEG = 40.0f;
  const float MAX_COVER_DIR_TO_ALONG_COS = cosf(DegToRad(MAX_COVER_DIR_TO_ALONG_DEG));

  const float MIN_CORRECT_ALONG_EDGE_LEN = 1.5f;
  const float MIN_CORRECT_ALONG_EDGE_LEN_SQ = sqr(MIN_CORRECT_ALONG_EDGE_LEN);

  CoversArr coversArr;
  build_covers_v2_positions(coversArr, tile_box, edges, cov_params, chf, edges_out);

  rcCompactHeightfield *chf2 = rcAllocCompactHeightfield();
  {
    rcContext ctx(false);
    const int WALKABLE_HEIGHT = (int)ceilf(EXPOSE_HEIGHT_CROUCH / chf->ch);
    const int WALKABLE_CLIMB = -1; // to include NULL AREA spans in modified Recast source code (rcBuildCompactHeightfield)
    rcBuildCompactHeightfield(&ctx, WALKABLE_HEIGHT, WALKABLE_CLIMB, *(rcHeightfield *)trace_solid, *chf2);
  }

  // if (cov_params.testPos != ZERO<Point3>() && coversArr.isInsideXZLimits(cov_params.testPos, tile_box, 7.0f))
  //   build_covers_v2_debug_voxels(cov_params.testPos, 7.0f, chf2, out_render_edges);

  CoversTracer tracer(chf2, trace_solid, SHOOT_HEIGHT_STAND, SHOOT_HEIGHT_CROUCH, SHOOT_HEIGHT_CRAWL, EXPOSE_HEIGHT_STAND,
    EXPOSE_HEIGHT_CROUCH, EXPOSE_HEIGHT_CRAWL, CRAWL_BODY_LEN, COVER_MAX_DIST, SHOOT_MIN_DIST, SHOOT_NEAR_DIST, GROUND_HOLE_HT,
    GOOD_SHOOTABLE_SUMDIST, MAX_SUMDIST_CRAWL, MAX_SUMDIST_CROUCH);

  const int NUM_TRACES_AROUND_TWICE = NUM_TRACES_AROUND * 2;
  const float invNumTracesAround = 1.0f / NUM_TRACES_AROUND;

  const Point3 upAbit(0.0f, 0.05f, 0.0f);

  for (int posIdx = 0; posIdx < (int)coversArr.arr.size(); ++posIdx)
  {
    const auto &coverPos = coversArr.arr[posIdx];
    if (coverPos.removed)
      continue;

    bool debugThisCover = false;
    if (cov_params.testPos != ZERO<Point3>())
    {
      debugThisCover = lengthSq(coverPos.pt - cov_params.testPos) < sqr(0.5f);
      // if (!debugThisCover)
      //   continue;
    }

    if (!tracer.init_position(coverPos.pt + upAbit, tile_box, debugThisCover))
    {
      // // FOR DEBUG
      // const Point3 pt = coverPos.pt;
      // const Point3 disp1(-0.1f, 0.0f, 0.1f);
      // const Point3 disp2(0.1f, 0.0f, 0.1f);
      // edges_out->push_back({pt + disp1, pt - disp1});
      // edges_out->push_back({pt + disp2, pt - disp2});
      continue;
    }

    bool anyCover = false;
    CoversTracer::ECover coverByDir[NUM_TRACES_AROUND];
    float coverDists[NUM_TRACES_AROUND];
    for (int dir = 0; dir < NUM_TRACES_AROUND; ++dir)
    {
      const float angle = dir * (TWOPI / NUM_TRACES_AROUND);
      float dx, dz;
      sincos(angle, dz, dx);
      float dist = TRACE_DIST;
      const CoversTracer::ECover coverType = tracer.trace_normalized(dx, dz, dist, debugThisCover && dir == 0);
      coverByDir[dir] = coverType;
      coverDists[dir] = dist;
      if (CoversTracer::is_cover(coverType))
        anyCover = true;

      if (debugThisCover)
      {
        const int shot1 = tracer.lastShot1;
        const int shot2 = tracer.lastShot2;
        const int shot3 = tracer.lastShot3;
        const int over1 = tracer.lastOver1;
        const int over2 = tracer.lastOver2;
        const int cshot1 = tracer.lastCoverShot1;
        const int cshot2 = tracer.lastCoverShot2;
        const int cshot3 = tracer.lastCoverShot3;

        const float coef = chf->cs / chf->ch;
        covtrace("!!! DIR %f (%d/%d) %s [%d %d %d] (%d %d) [%d %d %d] dist(%f) aim1(%f,%f)", RadToDeg(angle), dir, NUM_TRACES_AROUND,
          CoversTracer::get_cover_name(coverType), shot1, shot2, shot3, over1, over2, cshot1, cshot2, cshot3, dist,
          tracer.lastCoverAim1.debugAngleTop(coef), tracer.lastCoverAim1.debugAngleBot(coef));

        // FOR DEBUG
        float cdist = 0.0f;
        float cvup = 0.0f;
        switch (coverType)
        {
          case CoversTracer::NO_START:
          case CoversTracer::NO_DIRECTION: cdist = 50.0f; break;
          case CoversTracer::NO_COVER_BAD_ATTACK: cdist = 0.0f; break;
          case CoversTracer::NO_COVER_GOOD_ATTACK: cdist = 10.0f; break;
          case CoversTracer::INEFFECTIVE_COVER:
            cdist = dist;
            cvup = 0.2f;
            break;
          case CoversTracer::COVER_CROUCH_TO_STAND:
            cdist = dist;
            cvup = COVER_HEIGHT_CROUCH;
            break;
          case CoversTracer::COVER_CRAWL_TO_CROUCH:
            cdist = dist;
            cvup = COVER_HEIGHT_CRAWL;
            break;
          case CoversTracer::COVER_CRAWL_TO_STAND:
            cdist = dist;
            cvup = COVER_HEIGHT_CRAWL + 0.1f;
            break;
          case CoversTracer::COVER_FULL_WALL:
            cdist = dist;
            cvup = 2.5f;
            break;
          case CoversTracer::COVER_FULL_STAND:
            cdist = dist;
            cvup = COVER_HEIGHT_STAND;
            break;
          case CoversTracer::COVER_FULL_CROUCH:
            cdist = dist;
            cvup = COVER_HEIGHT_CROUCH + 0.2f;
            break;
          case CoversTracer::COVER_FULL_CRAWL:
            cdist = dist;
            cvup = COVER_HEIGHT_CRAWL + 0.2f;
            break;
        }
        const Point3 pt = coverPos.pt;
        const Point3 disp1(dx * cdist, 0.0f, dz * cdist);
        const Point3 disp2(0.0f, cvup, 0.0f);
        out_render_edges->push_back({pt, pt + disp1});
        out_render_edges->push_back({pt + disp1, pt + disp1 + disp2});
      }
    }
    if (!anyCover)
    {
      // FOR DEBUG
      // const Point3 pt = coverPos.pt;
      // const Point3 disp1(-0.05f, 0.0f, 0.05f);
      // const Point3 disp2(0.05f, 0.0f, 0.05f);
      // edges_out->push_back({pt + disp1, pt - disp1});
      // edges_out->push_back({pt + disp2, pt - disp2});
      continue;
    }

    auto calcStandHidden = [&]() {
      int count = 0;
      for (int dir = 0; dir < NUM_TRACES_AROUND; ++dir)
        if (CoversTracer::is_cover_for_stand(coverByDir[dir]))
          ++count;
      return count * invNumTracesAround;
    };

    auto calcCrouchHidden = [&]() {
      int count = 0;
      for (int dir = 0; dir < NUM_TRACES_AROUND; ++dir)
        if (CoversTracer::is_cover_for_crouch(coverByDir[dir]))
          ++count;
      return count * invNumTracesAround;
    };

    auto calcCrawlHidden = [&]() {
      int count = 0;
      for (int dir = 0; dir < NUM_TRACES_AROUND; ++dir)
        if (CoversTracer::is_cover_for_crawl(coverByDir[dir]))
          ++count;
      return count * invNumTracesAround;
    };

    const float hidden1 = calcStandHidden();
    const float hidden2 = calcCrouchHidden();
    const float hidden3 = calcCrawlHidden();
    const float hiddenOverall = hidden1 * hidden2 * hidden3;
    const float hiddenCrouched = hidden2 * hidden3;
    const float hiddenCrawling = hidden3;

    struct CoverSector
    {
      int dir = -1;
      int len = 0;

      bool is_valid() const { return dir >= 0 && len > 0; }

      void reset()
      {
        dir = -1;
        len = 0;
      }

      bool build(int at_dir, bool in1, bool in2, bool last)
      {
        if (in1)
        {
          if (dir < 0)
          {
            dir = at_dir;
            len = 1;
          }
          else
            ++len;
        }
        else if (dir >= 0)
        {
          if (in2)
            ++len;
          else
            return true;
        }
        return last;
      }
    };

    auto canCrawlAtCover = [&](const CoverSector &by_sector) {
      const float shootAngle = (by_sector.dir + by_sector.len * 0.5f) * (TWOPI / NUM_TRACES_AROUND);

      float dx, dz;
      sincos(shootAngle, dz, dx);
      Point3 shootDir(dx, 0.0f, dz);

      const float CRAWL_CHECK_BACK_DIST_1 = 0.45f;
      const float CRAWL_CHECK_BACK_DIST_2 = 0.90f;
      const float CRAWL_CHECK_MAX_H_DIFF = 0.10f;

      Point3 pt = coverPos.pt - shootDir * CRAWL_CHECK_BACK_DIST_1;
      if (trace_covers2_place(chf, COVERS_MIN_HEIGHT, nullptr, pt, coverPos.r) != 1 ||
          fabsf(pt.y - coverPos.pt.y) > CRAWL_CHECK_MAX_H_DIFF)
        return false;

      pt = coverPos.pt - shootDir * CRAWL_CHECK_BACK_DIST_2;
      if (trace_covers2_place(chf, COVERS_MIN_HEIGHT, nullptr, pt, coverPos.r) != 1 ||
          fabsf(pt.y - coverPos.pt.y) > CRAWL_CHECK_MAX_H_DIFF)
        return false;

      return true;
    };

    auto correctCoverAngleAlongEdge = [&](float &dirAngle) {
      float dx, dz;
      sincos(dirAngle, dz, dx);
      Point3 dirVec(dx, 0.0f, dz);
      if (abs(dot(coverPos.along, dirVec)) < MAX_COVER_DIR_TO_ALONG_COS)
      {
        Point3 forw(coverPos.along.z, 0.0f, -coverPos.along.x);
        if (dot(forw, dirVec) < 0.0f)
          forw = -forw;
        forw.normalize();
        dirVec += forw; // * dot(forw, dirVec) * 3.0f;
        dirVec.normalize();
        dirAngle = atan2(dirVec.z, dirVec.x);
        return true;
      }
      return false;
    };

    // auto correctCoverSectorAlongEdge = [&](CoverSector &sector) {
    //   float dirAngle = (sector.dir + sector.len * 0.5f) * (TWOPI / NUM_TRACES_AROUND);
    //   if (correctCoverAngleAlongEdge(dirAngle))
    //     sector.dir = (int)floorf((dirAngle * (NUM_TRACES_AROUND / TWOPI)) - sector.len * 0.5f);
    // };

    CoverSector bestSector;
    float bestHeight = 0.0f;
    float bestScore = 0.0f;
    bool bestCrawl = false;
    bool bestFull = false;
    bool bestWalkout = false;
    bool bestHasShootLeft = false;
    bool bestHasShootRight = false;
    Point3 bestShootLeftPos = ZERO<Point3>();
    Point3 bestShootRightPos = ZERO<Point3>();
    bool bestIsAround = false;

    // try find best crouch-to-stand cover
    {
      const float scoreCoef = hiddenCrouched;

      CoverSector sector;
      for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
      {
        const int dir = idir % NUM_TRACES_AROUND;
        const CoversTracer::ECover cover1 = coverByDir[dir];
        const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
        const bool in1 = cover1 == CoversTracer::COVER_CROUCH_TO_STAND || cover1 == CoversTracer::COVER_FULL_CROUCH;
        const bool in2 = cover2 == CoversTracer::COVER_CROUCH_TO_STAND || cover2 == CoversTracer::COVER_FULL_CROUCH;
        if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
        {
          bool isOKOnFulls = true;
          if (sector.dir >= 0 && sector.len >= SHOOT_WINMIN_DIRS)
          {
            int numFulls = 0;
            for (int j = 0; j < sector.len; ++j)
            {
              const int jdir = (sector.dir + j) % NUM_TRACES_AROUND;
              const CoversTracer::ECover jcover = coverByDir[jdir];
              if (jcover == CoversTracer::COVER_FULL_CROUCH)
                ++numFulls;
            }
            const int percFull = (numFulls * 100) / sector.len;
            if (percFull > 50)
              isOKOnFulls = false;
          }

          if (debugThisCover)
            covtrace("!!! 2to1 sector dir=%d len=%d min=%d okfulls=%d", sector.dir, sector.len, SHOOT_WINMIN_DIRS,
              isOKOnFulls ? 1 : 0);

          if (sector.dir >= 0 && sector.len >= SHOOT_WINMIN_DIRS && isOKOnFulls)
          {
            const int leftDir = (sector.dir + NUM_TRACES_AROUND + sector.len) % NUM_TRACES_AROUND;
            const int rightDir = (sector.dir + NUM_TRACES_AROUND - 1) % NUM_TRACES_AROUND;
            const bool leftWalled = CoversTracer::is_cover_for_stand(coverByDir[leftDir]);
            const bool rightWalled = CoversTracer::is_cover_for_stand(coverByDir[rightDir]);
            const float walledCoef = (leftWalled ? 1.0f : 0.9f) * (rightWalled ? 1.0f : 0.9f);
            const float atScore = sector.len * walledCoef * scoreCoef;

            const int minLen = (leftWalled && rightWalled)   ? SHOOT_WINMIN_DIRS
                               : (leftWalled || rightWalled) ? SHOOT_WINMID_DIRS
                                                             : SHOOT_WINMAX_DIRS;

            if (debugThisCover)
              covtrace("!!! 2to1 ~~~ len=%d minLen=%d lwall=%d rwall=%d score=%f bestScore=%f bestValid=%d", sector.len, minLen,
                leftWalled ? 1 : 0, rightWalled ? 1 : 0, atScore, bestScore, bestSector.is_valid() ? 1 : 0);

            if (sector.len >= minLen && (!bestSector.is_valid() || atScore > bestScore))
            {
              bestSector = sector;
              bestHeight = COVER_HEIGHT_CROUCH;
              bestScore = atScore;
              bestCrawl = false;
              bestFull = false;
            }
          }
          sector.reset();
        }
      }
    }

    // check if has better crawl-to-crouch cover
    {
      const float CRAWL_TO_CROUCH_SCORE_COEF = 0.75f;
      const float scoreCoef = hiddenCrawling * CRAWL_TO_CROUCH_SCORE_COEF;

      CoverSector sector;
      for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
      {
        const int dir = idir % NUM_TRACES_AROUND;
        const CoversTracer::ECover cover1 = coverByDir[dir];
        const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
        const bool in1 = cover1 == CoversTracer::COVER_CRAWL_TO_CROUCH;
        const bool in2 = cover2 == CoversTracer::COVER_CRAWL_TO_CROUCH;
        if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
        {
          if (debugThisCover)
            covtrace("!!! 3to2 sector dir=%d len=%d min=%d cancrawl=%d", sector.dir, sector.len, SHOOT_WINMIN_DIRS,
              canCrawlAtCover(sector) ? 1 : 0);

          if (sector.dir >= 0 && sector.len >= SHOOT_WINMIN_DIRS && canCrawlAtCover(sector))
          {
            const int leftDir = (sector.dir + NUM_TRACES_AROUND + sector.len) % NUM_TRACES_AROUND;
            const int rightDir = (sector.dir + NUM_TRACES_AROUND - 1) % NUM_TRACES_AROUND;
            const bool leftWalled = CoversTracer::is_cover_for_crouch(coverByDir[leftDir]);
            const bool rightWalled = CoversTracer::is_cover_for_crouch(coverByDir[rightDir]);
            const float walledCoef = (leftWalled ? 1.0f : 0.9f) * (rightWalled ? 1.0f : 0.9f);
            const float atScore = sector.len * walledCoef * scoreCoef;

            const int minLen = (leftWalled && rightWalled)   ? SHOOT_WINMIN_DIRS
                               : (leftWalled || rightWalled) ? SHOOT_WINMID_DIRS
                                                             : SHOOT_WINMAX_DIRS;

            if (debugThisCover)
              covtrace("!!! 3to2 ~~~ len=%d minLen=%d lwall=%d rwall=%d score=%f bestScore=%f bestValid=%d", sector.len, minLen,
                leftWalled ? 1 : 0, rightWalled ? 1 : 0, atScore, bestScore, bestSector.is_valid() ? 1 : 0);

            if (sector.len >= minLen && (!bestSector.is_valid() || atScore > bestScore))
            {
              bestSector = sector;
              bestHeight = COVER_HEIGHT_CRAWL;
              bestScore = atScore;
              bestCrawl = true;
              bestFull = false;
            }
          }
          sector.reset();
        }
      }
    }

    // check if has better crawl-to-stand cover
    {
      const float CRAWL_TO_STAND_SCORE_COEF = 0.25f;
      const float scoreCoef = hiddenCrawling * CRAWL_TO_STAND_SCORE_COEF;

      CoverSector sector;
      for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
      {
        const int dir = idir % NUM_TRACES_AROUND;
        const CoversTracer::ECover cover1 = coverByDir[dir];
        const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
        const bool in1 = cover1 == CoversTracer::COVER_CRAWL_TO_STAND || cover1 == CoversTracer::COVER_CROUCH_TO_STAND;
        const bool in2 = cover2 == CoversTracer::COVER_CRAWL_TO_STAND || cover1 == CoversTracer::COVER_CROUCH_TO_STAND;
        if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
        {
          if (debugThisCover)
            covtrace("!!! 3to1 sector dir=%d len=%d min=%d cancrawl=%d", sector.dir, sector.len, SHOOT_WINMIN_DIRS,
              canCrawlAtCover(sector) ? 1 : 0);

          if (sector.dir >= 0 && sector.len >= SHOOT_WINMIN_DIRS && canCrawlAtCover(sector))
          {
            const int leftDir = (sector.dir + NUM_TRACES_AROUND + sector.len) % NUM_TRACES_AROUND;
            const int rightDir = (sector.dir + NUM_TRACES_AROUND - 1) % NUM_TRACES_AROUND;
            const bool leftWalled = CoversTracer::is_cover_for_stand(coverByDir[leftDir]);
            const bool rightWalled = CoversTracer::is_cover_for_stand(coverByDir[rightDir]);
            const float walledCoef = (leftWalled ? 1.0f : 0.9f) * (rightWalled ? 1.0f : 0.9f);
            const float atScore = sector.len * walledCoef * scoreCoef;

            const int minLen = (leftWalled && rightWalled)   ? SHOOT_WINMIN_DIRS
                               : (leftWalled || rightWalled) ? SHOOT_WINMID_DIRS
                                                             : SHOOT_WINMAX_DIRS;

            if (debugThisCover)
              covtrace("!!! 3to1 ~~~ len=%d minLen=%d lwall=%d rwall=%d score=%f bestScore=%f bestValid=%d", sector.len, minLen,
                leftWalled ? 1 : 0, rightWalled ? 1 : 0, atScore, bestScore, bestSector.is_valid() ? 1 : 0);

            if (sector.len >= minLen && (!bestSector.is_valid() || atScore > bestScore))
            {
              bestSector = sector;
              bestHeight = COVER_HEIGHT_CRAWL;
              bestScore = atScore;
              bestCrawl = true;
              bestFull = false;
            }
          }
          sector.reset();
        }
      }
    }

    auto findWalkoutShootPosByDir = [&](Point3 &out_shoot_pos, const Point3 &shoot_dir, bool walkout_left) {
      Point3 offsLeft = Point3(-shoot_dir.z, 0.0f, shoot_dir.x);
      Point3 alongLeft = coverPos.along;
      if (dot(offsLeft, alongLeft) < 0.0f)
        alongLeft = -alongLeft;
      offsLeft += alongLeft;
      offsLeft.normalize();
      offsLeft *= COVERS_STANDARD_WIDTH * 0.5f;

      const int WALKOUT_MAX_HALFS = 4;
      const float WALKOUT_MAX_H_DIFF_M = 0.4f;
      // const float WALKOUT_SHOOT_EPS_DEG = 5.0f;

      int num = 0;
      for (int j = 2; j <= WALKOUT_MAX_HALFS * 3; ++j)
      {
        const float k = (walkout_left ? j : -j) * 0.333f;
        Point3 pt = coverPos.pt + offsLeft * k;
        if (trace_covers2_place(chf, COVERS_MIN_HEIGHT, nullptr, pt, coverPos.r) != 1 ||
            fabsf(pt.y - coverPos.pt.y) > WALKOUT_MAX_H_DIFF_M * Point3::x0z(pt - coverPos.pt).length())
          break;
        if (!tracer.init_position(pt, tile_box, false))
          continue;
        float dist = TRACE_DIST;
        const CoversTracer::ECover result = tracer.trace_normalized(shoot_dir.x, shoot_dir.z, dist, false);
        if (CoversTracer::is_good_for_stand_attack(result, tracer.lastCoverAim1))
        {
          // // FOR DEBUG
          // const float sdist = 5.0f;
          // const Point3 disp(0.0f, SHOOT_HEIGHT_STAND, 0.0f);
          // edges_out->push_back({pt + disp, pt + disp + Point3(dx * sdist, 0.0f, dz * sdist)});

          if (++num < 2)
            continue;
          out_shoot_pos = pt;
          return true;
        }
      }
      return false;
    };

    auto findWalkoutShootPos = [&](Point3 &out_shoot_pos, const CoverSector &by_sector, bool walkout_left) {
      const float shootAngle = (by_sector.dir + by_sector.len * 0.5f) * (TWOPI / NUM_TRACES_AROUND);
      float dx, dz;
      sincos(shootAngle, dz, dx);
      Point3 shootDir(dx, 0.0f, dz);
      return findWalkoutShootPosByDir(out_shoot_pos, shootDir, walkout_left);
    };

    bool crossing = false;
    if (!bestSector.is_valid())
    {
      const float MAX_CROSSED_COVER_DIST = 1.5f;
      const int MAX_NUM_CROSSED_FOR_CROSSING = 5;
      int maxCrossed = 0;

      int numCrossed = 0;
      for (int idir = 0; idir < NUM_TRACES_AROUND + MAX_NUM_CROSSED_FOR_CROSSING; ++idir)
      {
        bool crossed = false;
        for (int bend = -4; bend <= 4; bend += 2)
        {
          const int dirF = idir % NUM_TRACES_AROUND;
          const int dirB = (idir + NUM_TRACES_AROUND / 2 + bend) % NUM_TRACES_AROUND;
          if (!CoversTracer::is_cover(coverByDir[dirF]) || !CoversTracer::is_cover(coverByDir[dirB]))
            continue;
          if (coverDists[dirF] > MAX_CROSSED_COVER_DIST || coverDists[dirB] > MAX_CROSSED_COVER_DIST)
            continue;
          const int dirL = (idir + NUM_TRACES_AROUND / 4 + bend / 2) % NUM_TRACES_AROUND;
          const int dirR = (idir + NUM_TRACES_AROUND - NUM_TRACES_AROUND / 4 + bend / 2) % NUM_TRACES_AROUND;
          if (CoversTracer::is_cover(coverByDir[dirL]) || CoversTracer::is_cover(coverByDir[dirR]))
            continue;
          crossed = true;
          break;
        }
        if (!crossed)
          numCrossed = 0;
        else
        {
          ++numCrossed;
          maxCrossed = max(maxCrossed, numCrossed);
        }
      }
      crossing = maxCrossed > MAX_NUM_CROSSED_FOR_CROSSING;
    }

    if (!bestSector.is_valid() && !crossing) // if not found try full covers
    {
      // stand cover
      const float MIN_HIDDEN_OVERALL = 0.0005f;
      if (hiddenOverall >= MIN_HIDDEN_OVERALL)
      {
        CoverSector sector;
        for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
        {
          const int dir = idir % NUM_TRACES_AROUND;
          const CoversTracer::ECover cover1 = coverByDir[dir];
          const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
          const bool in1 = CoversTracer::is_cover_for_stand(cover1);
          const bool in2 = CoversTracer::is_cover_for_stand(cover2);
          if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
          {
            if (debugThisCover)
              covtrace("!!! w1 sector dir=%d len=%d min=%d", sector.dir, sector.len, SHOOT_AROUND_DIRS);

            if (sector.dir >= 0 && sector.len >= SHOOT_AROUND_DIRS)
            {
              // correctCoverSectorAlongEdge(sector);

              Point3 leftPos = ZERO<Point3>();
              Point3 rightPos = ZERO<Point3>();
              const bool hasLeft = findWalkoutShootPos(leftPos, sector, true);
              const bool hasRight = findWalkoutShootPos(rightPos, sector, false);

              const bool walkoutCover = hasLeft || hasRight;
              const bool safetyCover = sector.len >= SHOOT_SAFETY_DIRS && !hasLeft && !hasRight && hiddenOverall > 0.3f;

              if (debugThisCover)
                covtrace("!!! w1 ~~~ walkout=%d safety=%d hasLeft=%d hasRight=%d len=%d slen=%d", walkoutCover ? 1 : 0,
                  safetyCover ? 1 : 0, hasLeft ? 1 : 0, hasRight ? 1 : 0, sector.len, SHOOT_SAFETY_DIRS);

              if (walkoutCover || safetyCover)
              {
                const float shootCoef = (hasLeft ? 1.0f : 0.5f) * (hasRight ? 1.0f : 0.5f);
                const float atScore = sector.len * shootCoef;

                if (debugThisCover)
                  covtrace("!!! w1 ~~~ score=%f bestScore=%f bestValid=%d", atScore, bestScore, bestSector.is_valid() ? 1 : 0);

                if (!bestSector.is_valid() || atScore > bestScore)
                {
                  bestSector = sector;
                  bestHeight = COVER_HEIGHT_STAND;
                  bestScore = atScore;
                  bestCrawl = false;
                  bestFull = true;
                  bestWalkout = hasLeft || hasRight;
                  bestHasShootLeft = hasLeft;
                  bestHasShootRight = hasRight;
                  bestShootLeftPos = leftPos;
                  bestShootRightPos = rightPos;
                  bestIsAround = walkoutCover && !safetyCover;
                }
              }
            }
            sector.reset();
          }
        }
      }
      else if (debugThisCover)
      {
        covtrace("!!! w1 hiddenOverall %f < %f", hiddenOverall, MIN_HIDDEN_OVERALL);
      }

      // crouch cover
      const float MIN_HIDDEN_CROUCHED = 0.05f;
      if (!bestSector.is_valid() && hiddenCrouched >= MIN_HIDDEN_CROUCHED)
      {
        CoverSector sector;
        for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
        {
          const int dir = idir % NUM_TRACES_AROUND;
          const CoversTracer::ECover cover1 = coverByDir[dir];
          const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
          const bool in1 = CoversTracer::is_cover_for_crouch(cover1);
          const bool in2 = CoversTracer::is_cover_for_crouch(cover2);
          if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
          {
            if (debugThisCover)
              covtrace("!!! w2 sector dir=%d len=%d min=%d", sector.dir, sector.len, SHOOT_SAFETY_DIRS2);

            if (sector.dir >= 0 && sector.len >= SHOOT_SAFETY_DIRS2)
            {
              // correctCoverSectorAlongEdge(sector);

              const float atScore = (float)sector.len;

              if (debugThisCover)
                covtrace("!!! w2 ~~~ score=%f bestScore=%f bestValid=%d", atScore, bestScore, bestSector.is_valid() ? 1 : 0);

              if (!bestSector.is_valid() || atScore > bestScore)
              {
                bestSector = sector;
                bestHeight = COVER_HEIGHT_CROUCH;
                bestScore = atScore;
                bestCrawl = false;
                bestFull = true;
              }
            }
            sector.reset();
          }
        }
      }
      else if (debugThisCover && !bestSector.is_valid())
      {
        covtrace("!!! w2 hiddenCrouched %f < %f", hiddenCrouched, MIN_HIDDEN_CROUCHED);
      }

      // crawl cover
      const float MIN_HIDDEN_CRAWLING = 0.2f;
      if (!bestSector.is_valid() && hiddenCrawling > MIN_HIDDEN_CRAWLING)
      {
        CoverSector sector;
        for (int idir = 0; idir < NUM_TRACES_AROUND_TWICE; ++idir)
        {
          const int dir = idir % NUM_TRACES_AROUND;
          const CoversTracer::ECover cover1 = coverByDir[dir];
          const CoversTracer::ECover cover2 = coverByDir[(idir + 1) % NUM_TRACES_AROUND];
          const bool in1 = CoversTracer::is_cover_for_crawl(cover1);
          const bool in2 = CoversTracer::is_cover_for_crawl(cover2);
          if (sector.build(dir, in1, in2, idir == NUM_TRACES_AROUND_TWICE - 1))
          {
            if (debugThisCover)
              covtrace("!!! w3 sector dir=%d len=%d min=%d cancrawl=%d", sector.dir, sector.len, SHOOT_SAFETY_DIRS3,
                canCrawlAtCover(sector) ? 1 : 0);

            if (sector.dir >= 0 && sector.len >= SHOOT_SAFETY_DIRS3 && canCrawlAtCover(sector))
            {
              // correctCoverSectorAlongEdge(sector);

              const float atScore = (float)sector.len;

              if (debugThisCover)
                covtrace("!!! w3 ~~~ score=%f bestScore=%f bestValid=%d", atScore, bestScore, bestSector.is_valid() ? 1 : 0);

              if (!bestSector.is_valid() || atScore > bestScore)
              {
                bestSector = sector;
                bestHeight = COVER_HEIGHT_CRAWL;
                bestScore = atScore;
                bestCrawl = true;
                bestFull = true;
              }
            }
            sector.reset();
          }
        }
      }
      else if (debugThisCover && !bestSector.is_valid() && hiddenCrawling <= MIN_HIDDEN_CRAWLING)
      {
        covtrace("!!! NOT COVER CRAWLED %f < %f", hiddenCrawling, MIN_HIDDEN_CRAWLING);
      }
    }

    if (!bestSector.is_valid())
    {
      if (debugThisCover)
        covtrace("!!! COVER SECTOR not found");

      // FOR DEBUG
      // const Point3 pt = coverPos.pt;
      // const Point3 disp1(-0.05f, 0.0f, 0.05f);
      // const Point3 disp2(0.05f, 0.0f, 0.05f);
      // edges_out->push_back({pt + disp1, pt + disp2});
      // edges_out->push_back({pt + disp2, pt - disp1});
      // edges_out->push_back({pt - disp1, pt - disp2});
      // edges_out->push_back({pt - disp2, pt + disp1});
      continue;
    }

    if (bestFull)
    {
      const float COVER_POS_ATOP_RADIUS = 1.5f;
      const float COVER_POS_ATOP_HT_UP = 0.2f;
      const float COVER_POS_ATOP_HT_DOWN = 0.2f;
      const int COVER_POS_ATOP_NEAR_MIN = 2;

      Point3 pt = coverPos.pt;
      pt.y -= COVER_POS_ATOP_HT_DOWN;
      const float h = COVER_POS_ATOP_HT_DOWN + COVER_POS_ATOP_HT_UP;

      const int numNear = coversArr.countAround(pt, posIdx, COVER_POS_ATOP_RADIUS, h, COVER_POS_ATOP_NEAR_MIN, 0.0f);
      if (numNear < COVER_POS_ATOP_NEAR_MIN)
        continue;
    }

    float dirAngle = (bestSector.dir + bestSector.len * 0.5f) * (TWOPI / NUM_TRACES_AROUND);
    if (coversArr.calcEdgeLenSq(coverPos.edge) >= MIN_CORRECT_ALONG_EDGE_LEN_SQ)
      correctCoverAngleAlongEdge(dirAngle);

    float dx, dz;
    sincos(dirAngle, dz, dx);
    Point3 dirVec(dx, 0.0f, dz);

    if (debugThisCover)
    {
      covtrace("!!! COVER SECTOR dir=%d len=%d angle=%f left/right=%f full=%d", bestSector.dir, bestSector.len, RadToDeg(dirAngle),
        bestSector.len * 0.5f * 360.0 / NUM_TRACES_AROUND, bestFull ? 1 : 0);
    }

    Point3 offsLeft(-dirVec.z, 0.0f, dirVec.x);
    offsLeft *= COVERS_STANDARD_WIDTH * 0.5f;
    const Point3 posCenter = coverPos.pt - dirVec * (bestCrawl ? CRAWL_BODY_LEN : 0.0f);
    const Point3 posLeft = posCenter + offsLeft;
    const Point3 posRight = posCenter - offsLeft;

    if (!bestFull)
    {
      bestHasShootLeft = false;
      bestHasShootRight = false;
      bestShootLeftPos = posCenter;
      bestShootRightPos = posCenter;
    }

    // add cover
    {
      covers::Cover coverDesc;
      coverDesc.dir = dirVec;

      const Point3 wallOffset = coverDesc.dir * COVERS_WALL_OFFSET;
      coverDesc.groundLeft = posLeft + wallOffset;
      coverDesc.groundRight = posRight + wallOffset;

      coverDesc.hLeft = bestHeight;
      coverDesc.hRight = bestHeight;

      coverDesc.hasLeftPos = bestHasShootLeft;
      coverDesc.hasRightPos = bestHasShootRight;
      coverDesc.shootLeft = bestShootLeftPos;
      coverDesc.shootRight = bestShootRightPos;

      BBox3 traceBox;
      float findCt = recastcoll::build_box_on_sphere_cast(trace_solid, posLeft, 15, {10, 5}, traceBox);
      findCt *= recastcoll::build_box_on_sphere_cast(trace_solid, posRight, 15, {10, 5}, traceBox);
      if (findCt < cov_params.openingTreshold)
        traceBox = cov_params.maxVisibleBox;
      else
        traceBox.inflate(cov_params.boxOffset);
      coverDesc.visibleBox = traceBox;

      const int coverIdx = (int)out_covers.size();
      out_covers.push_back(coverDesc);

      auto &coverPosModifyData = coversArr.arr[posIdx];
      const bool isHalfCover = !bestFull;
      const bool isCrawlCover = bestHeight == COVER_HEIGHT_CRAWL;
      const bool isAroundCover = bestIsAround;
      const bool isWalkoutCover = bestHasShootLeft || bestHasShootRight;
      const bool isExtraCover = bestFull && isCrawlCover;
      //
      coverPosModifyData.coverIdx = coverIdx;
      coverPosModifyData.coverScore = coverPosModifyData.calcScore(isHalfCover && !isCrawlCover, isHalfCover && isCrawlCover,
        isAroundCover, isWalkoutCover, isExtraCover, hiddenOverall);
    }
  }

  build_covers_v2_join(out_covers, coversArr, cov_params);

  build_covers_v2_reduce(out_covers, coversArr, tile_box, cov_params);

  rcFreeCompactHeightfield(chf2);
}


enum
{
  COVER_FITFLAG_IS_LONG = 0x01,
};

int make_fit_cover_flags_v2(const covers::Cover &cover, const CoversParams &covParams)
{
  G_UNUSED(covParams);

  int flags = 0;
  const float LONG_COVER_MIN_LENGTH_SQ = sqr(2.5f);
  if (lengthSq(cover.groundLeft - cover.groundRight) >= LONG_COVER_MIN_LENGTH_SQ)
    flags |= COVER_FITFLAG_IS_LONG;
  return flags;
}

int try_fit_cover_v2(float &max_vol, covers::Cover &cover, int cover_flags, const covers::Cover &by_cover,
  const CoversParams &covParams)
{
  G_UNUSED(covParams);

  if (!(cover_flags & COVER_FITFLAG_IS_LONG))
    return 0;

  const float MAX_H_DIFF = 0.20f;
  const float MAX_Z_DIFF = 0.20f;

  const float h1min = min(cover.groundLeft.y, cover.groundRight.y);
  const float h1max = max(cover.groundLeft.y, cover.groundRight.y);
  const float h2min = min(by_cover.groundLeft.y, by_cover.groundRight.y);
  const float h2max = max(by_cover.groundLeft.y, by_cover.groundRight.y);
  if (abs(h1min - h2min) > MAX_H_DIFF || abs(h1max - h2max) > MAX_H_DIFF)
    return -1;

  const float h1 = (cover.hLeft + cover.hRight) * 0.5f;
  const float h2 = (by_cover.hLeft + by_cover.hRight) * 0.5f;
  if (h2 - h1 > MAX_H_DIFF)
    return -1;

  const float len1 = (cover.groundLeft - cover.groundRight).length();
  const Point3 pos1 = (cover.groundLeft + cover.groundRight) * 0.5f - cover.dir * COVERS_WALL_OFFSET;
  const float len2 = (by_cover.groundLeft - by_cover.groundRight).length();
  const Point3 pos2 = (by_cover.groundLeft + by_cover.groundRight) * 0.5f - by_cover.dir * COVERS_WALL_OFFSET;

  Point3 newDir = cover.dir + by_cover.dir;
  newDir.normalize();

  const float distZ = newDir.x * (pos2.x - pos1.x) + newDir.z * (pos2.z - pos1.z);
  if (abs(distZ) > MAX_Z_DIFF)
    return -1;

  const float distX = newDir.x * (pos2.z - pos1.z) + newDir.z * (pos1.x - pos2.x);
  const float leftX = distX - len1 * 0.5f;
  const float rightX = distX + len1 * 0.5f;
  const float leftEdgeX = -len2 * 0.5f;
  const float rightEdgeX = len2 * 0.5f;

  if (leftX > leftEdgeX && rightX < rightEdgeX)
    return 1;

  if (leftX < rightEdgeX && rightX > rightEdgeX)
  {
    const float shortenDist = rightEdgeX - leftX;
    if (len1 - shortenDist < COVERS_STANDARD_WIDTH)
      return 1;

    cover.hasLeftPos = false;
    cover.groundLeft += Point3(newDir.z, 0.0f, -newDir.x) * shortenDist;
    return -1;
  }

  if (rightX > leftEdgeX && leftX < leftEdgeX)
  {
    const float shortenDist = rightX - leftEdgeX;
    if (len1 - shortenDist < COVERS_STANDARD_WIDTH)
      return 1;

    cover.hasRightPos = false;
    cover.groundRight -= Point3(newDir.z, 0.0f, -newDir.x) * shortenDist;
    return -1;
  }

  max_vol = 0.4f;
  return 0;
}
} // namespace recastbuild
