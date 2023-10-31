// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef _DE2_PLUGIN_ROADS_ROADBUILDERIFACE_H_
#define _DE2_PLUGIN_ROADS_ROADBUILDERIFACE_H_
#pragma once


namespace roadbuildertool
{
enum ErrorValues
{
  ERROR_Undefined = 0x0001,
  ERROR_OutputDataMismatch = 0x0002,
  ERROR_BadLoftShape = 0x0004,
  ERROR_NoPointBetweenCrosses = 0x0008,
  ERROR_CannotEvaluateCrosslinks = 0x0010,
};

static const int MAX_LINES_NUMBER = 24;
enum
{
  ROADSEP_None = 0,    // no separation line (may be
  ROADSEP_Solid,       // single solid line
  ROADSEP_Dashed,      // single dashed line
  ROADSEP_SolidSolid,  // double solid line
  ROADSEP_DashedSolid, // solid(left) and dashed(right) lines
  ROADSEP_SolidDashed, // dashed(left) and solid(right) lines
};

struct PointProperties
{
  //! number of lines: from 1 to MAX_LINES_NUMBERS
  int linesNumber;
  //! turn radius on crossroads at this point
  float angleRadius;
  //! bezier spline data (all three in world space)
  Point3 pos, inHandle, outHandle;
  //! up direction for road in point (interpolated between points)
  Point3 updir;
  //! one line width (interpolated between points)
  float lineWidth;
  //! material to be used on crossroad in point
  int crossRoadMatId;
  float crossRoadVScale, crossRoadUScale;
  bool roadFlipUV;
};

enum
{
  SHAPE_BORDER = 0,
  SHAPE_SIDE,
  SHAPE_ROAD,
  SHAPE_SIDE_HAMMER,
  SHAPE_CENTER_HAMMER,
  SHAPE_CENTER_BORDER,
  SHAPE_WALL,
  SHAPE_BRIDGE
};

struct RoadShape
{
  struct ShapePoint
  {
    Point2 pos;
    bool soft;
  };

  bool isClosed;
  float uScale, vScale;

  ShapePoint *points;
  int pointsCount;
};

struct LampProperties
{
  enum
  {
    PLACE_Center,
    PLACE_Left,
    PLACE_Right,
    PLACE_BothSides
  };

  //! offset from road side (can be negative)
  float roadOffset;
  //! step between lamps (in meters); for any step lamps MUST NOT be on road
  float eachSize;
  //! lamp type id
  int typeId;
  //! specifies whether lamps rotate with spline (to always show to road)
  bool rotateToRoad;
  //! additional vertical offset, MUST BE always added to target pos
  float additionalHeightOffset;

  //! lamp placement positions type (PLACE_ enum)
  int placementType;
};

struct SideSplineProperties
{
  // [0] - at start point, [1] - on normalZone, [2] - at end point
  float scale[3]; // 1.0f ~ 100%
};

class SimpleLink
{
public:
  int pointId1, pointId2;
};

class LinkProperties : public SimpleLink
{
public:
  enum Attributes
  {
    LSIDE = 0x0001,
    LVERTICAL_WALL = 0x0002,
    LSIDE_BORDERS = 0x0004,
    CENTER_BORDER = 0x0008,
    HAMMER_SIDES = 0x0010,
    BRIDGE = 0x0020,
    HAMMER_CENTER = 0x0040,
    GENERATE_LAMPS = 0x0080,
    PAINTINGS = 0x0100,
    CUSTOM_BRIDGE = 0x0200,
    STOP_LINE = 0x0400,
    RSIDE_BORDERS = 0x0800,
    RSIDE = 0x1000,
    RVERTICAL_WALL = 0x2000,
    FLIP_ROAD_TEX = 0x4000,
    BRIDGE_SUPPORT = 0x8000,
  };

  int types[MAX_LINES_NUMBER];

  //! road segment attributes
  unsigned flags;
  //! material id for road
  int roadMatId;
  //! material id for side
  int sideMatId;
  int verticalWallsMatId;
  int centerBorderMatId;
  int hammerSidesMatId;
  int hammerCenterMatId;
  //! material id for paintings
  int paintingsMatId;

  SideSplineProperties sideSplineProperties;
  SideSplineProperties wallSplineProperties;

