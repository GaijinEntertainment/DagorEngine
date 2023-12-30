#include "textLayout.h"

#include <generic/dag_tabUtils.h>
#include <gui/dag_stdGuiRender.h>
#include <osApiWrappers/dag_localConv.h>
#include <utf8/utf8.h>
#include <memory/dag_framemem.h>


namespace textlayout
{


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
  block->~TextBlock();
  textBlockAllocator.freeOneBlock(block);
}


void FormattedText::updateText(const char *text, int len, ITextParams *tp)
{
  clear();
  appendText(text, len, tp);
}


void FormattedText::appendText(const char *text, int len, ITextParams *tp)
{
  parseAndSplitText(text, len, tp);
  lastFormatParamsForCurText.reset();
}


static bool check_space(char chr) { return (chr == ' ') || (chr == '\t'); }


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


void FormattedText::parseAndSplitText(const char *str, int strLen, ITextParams *tp)
{
  const bool useTags = (preformattedFlags & FMT_IGNORE_TAGS) == 0;
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
  String blockStr(framemem_ptr());
  blockStr.reserve(256);
  String errMsg(framemem_ptr());

  struct CharAttributes
  {
    E3DCOLOR fontColor = E3DCOLOR(0, 0, 0, 255);
    short fontId = -1, fontHt = 0;
    bool isDefaultColor = true;
    bool isSpace = false;
  };

  CharAttributes nextBlockAttr;
  CharAttributes lastBlockAttr;
  Tab<CharAttributes> attrStack(framemem_ptr());

  Tab<String> customTags(framemem_ptr());
  if (useTags)
    tp->getUserTags(customTags);

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
          while (constLen < sizeof(constName) - 1)
          {
            const char chr = str[i + constLen + 8];
            if (!chr || chr == '>')
              break;
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
          i += 8 + constLen + 1;
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
            isError = true;
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
                logerr("parseAndSplitText: mismatched closing tag %s at pos=%d string='%s'", i, str);

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
        }

        if (gotTag)
          endBlockByTag = true;
        else
          break;
      } // handle text modificators
    }

    const int endLineLen = check_endline(str + i, useTags);
    const bool isEndLine = (endLineLen > 0);

    nextBlockAttr.isSpace = check_space(str[i]);

    const bool endOfText = (i >= strLen) || !str[i];

    bool endBlock = endBlockByTag || isEndLine || (lastBlockAttr.isSpace != nextBlockAttr.isSpace) || endOfText ||
                    (keepSpaces && lastBlockAttr.isSpace);

    if (endBlock && (!blockStr.empty() || isEndLine))
    {
      TextBlock *block = allocateTextBlock();

      blockStr.push_back('\0');
      block->text = blockStr.str();
      blockStr.clear();

      block->type = lastBlockAttr.isSpace ? TextBlock::TBT_SPACE : TextBlock::TBT_TEXT;
      block->lineBreak = isEndLine;
      block->useCustomColor = !lastBlockAttr.isDefaultColor;
      block->customColor = lastBlockAttr.fontColor;
      block->fontId = lastBlockAttr.fontId;
      block->fontHt = lastBlockAttr.fontHt;

      blocks.push_back(block);
    }

    if (endOfText)
      break;

    if (endBlock)
      lastBlockAttr = nextBlockAttr;

    if (!isEndLine && str[i] != '\t')
      blockStr.push_back(str[i]);

    i += isEndLine ? endLineLen : 1;
  }

  /*
    String dbgText(str, strLen);
    debug("Source text = [%s]", dbgText.str());
    debug("----- Text blocks (%d): -----", blocks.size());
    for (auto &block : blocks)
      debug("(%d) [%s] lb = %d lineBreak=%d", block->type, block->text.str(), block->lineBreak, block->lineBreak);
  */
}


