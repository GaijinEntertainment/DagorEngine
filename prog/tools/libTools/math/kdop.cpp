#include <libTools/math/kdop.h>
#include <math/dag_mathAng.h>
#include <EASTL/sort.h>

struct IndicesSortHelper
{
  float angle;
  float len;
  int gIdx;
  int lIdx;
};

static bool compare_helper(const IndicesSortHelper &l, const IndicesSortHelper &r)
{
  if (l.angle != r.angle)
    return l.angle < r.angle;
  if (l.len != r.len)
    return l.len < r.len;
  return l.gIdx < r.gIdx;
}

static void push_to_plane_vertices(PlaneDefinition &plane_definition, const Point3 &p, int idx, float epsilon)
{
  auto isNear = [p, epsilon](const Point3 &vert) { return lengthSq(p - vert) < pow(epsilon, 2); };

  if (eastl::find_if(plane_definition.vertices.begin(), plane_definition.vertices.end(), isNear) == plane_definition.vertices.end())
  {
    plane_definition.vertices.push_back(p);
    plane_definition.indices.push_back(idx);
  }
}

PlaneDefinition::PlaneDefinition(const Point3 &plane_normal) : planeNormal(plane_normal), limit(-FLT_MAX), area(0) {}

void Kdop::setup6Dop()
{
  planeDefinitions.push_back(Point3{1.0f, 0.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 1.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, 1.0f});
  planeDefinitions.push_back(Point3{-1.0f, 0.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, -1.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, -1.0f});
}

void Kdop::setup14Dop()
{
  planeDefinitions.push_back(Point3{1.0f, 0.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 1.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, 1.0f});
  planeDefinitions.push_back(Point3{-1.0f, 0.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, -1.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, -1.0f});
  const float corner = 1 / sqrt(3.0f);
  planeDefinitions.push_back(Point3{corner, corner, corner});
  planeDefinitions.push_back(Point3{corner, corner, -corner});
  planeDefinitions.push_back(Point3{corner, -corner, corner});
  planeDefinitions.push_back(Point3{corner, -corner, -corner});
  planeDefinitions.push_back(Point3{-corner, corner, corner});
  planeDefinitions.push_back(Point3{-corner, corner, -corner});
  planeDefinitions.push_back(Point3{-corner, -corner, corner});
  planeDefinitions.push_back(Point3{-corner, -corner, -corner});
}

void Kdop::setup18Dop()
{
  const float quarterPi = PI / 4;
  float x = sin(PI / 4);
  float y = 0.0f;
  for (int i = 0; i < 4; ++i)
  {
    const float iterHalfPi = i * HALFPI;
    const float iterHalfPiPlusQuarterPi = iterHalfPi + quarterPi;
    planeDefinitions.push_back(Point3{cos(iterHalfPi), sin(iterHalfPi), 0.0f});
    planeDefinitions.push_back(Point3{cos(iterHalfPiPlusQuarterPi), sin(iterHalfPiPlusQuarterPi), 0.0f});
    planeDefinitions.push_back(Point3{x, y, cos(quarterPi)});
    planeDefinitions.push_back(Point3{x, y, -cos(quarterPi)});
    if (i == 1)
      y = -y;
    eastl::swap(x, y);
  }
  planeDefinitions.push_back(Point3{0.0f, 0.0f, 1.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, -1.0f});
}

