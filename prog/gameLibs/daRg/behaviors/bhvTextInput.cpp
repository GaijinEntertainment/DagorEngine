// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTextInput.h"

#include <daRg/dag_element.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>
#include <drv/hid/dag_hiCreate.h> // for _TARGET_HAS_IME
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_unicode.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>


#if _TARGET_HAS_IME
#include <ioSys/dag_dataBlock.h>
#include "scriptUtil.h"
#endif

#include "elementTree.h"
#include "guiScene.h"
#include "textUtil.h"
#include "stdRendObj.h"
#include "dargDebugUtils.h"


using namespace HumanInput;


namespace darg
{

static int utf_to_wchar(const char *utf_str, int utf_len, Tab<wchar_t> &wtext)
{
  wtext.resize(utf_len + 2);
  int wlen = utf8_to_wcs_ex(utf_str, utf_len, wtext.data(), wtext.size() - 1);
  wtext.resize(wlen + 1);
  wtext[wlen] = 0;
  return wlen;
}


BhvTextInput bhv_text_input;


static bool should_consume_key(Element *elem, int k)
{
  if ((k >= DKEY_1 && k <= DKEY_BACK) || (k >= DKEY_Q && k <= DKEY_RBRACKET) || (k >= DKEY_A && k <= DKEY_GRAVE) ||
      (k >= DKEY_BACKSLASH && k <= DKEY_SLASH) || k == DKEY_MULTIPLY || k == DKEY_SPACE || (k >= DKEY_NUMPAD7 && k <= DKEY_OEM_102) ||
      k == DKEY_NUMPADCOMMA || k == DKEY_DIVIDE || (k >= DKEY_HOME && k <= DKEY_DELETE))
    return true;

  if (k == DKEY_RETURN || k == DKEY_NUMPADENTER)
  {
    Sqrat::Table &desc = elem->props.scriptDesc;
    return !desc.RawGetSlot(elem->csk->onReturn).IsNull();
  }
  else if (k == DKEY_ESCAPE)
  {
    Sqrat::Table &desc = elem->props.scriptDesc;
    return !desc.RawGetSlot(elem->csk->onEscape).IsNull();
  }

  return false;
}


BhvTextInput::BhvTextInput() :
  Behavior(0,
    F_HANDLE_KEYBOARD | F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_HANDLE_JOYSTICK | F_FOCUS_ON_CLICK | F_CAN_HANDLE_CLICKS | F_DISPLAY_IME)
{}


int BhvTextInput::kbdEvent(ElementTree *etree, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t wc, int accum_res)
{
  if (elem->rendObjType != rendobj_text_id)
    return 0;

  if (accum_res & R_PROCESSED)
    return 0;

  G_ASSERT((flags & F_HANDLE_KEYBOARD_GLOBAL) == (flags & F_HANDLE_KEYBOARD)); // must be only focusable

  IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(0);

  if (event == INP_EV_PRESS)
  {
    if (etree->guiScene->config.kbCursorControl && (key_idx == DKEY_UP || key_idx == DKEY_DOWN) && !etree->hasCapturedKbFocus())
    {
      etree->setKbFocus(nullptr);
      return R_PROCESSED;
    }

    bool changed = false;

    const auto &text = elem->props.text;
    Tab<wchar_t> wtext(framemem_ptr());
    int wlen = utf_to_wchar(text.c_str(), text.length(), wtext);

    int cursorPos = elem->props.storage.RawGetSlotValue(elem->csk->cursorPos, wlen);
    int prevCursorPos = cursorPos;

    cursorPos = clamp(cursorPos, 0, wlen);

    Sqrat::Object password = elem->props.getObject(elem->csk->password);
    bool isPassword = password.Cast<bool>();
    bool isCtrlPressed = kbd && (kbd->getRawState().shifts & KeyboardRawState::CTRL_BITS);

    switch (key_idx)
    {
      case DKEY_BACK:
      {
        if (cursorPos > 0)
        {
          int oldPos = cursorPos;
          cursorPos = get_next_text_pos_u(wtext.data(), wlen, cursorPos, false, isCtrlPressed, isPassword);
          erase_items(wtext, cursorPos, oldPos - cursorPos);
          changed = true;
        }
        break;
      }

      case DKEY_DELETE:
      {
        if (cursorPos < wlen)
        {
          int clearToPos = get_next_text_pos_u(wtext.data(), wlen, cursorPos, true, isCtrlPressed, isPassword);
          erase_items(wtext, cursorPos, clearToPos - cursorPos);
          changed = true;
        }
        break;
      }

      case DKEY_LEFT:
      {
        cursorPos = get_next_text_pos_u(wtext.data(), wlen, cursorPos, false, isCtrlPressed, isPassword);
        break;
      }

      case DKEY_RIGHT:
      {
        cursorPos = get_next_text_pos_u(wtext.data(), wlen, cursorPos, true, isCtrlPressed, isPassword);
        break;
      }

      case DKEY_HOME:
      {
        cursorPos = 0;
        break;
      }

      case DKEY_END:
      {
        cursorPos = wlen;
        break;
      }

      default:
        if (isCtrlPressed)
        {
          if (key_idx == DKEY_C)
          {
            if (!isPassword)
              clipboard::set_clipboard_utf8_text(text.c_str());

            break;
          }
          else if (key_idx == DKEY_V)
          {
            char buf[256];
            if (clipboard::get_clipboard_utf8_text(buf, sizeof(buf)))
            {
              for (char *s = buf; s < buf + sizeof(buf) && *s; s++)
                if (*s == '\r' || *s == '\n')
                  *s = ' ';

              Sqrat::Object cmaskobj = elem->props.getObject(elem->csk->charMask);
              const char *cmask = "";
              if (cmaskobj.GetType() == OT_STRING)
                cmask = cmaskobj.GetVar<const char *>().value;
              int cmasklen = (int)strlen(cmask);

              Tab<wchar_t> w_inp;
              char utf8_buf[4];

              String text;
              bool isAllCharsPassed = true;

              if (cmasklen > 0)
              {
                convert_utf8_to_u16_buf(w_inp, buf, -1);
                for (int i = 0; i < w_inp.size(); i++)
                {
                  if (strstr(cmask, wchar_to_utf8(w_inp[i], utf8_buf, sizeof(utf8_buf))))
                    text.append(utf8_buf);
                  else
                    isAllCharsPassed = false;
                }
              }
              else
                text = buf;

              if (!isAllCharsPassed)
              {
                Sqrat::Function onWrongInput = elem->props.scriptDesc.GetFunction(elem->csk->onWrongInput);
                if (!onWrongInput.IsNull())
                  onWrongInput();
              }

              Tab<wchar_t> wbuf(framemem_ptr());
              int wblen = utf_to_wchar(text.c_str(), text.length(), wbuf);

              insert_items(wtext, cursorPos, wblen, wbuf.data());
              cursorPos += wblen;
              changed = true;
            }
            break;
          }
        }

        if (wc)
        {
          Sqrat::Object cmaskobj = elem->props.getObject(elem->csk->charMask);
          if (cmaskobj.GetType() == OT_STRING)
          {
            const char *cmask = cmaskobj.GetVar<const char *>().value;
            Tab<wchar_t> wmask(framemem_ptr());
            int cmasklen = (int)strlen(cmask);
            utf_to_wchar(cmask, cmasklen, wmask);
            if (cmasklen > 0 && wcsrchr(wmask.data(), wc) == NULL)
            {
              Sqrat::Function onWrongInput = elem->props.scriptDesc.GetFunction(elem->csk->onWrongInput);
              if (!onWrongInput.IsNull())
                onWrongInput();
              break;
            }
          }

          insert_item_at(wtext, cursorPos, wc);
          changed = true;
          ++cursorPos;
        }
        break;
    }

    elem->props.storage.SetValue(elem->csk->cursorPos, cursorPos);

    if (changed)
    {
      int wlen = wtext.size() - 1;
      int maxChars = elem->props.getInt(elem->csk->maxChars, 0);
      if (maxChars > 0 && wlen > maxChars)
      {
        erase_items(wtext, maxChars, wlen - maxChars);
        if (cursorPos >= maxChars)
          cursorPos = maxChars - 1;
      }

      Tab<char> dest_utf8(framemem_ptr());
      dest_utf8.resize(wlen * 6 + 1);
      G_VERIFY(wcs_to_utf8(wtext.data(), dest_utf8.data(), dest_utf8.size()));
      apply_new_value(elem, dest_utf8.data());
    }

    if (cursorPos != prevCursorPos)
      scroll_to_cursor(elem);

    if (!repeat)
    {
      if (key_idx == DKEY_RETURN || key_idx == DKEY_NUMPADENTER)
      {
        Sqrat::Table &desc = elem->props.scriptDesc;
        Sqrat::Function onReturn = desc.GetFunction(elem->csk->onReturn);
        if (!onReturn.IsNull())
          onReturn();
#if _TARGET_ANDROID || _TARGET_IOS
        if (!HumanInput::isImeAvailable())
        {
          // close screen keyboard (assume single line editing)
          HumanInput::showScreenKeyboard(false);
        }
#endif
      }
      else if (key_idx == DKEY_ESCAPE)
      {
        Sqrat::Table &desc = elem->props.scriptDesc;
        Sqrat::Function onEscape = desc.GetFunction(elem->csk->onEscape);
        if (!onEscape.IsNull())
          onEscape();
      }
    }
  }

  if (should_consume_key(elem, key_idx))
    return R_PROCESSED; // Both for presses and releases
  else
    return 0;
}


void BhvTextInput::apply_new_value(Element *elem, const char *new_value)
{
  elem->props.text = new_value; //< Only for correct content size update
                                //< Maybe it makes sense to return resulting text from
                                //< script handler
  discard_text_cache(elem->robjParams);
  elem->textSizeCache = Point2(-1, -1);

  Sqrat::Function onChange = elem->props.scriptDesc.GetFunction(elem->csk->onChange);
  if (!onChange.IsNull())
    onChange(elem->props.text.c_str());

  elem->recalcContentSize(0);
}


void BhvTextInput::scroll_to_cursor(Element *elem)
{
  RobjParamsText *params = (RobjParamsText *)elem->robjParams;

  Tab<wchar_t> wtext(framemem_ptr());
  int wlen = get_displayed_text_u(elem, wtext);

  int cursorPos = elem->props.storage.RawGetSlotValue(elem->csk->cursorPos, wlen);

  cursorPos = clamp(cursorPos, 0, wlen);
  Point2 tsz = calc_text_size_u(wtext.data(), cursorPos, params->fontId, params->spacing, params->monoWidth, params->fontHt);
  float visWidth = elem->screenCoord.size.x - elem->layout.padding.left() - elem->layout.padding.right();
  float minVisX = elem->screenCoord.scrollOffs.x;
  float maxVisX = elem->screenCoord.scrollOffs.x + visWidth;

  if (tsz.x < minVisX + 10)
  {
    int posX = max(0, int(tsz.x - 0.25 * visWidth));
    int textPos = text_width_to_char_idx_u(elem, posX, wtext);
    Point2 sp = calc_text_size_u(wtext.data(), textPos, params->fontId, params->spacing, params->monoWidth, params->fontHt);
    elem->scrollTo(Point2(sp.x, 0));
  }
  else if (tsz.x > maxVisX - 10)
  {
    int posX = max(0, int(tsz.x + 0.25 * visWidth));
    int textPos = text_width_to_char_idx_u(elem, posX, wtext);
    Point2 sp = calc_text_size_u(wtext.data(), textPos, params->fontId, params->spacing, params->monoWidth, params->fontHt);
    float offset = max(0, int(sp.x + 1 - visWidth)); // +1 for caret
    elem->scrollTo(Point2(offset, 0));
  }
}

void BhvTextInput::position_cursor_on_click(Element *elem, const Point2 &click_pos)
{
  Tab<wchar_t> wtext(framemem_ptr());
  get_displayed_text_u(elem, wtext);

  Point2 localCoord = elem->screenPosToElemLocal(click_pos);
  int newPos = text_width_to_char_idx_u(elem, localCoord.x, wtext);

  elem->props.storage.SetValue(elem->csk->cursorPos, newPos);
  open_ime(elem);
}

int BhvTextInput::get_displayed_text_u(Element *elem, Tab<wchar_t> &wtext)
{
  const auto &text = elem->props.text;
  int wlen = utf_to_wchar(text.c_str(), text.length(), wtext);
  RobjParamsText *params = (RobjParamsText *)elem->robjParams;
  if (params->passChar)
    for (int i = 0; i < wlen; i++)
      wtext[i] = params->passChar;
  return wlen;
}

int BhvTextInput::text_width_to_char_idx_u(Element *elem, int width_px, Tab<wchar_t> &wtext)
{
  int res = 0;
  RobjParamsText *params = (RobjParamsText *)elem->robjParams;
  float minDist = 1e6f;
  //== binary search would be faster
  for (int pos = 0; pos < wtext.size(); pos++)
  {
    Point2 sp = calc_text_size_u(wtext.data(), pos, params->fontId, params->spacing, params->monoWidth, params->fontHt);
    float screenDist = fabsf(sp.x - width_px);
    if (screenDist >= minDist)
      break;
    res = pos;
    minDist = screenDist;
  }
  return res;
}

int BhvTextInput::mouseEvent(ElementTree *etree, Element *elem, InputDevice /*device*/, InputEvent event, int /*pointer_id*/,
  int /*data*/, short mx, short my, int /*buttons*/, int accum_res)
{
  if (event == INP_EV_MOUSE_WHEEL || event == INP_EV_POINTER_MOVE)
    return 0;

  if (elem->rendObjType != rendobj_text_id)
    return 0;

  if (event == INP_EV_RELEASE && elem->hitTest(mx, my))
    return R_PROCESSED;

  if (event == INP_EV_PRESS && !(accum_res & R_PROCESSED) && elem->hitTest(mx, my) && !etree->hasCapturedKbFocus())
  {
    etree->setKbFocus(elem);
    position_cursor_on_click(elem, Point2(mx, my));
    return R_PROCESSED;
  }
  return 0;
}


int BhvTextInput::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/,
  int /*touch_idx*/, const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  if (elem->rendObjType != rendobj_text_id)
    return 0;

