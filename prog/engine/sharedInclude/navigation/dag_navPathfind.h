
#ifndef _NAV_PATHFIND_H_
#define _NAV_PATHFIND_H_

//-------------------------------------------------------------------------------------------------------------------
/**
 * Used when building a path to determine the kind of path to build
 */
enum RouteType
{
  FASTEST_ROUTE,
  SAFEST_ROUTE,
};


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class ShortestPathCost
{
public:
  float operator()(CNavArea *area, CNavArea *fromArea)
  {
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

      // if this is a "crouch" area, add penalty
      if (area->GetAttributes() & NAV_MESH_CROUCH)
      {
        const float crouchPenalty = 20.0f; // 10
        cost += crouchPenalty * dist;
      }

      // if this is a "jump" area, add penalty
      if (area->GetAttributes() & NAV_MESH_JUMP)
      {
        const float jumpPenalty = 5.0f;
        cost += jumpPenalty * dist;
      }

      return cost;
    }
  }
};

class ShortestRandomPathCost
{
public:
  float operator()(CNavArea *area, CNavArea *fromArea)
  {
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

      // if this is a "crouch" area, add penalty
      if (area->GetAttributes() & NAV_MESH_CROUCH)
      {
        const float crouchPenalty = 20.0f; // 10
        cost += crouchPenalty * dist;
      }

      // if this is a "jump" area, add penalty
      if (area->GetAttributes() & NAV_MESH_JUMP)
      {
        const float jumpPenalty = 5.0f;
        cost += jumpPenalty * dist;
      }

      cost += gfrnd() * 20.0f;

      return cost;
    }
  }
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Find path from startArea to goalArea via an A* search, using supplied cost heuristic.
 * If cost functor returns -1 for an area, that area is considered a dead end.
 * This doesn't actually build a path, but the path is defined by following parent
 * pointers back from goalArea to startArea.
 * If 'closestArea' is non-NULL, the closest area to the goal is returned (useful if the path fails).
 * If 'goalArea' is NULL, will compute a path as close as possible to 'goalPos'.
 * If 'goalPos' is NULL, will use the center of 'goalArea' as the goal position.
 * Returns true if a path exists.
 */
