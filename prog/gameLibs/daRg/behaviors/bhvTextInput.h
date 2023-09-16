#pragma once

#include <daRg/dag_behavior.h>
#include <generic/dag_tab.h>


namespace darg
{


class BhvTextInput : public darg::Behavior
{
public:
  BhvTextInput();

  virtual int kbdEvent(ElementTree *, Element *, InputEvent /*event*/, int /*key_idx*/, bool /*repeat*/, wchar_t /*wc*/,
    int /*accum_res*/);
  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/, int /*btn_idx*/,
    short /*mx*/, short /*my*/, int /*buttons*/, int /*accum_res*/);
  virtual int joystickBtnEvent(ElementTree *, Element *, const HumanInput::IGenJoystick *, InputEvent /*event*/, int /*key_idx*/,
    int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int /*accum_res*/);
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;
  virtual bool willHandleClick(Element *) override { return true; }

  virtual void onAttach(Element *);
  virtual void onDetach(Element *, DetachMode);

private:
  static void scroll_to_cursor(Element *elem);
  static void apply_new_value(Element *elem, const char *new_value);
  static void open_ime(Element *elem);
  static void close_ime(Element *elem);
  static void on_ime_finish(void *ud, const char *str, int status);
  static void position_cursor_on_click(Element *elem, const Point2 &click_pos);
  static int get_displayed_text_u(Element *elem, Tab<wchar_t> &wtext);
  static int text_width_to_char_idx_u(Element *elem, int width_px, Tab<wchar_t> &wtext);
};


extern BhvTextInput bhv_text_input;


} // namespace darg
