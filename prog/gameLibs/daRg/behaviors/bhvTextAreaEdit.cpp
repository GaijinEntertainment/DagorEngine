// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTextAreaEdit.h"
#include "bhvTextArea.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_scriptHandlers.h>

#include "textLayout.h"
#include "textUtil.h"
#include "elementTree.h"
#include "dargDebugUtils.h"
#include "guiScene.h"
#include "textUtil.h"
#include "editableText.h"

#include <drv/hid/dag_hiCreate.h> // for _TARGET_HAS_IME
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiKeyboard.h>
#include <startup/dag_inpDevClsDrv.h>

#include <ioSys/dag_dataBlock.h>

#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_unicode.h>
#include <wctype.h>


/*

TODO:

* Clipboard support
* IME support (Mobile)
* IME support (Consoles, activation with gamepad)

*/


namespace darg
{

using namespace textlayout;


BhvTextAreaEdit bhv_text_area_edit;


BhvTextAreaEdit::BhvTextAreaEdit() :
  Behavior(0,
    F_HANDLE_KEYBOARD | F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_HANDLE_JOYSTICK | F_FOCUS_ON_CLICK | F_CAN_HANDLE_CLICKS | F_DISPLAY_IME)
{}


void BhvTextAreaEdit::onAttach(Element *elem)
{
  EditableText *etext = elem->props.scriptDesc.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (!etext)
    return;

  if (etext->boundElem != elem) // should be set up in onElemSetup(), but just in case
  {
    if (etext->boundElem)
    {
      darg_assert_trace_var("EditableText may be used only by one element | onAttach", elem->props.scriptDesc,
        elem->csk->editableText);
    }
    else
    {
      elem->props.storage.SetValue(elem->csk->textAreaEditorData, etext);
      etext->boundElem = elem;
    }
  }
}


void BhvTextAreaEdit::onElemSetup(Element *elem, SetupMode setup_mode)
{
  G_UNUSED(setup_mode);

  EditableText *curText = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  EditableText *newText = elem->props.scriptDesc.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);

  if (curText != newText)
  {
    if (newText && newText->boundElem)
    {
      darg_assert_trace_var("EditableText may be used only by one element | onElemSetup", elem->props.scriptDesc,
        elem->csk->editableText);
      return;
    }

    if (curText)
    {
      curText->boundElem = nullptr;
      curText->pressedCharButtons.clear();
    }

    if (newText)
      newText->boundElem = elem;

    elem->props.storage.SetValue(elem->csk->editableText, newText);
  }
}


void BhvTextAreaEdit::onDetach(Element *elem, DetachMode dmode)
{
  G_UNUSED(dmode);

  EditableText *curText = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (curText)
  {
    curText->boundElem = nullptr;
    curText->pressedCharButtons.clear();
    elem->props.storage.RawDeleteSlot(elem->csk->editableText);
  }

  close_ime(elem);
}


void BhvTextAreaEdit::fill_format_params(const Element *elem, const Point2 &elem_size, textlayout::FormatParams &params)
{
  const Properties &props = elem->props;

  float styleMaxWidth = props.getFloat(elem->csk->maxContentWidth, VERY_BIG_NUMBER);
  if (styleMaxWidth == VERY_BIG_NUMBER)
    styleMaxWidth = elem->sizeSpecToPixels(elem->layout.maxSize[0], 0);

  params.reset();

  params.defFontId = props.getFontId();
  params.defFontHt = (int)floorf(props.getFontSize() + 0.5f);
  params.spacing = props.getFloat(elem->csk->spacing, 0);
  params.monoWidth = font_mono_width_from_sq(params.defFontId, params.defFontHt, props.getObject(elem->csk->monoWidth));
  params.lineSpacing = props.getFloat(elem->csk->lineSpacing, 0.0);
  params.parSpacing = props.getFloat(elem->csk->parSpacing, 0);
  params.indent = props.getFloat(elem->csk->indent, 0);
  params.hangingIndent = props.getFloat(elem->csk->hangingIndent, 0);
  params.maxWidth = (styleMaxWidth == VERY_BIG_NUMBER || styleMaxWidth <= 0) ? elem_size.x
                    : elem_size.x <= 0                                       ? styleMaxWidth
                                                                             : min(elem_size.x, styleMaxWidth);
  params.maxHeight = elem_size.y;
}


