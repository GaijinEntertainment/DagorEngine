// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTextArea.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include "guiScene.h"

#include "textLayout.h"
#include "textUtil.h"
#include "dargDebugUtils.h"
#include "profiler.h"
#include <perfMon/dag_statDrv.h>


namespace darg
{


struct TextAreaTextParams : public textlayout::ITextParams
{
  Sqrat::Table colorTable, tagsTable, embedTable;

  TextAreaTextParams(const Sqrat::Table &color_table, const Sqrat::Table &tags_table, const Sqrat::Table &embed_table) :
    colorTable(color_table), tagsTable(tags_table), embedTable(embed_table)
  {}

  virtual bool getColor(const char *color_name, E3DCOLOR &res, String &err_msg) override
  {
    Sqrat::Object color = colorTable.RawGetSlot(color_name);
    SQObjectType type = color.GetType();

    if (type == OT_NULL)
    {
      err_msg.printf(0, "Named color '%s' not found in color table", color_name);
      res = E3DCOLOR(255, 255, 255, 255);
      return false;
    }

    if (type != OT_INTEGER)
    {
      err_msg.printf(0, "Non-color value in text area colorTable for '%s'", color_name);
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
      attr.color = E3DCOLOR((unsigned int &)cVal);
    }

    attr.fontId = tagData.RawGetSlotValue<int>("font", -1);
    attr.fontHt = (int)floorf(tagData.RawGetSlotValue<float>("fontSize", 0) + 0.5);

    return true;
  }
};


BhvTextArea bhv_text_area;


BhvTextArea::BhvTextArea() : Behavior(0, 0) {}


int BhvTextArea::get_preformatted_flags(const Element *elem, Sqrat::Object &preformatted)
{
  if (preformatted.IsNull())
    return 0;
  else if (preformatted.GetType() == OT_BOOL)
    return preformatted.Cast<bool>() ? textlayout::FMT_AS_IS : 0;
  else if (preformatted.GetType() == OT_INTEGER)
    return preformatted.Cast<int>();
  else
    darg_immediate_error(elem->props.scriptDesc.GetVM(), String("unexpected type of 'preformatted', must be null, bool, or int"));

  return 0;
}


void BhvTextArea::update_formatted_text(textlayout::FormattedText *fmt_text, Element *elem)
{
  Sqrat::Object preformatted = elem->props.scriptDesc.RawGetSlot(elem->csk->preformatted);
  fmt_text->preformattedFlags = get_preformatted_flags(elem, preformatted);

  Sqrat::Table colorTable = elem->props.scriptDesc.RawGetSlot(elem->csk->colorTable);
  Sqrat::Table tagsTable = elem->props.scriptDesc.RawGetSlot(elem->csk->tagsTable);
  Sqrat::Table embedTable = elem->props.scriptDesc.RawGetSlot(elem->csk->embed);
  TextAreaTextParams tp(colorTable, tagsTable, embedTable);
  fmt_text->updateText(elem->props.text.c_str(), elem->props.text.length(), &tp);
}


void BhvTextArea::onElemSetup(Element *elem, SetupMode setup_mode)
{
  if (setup_mode == SM_INITIAL || setup_mode == SM_REBUILD_UPDATE)
  {
    textlayout::FormattedText *fmtText =
      elem->props.storage.RawGetSlotValue<textlayout::FormattedText *>(elem->csk->formattedText, nullptr);
    if (!fmtText)
    {
      fmtText = new textlayout::FormattedText();
      elem->props.storage.SetInstance(elem->csk->formattedText, fmtText);
      // ^ will be deleted in onDetach()/onDelete()
    }

    update_formatted_text(fmtText, elem);
  }

  elem->updFlags(Element::F_CLIP_CHILDREN, true);
}


void BhvTextArea::deleteData(Element *elem)
{
  textlayout::FormattedText *fmtText =
    elem->props.storage.RawGetSlotValue<textlayout::FormattedText *>(elem->csk->formattedText, nullptr);
  if (fmtText)
  {
    delete fmtText;
    elem->props.storage.DeleteSlot(elem->csk->formattedText);
  }
}


void BhvTextArea::onAttach(Element *)
{
  // Initialization is done in onElemSetup(elem, setup_mode=SM_INITIAL)
  // This is because of need to have a valid FormattedText in contributeChildren()
  // and onAttach() is called after both onElemSetup() and contributeChildren()
}


void BhvTextArea::onDetach(Element *elem, DetachMode dmode)
{
  if (dmode == DETACH_REBUILD)
  {
    // no onDelete will follow for this behavior in case of rebuild
    deleteData(elem);
  }
  // on final detach keep data until object deletion
}


void BhvTextArea::onDelete(Element *elem) { deleteData(elem); }


void BhvTextArea::contributeChildren(Element *elem, dag::Vector<Sqrat::Object, framemem_allocator> &children)
{
  using namespace textlayout;

  FormattedText *fmtText = elem->props.storage.RawGetSlotValue<FormattedText *>(elem->csk->formattedText, nullptr);
  if (!fmtText)
    return;

  for (auto &p : fmtText->embeddedComps)
    children.push_back(p.second);
}


void BhvTextArea::onRecalcLayout(Element *elem)
{
  TIME_PROFILE(bhv_textarea_onRecalcLayout);

  using namespace textlayout;
  FormattedText *fmtText = elem->props.storage.RawGetSlotValue<FormattedText *>(elem->csk->formattedText, nullptr);
  if (!fmtText)
    return;

  eastl::vector_set<Element *, eastl::less<Element *>, framemem_allocator> processedChildren;
  for (auto &p : fmtText->embeddedComps)
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
        // child->recalcScreenPositions();
        child->screenCoord.screenPos = elem->screenCoord.screenPos + child->screenCoord.relPos;
        processedChildren.insert(child);
      }
    }
  }
}


void BhvTextArea::recalc_content(const Element *elem, int /*axis*/, const Point2 &elem_size, Point2 &out_size)
{
  AutoProfileScope profile(GuiScene::get_from_elem(elem)->getProfiler(), M_RECALC_TEXTAREA);
  TIME_PROFILE(bhv_textarea_recalc_content);

  using namespace textlayout;

  out_size.zero();

  FormattedText *fmtText = elem->props.storage.RawGetSlotValue<FormattedText *>(elem->csk->formattedText, nullptr);
  if (!fmtText)
    return;

  const Properties &props = elem->props;

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

} // namespace darg
