// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_IObjectCreator.h>
#include <EditorCore/ec_interface.h>

class GridObject;

/// Used for creating boxes on scene with mouse.
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup ObjectCreating
class BoxCreator : public IObjectCreator
{
public:
  /// Constructor.
  BoxCreator();

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @param[in] rotate - if @b false, then box sides will be parallel to world
  ///                           coordinate axes
  /// \n                  - if @b true, then box base will be set
  ///                           as pointed by user
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}

  /// Render BoxCreator.
  /// Called from program code that created BoxCreator.
  void render() override;


  /// Index of current creator's stage
  inline unsigned getStageNo() const { return stageNo; }

protected:
  unsigned stageNo;
  Point3 point0;
  Point3 point1;
  Point3 point2;
  real height;
  Point2 clientPoint0;
  Point2 clientPoint2;
  int wrapedX;
  int wrapedY;
};


/// Used for creating spheres on scene with mouse.
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup ObjectCreating
class SphereCreator : public IObjectCreator
{
public:
  /// Sphere radius.
  real radius;

  /// Constructor.
  SphereCreator();

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}

  /// Render SphereCreator.
  /// Called from program code that created SphereCreator.
  void render() override;

protected:
  unsigned int stageNo;
  Point2 clientCenter0;
};


/// Used for selecting points on scene with mouse.
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup ObjectCreating
class PointCreator : public IObjectCreator
{
public:
  /// Constructor.
  PointCreator();

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}

  /// Render PointCreator.
  /// Called from program code that created PointCreator.
  void render() override;

protected:
  unsigned int stageNo;
  Point2 clientCenter0;
};


/// Used for creating camera targets on scene with mouse.
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup ObjectCreating
class TargetCreator : public IObjectCreator
{
public:
  /// Camera target position.
  Point3 target;

  /// Constructor.
  TargetCreator();

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}

  /// Render TargetCreator.
  /// Called from program code that created BoxCreator.
  void render() override;

protected:
  unsigned int stageNo;
  Point2 client0;
  Point3 point0;
};

/// Used for creating planes on scene with mouse.
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup ObjectCreating
class PlaneCreator : public IObjectCreator
{
public:
  Point2 halfSize;

  /// Constructor.
  PlaneCreator();

  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @param[in] rotate - if @b false, then plane sides will be parallel to world
  ///                           coordinate axes
  /// \n                  - if @b true, then plane base will be set
  ///                           as pointed by user
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}

  /// Render PlaneCreator.
  /// Called from program code that created PlaneCreator.
  void render() override;

  inline unsigned getStageNo() const { return stageNo; }

protected:
  unsigned int stageNo;
  Point3 point0;
  Point3 point1;
  Point3 point2;
  Point2 clientPoint0;
  Point2 clientPoint2;
  int wrapedX;
  int wrapedY;
};

class CircleCreator : public SphereCreator
{
public:
  void render() override;
};

class SurfaceMoveCreator : public IObjectCreator
{
public:
  /// @name Mouse events handlers.
  //@{
  /// Mouse move event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;

  /// Mouse left button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse left button release event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Mouse right button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  //@}
};


class CylinderCreator : public IObjectCreator
{
public:
  CylinderCreator();

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void render() override;

protected:
  unsigned stageNo;
  Point3 point0;
  Point3 point1;
  Point2 clientPoint;
  int wrapedX;
  int wrapedY;
};

struct PolygoneZone
{
  Tab<Point3> points;
  real topY = 0;

  Tab<Point3> getRoofPoints()
  {
    Tab<Point3> roofPoints;
    roofPoints.set_capacity(points.size());
    for (Point3 p : points)
    {
      p.y = topY;
      roofPoints.push_back(p);
    }
    return roofPoints;
  }

  real getMaxFloorY()
  {
    real max = MIN_REAL;
    for (const Point3 &p : points)
      max = eastl::max(p.y, max);
    return max;
  }

  real getHeight() { return topY - getMaxFloorY(); }

  void setHeight(real h)
  {
    h = eastl::max(h, 0);
    topY = getMaxFloorY() + h;
  }
};

class PolygoneZoneCreator : public IObjectCreator
{
public:
  PolygoneZoneCreator();

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void render() override;

  const PolygoneZone &getPolygoneZone() { return poly; }
  void setPolygoneZone(const PolygoneZone &p) { poly = p; }
  void setEditHeight(bool enable) { canEditHeight = enable; }

protected:
  enum class Stages
  {
    SetPoints,
    SetHeight,
  };

  void switchStages();

  Stages stageNo = Stages::SetPoints;
  Point3 cursorWorldPos = Point3(0, 0, 0);
  int nearestPointIndex = -1;
  int secondNearestPointIndex = -1;
  int selectedPointIndex = -1;
  bool isMovingPoint = false;
  bool canEditHeight = true;

  PolygoneZone poly;
};

class CapsuleCreator : public CylinderCreator
{
public:
  void render() override;
};

class PolyMeshCreator : public IObjectCreator
{
public:
  Tab<Point2> points;

  PolyMeshCreator();

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void render() override;

protected:
  unsigned stageNo;
  Point2 clientPoint;
  int wrapedX;
  int wrapedY;
  float height;
  float yPos;
  bool isRenderFirstPoint;
  bool isFailedMesh;

  void renderSegment(int point1_id, int point2_id);
  void convertPoints();
};


class SplineCreator : public IObjectCreator
{
public:
  SplineCreator();
  void render() override;
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true) override;
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  void getPoints(Tab<Point3> &get_points, bool &closed) const
  {
    get_points = points;
    closed = isClosed;
    if (isClosed)
      get_points.pop_back();
  }

  inline void clear()
  {
    points.clear();
    isClosed = isRenderFirstPoint = stateFinished = stateOk = false;
  }

private:
  Tab<Point3> points;
  bool isClosed;
  bool isRenderFirstPoint;
};


class StairCreator : public BoxCreator
{
public:
  void render() override;
};


class SpiralStairCreator : public CylinderCreator
{
public:
  void render() override;
};