void BhvTextAreaEdit::recalc_content(const Element *elem, int /*axis*/, const Point2 &elem_size, Point2 &out_size)
{
  out_size.zero();

  const Properties &props = elem->props;

  EditableText *etext = props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);

  if (!etext)
    return;

  FormattedText *fmtText = &etext->fmtText;

  // if (axis == 0) // already calculated for Y
  //  FIXME
  {
    FormatParams params = {};
    fill_format_params(elem, elem_size, params);
    fmtText->format(params);
  }

  for (int iLine = 0, nLines = fmtText->lines.size(); iLine < nLines; ++iLine)
  {
    const TextLine &line = fmtText->lines[iLine];
    out_size.x = ::max(out_size.x, line.contentWidth);
    if (iLine == nLines - 1)
      out_size.y = line.yPos + line.contentHeight;
  }

  if (elem->layout.size[1].mode != SizeSpec::CONTENT)
  {
    ElemAlign valign = (ElemAlign)props.getInt(elem->csk->valign, ALIGN_TOP);

    if (valign == ALIGN_CENTER)
      fmtText->yOffset = floorf((elem_size.y - out_size.y) * 0.5f);
    else if (valign == ALIGN_BOTTOM)
      fmtText->yOffset = (elem_size.y - out_size.y);
  }
}


void BhvTextAreaEdit::scroll_cursor_into_view(darg::Element *elem, EditableText *etext)
{
  FormattedText *fmtText = &etext->fmtText;

  int relChar = 0;
  int curBlockIdx = find_block_left(fmtText, etext->cursorPos, relChar);
  if (curBlockIdx >= 0)
  {
    GuiScene *guiScene = GuiScene::get_from_elem(elem);
    IPoint2 screenSize = guiScene->getDeviceScreenSize();
    float margin = ::min(float(screenSize.x) * 0.05f, elem->screenCoord.size.x * 0.25f);

    ScreenCoord &sc = elem->screenCoord;

    Point2 targetScrollPos = sc.scrollOffs;

    TextBlock *curBlock = fmtText->blocks[curBlockIdx];
    float cursorScreenX = curBlock->position.x + get_cursor_pixels_pos_in_block(elem, curBlock, relChar);
    if (cursorScreenX < sc.scrollOffs.x)
      targetScrollPos.x = cursorScreenX - margin;
    else if (cursorScreenX > sc.scrollOffs.x + sc.size.x)
      targetScrollPos.x = cursorScreenX - sc.size.x + margin;

    if (curBlock->position.y < sc.scrollOffs.y)
      targetScrollPos.y = curBlock->position.y - margin;
    else if (curBlock->position.y + curBlock->size.y > sc.scrollOffs.y + sc.size.y)
      targetScrollPos.y = curBlock->position.y + curBlock->size.y - sc.size.y + margin;

    elem->scrollTo(targetScrollPos);
  }
  else
    elem->scrollTo(Point2(0, 0));
}


int BhvTextAreaEdit::mouseEvent(ElementTree *etree, Element *elem, InputDevice /*device*/, InputEvent event, int /*pointer_id*/,
  int /*data*/, short mx, short my, int /*buttons*/, int accum_res)
{
  if (event == INP_EV_MOUSE_WHEEL || event == INP_EV_POINTER_MOVE)
    return 0;

  if (elem->rendObjType != rendobj_textarea_id)
    return 0;

  if (event == INP_EV_RELEASE && elem->hitTest(mx, my))
    return R_PROCESSED;

  if (event == INP_EV_PRESS && !(accum_res & R_PROCESSED) && elem->hitTest(mx, my) &&
      (!etree->hasCapturedKbFocus() || etree->kbFocus == elem))
  {
    if (etree->kbFocus != elem) // keep capture mode if already set
      etree->setKbFocus(elem);
    position_cursor_by_screen_coord(elem, Point2(mx, my));
    open_ime(elem);

    return R_PROCESSED;
  }
  return 0;
}


