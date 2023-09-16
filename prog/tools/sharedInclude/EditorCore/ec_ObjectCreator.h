// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#ifndef __GAIJIN_EDITORCORE_EC_OBJECT_CREATOR_H__
#define __GAIJIN_EDITORCORE_EC_OBJECT_CREATOR_H__
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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created BoxCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}

  /// Render BoxCreator.
  /// Called from program code that created BoxCreator.
  virtual void render();


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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}

  /// Render SphereCreator.
  /// Called from program code that created SphereCreator.
  virtual void render();

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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created PointCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}

  /// Render PointCreator.
  /// Called from program code that created PointCreator.
  virtual void render();

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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created TargetCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}

  /// Render TargetCreator.
  /// Called from program code that created BoxCreator.
  void render();

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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created PlaneCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}

  /// Render PlaneCreator.
  /// Called from program code that created PlaneCreator.
  void render();

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
  void render();
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
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);

  /// Mouse left button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse left button release event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Mouse right button press event handler.
  /// Called from program code that created SphereCreator.
  /// @param[in] wnd - pointer to viewport window that generated the message
  /// @param[in] x, y - <b>x,y</b> coordinates inside viewport
  /// @param[in] inside - @b true if the event occurred inside viewport
  /// @param[in] buttons - mouse buttons state flags
  /// @param[in] key_modif - <b>shift keys</b> state (see #CtlShiftKeys)
  /// @return @b true if event handling successful, @b false in other case
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  //@}
};


class CylinderCreator : public IObjectCreator
{
public:
  CylinderCreator();

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual void render();

protected:
  unsigned stageNo;
  Point3 point0;
  Point3 point1;
  Point2 clientPoint;
  int wrapedX;
  int wrapedY;
};

class CapsuleCreator : public CylinderCreator
{
public:
  void render();
};

class PolyMeshCreator : public IObjectCreator
{
public:
  Tab<Point2> points;

  PolyMeshCreator();

  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  void render();

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
  virtual void render();
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif, bool rotate = true);
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
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
  virtual void render();
};


class SpiralStairCreator : public CylinderCreator
{
public:
  virtual void render();
};

#endif
