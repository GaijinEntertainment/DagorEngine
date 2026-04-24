// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "textLayout.h"
#include "dargDebugUtils.h"

#include <daRg/dag_layout.h>

#include <generic/dag_tabUtils.h>
#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_localConv.h>
#include <utf8/utf8.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_unicode.h>

#define DEBUG_TEXTAREA 0

namespace textlayout
{

int TextBlock::toWChar(Tab<wchar_t> &wtext)
{
  if (type == TBT_SPACE)
  {
    wtext.resize(2);
    wtext[0] = ' ';
    wtext[1] = 0;
    return 1;
  }

  if (type == TBT_TEXT)
  {
    wtext.resize(text.length() + 2);
    int wlen = utf8_to_wcs_ex(text.c_str(), text.length(), wtext.data(), wtext.size() - 1);
    wtext.resize(wlen + 1);
    return wlen;
  }

  G_ASSERT(0);
  wtext.clear();
  return 0;
}


void TextBlock::calcNumChars()
{
  if (type == TBT_SPACE)
    numChars = 1;
  else if (type == TBT_TEXT)
    numChars = utf8_strlen(text.c_str());
  else if (type == TBT_LINE_BREAK)
    numChars = 1;
  else
  {
    G_ASSERT(0);
    numChars = 0;
  }
}


FormattedText::FormattedText()
{
  textBlockAllocator.init(sizeof(TextBlock), 32);
  lastFormatParamsForCurText.reset();

  yOffset = 0.f;
  preformattedFlags = FMT_AS_IS;
}


FormattedText::~FormattedText() { clear(); }


void FormattedText::clear()
{
  lines.clear();
  embeddedComps.clear();
  for (TextBlock *block : blocks)
    freeTextBlock(block);
  blocks.clear();
  yOffset = 0.f;
  lastFormatParamsForCurText.reset();
  formatErrorMsg.clear();
}


TextBlock *FormattedText::allocateTextBlock()
{
  TextBlock *block = (TextBlock *)textBlockAllocator.allocateOneBlock();
  new (block, _NEW_INPLACE) TextBlock();
  return block;
}


void FormattedText::freeTextBlock(TextBlock *block)
{
  G_ASSERT(block);
  if (block->type == TextBlock::TBT_COMPONENT)
  {
    for (auto it = embeddedComps.begin(); it != embeddedComps.end(); ++it)
    {
      if (it->first == block)
      {
        embeddedComps.erase(it);
        break;
      }
    }
  }
  block->~TextBlock();
  textBlockAllocator.freeOneBlock(block);
}


void FormattedText::setEmbeddedComp(TextBlock *block, Sqrat::Object &&comp)
{
  embeddedComps.push_back(eastl::make_pair(block, eastl::move(comp)));
}


Sqrat::Object *FormattedText::getEmbeddedComp(const TextBlock *block)
{
  for (auto &p : embeddedComps)
    if (p.first == block)
      return &p.second;
  return nullptr;
}


const Sqrat::Object *FormattedText::getEmbeddedComp(const TextBlock *block) const
{
  for (const auto &p : embeddedComps)
    if (p.first == block)
      return &p.second;
  return nullptr;
}


void FormattedText::updateText(const char *text, int len, ITextParams *tp)
{
  clear();
  appendText(text, len, tp);
}


void FormattedText::appendText(const char *text, int len, ITextParams *tp)
{
  parseAndSplitText(blocks, text, len, tp);
  lastFormatParamsForCurText.reset();
}


void FormattedText::invalidateShapes()
{
  for (TextBlock *block : blocks)
    block->hasValidShape = false;
  lastFormatParamsForCurText.reset();
}


bool is_space(char chr) { return (chr == ' ') || (chr == '\t'); }


E3DCOLOR FormattedText::strToColor(const char *str, int &out_length, bool &out_error)
{
  out_error = false;
  const char *finalSymbolPtr = strchr(str, '>');
  if (!finalSymbolPtr)
  {
    out_length = 0;
    out_error = true;
    return E3DCOLOR(0, 0, 0, 255);
  }

  out_length = finalSymbolPtr - str;

  if (out_length == 8)
  {
    char *endPtr = nullptr;
    E3DCOLOR res = E3DCOLOR((unsigned int)strtoul(str, &endPtr, 16));
    out_error = (endPtr != finalSymbolPtr);
    return res;
  }
  else if (out_length == 6)
  {
    char *endPtr = nullptr;
    E3DCOLOR res = E3DCOLOR(((unsigned int)strtoul(str, &endPtr, 16)) | 0xFF000000U);
    out_error = (endPtr != finalSymbolPtr);
    return res;
  }
  else
  {
    out_error = true;
    return E3DCOLOR(0, 0, 0, 255);
  }
}

#define STARTS_WITH(s, substr) (dd_strnicmp((s), (substr), int(sizeof(substr)) - 1) == 0)

static const char invalid_utf8[] = "INVALID UTF8";

static const String strColor("color");


static int check_endline(const char *s, bool useTags)
{
  if (useTags && STARTS_WITH(s, "<br>"))
    return 4;

  if (useTags && STARTS_WITH(s, "<br/>"))
    return 5;

  if (s[0] == '\n')
    return 1;

  if (s[0] == '\r' && s[1] == '\n')
    return 2;

  if (useTags && s[0] == '\\' && s[1] == 'n')
    return 2;

  return 0;
}


void FormattedText::parseAndSplitText(Tab<TextBlock *> &output, const char *str, int strLen, ITextParams *tp)
{
  const bool useTags = (preformattedFlags & FMT_IGNORE_TAGS) == 0 && tp != nullptr;
  const bool keepSpaces = (preformattedFlags & FMT_KEEP_SPACES) != 0;

  if (strLen < 0)
    strLen = (int)strlen(str);

  if (!utf8::is_valid(str, str + strLen))
  {
    logerr("Invalid UTF8 string '%s'", str);
    str = invalid_utf8;
    strLen = sizeof(invalid_utf8) - 1;
  }

  Tab<String> tagsStack(framemem_ptr());

  TextBlock::Type curBlockType = TextBlock::TBT_NONE;

  String blockStr(framemem_ptr());
  blockStr.reserve(256);
  String errMsg(framemem_ptr());

  struct CharAttributes
  {
    E3DCOLOR fontColor = E3DCOLOR(0, 0, 0, 255);
    short fontId = -1, fontHt = 0;
    bool isDefaultColor = true;
  };

  CharAttributes nextBlockAttr;
  CharAttributes lastBlockAttr;
  Tab<CharAttributes> attrStack(framemem_ptr());

  Tab<String> customTags(framemem_ptr());
  if (useTags)
    tp->getUserTags(customTags);

  Sqrat::Object embeddedComponent;

  for (int i = 0; i <= strLen;)
  {
    bool endBlockByTag = false;

    if (useTags)
    {
      while (i < strLen && str[i] == '<') // handle text modifiers
      {
        bool gotTag = false;

        if (STARTS_WITH(str + i, "<color=#"))
        {
          attrStack.push_back(nextBlockAttr);
          tagsStack.push_back(strColor);
          int length = 0;
          bool isError = false;
          nextBlockAttr.fontColor = strToColor(str + i + 8, length, isError);

          if (isError)
            logerr("parseAndSplitText: syntax error in color constant at pos=%i string='%s'", i, str);

          nextBlockAttr.isDefaultColor = isError;
          gotTag = true;
          i += length + 1 + 8;
        }
        else if (STARTS_WITH(str + i, "<color=@"))
        {
          attrStack.push_back(nextBlockAttr);
          tagsStack.push_back(strColor);

          char constName[128];
          int constLen = 0;
          bool hasClosingBracket = false;
          while (constLen < sizeof(constName) - 1 && i + constLen + 8 < strLen)
          {
            const char chr = str[i + constLen + 8];
            if (chr == '>')
            {
              hasClosingBracket = true;
              break;
            }
            constName[constLen++] = chr;
          }
          constName[constLen] = '\0';

          if (constName[0] != '\0')
          {
            nextBlockAttr.isDefaultColor = false;
            if (!tp->getColor(constName, nextBlockAttr.fontColor, errMsg))
            {
              nextBlockAttr.isDefaultColor = true;
              logerr("'%s' at pos=%i string='%s'", errMsg.c_str(), i, str);
            }
          }
          else
            logerr("parseAndSplitText: syntax error in color constant name string='%s'", str);

          gotTag = true;
          i += 8 + constLen + (hasClosingBracket ? 1 : 0);
        }
        else if (STARTS_WITH(str + i, "<color="))
        {
          attrStack.push_back(nextBlockAttr);
          tagsStack.push_back(strColor);

          bool isError = false;
          const char *startPtr = str + i + 7;
          char *endPtr = nullptr;
          const char *endOfIntPtr = strchr(startPtr, '>');
          if (!endOfIntPtr)
          {
            isError = true;
            i = strLen;
          }
          else
          {
            long long color = strtoll(startPtr, &endPtr, 10);
            if (endPtr != endOfIntPtr || color > (long long)(UINT_MAX) || color < INT_MIN)
            {
              isError = true;
              if (!color)
                color = E3DCOLOR(255, 255, 255, 255);
            }

            nextBlockAttr.fontColor = uint32_t(color);
            int length = int(endOfIntPtr - startPtr);
            i += length + 1 + 7;
          }

          if (isError)
            logerr("parseAndSplitText: syntax error in color constant at pos=%i string='%s'", i, str);

          nextBlockAttr.isDefaultColor = isError;
          gotTag = true;
        }
        else if (STARTS_WITH(str + i, "</color>"))
        {
          bool isValidTag = (!tagsStack.empty() && tagsStack.back() == strColor);
          if (isValidTag)
          {
            tagsStack.pop_back();
            if (attrStack.empty())
              logerr("parseAndSplitText: invalid tag </color> at pos=%d string='%s'", i, str);
            else
            {
              nextBlockAttr = attrStack.back();
              attrStack.pop_back();
            }

            gotTag = true;
            i += 8;
          }
        }
        else
        {
          for (const String &tagName : customTags)
          {
            if (str[i + 1] == '/' && dd_strnicmp(str + i + 2, tagName, tagName.length()) == 0 && str[i + 2 + tagName.length()] == '>')
            {
              // closing tag
              bool isValidTag = (!tagsStack.empty() && tagName == tagsStack.back());
              if (isValidTag)
              {
                gotTag = true;

                i += 3 + tagName.length();

                tagsStack.pop_back();

                if (attrStack.empty())
                  G_ASSERT(!"empty attrStack");
                else
                {
                  nextBlockAttr = attrStack.back();
                  attrStack.pop_back();
                }
              }
              else
                logerr("parseAndSplitText: mismatched closing tag %s at pos=%d string='%s'", tagName.c_str(), i, str);

              break;
            }
            else if (dd_strnicmp(str + i + 1, tagName, tagName.length()) == 0 && str[i + 1 + tagName.length()] == '>')
            {
              // opening tag
              gotTag = true;

              i += 2 + tagName.length();

              TagAttributes attr;

              tagsStack.push_back(tagName);
              attrStack.push_back(nextBlockAttr);
              if (tp->getTagAttr(tagName, attr))
              {
                if (attr.colorIsValid)
                {
                  nextBlockAttr.fontColor = attr.color;
                  nextBlockAttr.isDefaultColor = false;
                }
                if (attr.fontId >= 0)
                {
                  nextBlockAttr.fontId = attr.fontId;
                  nextBlockAttr.fontHt = attr.fontHt;
                }
                else if (attr.fontHt != 0)
                  nextBlockAttr.fontHt = attr.fontHt;
              }
              else
                logerr("parseAndSplitText: getTagAttr(%s) failed", tagName.c_str());

              break;
            }
          }

          if (!gotTag)
          {
            // Try embedded components
            // Find tag closing (limited length)
            int embedEnd = -1;
            const int maxEmbedNameLen = 32;
            for (int t = i + 2; embedEnd < 0 && t < strLen - 1 && t < i + 1 + maxEmbedNameLen; ++t)
              if (str[t] == '/' && str[t + 1] == '>')
                embedEnd = t;

            if (embedEnd > 0)
            {
              if (tp->getEmbeddedComponent(String(str + i + 1, embedEnd - i - 1), embeddedComponent))
                gotTag = true;
              i = embedEnd + 2;
            }
          }
        }

        if (gotTag)
          endBlockByTag = true;
        else
          break;
      } // handle text modificators
    }

    const int endLineLen = check_endline(str + i, useTags);
    const bool isEndLine = (endLineLen > 0);

    const bool isSpace = is_space(str[i]);

    TextBlock::Type newBlockType = isSpace ? TextBlock::TBT_SPACE : TextBlock::TBT_TEXT;

    const bool endOfText = (i >= strLen) || !str[i];

    bool endBlock = endBlockByTag || isEndLine || (newBlockType != curBlockType) || endOfText ||
                    (keepSpaces && curBlockType == TextBlock::TBT_SPACE);

    if (endBlock && !blockStr.empty())
    {
      TextBlock *block = allocateTextBlock();

      blockStr.push_back('\0');
      block->text = blockStr.str();
      blockStr.clear();

      block->type = curBlockType;
      block->useCustomColor = !lastBlockAttr.isDefaultColor;
      block->customColor = lastBlockAttr.fontColor;
      block->fontId = lastBlockAttr.fontId;
      block->fontHt = lastBlockAttr.fontHt;

      output.push_back(block);
    }

    if (!embeddedComponent.IsNull())
    {
      TextBlock *block = allocateTextBlock();

      block->type = TextBlock::TBT_COMPONENT;
      setEmbeddedComp(block, eastl::move(embeddedComponent));

      output.push_back(block);

      embeddedComponent.Release();
    }

    if (isEndLine)
    {
      TextBlock *block = allocateTextBlock();

      block->type = TextBlock::TBT_LINE_BREAK;
      block->useCustomColor = !lastBlockAttr.isDefaultColor;
      block->customColor = lastBlockAttr.fontColor;
      block->fontId = lastBlockAttr.fontId;
      block->fontHt = lastBlockAttr.fontHt;

      output.push_back(block);
    }

    if (endOfText)
      break;

    if (endBlock)
    {
      curBlockType = newBlockType;
      lastBlockAttr = nextBlockAttr;
    }

    if (!isEndLine && str[i] != '\t')
      blockStr.push_back(str[i]);

    i += isEndLine ? endLineLen : 1;
  }

#if DEBUG_TEXTAREA
  String dbgText(str, strLen);
  debug("TextArea: parse: source text = [%s]", dbgText.str());
  debug("----- Text blocks (%d): -----", output.size());
  for (const TextBlock *block : output)
    debug("(%d) [%s]", block->type, block->text.c_str());
#endif
}


static bool should_skip_leading_space_block(const TextLine &line, const TextBlock *block, int preformatted_flags)
{
  const bool keepSpaces = (preformatted_flags & FMT_KEEP_SPACES) != 0;

  // skip leading spaces
  if (!keepSpaces && line.blocks.empty() && block->type == TextBlock::TBT_SPACE)
    return true;
  return false;
}


// Helper class to effectively reuse TextBlock vectors for lines and reduce memory reallocations
// This assumes that the text blocks most probably will be laid out the same way between format() calls
class LinesBuilder
{
public:
  LinesBuilder(Tab<TextLine> &lines_) : lines(lines_), originalSize(lines_.size()), curSize(0) {}

