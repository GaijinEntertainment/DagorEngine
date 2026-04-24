// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_string.h>

class ConBatch
{
public:
  int numErrors;

  ConBatch() : numErrors(0) {}

  static void updateQueue();

  bool openAndRunBatch();
  bool runBatch(const char *path, bool async = false);
  bool runBatchStr(const Tab<String> &commands, bool async = false);

private:
  enum ProcessAction
  {
    PA_RUN_COMMAND,
    PA_CONTINUE,
    PA_TERMINATE,
  };

  bool prepared = false;
  bool tryBlock = false;
  bool gotoTryEnd = false;
  bool multiLineComment = false;

  void prepareToBatch();
  void startBatch(bool track_progress, int total_cnt);
  bool runCommandStr(String &cmdBuf);
  bool stopBatch(bool track_progress, bool ret);

  ProcessAction checkBatchSpecialWords(const char *str);

  void removeSingleLineComment(char *str) const;
  char *checkMultilineComment(char *str);
};

ConBatch *get_cmd_queue_batch();

void start_cmd_queue_idle_cycle();
void stop_cmd_queue_idle_cycle();

void register_cmd_queue_console_handler();
