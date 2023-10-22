//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <util/dag_safeArg.h>

class IBBox2;
class UndoSystem;
class ILogWriter;
class IObjectsList;

class IGenEventHandler;
class IGizmoClient;
class EditableObject;

class IDaEditor4Engine
{
public:
  static constexpr unsigned HUID = 0x814F7714u; // IDaEditor4Engine
  static constexpr int DAEDITOR4_VERSION = 0x400;

  // generic version compatibility check
  inline bool checkVersion() const { return (daeditor4InterfaceVer == DAEDITOR4_VERSION); }

  virtual IGenEventHandler *getCurEH() = 0;

  virtual bool worldToClient(const Point3 &world, Point2 &screen, float *screen_z = NULL) = 0;
  virtual void clientToWorld(const Point2 &screen, Point3 &world, Point3 &world_dir) = 0;
  virtual void clientToScreen(int &x, int &y) = 0;
  virtual void getCameraTransform(TMatrix &m) const = 0;
  virtual float getLinearSizeSq(const Point3 &pos, float world_rad, int xy) = 0;
  virtual void startRectangularSelection(int mx, int my, int type) = 0;
  virtual bool endRectangularSelection(IBBox2 *result, int *type) = 0;
  virtual bool zoomAndCenter(const BBox3 &box) = 0;

  virtual bool isShiftKeyPressed() = 0;
  virtual bool isCtrlKeyPressed() = 0;
  virtual bool isAltKeyPressed() = 0;

  virtual bool getMoveSnap() const = 0;
  virtual bool getScaleSnap() const = 0;
  virtual bool getRotateSnap() const = 0;
  virtual void setMoveSnap(bool snap) = 0;
  virtual void setScaleSnap(bool snap) = 0;
  virtual void setRotateSnap(bool snap) = 0;
  virtual Point3 snapToGrid(const Point3 &p) const = 0;
  virtual Point3 snapToAngle(const Point3 &p) const = 0;
  virtual Point3 snapToScale(const Point3 &p) const = 0;

  virtual bool getSelectionBox(BBox3 &box) const = 0;
  virtual bool getSelectionBoxFocused(BBox3 &box) const = 0;

  virtual bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL) = 0;
  virtual bool traceRayEx(const Point3 &src, const Point3 &dir, float &dist, EditableObject *exclude_obj, Point3 *out_norm = NULL) = 0;
  virtual void performPointAction(bool trace, const Point3 &from, const Point3 &dir, const Point3 &traced_pos, const char *op) = 0;

  //! simple console messages output
  virtual ILogWriter &getCon() = 0;
  virtual void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) = 0;
  virtual void conShow(bool show_console_wnd) = 0;

  //! simple fatal handler interface
  virtual void setFatalHandler(bool quiet = false) = 0;
  virtual bool getFatalStatus() = 0;
  virtual void resetFatalStatus() = 0;
  virtual void popFatalHandler() = 0;

#define DSA_OVERLOADS_PARAM_DECL
#define DSA_OVERLOADS_PARAM_PASS
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conError, conErrorV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conWarning, conWarningV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conNote, conNoteV);
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conRemark, conRemarkV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

  // Gizmo management
  enum ModeType
  {
    MODE_none,
    MODE_move = 0x0100,
    MODE_scale = 0x0200,
    MODE_rotate = 0x0400,
    MODE_movesurf = 0x0800
  };
  enum BasisType
  {
    BASIS_none,
    BASIS_world = 0x0001,
    BASIS_local = 0x0002
  };
  enum CenterType
  {
    CENTER_none,
    CENTER_pivot = 0x010000,
    CENTER_sel = 0x020000,
    CENTER_coord = 0x040000
  };
  enum
  {
    GIZMO_MASK_Mode = 0xFF00,
    GIZMO_MASK_Basis = 0x00FF,
    GIZMO_MASK_CENTER = 0xFF0000
  };

  virtual void setGizmo(IGizmoClient *gc, ModeType type, EditableObject *ex) = 0;
  virtual void startGizmo(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual ModeType getGizmoModeType() = 0;
  virtual BasisType getGizmoBasisType() = 0;
  virtual CenterType getGizmoCenterType() = 0;
  virtual void setGizmoBasisType(BasisType bt) = 0;
  virtual void setGizmoCenterType(CenterType ct) = 0;

  // Free camera
  virtual bool isFreeCameraActive() const = 0;

public:
  UndoSystem &undoSys;

  static inline IDaEditor4Engine &get() { return *__daeditor4_global_instance; }
  static inline void set(IDaEditor4Engine *eng) { __daeditor4_global_instance = eng; }
  static inline bool inited() { return __daeditor4_global_instance != NULL; }

protected:
  int daeditor4InterfaceVer;
  IDaEditor4Engine(UndoSystem &u) : undoSys(u), daeditor4InterfaceVer(DAEDITOR4_VERSION) { __daeditor4_global_instance = this; }
  void resetInstance() { __daeditor4_global_instance = NULL; }

private:
  static IDaEditor4Engine *__daeditor4_global_instance;
};

class IGenEventHandler
{
public:
  virtual void handleKeyPress(int dkey, int modif) = 0;
  virtual void handleKeyRelease(int dkey, int modif) = 0;

  virtual bool handleMouseMove(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual bool handleMouseLBPress(int x, int y, bool inside, int buttons, int key_modif) = 0;

  virtual bool handleMouseLBRelease(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual bool handleMouseRBPress(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual bool handleMouseRBRelease(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual bool handleMouseCBPress(int x, int y, bool inside, int buttons, int key_modif) = 0;
  virtual bool handleMouseCBRelease(int x, int y, bool inside, int buttons, int key_modif) = 0;

  virtual bool handleMouseWheel(int wheel_d, int x, int y, int key_modif) = 0;

  virtual void handlePaint2D() {}
};

class IGizmoClient
{
public:
  virtual Point3 getPt() = 0;
  virtual bool getRot(Point3 &p) = 0;
  virtual bool getScl(Point3 &p) = 0;
  virtual bool getAxes(Point3 &ax, Point3 &ay, Point3 &az) = 0;

  virtual void changed(const Point3 &delta) = 0;
  virtual void gizmoStarted() = 0;
  virtual void gizmoEnded(bool apply) = 0;
  virtual void release() = 0;

  virtual bool canStartChangeAt(int x, int y, int gizmo_sel) = 0;
  virtual int getAvailableTypes() { return 0; }
};

#define DAEDITOR4 IDaEditor4Engine::get()
