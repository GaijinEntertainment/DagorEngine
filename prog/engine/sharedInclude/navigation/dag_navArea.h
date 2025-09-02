// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tabUtils.h>

class CNavArea;

//-------------------------------------------------------------------------------------------------------------------
/**
 * The NavConnect union is used to refer to connections to areas
 */
union NavConnect
{
  unsigned int id;
  CNavArea *area;

  bool operator==(const NavConnect &other) const { return (area == other.area) ? true : false; }
};
typedef CUtlLinkedList<NavConnect, int> NavConnectList;

//--------------------------------------------------------------------------------------------------------------
/**
 * A HidingSpot is a good place for a bot to crouch and wait for enemies
 */
class HidingSpot : public INavHidingSpot
{
public:
  HidingSpot(void);                         ///< for use when loading from a file
  HidingSpot(const Vector *pos, int flags); ///< for use when generating - assigns unique ID

  bool HasGoodCover(void) const { return (m_flags & IN_COVER) ? true : false; } ///< return true if hiding spot in in cover
  //  bool IsGoodSniperSpot( void ) const  { return (m_flags & GOOD_SNIPER_SPOT) ? true : false; }
  //  bool IsIdealSniperSpot( void ) const { return (m_flags & IDEAL_SNIPER_SPOT) ? true : false; }

  virtual Point3 getPos() const { return m_pos; }

  virtual int getFlags(void) const { return m_flags; }
  void SetFlags(int flags) { m_flags |= flags; } ///< FOR INTERNAL USE ONLY
  // int  GetFlags( void ) const  { return m_flags; }

  void Save(IGenSave &crd) const;
  void Load(IGenLoad &crd);

  const Vector *GetPosition(void) const { return &m_pos; } ///< get the position of the hiding spot
  unsigned int GetID(void) const { return m_id; }

  void Mark(void) { m_marker = m_masterMarker; }
  bool IsMarked(void) const { return (m_marker == m_masterMarker) ? true : false; }
  static void ChangeMasterMarker(void) { ++m_masterMarker; }

private:
  friend class CNavMesh;

  Vector m_pos;          ///< world coordinates of the spot
  unsigned int m_id;     ///< this spot's unique ID
  unsigned int m_marker; ///< this spot's unique marker

  unsigned char m_flags; ///< bit flags

  static unsigned int m_nextID;       ///< used when allocating spot ID's
  static unsigned int m_masterMarker; ///< used to mark spots
};

typedef CUtlLinkedList<HidingSpot *, int> HidingSpotList;
extern HidingSpotList TheHidingSpotList;

extern HidingSpot *GetHidingSpotByID(unsigned int id);

//--------------------------------------------------------------------------------------------------------------
/**
 * Stores a pointer to an interesting "spot", and a parametric distance along a path
 */
struct SpotOrder
{
  float t; ///< parametric distance along ray where this spot first has LOS to our path
  union
  {
    HidingSpot *spot; ///< the spot to look at
    unsigned int id;  ///< spot ID for save/load
  };
};
typedef CUtlLinkedList<SpotOrder, int> SpotOrderList;

/**
 * This struct stores possible path segments thru a CNavArea, and the dangerous spots
 * to look at as we traverse that path segment.
 */
struct SpotEncounter
{
  NavConnect from;
  NavDirType fromDir;
  NavConnect to;
  NavDirType toDir;
  Ray path;               ///< the path segment
  SpotOrderList spotList; ///< list of spots to look at, in order of occurrence
};
typedef CUtlLinkedList<SpotEncounter *, int> SpotEncounterList;


//-------------------------------------------------------------------------------------------------------------------
/**
 * A CNavArea is a rectangular region defining a walkable area in a map
 */
class CNavArea : public INavArea
{
public:
  CNavArea(CNavNode *nwNode, CNavNode *neNode, CNavNode *seNode, CNavNode *swNode);
  CNavArea(void);
  CNavArea(const Vector *corner, const Vector *otherCorner);
  CNavArea(const Vector *nwCorner, const Vector *neCorner, const Vector *seCorner, const Vector *swCorner);

