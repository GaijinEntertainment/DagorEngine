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
#include "kbFocus.h"

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

    // Reparse in case if the tags vocabulary has changed.
    // It is fixed for an EditableText instance by contract;
    // changing it on a same-instance rebuild is not supported.
    if (newText && elem->props.scriptDesc.RawGetSlotValue<bool>(elem->csk->allowTags, false))
    {
      eastl::string raw;
      newText->fmtText.join(raw);
      newText->reformatFrom(raw.c_str(), raw.length());
    }
  }

  elem->updFlags(Element::F_CLIP_CHILDREN, true);
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


void BhvTextAreaEdit::onKbFocusChange(Element *elem, bool focused)
{
  if (!focused)
    if (EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr))
      etext->pressedCharButtons.clear();

  bool imeOnFocus = elem->props.scriptDesc.RawGetSlotValue<bool>("imeOnFocus", false);
  if (imeOnFocus)
  {
    if (focused)
      open_ime(elem);
    else
      close_ime(elem);
  }
}


void BhvTextAreaEdit::recalc_content(const Element *elem, int /*axis*/, const Point2 &elem_size, Point2 &out_size)
{
  out_size.zero();

  const Properties &props = elem->props;

  EditableText *etext = props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);

  if (!etext)
    return;

  FormattedText *fmtText = &etext->fmtText;

  if (!fmtText->canReuseLinesFor(calc_textarea_max_width(elem, elem_size)))
  {
    FormatParams params = {};
    fill_textarea_format_params(elem, elem_size, params);
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


static textlayout::FormattedText *get_editor_fmt_text(Element *elem)
{
  EditableText *etext = elem->props.storage.RawGetSlotValue<EditableText *>(elem->csk->editableText, nullptr);
  return etext ? &etext->fmtText : nullptr;
}


void BhvTextAreaEdit::contributeChildren(Element *elem, dag::Vector<Sqrat::Object, framemem_allocator> &children)
{
  FormattedText *ft = get_editor_fmt_text(elem);
  if (!ft)
    return;
  for (auto &p : ft->embeddedComps)
    children.push_back(p.second);
}


void BhvTextAreaEdit::onRecalcLayout(Element *elem)
{
  FormattedText *ft = get_editor_fmt_text(elem);
  if (!ft)
    return;

  eastl::vector_set<Element *, eastl::less<Element *>, framemem_allocator> processedChildren;
  for (auto &p : ft->embeddedComps)
  {
    TextBlock *block = p.first;
    const Sqrat::Object &comp = p.second;

    for (Element *child : elem->children)
    {
      if (processedChildren.find(child) != processedChildren.end())
        continue;

      if (child->props.scriptDesc.IsEqual(comp) || child->props.scriptBuilder.IsEqual(comp))
      {
        child->screenCoord.relPos = block->position;
        child->screenCoord.size = block->size;
        child->screenCoord.screenPos = elem->screenCoord.screenPos + child->screenCoord.relPos;
        child->recalcScreenPositions();
        processedChildren.insert(child);
        break;
      }
    }
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


int BhvTextAreaEdit::pointingEvent(ElementTree *etree, Element *elem, InputDevice /*device*/, InputEvent event, int /*pointer_id*/,
  int /*button_id*/, Point2 pos, int accum_res)
{
  if (event == INP_EV_MOUSE_WHEEL || event == INP_EV_POINTER_MOVE)
    return 0;

  if (elem->rendObjType != rendobj_textarea_id)
    return 0;

  if (event == INP_EV_RELEASE && elem->hitTest(pos))
    return R_PROCESSED;

  KbFocus &kbFocus = etree->guiScene->kbFocus;

  if (event == INP_EV_PRESS && !(accum_res & R_PROCESSED) && elem->hitTest(pos) &&
      (!kbFocus.hasCapturedFocus() || kbFocus.focus == elem))
  {
    if (kbFocus.focus != elem) // keep capture mode if already set
      kbFocus.setFocus(elem);
    position_cursor_by_screen_coord(elem, pos);
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


static bool is_same_text_style(const TextBlock *a, const TextBlock *b)
{
  if (a->useCustomColor != b->useCustomColor)
    return false;
  if (a->useCustomColor && a->customColor != b->customColor)
    return false;
  return a->fontId == b->fontId && a->fontHt == b->fontHt;
}


// Mutates prevBlock->text in place. Callers must call fmtText->invalidateShapes()
// before the next format() so the merged block gets reshaped.
static void merge_text_blocks(textlayout::FormattedText *fmtText)
{
  for (int i = int(fmtText->blocks.size()) - 1; i >= 1; --i)
  {
    TextBlock *curBlock = fmtText->blocks[i];
    TextBlock *prevBlock = fmtText->blocks[i - 1];

    if (curBlock->type == TextBlock::TBT_TEXT && prevBlock->type == TextBlock::TBT_TEXT && is_same_text_style(prevBlock, curBlock))
    {
      prevBlock->text += curBlock->text;
      prevBlock->numChars += curBlock->numChars;
      prevBlock->guiText.discard();
      fmtText->freeTextBlock(curBlock);
      fmtText->blocks.erase(fmtText->blocks.begin() + i);
    }
  }
}


// Mutates curBlock->text in place (truncates it). Callers must call
// fmtText->invalidateShapes() before the next format() so the truncated block
// gets reshaped; the inserted newBlock starts with hasValidShape=false already.
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
  newBlock->useCustomColor = curBlock->useCustomColor;
  newBlock->customColor = curBlock->customColor;
  newBlock->fontId = curBlock->fontId;
  newBlock->fontHt = curBlock->fontHt;
  curBlock->text.erase(curBlock->text.begin() + relBytes, curBlock->text.end());
  curBlock->numChars = rel_pos_chars;
  curBlock->guiText.discard();
  newBlock->guiText.discard();

  fmtText->blocks.insert(fmtText->blocks.begin() + block_idx + 1, newBlock);
  return true;
}


static int total_num_chars(const FormattedText *fmtText)
{
  int total = 0;
  for (const TextBlock *block : fmtText->blocks)
    total += block->numChars;
  return total;
}


// Drop/trim payload blocks from the end to fit max_chars. freeTextBlock also unregisters
// a dropped chip from embeddedComps, so fmtText is needed even though blocks is detached.
static void trim_blocks_to(FormattedText *fmtText, Tab<TextBlock *> &blocks, int max_chars)
{
  int total = 0;
  for (const TextBlock *block : blocks)
    total += block->numChars;

  for (int i = int(blocks.size()) - 1; i >= 0 && total > max_chars; --i)
  {
    TextBlock *block = blocks[i];
    int excess = total - max_chars;
    if (block->type == TextBlock::TBT_TEXT && block->numChars > excess)
    {
      int keepChars = block->numChars - excess;
      int keepBytes = utf_calc_bytes_for_symbols(block->text.c_str(), block->text.length(), keepChars);
      block->text.erase(block->text.begin() + keepBytes, block->text.end());
      block->numChars = keepChars;
      block->guiText.discard();
      total = max_chars;
    }
    else
    {
      total -= block->numChars;
      fmtText->freeTextBlock(block);
      blocks.erase(blocks.begin() + i);
    }
  }
}


void BhvTextAreaEdit::insert_text(Element *elem, EditableText *etext, const char *text, int len, int insert_pos)
{
  FormattedText *fmtText = &etext->fmtText;

  if (fmtText->isFormatInProgress())
  {
    LOGERR_ONCE("daRg textarea: text inserted from inside an embed-size closure; ignored");
    return;
  }

  if (len <= 0)
    return;

  int maxChars = elem->props.getInt(elem->csk->maxChars, 0);
  int cap = maxChars - total_num_chars(fmtText);
  if (maxChars > 0 && cap <= 0)
    return; // editor full

  int at = ::clamp(insert_pos < 0 ? etext->cursorPos : insert_pos, 0, total_num_chars(fmtText));

  Tab<TextBlock *> newBlocks;
  etext->parsePastedText(newBlocks, text, len);
  for (TextBlock *block : newBlocks)
    block->calcNumChars();

  if (maxChars > 0)
    trim_blocks_to(fmtText, newBlocks, cap);

  int insertedLen = 0;
  bool insertedComponent = false;
  for (TextBlock *block : newBlocks)
  {
    insertedLen += block->numChars;
    insertedComponent = insertedComponent || block->type == TextBlock::TBT_COMPONENT;
  }

  if (newBlocks.empty())
    return;

  int relChar = -1;
  int curBlockIdx = find_block_right(fmtText, at, relChar);
  if (curBlockIdx >= 0 && relChar > 0 && fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_TEXT)
  {
    if (split_text_block(fmtText, curBlockIdx, relChar))
      ++curBlockIdx;
  }

  int blockInsertPos = (curBlockIdx >= 0) ? curBlockIdx : fmtText->blocks.size();
  fmtText->blocks.insert(fmtText->blocks.begin() + blockInsertPos, newBlocks.begin(), newBlocks.end());
  merge_text_blocks(fmtText);

  etext->cursorPos = at + insertedLen;

  fmtText->invalidateShapes();

  // inserted chips need child Elements built at the deferred rebuild (like setText)
  if (insertedComponent)
    GuiScene::get_from_elem(elem)->invalidateElement(elem);

  recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

  scroll_cursor_into_view(elem, etext);
  call_change_script_handler(elem, etext, true);
}


int BhvTextAreaEdit::kbdEvent(ElementTree *etree, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t wc, int accum_res)
{
  using namespace HumanInput;

  G_UNUSED(etree);

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
            fmtText->invalidateShapes();

            if (curBlock->text.empty())
            {
              fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
              fmtText->freeTextBlock(curBlock);
            }
          }
        }
        else
        {
          bool wasComponent = curBlock->type == TextBlock::TBT_COMPONENT;
          fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
          fmtText->freeTextBlock(curBlock);
          fmtText->invalidateShapes();
          --etext->cursorPos;

          merge_text_blocks(fmtText);

          if (wasComponent)
            GuiScene::get_from_elem(elem)->invalidateElement(elem);
        }

        recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

        scroll_cursor_into_view(elem, etext);
        call_change_script_handler(elem, etext, true);

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
          fmtText->invalidateShapes();

          if (curBlock->text.empty())
          {
            fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
            fmtText->freeTextBlock(curBlock);

            merge_text_blocks(fmtText);
          }
        }
        else
        {
          bool wasComponent = curBlock->type == TextBlock::TBT_COMPONENT;
          fmtText->blocks.erase(fmtText->blocks.begin() + curBlockIdx);
          fmtText->freeTextBlock(curBlock);
          fmtText->invalidateShapes();

          merge_text_blocks(fmtText);

          if (wasComponent)
            GuiScene::get_from_elem(elem)->invalidateElement(elem);
        }

        recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

        scroll_cursor_into_view(elem, etext);
        call_change_script_handler(elem, etext, true);

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
      int numCharsTotal = total_num_chars(fmtText);

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
          else if (fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_LINE_BREAK ||
                   fmtText->blocks[curBlockIdx]->type == TextBlock::TBT_COMPONENT)
            etext->cursorPos += 1; // atomic one-char block
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
    else if (key_idx == DKEY_RETURN || key_idx == DKEY_NUMPADENTER)
    {
      Sqrat::Function onReturn = elem->props.scriptDesc.GetFunction(elem->csk->onReturn);
      if (!onReturn.IsNull())
      {
        if (!repeat)
          elem->etree->guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onReturn));
        return R_PROCESSED;
      }

      int maxChars = elem->props.getInt(elem->csk->maxChars, 0);
      if (maxChars > 0 && total_num_chars(fmtText) >= maxChars)
        return R_PROCESSED; // editor full

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

      fmtText->invalidateShapes();

      recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext, true);
      return R_PROCESSED;
    }
    else if (isCtrlPressed && key_idx == DKEY_V)
    {
      char buf[256];
      if (clipboard::get_clipboard_utf8_text(buf, sizeof(buf)))
        insert_text(elem, etext, buf, strlen(buf)); // full editor inserts nothing, but still consume the key
      etext->pressedCharButtons.insert(key_idx);    // Ctrl held yields no wc; consume the release by key_idx
      return R_PROCESSED;
    }
    else if (isCtrlPressed && key_idx == DKEY_C)
    {
      eastl::string text;
      fmtText->join(text);
      clipboard::set_clipboard_utf8_text(text.c_str());
      etext->pressedCharButtons.insert(key_idx); // Ctrl held yields no wc; consume the release by key_idx
      return R_PROCESSED;
    }
    else if (wc)
    {
      int maxChars = elem->props.getInt(elem->csk->maxChars, 0);
      if (maxChars > 0)
      {
        if (total_num_chars(fmtText) >= maxChars)
        {
          etext->pressedCharButtons.insert(key_idx);
          return R_PROCESSED;
        }
      }

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
      // release carries key_idx but wc==0, so consume the matching release by key_idx
      etext->pressedCharButtons.insert(key_idx);

      fmtText->invalidateShapes();

      recalc_content(elem, /*axis*/ 0, elem->screenCoord.size, elem->screenCoord.contentSize);

      scroll_cursor_into_view(elem, etext);
      call_change_script_handler(elem, etext, true);
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    // prevent propagation to other elements and behaviors
    if (key_idx == DKEY_BACK || key_idx == DKEY_DELETE || key_idx == DKEY_LEFT || key_idx == DKEY_RIGHT || key_idx == DKEY_HOME ||
        key_idx == DKEY_END || key_idx == DKEY_UP || key_idx == DKEY_DOWN || key_idx == DKEY_RETURN || key_idx == DKEY_NUMPADENTER)
    {
      return R_PROCESSED;
    }
    else if (etext->pressedCharButtons.find(key_idx) != etext->pressedCharButtons.end())
    {
      etext->pressedCharButtons.erase(key_idx);
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
      fill_textarea_format_params(elem, elem->screenCoord.size, fmtParams);

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
  else if (block->type == TextBlock::TBT_SPACE || block->type == TextBlock::TBT_COMPONENT)
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
    fill_textarea_format_params(elem, elem->screenCoord.size, fmtParams);

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


void BhvTextAreaEdit::call_change_script_handler(Element *elem, EditableText *etext, bool content_changed)
{
  if (content_changed && (elem->layout.size[0].mode == SizeSpec::CONTENT || elem->layout.size[1].mode == SizeSpec::CONTENT))
    elem->recalcLayout();

  Sqrat::Function onChange = elem->props.scriptDesc.GetFunction(elem->csk->onChange);
  if (!onChange.IsNull())
  {
    Sqrat::Object etextRef(etext, onChange.GetVM());
    auto handler = new ScriptHandlerSqFunc<Sqrat::Object>(onChange, etextRef);
    elem->etree->guiScene->queueScriptHandler(handler);
  }
}

#if _TARGET_HAS_IME

// Raw codepoint length of a block as join() emits it: a chip is its full <tag/> marker,
// while the editor counts it as one logical char. The IME unit bridging below relies on this.
static int block_raw_chars(const TextBlock *b)
{
  switch (b->type)
  {
    case TextBlock::TBT_TEXT:
    case TextBlock::TBT_COMPONENT: return utf8_strlen(b->text.c_str());
    case TextBlock::TBT_SPACE:
    case TextBlock::TBT_LINE_BREAK: return 1; // join() emits a single ' ' / '\n'
    default: return 0;
  }
}


static int logical_to_raw_pos(const FormattedText *ft, int logical_pos)
{
  int raw = 0, logical = 0;
  for (const TextBlock *b : ft->blocks)
  {
    if (logical >= logical_pos)
      break;
    if (logical + b->numChars <= logical_pos)
    {
      raw += block_raw_chars(b);
      logical += b->numChars;
    }
    else // partial: only TBT_TEXT splits, where 1 logical char == 1 codepoint
    {
      raw += logical_pos - logical;
      break;
    }
  }
  return raw;
}


// Map a raw codepoint offset in the IME-edited string to a logical position. Re-parsing the
// prefix is exact even for markup the user typed by hand, where a block walk would drift:
// tags are consumed and uncounted, a chip marker collapses to one char.
static int raw_to_logical_pos(EditableText *etext, const char *str, int raw_pos)
{
  if (raw_pos <= 0)
    return 0;
  int totalBytes = (int)strlen(str);
  raw_pos = min(raw_pos, utf8_strlen(str));
  int prefixBytes = utf_calc_bytes_for_symbols(str, totalBytes, raw_pos);

  Tab<TextBlock *> prefixBlocks(framemem_ptr());
  etext->parsePastedText(prefixBlocks, str, prefixBytes);
  int logical = 0;
  for (TextBlock *b : prefixBlocks)
  {
    b->calcNumChars();
    logical += b->numChars;
  }
  for (TextBlock *b : prefixBlocks) // also unregisters any prefix chips from embeddedComps
    etext->fmtText.freeTextBlock(b);
  return logical;
}


void BhvTextAreaEdit::on_ime_finish(void *ud, const char *str, int cursor, int status)
{
  if (status < 0)
    return;

  bool applied = (status == HumanInput::IME_STATUS_UPDATED || status == HumanInput::IME_STATUS_CLOSED);

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
      // map before setText() re-parses: raw_to_logical_pos relies on the current tag mode
      int logicalCursor = (cursor >= 0) ? raw_to_logical_pos(etext, str, cursor) : -1;
      etext->setText(str, -1);
      // prefix re-parse can overcount past the parsed text, so re-clamp after setText
      if (logicalCursor >= 0)
        etext->cursorPos = min(logicalCursor, total_num_chars(&etext->fmtText));
      scroll_cursor_into_view(elem, etext);
    }
  }

  if (status == HumanInput::IME_STATUS_CLOSED)
  {
    close_ime(elem);
    Sqrat::Object cbFunc = elem->props.scriptDesc.RawGetSlot("onImeFinish");
    if (!cbFunc.IsNull())
    {
      Sqrat::Function f(cbFunc.GetVM(), Sqrat::Object(cbFunc.GetVM()), cbFunc);
      GuiScene::get_from_elem(elem)->queueScriptHandler(new ScriptHandlerSqFunc<bool>(f, applied));
    }
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
      params.setStr("title", title.GetVar<const char *>().value);
    Sqrat::Object hint = elem->props.getObject(elem->csk->hint);
    if (hint.GetType() == OT_STRING)
      params.setStr("hint", hint.GetVar<const char *>().value);

    params.setStr("str", text.c_str());

    // maxChars/cursorPos are logical but the string is raw; convert to raw units so the
    // platform never truncates a marker or drops the caret inside one.
    int maxChars = elem->props.getInt(elem->csk->maxChars, 2048);
    int rawTotal = 0;
    for (const TextBlock *b : fmtText->blocks)
      rawTotal += block_raw_chars(b);
    int rawExtra = rawTotal - total_num_chars(fmtText);
    params.setInt("maxChars", maxChars + rawExtra);

    if (elem->props.getBool(elem->csk->imeNoAutoCap, false))
      params.setBool("optNoAutoCap", true);
    if (elem->props.getBool(elem->csk->imeNoCopy, false))
      params.setBool("optNoCopy", true);

    Sqrat::Object inputType = elem->props.getObject(elem->csk->inputType);
    if (inputType.GetType() == OT_STRING)
      params.setStr("type", inputType.GetVar<const char *>().value);
    else if (inputType.GetType() != OT_NULL)
      darg_assert_trace_var("inputType must be string", elem->props.scriptDesc, elem->csk->inputType);

    params.setInt("optCursorPos", logical_to_raw_pos(fmtText, etext->cursorPos));

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
