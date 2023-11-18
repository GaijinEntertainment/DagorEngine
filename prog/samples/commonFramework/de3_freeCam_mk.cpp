#include "de3_freeCam.h"
#include <ecs/input/hidEventRouter.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <workCycle/dag_workCycle.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiKeybData.h>
#include <startup/dag_inpDevClsDrv.h>
#include <util/dag_flyMode.h>
#include <gui/dag_visConsole.h>

static struct ConsoleMouseKbdHandler : public ecs::IGenHidEventHandler
{
  virtual bool ehIsActive() const { return true; }

  virtual bool gmehMove(const Context &ctx, float dx, float dy) { return false; }
  virtual bool gmehButtonDown(const Context &ctx, int btn_idx) { return false; }
  virtual bool gmehButtonUp(const Context &ctx, int btn_idx) { return false; }
  virtual bool gmehWheel(const Context &ctx, int dz) { return false; }
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wc)
  {
    if (console::get_visual_driver() && console::get_visual_driver()->processKey(btn_idx, wc, true))
      return true;
    return false;
  }
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx) { return false; }
} console_eh;

class MouseKbdFreeCameraDriver : public IFreeCameraDriver, public ecs::IGenHidEventHandler
{
public:
  MouseKbdFreeCameraDriver()
  {
    flyMode = create_flymode_bb();
    flyMode->setViewMatrix(TMatrix::IDENT);
    flyMode->turboSpd = flyMode->moveSpd * 20;
    fov = 1;
    HumanInput::raw_state_pnt.resetDelta();
    register_hid_event_handler(this, 0);
    register_hid_event_handler(&console_eh, 100);
    accelTime = 0;
  }
  ~MouseKbdFreeCameraDriver()
  {
    if (flyMode)
    {
      delete flyMode;
      flyMode = NULL;
    }
    unregister_hid_event_handler(this);
    unregister_hid_event_handler(&console_eh);
  }

  virtual void act()
  {
    if (console::get_visual_driver() && console::get_visual_driver()->is_visible())
    {
      flyMode->keys.left = flyMode->keys.right = flyMode->keys.fwd = flyMode->keys.back = flyMode->keys.turbo = 0;
    }
    else
    {
      HumanInput::KeyboardRawState &kbd = HumanInput::raw_state_kbd;
      flyMode->keys.left =
        int((kbd.isKeyDown(HumanInput::DKEY_LEFT) && moveWithArrows) || (kbd.isKeyDown(HumanInput::DKEY_A) && moveWithWASD));
      flyMode->keys.right =
        int((kbd.isKeyDown(HumanInput::DKEY_RIGHT) && moveWithArrows) || (kbd.isKeyDown(HumanInput::DKEY_D) && moveWithWASD));
      flyMode->keys.fwd =
        int((kbd.isKeyDown(HumanInput::DKEY_UP) && moveWithArrows) || (kbd.isKeyDown(HumanInput::DKEY_W) && moveWithWASD));
      flyMode->keys.back =
        int((kbd.isKeyDown(HumanInput::DKEY_DOWN) && moveWithArrows) || (kbd.isKeyDown(HumanInput::DKEY_S) && moveWithWASD));
      flyMode->keys.worldUp = int(kbd.isKeyDown(HumanInput::DKEY_E));
      flyMode->keys.worldDown = int(kbd.isKeyDown(HumanInput::DKEY_C));
      flyMode->keys.turbo = int(kbd.isKeyDown(HumanInput::DKEY_LSHIFT));
    }


    if (flyMode->keys.turbo && (flyMode->keys.left || flyMode->keys.right || flyMode->keys.fwd || flyMode->keys.back))
      accelTime += dagor_game_act_time;
    else
      accelTime = 0.f;

    flyMode->turboSpd = flyMode->moveSpd * IFreeCameraDriver::turboScale * (accelTime * 4.f + 1.f);
    flyMode->integrate(dagor_game_act_time, HumanInput::raw_state_pnt.mouse.deltaX, HumanInput::raw_state_pnt.mouse.deltaY);
    HumanInput::raw_state_pnt.resetDelta();
  }

  virtual void setView()
  {
    TMatrix vtm;
    flyMode->getViewMatrix(vtm);
    ::grs_cur_view.itm = inverse(vtm);
    ::grs_cur_view.itm.setcol(3, ::grs_cur_view.itm.getcol(3));
    vtm = inverse(::grs_cur_view.itm);
    ::grs_cur_view.tm = vtm;
    //::grs_cur_view.itm = inverse(vtm);
    ::grs_cur_view.pos = ::grs_cur_view.itm.getcol(3);

    d3d::settm(TM_VIEW, vtm);

    int w = 4, h = 3;
    d3d::get_render_target_size(w, h, NULL, 0);
    d3d::setpersp(Driver3dPerspective(1.3 * fov, 1.3 * fov * w / h, IFreeCameraDriver::zNear, IFreeCameraDriver::zFar, 0, 0));
    d3d::setview(0, 0, w, h, 0, 1);
  }

  virtual void setInvViewMatrix(const TMatrix &tm) { flyMode->setViewMatrix(tm); }

  virtual void getInvViewMatrix(TMatrix &tm) const { flyMode->getInvViewMatrix(tm); }

  virtual void scaleSensitivity(float move_sens_scale, float rotate_sens_scale)
  {
    flyMode->angSpdScaleH *= rotate_sens_scale;
    flyMode->angSpdScaleV *= rotate_sens_scale;
    flyMode->moveSpd *= move_sens_scale;
    flyMode->turboSpd *= move_sens_scale;
  }

  virtual bool ehIsActive() const { return IFreeCameraDriver::isActive; }

  virtual bool gmehMove(const Context &ctx, float dx, float dy) { return true; }
  virtual bool gmehButtonDown(const Context &ctx, int btn_idx) { return false; }
  virtual bool gmehButtonUp(const Context &ctx, int btn_idx) { return false; }
  virtual bool gmehWheel(const Context &ctx, int dz) { return false; }
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar)
  {
    switch (btn_idx)
    {
      case HumanInput::DKEY_F3: ::grs_draw_wire = !::grs_draw_wire; break;
      case HumanInput::DKEY_Z:
        fov *= 1.5;
        flyMode->angSpdScaleH /= 1.5;
        flyMode->angSpdScaleV /= 1.5;
        break;
      case HumanInput::DKEY_Q:
        if (fov > 1)
        {
          fov /= 1.5;
          flyMode->angSpdScaleH *= 1.5;
          flyMode->angSpdScaleV *= 1.5;
          fov = max(fov, 1.0f);
        }
        break;
    }
    return true;
  }
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx) { return true; }

protected:
  FlyModeBlackBox *flyMode;
  float fov;
};

IFreeCameraDriver *create_mskbd_free_camera()
{
  MouseKbdFreeCameraDriver *freeCamDrv = new MouseKbdFreeCameraDriver;
  return freeCamDrv;
}