  ~CNavArea();

  void ConnectTo(CNavArea *area, NavDirType dir); ///< connect this area to given area in given direction
  void Disconnect(CNavArea *area);                ///< disconnect this area from given area

  void Save(IGenSave &crd) const;
  void SaveOverlapIds(IGenSave &crd) const;
  void Load(IGenLoad &crd);
  void LoadOverlapIds(IGenLoad &crd);
  NavErrorType PostLoad(int version);

  unsigned int GetID(void) const { return m_id; }             ///< return this area's unique ID
  virtual unsigned int getIndex(void) const { return index; } ///< return this area's index
  void setIndex(unsigned int i) { index = i; }                ///< return this area's index

  void SetAttributes(int bits) { m_attributeFlags = (unsigned char)bits; }
  int GetAttributes(void) const { return m_attributeFlags; }

  void SetPlace(Place place) { m_place = place; } ///< set place descriptor
  Place GetPlace(void) const { return m_place; }  ///< get place descriptor
  unsigned int GetZone(void) const { return m_zone; }
  void SetZone(unsigned int zone) { m_zone = zone; }
  void FloodFillZone(unsigned int zone);

  bool IsOverlapping(const Vector *pos) const;     ///< return true if 'pos' is within 2D extents of area.
  bool IsOverlapping(const CNavArea *area) const;  ///< return true if 'area' overlaps our 2D extents
  bool IsOverlappingX(const CNavArea *area) const; ///< return true if 'area' overlaps our X extent
  bool IsOverlappingZ(const CNavArea *area) const; ///< return true if 'area' overlaps our Z extent
  // int GetPlayerCount( int teamID = 0 ) const; ///< return number of players with given teamID in this area (teamID==0 means any/all)
  float GetY(const Vector *pos) const;         ///< return Z of area at (x,y) of 'pos'
  float GetY(float x, float z) const;          ///< return Z of area at (x,y) of 'pos'
  bool Contains(const Vector *pos) const;      ///< return true if given point is on or above this area, but no others
  bool IsCoplanar(const CNavArea *area) const; ///< return true if this area and given area are approximately co-planar

  /// return closest point to 'pos' on this area - returned point in 'close'
  void GetClosestPointOnArea(const Vector *pos, Vector *close) const;

  float GetDistanceSquaredToPoint(const Vector *pos) const; ///< return shortest distance between point and this area
  bool IsDegenerate(void) const;                            ///< return true if this area is badly formed
  bool IsRoughlySquare(void) const;                         ///< return true if this area is approximately square

  bool IsEdge(NavDirType dir) const; ///< return true if there are no bi-directional links on the given side

  /// return number of connected areas in given direction
  int GetAdjacentCount(NavDirType dir) const { return m_connect[dir].Count(); }
  CNavArea *GetAdjacentArea(NavDirType dir, int i) const; ///< return the i'th adjacent area in the given direction
  CNavArea *GetRandomAdjacentArea(NavDirType dir) const;

  // dagor gcc
  int getAdjacentCount(NavDirType dir) const { return GetAdjacentCount(dir); }
  INavArea *getAdjacent(NavDirType dir, int i) const { return GetAdjacentArea(dir, i); }

  const NavConnectList *GetAdjacentList(NavDirType dir) const { return &m_connect[dir]; }
  bool IsConnected(const CNavArea *area, NavDirType dir) const; ///< return true if given area is connected in given direction
  float ComputeHeightChange(const CNavArea *area);              ///< compute change in height from this area to given area


  /// compute portal to adjacent area
  void ComputePortal(const CNavArea *to, NavDirType dir, Vector *center, float *halfWidth) const;
  /// compute closest point within the "portal" between to adjacent areas
  void ComputeClosestPointInPortal(const CNavArea *to, NavDirType dir, const Vector *fromPos, Vector *closePos) const;
  /// return direction from this area to the given point
  NavDirType ComputeDirection(const Vector *point) const;

