// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <navigation/dag_navInterface.h>
#include <ioSys/dag_genIo.h>
#include <navigation/dag_navMesh.h>

#define NAVIGATION_MESH_FILE_VERSION 4

void Navigator::save(IGenSave &crd) { TheNavMesh->Save(crd); }

//--------------------------------------------------------------------------------------------------------------
/**
 * Save a navigation area to the opened binary stream
 */
void CNavArea::Save(IGenSave &crd) const
{
  // save ID
  crd.writeInt(m_id);

  // save attribute flags
  crd.writeInt(m_attributeFlags);

  // save zone info
  crd.writeInt(m_zone);

  // save extent of area
  crd.writeReal(m_extent.lo.x);
  crd.writeReal(m_extent.lo.y);
  crd.writeReal(m_extent.lo.z);
  crd.writeReal(m_extent.hi.x);
  crd.writeReal(m_extent.hi.y);
  crd.writeReal(m_extent.hi.z);

  // save heights of implicit corners
  crd.writeReal(m_neY);
  crd.writeReal(m_swY);

  // save connections to adjacent areas
  // in the enum order NORTH, EAST, SOUTH, WEST
  for (int d = 0; d < NUM_DIRECTIONS; d++)
  {
    // save number of connections for this direction
    unsigned int count = m_connect[d].Count();
    crd.writeInt(count);

    FOR_EACH_LL(m_connect[d], it)
    {
      NavConnect connect = m_connect[d][it];
      crd.writeInt(connect.area->m_id);
    }
  }

  //
  // Store hiding spots for this area
  //
  unsigned char count;
  if (m_hidingSpotList.Count() > 255)
  {
    count = 255;
    Msg("Warning: NavArea #%d: Truncated hiding spot list to 255\n", m_id);
  }
  else
  {
    count = (unsigned char)m_hidingSpotList.Count();
  }
  crd.writeInt(count);

  // store HidingSpot objects
  unsigned int saveCount = 0;
  FOR_EACH_LL(m_hidingSpotList, hit)
  {
    HidingSpot *spot = m_hidingSpotList[hit];

    spot->Save(crd);

    // overflow check
    if (++saveCount == count)
      break;
  }

  //
  // Save the approach areas for this area
  //

  // save number of approach areas
  crd.writeInt(m_approachCount);

  // save approach area info
  unsigned char type;
  unsigned int zero = 0;
  for (int a = 0; a < m_approachCount; ++a)
  {
    if (m_approach[a].here.area)
      crd.writeInt(m_approach[a].here.area->m_id);
    else
      crd.writeInt(zero);

    if (m_approach[a].prev.area)
      crd.writeInt(m_approach[a].prev.area->m_id);
    else
      crd.writeInt(zero);
    type = (unsigned char)m_approach[a].prevToHereHow;
    crd.writeInt(type);

    if (m_approach[a].next.area)
      crd.writeInt(m_approach[a].next.area->m_id);
    else
      crd.writeInt(zero);
    type = (unsigned char)m_approach[a].hereToNextHow;
    crd.writeInt(type);
  }

  //
  // Save encounter spots for this area
  //
  {
    // save number of encounter paths for this area
    unsigned int count = m_spotEncounterList.Count();
    crd.writeInt(count);

    SpotEncounter *e;
    FOR_EACH_LL(m_spotEncounterList, it)
    {
      e = m_spotEncounterList[it];

      if (e->from.area)
        crd.writeInt(e->from.area->m_id);
      else
        crd.writeInt(zero);

      unsigned char dir = (unsigned char)e->fromDir;
      crd.writeInt(dir);

      if (e->to.area)
        crd.writeInt(e->to.area->m_id);
      else
        crd.writeInt(zero);

      dir = (unsigned char)e->toDir;
      crd.writeInt(dir);

      // write list of spots along this path
      unsigned char spotCount;
      if (e->spotList.Count() > 255)
      {
        spotCount = 255;
        Msg("Warning: NavArea #%d: Truncated encounter spot list to 255\n", m_id);
      }
      else
      {
        spotCount = (unsigned char)e->spotList.Count();
      }
      crd.writeInt(spotCount);

      saveCount = 0;
      FOR_EACH_LL(e->spotList, sit)
      {
        SpotOrder *order = &e->spotList[sit];

        // order->spot may be NULL if we've loaded a nav mesh that has been edited but not re-analyzed
        unsigned int id = (order->spot) ? order->spot->GetID() : 0;
        crd.writeInt(id);

        unsigned char t = (unsigned char)(255 * order->t);
        crd.writeInt(t);

        // overflow check
        if (++saveCount == spotCount)
          break;
      }
    }
  }

  Msg("Area ID %02d: %02d hide spots, %02d approach areas, %02d encounter spots", m_id, m_hidingSpotList.Count(), m_approachCount,
    m_spotEncounterList.Count());
}


//--------------------------------------------------------------------------------------------------------------
/**
 * save m_overlapList ids;
 */
void CNavArea::SaveOverlapIds(IGenSave &crd) const
{
  crd.writeInt(m_overlapList.Count());

  FOR_EACH_LL(m_overlapList, i)
  {
    FOR_EACH_LL(TheNavAreaList, id)
    {
      if (TheNavAreaList[id] == m_overlapList[i])
      {
        crd.writeInt(id);
      }
    }
  }
}

struct PointLink
{
  int point1;
  int point2;
};

bool findLinkInList(Tab<PointLink> &list, PointLink link)
{
  for (int n = 0; n < list.size(); n++)
  {
    const PointLink &l = list[n];
    if (l.point1 == link.point1 && l.point2 == link.point2)
      return true;
    if (l.point1 == link.point2 && l.point2 == link.point1)
      return true;
  }
  return false;
}


bool CNavMesh::Save(IGenSave &crd)
{

  GenerateZoneInfo();

  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];
    area->ComputeHidingSpots();
  }

  // save mesh params
  crd.writeInt(NAVIGATION_MESH_FILE_VERSION);
  crd.writeReal(generationStepSize);

  //
  // Store navigation areas
  //

  // store number of areas
  unsigned int count = TheNavAreaList.Count();
  crd.writeInt(count);

  // store each area
  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];
    area->Save(crd);
  }

  // hint points
  Tab<PointLink> links(midmem);

  crd.writeInt(hintPoints.size());
  for (int n = 0; n < hintPoints.size(); n++)
  {
    NavHintPoint *p = hintPoints[n];

    int areaid = p->getParentArea() ? (int)((CNavArea *)p->getParentArea())->GetID() : -1;
    crd.writeInt(areaid);

    crd.writeString(p->getName());
    crd.writeInt((int)p->getType());

    crd.writeReal(p->getPosition().x);
    crd.writeReal(p->getPosition().y);
    crd.writeReal(p->getPosition().z);

    crd.writeReal(p->getRadius());

    const Tab<NavHintPoint *> &linkpoints = p->getLinks();
    for (int nlink = 0; nlink < linkpoints.size(); nlink++)
    {
      PointLink link;
      link.point1 = n;
      link.point2 = getHintPointIndex(linkpoints[nlink]);

      if (!findLinkInList(links, link))
        links.push_back(link);
    }
  }

  crd.writeInt(links.size());
  for (int n = 0; n < links.size(); n++)
  {
    crd.writeInt(links[n].point1);
    crd.writeInt(links[n].point2);
  }

  //-----------------------------------------------------------------
  // save overlapList indexes for all areas for fast load navigation
  crd.beginTaggedBlock('OVRP');
  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];
    area->SaveOverlapIds(crd);
  }
  crd.endBlock();

  return true;
}
