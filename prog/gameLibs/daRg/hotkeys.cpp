#include "hotkeys.h"
#include "scriptUtil.h"
#include "behaviorHelpers.h"
#include "dargDebugUtils.h"

#include <daRg/dag_scriptHandlers.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include <util/dag_string.h>
#include <utf8/utf8.h>
#include <wctype.h>

#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiKeyboard.h>
#include <humanInput/dag_hiXInputMappings.h>
#include <humanInput/dag_hiJoystick.h>
#include <humanInput/dag_hiPointing.h>

#include <memory/dag_framemem.h>

#include "kbd_key_names.h"


namespace darg
{


bool HotkeyCombo::updateOnCombo()
{
  if (!buttons.size())
  {
    G_ASSERT(!eventName.empty());
    return false;
  }

  bool newPressed = true;
  HumanInput::IGenKeyboard *kbd = global_cls_drv_kbd ? global_cls_drv_kbd->getDevice(0) : nullptr;
  HumanInput::IGenJoystick *joy = global_cls_drv_joy ? global_cls_drv_joy->getDevice(0) : nullptr;
  HumanInput::IGenPointing *mouse = global_cls_drv_pnt ? global_cls_drv_pnt->getDevice(0) : nullptr;

  for (const HotkeyButton &btn : buttons)
  {
    switch (btn.devId)
    {
      case DEVID_KEYBOARD:
        if (!kbd || kbd->getRawState().isKeyDown(btn.btnId) == btn.ckNot)
          newPressed = false;
        break;
      case DEVID_JOYSTICK:
        if (!joy || joy->getButtonState(btn.btnId) == btn.ckNot)
          newPressed = false;
        break;
      case DEVID_MOUSE:
        if (!mouse || mouse->getRawState().mouse.isBtnDown(btn.btnId) == btn.ckNot)
          newPressed = false;
        break;
      default: newPressed = false; G_ASSERTF(0, "Unsupported device %d", btn.devId);
    }

    if (!newPressed)
      break;
  }

  if (newPressed != isPressed)
  {
    isPressed = newPressed;
    return true;
  }

  return false;
}


void HotkeyCombo::triggerOnCombo(IGuiScene *scene, Element *elem)
{
  bool canCallHandler = true;
  if (triggerFront == TF_RELEASE)
  {
    canCallHandler = elem->getStateFlags() & Element::S_HOTKEY_ACTIVE;
    if (isPressed)
      elem->setGroupStateFlags(Element::S_HOTKEY_ACTIVE);
    else
      elem->clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);
  }

  if (canCallHandler && isPressed == (triggerFront == TF_PRESS) && !handler.IsNull())
  {
    bool fromJoystick = false;
    for (const HotkeyButton &btn : buttons)
    {
      if (btn.devId == DEVID_JOYSTICK)
      {
        fromJoystick = true;
        break;
      }
    }
    call_hotkey_handler(scene, elem, handler, fromJoystick);
    elem->playSound(soundKey);
  }
}


bool HotkeyCombo::updateOnEvent(IGuiScene *scene, Element *elem, const char *event_id, int id_str_len, bool new_pressed)
{
  if (id_str_len != eventName.length() || strncmp(event_id, eventName, id_str_len) != 0)
    return false;

  if (isPressed == new_pressed)
    return false;

  isPressed = new_pressed;

  if (triggerFront == TF_RELEASE)
  {
    if (isPressed)
      elem->setGroupStateFlags(Element::S_HOTKEY_ACTIVE);
    else
      elem->clearGroupStateFlags(Element::S_HOTKEY_ACTIVE);
  }

  if (new_pressed == (triggerFront == TF_PRESS) && !handler.IsNull())
    call_hotkey_handler(scene, elem, handler, false);

  return true;
}


Hotkeys::Hotkeys() { collectButtonNames(btnNameMap); }


void Hotkeys::collectKeyboardKeyNames(FastStrMapT<EncodedKey> &name_map)
{
  for (auto &kbdKey : kbd_key_names)
    name_map.addStrId(kbdKey.name, (DEVID_KEYBOARD << DEV_ID_SHIFT) | kbdKey.id);
}