  //- for hunting algorithm ---------------------------------------------------------------------------
  void SetClearedTimestamp(int teamID) { m_clearedTimestamp[teamID] = ::globalTime; } ///< set this area's "clear" timestamp to now
  float GetClearedTimestamp(int teamID) { return m_clearedTimestamp[teamID]; }        ///< get time this area was marked "clear"

  //- hiding spots ------------------------------------------------------------------------------------
  const HidingSpotList *GetHidingSpotList(void) const { return &m_hidingSpotList; }
  void ComputeHidingSpots(void); ///< analyze local area neighborhood to find "hiding spots" in this area - for map learning
  void ComputeSniperSpots(void); ///< analyze local area neighborhood to find "sniper spots" in this area - for map learning

  virtual int getHidingSpotCount() const { return m_hidingSpotList.Count(); }
  virtual INavHidingSpot *getHidingSpot(int index) const { return m_hidingSpotList[index]; }

  /// given the areas we are moving between, return the spots we will encounter
  SpotEncounter *GetSpotEncounter(const CNavArea *from, const CNavArea *to);
  void ComputeSpotEncounters(void); ///< compute spot encounter data - for map learning

  //- "danger" ----------------------------------------------------------------------------------------
  void IncreaseDanger(int teamID, float amount); ///< increase the danger of this area for the given team
  float GetDanger(int teamID);                   ///< return the danger of this area (decays over time)

  float GetSizeX(void) const { return m_extent.hi.x - m_extent.lo.x; }
  float GetSizeZ(void) const { return m_extent.hi.z - m_extent.lo.z; }
  const Extent *GetExtent(void) const { return &m_extent; }
  const Vector *GetCenter(void) const { return &m_center; }
  Vector GetRandom(void);
  const Vector *GetCorner(NavCornerType corner) const;

  //- approach areas ----------------------------------------------------------------------------------
  struct ApproachInfo
  {
    NavConnect here; ///< the approach area
    NavConnect prev; ///< the area just before the approach area on the path
    NavTraverseType prevToHereHow;
    NavConnect next; ///< the area just after the approach area on the path
    NavTraverseType hereToNextHow;
  };
  const ApproachInfo *GetApproachInfo(int i) const { return &m_approach[i]; }
  int GetApproachInfoCount(void) const { return m_approachCount; }
  void ComputeApproachAreas(void); ///< determine the set of "approach areas" - for map learning

  //- A* pathfinding algorithm ------------------------------------------------------------------------
  static void MakeNewMarker(void)
  {
    ++m_masterMarker;
    if (m_masterMarker == 0)
      m_masterMarker = 1;
  }
  void Mark(void) { m_marker = m_masterMarker; }
  BOOL IsMarked(void) const { return (m_marker == m_masterMarker) ? true : false; }

  void SetParent(CNavArea *parent, NavTraverseType how = NUM_TRAVERSE_TYPES)
  {
    m_parent = parent;
    m_parentHow = how;
  }
  CNavArea *GetParent(void) const { return m_parent; }
  NavTraverseType GetParentHow(void) const { return m_parentHow; }

  bool IsOpen(void) const;     ///< true if on "open list"
  void AddToOpenList(void);    ///< add to open list in decreasing value order
  void UpdateOnOpenList(void); ///< a smaller value has been found, update this area on the open list
  void RemoveFromOpenList(void);
  static bool IsOpenListEmpty(void);
  static CNavArea *PopOpenList(void); ///< remove and return the first element of the open list

  bool IsClosed(void) const;  ///< true if on "closed list"
  void AddToClosedList(void); ///< add to the closed list
  void RemoveFromClosedList(void);

  static void ClearSearchLists(void); ///< clears the open and closed lists for a new search

  void SetTotalCost(float value) { m_totalCost = value; }
  float GetTotalCost(void) const { return m_totalCost; }

  void SetCostSoFar(float value) { m_costSoFar = value; }
  float GetCostSoFar(void) const { return m_costSoFar; }