int BhvTextAreaEdit::touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing * /*pnt*/,
  int /*touch_idx*/, const HumanInput::PointingRawState::Touch &touch, int accum_res)
{
  if (elem->rendObjType != rendobj_textarea_id)
    return 0;

  if (event == INP_EV_RELEASE && !(accum_res & R_PROCESSED) && elem->hitTest(touch.x, touch.y) && !etree->hasCapturedKbFocus())
  {
    etree->setKbFocus(elem);
    position_cursor_by_screen_coord(elem, Point2(touch.x, touch.y));
    open_ime(elem);

    return R_PROCESSED;
  }
  return 0;
}


int BhvTextAreaEdit::joystickBtnEvent(ElementTree * /*etree*/, Element * /*elem*/, const HumanInput::IGenJoystick *,
  InputEvent /*event*/, int /*btn_idx*/, int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int /*accum_res*/)
{
  // to implement in future
  return 0;
}


int BhvTextAreaEdit::find_block_right(textlayout::FormattedText *fmt_text, int cursor_pos, int &in_block_pos)
{
  int absPos = 0;
  for (int block = 0; block < fmt_text->blocks.size(); ++block)
  {
    textlayout::TextBlock *tb = fmt_text->blocks[block];
    int relPos = cursor_pos - absPos;
    if (relPos >= 0 && relPos < tb->numChars)
    {
      in_block_pos = relPos;
      return block;
    }
    absPos += tb->numChars;
  }

  in_block_pos = 0;
  return -1;
}


int BhvTextAreaEdit::find_block_left(textlayout::FormattedText *fmt_text, int cursor_pos, int &in_block_pos)
{
  int absPos = 0;
  for (int block = 0; block < fmt_text->blocks.size(); ++block)
  {
    textlayout::TextBlock *tb = fmt_text->blocks[block];
    int relPos = cursor_pos - absPos;
    if (relPos > 0 && relPos <= tb->numChars)
    {
      in_block_pos = relPos;
      return block;
    }
    absPos += tb->numChars;
  }

  in_block_pos = -1;
  return -1;
}


static void merge_text_blocks(textlayout::FormattedText *fmtText)
{
  for (int i = int(fmtText->blocks.size()) - 1; i > 1; --i)
  {
    TextBlock *curBlock = fmtText->blocks[i];
    TextBlock *prevBlock = fmtText->blocks[i - 1];

    if (curBlock->type == TextBlock::TBT_TEXT && prevBlock->type == TextBlock::TBT_TEXT)
    {
      prevBlock->text += curBlock->text;
      prevBlock->numChars += curBlock->numChars;
      prevBlock->guiText.discard();
      fmtText->freeTextBlock(curBlock);
      fmtText->blocks.erase(fmtText->blocks.begin() + i);
    }
  }
}


static bool split_text_block(textlayout::FormattedText *fmtText, int block_idx, int rel_pos_chars)
{
  TextBlock *curBlock = fmtText->blocks[block_idx];
  if (curBlock->type != TextBlock::TBT_TEXT)
    return false;

  if (rel_pos_chars <= 0 || rel_pos_chars >= curBlock->numChars)
    return false;

  int relBytes = utf_calc_bytes_for_symbols(curBlock->text.c_str(), curBlock->text.length(), rel_pos_chars);

  TextBlock *newBlock = fmtText->allocateTextBlock();
  newBlock->type = TextBlock::TBT_TEXT;
  newBlock->numChars = curBlock->numChars - rel_pos_chars;
  newBlock->text = eastl::string(curBlock->text.c_str() + relBytes);
  curBlock->text.erase(curBlock->text.begin() + relBytes, curBlock->text.end());
  curBlock->numChars = rel_pos_chars;
  curBlock->guiText.discard();
  newBlock->guiText.discard();

  fmtText->blocks.insert(fmtText->blocks.begin() + block_idx + 1, newBlock);
  return true;
}