void Hotkeys::collectJoystickButtonNames(FastStrMapT<EncodedKey> &name_map)
{
  using namespace HumanInput;

  const int devN = DEVID_JOYSTICK << DEV_ID_SHIFT;

#define ADD_BTN(name, btn) name_map.addStrId(name, btn | devN);

  ADD_BTN("J:D.Up", JOY_XINPUT_REAL_BTN_D_UP);
  ADD_BTN("J:D.Down", JOY_XINPUT_REAL_BTN_D_DOWN);
  ADD_BTN("J:D.Left", JOY_XINPUT_REAL_BTN_D_LEFT);
  ADD_BTN("J:D.Right", JOY_XINPUT_REAL_BTN_D_RIGHT);
  ADD_BTN("J:A", JOY_XINPUT_REAL_BTN_A);
  ADD_BTN("J:B", JOY_XINPUT_REAL_BTN_B);

  ADD_BTN("J:Start", JOY_XINPUT_REAL_BTN_START);
  ADD_BTN("J:Back", JOY_XINPUT_REAL_BTN_BACK);

  ADD_BTN("J:L.Thumb", JOY_XINPUT_REAL_BTN_L_THUMB_CENTER);
  ADD_BTN("J:LS", JOY_XINPUT_REAL_BTN_L_THUMB_CENTER);
  ADD_BTN("J:R.Thumb", JOY_XINPUT_REAL_BTN_R_THUMB_CENTER);
  ADD_BTN("J:RS", JOY_XINPUT_REAL_BTN_R_THUMB_CENTER);
  ADD_BTN("J:LS.Tilted", JOY_XINPUT_REAL_BTN_L_THUMB);
  ADD_BTN("J:RS.Tilted", JOY_XINPUT_REAL_BTN_R_THUMB);
  ADD_BTN("J:L.Shoulder", JOY_XINPUT_REAL_BTN_L_SHOULDER);
  ADD_BTN("J:LB", JOY_XINPUT_REAL_BTN_L_SHOULDER);
  ADD_BTN("J:R.Shoulder", JOY_XINPUT_REAL_BTN_R_SHOULDER);
  ADD_BTN("J:RB", JOY_XINPUT_REAL_BTN_R_SHOULDER);

  ADD_BTN("J:X", JOY_XINPUT_REAL_BTN_X);
  ADD_BTN("J:Y", JOY_XINPUT_REAL_BTN_Y);

  ADD_BTN("J:L.Trigger", JOY_XINPUT_REAL_BTN_L_TRIGGER);
  ADD_BTN("J:LT", JOY_XINPUT_REAL_BTN_L_TRIGGER);
  ADD_BTN("J:R.Trigger", JOY_XINPUT_REAL_BTN_R_TRIGGER);
  ADD_BTN("J:RT", JOY_XINPUT_REAL_BTN_R_TRIGGER);

  ADD_BTN("J:L.Thumb.Right", JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT);
  ADD_BTN("J:LS.Right", JOY_XINPUT_REAL_BTN_L_THUMB_RIGHT);
  ADD_BTN("J:L.Thumb.Left", JOY_XINPUT_REAL_BTN_L_THUMB_LEFT);
  ADD_BTN("J:LS.Left", JOY_XINPUT_REAL_BTN_L_THUMB_LEFT);
  ADD_BTN("J:L.Thumb.Up", JOY_XINPUT_REAL_BTN_L_THUMB_UP);
  ADD_BTN("J:LS.Up", JOY_XINPUT_REAL_BTN_L_THUMB_UP);
  ADD_BTN("J:L.Thumb.Down", JOY_XINPUT_REAL_BTN_L_THUMB_DOWN);
  ADD_BTN("J:LS.Down", JOY_XINPUT_REAL_BTN_L_THUMB_DOWN);

  ADD_BTN("J:R.Thumb.Right", JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT);
  ADD_BTN("J:RS.Right", JOY_XINPUT_REAL_BTN_R_THUMB_RIGHT);
  ADD_BTN("J:R.Thumb.Left", JOY_XINPUT_REAL_BTN_R_THUMB_LEFT);
  ADD_BTN("J:RS.Left", JOY_XINPUT_REAL_BTN_R_THUMB_LEFT);
  ADD_BTN("J:R.Thumb.Up", JOY_XINPUT_REAL_BTN_R_THUMB_UP);
  ADD_BTN("J:RS.Up", JOY_XINPUT_REAL_BTN_R_THUMB_UP);
  ADD_BTN("J:R.Thumb.Down", JOY_XINPUT_REAL_BTN_R_THUMB_DOWN);
  ADD_BTN("J:RS.Down", JOY_XINPUT_REAL_BTN_R_THUMB_DOWN);

#undef ADD_BTN
}


void Hotkeys::collectMouseButtonNames(FastStrMapT<EncodedKey> &name_map)
{
  String name;
  for (unsigned btn = 0; btn < 32; ++btn)
  {
    name.printf(0, "M:%d", btn);
    name_map.addStrId(name, (DEVID_MOUSE << DEV_ID_SHIFT) | btn);
  }
}