  //- editing -----------------------------------------------------------------------------------------
  void Draw(byte red, byte green, byte blue, int duration = 50); ///< draw area for debugging & editing
  void DrawConnectedAreas(void);
  void DrawMarkedCorner(NavCornerType corner, byte red, byte green, byte blue, int duration = 50);
  /// split this area into two areas at the given edge
  bool SplitEdit(bool splitAlongX, float splitEdge, CNavArea **outAlpha = NULL, CNavArea **outBeta = NULL);
  bool MergeEdit(CNavArea *adj);                      ///< merge this area and given adjacent area
  bool SpliceEdit(CNavArea *other);                   ///< create a new area between this area and given area
  void RaiseCorner(NavCornerType corner, int amount); ///< raise/lower a corner (or all corners if corner == NUM_CORNERS)

  float m_neY;
  float m_swY;

  virtual bool isJump() const;
  virtual const Point3 &getP1() const;
  virtual const Point3 &getP2() const;
  virtual const Point3 &getCenter() const;
  virtual const real getSwY() const;
  virtual const real getNeY() const;

  virtual bool splitEdit(bool split_along_x, real split_edge, INavArea **out_alpha = NULL, INavArea **out_beta = NULL);
  virtual bool mergeEdit(INavArea *adj);
  virtual real getY(real x, real z);
  virtual void connect(INavArea *adj);
  virtual void disconnect(INavArea *adj);

  // hint points
  virtual const Tab<NavHintPoint *> &getHintPoints() const { return hintPoints; }
  void addHintPoint(NavHintPoint *point);
  void removeHintPoint(NavHintPoint *point);

private:
  friend class CNavMesh;

  void Initialize(void); ///< to keep constructors consistent
  static bool m_isReset; ///< if true, don't bother cleaning up in destructor since everything is going away

  static unsigned int m_nextID; ///< used to allocate unique IDs
  unsigned int m_id;            ///< unique area ID
  unsigned int index;           ///< used to find area index
  /// extents of area in world coords (NOTE: lo.z is not necessarily the minimum Z, but corresponds to Z at point (lo.x, lo.y), etc
  Extent m_extent;
  Vector m_center;                ///< centroid of area
  unsigned char m_attributeFlags; ///< set of attribute bit flags (see NavAttributeType)
  Place m_place;                  ///< place descriptor

  /// height of the implicit corners

  // BOTPORT: Clean up relationship between team index and danger storage in nav areas
  static constexpr int MAX_AREA_TEAMS = 2;

  //- for hunting -------------------------------------------------------------------------------------
  float m_clearedTimestamp[MAX_AREA_TEAMS]; ///< time this area was last "cleared" of enemies

  //- "danger" ----------------------------------------------------------------------------------------
  /// danger of this area, allowing bots to avoid areas where they died in the past - zero is no danger
  float m_danger[MAX_AREA_TEAMS];
  float m_dangerTimestamp[MAX_AREA_TEAMS]; ///< time when danger value was set - used for decaying
  void DecayDanger(void);

  //- hiding spots ------------------------------------------------------------------------------------
  HidingSpotList m_hidingSpotList;
  bool IsHidingSpotCollision(const Vector *pos) const; ///< returns true if an existing hiding spot is too close to given position

  //- encounter spots ---------------------------------------------------------------------------------
  SpotEncounterList m_spotEncounterList; ///< list of possible ways to move thru this area, and the spots to look at as we do
  ///< add spot encounter data when moving from area to area
  void AddSpotEncounters(const CNavArea *from, NavDirType fromDir, const CNavArea *to, NavDirType toDir);

  //- approach areas ----------------------------------------------------------------------------------
  static constexpr int MAX_APPROACH_AREAS = 16;
  ApproachInfo m_approach[MAX_APPROACH_AREAS];
  unsigned char m_approachCount;

  void Strip(void); ///< remove "analyzed" data from nav area