int BhvTextAreaEdit::kbdEvent(ElementTree *etree, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t wc, int accum_res)
{
  using namespace HumanInput;

  G_UNUSED(etree);
  G_UNUSED(repeat);

  if (elem->rendObjType != rendobj_textarea_id)
    return 0;

  EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (!etext)
    return 0;

  FormattedText *fmtText = &etext->fmtText;

  if (accum_res & R_PROCESSED)
    return 0;

  G_ASSERT((flags & F_HANDLE_KEYBOARD_GLOBAL) == (flags & F_HANDLE_KEYBOARD)); // must be only focusable

  IGenKeyboard *kbd = global_cls_drv_kbd->getDevice(0);
  bool isCtrlPressed = kbd && (kbd->getRawState().shifts & KeyboardRawState::CTRL_BITS);

  if (event == INP_EV_PRESS)
  {
    if (key_idx == DKEY_BACK)
    {
      int relChar = -1;
      int curBlockIdx = find_block_left(fmtText, etext->cursorPos, relChar);
      if (curBlockIdx >= 0 && relChar > 0)
      {
        TextBlock *curBlock = fmtText->blocks[curBlockIdx];

        if (curBlock->type == TextBlock::TBT_TEXT)
        {
          // Handle UTF
          int relBytes = utf_calc_bytes_for_symbols(curBlock->text.c_str(), curBlock->text.length(), relChar);
          if (relBytes > 0) // safety check, should never happen
          {
            const char *eraseEnd = curBlock->text.c_str() + relBytes;
            int sz = get_prev_char_size(eraseEnd - 1, relBytes);
            curBlock->text.erase(eraseEnd - sz, eraseEnd);

            --curBlock->numChars;
            --etext->cursorPos;

            curBlock->guiText.discard();
            fmtText->lastFormatParamsForCurText.reset();

            if (curBlock->text.empty())
            {
              fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
              fmtText->freeTextBlock(curBlock);
            }
          }
        }
        else
        {
          fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
          fmtText->freeTextBlock(curBlock);
          fmtText->lastFormatParamsForCurText.reset();
          --etext->cursorPos;

          merge_text_blocks(fmtText);
        }

        recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

        scroll_cursor_into_view(elem, etext);
        call_change_script_handler(elem, etext);

        return R_PROCESSED;
      }
    }
    else if (key_idx == DKEY_DELETE)
    {
      int relChar = -1;
      int curBlockIdx = find_block_right(fmtText, etext->cursorPos, relChar);
      if (curBlockIdx >= 0)
      {
        TextBlock *curBlock = fmtText->blocks[curBlockIdx];

        if (curBlock->type == TextBlock::TBT_TEXT)
        {
          // Handle UTF
          int relBytes = utf_calc_bytes_for_symbols(curBlock->text.c_str(), curBlock->text.length(), relChar);
          const char *eraseStart = curBlock->text.c_str() + relBytes;
          int sz = get_next_char_size(eraseStart);
          curBlock->text.erase(eraseStart, eraseStart + sz);
          --curBlock->numChars;
          curBlock->guiText.discard();
          fmtText->lastFormatParamsForCurText.reset();

          if (curBlock->text.empty())
          {
            fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
            fmtText->freeTextBlock(curBlock);

            if (curBlockIdx == fmtText->blocks.size())
              etext->cursorPos = max(0, etext->cursorPos - 1);

            merge_text_blocks(fmtText);
          }
        }
        else
        {
          fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
          fmtText->freeTextBlock(curBlock);
          fmtText->lastFormatParamsForCurText.reset();

          if (curBlockIdx == fmtText->blocks.size())
            etext->cursorPos = max(0, etext->cursorPos - 1);

          merge_text_blocks(fmtText);
        }

        recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

        scroll_cursor_into_view(elem, etext);
        call_change_script_handler(elem, etext);

        return R_PROCESSED;
      }
    }
    else if (key_idx == DKEY_LEFT)
    {
      --etext->cursorPos;
      if (isCtrlPressed && etext->cursorPos > 0)
      {
        int relChar = -1;
        int curBlockIdx = find_block_left(fmtText, etext->cursorPos, relChar);
        if (curBlockIdx >= 0)
        {
          if (fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_TEXT)
            etext->cursorPos -= relChar;
          else
          {
            while (curBlockIdx > 0 && fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_SPACE)
            {
              etext->cursorPos -= fmtText->blocks[curBlockIdx]->numChars;
              --curBlockIdx;
            }
          }
        }
      }

      etext->cursorPos = max(0, etext->cursorPos);

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_RIGHT)
    {
      int numCharsTotal = 0;
      for (TextBlock *block : fmtText->blocks)
        numCharsTotal += block->numChars;

      etext->cursorPos = min(numCharsTotal, etext->cursorPos); // just in case

      if (!isCtrlPressed)
      {
        etext->cursorPos = min(numCharsTotal, etext->cursorPos + 1);
      }
      else if (etext->cursorPos < numCharsTotal)
      {
        int relChar = -1;
        int curBlockIdx = find_block_right(fmtText, etext->cursorPos, relChar);
        if (curBlockIdx >= 0)
        {
          if (fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_TEXT)
            etext->cursorPos += (fmtText->blocks[curBlockIdx]->numChars - relChar);
          else if (fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_LINE_BREAK)
            etext->cursorPos += 1;
          else
          {
            while (curBlockIdx < fmtText->blocks.size() && fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_SPACE)
            {
              etext->cursorPos += fmtText->blocks[curBlockIdx]->numChars;
              ++curBlockIdx;
            }
          }
        }
      }

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_HOME)
    {
      if (isCtrlPressed)
        etext->cursorPos = 0;
      else
      {
        int relChar = -1;
        int curBlockIdx = find_block_left(fmtText, etext->cursorPos, relChar);
        bool isFirst = true;
        while (curBlockIdx >= 0 && fmtText->blocks[curBlockIdx]->type != TextBlock::TBT_LINE_BREAK)
        {
          int delta = (isFirst ? relChar : fmtText->blocks[curBlockIdx]->numChars);
          etext->cursorPos -= delta;
          --curBlockIdx;
          isFirst = false;
        }
        etext->cursorPos = max(0, etext->cursorPos); // just in case
      }

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_END)
    {
      int numCharsTotal = 0;
      for (TextBlock *block : fmtText->blocks)
      {
        if (!isCtrlPressed && numCharsTotal >= etext->cursorPos && block->type == TextBlock::TBT_LINE_BREAK)
          break;
        numCharsTotal += block->numChars;
      }
      etext->cursorPos = numCharsTotal;

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_UP)
    {
      change_line(elem, etext, -1);
      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_DOWN)
    {
      change_line(elem, etext, +1);
      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (key_idx == DKEY_RETURN)
    {
      int relChar = -1;
      int curBlockIdx = find_block_right(fmtText, etext->cursorPos, relChar);
      if (curBlockIdx >= 0 && relChar > 0)
      {
        if (split_text_block(fmtText, curBlockIdx, relChar))
          ++curBlockIdx;
      }

      TextBlock *block = fmtText->allocateTextBlock();
      block->type = TextBlock::TBT_LINE_BREAK;
      block->numChars = 1;
      if (curBlockIdx >= 0)
        fmtText->blocks.insert(fmtText->blocks.begin() + curBlockIdx, block);
      else
        fmtText->blocks.push_back(block);

      ++etext->cursorPos;

      fmtText->lastFormatParamsForCurText.reset();

      recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
    else if (wc)
    {
      if (iswspace(wc))
      {
        int relChar = -1;
        int curBlockIdx = find_block_right(fmtText, etext->cursorPos, relChar);
        if (curBlockIdx >= 0 && relChar > 0 && fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_TEXT)
        {
          if (split_text_block(fmtText, curBlockIdx, relChar))
            ++curBlockIdx;
        }

        TextBlock *block = fmtText->allocateTextBlock();
        block->type = TextBlock::TBT_SPACE;
        block->numChars = 1;
        if (curBlockIdx >= 0)
          fmtText->blocks.insert(fmtText->blocks.begin() + curBlockIdx, block);
        else
          fmtText->blocks.push_back(block);
      }
      else
      {
        int relChar = -1;
        int curBlockIdx = find_block_right(fmtText, etext->cursorPos, relChar);
        if (curBlockIdx < 0 || fmtText->blocks[curBlockIdx]->type != TextBlock::TBT_TEXT)
          curBlockIdx = find_block_left(fmtText, etext->cursorPos, relChar);

        if (curBlockIdx < 0 || fmtText->blocks[curBlockIdx]->type != TextBlock::TBT_TEXT)
        {
          TextBlock *block = fmtText->allocateTextBlock();
          block->type = TextBlock::TBT_TEXT;
          if (curBlockIdx < 0 || fmtText->blocks.empty())
            curBlockIdx = 0;
          else if (fmtText->blocks[curBlockIdx]->type != TextBlock::TBT_TEXT)
            ++curBlockIdx;
          else
            curBlockIdx = clamp(curBlockIdx, 0, int(fmtText->blocks.size())); // probably not needed
          fmtText->blocks.insert(fmtText->blocks.begin() + curBlockIdx, block);
          relChar = 0;
        }

        char utf8Str[9];
        wchar_t wstr[2] = {wc, 0};
        G_VERIFY(wcs_to_utf8(wstr, utf8Str, sizeof(utf8Str)));

        TextBlock *block = fmtText->blocks[curBlockIdx];
        int relBytes = utf_calc_bytes_for_symbols(block->text.c_str(), block->text.length(), relChar);
        block->text.insert(relBytes, utf8Str);
        block->calcNumChars();
        block->guiText.discard();
      }

      ++etext->cursorPos;
      // because we don't have non-zero wc for INP_EV_RELEASE
      etext->pressedCharButtons.insert(wc);

      fmtText->lastFormatParamsForCurText.reset();

      recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext);
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    // prevent propagation to other elements and behaviors
    if (key_idx == DKEY_BACK || key_idx == DKEY_DELETE || key_idx == DKEY_LEFT || key_idx == DKEY_RIGHT || key_idx == DKEY_HOME ||
        key_idx == DKEY_END || key_idx == DKEY_UP || key_idx == DKEY_DOWN || key_idx == DKEY_RETURN)
    {
      return R_PROCESSED;
    }
    else if (etext->pressedCharButtons.find(wc) != etext->pressedCharButtons.end())
    {
      etext->pressedCharButtons.erase(wc);
      return R_PROCESSED;
    }
  }

  return 0;
}


void BhvTextAreaEdit::position_cursor_by_screen_coord(Element *elem, const Point2 &screen_pos)
{
  EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (!etext)
    return;

  FormattedText *fmtText = &etext->fmtText;

  etext->cursorPos = 0;

  if (fmtText->lines.empty())
    return;

  Point2 localPos = elem->screenPosToElemLocal(screen_pos);

  int curLineIdx = 0;

  for (int iLine = 0, nLines = fmtText->lines.size(); iLine < nLines; ++iLine)
  {
    const TextLine &line = fmtText->lines[iLine];
    if (line.yPos > localPos.y)
      break;
    curLineIdx = iLine;
  }

  if (curLineIdx < 0)
    return;

  const TextLine &line = fmtText->lines[curLineIdx];
  position_cursor_on_line_by_coord(elem, etext, line, localPos.x);
}


void BhvTextAreaEdit::position_cursor_on_line_by_coord(darg::Element *elem, EditableText *etext, const textlayout::TextLine &line,
  float xt)
{
  int nearestBlockIdx = -1;
  float nearestBlockDistance = VERY_BIG_NUMBER;

  for (int iBlock = 0, nBlocks = line.blocks.size(); iBlock < nBlocks; ++iBlock)
  {
    const TextBlock &block = *line.blocks[iBlock];

    float distance = VERY_BIG_NUMBER;
    if (block.position.x > xt)
      distance = block.position.x - xt;
    else if (block.position.x + block.size.x < xt)
      distance = xt - (block.position.x + block.size.x);
    else
      distance = 0;

    if (distance < nearestBlockDistance)
    {
      nearestBlockDistance = distance;
      nearestBlockIdx = iBlock;
      if (distance == 0)
        break;
    }
  }

  if (nearestBlockIdx < 0)
    return;

  int relChar = 0;
  TextBlock *block = line.blocks[nearestBlockIdx];
  if (block->type == TextBlock::TBT_TEXT)
  {
    if (block->numChars)
    {
      Tab<wchar_t> wtext(framemem_ptr());
      wtext.resize(block->text.length() + 2);
      int wlen = utf8_to_wcs_ex(block->text.c_str(), block->text.length(), wtext.data(), wtext.size() - 1);
      wtext.resize(wlen + 1);
      wtext[wlen] = 0;

      textlayout::FormatParams fmtParams = {};
      fill_format_params(elem, elem->screenCoord.size, fmtParams);

      StdGuiFontContext fctx;
      StdGuiRender::get_font_context(fctx, fmtParams.defFontId, fmtParams.spacing, fmtParams.monoWidth, fmtParams.defFontHt);

      float minDist = 1e6f;
      //== binary search would be faster
      for (int pos = 0; pos < wtext.size(); pos++)
      {
        Point2 sp =
          calc_text_size_u(wtext.data(), pos, fmtParams.defFontId, fmtParams.spacing, fmtParams.monoWidth, fmtParams.defFontHt);
        float screenDist = fabsf(sp.x - (xt - block->position.x));
        if (screenDist >= minDist)
          break;
        relChar = pos;
        minDist = screenDist;
      }
    }
  }
  else if (block->type == TextBlock::TBT_SPACE)
  {
    relChar = (xt - block->position.x > block->size.x * 0.5f) ? 1 : 0;
  }
  else if (block->type == TextBlock::TBT_LINE_BREAK)
  {
    relChar = 1;
  }

  int absPos = 0;
  for (TextBlock *tb : etext->fmtText.blocks)
  {
    if (tb == block)
    {
      etext->cursorPos = absPos + relChar;
      break;
    }
    absPos += tb->numChars;
  }
}


float BhvTextAreaEdit::get_cursor_pixels_pos_in_block(const Element *elem, textlayout::TextBlock *curBlock, int relPos)
{
  if (curBlock->type == textlayout::TextBlock::TBT_TEXT)
  {
    textlayout::FormatParams fmtParams = {};
    BhvTextAreaEdit::fill_format_params(elem, elem->screenCoord.size, fmtParams);

    StdGuiFontContext fctx;
    StdGuiRender::get_font_context(fctx, fmtParams.defFontId, fmtParams.spacing, fmtParams.monoWidth, fmtParams.defFontHt);

    int relPosBytes = utf_calc_bytes_for_symbols(curBlock->text.c_str(), curBlock->text.length(), relPos);
    BBox2 bbox = StdGuiRender::get_str_bbox(curBlock->text.c_str(), relPosBytes, fctx);
    return (bbox.isempty() ? 0 : bbox.right()) + fmtParams.spacing * 0.5f;
  }
  else
  {
    return curBlock->size.x;
  }
}

void BhvTextAreaEdit::change_line(Element *elem, EditableText *etext, int line_delta)
{
  FormattedText *fmtText = &etext->fmtText;

  if (fmtText->lines.size() < 2)
    return;

  int relPos = -1;
  int curBlockIdx = find_block_left(fmtText, etext->cursorPos, relPos);

  if (curBlockIdx < 0)
  {
    curBlockIdx = 0;
    relPos = 0;
  }

  textlayout::TextBlock *curBlock = fmtText->blocks[curBlockIdx];
  float cursorScreenPos = curBlock->position.x + get_cursor_pixels_pos_in_block(elem, curBlock, relPos);

  int curLineIdx = -1;
  for (int iLine = 0, nLines = fmtText->lines.size(); iLine < nLines; ++iLine)
  {
    TextLine &line = fmtText->lines[iLine];
    if (eastl::find(line.blocks.begin(), line.blocks.end(), curBlock) != line.blocks.end())
    {
      curLineIdx = iLine;
      break;
    }
  }

  if (curLineIdx < 0)
    return;

  int newLineIdx = curLineIdx + line_delta;
  if (newLineIdx < 0 || newLineIdx >= fmtText->lines.size())
    return;

  TextLine &nextLine = fmtText->lines[curLineIdx + line_delta];
  position_cursor_on_line_by_coord(elem, etext, nextLine, cursorScreenPos);
}


void BhvTextAreaEdit::call_change_script_handler(Element *elem, EditableText *etext)
{
  Sqrat::Function onChange = elem->props.scriptDesc.GetFunction(elem->csk->onChange);
  if (!onChange.IsNull())
  {
    Sqrat::Object etextRef(etext, onChange.GetVM());
    auto handler = new ScriptHandlerSqFunc<Sqrat::Object>(onChange, etextRef);
    elem->etree->guiScene->queueScriptHandler(handler);
  }
}

#if _TARGET_HAS_IME

void BhvTextAreaEdit::on_ime_finish(void *ud, const char *str, int cursor, int status)
{
  if (status < 0)
    return;

  bool applied = (status == 1);

  Element *elem = (Element *)ud;
  if (applied)
  {

    EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
    if (!etext)
    {
      LOGERR_ONCE("EditableText not found in element with TextAreaEdit | on_ime_finish");
    }
    else
    {
      etext->setText(str, -1);
      if (cursor >= 0)
        etext->cursorPos = cursor;
      close_ime(elem);
    }
  }

  Sqrat::Object cbFunc = elem->props.scriptDesc.RawGetSlot("onImeFinish");
  if (!cbFunc.IsNull())
  {
    Sqrat::Function f(cbFunc.GetVM(), Sqrat::Object(cbFunc.GetVM()), cbFunc);
    GuiScene::get_from_elem(elem)->queueScriptHandler(new ScriptHandlerSqFunc<bool>(f, applied));
  }
}


void BhvTextAreaEdit::open_ime(Element *elem)
{
  EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  if (!etext)
  {
    LOGERR_ONCE("EditableText not found in element with TextAreaEdit | open_ime");
  }

  if (HumanInput::isImeAvailable() && etext)
  {
    FormattedText *fmtText = &etext->fmtText;

    DataBlock params;
    params.setBool("optMultiLine", true);

    eastl::string text;
    fmtText->join(text);

    Sqrat::Object title = elem->props.getObject(elem->csk->title);
    if (title.GetType() == OT_STRING)
      params.setStr("title", title.GetVar<const SQChar *>().value);
    Sqrat::Object hint = elem->props.getObject(elem->csk->hint);
    if (hint.GetType() == OT_STRING)
      params.setStr("hint", hint.GetVar<const SQChar *>().value);

    params.setStr("str", text.c_str());
    params.setInt("maxChars", elem->props.getInt(elem->csk->maxChars, 2048));

    if (elem->props.getBool(elem->csk->imeNoAutoCap, false))
      params.setBool("optNoAutoCap", true);
    if (elem->props.getBool(elem->csk->imeNoCopy, false))
      params.setBool("optNoCopy", true);

    Sqrat::Object inputType = elem->props.getObject(elem->csk->inputType);
    if (inputType.GetType() == OT_STRING)
      params.setStr("type", inputType.GetVar<const SQChar *>().value);
    else if (inputType.GetType() != OT_NULL)
      darg_assert_trace_var("inputType must be string", elem->props.scriptDesc, elem->csk->inputType);

    params.setInt("optCursorPos", etext->cursorPos);

    HumanInput::showScreenKeyboard_IME(params, on_ime_finish, elem);
  }
#if _TARGET_ANDROID || _TARGET_IOS
  else
    HumanInput::showScreenKeyboard(true);
#endif
}

void BhvTextAreaEdit::close_ime(Element *elem)
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

void BhvTextAreaEdit::open_ime(Element *) {}
void BhvTextAreaEdit::close_ime(Element *) {}
void BhvTextAreaEdit::on_ime_finish(void *, const char *, int, int) {}

#endif

} // namespace darg
