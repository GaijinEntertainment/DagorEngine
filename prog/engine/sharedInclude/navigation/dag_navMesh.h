
#ifndef _NAV_MESH_H_
#define _NAV_MESH_H_

#include <navigation/dag_navigation.h>
#include <navigation/dag_navArea.h>
#include <generic/dag_hierGrid.h>


class CNavArea;

extern ConVar nav_edit;
extern ConVar nav_quicksave;
extern ConVar nav_show_approach_points;
extern ConVar nav_show_danger;
class IGenericProgressIndicator;

enum NavEditCmdType
{
  EDIT_NONE,
  EDIT_DELETE,                ///< delete current area
  EDIT_SPLIT,                 ///< split current area
  EDIT_MERGE,                 ///< merge adjacent areas
  EDIT_JOIN,                  ///< define connection between areas
  EDIT_BREAK,                 ///< break connection between areas
  EDIT_MARK,                  ///< mark an area for further operations
  EDIT_ATTRIB_CROUCH,         ///< toggle crouch attribute on current area
  EDIT_ATTRIB_JUMP,           ///< toggle jump attribute on current area
  EDIT_ATTRIB_PRECISE,        ///< toggle precise attribute on current area
  EDIT_ATTRIB_NO_JUMP,        ///< toggle inhibiting discontinuity jumping in current area
  EDIT_BEGIN_AREA,            ///< begin creating a new nav area
  EDIT_END_AREA,              ///< end creation of the new nav area
  EDIT_CONNECT,               ///< connect marked area to selected area
  EDIT_DISCONNECT,            ///< disconnect marked area from selected area
  EDIT_SPLICE,                ///< create new area in between marked and selected areas
  EDIT_TOGGLE_PLACE_MODE,     ///< switch between normal and place editing
  EDIT_TOGGLE_PLACE_PAINTING, ///< switch between "painting" places onto areas
  EDIT_PLACE_FLOODFILL,       ///< floodfill areas out from current area
  EDIT_PLACE_PICK,            ///< "pick up" the place at the current area
  EDIT_MARK_UNNAMED,          ///< mark an unnamed area for further operations
  EDIT_WARP_TO_MARK,          ///< warp a spectating local player to the selected mark
  EDIT_SELECT_CORNER,         ///< select a corner on the current area
  EDIT_RAISE_CORNER,          ///< raise a corner on the current area
  EDIT_LOWER_CORNER,          ///< lower a corner on the current area
};

//--------------------------------------------------------------------------------------------------------
/**
 * The CNavMesh is the global interface to the Navigation Mesh.
 * @todo Make this an abstract base class interface, and derive mod-specific implementations.
 */
class CNavMesh
{
public:
  CNavMesh(void);
  virtual ~CNavMesh();

  void Reset(bool reset_hint_points = true); ///< destroy Navigation Mesh data and revert to initial state
  void Update(void);                         ///< invoked on each game frame

  NavErrorType Load(IGenLoad &crd);                ///< load navigation data from a file
  bool IsLoaded(void) const { return m_isLoaded; } ///< return true if a Navigation Mesh has been loaded
  // available for TOOLS only.
  bool Save(IGenSave &crd); ///< store Navigation Mesh to a file

  unsigned int GetNavAreaCount(void) const { return m_areaCount; } ///< return total number of nav areas

  CNavArea *GetNavArea(const Vector *pos, float beneathLimt = 120.0f * 0.0254f) const; ///< given a position, return the nav area that
                                                                                       ///< IsOverlapping and is *immediately* beneath
                                                                                       ///< it
  CNavArea *GetNavAreaByID(unsigned int id) const;
  CNavArea *GetNearestNavArea(const Vector *pos, bool anyY = false) const;
  CNavArea *GetNearestNavAreaInZone(const Vector *pos, unsigned int zone, bool anyY = false) const;

  void GetListNavAreas(const BBox3 &box, Tab<CNavArea *> &list) const;

  Place GetPlace(const Vector *pos) const;          ///< return Place at given coordinate
  const char *PlaceToName(Place place) const;       ///< given a place, return its name
  Place NameToPlace(const char *name) const;        ///< given a place name, return a place ID or zero if no place is defined
  Place PartialNameToPlace(const char *name) const; ///< given the first part of a place name, return a place ID or zero if no place is
                                                    ///< defined, or the partial match is ambiguous
  void PrintAllPlaces(void) const;                  ///< output a list of names to the console