void Kdop::setup26Dop()
{
  const float quarterPi = PI / 4;
  float x = sin(PI / 4);
  float y = 0.0f;
  for (int i = 0; i < 4; ++i)
  {
    const float iterHalfPi = i * HALFPI;
    const float iterHalfPiPlusQuarterPi = iterHalfPi + quarterPi;
    planeDefinitions.push_back(Point3{cos(iterHalfPi), sin(iterHalfPi), 0.0f});
    planeDefinitions.push_back(Point3{cos(iterHalfPiPlusQuarterPi), sin(iterHalfPiPlusQuarterPi), 0.0f});
    planeDefinitions.push_back(Point3{x, y, cos(quarterPi)});
    planeDefinitions.push_back(Point3{x, y, -cos(quarterPi)});
    if (i == 1)
      y = -y;
    eastl::swap(x, y);
  }
  const float corner = 1 / sqrt(3.0f);
  planeDefinitions.push_back(Point3{corner, corner, corner});
  planeDefinitions.push_back(Point3{corner, corner, -corner});
  planeDefinitions.push_back(Point3{corner, -corner, corner});
  planeDefinitions.push_back(Point3{corner, -corner, -corner});
  planeDefinitions.push_back(Point3{-corner, corner, corner});
  planeDefinitions.push_back(Point3{-corner, corner, -corner});
  planeDefinitions.push_back(Point3{-corner, -corner, corner});
  planeDefinitions.push_back(Point3{-corner, -corner, -corner});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, 1.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, -1.0f});
}

void Kdop::setupKdopFixedX(int seg)
{
  planeDefinitions.push_back(Point3{1.0f, 0.0f, 0.0f});
  planeDefinitions.push_back(Point3{-1.0f, 0.0f, 0.0f});
  for (int i = 0; i < seg; ++i)
  {
    const float iterArg = i * TWOPI / seg - HALFPI;
    planeDefinitions.push_back(Point3{0.0f, sin(iterArg), cos(iterArg)});
  }
}

void Kdop::setupKdopFixedY(int seg)
{
  planeDefinitions.push_back(Point3{0.0f, 1.0f, 0.0f});
  planeDefinitions.push_back(Point3{0.0f, -1.0f, 0.0f});
  for (int i = 0; i < seg; ++i)
  {
    const float iterArg = i * TWOPI / seg - HALFPI;
    planeDefinitions.push_back(Point3{cos(iterArg), 0.0f, sin(iterArg)});
  }
}

void Kdop::setupKdopFixedZ(int seg)
{
  planeDefinitions.push_back(Point3{0.0f, 0.0f, 1.0f});
  planeDefinitions.push_back(Point3{0.0f, 0.0f, -1.0f});
  for (int i = 0; i < seg; ++i)
  {
    const float iterArg = i * TWOPI / seg - HALFPI;
    planeDefinitions.push_back(Point3{cos(iterArg), sin(iterArg), 0.0f});
  }
}

void Kdop::setupCustomKdop(int seg1, int seg2)
{
  const float cosSeg2 = cos(TWOPI / seg2);
  const float sinSeg2 = sin(TWOPI / seg2);
  const float cosSeg1 = cos(TWOPI / seg1);
  const float sinSeg1 = sin(TWOPI / seg1);
  Point3 n{0.0f, -1.0f, 0.0f};
  for (int i = 0; i < seg2; ++i)
  {
    planeDefinitions.push_back(n);
    n = normalize(Point3{n.x * cosSeg2 - n.y * sinSeg2, n.x * sinSeg2 + n.y * cosSeg2, n.z});
  }
  for (int i = 0; i < seg2; ++i)
  {
    n = planeDefinitions[i].planeNormal;
    for (int j = 1; j < seg1; ++j)
    {
      n = normalize(Point3{n.x * cosSeg1 + n.z * sinSeg1, n.y, n.x * -sinSeg1 + n.z * cosSeg1});
      float epsilon = 0.001f;
      auto isNear = [n, epsilon](const PlaneDefinition &p) { return lengthSq(n - p.planeNormal) < pow(epsilon, 2); };
      if (!(eastl::find_if(planeDefinitions.begin(), planeDefinitions.end(), isNear) != planeDefinitions.end()))
      {
        n.normalize();
        planeDefinitions.push_back(n);
      }
    }
  }
}