template <typename CostFunctor>
bool NavAreaBuildPath(CNavArea *startArea, CNavArea *goalArea, const Vector *goalPos, CostFunctor &costFunc,
  CNavArea **closestArea = NULL)
{
  if (startArea == NULL)
    return false;

  if (goalArea == NULL && goalPos == NULL)
    return false;

  startArea->SetParent(NULL);

  // if we are already in the goal area, build trivial path
  if (startArea == goalArea)
  {
    goalArea->SetParent(NULL);
    return true;
  }

  // determine actual goal position
  Vector actualGoalPos = (goalPos) ? *goalPos : *goalArea->GetCenter();

  // start search
  CNavArea::ClearSearchLists();

  // compute estimate of path length
  /// @todo Cost might work as "manhattan distance"
  startArea->SetTotalCost((*startArea->GetCenter() - actualGoalPos).length());

  float initCost = costFunc(startArea, NULL);
  if (initCost < 0.0f)
    return false;
  startArea->SetCostSoFar(initCost);

  startArea->AddToOpenList();

  // keep track of the area we visit that is closest to the goal
  if (closestArea)
    *closestArea = startArea;
  float closestAreaDist = startArea->GetTotalCost();

  // do A* search
  while (!CNavArea::IsOpenListEmpty())
  {
    // get next area to check
    CNavArea *area = CNavArea::PopOpenList();

    // check if we have found the goal area
    if (area == goalArea)
    {
      return true;
    }

    // search adjacent areas
    bool searchFloor = true;
    int dir = NORTH;
    const NavConnectList *floorList = area->GetAdjacentList(NORTH);
    int floorIter = floorList->Head();

    enum
    {
      AHEAD = 0,
      LEFT,
      RIGHT,
      BEHIND,
      NUM_TOP_DIRECTIONS
    };

    while (true)
    {
      CNavArea *newArea;
      NavTraverseType how;

      //
      // Get next adjacent area - either on floor or via ladder
      //
      if (searchFloor)
      {
        // if exhausted adjacent connections in current direction, begin checking next direction
        if (floorIter == floorList->InvalidIndex())
        {
          ++dir;

          if (dir == NUM_DIRECTIONS)
          {
            // checked all directions on floor - check ladders next
            searchFloor = false;
          }
          else
          {
            // start next direction
            floorList = area->GetAdjacentList((NavDirType)dir);
            floorIter = floorList->Head();
          }

          continue;
        }

        newArea = floorList->Element(floorIter).area;
        how = (NavTraverseType)dir;
        floorIter = floorList->Next(floorIter);
      }
      else // search ladders
      {
        break;
      }

      // don't backtrack
      if (!newArea)
        continue; // WORKAROUND!!!
      if (newArea == area)
        continue;

      float newCostSoFar = costFunc(newArea, area);

      // check if cost functor says this area is a dead-end
      if (newCostSoFar < 0.0f)
        continue;

      if ((newArea->IsOpen() || newArea->IsClosed()) && newArea->GetCostSoFar() <= newCostSoFar)
      {
        // this is a worse path - skip it
        continue;
      }
      else
      {
        // compute estimate of distance left to go
        float newCostRemaining = (*newArea->GetCenter() - actualGoalPos).length();

        // track closest area to goal in case path fails
        if (closestArea && newCostRemaining < closestAreaDist)
        {
          *closestArea = newArea;
          closestAreaDist = newCostRemaining;
        }

        newArea->SetParent(area, how);
        newArea->SetCostSoFar(newCostSoFar);
        newArea->SetTotalCost(newCostSoFar + newCostRemaining);

        if (newArea->IsClosed())
          newArea->RemoveFromClosedList();

        if (newArea->IsOpen())
        {
          // area already on open list, update the list order to keep costs sorted
          newArea->UpdateOnOpenList();
        }
        else
        {
          newArea->AddToOpenList();
        }
      }
    }

    // we have searched this area
    area->AddToClosedList();
  }

  return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Compute distance between two areas. Return -1 if can't reach 'endArea' from 'startArea'.
 */
template <typename CostFunctor>
float NavAreaTravelDistance(CNavArea *startArea, CNavArea *endArea, CostFunctor &costFunc)
{
  if (startArea == NULL)
    return -1.0f;

  if (endArea == NULL)
    return -1.0f;

  if (startArea == endArea)
    return 0.0f;

  // compute path between areas using given cost heuristic
  if (NavAreaBuildPath(startArea, endArea, NULL, costFunc) == false)
    return -1.0f;

  // compute distance along path
  float distance = 0.0f;
  for (CNavArea *area = endArea; area->GetParent(); area = area->GetParent())
  {
    distance += (*area->GetCenter() - *area->GetParent()->GetCenter()).length();
  }

  return distance;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Compute distance from area to position. Return -1 if can't reach position.
 */
template <typename CostFunctor>
float NavAreaTravelDistance(const Vector *startPos, CNavArea *startArea, const Vector *goalPos, CostFunctor &costFunc)
{
  if (startArea == NULL || startPos == NULL || goalPos == NULL)
    return -1.0f;

  // compute path between areas using given cost heuristic
  CNavArea *goalArea = NULL;
  if (NavAreaBuildPath(startArea, NULL, goalPos, costFunc, &goalArea) == false)
    return -1.0f;

  // compute distance along path
  if (goalArea->GetParent() == NULL)
  {
    return (*goalPos - *startPos).length();
  }
  else
  {
    CNavArea *area = goalArea->GetParent();

    float distance = (*goalPos - *area->GetCenter()).length();

    for (; area->GetParent(); area = area->GetParent())
    {
      distance += (*area->GetCenter() - *area->GetParent()->GetCenter()).length();
    }

    return distance;
  }
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Do a breadth-first search, invoking functor on each area.
 * If functor returns 'true', continue searching from this area.
 * If functor returns 'false', the area's adjacent areas are not explored (dead end).
 * If 'maxRange' is 0 or less, no range check is done (all areas will be examined).
 *
 * NOTE: Returns all areas that overlap range, even partially
 *
 * @todo Use ladder connections
 */

// helper function
inline void AddAreaToOpenList(CNavArea *area, CNavArea *parent, const Vector *startPos, float maxRange)
{
  if (area == NULL)
    return;

  if (!area->IsMarked())
  {
    area->Mark();
    area->SetTotalCost(0.0f);
    area->SetParent(parent);

    if (maxRange > 0.0f)
    {
      // make sure this area overlaps range
      Vector closePos;
      area->GetClosestPointOnArea(startPos, &closePos);
      Point2 flatV((closePos - *startPos).x, (closePos - *startPos).z);
      if (length(flatV) < maxRange)
      {
        // compute approximate distance along path to limit travel range, too
        float distAlong = parent->GetCostSoFar();
        distAlong += (*area->GetCenter() - *parent->GetCenter()).length();
        area->SetCostSoFar(distAlong);

        // allow for some fudge due to large size areas
        if (distAlong <= 1.5f * maxRange)
          area->AddToOpenList();
      }
    }
    else
    {
      // infinite range
      area->AddToOpenList();
    }
  }
}


template <typename Functor>
void SearchSurroundingAreas(CNavArea *startArea, const Vector *startPos, Functor &func, float maxRange = -1.0f)
{
  if (startArea == NULL || startPos == NULL)
    return;

  CNavArea::MakeNewMarker();
  CNavArea::ClearSearchLists();

  startArea->AddToOpenList();
  startArea->SetTotalCost(0.0f);
  startArea->SetCostSoFar(0.0f);
  startArea->SetParent(NULL);
  startArea->Mark();

  while (!CNavArea::IsOpenListEmpty())
  {
    // get next area to check
    CNavArea *area = CNavArea::PopOpenList();

    // invoke functor on area
    if (func(area))
    {
      // explore adjacent floor areas
      for (int dir = 0; dir < NUM_DIRECTIONS; ++dir)
      {
        int count = area->GetAdjacentCount((NavDirType)dir);
        for (int i = 0; i < count; ++i)
        {
          CNavArea *adjArea = area->GetAdjacentArea((NavDirType)dir, i);

          AddAreaToOpenList(adjArea, area, startPos, maxRange);
        }
      }
    }
  }
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Fuctor that returns lowest cost for farthest away areas
 * For use with FindMinimumCostArea()
 */
class FarAwayFunctor
{
public:
  float operator()(CNavArea *area, CNavArea *fromArea)
  {
    if (area == fromArea)
      return 9999999.9f;

    return 1.0f / (*fromArea->GetCenter() - *area->GetCenter()).length();
  }
};

/**
 * Fuctor that returns lowest cost for areas farthest from given position
 * For use with FindMinimumCostArea()
 */
class FarAwayFromPositionFunctor
{
public:
  FarAwayFromPositionFunctor(const Vector *pos) { m_pos = pos; }

  float operator()(CNavArea *area, CNavArea *fromArea) { return 1.0f / (*m_pos - *area->GetCenter()).length(); }

private:
  const Vector *m_pos;
};


/**
 * Pick a low-cost area of "decent" size
 */
template <typename CostFunctor>
CNavArea *FindMinimumCostArea(CNavArea *startArea, CostFunctor &costFunc)
{
  const float minSize = 150.0f * 0.0254;

  // collect N low-cost areas of a decent size
  static constexpr int NUM_CHEAP_AREAS = 32;
  struct
  {
    CNavArea *area;
    float cost;
  } cheapAreaSet[NUM_CHEAP_AREAS];
  int cheapAreaSetCount = 0;

  FOR_EACH_LL(TheNavAreaList, iter)
  {
    CNavArea *area = TheNavAreaList[iter];

    // skip the small areas
    const Extent *extent = area->GetExtent();
    if (extent->hi.x - extent->lo.x < minSize || extent->hi.z - extent->lo.z < minSize)
      continue;

    // compute cost of this area
    float cost = costFunc(area, startArea, NULL);

    if (cheapAreaSetCount < NUM_CHEAP_AREAS)
    {
      cheapAreaSet[cheapAreaSetCount].area = area;
      cheapAreaSet[cheapAreaSetCount++].cost = cost;
    }
    else
    {
      // replace most expensive cost if this is cheaper
      int expensive = 0;
      for (int i = 1; i < NUM_CHEAP_AREAS; ++i)
        if (cheapAreaSet[i].cost > cheapAreaSet[expensive].cost)
          expensive = i;

      if (cheapAreaSet[expensive].cost > cost)
      {
        cheapAreaSet[expensive].area = area;
        cheapAreaSet[expensive].cost = cost;
      }
    }
  }

  if (cheapAreaSetCount)
  {
    // pick one of the areas at random
    return cheapAreaSet[RandomInt(0, cheapAreaSetCount - 1)].area;
  }
  else
  {
    // degenerate case - no decent sized areas - pick a random area
    int numAreas = TheNavAreaList.Count();
    int which = RandomInt(0, numAreas - 1);

    FOR_EACH_LL(TheNavAreaList, iter)
    {
      if (which-- == 0)
        return TheNavAreaList[iter];
    }
  }
  return cheapAreaSet[RandomInt(0, cheapAreaSetCount - 1)].area;
}


#endif // _NAV_PATHFIND_H_