  bool GetGroundHeight(const Vector *pos, float *height, Vector *normal = NULL) const; ///< get the Z coordinate of the topmost ground
                                                                                       ///< level below the given point
  bool GetSimpleGroundHeight(const Vector *pos, float *height, Vector *normal = NULL) const; ///< get the Z coordinate of the ground
                                                                                             ///< level directly below the given point


  /// increase "danger" weights in the given nav area and nearby ones
  void IncreaseDangerNearby(int teamID, float amount, CNavArea *area, const Vector *pos, float maxRadius);
  void DrawDanger(void) const; ///< draw the current danger levels


  //-------------------------------------------------------------------------------------
  // Auto-generation
  //
  void BeginGeneration(real generation_step_size, bool reset_hint_points = true); ///< initiate the generation process
  bool IsGenerating(void) const { return m_isGenerating; } ///< return true while a Navigation Mesh is being generated
  const char *GetPlayerSpawnName(void) const;              ///< return name of player spawn entity
  void SetPlayerSpawnName(const char *name);               ///< define the name of player spawn entities

  //-------------------------------------------------------------------------------------
  // Edit mode
  //
  unsigned int GetNavPlace(void) const { return m_navPlace; }
  void SetNavPlace(unsigned int place) { m_navPlace = place; }

  void Edit(NavEditCmdType cmd) { m_editCmd = cmd; } ///< edit the mesh

  void SetMarkedArea(CNavArea *area) { m_markedArea = area; }  ///< mark area for further edit operations
  CNavArea *GetMarkedArea(void) const { return m_markedArea; } ///< return area marked by user in edit mode

  float SnapToGrid(float x) const ///< snap given coordinate to generation grid boundary
  {
    int gx = (int)(x / getUsableGenerationStepSize());
    return gx * getUsableGenerationStepSize();
  }

  const Vector &GetEditCursorPosition(void) const { return m_editCursorPos; } ///< return position of edit cursor
  void StripNavigationAreas(void);
  const char *GetFilename(void) const; ///< return the filename for this map's "nav" file