  if (event == INP_EV_RELEASE && !(accum_res & R_PROCESSED) && elem->hitTest(touch.x, touch.y) && !etree->hasCapturedKbFocus())
  {
    etree->setKbFocus(elem);
    position_cursor_on_click(elem, Point2(touch.x, touch.y));
    return R_PROCESSED;
  }
  return 0;
}


int BhvTextInput::joystickBtnEvent(ElementTree *etree, Element *elem, const HumanInput::IGenJoystick *, InputEvent event, int btn_idx,
  int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int accum_res)
{
  if (::dgs_app_active && event == INP_EV_RELEASE && elem == etree->kbFocus && !(accum_res & R_PROCESSED))
  {
    int imeOpenJoyBtn = JOY_XINPUT_REAL_BTN_L_TRIGGER;

    // find custom button for opening IME
    Sqrat::Object imeBtnObj = elem->props.scriptDesc.RawGetSlot("imeOpenJoyBtn");
    if (imeBtnObj.GetType() == OT_STRING)
    {
      Sqrat::Var<const char *> imeString = imeBtnObj.GetVar<const char *>();
      HotkeyButton hkBtn = etree->guiScene->getHotkeys().resolveButton(imeString.value);
      if (hkBtn.devId == DEVID_JOYSTICK)
        imeOpenJoyBtn = hkBtn.btnId;
    }

    if (btn_idx == imeOpenJoyBtn)
    {
      open_ime(elem);
      return R_PROCESSED;
    }
  }
  return 0;
}


