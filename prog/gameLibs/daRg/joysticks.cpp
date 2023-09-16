#include <daRg/dag_joystick.h>
#include "guiScene.h"

#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>


namespace darg
{


int JoystickHandler::onJoystickStateChanged(dag::ConstSpan<IGuiScene *> scenes, HumanInput::IGenJoystick *joy, int joy_ord_id)
{
  const HumanInput::JoystickRawState &rs = joy->getRawState();

  if (is_main_thread())
  {
    curJoy = joy;
    curJoyOrdId = joy_ord_id;
    int result = processPendingBtnStack(scenes);
    if (!rs.buttons.cmpEq(rs.buttonsPrev))
      result |= dispatchJoystickStateChanged(scenes, rs.buttons, rs.buttonsPrev);
    return result;
  }
  else
  {
    if (joy != curJoy || joy_ord_id != curJoyOrdId)
      return 0;
    if (rs.buttons.cmpEq(rs.buttonsPrev))
      return 0;
    if (!btnStack.size())
      btnStack.push_back() = rs.buttonsPrev;
    btnStack.push_back() = rs.buttons;
    return 0;
  }
}


int JoystickHandler::dispatchJoystickStateChanged(dag::ConstSpan<IGuiScene *> scenes, const HumanInput::ButtonBits &buttons,
  const HumanInput::ButtonBits &buttonsPrev)
{
  HumanInput::ButtonBits btnXor;
  buttons.bitXor(btnXor, buttonsPrev);

  static constexpr darg::InputEvent map_flag_to_input_ev[2] = {INP_EV_RELEASE, INP_EV_PRESS};

  int result = 0;
  for (int i = 0, inc = 0; i < btnXor.FULL_SZ; i += inc)
  {
    if (btnXor.getIter(i, inc))
    {
      for (IGuiScene *scn : scenes)
      {
        G_ASSERT(scn);
        result |= scn->onJoystickBtnEvent(curJoy, map_flag_to_input_ev[buttons.get(i) ? 1 : 0], i, curJoyOrdId, buttons, result);
      }
    }
  }

  return result;
}


int JoystickHandler::processPendingBtnStack(dag::ConstSpan<IGuiScene *> scenes)
{
  if (EASTL_LIKELY(global_cls_drv_update_cs.tryLock()))
    ;
  else if (!lastProcessSkipped) // lock failed, wait a bit in hope that it will be unlocked soon
  {
    int spins = 1024;
    for (; spins > 0 && !global_cls_drv_update_cs.tryLock(); --spins)
      cpu_yield();
    if (!spins) // wait timeouted, was dainput::poll scheduled out?
    {
      lastProcessSkipped = true;
      return 0;
    }
  }
  else
    global_cls_drv_update_cs.lock();

  lastProcessSkipped = false;
  int result = 0;
  for (int i = 1; i < btnStack.size(); i++)
    result |= dispatchJoystickStateChanged(scenes, btnStack[i], btnStack[i - 1]);
  btnStack.clear();
  global_cls_drv_update_cs.unlock();
  return result;
}

} // namespace darg
