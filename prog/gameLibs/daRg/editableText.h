// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "textLayout.h"

#include <sqrat.h>
#include <EASTL/vector_set.h>

namespace darg
{

class EditableText
{
public:
  textlayout::FormattedText fmtText;
  int cursorPos = 0;

  Element *boundElem = nullptr;

  eastl::vector_set<int> pressedCharButtons;

  static SQInteger script_ctor(HSQUIRRELVM vm);
  static SQInteger get_text(HSQUIRRELVM vm);
  static SQInteger set_text(HSQUIRRELVM vm);

  void setText(const char *text, int textLen = -1);

  static void bind_script(Sqrat::Table &exports);
};

} // namespace darg
