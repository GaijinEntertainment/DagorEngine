#include <daEditorE/daEditorE.h>
#include <daEditorE/de_interface.h>
#include <daEditorE/de_objEditor.h>
#include "gizmofilter.h"
#include <libTools/util/undo.h>
#include <libTools/util/iLogWriter.h>
#include <ecs/input/hidEventRouter.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiPointing.h>
#include <humanInput/dag_hiGlobals.h>
#include <daInput/input_api.h>
#include <gui/dag_stdGuiRenderEx.h>
#include <gui/dag_baseCursor.h>
#include <3d/dag_render.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_Point4.h>
#include <math/integer/dag_IBBox2.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <startup/dag_startupTex.h>
#include <debug/dag_debug3d.h>
#include <util/dag_console.h>
#include <perfMon/dag_statDrv.h>

extern void register_da_editor4_objed_ptr(ObjectEditor **oe_ptr);

namespace
{
static bool de4_active = false, de4_freecam_active = false;
dainput::action_set_handle_t de4_cameraActionSet;
dainput::action_set_handle_t de4_globalsActionSet;
static void reset_gui_cursor()
{
  del_it(::gui_cursor);
  GuiNameId::shutdown();
}

static struct De4ViewSetts
{
  TMatrix4 projTm, globTm;
  DagorCurView view;
  void setPos(const Point3 &p)
  {
    view.pos = p;
    view.itm.setcol(3, p);
    view.tm = inverse(view.itm);
  }
} cur_view;

static struct RectSel
{
  bool active;
  int type;
  IBBox2 sel;
} rectSelect;

static int scrW = 1, scrH = 1;
static IGenEventHandler *curEH = NULL;
static GizmoEventFilter gizmoEH;

static bool mousePosSaved = false;
static int mousePosSavedX = 0, mousePosSavedY = 0;

struct FatalHandlerCtx
{
  bool (*handler)(const char *msg, const char *call_stack, const char *file, int line);
  bool status, quiet;
};
static Tab<FatalHandlerCtx> fhCtx(inimem);
static bool fatalStatus = false, fatalQuiet = false;

static bool de4_on_active_changed_stub(bool) { return true; }
static bool (*de4_on_active_changed)(bool activate) = &de4_on_active_changed_stub;

static void de4_on_update_stub(float) {}
static void (*de4_on_update)(float) = &de4_on_update_stub;

static void de4_on_state_changed_stub() {}
static void (*de4_on_state_changed)() = &de4_on_state_changed_stub;
} // namespace

static bool de4_gen_fatal_handler(const char *msg, const char *call_stack, const char *file, int line)
{
  G_UNUSED(call_stack);
  fatalStatus = true;
  if (!fatalQuiet)
    DAEDITOR4.conError("FATAL: %s,%d:\n    %s", file, line, msg);
  return false;
}

static void save_mouse_pos()
{
  if (mousePosSaved || !global_cls_drv_pnt->getDevice(0))
    return;
  mousePosSavedX = global_cls_drv_pnt->getDevice(0)->getRawState().mouse.x;
  mousePosSavedY = global_cls_drv_pnt->getDevice(0)->getRawState().mouse.y;
  mousePosSaved = true;
}
static void restore_mouse_pos()
{
  if (!mousePosSaved)
    return;

  if (global_cls_drv_pnt->getDevice(0))
    global_cls_drv_pnt->getDevice(0)->setPosition(mousePosSavedX, mousePosSavedY);
  mousePosSaved = false;
}

#define APP_EVENT_HANDLER(command)                                                       \
  if (gizmoEH.getGizmoClient() && gizmoEH.getGizmoType() != IDaEditor4Engine::MODE_none) \
    gizmoEH.command;                                                                     \
  else if (curEH)                                                                        \
    curEH->command;

