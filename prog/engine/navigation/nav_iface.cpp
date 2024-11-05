// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <navigation/dag_navInterface.h>
#include <navigation/dag_navigation.h>
#include <navigation/dag_navMesh.h>
#include <navigation/dag_navNode.h>
#include <navigation/dag_navPathfind.h>
#include <math/dag_math3d.h>
#include <generic/dag_tabUtils.h>

static unsigned int ref_counter = 0;
float globalTime = 0.f;

ConVar nav_edit = 0;
ConVar nav_quicksave = 0;
ConVar nav_show_approach_points = 0;
ConVar nav_show_danger = 0;

/*****************************************************/

NavigatorOptions::NavigatorOptions() :
  GenerationStepSize(0.5f),
  SpawnPointsStepSize(5.0f),
  JumpCrouchHeight(0.5f),
  MaxSurfaceSlope(0.7f), // MaxUnitYSlope
  DeathDrop(3.0f),
  HumanRadius(0.2f),
  HumanHeightCrouch(1.2f), // HalfHumanHeight(1.2f),
  HumanHeight(1.8f),
  HumanDownHeightCheck(0.4f),
  buildHidingAndSnipingSpots(false)
//  HumanCapsulePointUp(1.6f),
//  HumanCapsulePointDown(0.6f)
{}

NavigatorOptions navOptions;
NavigatorOptions navOptionsDefault;

//};

/*****************************************************/


Navigator::Navigator()
{
  if (!TheNavMesh)
    TheNavMesh = new CNavMesh;
  ref_counter++;
};

Navigator::~Navigator()
{
  ref_counter--;
  if (!ref_counter)
  {
    delete TheNavMesh;
    TheNavMesh = NULL;
  }
};

void Navigator::clear()
{
  delete TheNavMesh;
  TheNavMesh = new CNavMesh;
  ::navOptions = navOptionsDefault;
}

/*****************************************************/

/*****************************************************/

NavErrorType Navigator::load(IGenLoad &crd) { return TheNavMesh->Load(crd); }

/*****************************************************/

unsigned int Navigator::getAreaCount() const { return TheNavMesh->GetNavAreaCount(); }

INavArea *Navigator::getAreaByIndex(unsigned int index) const { return TheNavAreaList[index]; }

/*
unsigned int Navigator::getAreaHidePointCount(unsigned int iArea) const
{
  CNavArea * area = TheNavAreaList[ iArea ];
  return area->GetHidingSpotList()->Count();
}

NavigatorHidePoint Navigator::getAreaHidePoint(unsigned int iArea, unsigned int iPoint) const
{
  CNavArea * area = TheNavAreaList[ iArea ];
  HidingSpot *spot = area->GetHidingSpotList()->Element( iPoint );
  NavigatorHidePoint pt;
  pt.pos = *(spot->GetPosition());
  pt.flags = spot->GetFlags();
  return pt;
}
*/

Point3 Navigator::getAreaPortal(INavArea *areafrom, INavArea *areato, NavDirType dir) const
{
  CNavArea *areaFrom = (CNavArea *)areafrom;
  CNavArea *areaTo = (CNavArea *)areato;
  Point3 res;
  float half_width;
  areaFrom->ComputePortal(areaTo, (NavDirType)dir, &res, &half_width);
  res.y = areaFrom->GetY(res.x, res.z);
  return res;
}

INavArea *Navigator::getAreaByPos(const Point3 &pos, bool strict, bool *outside)
{
  if (outside)
    *outside = false;
  CNavArea *area = TheNavMesh->GetNavArea(&pos);
  //  if (area)
  //  {
  //    debug("beneath " FMT_P3 "", P3D(*area->GetCenter()));
  //  }
  if (!area && !strict)
  {
    area = TheNavMesh->GetNearestNavArea(&pos);
    if (outside)
      *outside = true;
    //    if (area)
    //      debug("closest " FMT_P3 "", P3D(*area->GetCenter()));
    //    else
    //      debug("no area at all");
  }
  return area;
}

/*
INavArea* Navigator::getAreaByPos(const Point3& pos, real beneathlimit)
{
  return TheNavMesh->GetNavArea(&pos, beneathlimit);
}
*/

void Navigator::addDirectionVector(Point3 *v, NavDirType dir, float amount) { ::AddDirectionVector(v, dir, amount); }

/*****************************************************/