  //! array of lamp properties
  const LampProperties *lamps;
  //! lamp properties count
  int lampsCount;

  //! ids for 2 point connected by this link (connection is directed: 1->2)
  // int pointId1, pointId2;

  int addLoftCaps;

  //! first and last point on both sides of read, in terms of spline interpolant t (0..1)
  //! for now can be ignored (it is sent as 0.0, 1.0, 0.0, 1.0), but in future should
  //! produce ending points on splines to prevent road segment overlap
  float leftMint, leftMaxt;
  float rightMint, rightMaxt;

  // tesselation precision
  float curvatureStrength, minStep, maxStep;

  float leftWallHeight, rightWallHeight;
  float centerBorderHeight, leftBorderHeight, rightBorderHeight;
  float rightHammerYOffs, rightHammerHeight, leftHammerYOffs, leftHammerHeight;
  float centerHammerYOffs, centerHammerHeight;
  float bridgeRoadThickness, bridgeStandHeight;

  float walkablePartVScale, walkablePartUScale;
};


//! Decals (additional paintings). Decals must be projected to road, but MUST NOT go outside
struct DecalProperties
{
  //! material id
  int materialId;
  //! decal center point
  Point3 point;
  //! decal size
  float width, height;
  //! whether texcoords should be flipped
  bool uFlip, vFlip;
  //! rotate angle around world Y, in radians
  float angle;
};

struct Roads
{
  LinkProperties *links;
  int linksCount;
  const PointProperties *points;
  int pointsCount;
  const DecalProperties *decals;
  int decalsCount;

  const SimpleLink *boundLinks;
  int boundLinksCount;

  // % of side length, value from 0.0f to 0.5f
  float transitionPart;
};

struct GeometryFaceDetails
{
  enum
  {
    TYPE_UNDEFINED,
    TYPE_LOFTCAP,
    TYPE_LOFTSIDE,
    TYPE_WALKABLEPART,
    TYPE_CROSSWALKABLECENTER_SIDE,
    TYPE_CROSSWALKABLECENTER_TRIANGLE,
    TYPE_WHITELINES,
    TYPE_GROUND,
    TYPE_OTHER,
  };

  //! max like smoothing group
  unsigned smoothingGroup;
  int materialId;
  //! face type, used to classify faces
  int type;
};

struct FaceIndices
{
  int v[3]; //< vertex indices
};

struct Lamp
{
  //! position of lamp
  Point3 position;
  //! euler angles (x=pitch, y=yaw, z=row)
  Point3 pitchYawRow;
  //! lamp type
  int typeId;
};

// Geometry structure, returned by road builder tool
struct Geometry
{
  //! face_count indices in vertiecs
  FaceIndices *geomFaces;
  //! face_count indices in textureCoordinates
  FaceIndices *textureFaces;
  //! face_count indices in normals
  FaceIndices *normalIndices;
  //! face_count
  GeometryFaceDetails *details;
  int faceCount;
  Point3 *vertices;
  int vertCount;
  Point2 *textureCoordinates;
  int tcCount;
  Point3 *normals;
  int normalCount;
  Lamp *lamps;
  int lampsCount;
};

// Road builder tool interface
class RoadBuilder
{
  int version;

public:
  static const int VERSION = 0x102;

  inline RoadBuilder() : version(VERSION) {}
  inline int getVersion() const { return version; }
  inline bool checkVersion() const { return (version == VERSION); }

  //! should allocate only common memory (used for any build)
  virtual void __stdcall init() = 0;
  //! all memory allocated in 'init' memory should be cleaned here
  virtual void __stdcall release() = 0;

  //! builds roads, can allocate memory specific for current build
  virtual bool __stdcall build(const Roads &roads, Geometry &geometry) = 0;
  virtual bool __stdcall buildGround(const Roads &roads, int grassMatId, Geometry &geometry, Point2 helperPt) = 0;

  //! all memory allocated in 'build' memory should be cleaned here
  virtual void __stdcall endBuild(Geometry &geometry) = 0;

  virtual void __stdcall setRoadShape(int shapeId, RoadShape *shape) = 0;

  //! returns error messages
  virtual int __stdcall getErrorMessages() = 0;
};
} // namespace roadbuildertool

#endif
