// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <navigation/dag_navInterface.h>
#include <navigation/dag_navMesh.h>
#include <navigation/dag_navNode.h>
#include <navigation/dag_navPathfind.h>
#include <libTools/navGeneration/nav_external.h>
#include <util/dag_string.h>
#include <math/dag_capsule.h>
#include <libTools/util/progressInd.h>
#include <perfMon/dag_perfTimer2.h>

static const int MAX_BLOCKED_AREAS = 256;
static unsigned int blockedID[MAX_BLOCKED_AREAS];
static int blockedIDCount = 0;

// --- nav_iface.cpp
bool Navigator::isFreeCapsuleToPosition(const Point3 &pt) const { return TheNavMesh->isFreeCapsuleToPosition(pt); }

void Navigator::generate(IGenericProgressIndicator *progress_ind, bool reset_hint_points)
{
  if (progress_ind)
  {
    progress_ind->setTotal(0);
    progress_ind->setDone(0);
    progress_ind->setActionDesc("Begin generation");
  }

  TheNavMesh->Reset(reset_hint_points);
  // debug_flush(true);
  TheNavMesh->BeginGeneration(::navOptions.GenerationStepSize, reset_hint_points);
  while (TheNavMesh->UpdateGeneration(progress_ind))
    ;

  const Tab<NavHintPoint *> &hints = getHintPoints();
  for (int n = 0; n < hints.size(); n++)
  {
    NavHintPoint *point = hints[n];
    Point3 pos = point->getPosition();
    point->setPosition(pos);
  }

  //  ::navOptions.GenerationStepSize = safegenerationstepsize;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Shortest path cost, paying attention to "blocked" areas
 */
class ApproachAreaCost
{
public:
  float operator()(CNavArea *area, CNavArea *fromArea)
  {
    // check if this area is "blocked"
    for (int i = 0; i < blockedIDCount; ++i)
    {
      if (area->GetID() == blockedID[i])
      {
        return -1.0f;
      }
    }

    if (fromArea == NULL)
    {
      // first area in path, no cost
      return 0.0f;
    }
    else
    {
      // compute distance travelled along path so far
      float dist;

      dist = (*area->GetCenter() - *fromArea->GetCenter()).length();

      float cost = dist + fromArea->GetCostSoFar();

      return cost;
    }
  }
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Can we see this area?
 * For now, if we can see any corner, we can see the area
 * @todo Need to check LOS to more than the corners for large and/or long areas
 */
inline bool IsAreaVisible(const Vector *pos, const CNavArea *area)
{
  Vector corner;

  for (int c = 0; c < NUM_CORNERS; ++c)
  {
    corner = *area->GetCorner((NavCornerType)c);
    corner.y += 0.75f * ::navOptions.HumanHeight;

    Point3 dir = corner - *pos;
    float dist = length(dir);
    if (!nav_rt_trace_ray(*pos, dir / dist, dist))
      return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Determine the set of "approach areas".
 * An approach area is an area representing a place where players
 * move into/out of our local neighborhood of areas.
 */
void CNavArea::ComputeApproachAreas(void)
{
  m_approachCount = 0;

  if (nav_quicksave)
    return;

  // use the center of the nav area as the "view" point
  Vector eye = m_center;
  if (TheNavMesh->GetGroundHeight(&eye, &eye.y) == false)
    return;

  // approximate eye position
  if (GetAttributes() & NAV_MESH_CROUCH)
    eye.y += 0.9f * ::navOptions.HumanHeightCrouch;
  else
    eye.y += 0.9f * ::navOptions.HumanHeight;

  static const int MAX_PATH_LENGTH = 256;
  CNavArea *path[MAX_PATH_LENGTH];

  //
  // In order to enumerate all of the approach areas, we need to
  // run the algorithm many times, once for each "far away" area
  // and keep the union of the approach area sets
  //
  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *farArea = TheNavAreaList[it];

    blockedIDCount = 0;

    // skip the small areas
    const float minSize = 1.5f; // 150
    const Extent *extent = farArea->GetExtent();
    if (extent->SizeX() < minSize || extent->SizeZ() < minSize)
    {
      continue;
    }

    // if we can see 'farArea', try again - the whole point is to go "around the bend", so to speak
    if (IsAreaVisible(&eye, farArea))
    {
      continue;
    }

    // make first path to far away area
    ApproachAreaCost cost;
    if (NavAreaBuildPath(this, farArea, NULL, cost) == false)
    {
      continue;
    }

    //
    // Keep building paths to farArea and blocking them off until we
    // cant path there any more.
    // As areas are blocked off, all exits will be enumerated.
    //
    while (m_approachCount < MAX_APPROACH_AREAS)
    {
      // find number of areas on path
      int count = 0;
      CNavArea *area;
      for (area = farArea; area; area = area->GetParent())
        ++count;

      if (count > MAX_PATH_LENGTH)
        count = MAX_PATH_LENGTH;

      // build path in correct order - from eye outwards
      int i = count;
      for (area = farArea; i && area; area = area->GetParent())
        path[--i] = area;

      // traverse path to find first area we cannot see (skip the first area)
      for (i = 1; i < count; ++i)
      {
        // if we see this area, continue on
        if (IsAreaVisible(&eye, path[i]))
          continue;

        // we can't see this area.
        // mark this area as "blocked" and unusable by subsequent approach paths
        if (blockedIDCount == MAX_BLOCKED_AREAS)
        {
          Msg("Overflow computing approach areas for area #%d.\n", m_id);
          return;
        }

        // if the area to be blocked is actually farArea, block the one just prior
        // (blocking farArea will cause all subsequent pathfinds to fail)
        int block = (path[i] == farArea) ? i - 1 : i;

        blockedID[blockedIDCount++] = path[block]->GetID();

        if (block == 0)
          break;

        // store new approach area if not already in set
        int a;
        for (a = 0; a < m_approachCount; ++a)
          if (m_approach[a].here.area == path[block - 1])
            break;

        if (a == m_approachCount)
        {
          m_approach[m_approachCount].prev.area = (block >= 2) ? path[block - 2] : NULL;

          m_approach[m_approachCount].here.area = path[block - 1];
          m_approach[m_approachCount].prevToHereHow = path[block - 1]->GetParentHow();

          m_approach[m_approachCount].next.area = path[block];
          m_approach[m_approachCount].hereToNextHow = path[block]->GetParentHow();

          ++m_approachCount;
        }

        // we are done with this path
        break;
      }

      // find another path to 'farArea'
      ApproachAreaCost cost;
      if (NavAreaBuildPath(this, farArea, NULL, cost) == false)
      {
        // can't find a path to 'farArea' means all exits have been already tested and blocked
        break;
      }
    }
  }
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Mark all areas that require a jump to get through them.
 * This currently relies on jump areas having extreme slope.
 */
void CNavMesh::MarkJumpAreas(void)
{
  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];
    Vector u, v;

    // compute our unit surface normal
    u.x = area->m_extent.hi.x - area->m_extent.lo.x;
    u.z = 0.0f;
    u.y = area->m_neY - area->m_extent.lo.y;

    v.x = 0.0f;
    v.z = area->m_extent.hi.z - area->m_extent.lo.z;
    v.y = area->m_swY - area->m_extent.lo.y;

    Vector normal = -(u % v); // REVERSE
    normal.normalize();

    // debug("Checking jump area: normal.y = %f",normal.y);

    if (normal.y < ::navOptions.MaxSurfaceSlope)
    {
      area->SetAttributes(area->GetAttributes() | NAV_MESH_JUMP);
      // debug("!!!!!!found");
    }
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Recursively chop area in half along X until child areas are roughly square
 */
static void splitX(CNavArea *area)
{
  if (area->IsRoughlySquare())
    return;

  float split = area->GetSizeX();
  split /= 2.0f;
  split += area->GetExtent()->lo.x;

  split = TheNavMesh->SnapToGrid(split);

  const float epsilon = 0.05f;
  if (fabs(split - area->GetExtent()->lo.x) < epsilon || fabs(split - area->GetExtent()->hi.x) < epsilon)
  {
    // too small to subdivide
    return;
  }

  CNavArea *alpha, *beta;
  if (area->SplitEdit(false, split, &alpha, &beta))
  {
    // split each new area until square
    splitX(alpha);
    splitX(beta);
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Recursively chop area in half along Y until child areas are roughly square
 */
static void splitZ(CNavArea *area)
{
  if (area->IsRoughlySquare())
    return;

  float split = area->GetSizeZ();
  split /= 2.0f;
  split += area->GetExtent()->lo.z;

  split = TheNavMesh->SnapToGrid(split);

  const float epsilon = 0.05f;
  if (fabs(split - area->GetExtent()->lo.z) < epsilon || fabs(split - area->GetExtent()->hi.z) < epsilon)
  {
    // too small to subdivide
    return;
  }

  CNavArea *alpha, *beta;
  if (area->SplitEdit(true, split, &alpha, &beta))
  {
    // split each new area until square
    splitZ(alpha);
    splitZ(beta);
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Split any long, thin, areas into roughly square chunks.
 */
void CNavMesh::SquareUpAreas(void)
{
  int it = TheNavAreaList.Head();

  while (it != TheNavAreaList.InvalidIndex())
  {
    CNavArea *area = TheNavAreaList[it];

    // move the iterator in case the current area is split and deleted
    it = TheNavAreaList.Next(it);

    if (!area->IsRoughlySquare())
    {
      // chop this area into square pieces
      if (area->GetSizeX() > area->GetSizeZ())
        splitX(area);
      else
        splitZ(area);
    }
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Start at given position and find first area in given direction
 */
inline CNavArea *findFirstAreaInDirection(const Vector *start, NavDirType dir, float range, float beneathLimit,
  CBaseEntity *traceIgnore = NULL, Vector *closePos = NULL)
{
  CNavArea *area = NULL;

  Vector pos = *start;

  int end = (int)((range / TheNavMesh->getUsableGenerationStepSize()) + 0.5f);

  for (int i = 1; i <= end; i++)
  {
    AddDirectionVector(&pos, dir, TheNavMesh->getUsableGenerationStepSize());

    // make sure we dont look thru the wall

    Point3 dir = pos - *start;
    float dist = length(dir);
    if (nav_rt_trace_ray(*start, dir / dist, dist))
      break;

    area = TheNavMesh->GetNavArea(&pos, beneathLimit);
    if (area)
    {
      if (closePos)
      {
        closePos->x = pos.x;
        closePos->z = pos.z;
        closePos->y = area->GetY(pos.x, pos.z);
      }

      break;
    }
  }

  return area;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Determine if we can "jump down" from given point
 */
inline bool testJumpDown(const Vector *fromPos, const Vector *toPos)
{
  float dy = fromPos->y - toPos->y;

  // drop can't be too far, or too short (or nonexistant)
  if (dy <= 0 || dy >= ::navOptions.DeathDrop)
    return false;

  //
  // Check LOS out and down
  //
  // ------+
  //       |
  // F     |
  //       |
  //       T
  //

  Vector from(fromPos->x, fromPos->y + ::navOptions.HumanHeight, fromPos->z);
  Vector to(toPos->x, from.y, toPos->z);

  Point3 dir = to - from;
  float dist = length(dir);
  if (nav_rt_trace_ray(from, dir / dist, dist))
    return false;

  from = to;
  to.y = toPos->y + 0.05f;

  dir = to - from;
  dist = length(dir);
  if (nav_rt_trace_ray(from, dir / dist, dist))
    return false;

  return true;
}


//--------------------------------------------------------------------------------------------------------------
inline CNavArea *findJumpDownArea(const Vector *fromPos, NavDirType dir, bool &can_jump_back)
{
  can_jump_back = false;

  Vector start(fromPos->x, fromPos->y + ::navOptions.HumanHeightCrouch, fromPos->z);
  AddDirectionVector(&start, dir, TheNavMesh->getUsableGenerationStepSize() / 2.0f);

  Vector toPos;
  CNavArea *downArea =
    findFirstAreaInDirection(&start, dir, 4.0f * TheNavMesh->getUsableGenerationStepSize(), ::navOptions.DeathDrop, NULL, &toPos);

  if (downArea && testJumpDown(fromPos, &toPos))
  {
    if (fromPos->y - toPos.y <= ::navOptions.JumpCrouchHeight)
      can_jump_back = true;

    return downArea;
  }

  return NULL;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Define connections between adjacent generated areas
 */
void CNavMesh::ConnectGeneratedAreas(void)
{
  Msg("Connecting navigation areas...\n");

  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];

    // scan along edge nodes, stepping one node over into the next area
    // for now, only use bi-directional connections

    // north edge
    CNavNode *node;
    for (node = area->m_node[NORTH_WEST]; node != area->m_node[NORTH_EAST]; node = node->GetConnectedNode(EAST))
    {
      CNavNode *adj = node->GetConnectedNode(NORTH);

      if (adj && adj->GetArea() && adj->GetConnectedNode(SOUTH) == node)
      {
        area->ConnectTo(adj->GetArea(), NORTH);
      }
      else
      {
        bool canJumpBack;
        CNavArea *downArea = findJumpDownArea(node->GetPosition(), NORTH, canJumpBack);

        if (downArea && downArea != area)
        {
          area->ConnectTo(downArea, NORTH);
          if (canJumpBack)
            downArea->ConnectTo(area, SOUTH);
        }
      }
    }

    // west edge
    for (node = area->m_node[NORTH_WEST]; node != area->m_node[SOUTH_WEST]; node = node->GetConnectedNode(SOUTH))
    {
      CNavNode *adj = node->GetConnectedNode(WEST);

      if (adj && adj->GetArea() && adj->GetConnectedNode(EAST) == node)
      {
        area->ConnectTo(adj->GetArea(), WEST);
      }
      else
      {
        bool canJumpBack;
        CNavArea *downArea = findJumpDownArea(node->GetPosition(), WEST, canJumpBack);

        if (downArea && downArea != area)
        {
          area->ConnectTo(downArea, WEST);
          if (canJumpBack)
            downArea->ConnectTo(area, EAST);
        }
      }
    }

    // south edge - this edge's nodes are actually part of adjacent areas
    // move one node north, and scan west to east
    /// @todo This allows one-node-wide areas - do we want this?
    node = area->m_node[SOUTH_WEST];
    node = node->GetConnectedNode(NORTH);
    if (node)
    {
      CNavNode *end = area->m_node[SOUTH_EAST]->GetConnectedNode(NORTH);
      /// @todo Figure out why cs_backalley gets a NULL node in here...
      for (; node && node != end; node = node->GetConnectedNode(EAST))
      {
        CNavNode *adj = node->GetConnectedNode(SOUTH);

        if (adj && adj->GetArea() && adj->GetConnectedNode(NORTH) == node)
        {
          area->ConnectTo(adj->GetArea(), SOUTH);
        }
        else
        {
          bool canJumpBack;
          CNavArea *downArea = findJumpDownArea(node->GetPosition(), SOUTH, canJumpBack);

          if (downArea && downArea != area)
          {
            area->ConnectTo(downArea, SOUTH);
            if (canJumpBack)
              downArea->ConnectTo(area, NORTH);
          }
        }
      }
    }

    // east edge - this edge's nodes are actually part of adjacent areas
    node = area->m_node[NORTH_EAST];
    node = node->GetConnectedNode(WEST);
    if (node)
    {
      CNavNode *end = area->m_node[SOUTH_EAST]->GetConnectedNode(WEST);
      for (; node && node != end; node = node->GetConnectedNode(SOUTH))
      {
        CNavNode *adj = node->GetConnectedNode(EAST);

        if (adj && adj->GetArea() && adj->GetConnectedNode(WEST) == node)
        {
          area->ConnectTo(adj->GetArea(), EAST);
        }
        else
        {
          bool canJumpBack;
          CNavArea *downArea = findJumpDownArea(node->GetPosition(), EAST, canJumpBack);

          if (downArea && downArea != area)
          {
            area->ConnectTo(downArea, EAST);
            if (canJumpBack)
              downArea->ConnectTo(area, WEST);
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Merge areas together to make larger ones (must remain rectangular - convex).
 * Areas can only be merged if their attributes match.
 */
void CNavMesh::MergeGeneratedAreas(void)
{
  Msg("Merging navigation areas...\n");
  // TODO
  bool merged;

  do
  {
    merged = false;

    FOR_EACH_LL(TheNavAreaList, it)
    {
      CNavArea *area = TheNavAreaList[it];

      // north edge
      FOR_EACH_LL(area->m_connect[NORTH], nit)
      {
        CNavArea *adjArea = area->m_connect[NORTH][nit].area;

        if (area->m_node[NORTH_WEST] == adjArea->m_node[SOUTH_WEST] && area->m_node[NORTH_EAST] == adjArea->m_node[SOUTH_EAST] &&
            area->GetAttributes() == adjArea->GetAttributes() && area->IsCoplanar(adjArea))
        {
          // merge vertical
          area->m_node[NORTH_WEST] = adjArea->m_node[NORTH_WEST];
          area->m_node[NORTH_EAST] = adjArea->m_node[NORTH_EAST];

          merged = true;
          // CONSOLE_ECHO( "  Merged (north) areas #%d and #%d\n", area->m_id, adjArea->m_id );

          area->FinishMerge(adjArea);

          // restart scan - iterator is invalidated
          break;
        }
      }

      if (merged)
        break;

      // south edge
      FOR_EACH_LL(area->m_connect[SOUTH], sit)
      {
        CNavArea *adjArea = area->m_connect[SOUTH][sit].area;

        if (adjArea->m_node[NORTH_WEST] == area->m_node[SOUTH_WEST] && adjArea->m_node[NORTH_EAST] == area->m_node[SOUTH_EAST] &&
            area->GetAttributes() == adjArea->GetAttributes() && area->IsCoplanar(adjArea))
        {
          // merge vertical
          area->m_node[SOUTH_WEST] = adjArea->m_node[SOUTH_WEST];
          area->m_node[SOUTH_EAST] = adjArea->m_node[SOUTH_EAST];

          merged = true;
          // CONSOLE_ECHO( "  Merged (south) areas #%d and #%d\n", area->m_id, adjArea->m_id );

          area->FinishMerge(adjArea);

          // restart scan - iterator is invalidated
          break;
        }
      }

      if (merged)
        break;


      // west edge
      FOR_EACH_LL(area->m_connect[WEST], wit)
      {
        CNavArea *adjArea = area->m_connect[WEST][wit].area;

        if (area->m_node[NORTH_WEST] == adjArea->m_node[NORTH_EAST] && area->m_node[SOUTH_WEST] == adjArea->m_node[SOUTH_EAST] &&
            area->GetAttributes() == adjArea->GetAttributes() && area->IsCoplanar(adjArea))
        {
          // merge horizontal
          area->m_node[NORTH_WEST] = adjArea->m_node[NORTH_WEST];
          area->m_node[SOUTH_WEST] = adjArea->m_node[SOUTH_WEST];

          merged = true;
          // CONSOLE_ECHO( "  Merged (west) areas #%d and #%d\n", area->m_id, adjArea->m_id );

          area->FinishMerge(adjArea);

          // restart scan - iterator is invalidated
          break;
        }
      }

      if (merged)
        break;

      // east edge
      FOR_EACH_LL(area->m_connect[EAST], eit)
      {
        CNavArea *adjArea = area->m_connect[EAST][eit].area;

        if (adjArea->m_node[NORTH_WEST] == area->m_node[NORTH_EAST] && adjArea->m_node[SOUTH_WEST] == area->m_node[SOUTH_EAST] &&
            area->GetAttributes() == adjArea->GetAttributes() && area->IsCoplanar(adjArea))
        {
          // merge horizontal
          area->m_node[NORTH_EAST] = adjArea->m_node[NORTH_EAST];
          area->m_node[SOUTH_EAST] = adjArea->m_node[SOUTH_EAST];

          merged = true;
          // CONSOLE_ECHO( "  Merged (east) areas #%d and #%d\n", area->m_id, adjArea->m_id );

          area->FinishMerge(adjArea);

          // restart scan - iterator is invalidated
          break;
        }
      }

      if (merged)
        break;
    }
  } while (merged);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Check if an rectangular area of the given size can be
 * made starting from the given node as the NW corner.
 * Only consider fully connected nodes for this check.
 * All of the nodes within the test area must have the same attributes.
 * All of the nodes must be approximately co-planar w.r.t the NW node's normal, with the
 * exception of 1x1 areas which can be any angle.
 */
bool CNavMesh::TestArea(CNavNode *node, int width, int height)
{
  Vector normal = *node->GetNormal();
  float d = -(normal * (*node->GetPosition()));

  const float offPlaneTolerance = 0.25f; // 5.0f;

  CNavNode *vertNode, *horizNode;

  vertNode = node;
  for (int z = 0; z < height; z++)
  {
    horizNode = vertNode;

    for (int x = 0; x < width; x++)
    {
      // all nodes must have the same attributes
      if (horizNode->GetAttributes() != node->GetAttributes())
        return false;

      if (horizNode->IsCovered())
        return false;

      if (!horizNode->IsClosedCell())
        return false;

      horizNode = horizNode->GetConnectedNode(EAST);
      if (horizNode == NULL)
        return false;

      // nodes must lie on/near the plane
      if (width > 1 || height > 1)
      {
        float dist = (float)fabs((*horizNode->GetPosition()) * normal + d);
        if (dist > offPlaneTolerance)
          return false;
      }
    }

    vertNode = vertNode->GetConnectedNode(SOUTH);
    if (vertNode == NULL)
      return false;

    // nodes must lie on/near the plane
    if (width > 1 || height > 1)
    {
      float dist = (float)fabs((*vertNode->GetPosition()) * normal + d);
      if (dist > offPlaneTolerance)
        return false;
    }
  }

  // check planarity of southern edge
  if (width > 1 || height > 1)
  {
    horizNode = vertNode;

    for (int x = 0; x < width; x++)
    {
      horizNode = horizNode->GetConnectedNode(EAST);
      if (horizNode == NULL)
        return false;

      // nodes must lie on/near the plane
      float dist = (float)fabs(((*horizNode->GetPosition()) * normal) + d);
      if (dist > offPlaneTolerance)
        return false;
    }
  }

  return true;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Create a nav area, and mark all nodes it overlaps as "covered"
 * NOTE: Nodes on the east and south edges are not included.
 * Returns number of nodes covered by this area, or -1 for error;
 */
int CNavMesh::BuildArea(CNavNode *node, int width, int height)
{
  CNavNode *nwNode = node;
  CNavNode *neNode = NULL;
  CNavNode *swNode = NULL;
  CNavNode *seNode = NULL;

  CNavNode *vertNode = node;
  CNavNode *horizNode;

  int coveredNodes = 0;

  for (int y = 0; y < height; y++)
  {
    horizNode = vertNode;

    for (int x = 0; x < width; x++)
    {
      horizNode->Cover();
      ++coveredNodes;

      horizNode = horizNode->GetConnectedNode(EAST);
    }

    if (y == 0)
      neNode = horizNode;

    vertNode = vertNode->GetConnectedNode(SOUTH);
  }

  swNode = vertNode;

  horizNode = vertNode;
  for (int x = 0; x < width; x++)
  {
    horizNode = horizNode->GetConnectedNode(EAST);
  }
  seNode = horizNode;

  if (!nwNode || !neNode || !seNode || !swNode)
  {
    Error("BuildArea - NULL node.\n");
    return -1;
  }

  CNavArea *area = new CNavArea(nwNode, neNode, seNode, swNode);
  TheNavAreaList.AddToTail(area);

  // since all internal nodes have the same attributes, set this area's attributes
  area->SetAttributes(node->GetAttributes());

  return coveredNodes;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * This function uses the CNavNodes that have been sampled from the map to
 * generate CNavAreas - rectangular areas of "walkable" space. These areas
 * are connected to each other, proving information on know how to move from
 * area to area.
 *
 * This is a "greedy" algorithm that attempts to cover the walkable area
 * with the fewest, largest, rectangles.
 */
void CNavMesh::CreateNavAreasFromNodes(void)
{
  // haven't yet seen a map use larger than 30...
  int tryWidth = 50;
  int tryHeight = 50;
  int uncoveredNodes = CNavNode::GetListLength();

  while (uncoveredNodes > 0)
  {
    for (CNavNode *node = CNavNode::GetFirst(); node; node = node->GetNext())
    {
      if (node->IsCovered())
        continue;

      if (TestArea(node, tryWidth, tryHeight))
      {
        int covered = BuildArea(node, tryWidth, tryHeight);
        if (covered < 0)
        {
          Error("Generate: Error - Data corrupt.\n");
          return;
        }

        uncoveredNodes -= covered;
      }
    }

    if (tryWidth >= tryHeight)
      --tryWidth;
    else
      --tryHeight;

    if (tryWidth <= 0 || tryHeight <= 0)
      break;
  }

  Extent extent;
  extent.lo.x = 9999999999.9f;
  extent.lo.z = 9999999999.9f;
  extent.hi.x = -9999999999.9f;
  extent.hi.z = -9999999999.9f;

  // compute total extent
  FOR_EACH_LL(TheNavAreaList, it)
  {
    CNavArea *area = TheNavAreaList[it];
    const Extent *areaExtent = area->GetExtent();

    if (areaExtent->lo.x < extent.lo.x)
      extent.lo.x = areaExtent->lo.x;
    if (areaExtent->lo.z < extent.lo.z)
      extent.lo.z = areaExtent->lo.z;
    if (areaExtent->hi.x > extent.hi.x)
      extent.hi.x = areaExtent->hi.x;
    if (areaExtent->hi.z > extent.hi.z)
      extent.hi.z = areaExtent->hi.z;
  }

  // add the areas to the grid
  AllocateGrid(extent.lo.x, extent.hi.x, extent.lo.z, extent.hi.z);

  FOR_EACH_LL(TheNavAreaList, git) { AddNavArea(TheNavAreaList[git]); }


  ConnectGeneratedAreas();
  MergeGeneratedAreas();
  SquareUpAreas();


  // MarkJumpAreas();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Initiate the generation process
 */
void CNavMesh::BeginGeneration(real generation_step_size, bool reset_hint_points)
{
  generationStepSize = generation_step_size;

  usableGenerationStepSize = generationStepSize;
  usableGenerationStepSize /= 0.03125f;
  usableGenerationStepSize = (int)usableGenerationStepSize;
  usableGenerationStepSize *= 0.03125f;


  m_generationState = SAMPLE_WALKABLE_SPACE;
  m_isGenerating = true;

  sampleModeStep = SampleModeStep_GeneratePoints;

  BBox3 boundingbox = nav_rt_get_bbox3();

  debug("NavMesh Bounding box: %f %f %f %f %f %f", boundingbox.lim[0].x, boundingbox.lim[0].y, boundingbox.lim[0].z,
    boundingbox.lim[1].x, boundingbox.lim[1].y, boundingbox.lim[1].z);


  currentPointPositionXZ = Point2(boundingbox[0][0], boundingbox[0][2]);
  currentPointPositionXZ[0] = SnapToGrid(currentPointPositionXZ[0]);
  currentPointPositionXZ[1] = SnapToGrid(currentPointPositionXZ[1]);


  // clear any previous mesh
  DestroyNavigationMesh(reset_hint_points);

  // TODO: find point to start sampling

  // start sampling from a spawn point
  // CBaseEntity *spawn = gEntList.FindEntityByClassname( NULL, GetPlayerSpawnName() );

  /*      if (spawn)
  {
  // snap it to the sampling grid
  Vector pos = spawn->GetAbsOrigin();
  pos.x = TheNavMesh->SnapToGrid( pos.x );
  pos.z = TheNavMesh->SnapToGrid( pos.z );

  Vector normal;
  if (GetGroundHeight( &pos, &pos.y, &normal ))
  {
  m_currentNode = new CNavNode( &pos, &normal, NULL );
  }
  }
  else     */
  {
    // the system will see this NULL and select the next walkable seed
    m_currentNode = NULL;
  }

  // if there are no seed points, we can't generate
  /*!!!!!!!!!
  if (m_walkableSeedList.Count() == 0 && m_currentNode == NULL)
  {
  m_isGenerating = false;
  Msg( "No valid walkable seed positions.  Cannot generate Navigation Mesh.\n" );
  return;
  }

  // initialize seed list index
  m_seedIdx = m_walkableSeedList.Head();
  */

  Msg("Generating Navigation Mesh...\n");

  Msg("Sampling walkable space...\n");
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Process the auto-generation for 'maxTime' seconds. return false if generation is complete.
 */
extern void addDebugNode(Point3 pt);
bool CNavMesh::UpdateGeneration(IGenericProgressIndicator *progress_ind /*, float maxTime*/)
{
  double startTime = Plat_FloatTime();

  switch (m_generationState)
  {
      //---------------------------------------------------------------------------
    case SAMPLE_WALKABLE_SPACE:
    {
      //      Msg( "Sampling walkable space...\n" );

      //                        while( Plat_FloatTime() - startTime < maxTime )
      //      {
      if (!SampleStep(progress_ind))
      {
        // sampling is complete, now wbuild nav areas
        m_generationState = CREATE_AREAS_FROM_SAMPLES;
        return true;
      }
      //      }

      return true;
    }

      //---------------------------------------------------------------------------
    case CREATE_AREAS_FROM_SAMPLES:
    {
      if (progress_ind)
      {
        progress_ind->setTotal(0);
        progress_ind->setDone(0);
        progress_ind->setActionDesc("Creating navigation area from sampled data");
      }

      Msg("Creating navigation area from sampled data...\n");

      // for( CNavNode *node = CNavNode::GetFirst(); node; node = node->GetNext() )
      //{
      //   addDebugNode(*node->GetPosition());
      // }

      CreateNavAreasFromNodes();
      DestroyHidingSpots();

      m_generationState = FIND_HIDING_SPOTS;

      Msg("Generating zones...\n");
      GenerateZoneInfo();

      return true;
    }

      //---------------------------------------------------------------------------
    case FIND_HIDING_SPOTS:
    {
      if (!navOptions.buildHidingAndSnipingSpots)
      {
        m_isGenerating = false;
        unsigned int sz = GetNavAreaCount();
        debug("Areas created : %d", sz);
        return false;
      }

      if (progress_ind)
      {
        progress_ind->setTotal(0);
        progress_ind->setDone(0);
        progress_ind->setActionDesc("Finding hiding spots");
      }

      Msg("Finding hiding spots...\n");

      FOR_EACH_LL(TheNavAreaList, it)
      {
        CNavArea *area = TheNavAreaList[it];

        area->ComputeHidingSpots();

        // don't go over our time allotment
        /*
        if( Plat_FloatTime() - startTime > maxTime )
        {
        return true;
        }
        */
      }

      m_generationState = FIND_APPROACH_AREAS;
      return true;
    }

      //---------------------------------------------------------------------------
    case FIND_APPROACH_AREAS:
    {
      if (progress_ind)
      {
        progress_ind->setTotal(0);
        progress_ind->setDone(0);
        progress_ind->setActionDesc("Finding approach areas");
      }

      Msg("Finding approach areas...\n");
      FOR_EACH_LL(TheNavAreaList, it)
      {
        CNavArea *area = TheNavAreaList[it];

        area->ComputeApproachAreas();

        //              // don't go over our time allotment
        //              if( Plat_FloatTime() - startTime > maxTime )
        //              {
        //                      return true;
        //              }
      }

      m_generationState = FIND_ENCOUNTER_SPOTS;
      return true;
    }

      //---------------------------------------------------------------------------
    case FIND_ENCOUNTER_SPOTS:
    {
      if (progress_ind)
      {
        progress_ind->setTotal(0);
        progress_ind->setDone(0);
        progress_ind->setActionDesc("Finding encounter spots");
      }

      Msg("Finding encounter spots...\n");
      FOR_EACH_LL(TheNavAreaList, it)
      {
        CNavArea *area = TheNavAreaList[it];

        area->ComputeSpotEncounters();

        //              // don't go over our time allotment
        //              if( Plat_FloatTime() - startTime > maxTime )
        //              {
        //                      return true;
        //              }
      }


      m_generationState = FIND_SNIPER_SPOTS;
      return true;
    }
      //---------------------------------------------------------------------------
    case FIND_SNIPER_SPOTS:
    {
      Msg("Finding sniper spots...\n");

      FOR_EACH_LL(TheNavAreaList, it)
      {
        CNavArea *area = TheNavAreaList[it];

        area->ComputeSniperSpots();

        //              // don't go over our time allotment
        //              if( Plat_FloatTime() - startTime > maxTime )
        //              {
        //                      return true;
        //              }
      }

      // generation complete!
      Msg("Generation complete!\n");
      m_isGenerating = false;

      unsigned int sz = GetNavAreaCount();
      debug("Areas created : %d", sz);

      /*                      // save the mesh
      if (Save())
      {
      Msg( "Navigation map '%s' saved.\n", GetFilename() );
      }
      else
      {
      const char *filename = GetFilename();
      Msg( "ERROR: Cannot save navigation map '%s'.\n", (filename) ? filename : "(null)" );
      }
      */
      return false;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Define the name of player spawn entities
 */
void CNavMesh::SetPlayerSpawnName(const char *name)
{
  if (m_spawnName)
  {
    delete[] m_spawnName;
  }

  m_spawnName = new char[strlen(name) + 1];
  strcpy(m_spawnName, name);
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return name of player spawn entity
 */
const char *CNavMesh::GetPlayerSpawnName(void) const
{
  if (m_spawnName)
    return m_spawnName;

  // default value
  return "info_player_start";
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Add a nav node and connect it.
 * Node Y positions are ground level.
 */
CNavNode *CNavMesh::AddNode(const Vector *destPos, const Vector *normal, NavDirType dir, CNavNode *source)
{
  // check if a node exists at this location
  CNavNode *node = CNavNode::GetNode(destPos);
  // if no node exists, create one
  bool useNew = false;
  if (node == NULL)
  {
    node = new CNavNode(destPos, normal, source);
    useNew = true;
  }


  // connect source node to new node
  source->ConnectTo(node, dir);

  // optimization: if deltaZ changes very little, assume connection is commutative
  const float yTolerance = 0.05f;
  if (fabs(source->GetPosition()->y - destPos->y) < yTolerance)
  {
    node->ConnectTo(source, OppositeDirection(dir));
    node->MarkAsVisited(OppositeDirection(dir));
  }

  if (useNew)
  {
    // new node becomes current node
    // debug("node %.3f,%.3f,%.3f becomes current",node->GetPosition()->x,node->GetPosition()->y,node->GetPosition()->z );
    m_currentNode = node;
  }

  // check if there is enough room to stand at this point, or if we need to crouch
  // check an area, not just a point

  Vector floor, ceiling;
  bool crouch = false;
  for (float z = -::navOptions.HumanRadius; !crouch && z <= ::navOptions.HumanRadius + 0.02f; z += ::navOptions.HumanRadius)
  {
    for (float x = -::navOptions.HumanRadius; x <= ::navOptions.HumanRadius + 0.02f; x += ::navOptions.HumanRadius)
    {
      floor.x = destPos->x + x;
      floor.z = destPos->z + z;
      floor.y = destPos->y + 0.1f;

      ceiling.x = floor.x;
      ceiling.z = floor.z;
      ceiling.y = destPos->y + ::navOptions.HumanHeight - 0.02f;

      Point3 dir = floor - ceiling;
      float dist = length(dir);
      if (nav_rt_trace_ray(floor, dir / dist, dist))
      {
        crouch = true;
        break;
      }
    }
  }

  if (crouch)
    node->SetAttributes(node->GetAttributes() | NAV_MESH_CROUCH);

  return node;
}

//--------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------

static int navAreaCount = 0;
static int currentIndex = 0;
const float updateTimesliceDuration = 0.5f;
extern int gmsgBotProgress;

// cheap-o progress meter
#define PROGRESS_TICKS 40
static void drawProgressMeter(char *title, float progress, char *vguiLabelText)
{
  char progressBuffer[256];

  int cursor = (int)(progress * (PROGRESS_TICKS - 1));

  // don't redraw if cursor hasn't moved
  static int prevCursor = 99999;
  if (cursor == prevCursor)
    return;
  prevCursor = cursor;

  char *c = progressBuffer;

  if (title)
  {
    while (true)
    {
      if (*title == '\000')
        break;

      *c++ = *title++;
    }
    *c++ = '\n';
  }

  *c++ = '[';
  for (int i = 0; i < PROGRESS_TICKS; ++i)
    *c++ = (i < cursor) ? '#' : '_';
  *c++ = ']';
  *c = '\000';

  // HintMessageToAllPlayers( progressBuffer );

  /* BOTPORT: Wire up learning progress bar

  MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
  WRITE_BYTE( 0 ); // don't create/hide progress bar
  WRITE_BYTE( (int)(progress * 100) );
  WRITE_STRING( vguiLabelText );
  MESSAGE_END();
  */
}

static void startProgressMeter(const char *title)
{
  /*
  MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
  WRITE_BYTE( 1 ); // create progress bar
  WRITE_STRING( title );
  MESSAGE_END();
  */
}

static void hideProgressMeter()
{
  /*
  MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
  WRITE_BYTE( 2 ); // hide progress bar
  MESSAGE_END();
  */
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Search the world and build a map of possible movements.
 * The algorithm begins at the bot's current location, and does a recursive search
 * outwards, tracking all valid steps and generating a directed graph of CNavNodes.
 *
 * Sample the map one "step" in a cardinal direction to learn the map.
 *
 * Returns true if sampling needs to continue, or false if done.
 */

bool CNavMesh::isFreeCapsuleToPosition(const Point3 &pt) const
{
  Capsule c;
  c.set(pt + Point3(0, ::navOptions.HumanDownHeightCheck + ::navOptions.HumanRadius /*::navOptions.HumanCapsulePointDown*/, 0),
    pt + Point3(0,
           ::navOptions.HumanHeight - ::navOptions.HumanRadius
           /*::navOptions.HumanCapsulePointUp*/,
           0),
    navOptions.HumanRadius /*::navOptions.HalfHumanWidth*/);
  Point3 cpt, wpt;

  return nav_rt_clip_capsule(c, cpt, wpt) >= -1e-3f;
}


bool CNavMesh::SampleStep(IGenericProgressIndicator *progress_ind)
{
  switch (sampleModeStep)
  {
    case SampleModeStep_GeneratePoints:
    {
      BBox3 boundingbox = nav_rt_get_bbox3();

      static int count = 0;
      count++;
      if (count > 100)
      {
        count = 0;

        if (progress_ind)
        {
          Point3 size = boundingbox.width();
          Point2 steps = Point2(size.x / navOptions.SpawnPointsStepSize + 1, size.z / navOptions.SpawnPointsStepSize + 1);

          Point2 possteps = Point2(currentPointPositionXZ[0] - boundingbox[0][0], currentPointPositionXZ[1] - boundingbox[0][2]);
          possteps /= navOptions.SpawnPointsStepSize;

          int total = (int)(steps[0] * steps[1]);
          int done = (int)(possteps[0] + possteps[1] * steps[1]);
          if (done > total)
            done = total;

          progress_ind->setTotal(total);
          progress_ind->setDone(done);
          progress_ind->setActionDesc("Sampling walkable space : generate points");
        }
      }

      currentPointPositionXZ[0] += ::navOptions.SpawnPointsStepSize;
      if (currentPointPositionXZ[0] >= boundingbox[1].x)
      {
        currentPointPositionXZ[0] = boundingbox[0].x;
        currentPointPositionXZ[1] += ::navOptions.SpawnPointsStepSize;
        if (currentPointPositionXZ[1] >= boundingbox[1].z)
        {
          sampleModeStep = SampleModeStep_AddNodes;
          currentPointPositionXZ = Point2(boundingbox[0][0], boundingbox[0][2]);
          maxPointsCount = points.size();
          break;
        }
      }

      real currenty = boundingbox[1].y;

      while (1)
      {
        Point3 pos(currentPointPositionXZ[0], currenty, currentPointPositionXZ[1]);

        real dist = 99999.0f;

        Point3 normal;
        if (!nav_rt_trace_ray_norm(pos, Point3(0, -1, 0), dist, normal))
          break;

        Point3 pt = pos + Point3(0, -1, 0) * (dist - 0.1f);
        {
          // pt - finded point
          // normal - finded normal

          if (nav_is_relevant_point(pt))
          {
            if (isFreeCapsuleToPosition(pt))
            {
              SamplePoint samplepoint;
              samplepoint.pos = pt;
              samplepoint.normal = normal;
              points.push_back(samplepoint);
            }
          }
        }

        currenty -= dist + 0.1f;
      }
    }
    break;

    case SampleModeStep_AddNodes:
    {
      static int count = 0;
      count++;
      if (count > 1000)
      {
        count = 0;

        if (progress_ind)
        {
          progress_ind->setTotal(maxPointsCount);
          progress_ind->setDone(maxPointsCount - points.size());
          progress_ind->setActionDesc(String(100, "Sampling walkable space : add nodes (%d)", points.size()));
        }
      }

      // take a step
      while (true)
      {
        if (!m_currentNode)
        {
          // Msg("Trying next step");
          //  sampling is complete from current seed, try next one
          while (points.size())
          {
            SamplePoint samplepoint = points.back();
            erase_items(points, points.size() - 1, 1);

            if (CNavNode::GetNode(&samplepoint.pos, TheNavMesh->getUsableGenerationStepSize() * 1.1f))
            {
              continue;
            }

            m_currentNode = new CNavNode(&samplepoint.pos, &samplepoint.normal, NULL);
            break;
          }

          if (!m_currentNode)
          {
            Msg("No more steps");
            // all seeds exhausted, sampling complete
            return false;
          }
        }

        //
        // Take a step from this node
        //
        for (int dir = NORTH; dir < NUM_DIRECTIONS; dir++)
        {
          if (!m_currentNode->HasVisited((NavDirType)dir))
          {
            // have not searched in this direction yet

            // start at current node position
            Vector pos = *m_currentNode->GetPosition();
            // Msg("Found " FMT_P3 "...", P3D(pos));
            // Msg("::navOptions.GenerationStepSize = %f",navOptions.GenerationStepSize);
            //  snap to grid
            int cx = pos.x / TheNavMesh->getUsableGenerationStepSize();
            int cz = pos.z / TheNavMesh->getUsableGenerationStepSize();
            // Msg("cx=%d, cz = %d", cx, cz);

            // attempt to move to adjacent node
            switch (dir)
            {
              case NORTH: --cz; break;
              case SOUTH: ++cz; break;
              case EAST: ++cx; break;
              case WEST: --cx; break;
            }

            // Msg("cx=%d, cz = %d", cx, cz);

            pos.x = cx * TheNavMesh->getUsableGenerationStepSize();
            pos.z = cz * TheNavMesh->getUsableGenerationStepSize();
            // Msg("Next " FMT_P3 "...", P3D(pos));

            m_generationDir = (NavDirType)dir;

            // mark direction as visited
            m_currentNode->MarkAsVisited(m_generationDir);

            // test if we can move to new position
            Vector from, to;

            // modify position to account for change in ground level during step
            to.x = pos.x;
            to.z = pos.z;
            from = *m_currentNode->GetPosition();

            Vector toNormal;
            if (GetGroundHeight(&pos, &to.y, &toNormal) == false)
            {
              return true;
            }

            if (!nav_is_relevant_point(to))
            {
              return true;
            }

            // Msg("Trying point " FMT_P3 " ", P3D(pos));
            from = *m_currentNode->GetPosition();

            Vector fromOrigin = from + Vector(0, ::navOptions.HumanHeightCrouch, 0);
            Vector toOrigin = to + Vector(0, ::navOptions.HumanHeightCrouch, 0);

            Vector dir = toOrigin - fromOrigin;
            float dist = length(dir);

            bool walkable = true, jumpable = false;

            float epsilon = 0.1f;

            if (!nav_rt_trace_ray(fromOrigin, dir / dist, dist))
            {
              // the trace didn't hit anything - clear
              // Msg("No hits");

              float toGround = to.y;
              float fromGround = from.y;

              // debug("toGround=%f, fromGround=%f", toGround, fromGround);
              //  check if ledge is too high to reach or will cause us to fall to our death
              if (
                toGround - fromGround > ::navOptions.HumanDownHeightCheck + epsilon || fromGround - toGround > ::navOptions.DeathDrop)
              {
                // debug("ledge is too high to reach or will cause us to fall to our death");
                walkable = false;
                jumpable = false;
              }

              if (walkable)
              {
                // check surface normals along this step to see if we would cross any impassable slopes
                Vector delta = to - from;
                const float inc = 0.05;
                float along = inc;
                bool done = false;
                float ground;
                Vector normal;

                while (!done)
                {
                  Vector p;

                  // need to guarantee that we test the exact edges
                  if (along >= TheNavMesh->getUsableGenerationStepSize())
                  {
                    p = to;
                    done = true;
                  }
                  else
                  {
                    p = from + delta * (along / TheNavMesh->getUsableGenerationStepSize());
                  }

                  if (GetGroundHeight(&p, &ground, &normal) == false)
                  {
                    // debug("oops 1");
                    walkable = false;
                    break;
                  }

                  // check for maximum allowed slope
                  if (normal.y < ::navOptions.MaxSurfaceSlope)
                  {
                    // debug("normal.y = %f, ::navOptions.MaxUnitYSlope = %f",normal.y,::navOptions.MaxUnitYSlope);
                    walkable = false;
                  }
                  along += inc;
                } // while
              }
            }
            else // TraceLine hit something...
            {
              // Msg("Hit, means not walkable");
              walkable = false;
            }

            /*
            if (jumpable)
            {
            jumpable=false;

            if (to.y >= from.y && to.y-from.y <=
            ::navOptions.JumpCrouchHeight+epsilon)
            {
            // check for jump up
            dist=to.y-from.y;

            if (!nav_rt_trace_ray(from+Point3(0, ::navOptions.HumanHeight, 0),
            Point3(0, 1, 0), dist))
            {
            Point3 pt=from;
            pt.y=to.y+::navOptions.HumanHeight;

            dir=to-from;
            dir.y=0;
            dist=length(dir);

            if (!nav_rt_trace_ray(pt, dir/dist, dist))
            jumpable=true;
            }
            }
            }
            //*/

            if ((walkable || jumpable) && !isFreeCapsuleToPosition(to))
            {
              walkable = false;
              jumpable = false;
            }

            if (walkable || jumpable)
            {
              {
                int index = findPoint(to, TheNavMesh->getUsableGenerationStepSize() * 1.1f);
                if (index != -1)
                  erase_items(points, index, 1);
              }

              // we can move here
              // create a new navigation node, and update current node pointer

              // static int totalNodes = 0;
              // PerformanceTimer2 t1;
              // t1.go();

              CNavNode *node = AddNode(&to, &toNormal, m_generationDir, m_currentNode);
              // debug("Adding node %p at " FMT_P3 ", normal = " FMT_P3 "", node, P3D(to), P3D(toNormal));
              // if (node && jumpable && !walkable)
              //   node->SetAttributes(node->GetAttributes() | NAV_MESH_JUMP);

              // t1.stop();
              // totalNodes++;
              // debug("node %d added in %f ms", totalNodes, float(t1.getTotalUsec())/1000.0);
            }

            return true;
          }
        }

        // all directions have been searched from this node - pop back to its parent and continue
        m_currentNode = m_currentNode->GetParent();
      }
    }
    break;


    default: G_ASSERT(0); return false;
  }

  return true;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Add given walkable position to list of seed positions for map sampling
 */
/*!!!!!!!!!
void CNavMesh::AddWalkableSeed( const Vector &pos, const Vector &normal )
{
WalkableSeedSpot seed;

seed.pos = pos;
seed.normal = normal;

m_walkableSeedList.AddToTail( seed );
}
*/

//--------------------------------------------------------------------------------------------------------------
/**
 * Return the next walkable seed as a node
 */
/*!!!!!!!!!!
CNavNode *CNavMesh::GetNextWalkableSeedNode( void )
{
if (!m_walkableSeedList.IsValidIndex( m_seedIdx ))
return NULL;

WalkableSeedSpot spot = m_walkableSeedList.Element( m_seedIdx );

m_seedIdx = m_walkableSeedList.Next( m_seedIdx );

debug("GetNextWalkableSeedNode : " FMT_P3 "", P3D(spot.pos));
return new CNavNode( &spot.pos, &spot.normal, NULL );
}
*/

void CNavMesh::GenerateZoneInfo(void)
{
  for (int i = 0; i < GetNavAreaCount(); i++)
  {
    CNavArea *area = TheNavAreaList[i];
    area->SetZone(UNKNOWN_ZONE);
  }

  unsigned int nZone = 0;
  unsigned int nSolo = 0;
  for (int i = 0; i < GetNavAreaCount(); i++)
  {
    CNavArea *area = TheNavAreaList[i];
    if (area->GetZone() != UNKNOWN_ZONE)
      continue;
    unsigned int nCon = 0;
    for (int dir = 0; dir < 4; dir++)
      nCon += area->GetAdjacentCount((NavDirType)dir);
    if (nCon)
    {
      nZone++;
      area->FloodFillZone(nZone);
    }
    else
    {
      nSolo++;
      area->SetZone(SOLO_ZONE);
    }
  }
  Msg("Generated %d zones and %d solo areas", nZone, nSolo);
}
