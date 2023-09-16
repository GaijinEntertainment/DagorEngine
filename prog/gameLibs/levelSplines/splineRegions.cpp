#include <levelSplines/splineRegions.h>
#include <math/dag_polyUtils.h>
#include <math/dag_math2d.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <EASTL/array.h>
#include <EASTL/sort.h>

bool splineregions::RegionConvex::checkPoint(const Point2 &pt) const
{
  for (const Line2 &line : lines)
    if (line.distance(pt) >= 0.f)
      return false;
  return true;
}

bool splineregions::SplineRegion::checkPoint(const Point2 &pt) const
{
  for (const RegionConvex &convex : convexes)
    if (convex.checkPoint(pt))
      return true;
  return false;
}

Point3 splineregions::SplineRegion::getAnyBorderPoint() const
{
  if (border.empty())
  {
    logerr("getAnyBorderPoint() called on empty SplineRegion");
    return Point3();
  }
  return border[grnd() % border.size()];
}

Point3 splineregions::SplineRegion::getRandomBorderPoint() const
{
  if (border.empty())
  {
    logerr("getRandomBorderPoint() called on empty SplineRegion");
    return Point3();
  }
  const int idx = grnd() % border.size();
  return lerp(border[idx], border[(idx + 1) % border.size()], gfrnd());
}

Point3 splineregions::SplineRegion::getRandomPoint() const
{
  if (convexes.empty() || border.empty())
  {
    logerr("getRandomPoint() called on empty SplineRegion");
    return Point3();
  }
  float sample = gfrnd() * area;
  int idx = 0;
  sample -= convexes[0].area;
  while (idx < convexes.size() && sample > 0)
  {
    idx++;
    sample -= convexes[idx].area;
  }
  const RegionConvex &convex = convexes[idx];
  float d;
  Point2 norm, st0, st1, st2, end0, end1, end2, int01, int12, int20;
  d = convex.lines[0].d;
  norm = convex.lines[0].norm;
  st0 = norm * (-d);
  end0 = st0 + Point2(norm.y, -norm.x);
  d = convex.lines[1].d;
  norm = convex.lines[1].norm;
  st1 = norm * (-d);
  end1 = st1 + Point2(norm.y, -norm.x);
  d = convex.lines[2].d;
  norm = convex.lines[2].norm;
  st2 = norm * (-d);
  end2 = st2 + Point2(norm.y, -norm.x);
  get_lines_intersection(st0, end0, st1, end1, int01);
  get_lines_intersection(st1, end1, st2, end2, int12);
  get_lines_intersection(st2, end2, st0, end0, int20);
  return Point3::xVy(get_random_point_in_triangle(int01, int12, int20), border[0].y);
}

const char *splineregions::SplineRegion::getNameStr() const { return name.str(); }

void splineregions::construct_region_from_spline(SplineRegion &region, const levelsplines::Spline &spline, float expand_dist)
{
  size_t ptsCnt = spline.pathPoints.size();
  Tab<Point2> pts(framemem_ptr());
  pts.reserve(ptsCnt);

  for (const auto &point : spline.pathPoints)
    pts.push_back(Point2::xz(point.pt));
  float expandDist = is_poly_ccw(pts) ? expand_dist : -expand_dist;
  clear_and_shrink(pts);

  for (size_t i = 0; i < ptsCnt; ++i)
  {
    region.originalBorder.push_back(spline.pathPoints[i].pt);
    Point2 p0 = Point2::xz(spline.pathPoints[i == 0 ? ptsCnt - 1 : i - 1].pt);
    Point2 p1 = Point2::xz(spline.pathPoints[i].pt);
    Point2 p2 = Point2::xz(spline.pathPoints[(i + 1) % ptsCnt].pt);
    Line2 ln0 = Line2(p0, p1);
    Line2 ln1 = Line2(p1, p2);
    ln0.d += expandDist;
    ln1.d += expandDist;

    Point2 intPt;
    if (is_equal_float(fabs(ln0.norm * ln1.norm), 1.f))
      continue;
    G_VERIFY(get_lines_intersection(ln0, ln1, intPt));

    pts.push_back(intPt);
    Point3 bordPt = Point3::xVy(intPt, spline.pathPoints[i].pt.y);
    region.border.push_back(bordPt);
  }

  region.area = 0;
  triangulate_poly(pts, region.triangulationIndexes);
  for (int i = 0; i < region.triangulationIndexes.size(); i += 3)
  {
    auto &p1 = pts[region.triangulationIndexes[i + 0]];
    auto &p2 = pts[region.triangulationIndexes[i + 1]];
    auto &p3 = pts[region.triangulationIndexes[i + 2]];
    RegionConvex &conv = region.convexes.push_back();
    conv.lines.push_back(Line2(p1, p2));
    conv.lines.push_back(Line2(p2, p3));
    conv.lines.push_back(Line2(p3, p1));
    conv.area = fabs(p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)) / 2.f;
    region.area += conv.area;
  }

  // These are approximate calculations
  BBox3 box;
  for (const auto &point : spline.pathPoints)
    box += point.pt;
  region.center = box.center();
  region.boundingRadius = 0;
  for (const auto &point : spline.pathPoints)
    region.boundingRadius = max((region.center - point.pt).lengthSq(), region.boundingRadius);
  region.boundingRadius = sqrt(region.boundingRadius);

  region.name = spline.name;
  region.isVisible = true;
}

void splineregions::load_regions_from_splines(LevelRegions &regions, dag::ConstSpan<levelsplines::Spline> splines,
  const DataBlock &blk)
{
  if (blk.isEmpty())
    return;
  regions.reserve(splines.size());
  for (const levelsplines::Spline &spline : splines)
  {
    const DataBlock *regionBlk = blk.getBlockByNameEx(spline.name.str(), nullptr);
    if (!regionBlk)
      continue;
    SplineRegion &region = regions.push_back();
    construct_region_from_spline(region, spline, regionBlk->getReal("expand", 0.f));
    region.name = SimpleString(regionBlk->getStr("name", spline.name.str()));
    region.isVisible = regionBlk->getBool("visible", true);
  }
}
