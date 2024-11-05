// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <strclass.h>
#include <vector>

class DagorLogWindow
{
public:
#if (__cplusplus >= 201100L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201100L)
  enum class LogLevel
#else
  enum LogLevel
#endif
  {
    Note,
    Warning,
    Error,
  };

  static void show(bool reset_position_and_size = false);
  static void hide();

  static void addToLog(LogLevel level, const TCHAR *format, ...);
  static void addToLog(LogLevel level, const TCHAR *format, va_list args);
  static void clearLog();

  static bool getHasWarningOrError();
  static std::vector<TSTR> getLogLines();

private:
  static void updateLogEditBox();
  static INT_PTR CALLBACK dialogProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam);

  static HWND hwnd;
  static TSTR logText;
  static bool hasWarningOrError;
};

// Automatically shows the Dagor Log window if an error or warnings gets written to the log.
class DagorLogWindowAutoShower
{
public:
  explicit DagorLogWindowAutoShower(bool clear_log);
  ~DagorLogWindowAutoShower();

private:
  bool hasWarningOrError;
};