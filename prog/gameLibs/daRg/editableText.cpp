// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "editableText.h"
#include "behaviors/bhvTextAreaEdit.h"

#include <daRg/dag_element.h>

namespace darg
{

using namespace textlayout;


static const int editor_preformatted_flags = textlayout::FMT_KEEP_SPACES | textlayout::FMT_IGNORE_TAGS | textlayout::FMT_HIDE_ELLIPSIS;


struct DefaultTextParams : public textlayout::ITextParams
{
  virtual bool getColor(const char * /*color_name*/, E3DCOLOR &res, String &err_msg) override
  {
    err_msg = "Named colors are not supported";
    res = E3DCOLOR(255, 255, 255, 255);
    return false;
  }

  virtual void getUserTags(Tab<String> &tags) override
  {
    G_ASSERT(0); // tags are disabled
    tags.clear();
  }

  virtual bool getTagAttr(const char * /*tag*/, textlayout::TagAttributes & /*attr*/) override
  {
    G_ASSERT(0); // tags are disabled
    return false;
  }
};


SQInteger EditableText::script_ctor(HSQUIRRELVM vm)
{
  EditableText *self = new EditableText();
  self->fmtText.preformattedFlags = editor_preformatted_flags;

  const char *text = nullptr;
  SQInteger textLen = 0;
  sq_getstringandsize(vm, 2, &text, &textLen);

  DefaultTextParams tp;
  self->fmtText.updateText(text, textLen, &tp);

  for (textlayout::TextBlock *block : self->fmtText.blocks)
    block->calcNumChars();

  Sqrat::ClassType<EditableText>::SetManagedInstance(vm, 1, self);
  return 0;
}


SQInteger EditableText::get_text(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EditableText *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EditableText *> self(vm, 1);

  String text;
  for (TextBlock *block : self.value->fmtText.blocks)
  {
    switch (block->type)
    {
      case TextBlock::TBT_TEXT: text.append(block->text.c_str(), block->text.length()); break;
      case TextBlock::TBT_SPACE: text.append(" "); break;
      case TextBlock::TBT_LINE_BREAK: text.append("\n"); break;
      default: G_ASSERTF(0, "Unexpected block type %d", block->type);
    }
  }

  sq_pushstring(vm, text.c_str(), text.size());
  return 1;
}


SQInteger EditableText::set_text(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EditableText *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EditableText *> self(vm, 1);

  const char *text = nullptr;
  SQInteger textLen = 0;
  sq_getstringandsize(vm, 2, &text, &textLen);

  self.value->setText(text, textLen);

  return 0;
}


void EditableText::setText(const char *text, int textLen)
{
  DefaultTextParams tp;
  fmtText.updateText(text, textLen, &tp);

  int numCharsTotal = 0;
  for (textlayout::TextBlock *block : fmtText.blocks)
  {
    block->calcNumChars();
    numCharsTotal += block->numChars;
  }

  cursorPos = ::clamp(cursorPos, 0, numCharsTotal);

  if (boundElem)
  {
    auto &behaviors = boundElem->behaviors;
    bool hasEditBhv = eastl::find(behaviors.begin(), behaviors.end(), &bhv_text_area_edit) != behaviors.end();
    G_ASSERT(hasEditBhv);
    if (hasEditBhv)
    {
      Point2 outSize;
      BhvTextAreaEdit::recalc_content(boundElem, /*axis*/ 0, boundElem->screenCoord.size, outSize);
      // scroll_cursor_into_view(elem, etext);
      BhvTextAreaEdit::call_change_script_handler(boundElem, this);
    }
  }
}


void EditableText::bind_script(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();
  Sqrat::Class<EditableText, Sqrat::NoCopy<EditableText>> sqEditableText(vm, "EditableText");
  sqEditableText.SquirrelCtor(script_ctor, 2, ".s").SquirrelProp("text", &get_text, &set_text);

  exports.Bind("EditableText", sqEditableText);
}


} // namespace darg