bool Navigator::findPath(INavArea *from, INavArea *to, Tab<Point3> &path, Point3 *realGoal, bool includeFirst, PathFindMode mode)
{
  path.resize(0);

  if (!from || !to || from == to)
    return false;

  CNavArea *from_area = (CNavArea *)from;
  CNavArea *to_area = (CNavArea *)to;
  if ((from_area->GetZone() == UNKNOWN_ZONE) || (to_area->GetZone() == UNKNOWN_ZONE))
  {
    debug("Nav: found unknown zone...");
    return false;
  }

  if ((from_area->GetZone() == SOLO_ZONE) || (from_area->GetZone() == SOLO_ZONE))
  {
    return false; // that's what we need zones for...
  }

  if (from_area->GetZone() != to_area->GetZone())
  {
    return false; // that's what we need zones for...
  }


  if (mode == PATHMODE_RANDOM)
  {
    ShortestRandomPathCost cost;
    if (NavAreaBuildPath(from_area, to_area, realGoal, cost) == false)
    {
      return false;
    }
  }
  else
  {
    ShortestPathCost cost;
    if (NavAreaBuildPath(from_area, to_area, realGoal, cost) == false)
    {
      return false;
    }
  }

  // OK... we've found a path

  // Do we need to add actual goal point to path?
  Point3 lastPoint = *(to_area->GetCenter());
  if (realGoal)
  {
    lastPoint = *realGoal;
  }

  path.push_back(lastPoint);

  CNavArea *current = to_area;
  while (current)
  {
    CNavArea *next = current->GetParent();
    if (next)
    {
      NavTraverseType how = current->GetParentHow();
      if (how == GO_NORTH || how == GO_EAST || how == GO_SOUTH || how == GO_WEST)
      {
        NavDirType dirType = NavDirType((int)how);
        /*
                Point3 posInPortal;
                next->ComputeClosestPointInPortal(current, dirType, next->GetCenter(), &posInPortal);
                posInPortal.y = (current->getCenter().y + current->getCenter().y)*0.5f;
                path.push_back(posInPortal);
        */
        Point3 nm = getAreaPortal(next, current, dirType);
        path.push_back(nm);
      }

      /*     if (mode==PATHMODE_RANDOM)
             lastPoint = (next->GetRandom());
           else*/
      lastPoint = *(next->GetCenter());
      if (includeFirst || next->GetParent())
        path.push_back(lastPoint);
    }
    current = next;
  };
  return true;
}


bool Navigator::findPath(const Point3 &from, Point3 to, Tab<Point3> &path, PathFindMode mode)
{
  path.resize(0);
  bool outside = false;

  INavArea *areafrom = getAreaByPos(from, false, &outside);
  if (!areafrom)
    return false;

  INavArea *areato = getAreaByPos(to);
  if (!areato)
    return false;

  if (areafrom == areato)
  {
    path.resize(1);
    path[0] = to;
    return true;
  }

  return findPath(areafrom, areato, path, &to, outside, mode);
}


INavArea *Navigator::getClosestAreaByPosAndZone(const Point3 &pos, unsigned int zone)
{
  if (zone == UNKNOWN_ZONE)
    return NULL;
  if (zone == SOLO_ZONE)
    return NULL;
  CNavArea *area = TheNavMesh->GetNearestNavAreaInZone(&pos, zone);
  return area;
}

void Navigator::getAreasByBox(const BBox3 &box, Tab<INavArea *> &list) const
{
  TheNavMesh->GetListNavAreas(box, (Tab<CNavArea *> &)list);
}


