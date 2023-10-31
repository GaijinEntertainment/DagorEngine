#include "roadsSnapshot.h"
#include "hmlObjectsEditor.h"
#include "hmlSplineObject.h"
#include "hmlSplinePoint.h"
#include "crossRoad.h"
#include "roadUtil.h"
#include <de3_splineClassData.h>


static bool isRoadSpline(SplineObject &o)
{
  if (o.isPoly())
    return false;

  for (int i = o.points.size() - 1; i >= 0; i--)
    if (getRoad(o.points[i], false))
      return true;
  return false;
}

static IRoadsProvider::RoadSpline *findRoad(dag::Span<IRoadsProvider::RoadSpline> rspl, SplineObject *o,
  dag::ConstSpan<SplineObject *> used_o)
{
  for (int i = 0; i < used_o.size(); i++)
    if (used_o[i] == o)
      return &rspl[i];
  return NULL;
}


RoadsSnapshot::RoadsSnapshot(HmapLandObjectEditor &objEd) : rspl(midmem), rc(midmem), rcr(midmem), rpt(midmem)
{
  Tab<SplineObject *> usedSpl(tmpmem);
  int rs_num = 0, rc_num = 0, rcr_num = 0, rpt_num = 0;

  // calculate required size
  for (int i = 0; i < objEd.objectCount(); i++)
  {
    SplineObject *p = RTTI_cast<SplineObject>(objEd.getObject(i));
    if (!p || !isRoadSpline(*p))
      continue;
    rs_num++;
    rpt_num += p->points.size();
  }
  for (int i = 0; i < objEd.crossRoads.size(); i++)
  {
    rcr_num += objEd.crossRoads[i]->points.size();
    rc_num++;
  }

  // allocate storage
  rspl.reserve(rs_num);
  rc.reserve(rc_num);
  rcr.reserve(rcr_num);
  rpt.reserve(rpt_num);

  // build snapshot
  for (int i = 0; i < objEd.objectCount(); i++)
  {
    SplineObject *p = RTTI_cast<SplineObject>(objEd.getObject(i));
    if (!p || !isRoadSpline(*p))
      continue;

    p->getSpline();
    usedSpl.push_back(p);

    IRoadsProvider::RoadSpline &rs = rspl.push_back();
    int ptidx = append_items(rpt, p->points.size());

    rs.closed = p->isClosed();
    rs.pt.set(&rpt[ptidx], p->points.size());
    rs.curve = p->getBezierSpline();

    const splineclass::AssetData *asset = NULL;
    for (int j = 0; j < rs.pt.size(); j++)
    {
      asset = getRoad(p->points[j], false) ? p->points[j]->getSplineClass() : NULL;

      rpt[ptidx + j].pt = p->points[j]->getPt();
      rpt[ptidx + j].relIn = p->points[j]->getPtEffRelBezierIn();
      rpt[ptidx + j].relOut = p->points[j]->getPtEffRelBezierOut();
      // rpt[ptidx+j].upDir = p->points[j]->getUpDir();
      rpt[ptidx + j].asset = asset;
    }
  }
  for (int i = 0; i < objEd.crossRoads.size(); i++)
  {
    IRoadsProvider::RoadCross &c = rc.push_back();
    int ptidx = append_items(rcr, objEd.crossRoads[i]->points.size());
    c.roads.set(&rcr[ptidx], objEd.crossRoads[i]->points.size());
    for (int j = 0; j < c.roads.size(); j++)
    {
      rcr[ptidx + j].road = findRoad(make_span(rspl), objEd.crossRoads[i]->points[j]->spline, usedSpl);
      rcr[ptidx + j].ptIdx = objEd.crossRoads[i]->points[j]->arrId;
    }
  }

  G_ASSERT(rspl.size() <= rs_num);
  G_ASSERT(rc.size() <= rc_num);
  G_ASSERT(rcr.size() <= rcr_num);
  G_ASSERT(rpt.size() <= rpt_num);

  roads = rspl;
  cross = rc;
}


#include <debug/dag_debug.h>

void RoadsSnapshot::debugDump()
{
  debug("=== %d roads:", roads.size());
  for (int i = 0; i < roads.size(); i++)
  {
    debug("  roads %d: %p, %d pts", i, &roads[i], roads[i].pt.size());
    for (int j = 0; j < roads[i].pt.size(); j++)
      debug("    pt %d: pos=" FMT_P3 " in=" FMT_P3 " out=" FMT_P3 " asset=%p", j, P3D(roads[i].pt[j].pt), P3D(roads[i].pt[j].relIn),
        P3D(roads[i].pt[j].relOut), roads[i].pt[j].asset);
  }

  debug("=== %d crossroads:", cross.size());
  for (int i = 0; i < cross.size(); i++)
  {
    debug("  cross %d: %d roads", i, cross[i].roads.size());
    for (int j = 0; j < cross[i].roads.size(); j++)
      debug("    road %d: road %p ptIdx=%d", j, cross[i].roads[j].road, cross[i].roads[j].ptIdx);
  }
  debug("---");
}