struct De4ActivationHandler : public ecs::IGenHidEventHandler
{
  virtual bool ehIsActive() const { return IDaEditor4Engine::inited(); }
  virtual bool gmehMove(const Context &, float, float) { return false; }
  virtual bool gmehButtonDown(const Context &, int) { return false; }
  virtual bool gmehButtonUp(const Context &, int) { return false; }
  virtual bool gmehWheel(const Context &, int) { return false; }
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar)
  {
    G_UNUSED(repeat);
    G_UNUSED(wchar);
    if (de4_active && de4_freecam_active && btn_idx == HumanInput::DKEY_SPACE)
      return true;
    if (de4_active && de4_freecam_active && btn_idx == HumanInput::DKEY_ESCAPE)
      return true;
    const unsigned shiftOrAltBits = HumanInput::KeyboardRawState::SHIFT_BITS | HumanInput::KeyboardRawState::ALT_BITS;
    return btn_idx == HumanInput::DKEY_F12 && (ctx.keyModif & shiftOrAltBits) == 0;
  }
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx)
  {
    if (de4_active && de4_freecam_active && (btn_idx == HumanInput::DKEY_SPACE || btn_idx == HumanInput::DKEY_ESCAPE))
    {
      de4_freecam_active = false;
      dainput::activate_action_set(de4_cameraActionSet, false);
      global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(false);
      activeChanged();
      restore_mouse_pos();
      de4_on_state_changed();
      return true;
    }
    const unsigned shiftOrAltBits = HumanInput::KeyboardRawState::SHIFT_BITS | HumanInput::KeyboardRawState::ALT_BITS;
    if (btn_idx != HumanInput::DKEY_F12 || (ctx.keyModif & shiftOrAltBits) != 0)
      return false;

    if (!de4_on_active_changed(!de4_active))
      return true;
    de4_active = !de4_active;
    if (de4_active)
    {
      de4_cameraActionSet = dainput::get_action_set_handle("Camera");
      de4_globalsActionSet = dainput::get_action_set_handle("Globals"); // screenshot/voicechat/etc.
      savedActionSets.reserve(dainput::get_action_set_stack_depth());
      for (int i = 0; i < dainput::get_action_set_stack_depth(); i++)
        if (dainput::get_action_set_stack_item(i) != de4_cameraActionSet &&
            dainput::get_action_set_stack_item(i) != de4_globalsActionSet)
          savedActionSets.push_back(dainput::get_action_set_stack_item(i));
      dainput::reset_action_set_stack();
      dainput::activate_action_set(de4_globalsActionSet, true);

      de4_freecam_active = false;
      de4_on_state_changed();
    }
    else
    {
      dainput::reset_action_set_stack();
      dainput::activate_action_set(de4_globalsActionSet, true);
      for (int i = savedActionSets.size() - 1; i >= 0; i--)
        dainput::activate_action_set(savedActionSets[i], true);
      savedActionSets.clear();
    }

    global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(!de4_active);
    activeChanged();
    return true;
  }
  void activeChanged();

  Tab<dainput::action_set_handle_t> savedActionSets;
};

struct De4InpHandler : public ecs::IGenHidEventHandler
{
  De4InpHandler()
  {
    moveSensXY = 0.1;
    rotSensXY = 0.001;
    moveSensZ = 0.01;
    fineMoveScale = 0.1;
    turboMoveScale = 10.0;
    isMoveRotateAllowed = false;
    isXLocked = false;
    isYLocked = false;
  }
  ~De4InpHandler() { reset_gui_cursor(); }
  void loadInputSettings(const DataBlock &blk)
  {
    const DataBlock &b = *blk.getBlockByNameEx("maxCamera");
    moveSensXY = b.getReal("moveSensXY", 0.1);
    rotSensXY = b.getReal("rotSensXY", 0.001);
    moveSensZ = b.getReal("moveSensZ", 0.01);
    fineMoveScale = b.getReal("fineMoveScale", 0.1);
    turboMoveScale = b.getReal("turboMoveScale", 10.0);
  }

  virtual bool ehIsActive() const { return IDaEditor4Engine::inited() && de4_active && !de4_freecam_active; }