  //- A* pathfinding algorithm ------------------------------------------------------------------------
  static unsigned int m_masterMarker;
  unsigned int m_marker;       ///< used to flag the area as visited
  CNavArea *m_parent;          ///< the area just prior to this on in the search path
  NavTraverseType m_parentHow; ///< how we get from parent to us
  float m_totalCost;           ///< the distance so far plus an estimate of the distance left
  float m_costSoFar;           ///< distance travelled so far

  static CNavArea *m_openList;
  CNavArea *m_nextOpen, *m_prevOpen; ///< only valid if m_openMarker == m_masterMarker
  unsigned int m_openMarker;         ///< if this equals the current marker value, we are on the open list

  //- connections to adjacent areas -------------------------------------------------------------------
  NavConnectList m_connect[NUM_DIRECTIONS]; ///< a list of adjacent areas for each direction

  //---------------------------------------------------------------------------------------------------
  CNavNode *m_node[NUM_CORNERS]; ///< nav nodes at each corner of the area

  void FinishMerge(CNavArea *adjArea);              ///< recompute internal data once nodes have been adjusted during merge
  void MergeAdjacentConnections(CNavArea *adjArea); ///< for merging with "adjArea" - pick up all of "adjArea"s connections
  void AssignNodes(CNavArea *area);                 ///< assign internal nodes to the given area

  void FinishSplitEdit(CNavArea *newArea, NavDirType ignoreEdge); ///< given the portion of the original area, update its internal data

  CUtlLinkedList<CNavArea *, int> m_overlapList; ///< list of areas that overlap this area

  void OnDestroyNotify(CNavArea *dead); ///< invoked when given area is going away

  CNavArea *m_prevHash, *m_nextHash; ///< for hash table in CNavMesh

  unsigned int m_zone;

  Tab<NavHintPoint *> hintPoints;
};

typedef CUtlLinkedList<CNavArea *, int> NavAreaList;
extern NavAreaList TheNavAreaList;


//--------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------
//
// Inlines
//

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsDegenerate(void) const { return (m_extent.lo.x >= m_extent.hi.x || m_extent.lo.z >= m_extent.hi.z); }

//--------------------------------------------------------------------------------------------------------------
inline CNavArea *CNavArea::GetAdjacentArea(NavDirType dir, int i) const
{
  int iter;
  for (iter = m_connect[dir].Head(); iter != m_connect[dir].InvalidIndex(); iter = m_connect[dir].Next(iter))
  {
    if (i == 0)
      return m_connect[dir][iter].area;
    --i;
  }

  return NULL;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsOpen(void) const { return (m_openMarker == m_masterMarker) ? true : false; }

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsOpenListEmpty(void) { return (m_openList) ? false : true; }

//--------------------------------------------------------------------------------------------------------------
inline CNavArea *CNavArea::PopOpenList(void)
{
  if (m_openList)
  {
    CNavArea *area = m_openList;

    // disconnect from list
    area->RemoveFromOpenList();

    return area;
  }

  return NULL;
}

//--------------------------------------------------------------------------------------------------------------
inline bool CNavArea::IsClosed(void) const
{
  if (IsMarked() && !IsOpen())
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::AddToClosedList(void) { Mark(); }

//--------------------------------------------------------------------------------------------------------------
inline void CNavArea::RemoveFromClosedList(void)
{
  // since "closed" is defined as visited (marked) and not on open list, do nothing
}

inline bool CNavArea::isJump() const { return (GetAttributes() & NAV_MESH_JUMP); }

inline const Point3 &CNavArea::getP1() const { return GetExtent()->lo; }

inline const Point3 &CNavArea::getP2() const { return GetExtent()->hi; }

inline const Point3 &CNavArea::getCenter() const { return m_center; }

inline const real CNavArea::getSwY() const { return m_swY; }

inline const real CNavArea::getNeY() const { return m_neY; }

inline real CNavArea::getY(real x, real z) { return GetY(x, z); }

inline void CNavArea::addHintPoint(NavHintPoint *point) { hintPoints.push_back(point); }

inline void CNavArea::removeHintPoint(NavHintPoint *point) { tabutils::erase(hintPoints, point); }
