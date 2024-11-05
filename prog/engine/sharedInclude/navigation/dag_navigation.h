// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <navigation/dag_navInterface.h>
#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <debug/dag_debug.h>
#include <stdlib.h>

// Macros to simplify porting (Dront)
#define Vector2D Point2
#define Vector   Point3


#define BOOL   int
#define ConVar int
typedef unsigned char byte;
typedef unsigned int FileHandle_t;
#define MEM_ALLOC_CREDIT_CLASS()
#define Error debug
#define Msg   debug

extern float globalTime;
inline float Plat_FloatTime() { return globalTime; }

inline int RandomInt(int a, int b) { return (rand() % (b - a)) + a; }

// we can't work with entities anyway
#define CBaseEntity void


#define NAV_MAGIC_NUMBER 0xFEEDFACE ///< to help identify nav files


/**
 * A place is a named group of navigation areas
 */
typedef unsigned int Place;
#define UNDEFINED_PLACE 0 // ie: "no place"
#define ANY_PLACE       0xFFFF

enum NavAttributeType
{
  NAV_MESH_CROUCH = 0x01,  ///< must crouch to use this node/area
  NAV_MESH_JUMP = 0x02,    ///< must jump to traverse this area
  NAV_MESH_PRECISE = 0x04, ///< do not adjust for obstacles, just move along area
  NAV_MESH_NO_JUMP = 0x08, ///< inhibit discontinuity jumping
};

/**
 * Defines possible ways to move from one area to another
 */
enum NavTraverseType
{
  // NOTE: First 4 directions MUST match NavDirType
  GO_NORTH = 0,
  GO_EAST,
  GO_SOUTH,
  GO_WEST,
  GO_JUMP,

  NUM_TRAVERSE_TYPES
};

#define SOLO_ZONE    0xFFFFFFFF
#define UNKNOWN_ZONE 0x00000000

enum NavCornerType
{
  NORTH_WEST = 0,
  NORTH_EAST = 1,
  SOUTH_EAST = 2,
  SOUTH_WEST = 3,

  NUM_CORNERS
};

enum NavRelativeDirType
{
  FORWARD = 0,
  RIGHT,
  BACKWARD,
  LEFT,
  UP,
  DOWN,

  NUM_RELATIVE_DIRECTIONS
};

struct Extent
{
  Vector lo, hi;

  float SizeX(void) const { return hi.x - lo.x; }
  float SizeY(void) const { return hi.y - lo.y; }
  float SizeZ(void) const { return hi.z - lo.z; }
  float Area(void) const { return SizeX() * SizeZ(); }

  /// return true if 'pos' is inside of this extent
  bool Contains(const Vector *pos) const
  {
    return (pos->x >= lo.x && pos->x <= hi.x && pos->y >= lo.y && pos->y <= hi.y && pos->z >= lo.z && pos->z <= hi.z);
  }
};

struct Ray
{
  Vector from, to;
};


class CNavArea;
class CNavNode;


//--------------------------------------------------------------------------------------------------------------
inline NavDirType OppositeDirection(NavDirType dir)
{
  switch (dir)
  {
    case NORTH: return SOUTH;
    case SOUTH: return NORTH;
    case EAST: return WEST;
    case WEST: return EAST;
  }

  return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType DirectionLeft(NavDirType dir)
{
  switch (dir)
  {
    case NORTH: return WEST;
    case SOUTH: return EAST;
    case EAST: return NORTH;
    case WEST: return SOUTH;
  }

  return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType DirectionRight(NavDirType dir)
{
  switch (dir)
  {
    case NORTH: return EAST;
    case SOUTH: return WEST;
    case EAST: return SOUTH;
    case WEST: return NORTH;
  }

  return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void AddDirectionVector(Vector *v, NavDirType dir, float amount)
{
  switch (dir)
  {
    case NORTH: v->z -= amount; return;
    case SOUTH: v->z += amount; return;
    case EAST: v->x += amount; return;
    case WEST: v->x -= amount; return;
  }
}

//--------------------------------------------------------------------------------------------------------------
inline float DirectionToAngle(NavDirType dir)
{
  switch (dir)
  {
    case NORTH: return 270.0f;
    case SOUTH: return 90.0f;
    case EAST: return 0.0f;
    case WEST: return 180.0f;
  }

  return 0.0f;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType AngleToDirection(float angle)
{
  while (angle < 0.0f)
    angle += 360.0f;

  while (angle > 360.0f)
    angle -= 360.0f;

  if (angle < 45 || angle > 315)
    return EAST;

  if (angle >= 45 && angle < 135)
    return SOUTH;

  if (angle >= 135 && angle < 225)
    return WEST;

  return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void DirectionToVector2D(NavDirType dir, Vector2D *v)
{
  switch (dir)
  {
    case NORTH:
      v->x = 0.0f;
      v->y = -1.0f;
      break;
    case SOUTH:
      v->x = 0.0f;
      v->y = 1.0f;
      break;
    case EAST:
      v->x = 1.0f;
      v->y = 0.0f;
      break;
    case WEST:
      v->x = -1.0f;
      v->y = 0.0f;
      break;
  }
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given entity can be ignored when moving
 */
#define WALK_THRU_DOORS      0x01
#define WALK_THRU_BREAKABLES 0x02
inline bool IsEntityWalkable(CBaseEntity *entity, unsigned int flags) { return true; }

//--------------------------------------------------------------------------------------------------------------
/**
 * Check LOS, ignoring any entities that we can walk through
 */
inline bool IsWalkableTraceLineClear(Vector &from, Vector &to, unsigned int flags = 0)
{
  Point3 dir = to - from;
  float dist = length(dir);
  if (!nav_rt_trace_ray(from, dir / dist, dist))
    return true;

  return false;
}


#define FOR_EACH_LL(listName, iteratorName) \
  for (int iteratorName = listName.Head(); iteratorName != listName.InvalidIndex(); iteratorName = listName.Next(iteratorName))


template <typename T, typename I = unsigned int>
class CUtlLinkedList
{
public:
  typedef T ElemType;
  typedef I IndexType;


  CUtlLinkedList() : items(midmem) {}
  ~CUtlLinkedList() {}

  T &Element(I i) { return items[i]; }
  T const &Element(I i) const { return items[i]; }
  T &operator[](I i) { return items[i]; }
  T const &operator[](I i) const { return items[i]; }

  I Head() const
  {
    if (!items.size())
      return InvalidIndex();
    return 0;
  }
  I Tail() const
  {
    if (!items.size())
      return InvalidIndex();
    return items.size() - 1;
  }
  I Previous(I i) const
  {
    if (i == 0)
      return InvalidIndex();
    return i - 1;
  }
  I Next(I i) const { return (i + 1 < items.size()) ? i + 1 : InvalidIndex(); }

  inline static I InvalidIndex() { return (I)-1; }


  int Count() const { return items.size(); }
  void RemoveAll() { items.resize(0); }
  I AddToTail(T const &src)
  {
    items.push_back(src);
    return items.size() - 1;
  }
  bool IsValidIndex(I i) const
  {
    if (i < 0)
      return false;
    if (i >= items.size())
      return false;
    return true;
  }
  bool FindAndRemove(const T &src)
  {
    for (int i = 0; i < items.size(); i++)
    {
      if (items[i] == src)
      {
        erase_items(items, i, 1);
        return true;
      }
    }
    return false;
  }

  void setmem(IMemAlloc *alloc) { dag::set_allocator(items, alloc); }

private:
  Tab<T> items;
};