void BhvTextInput::onAttach(Element *elem)
{
  elem->recalcContentSize(0);
  scroll_to_cursor(elem);
}


void BhvTextInput::onDetach(Element *elem, DetachMode) { close_ime(elem); }


#if _TARGET_HAS_IME

void BhvTextInput::on_ime_finish(void *ud, const char *str, int cursor, int status)
{
  if (status < 0)
    return;

  bool applied = (status == 1);

  Element *elem = (Element *)ud;
  if (applied)
  {
    String text;
    Sqrat::Object cmaskobj = elem->props.getObject(elem->csk->charMask);
    if (cmaskobj.GetType() == OT_STRING)
    {
      const char *cmask = cmaskobj.GetVar<const char *>().value;
      if (strlen(cmask) > 0)
      {
        Tab<wchar_t> w_inp;
        char utf8_buf[4];

        bool isAllCharsPassed = true;
        convert_utf8_to_u16_buf(w_inp, str, -1);
        for (int i = 0; i < w_inp.size(); i++)
        {
          if (strstr(cmask, wchar_to_utf8(w_inp[i], utf8_buf, sizeof(utf8_buf))))
            text.append(utf8_buf);
          else
            isAllCharsPassed = false;
        }
        str = text.c_str();

        if (!isAllCharsPassed)
        {
          Sqrat::Function onWrongInput = elem->props.scriptDesc.GetFunction(elem->csk->onWrongInput);
          if (!onWrongInput.IsNull())
            onWrongInput();
        }
      }
    }

    apply_new_value(elem, str);

    elem->props.storage.SetValue(elem->csk->cursorPos, cursor < 0 ? elem->props.text.length() : cursor);

    close_ime(elem);
  }

  Sqrat::Object cbFunc = elem->props.scriptDesc.RawGetSlot("onImeFinish");
  if (!cbFunc.IsNull())
  {
    Sqrat::Function f(cbFunc.GetVM(), Sqrat::Object(cbFunc.GetVM()), cbFunc);
    GuiScene::get_from_elem(elem)->queueScriptHandler(new ScriptHandlerSqFunc<bool>(f, applied));
  }
}

