#include "bhvBrowser.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_wndProcComponent.h>

#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
#include <webbrowser/webbrowser.h>
#include <webBrowserHelper/webBrowser.h>

#if _TARGET_PC_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace darg
{

BhvBrowser bhv_browser;

static const char *dataSlotName = "browser:data";


struct BhvBrowserData
{
  InputDevice pressedByDevice = DEVID_NONE;
  int pressedByPointer = -1;
  int pressedButton = -1;

  bool isButtonPressed() { return pressedByDevice != DEVID_NONE; }
  bool isButtonPressed(InputDevice dev, int pointer) { return pressedByDevice == dev && pressedByPointer == pointer; }
  bool isButtonPressed(InputDevice dev, int pointer, int button)
  {
    return pressedByDevice == dev && pressedByPointer == pointer && pressedButton == button;
  }

  void press(InputDevice dev, int pointer, int button)
  {
    G_ASSERT(!isButtonPressed());
    pressedByDevice = dev;
    pressedByPointer = pointer;
    pressedButton = button;
  }

  void release() { pressedByDevice = DEVID_NONE; }
};


BhvBrowser::BhvBrowser() :
  Behavior(STAGE_BEFORE_RENDER, F_HANDLE_KEYBOARD | F_HANDLE_MOUSE | F_FOCUS_ON_CLICK | F_CAN_HANDLE_CLICKS | F_DISPLAY_IME)
{}


void BhvBrowser::onAttach(darg::Element *elem)
{
  G_UNUSED(elem);
  if (webbrowser::get_helper()->createBrowserWindow())
  {
    ::add_wnd_proc_component(this);
    bool hasDefaultUrl = false;
    Sqrat::Object url = elem->props.getObject(elem->csk->defaultUrl, &hasDefaultUrl);
    if (hasDefaultUrl && url.GetType() == OT_STRING)
      webbrowser::get_helper()->go(url.GetVar<const SQChar *>().value);
  }

  BhvBrowserData *bhvData = new BhvBrowserData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
}


void BhvBrowser::onDetach(darg::Element *elem, DetachMode)
{
  G_UNUSED(elem);
  if (webbrowser::get_helper()->browserExists())
  {
    webbrowser::get_helper()->destroyBrowserWindow();
    ::del_wnd_proc_component(this);
  }

  BhvBrowserData *bhvData = elem->props.storage.RawGetSlotValue<BhvBrowserData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}


int BhvBrowser::update(UpdateStage, Element *elem, float /*dt*/)
{
  webbrowser::get_helper()->update(elem->screenCoord.size.x, elem->screenCoord.size.y);
  return 0;
}

IWndProcComponent::RetCode BhvBrowser::process(void *hwnd, unsigned msg, uintptr_t wParam, intptr_t lParam, intptr_t &result)
{
  if (!webbrowser::get_helper()->browserExists())
    return IWndProcComponent::RetCode::PROCEED_OTHER_COMPONENTS;

  webbrowser::IBrowser *browser = webbrowser::get_helper()->getBrowser();

  G_UNUSED(hwnd);
  G_UNUSED(result);

#if _TARGET_PC_WIN
  switch (msg)
  {
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    {
      if ((msg == WM_ACTIVATEAPP && wParam == FALSE) || (msg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE))
        browser->hide();
      else
        browser->show();
    }
    break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_SYSCHAR:
    {
      webbrowser::io::Key e;
      e.isSysKey = (msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP || msg == WM_SYSCHAR);
      e.winKey = wParam;
      e.nativeKey = lParam;

      const unsigned short shifted = 0x8000;
      e.modifiers = webbrowser::io::Key::NONE;
      if (::GetKeyState(VK_SHIFT) & shifted)
        e.modifiers |= webbrowser::io::Key::SHIFT;
      if (::GetKeyState(VK_CONTROL) & shifted)
        e.modifiers |= webbrowser::io::Key::CTRL;
      if (::GetKeyState(VK_MENU) & shifted)
        e.modifiers |= webbrowser::io::Key::ALT;

      if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
        e.event = webbrowser::io::Key::PRESS;
      else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
        e.event = webbrowser::io::Key::RELEASE;
      else
        e.event = webbrowser::io::Key::CHAR;

      browser->consume(e);
    }
    break;
    default: break;
  }
#else
  G_UNUSED(msg);
  G_UNUSED(wParam);
  G_UNUSED(lParam);
#endif

  return IWndProcComponent::RetCode::PROCEED_OTHER_COMPONENTS;
}

int BhvBrowser::mouseEvent(ElementTree *, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/, int data, short mx,
  short my, int /*buttons*/, int accum_res)
{
  /*
  TODO: for touch send actual touch events instead of remapping them to mouse.
  Support for touch events needs to be added to webbrowser library first.
  */

  if (!webbrowser::get_helper()->browserExists())
    return 0;

  BhvBrowserData *bhvData = elem->props.storage.RawGetSlotValue<BhvBrowserData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  webbrowser::IBrowser *browser = webbrowser::get_helper()->getBrowser();
  Point2 pos(mx, my);
  int button_id = data;
  int pointer_id = 0;

  if (event == INP_EV_MOUSE_WHEEL)
  {
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      webbrowser::io::MouseWheel e;
      e.x = pos.x - elem->screenCoord.screenPos.x;
      e.y = pos.y - elem->screenCoord.screenPos.y;
      e.dx = 0;
      e.dy = data;
      browser->consume(e);
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_PRESS)
  {
    // TODO: handle all mouse buttons: currently onle left click
    // Right click was creating a context menu outside of the game
    // Middle click should spawn a new tab, but we dont support tabs
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      if (button_id == 0 && !bhvData->isButtonPressed())
      {
        webbrowser::io::MouseButton e;
        e.x = pos.x - elem->screenCoord.screenPos.x;
        e.y = pos.y - elem->screenCoord.screenPos.y;
        e.isUp = 0;
        e.btn = webbrowser::io::MouseButton::Id(button_id);

        browser->consume(e);

        bhvData->press(device, pointer_id, button_id);
      }
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isButtonPressed(device, pointer_id, button_id))
    {
      webbrowser::io::MouseButton e;
      e.x = pos.x - elem->screenCoord.screenPos.x;
      e.y = pos.y - elem->screenCoord.screenPos.y;
      e.isUp = 1;
      e.btn = webbrowser::io::MouseButton::Id(button_id);

      browser->consume(e);
      bhvData->release();
      return R_PROCESSED;
    }
    if (elem->hitTest(pos))
    {
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (elem->hitTest(pos) || bhvData->isButtonPressed(device, pointer_id))
    {
      webbrowser::io::MouseMove e;
      e.x = pos.x - elem->screenCoord.screenPos.x;
      e.y = pos.y - elem->screenCoord.screenPos.y;
      browser->consume(e);
      return R_PROCESSED;
    }
  }

  return 0;
}

} // namespace darg
