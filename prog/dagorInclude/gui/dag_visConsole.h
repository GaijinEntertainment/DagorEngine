//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <util/dag_console.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_critSec.h>

namespace console
{
struct ConsoleLine
{
  LineType line_type;
  String line;
};

struct ConsoleProgressIndicator
{
  String indicatorId;
  String title;
};

class IVisualConsoleDriver
{
public:
  IVisualConsoleDriver(){};

  virtual ~IVisualConsoleDriver(){};

  virtual void init(const char *script_filename) = 0;
  virtual void shutdown() = 0;

  virtual void render() = 0;
  virtual void update() = 0;
  virtual bool processKey(int btn_id, int wchar, bool handle_tilde) = 0;

  virtual void puts(const char *str, console::LineType type = console::CONSOLE_DEBUG) = 0;

  virtual void show() = 0;
  virtual void hide() = 0;
  virtual bool is_visible() = 0;

  virtual real get_cur_height() = 0;
  // setup current console height (0.0f < h <= 1.0f)
  virtual void set_cur_height(real h) = 0;
  virtual void reset_important_lines() = 0; // call it before script reload

  virtual void set_progress_indicator(const char *id, const char *title) = 0; // remove if title is empty

  virtual void save_text(const char *filename) = 0;

  virtual void setFontId(int fId) = 0;

  virtual const char *getEditTextBeforeModify() const = 0;
};

class DefaultDagorVisualConsoleDriver : public IVisualConsoleDriver
{
public:
  DefaultDagorVisualConsoleDriver();
  virtual ~DefaultDagorVisualConsoleDriver();

  virtual void render();
  virtual void update() {}
  virtual bool processKey(int btn_id, int wchar, bool handle_tilde);

  virtual void reset_important_lines();
  void puts(const char *str, console::LineType type = console::CONSOLE_DEBUG) override;

  virtual void init(const char * /*script_filename*/) {}
  virtual void shutdown() {}

  virtual void show()
  {
    isVisible = true;
    scrollPos = 0;
  }
  virtual void hide() { isVisible = false; }
  virtual bool is_visible() { return isVisible; }

  virtual real get_cur_height() { return 1.f; }
  virtual void set_cur_height(real /*h*/) {}
  virtual void set_progress_indicator(const char *id, const char *title); // remove if title is empty

  virtual void save_text(const char * /*filename*/) {}

  virtual void getCommandList(console::CommandList &out_list);

  virtual void getFilteredCommandList(console::CommandList &out_list, const String &prefix);

  virtual void setFontId(int fId) { fontId = fId; }

  virtual const char *getEditTextBeforeModify() const override { return editTextBeforeModify.c_str(); }

protected:
  bool isVisible;
  int bufferLines;
  int screenLines;
  int scrollPos; // dafault:0 scroll_up:+ scroll_down:-
  int tipsScrollPos;
  int maxTipsCount;
  int tipsOutOfScreen;
  int lineHeight;
  Tab<ConsoleProgressIndicator> progressIndicators;
  Tab<ConsoleLine> linesList;
  Tab<int> importantLines;
  String editText;
  int editPos;
  String prevTabText;
  int commandsDisplayed;
  int commandIndex;
  bool controlPressed;
  bool shiftPressed;
  bool altPressed;
  bool modified;
  String editTextBeforeModify;
  int fontId = 0;
  WinCritSec consoleCritSec;

  void onCommandModified();
  void printHelp();
};

void set_visual_driver(IVisualConsoleDriver *driver, const char *script_filename);
void set_visual_driver_raw(IVisualConsoleDriver *driver);

IVisualConsoleDriver *get_visual_driver();

//! creates instance of TTY-capable console driver (where applicable), or simply DefaultDagorVisualConsoleDriver
IVisualConsoleDriver *create_def_console_driver(bool allow_tty);
} // namespace console
