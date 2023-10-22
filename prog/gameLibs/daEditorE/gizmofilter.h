#pragma once

#include <daEditorE/de_interface.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <generic/dag_tab.h>


class GizmoEventFilter : public IGenEventHandler
{
public:
  enum SelectedAxes
  {
    AXIS_X = 1,
    AXIS_Y = 2,
    AXIS_Z = 4
  };

  virtual void handleKeyPress(int dkey, int modif);
  virtual bool handleMouseMove(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(int x, int y, bool inside, int buttons, int key_modif);
  virtual void handlePaint2D();

  virtual void handleKeyRelease(int dkey, int modif)
  {
    if (DAEDITOR4.getCurEH())
      DAEDITOR4.getCurEH()->handleKeyRelease(dkey, modif);
  }
  virtual bool handleMouseRBRelease(int x, int y, bool inside, int buttons, int key_modif)
  {
    if (DAEDITOR4.getCurEH())
      return DAEDITOR4.getCurEH()->handleMouseRBRelease(x, y, inside, buttons, key_modif);
    return false;
  }
  virtual bool handleMouseCBPress(int x, int y, bool inside, int buttons, int key_modif)
  {
    if (DAEDITOR4.getCurEH())
      return DAEDITOR4.getCurEH()->handleMouseCBPress(x, y, inside, buttons, key_modif);
    return false;
  }
  virtual bool handleMouseCBRelease(int x, int y, bool inside, int buttons, int key_modif)
  {
    if (DAEDITOR4.getCurEH())
      return DAEDITOR4.getCurEH()->handleMouseCBRelease(x, y, inside, buttons, key_modif);
    return false;
  }
  virtual bool handleMouseWheel(int wheel_d, int x, int y, int key_modif)
  {
    if (DAEDITOR4.getCurEH())
      return DAEDITOR4.getCurEH()->handleMouseWheel(wheel_d, x, y, key_modif);
    return false;
  }

  void setGizmoType(IDaEditor4Engine::ModeType type) { gizmo.type = type; }
  void setGizmoClient(IGizmoClient *client) { gizmo.client = client; }
  void setGizmoExclude(EditableObject *exclude) { gizmo.exclude = exclude; }
  void zeroOverAndSelected() { gizmo.over = gizmo.selected = 0; }

  inline IDaEditor4Engine::ModeType getGizmoType() const { return gizmo.type; }
  inline IGizmoClient *getGizmoClient() const { return gizmo.client; }

  bool isStarted() { return moveStarted; }

protected:
  struct GizmoParams
  {
    IDaEditor4Engine::ModeType prevMode = IDaEditor4Engine::MODE_none;

    int over = 0;
    int selected = 0;

    IGizmoClient *client = nullptr;
    EditableObject *exclude = nullptr;

    IDaEditor4Engine::ModeType type = IDaEditor4Engine::MODE_none;
  } gizmo;

  struct VpInfo
  {
    Point2 center = ZERO<Point2>(), ax = ZERO<Point2>(), ay = ZERO<Point2>(), az = ZERO<Point2>();
  } vp, s_vp;

  Point3 scale = Point3(1, 1, 1);
  Point2 gizmoDelta = ZERO<Point2>();
  Point2 mousePos = ZERO<Point2>();
  Point3 startPos = ZERO<Point3>();
  Point2 startPos2d = ZERO<Point2>();
  float rotAngle = 0, startRotAngle = 0;
  int deltaX = 0, deltaY = 0;

  Point3 movedDelta = ZERO<Point3>();
  Point3 planeNormal = Point3(0, 1, 0);

  bool moveStarted = false;
  Point2 rotateDir = ZERO<Point2>();

  void startGizmo(int x, int y);
  void endGizmo(bool apply);

  void drawGizmo();
  void recalcViewportGizmo();

  void drawGizmoLine(const Point2 &from, const Point2 &delta, float offset = 0.1);
  void drawGizmoLineFromTo(const Point2 &from, const Point2 &to, float offset = 0.1);
  void drawGizmoArrow(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, float offset = 0.1);
  void drawGizmoArrowScale(const Point2 &from, const Point2 &delta, E3DCOLOR col_fill, float offset = 0.1);
  void drawGizmoQuad(const Point2 &from, const Point2 &delta1, const Point2 &delta2, E3DCOLOR col1, E3DCOLOR col2, E3DCOLOR col_sel,
    bool sel, float div = 3.0);
  void drawGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, float start = 0, float end = 2 * PI);
  void fillGizmoEllipse(const Point2 &center, const Point2 &a, const Point2 &b, E3DCOLOR color, int fp, float start = 0,
    float end = TWOPI);

  void drawMoveGizmo(int sel);
  void drawSurfMoveGizmo(int sel);
  void drawScaleGizmo(int sel);
  void drawRotateGizmo(int sel);

  bool checkGizmo(int x, int y);

  bool isPointInEllipse(const Point2 &center, const Point2 &a, const Point2 &b, float width, const Point2 &point, float start = 0,
    float end = TWOPI);

  void correctScaleGizmo(const Point3 &x_dir, const Point3 &y_dir, const Point3 &z_dir, Point2 &ax, Point2 &ay, Point2 &az);

private:
  void surfaceMove(int x, int y, const Point3 &pos, Point3 &move_delta);
};