void Kdop::setPreset(KdopPreset set_preset, float cut_off_threshold, int seg1, int seg2)
{
  clear();
  cutOffThreshold = cut_off_threshold;

  switch (set_preset)
  {
    case KdopPreset::SET_EMPTY: break;

    case KdopPreset::SET_6DOP: setup6Dop(); break;

    case KdopPreset::SET_14DOP: setup14Dop(); break;

    case KdopPreset::SET_18DOP: setup18Dop(); break;

    case KdopPreset::SET_26DOP: setup26Dop(); break;

    case KdopPreset::SET_KDOP_FIXED_X: setupKdopFixedX(seg1); break;

    case KdopPreset::SET_KDOP_FIXED_Y: setupKdopFixedY(seg1); break;

    case KdopPreset::SET_KDOP_FIXED_Z: setupKdopFixedZ(seg1); break;

    case KdopPreset::SET_CUSTOM_KDOP: setupCustomKdop(seg1, seg2); break;
  }
}

void Kdop::setRotation(const Point3 &rot_deg)
{
  Quat quat;
  euler_to_quat(rot_deg.y * DEG_TO_RAD, rot_deg.z * DEG_TO_RAD, rot_deg.x * DEG_TO_RAD, quat);
  rm.makeTM(quat);
}

void Kdop::reset()
{
  rm.identity();
  clear();
}

bool Kdop::isPointInside(const Point3 &p)
{
  const Point3 &o = center;
  for (int i = 0; i < planeDefinitions.size(); ++i)
  {
    const Point3 &n = planeDefinitions[i].planeNormal;
    float t = (p - o) * n;
    if (planeDefinitions[i].limit < t - eps)
    {
      return false;
    }
  }
  return true;
}

void Kdop::calcKdop()
{
  for (int i = 0; i < planeDefinitions.size(); ++i)
  {
    eps = max(eps, planeDefinitions[i].limit * 1e-5f);
  }

  calcPlanes();
  calcIndices();
  if (cutOffThreshold > 0.0f)
  {
    calcArea();
    cutOffPlanes();
  }
}

void Kdop::calcPlanes()
{
  for (int i = 0; i < planeDefinitions.size(); ++i)
  {
    for (int j = i + 1; j < planeDefinitions.size(); ++j)
    {
      for (int k = j + 1; k < planeDefinitions.size(); ++k)
      {
        const Point3 &n1 = planeDefinitions[i].planeNormal;
        const Point3 &n2 = planeDefinitions[j].planeNormal;
        const Point3 &n3 = planeDefinitions[k].planeNormal;
        const float f1 = -planeDefinitions[i].limit;
        const float f2 = -planeDefinitions[j].limit;
        const float f3 = -planeDefinitions[k].limit;

        Point3 n1xn2 = cross(n1, n2);
        float det = dot(n1xn2, n3);
        if (fabsf(det) < 0.001f)
          continue;
        Point3 planeIntersection = (cross(n3, n2) * f1 + cross(n1, n3) * f2 - n1xn2 * f3) / det + center;

        if (isPointInside(planeIntersection))
        {
          float epsilon = 10.0f * eps;
          auto isNear = [planeIntersection, epsilon](const Point3 &vert) {
            return lengthSq(planeIntersection - vert) < pow(epsilon, 2);
          };

          auto it = eastl::find_if(vertices.begin(), vertices.end(), isNear);
          int idx = it - vertices.begin();
          if (it == vertices.end())
            vertices.push_back(planeIntersection);
          push_to_plane_vertices(planeDefinitions[i], planeIntersection, idx, epsilon);
          push_to_plane_vertices(planeDefinitions[j], planeIntersection, idx, epsilon);
          push_to_plane_vertices(planeDefinitions[k], planeIntersection, idx, epsilon);
        }
      }
    }
  }
}

