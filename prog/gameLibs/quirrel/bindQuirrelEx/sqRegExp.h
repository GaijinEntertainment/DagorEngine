// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <squirrel.h>
#include <pcrecpp_nostl.h>
#include <EASTL/functional.h>

namespace bindquirrel
{
class RegExp
{
public:
  RegExp();
  RegExp(const char *pattern);

  // Check that 'match' string match any 'pattern' substrings.
  bool match(const char *match);

  // Check that 'match' string match 'pattern' string exactly.
  bool fullMatch(const char *match);

  // Replace all occurrences of 'pattern' in 'text' with 'replacement'.
  const char *replace(const char *replacement, const char *text);

  // Extract all occurences of 'pattern' from 'text' using 'extractTemplate'.
  // \0 in template is replaced with the complete matched substring, and
  // \1 through \9 are replaced with the corresponding capture group
  void multiExtract(const char *extractTemplate, const char *text, const eastl::function<void(const char *)> cb);

  // re.sqMultiExtract(template, text)  -->  ["match1", "match2", ...]
  static SQInteger sqMultiExtract(HSQUIRRELVM vm);

  // Return 'pattern' string, for debug.
  const char *pattern();

private:
  pcrecpp::RE2 re; // PCRE C++ interface
};
} // namespace bindquirrel