  virtual bool gmehMove(const Context &ctx, float dx, float dy)
  {
    // handle Max-like camera
    if (isMoveRotateAllowed && (ctx.msButtons & 0x4))
    {
      float deltaX = dx, deltaY = dy;

      if (ctx.keyModif & HumanInput::raw_state_kbd.SHIFT_BITS)
      {
        if (!isXLocked && !isYLocked)
        {
          if (fabs(deltaX) > fabs(deltaY))
            isYLocked = true;
          else
            isXLocked = true;
        }

        if (isXLocked)
          deltaX = 0;
        else if (isYLocked)
          deltaY = 0;
      }

      if (ctx.keyModif & HumanInput::raw_state_kbd.ALT_BITS)
      {
        if (ctx.keyModif & HumanInput::raw_state_kbd.CTRL_BITS)
          moveCamera(0, 0, -deltaY * moveSensXY);
        else
          rotateCamera(-deltaX * rotSensXY, -deltaY * rotSensXY, true);
      }
      else
      {
        BBox3 box;
        if (DAEDITOR4.getSelectionBoxFocused(box))
        {
          Point3 pos = box.center();
          real sx = sqrtf(DAEDITOR4.getLinearSizeSq(pos, 1, 0));
          real sy = sqrtf(DAEDITOR4.getLinearSizeSq(pos, 1, 1));

          moveCamera(-deltaX / sx, deltaY / sy, 0);
        }
        else if (ctx.keyModif & HumanInput::raw_state_kbd.CTRL_BITS)
          moveCamera(-deltaX * moveSensXY * turboMoveScale, deltaY * moveSensXY * turboMoveScale, 0);
        else
          moveCamera(-deltaX * moveSensXY, deltaY * moveSensXY, 0);
      }
      return true;
    }

    // handle rectangular selection
    if (rectSelect.active)
    {
      rectSelect.sel[1].x = ctx.msX;
      rectSelect.sel[1].y = ctx.msY;
    }
    APP_EVENT_HANDLER(handleMouseMove(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif));
    return true;
  }
  virtual bool gmehButtonDown(const Context &ctx, int btn_idx)
  {
    // handle Max-like camera
    if (btn_idx == 2 && ctx.msOverWnd)
    {
      isMoveRotateAllowed = true;
      isXLocked = false;
      isYLocked = false;
      save_mouse_pos();
      return true;
    }

    switch (btn_idx)
    {
      case 0: APP_EVENT_HANDLER(handleMouseLBPress(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
      case 1: APP_EVENT_HANDLER(handleMouseRBPress(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
      case 2: APP_EVENT_HANDLER(handleMouseCBPress(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
    }
    return true;
  }
  virtual bool gmehButtonUp(const Context &ctx, int btn_idx)
  {
    // handle Max-like camera
    if (btn_idx == 2)
    {
      restore_mouse_pos();
      isMoveRotateAllowed = false;
      return true;
    }

    switch (btn_idx)
    {
      case 0: APP_EVENT_HANDLER(handleMouseLBRelease(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
      case 1: APP_EVENT_HANDLER(handleMouseRBRelease(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
      case 2: APP_EVENT_HANDLER(handleMouseCBRelease(ctx.msX, ctx.msY, ctx.msOverWnd, ctx.msButtons, ctx.keyModif)); break;
    }
    return true;
  }
  virtual bool gmehWheel(const Context &ctx, int dz)
  {
    // handle Max-like camera
    if (ctx.msOverWnd)
    {
      float mul = 1.0;
      if (ctx.keyModif & HumanInput::raw_state_kbd.ALT_BITS)
        mul = fineMoveScale;
      else if (ctx.keyModif & HumanInput::raw_state_kbd.CTRL_BITS)
        mul = turboMoveScale;
      moveCamera(0, 0, dz * mul * moveSensZ);
      return true;
    }

    APP_EVENT_HANDLER(handleMouseWheel(dz, ctx.msX, ctx.msY, ctx.keyModif));
    return true;
  }
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar)
  {
    G_UNUSED(ctx);
    G_UNUSED(repeat);
    G_UNUSED(wchar);
    if (de4_active && !de4_freecam_active && btn_idx == HumanInput::DKEY_SPACE)
      return true;
    APP_EVENT_HANDLER(handleKeyPress(btn_idx, ctx.keyModif));
    return true;
  }
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx)
  {
    if (de4_active && !de4_freecam_active && btn_idx == HumanInput::DKEY_SPACE)
    {
      de4_freecam_active = true;
      dainput::activate_action_set(de4_cameraActionSet, true);
      global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(true);

      HumanInput::raw_state_pnt.resetDelta();
      resetState();
      if (rectSelect.active)
        DAEDITOR4.endRectangularSelection(NULL, NULL);

      save_mouse_pos();
      de4_on_state_changed();
      return true;
    }
    APP_EVENT_HANDLER(handleKeyRelease(btn_idx, ctx.keyModif));
    return true;
  }

  bool shouldHideCursor() const { return isMoveRotateAllowed; }
  void resetState()
  {
    restore_mouse_pos();
    isMoveRotateAllowed = false;
    isXLocked = false;
    isYLocked = false;
  }

protected:
  float moveSensXY, moveSensZ, rotSensXY;
  float fineMoveScale, turboMoveScale;
  bool isMoveRotateAllowed, isXLocked, isYLocked;

  void moveCamera(float deltaX, float deltaY, float deltaZ)
  {
    cur_view.setPos(cur_view.view.pos + cur_view.view.itm % Point3(deltaX, deltaY, deltaZ));
  }
  void rotateCamera(float deltaX, float deltaY, bool aroundSelection)
  {
    BBox3 box;
    bool selection = DAEDITOR4.getSelectionBoxFocused(box);
    Point3 rotationCenter = (selection && aroundSelection) ? box.center() : cur_view.view.itm.getcol(3);

    TMatrix rotSpaceTm;
    rotSpaceTm.identity();
    rotSpaceTm.setcol(3, rotationCenter);

    TMatrix camRotTm = cur_view.view.itm;
    camRotTm.setcol(3, Point3(0.f, 0.f, 0.f));

    TMatrix rot = rotyTM(deltaX) * camRotTm * rotxTM(deltaY) * inverse(camRotTm);
    TMatrix newItm = rotSpaceTm * rot * inverse(rotSpaceTm) * cur_view.view.itm;

    const real PINCH_MIN_LIMIT = 0.1;
    if (::fabs(newItm.getcol(2).y) > cos(PINCH_MIN_LIMIT) || newItm.getcol(1).y < 0)
      newItm = rotSpaceTm * rotyTM(deltaX) * inverse(rotSpaceTm) * cur_view.view.itm;

    newItm.orthonormalize();

    cur_view.view.itm = newItm;
    cur_view.view.tm = inverse(cur_view.view.itm);
    cur_view.view.pos = cur_view.view.itm.getcol(3);
  }
};
struct De4InpHandler de4_inp_handler;
struct De4ActivationHandler de4_activation_handler;

void De4ActivationHandler::activeChanged()
{
  HumanInput::raw_state_pnt.resetDelta();
  de4_inp_handler.resetState();
  if (rectSelect.active)
    DAEDITOR4.endRectangularSelection(NULL, NULL);
}

struct IDaEditor4EngineImpl : public IDaEditor4Engine, public ILogWriter, public IDaEditor4EmbeddedComponent
{
  struct Grid
  {
    bool moveSnap, scaleSnap, rotateSnap;
    Grid() { moveSnap = scaleSnap = rotateSnap = false; }
  } grid;

  ObjectEditor *objEd;
  BasisType gBasis;
  CenterType gCenter;
  bool (*traceRayCb)(const Point3 &p0, const Point3 &dir, float &dist, Point3 *out_norm);
  bool (*traceRayExCb)(const Point3 &p0, const Point3 &dir, float &dist, EditableObject *exclude_obj, Point3 *out_norm);
  void (*performPointActionCb)(bool trace, const Point3 &p0, const Point3 &dir, const Point3 &traced_pos, const char *op, int mod);

public:
  IDaEditor4EngineImpl() : IDaEditor4Engine(*create_undo_system("de4", 2 << 20, NULL))
  {
    memset(&rectSelect, 0, sizeof(rectSelect));
    curEH = NULL;
    objEd = NULL;
    traceRayCb = NULL;
    traceRayExCb = NULL;
    performPointActionCb = NULL;
    gBasis = BASIS_world;
    gCenter = CENTER_pivot;
    const DataBlock *edBlk = dgs_get_settings()->getBlockByNameEx("daEditor");
    if (edBlk == &DataBlock::emptyBlock)
      edBlk = dgs_get_settings()->getBlockByNameEx("daEditor4"); // for smoother transition
    de4_inp_handler.loadInputSettings(*edBlk->getBlockByNameEx("input"));
    register_da_editor4_objed_ptr(&objEd);
  }
  ~IDaEditor4EngineImpl()
  {
    register_da_editor4_objed_ptr(NULL);
    resetInstance();
    delete &undoSys;
    reset_gui_cursor();
  }

  virtual IGenEventHandler *getCurEH() { return curEH; }

  virtual bool worldToClient(const Point3 &world, Point2 &screen, float *screen_z = NULL)
  {
    Point4 sp4 = Point4::xyz1(cur_view.view.tm * world) * cur_view.projTm;

    if (sp4.w != 0.f)
      sp4 *= 1.0 / sp4.w;

    screen.x = 0.5f * scrW * (sp4.x + 1.f);
    screen.y = 0.5f * scrH * (-sp4.y + 1.f);
    if (screen_z)
      *screen_z = sp4.z;
    return sp4.x >= -sp4.w && sp4.x <= sp4.w && sp4.y >= -sp4.w && sp4.y <= sp4.w && sp4.z >= 0.f && sp4.z <= sp4.w;
  }
  virtual void clientToWorld(const Point2 &screen, Point3 &world, Point3 &world_dir)
  {
    Point2 normalizedScreen;
    normalizedScreen.x = 2.f * screen.x / scrW - 1.f;
    normalizedScreen.y = -(2.f * screen.y / scrH - 1.f);

    TMatrix &camera = cur_view.view.itm;
    world = camera.getcol(3);
    world_dir = camera.getcol(0) * normalizedScreen.x / cur_view.projTm[0][0] +
                camera.getcol(1) * normalizedScreen.y / cur_view.projTm[1][1] + camera.getcol(2);

    world_dir.normalize();
  }
  virtual void clientToScreen(int &x, int &y)
  {
    G_UNUSED(x);
    G_UNUSED(y);
  }
  virtual void getCameraTransform(TMatrix &m) const { m = cur_view.view.itm; }
  virtual float getLinearSizeSq(const Point3 &pos, float world_rad, int xy)
  {
    Point3 x = pos + cur_view.view.itm.getcol(xy) * world_rad;

    Point2 coord;
    Point2 xs;

    worldToClient(pos, coord);
    worldToClient(x, xs);

    return lengthSq(coord - xs);
  }
  virtual void startRectangularSelection(int mx, int my, int type)
  {
    if (rectSelect.active)
      endRectangularSelection(NULL, NULL);

    rectSelect.active = true;
    rectSelect.type = type;
    rectSelect.sel[0].x = mx;
    rectSelect.sel[0].y = my;
    rectSelect.sel[1].x = mx;
    rectSelect.sel[1].y = my;
  }
  virtual bool endRectangularSelection(IBBox2 *result, int *type)
  {
    if (!rectSelect.active)
      return false;

    rectSelect.active = false;
    if (result)
    {
#define SWAP(a, b) _t = (a), (a) = (b), (b) = _t
      real _t;
      if (rectSelect.sel[0].x > rectSelect.sel[1].x)
        SWAP(rectSelect.sel[0].x, rectSelect.sel[1].x);
      if (rectSelect.sel[0].y > rectSelect.sel[1].y)
        SWAP(rectSelect.sel[0].y, rectSelect.sel[1].y);
      *result = rectSelect.sel;
#undef SWAP
    }
    if (type)
      *type = rectSelect.type;

    return true;
  }
  virtual bool zoomAndCenter(const BBox3 &box)
  {
    if (box.isempty())
      return false;

    const Point3 width = box.width();
    Point3 vert[8] = {
      box[0],
      box[0] + Point3(width.x, 0, 0),
      box[0] + Point3(0, 0, width.z),
      box[0] + Point3(width.x, 0, width.z),

      box[1],
      box[1] - Point3(width.x, 0, 0),
      box[1] - Point3(0, 0, width.z),
      box[1] - Point3(width.x, 0, width.z),
    };

    const Point3 dir = cur_view.view.itm.getcol(2);
    Point3 pos = box.center();
    float step = width.length();

    const float MAX_AWAY = 1500.0f;
    if (step > MAX_AWAY)
      step = MAX_AWAY;

    pos -= step * dir;
    step *= 0.5f;

    for (int i = 0; i < 1024; ++i)
    {
      cur_view.setPos(pos);

      bool fit = true;
      for (int j = 0; j < 8; ++j)
      {
        Point2 pt;
        if (!worldToClient(vert[j], pt))
        {
          fit = false;
          break;
        }
      }

      pos += (fit ? step : -step) * dir;
      step *= 0.5f;
    }

    const float AWAY_DIST = 3.0f;
    pos -= AWAY_DIST * dir;

    cur_view.setPos(pos);
    return true;
  }

  virtual bool isShiftKeyPressed() { return (HumanInput::raw_state_kbd.shifts & HumanInput::raw_state_kbd.SHIFT_BITS) != 0; }
  virtual bool isCtrlKeyPressed() { return (HumanInput::raw_state_kbd.shifts & HumanInput::raw_state_kbd.CTRL_BITS) != 0; }
  virtual bool isAltKeyPressed() { return (HumanInput::raw_state_kbd.shifts & HumanInput::raw_state_kbd.ALT_BITS) != 0; }

  virtual bool getMoveSnap() const { return grid.moveSnap; }
  virtual bool getScaleSnap() const { return grid.scaleSnap; }
  virtual bool getRotateSnap() const { return grid.rotateSnap; }
  virtual void setMoveSnap(bool snap) { grid.moveSnap = snap; }
  virtual void setScaleSnap(bool snap) { grid.scaleSnap = snap; }
  virtual void setRotateSnap(bool snap) { grid.rotateSnap = snap; }
  virtual Point3 snapToGrid(const Point3 &p) const { return p; }
  virtual Point3 snapToAngle(const Point3 &p) const { return floor(p / (DEG_TO_RAD * 15.0)) * DEG_TO_RAD * 15.0; }
  virtual Point3 snapToScale(const Point3 &p) const { return p; }

  virtual bool getSelectionBox(BBox3 &box) const { return objEd ? objEd->getSelectionBox(box) : false; }
  virtual bool getSelectionBoxFocused(BBox3 &box) const { return objEd ? objEd->getSelectionBoxFocused(box) : false; }

  virtual bool traceRay(const Point3 &src, const Point3 &dir, float &dist, Point3 *out_norm = NULL) override
  {
    return traceRayCb ? traceRayCb(src, dir, dist, out_norm) : false;
  }
  virtual bool traceRayEx(const Point3 &src, const Point3 &dir, float &dist, EditableObject *exclude_obj,
    Point3 *out_norm = NULL) override
  {
    return traceRayExCb ? traceRayExCb(src, dir, dist, exclude_obj, out_norm) : traceRay(src, dir, dist, out_norm);
  }
  virtual void performPointAction(bool trace, const Point3 &from, const Point3 &dir, const Point3 &traced_pos, const char *op) override
  {
    int mod = 0;
    if (isShiftKeyPressed())
      mod |= 1;
    if (isCtrlKeyPressed())
      mod |= 2;
    if (isAltKeyPressed())
      mod |= 4;

    if (performPointActionCb)
      performPointActionCb(trace, from, dir, traced_pos, op, mod);
  }

  //! simple console messages output
  virtual ILogWriter &getCon() { return *this; }
  virtual void conErrorV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::ERROR, fmt, arg, anum); }
  virtual void conWarningV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::WARNING, fmt, arg, anum); }
  virtual void conNoteV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::NOTE, fmt, arg, anum); }
  virtual void conRemarkV(const char *fmt, const DagorSafeArg *arg, int anum) { conMessageV(ILogWriter::REMARK, fmt, arg, anum); }
  virtual void conShow(bool) {}

#define DSA_OVERLOADS_PARAM_DECL ILogWriter::MessageType type,
#define DSA_OVERLOADS_PARAM_PASS type,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void conMessage, conMessageV);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
  void conMessageV(ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum)
  {
    logmessage_fmt(type, msg, arg, anum);
  }
  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
  {
    conMessageV(type, fmt, arg, anum);
  }
  virtual bool hasErrors() const { return false; }
  virtual void startLog() {}
  virtual void endLog() {}

  //! simple fatal handler interface
  virtual void setFatalHandler(bool quiet)
  {
    FatalHandlerCtx &ctx = fhCtx.push_back();
    ctx.handler = dgs_fatal_handler;
    ctx.status = fatalStatus;
    ctx.quiet = fatalQuiet;
    dgs_fatal_handler = de4_gen_fatal_handler;
    fatalStatus = false;
    fatalQuiet = quiet;
  }
  virtual bool getFatalStatus() { return fatalStatus; }
  virtual void resetFatalStatus() { fatalStatus = false; }
  virtual void popFatalHandler()
  {
    if (fhCtx.size() < 1)
      return;
    dgs_fatal_handler = fhCtx.back().handler;
    if (fatalStatus)
      conShow(true);
    fatalStatus = fhCtx.back().status | fatalStatus;
    fatalQuiet = fhCtx.back().quiet;
    fhCtx.pop_back();
  }

  virtual void setGizmo(IGizmoClient *gc, ModeType type, EditableObject *ex)
  {
    if (gizmoEH.isStarted() && !gc)
      gizmoEH.handleKeyPress(HumanInput::DKEY_ESCAPE, 0);

    if (gc && gizmoEH.getGizmoClient() == gc)
    {
      gizmoEH.setGizmoType(type);
      return;
    }

    if (gizmoEH.getGizmoClient())
      gizmoEH.getGizmoClient()->release();

    gizmoEH.setGizmoType(type);
    gizmoEH.setGizmoClient(gc);
    gizmoEH.setGizmoExclude(ex);
    gizmoEH.zeroOverAndSelected();
  }
  virtual void startGizmo(int x, int y, bool inside, int buttons, int key_modif)
  {
    if (gizmoEH.getGizmoClient() && gizmoEH.getGizmoType() == MODE_none)
    {
      IGenEventHandler *eh = curEH;
      curEH = NULL;
      gizmoEH.handleMouseLBPress(x, y, inside, buttons, key_modif);
      curEH = eh;
    }
  }
  virtual ModeType getGizmoModeType() { return gizmoEH.getGizmoType(); }
  virtual BasisType getGizmoBasisType() { return gBasis; }
  virtual CenterType getGizmoCenterType() { return gCenter; }
  virtual void setGizmoBasisType(BasisType bt) { gBasis = bt; }
  virtual void setGizmoCenterType(CenterType ct) { gCenter = ct; }

  // IDaEditor4EmbeddedComponent
  virtual void setStaticTraceRay(bool (*trace_ray)(const Point3 &p0, const Point3 &dir, float &dist, Point3 *out_norm))
  {
    traceRayCb = trace_ray;
  }
  virtual void setStaticTraceRayEx(
    bool (*trace_ray_ex)(const Point3 &p0, const Point3 &dir, float &dist, EditableObject *exclude_obj, Point3 *out_norm))
  {
    traceRayExCb = trace_ray_ex;
  }
  virtual void setStaticPerformPointAction(
    void (*perform_point_action)(bool trace, const Point3 &p0, const Point3 &dir, const Point3 &traced_pos, const char *op, int mod))
  {
    performPointActionCb = perform_point_action;
  }
  virtual void setObjectEditor(ObjectEditor *ed)
  {
    curEH = ed;
    objEd = ed;
  }
  virtual void setActivationHandler(bool (*on_active_changed)(bool activate))
  {
    de4_on_active_changed = on_active_changed ? on_active_changed : &de4_on_active_changed_stub;
  }
  virtual void setUpdateHandler(void (*on_update)(float)) { de4_on_update = on_update ? on_update : &de4_on_update_stub; }
  virtual void setChangedHandler(void (*on_state_changed)())
  {
    de4_on_state_changed = on_state_changed ? on_state_changed : &de4_on_state_changed_stub;
  }
  virtual bool isActive() const { return de4_active; }
  virtual bool isFreeCameraActive() const override { return de4_active && de4_freecam_active; }
  virtual const TMatrix *getCameraForcedItm() const { return (de4_active && !de4_freecam_active) ? &cur_view.view.itm : NULL; }
  virtual void act(float dt)
  {
    if (!de4_active)
      return;
    if (objEd)
      objEd->update(dt);
    de4_on_update(dt);
  }
  virtual void beforeRender()
  {
    d3d::gettm(TM_PROJ, &cur_view.projTm);
    cur_view.view = grs_cur_view;
    if (cur_view.view.itm.getcol(0).lengthSq() < 1e-3)
    {
      if (de4_active)
        logwarn("bad itm=%@ detected, changed to IDENT", cur_view.view.itm);
      cur_view.view.itm = cur_view.view.tm = TMatrix::IDENT;
      cur_view.view.pos.set(0, 0, 0);
    }
    if (!de4_active || !objEd)
      return;
    objEd->beforeRender();
  }
  virtual void render3d(const Frustum &frustum, const Point3 &camera_pos)
  {
    if (!de4_active || !objEd)
      return;
    TIME_D3D_PROFILE(daEditorE)
    ::begin_draw_cached_debug_lines();
    objEd->render(frustum, camera_pos);
    ::end_draw_cached_debug_lines();
  }
  virtual void renderUi()
  {
    if (!de4_active)
      return;

    StdGuiRender::ScopeStarterOptional strt;

    StdGuiRender::set_color(COLOR_YELLOW);
    StdGuiRender::render_frame(0, 0, scrW, scrH, 3);

    APP_EVENT_HANDLER(handlePaint2D())

    if (rectSelect.active && rectSelect.sel[0] != rectSelect.sel[1])
    {
      StdGuiRender::set_color(COLOR_LTRED);
      StdGuiRender::render_frame(rectSelect.sel[0].x, rectSelect.sel[0].y, rectSelect.sel[1].x, rectSelect.sel[1].y, 1);

      StdGuiRender::set_color(COLOR_LTGREEN);
      StdGuiRender::render_frame(rectSelect.sel[0].x + 1, rectSelect.sel[0].y + 1, rectSelect.sel[1].x - 1, rectSelect.sel[1].y - 1,
        1);
    }

    const HumanInput::PointingRawState::Mouse &rs = HumanInput::raw_state_pnt.mouse;
    if (!de4_freecam_active && ::gui_cursor && !de4_inp_handler.shouldHideCursor() &&
        global_cls_drv_pnt->getDevice(0)->isPointerOverWindow() && ::gui_cursor->getVisible())
      ::gui_cursor->render(Point2(rs.x, rs.y), NULL);
  }
};
IDaEditor4Engine *IDaEditor4Engine::__daeditor4_global_instance = NULL;


IDaEditor4EmbeddedComponent *create_da_editor4(const char *mouse_cursor_texname)
{
  auto mouse = global_cls_drv_pnt ? global_cls_drv_pnt->getDevice(0) : nullptr;
  if (!mouse) // editor won't work without mouse
    return nullptr;

  scrW = StdGuiRender::real_screen_width();
  scrH = StdGuiRender::real_screen_height();
  memset(&cur_view, 0, sizeof(cur_view));

  register_hid_event_handler(&de4_inp_handler, 20);
  register_hid_event_handler(&de4_activation_handler, 220);
  mouse->setClipRect(0, 0, scrW, scrH);

  if (mouse_cursor_texname)
  {
    GuiNameId GUI_CURSOR_NORMAL("normal");
    ::register_tga_tex_load_factory();
    ::register_loadable_tex_create_factory();
    ::gui_cursor = ::create_gui_cursor_lite();
    ::gui_cursor->setModeCursor(GUI_CURSOR_NORMAL, 32, 32, 0, 0, mouse_cursor_texname);
    ::gui_cursor->setMode(GUI_CURSOR_NORMAL);
    ::gui_cursor->setVisible(true);
  }
  return new IDaEditor4EngineImpl;
}

static bool open_daeditor_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("daEd4", "open", 1, 2)
  {
    if (argc == 1 || de4_active != bool(atoi(argv[1])))
      de4_activation_handler.gkehButtonUp(ecs::IGenHidEventHandler::Context{}, HumanInput::DKEY_F12);
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(open_daeditor_console_handler);