void Kdop::calcIndices()
{
  for (auto &planeDefinition : planeDefinitions)
  {
    if (planeDefinition.vertices.size() < 3)
      continue;

    Point3 x = normalize(planeDefinition.vertices[0] - planeDefinition.vertices[1]);
    Point3 y = normalize(cross(x, planeDefinition.planeNormal));
    TMatrix tm;
    tm.setcol(0, x.x, y.x, 0.0f);
    tm.setcol(1, x.y, y.y, 0.0f);
    tm.setcol(2, x.z, y.z, 0.0f);
    tm.setcol(3, 0.0f, 0.0f, 0.0f);
    dag::Vector<IndicesSortHelper> angles;
    dag::Vector<IndicesSortHelper> queue;
    dag::Vector<Point2> points;
    int minLeftIdx = 0;
    Point3 minLeft{FLT_MAX, FLT_MAX, FLT_MAX};

    // Graham scan
    for (int i = 0; i < planeDefinition.vertices.size(); ++i)
    {
      Point3 p = tm * planeDefinition.vertices[i];
      points.push_back({p.x, p.y});
      if (p.y < minLeft.y || (is_equal_float(p.y, minLeft.y) && p.x < minLeft.x))
      {
        minLeftIdx = i;
        minLeft = p;
      }
    }
    for (int i = 0; i < points.size(); ++i)
    {
      float dy = points[i].y - minLeft.y;
      float dx = points[i].x - minLeft.x;
      angles.push_back({atan2(dy, dx), Point2(dx, dy).length(), planeDefinition.indices[i], i});
    }
    queue.push_back(angles[minLeftIdx]);
    angles.erase(angles.begin() + minLeftIdx);
    eastl::sort(angles.begin(), angles.end(), compare_helper);

    queue.push_back(angles[0]);
    for (int i = 1; i < angles.size(); ++i)
    {
      float cr = -1.0f;
      while (cr < 0 && queue.size() > 1)
      {
        const Point2 &a = points[queue[queue.size() - 2].lIdx];
        const Point2 &b = points[queue[queue.size() - 1].lIdx];
        const Point2 &c = points[angles[i].lIdx];
        Point2 u = b - a;
        Point2 v = c - b;
        cr = u.x * v.y - u.y * v.x;
        if (cr < 0)
        {
          queue.pop_back();
        }
      }
      queue.push_back(angles[i]);
    }

    planeDefinition.indices.clear();
    planeDefinition.vertices.clear();
    for (int i = 0; i < queue.size(); ++i)
    {
      planeDefinition.indices.push_back(queue[i].gIdx);
      planeDefinition.vertices.push_back(vertices[queue[i].gIdx]);
    }

    for (int i = 2; i < planeDefinition.indices.size(); ++i)
    {
      indices.push_back(planeDefinition.indices[0]);
      indices.push_back(planeDefinition.indices[i - 1]);
      indices.push_back(planeDefinition.indices[i]);
    }
  }
}

void Kdop::calcArea()
{
  for (auto &planeDefinition : planeDefinitions)
  {
    for (int i = 2; i < planeDefinition.vertices.size(); ++i)
    {
      const Point3 &p1 = planeDefinition.vertices[0];
      const Point3 &p2 = planeDefinition.vertices[i - 1];
      const Point3 &p3 = planeDefinition.vertices[i];
      planeDefinition.area += 0.5 * length(cross(p2 - p1, p3 - p1));
      if (planeDefinition.area > areaLimit)
      {
        areaLimit = planeDefinition.area;
      }
    }
  }
}

void Kdop::cutOffPlanes()
{
  const float cutOffArea = areaLimit * cutOffThreshold * 0.01f;
  for (int i = 0; i < planeDefinitions.size(); ++i)
  {
    if (planeDefinitions[i].area < cutOffArea)
    {
      planeDefinitions.erase(planeDefinitions.begin() + i--);
    }
  }
  cutOffThreshold = 0.0f;
  vertices.clear();
  indices.clear();
  for (auto &planeDefinition : planeDefinitions)
  {
    planeDefinition.vertices.clear();
    planeDefinition.indices.clear();
  }
  calcKdop();
}

void Kdop::clear()
{
  vertices.clear();
  indices.clear();
  planeDefinitions.clear();
  center.zero();
  eps = 0.0f;
  cutOffThreshold = 0.0f;
  areaLimit = -FLT_MAX;
}
