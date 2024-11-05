// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <sqrat.h>


namespace textlayout
{
class FormattedText;
struct FormatParams;
struct TextLine;
struct TextBlock;
} // namespace textlayout


namespace darg
{

class EditableText;


class BhvTextAreaEdit : public darg::Behavior
{
public:
  BhvTextAreaEdit();

  virtual void onAttach(Element *elem);
  virtual void onDetach(Element *elem, DetachMode);
  virtual void onElemSetup(Element *, SetupMode);

  virtual int kbdEvent(ElementTree *, Element *, InputEvent /*event*/, int /*key_idx*/, bool /*repeat*/, wchar_t /*wc*/,
    int /*accum_res*/);
  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/, int /*btn_idx*/,
    short /*mx*/, short /*my*/, int /*buttons*/, int /*accum_res*/);
  virtual int joystickBtnEvent(ElementTree *, Element *, const HumanInput::IGenJoystick *, InputEvent /*event*/, int /*key_idx*/,
    int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int /*accum_res*/);
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;
  virtual bool willHandleClick(Element *) override { return true; }

  static void fill_format_params(const Element *elem, const Point2 &elem_size, textlayout::FormatParams &params);

  static int find_block_right(textlayout::FormattedText *fmt_text, int cursor_pos, int &in_block_pos);
  static int find_block_left(textlayout::FormattedText *fmt_text, int cursor_pos, int &in_block_pos);

  static void recalc_content(const Element *elem, int /*axis*/, const Point2 &elem_size, Point2 &out_size);

private:
  static void position_cursor_by_screen_coord(Element *elem, const Point2 &click_pos);
  static void position_cursor_on_line_by_coord(darg::Element *elem, EditableText *etext, const textlayout::TextLine &line, float xt);
  static void change_line(Element *elem, EditableText *etext, int line_delta);
  static float get_cursor_pixels_pos_in_block(const Element *elem, textlayout::TextBlock *curBlock, int relPos);
  static void scroll_cursor_into_view(darg::Element *elem, EditableText *etext);
  static void call_change_script_handler(Element *elem, EditableText *etext);

  static void open_ime(Element *elem);
  static void close_ime(Element *elem);
  static void on_ime_finish(void *ud, const char *str, int curosr, int status);

  friend class EditableText;
};


extern BhvTextAreaEdit bhv_text_area_edit;


} // namespace darg
