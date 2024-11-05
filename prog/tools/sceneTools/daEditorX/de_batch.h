// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>


class DeBatch
{
public:
  int numErrors;

  DeBatch() : numErrors(0) {}

  bool openAndRunBatch();
  bool runBatch(const char *path);
  bool runBatchStr(const Tab<String> &commands);

private:
  enum ProcessAction
  {
    PA_RUN_COMMAND,
    PA_CONTINUE,
    PA_TERMINATE,
  };

  bool tryBlock;
  bool gotoTryEnd;
  bool multiLineComment;

  void prepareToBatch();
  ProcessAction checkBatchSpecialWords(const char *str);

  void removeSingleLineComment(char *str) const;
  char *checkMultilineComment(char *str);
};