void Hotkeys::collectButtonNames(FastStrMapT<EncodedKey> &name_map)
{
  collectKeyboardKeyNames(name_map);
  collectJoystickButtonNames(name_map);
  collectMouseButtonNames(name_map);
}


const char *Hotkeys::getButtonName(HotkeyButton btn) const
{
  G_ASSERT(btn.btnId <= 255);
  EncodedKey ek = (btn.devId << DEV_ID_SHIFT) | btn.btnId;
  return btnNameMap.getStrSlow(ek);
}


HotkeyButton Hotkeys::resolveButton(const char *name) const
{
  EncodedKey ek = btnNameMap.getStrId(name);
  if (ek < 0)
    return HotkeyButton(DEVID_NONE, 0);
  return HotkeyButton(InputDevice(ek >> DEV_ID_SHIFT), (ek & BTN_ID_MASK));
}


void Hotkeys::parseButtonsString(const char *str, Tab<Tab<HotkeyButton>> &out_buttons, Tab<String> &out_events,
  HotkeyCombo::TriggerFront &out_front) const
{
  clear_and_shrink(out_buttons);
  clear_and_shrink(out_events);
  out_front = HotkeyCombo::TF_PRESS;

  G_ASSERT_RETURN(str && utf8::is_valid(str, str + strlen(str)), );

  const char *keyStart = nullptr;
  const char *eventStart = nullptr;
  bool inverse = false;

  Tab<HotkeyButton> currentButtons;
  bool isCurSetValid = true;
  String currentEvent;
  String tmp;

  if (*str == '^')
  {
    out_front = HotkeyCombo::TF_RELEASE;
    ++str;
  }

  for (const char *p = str;;)
  {
    const char *nextp = p;
    wint_t cp = utf8::next(nextp);
    // button name end
    if (*p == '|' || *p == 0 || iswspace(cp))
    {
      if (eventStart)
      {
        if (!currentEvent.empty())
          logerr_ctx("Invalid hotkey string '%s' - event %s must be separated", str, currentEvent.c_str());
        else
          currentEvent.setStr(eventStart, p - eventStart);
        eventStart = nullptr;
      }
      else if (keyStart)
      {
        tmp.setStr(keyStart, p - keyStart);
        keyStart = nullptr;

        EncodedKey ek = btnNameMap.getStrId(tmp);
        if (ek < 0)
        {
          logerr_ctx("Can't find hotkey button '%s'", tmp.str());
          isCurSetValid = false;
          clear_and_shrink(currentButtons);
        }

        if (isCurSetValid)
        {
          HotkeyButton hkb;
          hkb.devId = InputDevice(ek >> 8);
          hkb.btnId = (ek & 0xFF);
          hkb.ckNot = inverse;
          currentButtons.push_back(hkb);
        }
      }
    }
    else
    {
      // button name text
      if (!eventStart && !keyStart)
      {
        if (*p == '@')
        {
          eventStart = p + 1;
        }
        else if (*p == '!')
        {
          keyStart = p + 1;
          inverse = true;
        }
        else
        {
          keyStart = p;
          inverse = false;
        }
      }
    }

    // combo end
    if (*p == '|' || *p == 0)
    {
      G_ASSERTF(!keyStart, "dangling keyStart: [%s] [%s]", str, p);
      G_ASSERTF(!eventStart, "dangling eventStart: [%s] [%s]", str, p);
      if (isCurSetValid && currentButtons.size())
        out_buttons.push_back(currentButtons);
      if (!currentEvent.empty())
        out_events.push_back(currentEvent);

      clear_and_shrink(currentButtons);
      clear_and_shrink(currentEvent);
      isCurSetValid = true;
    }

    if (!*p)
      break;
    else
      p = nextp;
  }
}


