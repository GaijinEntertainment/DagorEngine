//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <math/random/dag_random.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>

class INavArea;
class IGenericProgressIndicator;
class IGenSave;
class IGenLoad;
class Navigator;
struct Capsule;
class BBox3;


enum NavDirType
{
  NORTH = 0,
  EAST = 1,
  SOUTH = 2,
  WEST = 3,

  NUM_DIRECTIONS
};

enum NavErrorType
{
  NAV_OK,
  NAV_CANT_ACCESS_FILE,
  NAV_INVALID_FILE,
  NAV_BAD_FILE_VERSION,
  NAV_CORRUPT_DATA,
};

struct NavigatorOptions
{
  NavigatorOptions();

  float GenerationStepSize;

  float SpawnPointsStepSize;
  float JumpCrouchHeight;

  float MaxSurfaceSlope;
  float DeathDrop;

  float HumanRadius;
  float HumanHeightCrouch;
  float HumanHeight;

  float HumanDownHeightCheck;

  bool buildHidingAndSnipingSpots;
};

extern NavigatorOptions navOptions;
extern NavigatorOptions navOptionsDefault;

class INavHidingSpot
{
public:
  enum
  {
    IN_COVER = 0x01,         ///< in a corner with good hard cover nearby
    GOOD_SNIPER_SPOT = 0x02, ///< had at least one decent sniping corridor
    IDEAL_SNIPER_SPOT = 0x04 ///< can see either very far, or a large area, or both
  };

  virtual int getFlags() const = 0;
  virtual Point3 getPos() const = 0;
};

// maybe INavHintPoint in future
class NavHintPoint
{
public:
  enum Type
  {
    Type_Invalid,

    Type_Hide,
    Type_Sniper,
    Type_SniperTarget,

    Type_Count
  };

  INavArea *getParentArea() const { return parentArea; }
  void setParentArea(INavArea *area);

  const SimpleString &getName() const { return name; }
  void setName(const char *name) { this->name = name; }

  void addLink(NavHintPoint *point);
  void removeLink(NavHintPoint *point);
  const Tab<NavHintPoint *> &getLinks() const { return links; }

  NavHintPoint::Type getType() const { return type; }
  void setType(NavHintPoint::Type type) { this->type = type; }

  const Point3 &getPosition() const { return position; }
  void setPosition(const Point3 &pos);

  real getRadius() const { return radius; }
  void setRadius(real radius) { this->radius = radius; }

  void calculateParentArea();

private:
  //  Navigator* navigator;

  INavArea *parentArea;

  SimpleString name;
  Type type;
  Point3 position;
  real radius;

  Tab<NavHintPoint *> links;

  NavHintPoint();
  virtual ~NavHintPoint();
  friend class CNavMesh;
  friend class Navigator;
};


class INavArea
{
public:
  // get data
  virtual const Point3 &getP1() const = 0;
  virtual const Point3 &getP2() const = 0;
  virtual const Point3 &getCenter() const = 0;
  virtual const real getSwY() const = 0;
  virtual const real getNeY() const = 0;
  virtual bool isJump() const = 0;
  virtual int getAdjacentCount(NavDirType dir) const = 0;
  virtual INavArea *getAdjacent(NavDirType dir, int i) const = 0;
  virtual unsigned int getIndex(void) const = 0;

  // modify
  virtual bool splitEdit(bool split_along_x, real split_edge, INavArea **out_alpha = NULL, INavArea **out_beta = NULL) = 0;
  virtual bool mergeEdit(INavArea *adj) = 0;
  virtual void connect(INavArea *adj) = 0;
  virtual void disconnect(INavArea *adj) = 0;

  virtual real getY(real x, real z) = 0;
  virtual unsigned int GetZone() const = 0;

  // hint points
  virtual const Tab<NavHintPoint *> &getHintPoints() const = 0;

  virtual int getHidingSpotCount() const = 0;
  virtual INavHidingSpot *getHidingSpot(int index) const = 0;
};

class Navigator
{
public:
  Navigator();
  ~Navigator();

  void clear();

  // current mesh generated step size
  real getGenerationStepSize() const;
  bool isValidGenerationStepSize() const;

  // Serialization
  void save(IGenSave &crd); // available for TOOLS only.
  NavErrorType load(IGenLoad &crd);

  // Areas
  unsigned int getAreaCount() const;
  INavArea *getAreaByIndex(unsigned int index) const;
  void getAreasByBox(const BBox3 &box, Tab<INavArea *> &list) const;

  // Modify
  // available for TOOLS only.
  void generate(IGenericProgressIndicator *progress_ind = NULL, bool reset_hint_points = true);
  INavArea *createArea(const Point3 &nw, const Point3 &ne, const Point3 &se, const Point3 &sw);
  void deleteArea(INavArea *area);

  // Calculations
  Point3 getAreaPortal(INavArea *areafrom, INavArea *areato, NavDirType dir) const;
  INavArea *getAreaByPos(const Point3 &pos, bool strict = false, bool *outside = NULL);
  INavArea *getClosestAreaByPosAndZone(const Point3 &pos, unsigned int zone);
  void snapByGenerationStepSize(real &v);
  //  INavArea* getAreaByPos(const Point3& pos, real beneathlimit);

  // change 'v' by 'dir' and 'amount'
  void addDirectionVector(Point3 *v, NavDirType dir, float amount);

  // available for TOOLS only.
  bool isFreeCapsuleToPosition(const Point3 &pt) const;

  // for internal bug fix check
  real getUsableGenerationStepSize() const;

  // Find
  enum PathFindMode
  {
    PATHMODE_SHORTEST,
    PATHMODE_RANDOM,
  };

  Point3 getRandomPoint(INavArea *area);

  bool findPath(INavArea *from, INavArea *to, Tab<Point3> &path, Point3 *realGoal = NULL, bool includeFirst = false,
    PathFindMode mode = PATHMODE_SHORTEST);
  bool findPath(const Point3 &from, Point3 to, Tab<Point3> &path, PathFindMode mode = PATHMODE_SHORTEST);

  // Hint points
  const Tab<NavHintPoint *> &getHintPoints() const;
  NavHintPoint *createHintPoint(INavArea *parent_area = NULL);
  void destroyHintPoint(NavHintPoint *point);
  void calculateHintPointsParentAreas();
};

inline bool Navigator::isValidGenerationStepSize() const { return getGenerationStepSize() == ::navOptions.GenerationStepSize; }

// external interface for navigation generation
extern bool (*nav_rt_trace_ray)(const Point3 &p, const Point3 &dir, float &maxt);
extern bool (*nav_rt_get_height)(const Point3 &p, float *ht, Point3 *normal);
