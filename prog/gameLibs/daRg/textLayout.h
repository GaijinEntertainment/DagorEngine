#pragma once

#include <util/dag_string.h>
#include <math/dag_e3dColor.h>
#include <math/dag_Point2.h>
#include <EASTL/vector.h>
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
    TBT_SPACE
  };

  Type type = TBT_NONE;
  String text;
  E3DCOLOR customColor = E3DCOLOR(255, 255, 255, 255);
  short fontId = -1, fontHt = 0;

  darg::GuiTextCache guiText; //< updated on render

  float indent = 0;
  Point2 size = Point2(0, 0);     //< only valid afer format() call
  Point2 position = Point2(0, 0); //< only valid afer format() call

  bool lineBreak = false;
  bool useCustomColor = false;
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
  int maxHeight = 0;

  void reset() { memset(this, 0, sizeof(*this)); }
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

  void format(const FormatParams &params);

  bool hasFormatError() const { return !formatErrorMsg.empty(); }

  int preformattedFlags;

private:
  void parseAndSplitText(const char *str, int strLen = -1, ITextParams *tp = nullptr);
  E3DCOLOR strToColor(const char *str, int &out_length, bool &out_error);

public:
  Tab<TextBlock *> blocks;
  Tab<TextLine> lines;
  float yOffset;
  FormatParams lastFormatParamsForCurText;

  FixedBlockAllocator textBlockAllocator;

  TextBlock *allocateTextBlock();
  void freeTextBlock(TextBlock *block);

  String formatErrorMsg;
};


} // namespace textlayout
