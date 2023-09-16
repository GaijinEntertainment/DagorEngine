
#include <navigation/dag_navInterface.h>
#include <ioSys/dag_genIo.h>
#include <navigation/dag_navMesh.h>

#define NAVIGATION_MESH_FILE_VERSION 4

//--------------------------------------------------------------------------------------------------------------
/**
 * load m_overlapList
 */
void CNavArea::LoadOverlapIds(IGenLoad &crd)
{
  int count = crd.readInt();

  while (--count >= 0)
  {
    int id = crd.readInt();
    m_overlapList.AddToTail(TheNavAreaList[id]);
  }
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Load a navigation area from the file
 */
void CNavArea::Load(IGenLoad &crd)
{
  // load ID
  m_id = crd.readInt();

  // update nextID to avoid collisions
  if (m_id >= m_nextID)
    m_nextID = m_id + 1;

  // load attribute flags
  m_attributeFlags = crd.readInt();

  m_zone = crd.readInt();

  // load extent of area
  crd.readReal(m_extent.lo.x);
  crd.readReal(m_extent.lo.y);
  crd.readReal(m_extent.lo.z);
  crd.readReal(m_extent.hi.x);
  crd.readReal(m_extent.hi.y);
  crd.readReal(m_extent.hi.z);

  m_center.x = (m_extent.lo.x + m_extent.hi.x) / 2.0f;
  m_center.y = (m_extent.lo.y + m_extent.hi.y) / 2.0f;
  m_center.z = (m_extent.lo.z + m_extent.hi.z) / 2.0f;

  // load heights of implicit corners
  crd.readReal(m_neY);
  crd.readReal(m_swY);

  // load connections (IDs) to adjacent areas
  // in the enum order NORTH, EAST, SOUTH, WEST
  for (int d = 0; d < NUM_DIRECTIONS; d++)
  {
    // load number of connections for this direction
    unsigned int count = crd.readInt();

    for (unsigned int i = 0; i < count; ++i)
    {
      NavConnect connect;
      connect.id = crd.readInt();

      m_connect[d].AddToTail(connect);
    }
  }

  //
  // Load hiding spots
  //

  // load number of hiding spots
  unsigned char hidingSpotCount = crd.readInt();

  // load HidingSpot objects for this area
  for (int h = 0; h < hidingSpotCount; ++h)
  {
    // create new hiding spot and put on master list
    HidingSpot *spot = new HidingSpot;

    spot->Load(crd);

    m_hidingSpotList.AddToTail(spot);
  }

  //
  // Load number of approach areas
  //
  m_approachCount = crd.readInt();

  // load approach area info (IDs)
  unsigned char type;
  for (int a = 0; a < m_approachCount; ++a)
  {
    m_approach[a].here.id = crd.readInt();

    m_approach[a].prev.id = crd.readInt();
    type = crd.readInt();
    m_approach[a].prevToHereHow = (NavTraverseType)type;

    m_approach[a].next.id = crd.readInt();
    type = crd.readInt();
    m_approach[a].hereToNextHow = (NavTraverseType)type;
  }


  //
  // Load encounter paths for this area
  //
  unsigned int count = crd.readInt();

  for (unsigned int e = 0; e < count; ++e)
  {
    SpotEncounter *encounter = new SpotEncounter;

    encounter->from.id = crd.readInt();

    unsigned char dir;
    dir = crd.readInt();
    encounter->fromDir = static_cast<NavDirType>(dir);

    encounter->to.id = crd.readInt();

    dir = crd.readInt();
    encounter->toDir = static_cast<NavDirType>(dir);

    // read list of spots along this path
    unsigned char spotCount;
    spotCount = crd.readInt();

    SpotOrder order;
    for (int s = 0; s < spotCount; ++s)
    {
      order.id = crd.readInt();

      unsigned char t;
      t = crd.readInt();

      order.t = (float)t / 255.0f;

      encounter->spotList.AddToTail(order);
    }

    m_spotEncounterList.AddToTail(encounter);
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Convert loaded IDs to pointers
 * Make sure all IDs are converted, even if corrupt data is encountered.
 */
NavErrorType CNavArea::PostLoad(int version)
{
  NavErrorType error = NAV_OK;

  // connect areas together
  for (int d = 0; d < NUM_DIRECTIONS; d++)
  {
    FOR_EACH_LL(m_connect[d], it)
    {
      NavConnect *connect = &m_connect[d][it];

      unsigned int id = connect->id;
      connect->area = TheNavMesh->GetNavAreaByID(id);
      if (id && connect->area == NULL)
      {
        Error("CNavArea::PostLoad: Corrupt navigation data. Cannot connect Navigation Areas.\n");
        error = NAV_CORRUPT_DATA;
      }
    }
  }

  // resolve approach area IDs
  for (int a = 0; a < m_approachCount; ++a)
  {
    m_approach[a].here.area = TheNavMesh->GetNavAreaByID(m_approach[a].here.id);
    if (m_approach[a].here.id && m_approach[a].here.area == NULL)
    {
      Error("CNavArea::PostLoad: Corrupt navigation data. Missing Approach Area (here).\n");
      error = NAV_CORRUPT_DATA;
    }

    m_approach[a].prev.area = TheNavMesh->GetNavAreaByID(m_approach[a].prev.id);
    if (m_approach[a].prev.id && m_approach[a].prev.area == NULL)
    {
      Error("CNavArea::PostLoad: Corrupt navigation data. Missing Approach Area (prev).\n");
      error = NAV_CORRUPT_DATA;
    }

    m_approach[a].next.area = TheNavMesh->GetNavAreaByID(m_approach[a].next.id);
    if (m_approach[a].next.id && m_approach[a].next.area == NULL)
    {
      Error("CNavArea::PostLoad: Corrupt navigation data. Missing Approach Area (next).\n");
      error = NAV_CORRUPT_DATA;
    }
  }


  // resolve spot encounter IDs
  SpotEncounter *e;
  FOR_EACH_LL(m_spotEncounterList, it)
  {
    e = m_spotEncounterList[it];

    e->from.area = TheNavMesh->GetNavAreaByID(e->from.id);
    if (e->from.area == NULL)
    {
      Error("CNavArea::PostLoad: Corrupt navigation data. Missing \"from\" Navigation Area for Encounter Spot.\n");
      error = NAV_CORRUPT_DATA;
    }

    e->to.area = TheNavMesh->GetNavAreaByID(e->to.id);
    if (e->to.area == NULL)
    {
      Error("CNavArea::PostLoad: Corrupt navigation data. Missing \"to\" Navigation Area for Encounter Spot.\n");
      error = NAV_CORRUPT_DATA;
    }

    if (e->from.area && e->to.area)
    {
      // compute path
      float halfWidth;
      ComputePortal(e->to.area, e->toDir, &e->path.to, &halfWidth);
      ComputePortal(e->from.area, e->fromDir, &e->path.from, &halfWidth);

      const float eyeHeight = ::navOptions.HumanHeightCrouch;
      e->path.from.y = e->from.area->GetY(&e->path.from) + eyeHeight;
      e->path.to.y = e->to.area->GetY(&e->path.to) + eyeHeight;
    }

    // resolve HidingSpot IDs
    FOR_EACH_LL(e->spotList, sit)
    {
      SpotOrder *order = &e->spotList[sit];

      order->spot = GetHidingSpotByID(order->id);
      if (order->spot == NULL)
      {
        Error("CNavArea::PostLoad: Corrupt navigation data. Missing Hiding Spot\n");
        error = NAV_CORRUPT_DATA;
      }
    }
  }

  // build overlap list
  /// @todo Optimize this
  if (version < 4)
  {
    FOR_EACH_LL(TheNavAreaList, nit)
    {
      CNavArea *area = TheNavAreaList[nit];

      if (area == this)
        continue;

      if (IsOverlapping(area))
        m_overlapList.AddToTail(area);
    }
  }

  return error;
}


/**
 * Store Navigation Mesh to a file
 */

//--------------------------------------------------------------------------------------------------------------
/**
 * Load AI navigation data from a file
 */

NavErrorType CNavMesh::Load(IGenLoad &crd)
{
  // free previous navigation mesh data
  Reset();

  // get mesh params
  int version = crd.readInt();
  if (version > NAVIGATION_MESH_FILE_VERSION)
    return NAV_BAD_FILE_VERSION;

  generationStepSize = crd.readReal();

  usableGenerationStepSize = generationStepSize;
  usableGenerationStepSize /= 0.03125f;
  usableGenerationStepSize = (int)usableGenerationStepSize;
  usableGenerationStepSize *= 0.03125f;

  CNavArea::m_nextID = 1;

  // get number of areas
  unsigned int count;
  count = crd.readInt();

  Extent extent;
  extent.lo.x = 9999999999.9f;
  extent.lo.z = 9999999999.9f;
  extent.hi.x = -9999999999.9f;
  extent.hi.z = -9999999999.9f;

  // load the areas and compute total extent
  for (unsigned int i = 0; i < count; ++i)
  {
    CNavArea *area = new CNavArea;
    area->Load(crd);
    area->setIndex(i);
    TheNavAreaList.AddToTail(area);

    const Extent *areaExtent = area->GetExtent();

    // check validity of nav area
    if (areaExtent->lo.x >= areaExtent->hi.x || areaExtent->lo.z >= areaExtent->hi.z)
      Msg("WARNING: Degenerate Navigation Area #%d at ( %g, %g, %g )\n", area->GetID(), area->m_center.x, area->m_center.y,
        area->m_center.z);

    if (areaExtent->lo.x < extent.lo.x)
      extent.lo.x = areaExtent->lo.x;
    if (areaExtent->lo.z < extent.lo.z)
      extent.lo.z = areaExtent->lo.z;
    if (areaExtent->hi.x > extent.hi.x)
      extent.hi.x = areaExtent->hi.x;
    if (areaExtent->hi.z > extent.hi.z)
      extent.hi.z = areaExtent->hi.z;
  }

  if (count == 0)
  {
    extent.lo.x = 0.f;
    extent.lo.z = 0.f;
    extent.hi.x = 0.f;
    extent.hi.z = 0.f;
  }

  // add the areas to the grid
  AllocateGrid(extent.lo.x, extent.hi.x, extent.lo.z, extent.hi.z);

  FOR_EACH_LL(TheNavAreaList, it) { AddNavArea(TheNavAreaList[it]); }

  // allow areas to connect to each other, etc
  FOR_EACH_LL(TheNavAreaList, pit)
  {
    CNavArea *area = TheNavAreaList[pit];
    area->PostLoad(version);
  }

  // hint points
  {
    int count = crd.readInt();
    for (int n = 0; n < count; n++)
    {
      int areaid = crd.readInt();
      CNavArea *area = (areaid != -1) ? GetNavAreaByID(areaid) : NULL;

      NavHintPoint *p = createHintPoint(area);

      SimpleString s;
      crd.readString(s);
      p->setName(s);
      p->setType((NavHintPoint::Type)crd.readInt());

      p->setPosition(Point3(crd.readReal(), crd.readReal(), crd.readReal()));

      p->setRadius(crd.readReal());
    }

    count = crd.readInt();
    for (int n = 0; n < count; n++)
    {
      NavHintPoint *point1 = hintPoints[crd.readInt()];
      NavHintPoint *point2 = hintPoints[crd.readInt()];
      point1->addLink(point2);
    }
  }


  //-----------------------------------------------------------------
  // load overlapList indexes for all areas for fast load navigation
  if (version >= 4)
  {
    crd.beginTaggedBlock();
    FOR_EACH_LL(TheNavAreaList, it)
    {
      CNavArea *area = TheNavAreaList[it];
      area->LoadOverlapIds(crd);
    }
    crd.endBlock();
  }

  // the Navigation Mesh has been successfully loaded
  m_isLoaded = true;

  return NAV_OK;
}
