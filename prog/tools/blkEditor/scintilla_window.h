// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <winGuiWrapper/wgw_timer.h>
#include <libTools/util/hdpiUtil.h>

class DataBlock;
class SimpleString;
struct SCNotification;

class ScintillaEH
{
public:
  virtual void onModify() = 0;
};

class CScintillaWindow : public ITimerCallBack
{
public:
  CScintillaWindow(ScintillaEH *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[]);
  ~CScintillaWindow() override;

  void *getParentWindowHandle() { return mParentHandle; }

  void *scintilaDefProc;

  void loadFromDataBlock(const DataBlock &blk);
  bool saveToDataBlock(DataBlock *blk);

  void getText(SimpleString &text);
  void setText(const char *text);

  int getCursorPos();
  void setCursorPos(int new_pos);

  int getScrollPos();
  void setScrollPos(int new_pos);

  void getFoldingInfo(DataBlock &blk);
  void setFoldingInfo(const DataBlock &blk);

  void notify(SCNotification *notification);

  void enableWindow(bool enable);
  void moveWindow(int x, int y, int width, int height);
  bool isWindowFocused() const;

protected:
  // ITimerCallBack
  void update() override;

  void initScintilla();
  int sendEditor(unsigned Msg, uintptr_t wParam = 0, intptr_t lParam = 0);
  void setAStyle(int style, unsigned long fore, unsigned long back = 0xFFFFFF, int size = -1, const char *face = 0);
  void setupScintilla();
  void onEditorChange();

protected:
  void *hwndScintilla;
  void *hwndScParent;
  void *mParentHandle;

  static void *sciLexerDll;
  ScintillaEH *eventHandler;
  unsigned long lastSetTime;
  bool needCallModify;
  WinTimer *updateTimer;
};
