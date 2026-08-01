// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "editableText.h"
#include "behaviors/bhvTextAreaEdit.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include "guiScene.h"

#include <debug/dag_log.h>

namespace darg
{

using namespace textlayout;


static const int editor_base_flags = textlayout::FMT_KEEP_SPACES | textlayout::FMT_HIDE_ELLIPSIS;
static const int editor_preformatted_flags = editor_base_flags | textlayout::FMT_IGNORE_TAGS;


static bool elem_allows_tags(const Element *elem)
{
  return elem && elem->props.scriptDesc.RawGetSlotValue<bool>(elem->csk->allowTags, false);
}


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
  virtual bool getEmbeddedComponent(const char * /*tag*/, Sqrat::Object & /*comp*/) override
  {
    G_ASSERT(0); // tags are disabled
    return false;
  }
};


struct EmbedTextParams : public textlayout::ITextParams
{
  Sqrat::Table colorTable, tagsTable, embedTable;

  EmbedTextParams(const Sqrat::Table &color_table, const Sqrat::Table &tags_table, const Sqrat::Table &embed_table) :
    colorTable(color_table), tagsTable(tags_table), embedTable(embed_table)
  {}

  virtual bool getColor(const char *color_name, E3DCOLOR &res, String &err_msg) override
  {
    Sqrat::Object color = colorTable.RawGetSlot(color_name);
    if (color.GetType() != OT_INTEGER)
    {
      err_msg.printf(0, "Named color '%s' not found in color table", color_name);
      res = E3DCOLOR(255, 255, 255, 255);
      return false;
    }
    err_msg.clear();
    res = E3DCOLOR((unsigned int)color.Cast<int>());
    return true;
  }

  virtual void getUserTags(Tab<String> &tags) override
  {
    Sqrat::Object::iterator it;
    while (tagsTable.Next(it))
      tags.push_back(String(it.getName()));
  }

  virtual bool getEmbeddedComponent(const char *tag, Sqrat::Object &comp) override
  {
    Sqrat::Object val = embedTable.RawGetSlot(tag);
    SQObjectType tp = val.GetType();
    if (tp == OT_TABLE || tp == OT_CLOSURE)
    {
      comp = val;
      return true;
    }
    return false;
  }

  virtual bool getTagAttr(const char *tag_name, textlayout::TagAttributes &attr) override
  {
    Sqrat::Table tagData = tagsTable.RawGetSlot(tag_name);
    if (tagData.GetType() != OT_TABLE)
      return false;
    Sqrat::Object colorObj = tagData.RawGetSlot("color");
    attr.colorIsValid = colorObj.GetType() == OT_INTEGER;
    if (attr.colorIsValid)
    {
      int cVal = colorObj.Cast<int>();
      attr.color = E3DCOLOR((unsigned int)cVal);
    }
    attr.fontId = tagData.RawGetSlotValue<int>("font", -1);
    attr.fontHt = (int)floorf(tagData.RawGetSlotValue<float>("fontSize", 0) + 0.5);
    return true;
  }
};


static EmbedTextParams make_embed_text_params(const Element *elem)
{
  const StringKeys *csk = elem->csk;
  Sqrat::Table sd = elem->props.scriptDesc;
  Sqrat::Table colorTable = sd.RawGetSlot(csk->colorTable);
  Sqrat::Table tagsTable = sd.RawGetSlot(csk->tagsTable);
  Sqrat::Table embedTable = sd.RawGetSlot(csk->embed);
  return EmbedTextParams(colorTable, tagsTable, embedTable);
}


void EditableText::reformatFrom(const char *text, int textLen)
{
  if (fmtText.isFormatInProgress())
  {
    LOGERR_ONCE("daRg textarea: text mutated from inside an embed-size closure; ignored");
    return;
  }

  const bool tags = elem_allows_tags(boundElem);
  fmtText.preformattedFlags = tags ? editor_base_flags : editor_preformatted_flags;

  if (tags)
  {
    EmbedTextParams tp = make_embed_text_params(boundElem);
    fmtText.updateText(text, textLen, &tp);
  }
  else
  {
    DefaultTextParams tp;
    fmtText.updateText(text, textLen, &tp);
  }

  for (textlayout::TextBlock *block : fmtText.blocks)
    block->calcNumChars();
}


void EditableText::parsePastedText(Tab<textlayout::TextBlock *> &out_blocks, const char *text, int len)
{
  // Whether tags are honored also depends on fmtText.preformattedFlags, set by reformatFrom.
  if (elem_allows_tags(boundElem))
  {
    EmbedTextParams tp = make_embed_text_params(boundElem);
    fmtText.parseAndSplitText(out_blocks, text, len, &tp);
  }
  else
  {
    DefaultTextParams tp;
    fmtText.parseAndSplitText(out_blocks, text, len, &tp);
  }
}


SQInteger EditableText::script_ctor(HSQUIRRELVM vm)
{
  EditableText *self = new EditableText();

  const char *text = nullptr;
  SQInteger textLen = 0;
  sq_getstringandsize(vm, 2, &text, &textLen);

  self->reformatFrom(text, textLen);

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
      case TextBlock::TBT_TEXT:
      case TextBlock::TBT_COMPONENT: text.append(block->text.c_str(), block->text.length()); break;
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
  if (!Sqrat::check_signature<EditableText *, const char *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EditableText *> self(vm, 1);

  const char *text = nullptr;
  SQInteger textLen = 0;
  sq_getstringandsize(vm, 2, &text, &textLen);

  self.value->setText(text, textLen);

  return 0;
}


SQInteger EditableText::insert_text(HSQUIRRELVM vm)
{
  if (!Sqrat::check_signature<EditableText *, const char *>(vm))
    return SQ_ERROR;

  Sqrat::Var<EditableText *> self(vm, 1);

  const char *text = nullptr;
  SQInteger textLen = 0;
  sq_getstringandsize(vm, 2, &text, &textLen);

  SQInteger pos = -1;
  if (sq_gettop(vm) >= 3)
    sq_getinteger(vm, 3, &pos);

  if (!self.value->boundElem)
    return sq_throwerror(vm, "An attached editor is required");

  BhvTextAreaEdit::insert_text(self.value->boundElem, self.value, text, textLen, (int)pos);

  return 0;
}


void EditableText::setText(const char *text, int textLen)
{
  reformatFrom(text, textLen);

  int numCharsTotal = 0;
  for (textlayout::TextBlock *block : fmtText.blocks)
    numCharsTotal += block->numChars;

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
      if (elem_allows_tags(boundElem))
        GuiScene::get_from_elem(boundElem)->invalidateElement(boundElem);
      BhvTextAreaEdit::call_change_script_handler(boundElem, this, /*content_changed*/ true);
    }
  }
}


void EditableText::bind_script(Sqrat::Table &exports)
{
  HSQUIRRELVM vm = exports.GetVM();
  Sqrat::Class<EditableText, Sqrat::NoCopy<EditableText>> sqEditableText(vm, "EditableText");
  sqEditableText //
    .SquirrelCtor(script_ctor, 2, ".s")
    .SquirrelProp("text", &get_text, &set_text)
    .SquirrelFuncDeclString(insert_text, "instance.insertText(text: string, [pos: int]): null")
    /**/;

  exports.Bind("EditableText", sqEditableText);
}


} // namespace darg
