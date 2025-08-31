// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_geneditordata.h>
#include <EditorCore/ec_gridobject.h>
#include <EASTL/unique_ptr.h>

class IGizmoRenderer;
#define GIZMO_PIXEL_WIDTH _pxS(4)
#define AXIS_LEN_PIX      _pxS(100)

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

  enum class Style : int
  {
    Classic = 0,
    New = 1
  };

  /// Constructor.
  /// @param[in] ged - reference to Editor's #GeneralEditorData
  /// @param[in] grid - reference to Editor grid
  GizmoEventFilter(GeneralEditorData &ged, const GridObject &grid);

  /// End the currently onging gizmo operation the same way as if the user finished it by releasing the left mouse
  /// button or canceled it by pressing the right mouse button.
  /// @param[in] apply - whether the apply or cancel the onging operation
  void endGizmo(bool apply);

  // Handle mouse move.
  bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // Handle left mouse button press.
  bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;
  // Handle left mouse button release.
  bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  // Handle right mouse button press.
  bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override;

  /// Handle mouse right button release\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseRBRelease
  bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ged.curEH)
      return ged.curEH->handleMouseRBRelease(wnd, x, y, inside, buttons, key_modif);
    return false;
  }

  /// Handle mouse center button press\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseCBPress
  bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ged.curEH)
      return ged.curEH->handleMouseCBPress(wnd, x, y, inside, buttons, key_modif);
    return false;
  }
  /// Handle mouse center button release\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseCBRelease
  bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) override
  {
    if (ged.curEH)
      return ged.curEH->handleMouseCBRelease(wnd, x, y, inside, buttons, key_modif);
    return false;
  }
  /// Handle mouse scroll wheel\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseWheel
  bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) override
  {
    if (ged.curEH)
      return ged.curEH->handleMouseWheel(wnd, wheel_d, x, y, key_modif);
    return false;
  }
  /// Handles mouse double-click\n
  /// The function just passes event to Editor's event handler.
  /// @copydoc IGenEventHandler::handleMouseDoubleClick
  bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) override
  {
    if (ged.curEH)
      return ged.curEH->handleMouseDoubleClick(wnd, x, y, key_modif);
    return false;
  }

  /// Render Gizmo.
  /// @copydoc IGenEventHandler::handleViewportPaint
  void handleViewportPaint(IGenViewportWnd *wnd) override;

  /// Rerender Gizmo in accordance with its new view in viewport.
  /// @copydoc IGenEventHandler::handleViewChange
  void handleViewChange(IGenViewportWnd *wnd) override;

  void setGizmoStyle(Style style);
  void setGizmoType(IEditorCoreEngine::ModeType type) { gizmo.type = type; }
  void setGizmoClient(IGizmoClient *client) { gizmo.client = client; }
  void zeroOverAndSelected() { gizmo.over = gizmo.selected = 0; }

  Style getGizmoStyle() const;
  inline IEditorCoreEngine::ModeType getGizmoType() const { return gizmo.type; }
  inline IGizmoClient *getGizmoClient() const { return gizmo.client; }
  inline Point3 getMoveDelta() const { return movedDelta; }

  void correctCursorInSurfMove(const Point3 &delta);
  bool isStarted() const { return moveStarted; }

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
  Point2 mouseCurrentPos;
  Point3 startPos;
  Tab<Point2> startPos2d;
  real rotAngle, startRotAngle;
  int deltaX, deltaY;

  Point3 movedDelta;
  Point3 planeNormal;

  bool moveStarted;
  Point2 rotateDir;
  eastl::unique_ptr<IGizmoRenderer> renderer;

  void startGizmo(IGenViewportWnd *wnd, int x, int y);

  void drawGizmo(IGenViewportWnd *w);
  void recalcViewportGizmo();

  bool checkGizmo(IGenViewportWnd *wnd, int x, int y);

private:
  inline void repaint()
  {
    IEditorCoreEngine::get()->updateViewports();
    IEditorCoreEngine::get()->invalidateViewportCache();
  }

  void surfaceMove(IGenViewportWnd *wnd, int x, int y, int vp_i, const Point3 &pos, Point3 &move_delta);
};
