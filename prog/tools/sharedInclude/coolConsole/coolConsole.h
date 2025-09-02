// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/iLogWriter.h>
#include <libTools/util/progressInd.h>
#include <libTools/containers/dag_StrMap.h>
#include <gui/dag_visConsole.h>
#include <math/dag_e3dColor.h>
#include <propPanel/messageQueue.h>

#include <util/dag_string.h>

#include "iConsoleCmd.h"

class DataBlock;

class CoolConsole : public ILogWriter,
                    public IGenericProgressIndicator,
                    public IConsoleCmd,
                    public console::IVisualConsoleDriver,
                    public PropPanel::IDelayedCallbackHandler
{
public:
  CoolConsole();
  ~CoolConsole() override;

  inline void setCanClose(bool can_close = true);
  inline void setCanMinimize(bool can_minimize = true);

  inline bool isCanClose() const { return flags & CC_CAN_CLOSE; }
  inline bool isCanMinimize() const { return flags & CC_CAN_MINIMIZE; }
  inline bool isProgress() const { return flags & CC_PROGRESS; }
  inline bool isCountMessages() const { return flags & CC_COUNT_MESSAGES; }

  bool isVisible() const;

  inline int getNotesCount() const { return notesCnt; }
  inline int getWarningsCount() const { return warningsCnt; }
  inline int getErrorsCount() const { return errorsCnt; }
  inline int getFatalsCount() const { return fatalsCnt; }

  bool hasErrors() const override { return errorsCnt || fatalsCnt; }
  inline bool hasWarnings() const { return warningsCnt; }

  // Global counters are never reset, get a counter before operation(s) and compare it after one.
  inline int getGlobalWarningsCounter() const { return globWarnCnt; }
  inline int getGlobalErrorsCounter() const { return globErrCnt; }
  inline int getGlobalFatalsCounter() const { return globFatalCnt; }

  // registers command
  bool registerCommand(const char *cmd, IConsoleCmd *handler);
  // unregisters command if it was registered to mentioned handler
  bool unregisterCommand(const char *cmd, IConsoleCmd *handler);
  // runs command line 'cmd'
  bool runCommand(const char *cmd);

  bool initConsole();
  void destroyConsole();

  void showConsole(bool activate = false);
  void hideConsole();
  void clearConsole();
  void saveToFile(const char *file_name) const;

  void startLog() override;
  void endLog() override;

  void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum) override;

  void startProgress(IProgressCB *progress_cb = NULL) override;
  void endProgress() override;
  void setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum) override;
  void setTotal(int total_cnt) override;
  void setDone(int done_cnt) override;
  void incDone(int inc = 1) override;

  inline void endLogAndProgress()
  {
    endLog();
    endProgress();
  }

  // IConsoleCmd
  bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params) override;
  const char *onConsoleCommandHelp(const char *cmd) override;

  String onCmdArrowKey(bool up, const char *current_command);
  void saveCmdHistory(DataBlock &blk) const;
  void loadCmdHistory(const DataBlock &blk);

  // IVisualConsoleDriver
  void puts(const char *str, console::LineType type = console::CONSOLE_DEBUG) override;
  void show() override { showConsole(); }
  void hide() override { hideConsole(); }
  bool is_visible() override { return isVisible(); }

  void init(const char *) override {}
  void shutdown() override {}
  void render() override {}
  void update() override {}
  bool processKey(int, int, bool) override { return false; }
  real get_cur_height() override { return 0; }
  void set_cur_height(real) override {}
  void reset_important_lines() override {}
  void set_progress_indicator(const char *, const char *) override {}
  void save_text(const char *) override {}
  void setFontId(int) override {}
  const char *getEditTextBeforeModify() const override { return nullptr; }

  void updateImgui();

private:
  unsigned flags;
  bool visible = false;
  bool justMadeVisible = false;
  bool lastVisible;
  bool activateOnShow = false;
  bool scrollToBottomOnShow = false;

  IProgressCB *progressCb;
  int progressPosition = 0;
  int progressMaxPosition = 0;

  int notesCnt;
  int warningsCnt;
  int errorsCnt;
  int fatalsCnt;

  int globWarnCnt, globErrCnt, globFatalCnt;
  int limitOutputLinesLeft = -1;

  Tab<String> cmdNames;
  StriMap<IConsoleCmd *> commands;

  String commandInputText;
  Tab<String> cmdHistory;
  bool cmdHistoryLastUnfinished;
  int cmdHistoryPos;

  enum
  {
    CC_CAN_CLOSE = 1 << 0,
    CC_CAN_MINIMIZE = 1 << 1,
    CC_PROGRESS = 1 << 2,
    CC_COUNT_MESSAGES = 1 << 3,
  };

  void redrawScreen() override {}
  void destroy() override {}

  void onImguiDelayedCallback(void *user_data) override;

  void runHelp(const char *command);
  void runCmdList();

  void getCmds(const char *prefix, Tab<String> &cmds) const;

  void addTextToLog(const char *text, int color_index, bool bold = false, bool start_new_line = true);

  void renderLogTextUI(float height);
  void renderCommandInputUI();

  static int compareCmd(const String *s1, const String *s2);
};


//==============================================================================
inline void CoolConsole::setCanClose(bool can_close)
{
  if (can_close)
    flags |= CC_CAN_CLOSE;
  else
    flags &= ~CC_CAN_CLOSE;
}


//==============================================================================
inline void CoolConsole::setCanMinimize(bool can_minimize)
{
  if (can_minimize)
    flags |= CC_CAN_MINIMIZE;
  else
    flags &= ~CC_CAN_MINIMIZE;
}

//==============================================================================
inline void CoolConsole::startLog()
{
  notesCnt = 0;
  warningsCnt = 0;
  errorsCnt = 0;
  fatalsCnt = 0;

  flags |= CC_COUNT_MESSAGES;
}


//==============================================================================
inline void CoolConsole::endLog()
{
  flags &= ~CC_COUNT_MESSAGES;

  notesCnt = 0;
  warningsCnt = 0;
  errorsCnt = 0;
  fatalsCnt = 0;
}

/// stub implementation of coolconsole interface
/// (can be used when coolconsol required but is not useful)
class QuietConsole : public CoolConsole
{
public:
  void setTotal(int) override {}
  void setDone(int) override {}
  void incDone(int) override {}
  void redrawScreen() override {}
  void destroy() override {}
};