static bool should_skip_leading_block(const TextLine &line, const TextBlock *block, int preformatted_flags)
{
  const bool keepSpaces = (preformatted_flags & FMT_KEEP_SPACES) != 0;

  // skip leading spaces
  if (!keepSpaces && line.blocks.empty() && block->type == TextBlock::TBT_SPACE && !block->lineBreak)
    return true;
  return false;
}


void FormattedText::format(const FormatParams &params)
{
  formatErrorMsg.clear();

  const bool lastResultIsStillValid =
    !hasFormatError() && memcmp(&lastFormatParamsForCurText, &params, sizeof(FormatParams)) == 0; //-V1014

  if (lastResultIsStillValid)
    return;

  lastFormatParamsForCurText = params;

  lines.clear();

  if (!blocks.size())
    return;

  if (!StdGuiRender::get_font(0))
  {
    formatErrorMsg = "No fonts are loaded during formatting";
    return;
  }

  StdGuiFontContext fontCtx;
  fontCtx.spacing = params.spacing;
  fontCtx.monoW = params.monoWidth;

  const bool allowWrap = (preformattedFlags & FMT_NO_WRAP) == 0;

  // Form lines

  TextLine curLine;
  curLine.paragraphStart = true;

  for (int blockIdx = 0; blockIdx < blocks.size(); blockIdx++)
  {
    TextBlock *block = blocks[blockIdx];

    if (should_skip_leading_block(curLine, block, preformattedFlags))
      continue;

    block->indent = curLine.paragraphStart && curLine.blocks.empty() ? params.indent : 0;

    if (block->fontId < 0)
    {
      block->fontId = params.defFontId;
      if (!block->fontHt) //-V1051
        block->fontHt = params.defFontHt;
    }

    fontCtx.font = StdGuiRender::get_font(block->fontId);
    if (!fontCtx.font)
    {
      logerr("Invalid fontId=%d in text block '%s'", block->fontId, block->text.c_str());
      fontCtx.font = StdGuiRender::get_font(params.defFontId);
    }
    if (!fontCtx.font)
      DAG_FATAL("Fonts are broken: block fontId=%d defFontId=%d", block->fontId, params.defFontId);
    fontCtx.fontHt = block->fontHt;

    if (block->type == TextBlock::TBT_TEXT)
    {
      BBox2 strBox = StdGuiRender::get_str_bbox(block->text.str(), block->text.length(), fontCtx);
      block->size.x = strBox.right();
    }
    else // TBT_SPACE
    {
      const char *spaces = "  ";
      BBox2 box1 = StdGuiRender::get_str_bbox(spaces, 2, fontCtx);
      BBox2 box2 = StdGuiRender::get_str_bbox(spaces, 1, fontCtx);
      block->size.x = box1.right() - box2.right();
    }

    float ascent = StdGuiRender::get_font_ascent(fontCtx);
    float descent = StdGuiRender::get_font_descent(fontCtx);
    block->size.y = ascent + descent;

    if (allowWrap && params.maxWidth > 0 && curLine.contentWidth > 0 && (curLine.contentWidth + block->size.x > params.maxWidth))
    {
      lines.push_back(eastl::move(curLine));
      curLine.reset();
      block->indent = params.hangingIndent;
    }

    if (should_skip_leading_block(curLine, block, preformattedFlags))
      continue;

    curLine.blocks.push_back(block);
    curLine.contentWidth += block->indent + block->size.x;
    curLine.contentHeight = max(curLine.contentHeight, block->size.y);
    curLine.baseLineY = max(curLine.baseLineY, ascent);

    if (block->lineBreak)
    {
      lines.push_back(eastl::move(curLine));

      curLine.reset();
      curLine.paragraphStart = true;
    }
  }

  if (curLine.blocks.size())
    lines.push_back(eastl::move(curLine));


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
      curX = block->position.x + block->size.x;
    }

    curY += line.contentHeight + params.lineSpacing;
  }
}


} // namespace textlayout
