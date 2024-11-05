// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_guiScene.h>
#include <ecs/input/hidEventRouter.h>
#include <sqrat.h>


struct DaRgMouseKbdHandler : public ecs::IGenHidEventHandler
{
  DaRgMouseKbdHandler() : scene(NULL) {}

  virtual bool ehIsActive() const { return scene; }

  virtual bool gmehMove(const Context &ctx, float dx, float dy)
  {
    if (scene && scene->hasInteractiveElements())
    {
      scene->onMouseEvent(darg::INP_EV_MOUSE_MOVE, 0, ctx.msX, ctx.msY, ctx.msButtons);
      return true;
    }
    return false;
  }

  virtual bool gmehButtonDown(const Context &ctx, int btn_idx)
  {
    if (scene && scene->hasInteractiveElements())
    {
      scene->onMouseEvent(darg::INP_EV_MOUSE_BTN_DOWN, btn_idx, ctx.msX, ctx.msY, ctx.msButtons);
      return true;
    }
    return false;
  }

  virtual bool gmehButtonUp(const Context &ctx, int btn_idx)
  {
    if (scene && scene->hasInteractiveElements())
    {
      scene->onMouseEvent(darg::INP_EV_MOUSE_BTN_UP, btn_idx, ctx.msX, ctx.msY, ctx.msButtons);
      return true;
    }
    return false;
  }

  virtual bool gmehWheel(const Context &ctx, int dz)
  {
    if (scene && scene->hasInteractiveElements())
    {
      scene->onMouseEvent(darg::INP_EV_MOUSE_WHEEL, dz, ctx.msX, ctx.msY, ctx.msButtons);
      return true;
    }
    return false;
  }

  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wc)
  {
    if ((btn_idx == HumanInput::DKEY_RCONTROL || btn_idx == HumanInput::DKEY_RALT) && toggleUiOnSpecialKey())
      return true;

    if (scene && scene->hasInteractiveElements())
    {
      scene->onKbdEvent(darg::INP_EV_KEY_DOWN, btn_idx, ctx.keyModif, wc);
      return true;
    }
    return false;
  }

  virtual bool gkehButtonUp(const Context &ctx, int btn_idx)
  {
    if (scene && scene->hasInteractiveElements())
    {
      scene->onKbdEvent(darg::INP_EV_KEY_UP, btn_idx, ctx.keyModif, 0);
      return true;
    }
    return false;
  }

  bool toggleUiOnSpecialKey()
  {
    if (scene)
    {
      HSQUIRRELVM v = scene->getScriptVM();
      if (v)
      {
        Sqrat::Table interop = Sqrat::RootTable(v).GetSlot("interop");
        Sqrat::Function f(interop, "toggleUi");
        if (!f.IsNull())
        {
          bool uiOn = false;
          f.Evaluate(uiOn);

          HumanInput::IGenPointing *mouse = global_cls_drv_pnt->getDevice(0);
          mouse->setRelativeMovementMode(!uiOn);

          HumanInput::raw_state_pnt.resetDelta();
          return true;
        }
      }
    }
    return false;
  }

  darg::IGuiScene *scene;
};
