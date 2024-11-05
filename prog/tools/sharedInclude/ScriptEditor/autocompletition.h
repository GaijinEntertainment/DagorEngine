// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

class ScriptEditor;
class AutoCompletition
{
public:
  AutoCompletition(ScriptEditor *ed_);
  void charAdded(char ch);
  Tab<String> wordsNoCase;

private:
  bool startAutoCompleteWord(bool onlyOneWord);
  bool startAutoComplete();
  void continueCallTip();
  bool startCallTip();
  String getNearestWord(const char *wordStart, int searchLen);
  String getNearestWords(const char *wordStart, int searchLen);
  void fillFunctionDefinition(int pos = -1);

  int braceCount;
  String autoCompleteStartCharacters;
  String calltipWordCharacters;
  String wordCharacters;
  bool autoCCausedByOnlyOne;
  String calltipParametersEnd;
  String calltipParametersStart;
  int currentCallTip;
  String currentCallTipWord;
  int lastPosCallTip;
  int maxCallTips;
  String functionDefinition;
  String calltipEndDefinition;
  bool callTipIgnoreCase;
  int startCalltipWord;
  String calltipParametersSeparators;
  bool sortedNoCase;

  ScriptEditor *ed;
};