INavArea *Navigator::createArea(const Point3 &nw, const Point3 &ne, const Point3 &se, const Point3 &sw)
{
  CNavArea *area = new CNavArea(&nw, &ne, &se, &sw);
  TheNavAreaList.AddToTail(area);
  TheNavMesh->AddNavArea(area);

  debug("createArea p1:%f,%f,%f p2:%f,%f,%f swy:%f ney:%f", area->getP1().x, area->getP1().y, area->getP1().z, area->getP2().x,
    area->getP2().y, area->getP2().z, area->getSwY(), area->getNeY());

  real stepsize = getUsableGenerationStepSize();

  // connect adjacent areas
  {
    const real tolerancexz = stepsize / 4;
    const real tolerancey = stepsize;

    real miny = min(min(nw.y, ne.y), min(se.y, sw.y));
    real maxy = max(max(nw.y, ne.y), max(se.y, sw.y));

    miny -= tolerancey;
    maxy += tolerancey;

    Tab<INavArea *> list(midmem_ptr());
    BBox3 box;

    // south
    box = BBox3(Point3(area->getP1().x + tolerancexz, miny, area->getP2().z),
      Point3(area->getP2().x - tolerancexz, maxy, area->getP2().z + tolerancexz));
    getAreasByBox(box, list);
    for (int n = 0; n < list.size(); n++)
    {
      CNavArea *neararea = (CNavArea *)list[n];
      if (neararea == area)
        continue;
      area->ConnectTo(neararea, SOUTH);
      neararea->ConnectTo(area, NORTH);
    }


    // north
    box = BBox3(Point3(area->getP1().x + tolerancexz, miny, area->getP1().z - tolerancexz),
      Point3(area->getP2().x - tolerancexz, maxy, area->getP1().z));
    getAreasByBox(box, list);
    for (int n = 0; n < list.size(); n++)
    {
      CNavArea *neararea = (CNavArea *)list[n];
      if (neararea == area)
        continue;
      area->ConnectTo(neararea, NORTH);
      neararea->ConnectTo(area, SOUTH);
    }


    // west
    box = BBox3(Point3(area->getP1().x - tolerancexz, miny, area->getP1().z + tolerancexz),
      Point3(area->getP1().x, maxy, area->getP2().z - tolerancexz));
    getAreasByBox(box, list);
    for (int n = 0; n < list.size(); n++)
    {
      CNavArea *neararea = (CNavArea *)list[n];
      if (neararea == area)
        continue;
      area->ConnectTo(neararea, WEST);
      neararea->ConnectTo(area, EAST);
    }

    // east
    box = BBox3(Point3(area->getP2().x, miny, area->getP1().z + tolerancexz),
      Point3(area->getP2().x + tolerancexz, maxy, area->getP2().z - tolerancexz));
    getAreasByBox(box, list);
    for (int n = 0; n < list.size(); n++)
    {
      CNavArea *neararea = (CNavArea *)list[n];
      if (neararea == area)
        continue;
      area->ConnectTo(neararea, EAST);
      neararea->ConnectTo(area, WEST);
    }
  }

  return area;
}

void Navigator::deleteArea(INavArea *area)
{
  G_ASSERT(area);
  bool removed = TheNavAreaList.FindAndRemove((CNavArea *)area);
  G_ASSERT(removed);
  delete (CNavArea *)area;
}

real Navigator::getGenerationStepSize() const { return TheNavMesh->getGenerationStepSize(); }

void Navigator::snapByGenerationStepSize(real &v)
{
  real stepsize = getUsableGenerationStepSize();
  // internal bug fix check

  if (v > 0.0f)
    v += stepsize * 0.5f;
  else
    v -= stepsize * 0.5f;
  v /= stepsize;
  v = (int)v;
  v *= stepsize;
}

real Navigator::getUsableGenerationStepSize() const { return TheNavMesh->getUsableGenerationStepSize(); }

Point3 Navigator::getRandomPoint(INavArea *area_)
{
  CNavArea *area = (CNavArea *)area_;
  return area->GetRandom();
}

const Tab<NavHintPoint *> &Navigator::getHintPoints() const { return TheNavMesh->getHintPoints(); }

NavHintPoint *Navigator::createHintPoint(INavArea *parent_area) { return TheNavMesh->createHintPoint(parent_area); }

void Navigator::destroyHintPoint(NavHintPoint *point) { TheNavMesh->deleteHintPoint(point); }

void Navigator::calculateHintPointsParentAreas()
{
  const Tab<NavHintPoint *> &points = getHintPoints();
  for (int n = 0; n < points.size(); n++)
    points[n]->calculateParentArea();
}

NavHintPoint::NavHintPoint() : links(midmem)
{
  //  navigator = NULL;
  parentArea = NULL;
  type = (Type)0;
  position.zero();
  radius = 1.0f;
}

NavHintPoint::~NavHintPoint()
{
  setParentArea(NULL);
  while (links.size())
    removeLink(links[0]);
}

void NavHintPoint::setParentArea(INavArea *area)
{
  if (parentArea)
    ((CNavArea *)parentArea)->removeHintPoint(this);
  parentArea = area;
  if (parentArea)
    ((CNavArea *)parentArea)->addHintPoint(this);
}

void NavHintPoint::addLink(NavHintPoint *point)
{
  removeLink(point);

  links.push_back(point);
  point->links.push_back(this);
}

void NavHintPoint::removeLink(NavHintPoint *point)
{
  tabutils::erase(links, point);
  tabutils::erase(point->links, this);
}

void NavHintPoint::setPosition(const Point3 &pos)
{
  this->position = pos;
  calculateParentArea();
}

void NavHintPoint::calculateParentArea()
{
  Point3 p = getPosition();
  CNavArea *area = TheNavMesh->GetNavArea(&p, .1f);
  setParentArea(area);
}
