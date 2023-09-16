#ifndef __GAIJIN_COOL_CONSOLE__
#define __GAIJIN_COOL_CONSOLE__
#pragma once


#include <libTools/util/iLogWriter.h>
#include <libTools/util/progressInd.h>
#include <libTools/containers/dag_StrMap.h>

#include <util/dag_string.h>

#include "iConsoleCmd.h"


struct HWND__;
struct HFONT__;
class DataBlock;

class CoolConsole : public ILogWriter, public IGenericProgressIndicator, public IConsoleCmd
{
public:
  CoolConsole(const char *caption, HWND__ *parent_wnd = NULL);
  ~CoolConsole();

  void setCaption(const char *capt);
  inline void setCanClose(bool can_close = true);
  inline void setCanMinimize(bool can_minimize = true);
  inline void setParentWnd(HWND__ *wnd) { parentWnd = wnd; }

  inline bool isCanClose() const { return flags & CC_CAN_CLOSE; }
  inline bool isCanMinimize() const { return flags & CC_CAN_MINIMIZE; }
  inline bool isProgress() const { return flags & CC_PROGRESS; }
  inline bool isCountMessages() const { return flags & CC_COUNT_MESSAGES; }

  bool isVisible() const;
  bool isTopmost() const;

  inline int getNotesCount() const { return notesCnt; }
  inline int getWarningsCount() const { return warningsCnt; }
  inline int getErrorsCount() const { return errorsCnt; }
  inline int getFatalsCount() const { return fatalsCnt; }

  virtual bool hasErrors() const { return errorsCnt || fatalsCnt; }
  inline bool hasWarnings() const { return warningsCnt; }

  // Global counters are never reset, get a counter before operation(s) and compare it after one.
  inline int getGlobalWarningsCounter() const { return globWarnCnt; }
  inline int getGlobalErrorsCounter() const { return globErrCnt; }
  inline int getGlobalFatalsCounter() const { return globFatalCnt; }

  // registers command
  bool registerCommand(const char *cmd, IConsoleCmd *handler);
  // unregisters command if it was registered to mentioned handler
  bool unregisterCommand(const char *cmd, IConsoleCmd *handler);
  // runs command line 'cmd'. if 'cmd' is NULL run command from console's command line
  bool runCommand(const char *cmd = NULL);

  bool initConsole();
  void destroyConsole();

  void showConsole(bool activate = false);
  void hideConsole() const;
  void clearConsole() const;
  void saveToFile(const char *file_name) const;

  void moveOnBottom() const;

  virtual void startLog();
  virtual void endLog();

  virtual void addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum);

  virtual void startProgress(IProgressCB *progress_cb = NULL);
  virtual void endProgress();
  virtual void setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum);
  virtual void setTotal(int total_cnt);
  virtual void setDone(int done_cnt);
  virtual void incDone(int inc = 1);

  inline void endLogAndProgress()
  {
    endLog();
    endProgress();
  }

  // IConsoleCmd
  virtual bool onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params);
  virtual const char *onConsoleCommandHelp(const char *cmd);

  void onCmdArrowKey(bool up, bool ctrl_modif);
  void saveCmdHistory(DataBlock &blk) const;
  void loadCmdHistory(const DataBlock &blk);

private:
  HWND__ *hWnd;
  HWND__ *parentWnd;
  HWND__ *logWnd;
  HWND__ *progressWnd;
  HWND__ *cmdWnd;
  HWND__ *cmdListWnd;
  HWND__ *cancelBtn;

  String caption;
  unsigned flags;
  bool lastVisible;

  IProgressCB *progressCb;

  int notesCnt;
  int warningsCnt;
  int errorsCnt;
  int fatalsCnt;

  int globWarnCnt, globErrCnt, globFatalCnt;

  Tab<String> cmdNames;
  StriMap<IConsoleCmd *> commands;

  Tab<String> cmdHistory;
  bool cmdHistoryLastUnfinished;
  int cmdHistoryPos;

  static unsigned wndClassId;
  static void *richEditDll;

  static HFONT__ *conFont;
  static int conFontHeight;

  static HFONT__ *interfaceFont;

  static int conCount;
  static int lastDagorIdleCall;

  enum
  {
    CC_CAN_CLOSE = 1 << 0,
    CC_CAN_MINIMIZE = 1 << 1,
    CC_PROGRESS = 1 << 2,
    CC_COUNT_MESSAGES = 1 << 3,
  };

  virtual void redrawScreen() {}
  virtual void destroy() {}

  void alignControls(bool align_progress_only = false);

  void runHelp(const char *command);
  void runCmdList();

  void getCmds(const char *prefix, Tab<String> &cmds) const;
  void showCmdList();

  void addTextToLog(const char *text, unsigned color = 0, const unsigned *pos = NULL, bool bold = false);

  static bool registerWndClass();
  static intptr_t __stdcall wndProc(void *wnd, unsigned msg, intptr_t w_param, intptr_t l_param);

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
  QuietConsole() : CoolConsole("") {}
  virtual void setActionDesc(const char *desc) {}
  virtual void setTotal(int total_cnt) {}
  virtual void setDone(int done_cnt) {}
  virtual void incDone(int inc = 1) {}
  virtual void redrawScreen() {}
  virtual void destroy() {}
};


#endif //__GAIJIN_COOL_CONSOLE__