  //-------------------------------------------------------------------------------------
  /**
   * Apply the functor to all navigation areas.
   * If functor returns false, stop processing and return false.
   */
  template <typename Functor>
  bool ForAllAreas(Functor &func)
  {
    FOR_EACH_LL(TheNavAreaList, it)
    {
      CNavArea *area = TheNavAreaList[it];

      if (func(area) == false)
        return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------------
  // Auto-generation
  //
  bool UpdateGeneration(IGenericProgressIndicator *progress_ind = NULL /*, float maxTime = 0.25f*/); ///< process the auto-generation
                                                                                                     ///< for 'maxTime' seconds. return
                                                                                                     ///< false if generation is
                                                                                                     ///< complete.

  // betauser (moved from private)
  void AddNavArea(CNavArea *area);    ///< add an area to the grid
  void RemoveNavArea(CNavArea *area); ///< remove an area from the grid

  real getGenerationStepSize() const { return generationStepSize; }
  real getUsableGenerationStepSize() const { return usableGenerationStepSize; }

  bool isFreeCapsuleToPosition(const Point3 &pt) const;

  const Tab<NavHintPoint *> &getHintPoints() const { return hintPoints; }
  NavHintPoint *createHintPoint(INavArea *parent_area);
  void deleteHintPoint(NavHintPoint *point);

private:
  friend class CNavArea;

  real generationStepSize;
  real usableGenerationStepSize; // for internal bug fix

  NavAreaList *m_grid;
  float m_gridCellSize; ///< the width/height of a grid cell for spatially partitioning nav areas for fast access
  int m_gridSizeX;
  int m_gridSizeZ;
  float m_minX;
  float m_minZ;
  unsigned int m_areaCount; ///< total number of nav areas

  bool m_isLoaded; ///< true if a Navigation Mesh has been loaded

  static constexpr int HASH_TABLE_SIZE = 256;
  CNavArea *m_hashTable[HASH_TABLE_SIZE];    ///< hash table to optimize lookup by ID
  int ComputeHashKey(unsigned int id) const; ///< returns a hash key for the given nav area ID

  int WorldToGridX(float wx) const;                                  ///< given X component, return grid index
  int WorldToGridZ(float wz) const;                                  ///< given Y component, return grid index
  void AllocateGrid(float minX, float maxX, float minZ, float maxZ); ///< clear and reset the grid to the given extents

  void DestroyNavigationMesh(bool reset_hint_points = true); ///< free all resources of the mesh and reset it to empty state
  void DestroyHidingSpots(void);


  //----------------------------------------------------------------------------------
  // Place directory
  //
  char **m_placeName;           ///< master directory of place names (ie: "places")
  unsigned int m_placeCount;    ///< number of "places" defined in placeName[]
  void LoadPlaceDatabase(void); ///< load the place names from a file

  //----------------------------------------------------------------------------------
  // Edit mode
  //
  NavEditCmdType m_editCmd;  ///< edit command for editing the nav mesh
  unsigned int m_navPlace;   ///< current navigation place for editing
  CNavArea *m_markedArea;    ///< currently marked area for edit operations
  void EditReset(void);      ///< REMOVE ME
  void UpdateEditMode(void); ///< draw navigation areas and edit them
  Vector m_editCursorPos;    ///< current position of the cursor


  CNavNode *m_currentNode; ///< the current node we are sampling from
  NavDirType m_generationDir;
  CNavNode *AddNode(const Vector *destPos, const Vector *destNormal, NavDirType dir, CNavNode *source); ///< add a nav node and connect
                                                                                                        ///< it, update current node


  bool SampleStep(IGenericProgressIndicator *progress_ind); ///< sample the walkable areas of the map
  void CreateNavAreasFromNodes(void);                       ///< cover all of the sampled nodes with nav areas

  bool TestArea(CNavNode *node, int width, int height); ///< check if an area of size (width, height) can fit, starting from node as
                                                        ///< upper left corner
  int BuildArea(CNavNode *node, int width, int height); ///< create a CNavArea of size (width, height) starting fom node at upper left
                                                        ///< corner

  void MarkJumpAreas(void);
  void SquareUpAreas(void);
  void MergeGeneratedAreas(void);
  void ConnectGeneratedAreas(void);

  void GenerateZoneInfo(void);

  enum GenerationStateType
  {
    SAMPLE_WALKABLE_SPACE,
    CREATE_AREAS_FROM_SAMPLES,
    FIND_HIDING_SPOTS,
    FIND_APPROACH_AREAS,
    FIND_ENCOUNTER_SPOTS,
    FIND_SNIPER_SPOTS,

    NUM_GENERATION_STATES
  } m_generationState; ///< the state of the generation process
  bool m_isGenerating; ///< true while a Navigation Mesh is being generated

  char *m_spawnName; ///< name of player spawn entity, used to initiate sampling

  enum SampleModeStep
  {
    SampleModeStep_GeneratePoints, // with erase if capsule not free
    //          SampleModeStep_EraseNearPoints,
    SampleModeStep_AddNodes,
  };
  SampleModeStep sampleModeStep;

  struct SamplePoint
  {
    Point3 pos;
    Point3 normal;
  };
  Tab<SamplePoint> points;

  int maxPointsCount; // for progress indicator

  Point2 currentPointPositionXZ; //[0] - x, [1] - z

  int findPoint(const Vector &pos, real tolerance);

  // hint points
  Tab<NavHintPoint *> hintPoints;

  int getHintPointIndex(NavHintPoint *point);
};

// the global singleton interface
extern CNavMesh *TheNavMesh;


//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::ComputeHashKey(unsigned int id) const { return id & 0xFF; }

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridX(float wx) const
{
  int x = (int)((wx - m_minX) / m_gridCellSize);

  if (x < 0)
    x = 0;
  else if (x >= m_gridSizeX)
    x = m_gridSizeX - 1;

  return x;
}

//--------------------------------------------------------------------------------------------------------------
inline int CNavMesh::WorldToGridZ(float wz) const
{
  int z = (int)((wz - m_minZ) / m_gridCellSize);

  if (z < 0)
    z = 0;
  else if (z >= m_gridSizeZ)
    z = m_gridSizeZ - 1;

  return z;
}

inline int CNavMesh::getHintPointIndex(NavHintPoint *point)
{
  for (int n = 0; n < hintPoints.size(); n++)
    if (hintPoints[n] == point)
      return n;
  return -1;
}


/*
inline Point2 CNavMesh::getPointPositionXZ(const IPoint2& point_index)
{
  Point2 pt;
  pt[0] = float(point_index[0] * ::navOptions.GenerationStepSize) + clippingBoundingBox[0][0];
  pt[1] = float(point_index[1] * ::navOptions.GenerationStepSize) + clippingBoundingBox[0][1];
  return pt;
}
*/


//--------------------------------------------------------------------------------------------------------------
//
// Function prototypes
//

extern void ApproachAreaAnalysisPrep(void);
extern void CleanupApproachAreaAnalysisPrep(void);

#endif // _NAV_MESH_H_
