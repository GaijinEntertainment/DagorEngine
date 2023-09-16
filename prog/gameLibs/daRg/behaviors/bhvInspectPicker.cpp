#include "bhvInspectPicker.h"
#include "behaviorHelpers.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include "elementTree.h"
#include "inputStack.h"
#include "scriptUtil.h"

#include <humanInput/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>


namespace darg
{


BhvInspectPicker bhv_inspect_picker;


BhvInspectPicker::BhvInspectPicker() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH) {}


static bool elem_is_picker_child(Element *elem, Element *picker)
{
  for (; elem; elem = elem->parent)
    if (elem == picker)
      return true;

  return false;
}


static Tab<Element *> trace_elements(ElementTree *etree, Element *picker, const Point2 &p, bool include_no_rendobj)
{
  Tab<Element *> res;
  InputStack stack;
  int hier_order = 0;
  etree->root->traceHit(p, &stack, 0, hier_order);
  if (stack.stack.empty())
    return res;

  for (InputStack::Stack::iterator it = stack.stack.begin(); it != stack.stack.end(); ++it)
  {
    if (!elem_is_picker_child(it->elem, picker) && !it->elem->props.scriptDesc.GetSlotValue("skipInspection", false) &&
        (it->elem->rendObj() || include_no_rendobj))
    {
      res.push_back(it->elem);
    }
  }

  return res;
}


int BhvInspectPicker::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/,
  int touch_idx, const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}

int BhvInspectPicker::mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data,
  short mx, short my, int /*buttons*/, int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvInspectPicker::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/,
  int /*button_id*/, const Point2 &p, int /*accum_res*/)
{
  HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(0);
  bool includeNoRendObj = kbd && (kbd->getRawState().shifts & HumanInput::KeyboardRawState::CTRL_BITS);

  if (event == INP_EV_POINTER_MOVE)
  {
    Sqrat::Function onChange = elem->props.scriptDesc.GetFunction(elem->csk->onChange);

    if (!onChange.IsNull())
    {
      Tab<Element *> stack = trace_elements(etree, elem, p, includeNoRendObj);
      HSQUIRRELVM vm = onChange.GetVM();

      if (!stack.empty())
      {
        Sqrat::Array data(vm);
        for (Element *it : stack)
        {
          Sqrat::Table item = it->getElementBBox(vm);
          item.SetValue("elem", it->getRef(vm));
          data.Append(item);
        }
        onChange(data);
      }
      else
      {
        onChange(Sqrat::Object(vm));
      }
    }
  }
  else if (event == INP_EV_PRESS)
  {
    elem->setGroupStateFlags(active_state_flags_for_device(device));
  }
  else if (event == INP_EV_RELEASE)
  {
    int sfActive = active_state_flags_for_device(device);
    if (elem->getStateFlags() & sfActive)
    {
      elem->clearGroupStateFlags(sfActive);

      Sqrat::Function onClick = elem->props.scriptDesc.GetFunction(elem->csk->onClick);
      if (!onClick.IsNull())
      {
        HSQUIRRELVM vm = onClick.GetVM();
        Tab<Element *> stack = trace_elements(etree, elem, p, includeNoRendObj);
        if (!stack.empty())
        {
          Sqrat::Array data(vm);
          for (Element *it : stack)
          {
            Sqrat::Table item = it->getElementInfo(vm);
            data.Append(item);
          }
          onClick(data);
        }
        else
        {
          onClick(Sqrat::Object(vm));
        }
      }
    }
  }

  return R_PROCESSED;
}


} // namespace darg
