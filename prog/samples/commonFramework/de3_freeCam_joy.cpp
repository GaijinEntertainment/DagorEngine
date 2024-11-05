// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_freeCam.h"
#include "dag_cur_view.h"
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_flyMode.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>
#include <stdlib.h>

class GamepadFreeCameraDriver : public IFreeCameraDriver
{
public:
  GamepadFreeCameraDriver()
  {
    fov = 1.0f;
    flyMode = create_flymode_bb();
    flyMode->setViewMatrix(TMatrix::IDENT);
    fov = 1;
  }
  ~GamepadFreeCameraDriver()
  {
    if (flyMode)
    {
      delete flyMode;
      flyMode = NULL;
    }
  }

  virtual void act()
  {
    if (global_cls_drv_joy)
    {
      global_cls_drv_joy->updateDevices();

      if (global_cls_drv_joy->isDeviceConfigChanged())
      {
        global_cls_drv_joy->refreshDeviceList();
        global_cls_drv_joy->enable(global_cls_drv_joy->getDeviceCount() > 0);
        global_cls_drv_joy->setDefaultJoystick(NULL);
        global_cls_drv_joy->enableAutoDefaultJoystick(true);
        global_cls_drv_joy->updateDevices();
      }

      flyMode->keys.right = HumanInput::raw_state_joy.x > 8000;
      flyMode->keys.left = HumanInput::raw_state_joy.x < -8000;
      flyMode->keys.fwd = HumanInput::raw_state_joy.y > 8000;
      flyMode->keys.back = HumanInput::raw_state_joy.y < -8000;
      flyMode->keys.turbo = (HumanInput::raw_state_joy.buttons.getWord0() & (1 << 13)) != 0 ||
                            HumanInput::raw_state_joy.slider[0] > 8000 || HumanInput::raw_state_joy.slider[1] > 8000;

      if (abs(HumanInput::raw_state_joy.rx) < 8000)
        HumanInput::raw_state_joy.rx = 0;
      if (abs(HumanInput::raw_state_joy.ry) < 8000)
        HumanInput::raw_state_joy.ry = 0;

      // check Back button
      if (HumanInput::raw_state_joy.buttons.getWord0() & (1 << 5))
        exit_button_pressed = true;
    }
    flyMode->turboSpd = flyMode->moveSpd * IFreeCameraDriver::turboScale;
    flyMode->integrate(dagor_game_act_time, HumanInput::raw_state_joy.rx / 8000.0, -HumanInput::raw_state_joy.ry / 8000.0);
  }
  virtual void setView()
  {
    TMatrix vtm;
    flyMode->getViewMatrix(vtm);
    ::grs_cur_view.tm = vtm;
    ::grs_cur_view.itm = inverse(vtm);
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

protected:
  FlyModeBlackBox *flyMode;
  float fov;
};

IFreeCameraDriver *create_gamepad_free_camera() { return new GamepadFreeCameraDriver; }