#else

void BhvTextInput::on_ime_finish(void *, const char *, int, int) {}

#endif


#if _TARGET_HAS_IME
void BhvTextInput::open_ime(Element *elem)
{
  if (HumanInput::isImeAvailable())
  {
    DataBlock params;
    Sqrat::Object title = elem->props.getObject(elem->csk->title);
    if (title.GetType() == OT_STRING)
      params.setStr("title", title.GetVar<const SQChar *>().value);
    Sqrat::Object hint = elem->props.getObject(elem->csk->hint);
    if (hint.GetType() == OT_STRING)
      params.setStr("hint", hint.GetVar<const SQChar *>().value);

    params.setStr("str", elem->props.text.c_str());
    params.setInt("maxChars", elem->props.getInt(elem->csk->maxChars, 512));

    if (elem->props.getBool(elem->csk->imeNoAutoCap, false))
      params.setBool("optNoAutoCap", true);
    if (elem->props.getBool(elem->csk->imeNoCopy, false))
      params.setBool("optNoCopy", true);
    if (elem->props.getObject(elem->csk->password).Cast<bool>())
      params.setBool("optPassw", true);

    Sqrat::Object inputType = elem->props.getObject(elem->csk->inputType);
    if (inputType.GetType() == OT_STRING)
      params.setStr("type", inputType.GetVar<const SQChar *>().value);
    else if (inputType.GetType() != OT_NULL)
      darg_assert_trace_var("inputType must be string", elem->props.scriptDesc, elem->csk->inputType);

    Tab<wchar_t> wtext(framemem_ptr());
    int wlen = get_displayed_text_u(elem, wtext);
    int cursorPos = elem->props.storage.RawGetSlotValue(elem->csk->cursorPos, wlen);
    cursorPos = clamp(cursorPos, 0, wlen);
    params.setInt("optCursorPos", cursorPos);

    HumanInput::showScreenKeyboard_IME(params, on_ime_finish, elem);
  }
#if _TARGET_ANDROID || _TARGET_IOS
  else
    HumanInput::showScreenKeyboard(true);
#endif
}


void BhvTextInput::close_ime(Element *elem)
{
  if (HumanInput::isImeAvailable())
  {
    HumanInput::showScreenKeyboard_IME(DataBlock(), NULL, elem);
  }
#if _TARGET_ANDROID || _TARGET_IOS
  else
    HumanInput::showScreenKeyboard(false);
#endif
}


#else
void BhvTextInput::open_ime(Element *) {}
void BhvTextInput::close_ime(Element *) {}
#endif


} // namespace darg