  void aquireNextLineBlocksVector(TextLine &dest_line)
  {
    if (curSize < originalSize)
    {
      dest_line.blocks = eastl::move(lines[curSize].blocks);
      dest_line.blocks.resize(0);
    }
  }

  void pushBack(TextLine &&cur_line)
  {
    if (curSize < originalSize)
      lines[curSize] = eastl::move(cur_line);
    else
      lines.push_back(eastl::move(cur_line));

    ++curSize;
  }

  void applySize() { lines.resize(curSize); }

private:
  int originalSize, curSize;
  Tab<TextLine> &lines;
};

Point2 FormattedText::calcEmbeddedComponentSize(const Sqrat::Object &comp, float def_height)
{
  using namespace darg;

  const Point2 defSize(def_height, def_height);
  HSQUIRRELVM vm = comp.GetVM();

  Sqrat::Table desc;
  if (comp.GetType() == OT_CLOSURE)
  {
    Sqrat::Function f(vm, Sqrat::Object(vm).GetObject(), comp.GetObject());
    Sqrat::Object result;
    if (!f.Execute(result))
    {
      formatErrorMsg = "Failed to build embedded component";
      return defSize;
    }
    desc = result;
  }
  else
    desc = comp;

  SizeSpec sizeSpec[2];
  const char *errMsg = nullptr;
  Sqrat::Object strSize("size", vm, 4);
  if (!Layout::read_size(desc.RawGetSlot(strSize), sizeSpec, &errMsg))
  {
    darg_assert_trace_var(String(0, "Error reading size: %s", errMsg), desc, strSize);
    return defSize;
  }

  Point2 resSize = defSize;
  for (int axis = 0; axis < 2; ++axis)
  {
    const SizeSpec &s = sizeSpec[axis];
    if (s.mode == SizeSpec::PIXELS)
      resSize[axis] = s.value;
    else if (s.mode == SizeSpec::FONT_H)
      resSize[axis] = ceilf(def_height * s.value * 0.01f);
    else
    {
      darg_assert_trace_var("For embedded components only absolute sizes (pixels, fontH) are allowed", desc, strSize);
      resSize[axis] = def_height;
    }
  }

  return resSize;
}


static void resolve_block_font(const TextBlock *block, const FormatParams &params, StdGuiFontContext &fontCtx, float &ascent,
  float &descent)
{
  fontCtx.font = StdGuiRender::get_font(block->fontId);
  if (!fontCtx.font)
  {
    logerr("Invalid fontId=%d in text block '%s'", block->fontId, block->text.c_str());
    fontCtx.font = StdGuiRender::get_font(params.defFontId);
  }
  if (!fontCtx.font)
    DAG_FATAL("Fonts are broken: block fontId=%d defFontId=%d", block->fontId, params.defFontId);
  fontCtx.fontHt = block->fontHt;

  ascent = StdGuiRender::get_font_ascent(fontCtx);
  descent = StdGuiRender::get_font_descent(fontCtx);
}


void FormattedText::shapeBlock(TextBlock *block, const StdGuiFontContext &fontCtx, float ascent, float descent)
{
  if (block->hasValidShape)
    return;

  block->size.y = ascent + descent;

  switch (block->type)
  {
    case TextBlock::TBT_TEXT:
    {
      BBox2 strBox = StdGuiRender::get_str_bbox(block->text.c_str(), block->text.length(), fontCtx);
      block->size.x = strBox.right();
      break;
    }
    case TextBlock::TBT_SPACE:
    {
      const char *spaces = "  ";
      BBox2 box1 = StdGuiRender::get_str_bbox(spaces, 2, fontCtx);
      BBox2 box2 = StdGuiRender::get_str_bbox(spaces, 1, fontCtx);
      block->size.x = box1.right() - box2.right();
      break;
    }
    case TextBlock::TBT_LINE_BREAK: //
      block->size.x = 0;
      break;
    case TextBlock::TBT_COMPONENT:
    {
      Sqrat::Object *comp = getEmbeddedComp(block);
      block->size = comp ? calcEmbeddedComponentSize(*comp, ascent) : Point2(0, 0);
      break;
    }
    default: //
      G_ASSERTF(0, "Invalid block type %d", block->type);
  }

  block->hasValidShape = true;
}


void FormattedText::format(const FormatParams &params)
{
  const bool lastResultIsStillValid = !hasFormatError() && lastFormatParamsForCurText.isSameCacheKey(params);

  if (lastResultIsStillValid)
    return;

  // Any change to a shape-affecting field invalidates every block's cached
  // block->size; wrap-only changes (maxWidth, spacing/indent) preserve it.
  const bool shapeInputsChanged =
    lastFormatParamsForCurText.defFontId != params.defFontId || lastFormatParamsForCurText.defFontHt != params.defFontHt ||
    lastFormatParamsForCurText.spacing != params.spacing || lastFormatParamsForCurText.monoWidth != params.monoWidth;
  if (shapeInputsChanged)
    for (TextBlock *block : blocks)
      block->hasValidShape = false;

  lastFormatParamsForCurText = params;

  formatErrorMsg.clear();

  if (!blocks.size())
  {
    lines.clear();
    return;
  }

  if (!StdGuiRender::get_font(0))
  {
    formatErrorMsg = "No fonts are loaded during formatting";
    lines.clear();
    return;
  }

  StdGuiFontContext fontCtx;
  fontCtx.spacing = params.spacing;
  fontCtx.monoW = params.monoWidth;

  const bool allowWrap = (preformattedFlags & FMT_NO_WRAP) == 0;

  // Form lines

  LinesBuilder linesBuilder(lines);

  TextLine curLine;
  float curMaxDescent = 0;
  linesBuilder.aquireNextLineBlocksVector(curLine);
  curLine.paragraphStart = true;
  if (lines.empty()) // first format call with no previous data
    curLine.blocks.reserve(::min(blocks.size(), 8u));

  for (int blockIdx = 0; blockIdx < blocks.size(); blockIdx++)
  {
    TextBlock *block = blocks[blockIdx];

    if (should_skip_leading_space_block(curLine, block, preformattedFlags))
      continue;

    block->indent = curLine.paragraphStart && curLine.blocks.empty() ? params.indent : 0;

    if (block->fontId < 0)
    {
      block->fontId = params.defFontId;
      if (!block->fontHt) //-V1051
        block->fontHt = params.defFontHt;
    }

    float ascent, descent;
    resolve_block_font(block, params, fontCtx, ascent, descent);
    shapeBlock(block, fontCtx, ascent, descent);

    if (allowWrap && params.maxWidth > 0 && curLine.contentWidth > 0 && (curLine.contentWidth + block->size.x > params.maxWidth) &&
        block->type != TextBlock::TBT_LINE_BREAK) // no duplicate line breaks (TODO: review logic)
    {
      linesBuilder.pushBack(eastl::move(curLine));
      curLine.reset();
      curMaxDescent = 0;
      linesBuilder.aquireNextLineBlocksVector(curLine);
      block->indent = params.hangingIndent;
    }

    if (should_skip_leading_space_block(curLine, block, preformattedFlags))
      continue;

    // add new line block to the _next_ line for consistent cursor positioning
    if (block->type == TextBlock::TBT_LINE_BREAK)
    {
      linesBuilder.pushBack(eastl::move(curLine));
      curLine.reset();
      curMaxDescent = 0;
      linesBuilder.aquireNextLineBlocksVector(curLine);
      curLine.paragraphStart = true;
    }

    curLine.blocks.push_back(block);
    curLine.contentWidth += block->indent + block->size.x;
    if (block->type == TextBlock::TBT_COMPONENT)
      curLine.baseLineY = max(curLine.baseLineY, block->size.y);
    else
    {
      curLine.baseLineY = max(curLine.baseLineY, ascent);
      curMaxDescent = max(curMaxDescent, descent);
    }
    curLine.contentHeight = curLine.baseLineY + curMaxDescent;
  }

  if (curLine.blocks.size())
    linesBuilder.pushBack(eastl::move(curLine));

  linesBuilder.applySize();

  // Trim trailing spaces

  if (!(preformattedFlags & FMT_KEEP_SPACES))
  {
    for (TextLine &line : lines)
    {
      while (line.blocks.size())
      {
        TextBlock *block = line.blocks.back();
        if (block->type != TextBlock::TBT_SPACE)
          break;
        line.blocks.pop_back();
        line.contentWidth -= (block->indent + block->size.x);
      }
    }
  }


  // Layout blocks

  float curY = 0;
  for (int iLine = 0, nLines = lines.size(); iLine < nLines; ++iLine)
  {
    TextLine &line = lines[iLine];
    if (iLine && line.paragraphStart)
      curY += params.parSpacing - params.lineSpacing;

    line.yPos = curY;

    float curX = 0;
    for (int iBlock = 0, nBlocks = line.blocks.size(); iBlock < nBlocks; ++iBlock)
    {
      TextBlock *block = line.blocks[iBlock];
      block->position = Point2(curX + block->indent, curY);
      if (block->type == TextBlock::TBT_COMPONENT)
        block->position.y += line.baseLineY - block->size.y;
      curX = block->position.x + block->size.x;
    }

    curY += line.contentHeight + params.lineSpacing;
  }

#if DEBUG_TEXTAREA
  debug("TextArea after format: %d lines", lines.size());
  for (int iLine = 0, nLines = lines.size(); iLine < nLines; ++iLine)
  {
    TextLine &line = lines[iLine];
    debug("Line %d/%d, %d blocks", iLine, nLines, line.blocks.size());
    for (const TextBlock *block : line.blocks)
      debug("(%d) [%s]", block->type, block->text.c_str());
  }
#endif
}


void FormattedText::join(eastl::string &dest) const
{
  dest.clear();
  for (const TextBlock *block : blocks)
  {
    switch (block->type)
    {
      case TextBlock::TBT_TEXT: dest += block->text; break;
      case TextBlock::TBT_SPACE: dest += ' '; break;
      case TextBlock::TBT_LINE_BREAK: dest += '\n'; break;
      default: G_ASSERTF(0, "Unexpected block type %d", block->type);
    }
  }
}


} // namespace textlayout