void Hotkeys::loadCombos(const StringKeys *csk, const Sqrat::Table &comp_desc, const Sqrat::Array &hotkeys_data,
  dag::Vector<eastl::unique_ptr<HotkeyCombo>> &out) const
{
  if (hotkeys_data.IsNull())
    return;

  HSQUIRRELVM vm = hotkeys_data.GetVM();

  int len = hotkeys_data.Length();
  out.reserve(len); // guess

  Tab<Tab<HotkeyButton>> buttonsTmp(framemem_ptr());
  Tab<String> eventsTmp(framemem_ptr());

  for (int iItem = 0; iItem < len; ++iItem)
  {
    Sqrat::Object itemDesc = hotkeys_data.RawGetSlot(SQInteger(iItem));
    Sqrat::Object key;
    Sqrat::Object description;
    Sqrat::Function handler;
    Sqrat::Object soundKey;
    bool inputPassive = false;
    bool ignoreConsumerCallback = false;


    SQObjectType itemType = itemDesc.GetType();
    if (itemType == OT_ARRAY)
    {
      Sqrat::Array arr = itemDesc;
      SQInteger arrLen = arr.Length();
      if (arrLen < 1)
      {
        logerr_ctx("Empty hotkey item array");
        continue;
      }
      key = arr.RawGetSlot(SQInteger(0));
      for (int iSubItem = 1; iSubItem < arrLen; ++iSubItem)
      {
        Sqrat::Object subObj = arr.RawGetSlot(SQInteger(iSubItem));
        if (subObj.GetType() == OT_CLOSURE)
          handler = Sqrat::Function(vm, Sqrat::Object(vm), subObj);
        else if (subObj.GetType() == OT_STRING)
          description = subObj;
        else if (subObj.GetType() == OT_TABLE && iSubItem == 1)
        {
          Sqrat::Object action = subObj.RawGetSlot(csk->action);
          if (!action.IsNull())
          {
            SQObjectType tp = action.GetType();
            if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
              handler = Sqrat::Function(vm, Sqrat::Object(vm), action);
            else
              darg_assert_trace_var(String(0, "Unexpected action type %s (%X)", sq_objtypestr(tp), tp), subObj, csk->action);
          }
          description = subObj.RawGetSlot(csk->description);
          inputPassive = subObj.RawGetSlotValue(csk->inputPassive, false);
          ignoreConsumerCallback = subObj.RawGetSlotValue(csk->ignoreConsumerCallback, false);
          soundKey = subObj.RawGetSlot(csk->sound);
        }
        else
          darg_assert_trace_var(String(0, "Unexpected type %s (%X) of hotkey list item %d, array item %d",
                                  sq_objtypestr(subObj.GetType()), subObj.GetType(), iItem, iSubItem),
            comp_desc, csk->hotkeys);
      }
    }
    else if (itemType == OT_STRING)
    {
      key = itemDesc;
    }
    else
    {
      darg_assert_trace_var(String(0, "Unexpected hotkey item type %s (%X)", sq_objtypestr(itemType), itemType), comp_desc,
        csk->hotkeys);
      continue;
    }

    if (handler.IsNull())
    {
      Sqrat::Object onClickFunc = comp_desc.RawGetSlot(csk->onClick);
      if (!onClickFunc.IsNull())
      {
        SQObjectType tp = onClickFunc.GetType();
        if (tp == OT_CLOSURE || tp == OT_NATIVECLOSURE)
        {
          handler = Sqrat::Function(vm, Sqrat::Object(vm), onClickFunc);
          if (soundKey.IsNull())
            soundKey = csk->click;
        }
        else
          darg_assert_trace_var(String(0, "Unexpected onClick type %s (%X)", sq_objtypestr(tp), tp), comp_desc, csk->onClick);
      }
    }

    if (key.GetType() == OT_STRING)
    {
      auto buttonsString = key.GetVar<const SQChar *>();
      HotkeyCombo::TriggerFront front;
      parseButtonsString(buttonsString.value, buttonsTmp, eventsTmp, front);

      for (const Tab<HotkeyButton> &comboSrc : buttonsTmp)
      {
        if (!comboSrc.size())
          continue;

        HotkeyCombo *combo = new HotkeyCombo();
        combo->buttons = comboSrc;
        combo->handler = handler;
        combo->triggerFront = front;
        combo->description = description;
        combo->soundKey = soundKey;
        combo->ignoreConsumerCallback = ignoreConsumerCallback; //-V547 False positive: equivalent to the 'var = false'

        // debug_ctx("@#@ loaded hotkey combo [%s] evt = %s:", buttonsString.value, combo->eventName.c_str());
        // for (int i=0; i<combo->buttons.size(); ++i)
        //   debug_ctx("* %d:%d", combo->buttons[i].devId, combo->buttons[i].btnId);

        out.emplace_back(combo);
      }

      for (const String &eventName : eventsTmp)
      {
        HotkeyCombo *combo = new HotkeyCombo();
        combo->eventName = eventName;
        combo->handler = handler;
        combo->triggerFront = front;
        combo->description = description;
        combo->inputPassive = inputPassive; //-V547 False positive: equivalent to the 'var = false'
        combo->soundKey = soundKey;
        combo->ignoreConsumerCallback = ignoreConsumerCallback; //-V547 False positive: equivalent to the 'var = false'

        // debug_ctx("@#@ loaded hotkey combo [%s] evt = %s:", buttonsString.value, combo->eventName.c_str());

        out.emplace_back(combo);
      }
    }
  }
}


} // namespace darg
