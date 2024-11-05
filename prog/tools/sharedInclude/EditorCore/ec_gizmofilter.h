// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_gridobject.h>


/// Gizmo implementation.
/// The class renders Gizmo, handles Gizmo events, calls proper
/// functions of linked Gizmo client (see #IGizmoClient).
/// @ingroup EditorCore
/// @ingroup Gizmo
class GizmoEventFilter : public IGenEventHandler
{
public:
  enum SelectedAxes
  {
    AXIS_X = 1,
    AXIS_Y = 2,
    AXIS_Z = 4
  };

  /// Constructor.
  /// @param[in] ged - reference to Editor's #GeneralEditorData
  /// @param[in] grid - reference to Editor grid
  GizmoEventFilter(GeneralEditorData &ged, const GridObject &grid);

  // Handle key press.
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);

  /// Handle key release\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleKeyRelease
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif)
  {
    if (ged.curEH)
      ged.curEH->handleKeyRelease(wnd, vk, modif);
  }

  // Handle mouse move.
  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // Handle left mouse button press.
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  // Handle left mouse button release.
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  // Handle right mouse button press.
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);

  /// Handle mouse right button release\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseRBRelease
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ged.curEH)
      return ged.curEH->handleMouseRBRelease(wnd, x, y, inside, buttons, key_modif);
    return false;
  }

  /// Handle mouse center button press\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseCBPress
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ged.curEH)
      return ged.curEH->handleMouseCBPress(wnd, x, y, inside, buttons, key_modif);
    return false;
  }
  /// Handle mouse center button release\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseCBRelease
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif)
  {
    if (ged.curEH)
      return ged.curEH->handleMouseCBRelease(wnd, x, y, inside, buttons, key_modif);
    return false;
  }
  /// Handle mouse scroll wheel\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseWheel
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif)
  {
    if (ged.curEH)
      return ged.curEH->handleMouseWheel(wnd, wheel_d, x, y, key_modif);
    return false;
  }
  /// Handles mouse double-click\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseDoubleClick
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif)
  {
    if (ged.curEH)
      return ged.curEH->handleMouseDoubleClick(wnd, x, y, key_modif);
    return false;
  }

  /// Render Gizmo.
  /// @copydoc IGenEventHandler::handleViewportPaint
  virtual void handleViewportPaint(IGenViewportWnd *wnd);

  /// Rerender Gizmo in accordance with its new view in viewport.
  /// @copydoc IGenEventHandler::handleViewChange
  virtual void handleViewChange(IGenViewportWnd *wnd);

  void setGizmoType(IEditorCoreEngine::ModeType type) { gizmo.type = type; }
  void setGizmoClient(IGizmoClient *client) { gizmo.client = client; }
  void zeroOverAndSelected() { gizmo.over = gizmo.selected = 0; }

  inline IEditorCoreEngine::ModeType getGizmoType() const { return gizmo.type; }
  inline IGizmoClient *getGizmoClient() const { return gizmo.client; }

  void correctCursorInSurfMove(const Point3 &delta);
  bool isStarted() { return moveStarted; }

protected:
  struct GizmoParams
  {
    IEditorCoreEngine::ModeType prevMode;

    int over;
    int selected;

    IGizmoClient *client;

    IEditorCoreEngine::ModeType type;
  } gizmo;

  GeneralEditorData &ged;
  const GridObject &grid;

  struct VpInfo
  {
    Point2 center, ax, ay, az;
  };
  Tab<VpInfo> vp, s_vp;

  Point3 scale;
  Point2 gizmoDelta;
  Point2 mousePos;
  Point3 startPos;
  Tab<Point2> startPos2d;
  real rotAngle, startRotAngle;
  int deltaX, deltaY;

  Point3 movedDelta;
  Point3 planeNormal;

  bool moveStarted;
  Point2 rotateDir;

  void startGizmo(IGenViewportWnd *wnd, int x, int y);
  void endGizmo(bool apply);

  void drawGizmo(IGenViewportWnd *w);
  void recalcViewportGizmo();

  void drawGizmoLine(const Point2 &from, const Point2 &delta, real offset = 0.1);
  void drawGizmoLineFromTo(const Point2 &from, const Point2 &to, real offset = 0.1);
  void drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset = 0.1);
  void drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, real offset = 0.1);
  void drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2, E3DCOLOR col_sel,
    bool sel, real div = 3.0);
  void drawGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real start = 0, real end = 2 * PI);
  void fillGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, E3DCOLOR color, int fp, real start = 0,
    real end = TWOPI);


  void drawMoveGizmo(int vp_i, int sel);
  void drawSurfMoveGizmo(int vp_i, int sel);
  void drawScaleGizmo(int vp_i, int sel);
  void drawRotateGizmo(IGenViewportWnd *w, int vp_i, int sel);

  bool checkGizmo(IGenViewportWnd *wnd, int x, int y);

  bool isPointInEllipse(const Point2 &center, const Point2 &a, const Point2 &b, real width, const Point2 &point, real start = 0,
    real end = TWOPI);

  void correctScaleGizmo(int index, const Point3 &x_dir, const Point3 &y_dir, const Point3 &z_dir, Point2 &ax, Point2 &ay, Point2 &az);

private:
  inline void repaint()
  {
    IEditorCoreEngine::get()->updateViewports();
    IEditorCoreEngine::get()->invalidateViewportCache();
  }

  void surfaceMove(IGenViewportWnd *wnd, int x, int y, int vp_i, const Point3 &pos, Point3 &move_delta);
};
