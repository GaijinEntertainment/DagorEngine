// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_string.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/utility.h>
#include <generic/dag_tab.h>
#include <memory/dag_fixedBlockAllocator.h>
#include "stdRendObj.h"


namespace textlayout
{

struct TagColor;

typedef dag::Vector<TagColor> TagsList;


struct TextBlock
{
  enum Type
  {
    TBT_NONE,
    TBT_TEXT,
    TBT_SPACE,
    TBT_LINE_BREAK,
    TBT_COMPONENT
  };

  Type type = TBT_NONE;
  eastl::string text;
  E3DCOLOR customColor = E3DCOLOR(255, 255, 255, 255);
  short fontId = -1, fontHt = 0;

  darg::GuiTextCache guiText; //< updated on render

  float indent = 0;
  Point2 size = Point2(0, 0);     //< only valid afer format() call
  Point2 position = Point2(0, 0); //< only valid afer format() call

  // Shape cache. When true, block->size is still valid and FormattedText::format
  // skips the harfbuzz/get_str_bbox call for this block. Cleared in bulk by
  // format() when any shape-affecting FormatParams field changes, or by
  // FormattedText::invalidateShapes() on in-place text mutation. Parsed blocks
  // default to false.
  bool hasValidShape = false;

  bool useCustomColor = false;

  int numChars = -1; // Calculated only for editable textarea, otherwise -1

  int toWChar(Tab<wchar_t> &wtext);
  void calcNumChars();
};


struct TextLine
{
  Tab<TextBlock *> blocks;

  float contentWidth = 0;
  float contentHeight = 0;
  float yPos = 0;
  bool paragraphStart = false;
  float baseLineY = 0;

  void reset()
  {
    blocks.clear();
    contentWidth = 0;
    contentHeight = 0;
    yPos = 0;
    paragraphStart = false;
    baseLineY = 0;
  }
};


struct FormatParams
{
  short defFontId = 0, defFontHt = 0;
  int spacing = 0;
  int monoWidth = 0;
  float lineSpacing = 0;
  float parSpacing = 0;
  float indent = 0;
  float hangingIndent = 0;
  int maxWidth = 0;

  void reset() { memset(this, 0, sizeof(*this)); }

  bool isSameCacheKey(const FormatParams &fp) const
  {
    return maxWidth == fp.maxWidth && // most volatile
           defFontId == fp.defFontId && defFontHt == fp.defFontHt && spacing == fp.spacing && monoWidth == fp.monoWidth &&
           lineSpacing == fp.lineSpacing && parSpacing == fp.parSpacing && indent == fp.indent && hangingIndent == fp.hangingIndent;
  }
};


struct TagAttributes
{
  E3DCOLOR color = 0;
  short fontId = -1, fontHt = 0;
  bool colorIsValid = false;
};


struct ITextParams
{
  virtual bool getColor(const char *color_name, E3DCOLOR &res, String &err_msg) = 0;
  virtual void getUserTags(Tab<String> &tags) = 0;
  virtual bool getTagAttr(const char *tag, TagAttributes &attr) = 0;
  virtual bool getEmbeddedComponent(const char *tag, Sqrat::Object &comp) = 0;
};


enum
{
  FMT_NO_WRAP = 1 << 0,
  FMT_KEEP_SPACES = 1 << 1,
  FMT_IGNORE_TAGS = 1 << 2,
  FMT_HIDE_ELLIPSIS = 1 << 3,
  FMT_AS_IS = int(~0),
};

class FormattedText
{
public:
  FormattedText();
  ~FormattedText();

  void clear();
  void updateText(const char *text, int len = -1, ITextParams *tp = nullptr);
  void appendText(const char *text, int len = -1, ITextParams *tp = nullptr);
  void parseAndSplitText(Tab<TextBlock *> &output, const char *str, int strLen = -1, ITextParams *tp = nullptr);

  void format(const FormatParams &params);

  // Call when block text or font changes without going through parseAndSplitText
  // (e.g. in-place edits in bhvTextAreaEdit). Resets both the format params
  // cache and every block's per-block shape cache.
  void invalidateShapes();

  bool hasFormatError() const { return !formatErrorMsg.empty(); }

  // True when the previously formatted lines are still valid for a call that
  // would use this maxWidth. Safe because every other FormatParams input comes
  // from script properties; any change to those goes through updateText/clear
  // (empties lines) or invalidateShapes/reset (zeroes lastFormatParamsForCurText),
  // so lines being non-empty with a matching stored maxWidth implies a full match.
  bool canReuseLinesFor(int max_width) const
  {
    return !lines.empty() && !hasFormatError() && lastFormatParamsForCurText.maxWidth == max_width;
  }

  void join(eastl::string &dest) const;

  int preformattedFlags;

private:
  E3DCOLOR strToColor(const char *str, int &out_length, bool &out_error);
  Point2 calcEmbeddedComponentSize(const Sqrat::Object &desc, float def_height);
  void shapeBlock(TextBlock *block, const StdGuiFontContext &fontCtx, float ascent, float descent);

public:
  Tab<TextBlock *> blocks;
  Tab<TextLine> lines;
  float yOffset;
  FormatParams lastFormatParamsForCurText;

  FixedBlockAllocator textBlockAllocator;

  TextBlock *allocateTextBlock();
  void freeTextBlock(TextBlock *block);

  void setEmbeddedComp(TextBlock *block, Sqrat::Object &&comp);
  Sqrat::Object *getEmbeddedComp(const TextBlock *block);
  const Sqrat::Object *getEmbeddedComp(const TextBlock *block) const;

  eastl::vector<eastl::pair<TextBlock *, Sqrat::Object>> embeddedComps;

  String formatErrorMsg;
};

bool is_space(char c);

} // namespace textlayout
