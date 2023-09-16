#include "dagorLogWindow.h"
#include "resource.h"
#include <max.h>
#include <tab.h>

HWND DagorLogWindow::hwnd = 0;
TSTR DagorLogWindow::logText;
bool DagorLogWindow::hasWarningOrError = false;

void DagorLogWindow::show(bool reset_position_and_size)
{
  if (reset_position_and_size && hwnd)
    DestroyWindow(hwnd);

  if (hwnd)
  {
    ShowWindow(hwnd, SW_SHOW);
    SetActiveWindow(hwnd);
  }
  else
  {
    Interface *ip = GetCOREInterface();
    CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DAGOR_LOG_WINDOW), ip->GetMAXHWnd(), &DagorLogWindow::dialogProc);
    ip->RegisterDlgWnd(hwnd);
    ::ShowWindow(hwnd, SW_SHOW);
  }
}

void DagorLogWindow::hide()
{
  if (hwnd)
    ShowWindow(hwnd, SW_HIDE);
}

void DagorLogWindow::addToLog(LogLevel level, const TCHAR *format, ...)
{
  va_list args;
  va_start(args, format);
  addToLog(level, format, args);
  va_end(args);
}

void DagorLogWindow::addToLog(LogLevel level, const TCHAR *format, va_list args)
{
  TSTR textToAdd;
  textToAdd.vprintf(format, args);
  logText += textToAdd;

  if (level == LogLevel::Warning || level == LogLevel::Error)
    hasWarningOrError = true;

  updateLogEditBox();
}

void DagorLogWindow::clearLog()
{
  logText = _T("");
  hasWarningOrError = false;
  updateLogEditBox();
}

bool DagorLogWindow::getHasWarningOrError() { return hasWarningOrError; }

void DagorLogWindow::updateLogEditBox()
{
  if (hwnd)
    SetDlgItemText(hwnd, IDC_LOGTEXT, logText);
}

std::vector<TSTR> DagorLogWindow::getLogLines()
{
  std::vector<TSTR> lines;
  int lineStartIndex = 0;

  const int length = logText.length();
  for (int i = 0; i < length; ++i)
  {
    if (logText[i] == '\r' && (i + 1) < length && logText[i + 1] == '\n')
    {
      lines.emplace_back(logText.Substr(lineStartIndex, i - lineStartIndex));
      lineStartIndex = i + 2;
      ++i;
    }
  }

  if (lineStartIndex < length)
    lines.emplace_back(logText.Substr(lineStartIndex, length - lineStartIndex));

  return lines;
}

INT_PTR CALLBACK DagorLogWindow::dialogProc(HWND hw, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
      hwnd = hw;
      updateLogEditBox();

      // Prevent the default selection of the entire contents of the edit box.
      SendDlgItemMessage(hwnd, IDC_LOGTEXT, EM_SETSEL, -1, -1);
      SetFocus(GetDlgItem(hwnd, IDC_LOGTEXT));
      return false;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_LOGTEXT:
        {
          if (HIWORD(wParam) == EN_SETFOCUS)
            DisableAccelerators();
          else if (HIWORD(wParam) == EN_KILLFOCUS)
            EnableAccelerators();
        }
        break;
      }
      break;

    case WM_CLOSE: hide(); break;

    case WM_DESTROY:
    {
      EndDialog(hwnd, FALSE);
      Interface *ip = GetCOREInterface();
      ip->UnRegisterDlgWnd(hwnd);
      hwnd = 0;
    }
    break;

    case WM_SIZE:
    {
      HWND editBox = GetDlgItem(hwnd, IDC_LOGTEXT);
      RECT rc;
      GetWindowRect(editBox, &rc);
      POINT pt = {rc.left, rc.top};
      ScreenToClient(hwnd, &pt);

      const int margin = pt.x;
      const int width = LOWORD(lParam);
      const int height = HIWORD(lParam);
      MoveWindow(editBox, margin, margin, width - (margin * 2), height - (margin * 2), TRUE);
    }
    break;

    default: return FALSE;
  }

  return TRUE;
}

DagorLogWindowAutoShower::DagorLogWindowAutoShower(bool clear_log)
{
  if (clear_log)
    DagorLogWindow::clearLog();

  hasWarningOrError = DagorLogWindow::getHasWarningOrError();
}

DagorLogWindowAutoShower::~DagorLogWindowAutoShower()
{
  if (!hasWarningOrError && DagorLogWindow::getHasWarningOrError())
    DagorLogWindow::show();
}

class DagorLogStaticInterface : public FPStaticInterface
{
public:
  DECLARE_DESCRIPTOR(DagorLogStaticInterface);

  struct FunctionId
  {
    enum
    {
      ShowDialog,
      HideDialog,
      LogNote,
      LogWarning,
      LogError,
      ClearLog,
      GetLogLines,
      HasWarningOrError,
    };
  };

  BEGIN_FUNCTION_MAP
  VFN_0(FunctionId::ShowDialog, showDialog)
  VFN_0(FunctionId::HideDialog, hideDialog)
  VFN_1(FunctionId::LogNote, logNote, TYPE_STRING)
  VFN_1(FunctionId::LogWarning, logWarning, TYPE_STRING)
  VFN_1(FunctionId::LogError, logError, TYPE_STRING)
  VFN_0(FunctionId::ClearLog, clearLog)
  FN_0(FunctionId::GetLogLines, TYPE_STRING_TAB, getLogLines)
  FN_0(FunctionId::HasWarningOrError, TYPE_BOOL, hasWarningOrError)
  END_FUNCTION_MAP

  void showDialog() { DagorLogWindow::show(); }
  void hideDialog() { DagorLogWindow::hide(); }
  void logNote(const TCHAR *message) { log(DagorLogWindow::LogLevel::Note, message); }
  void logWarning(const TCHAR *message) { log(DagorLogWindow::LogLevel::Warning, message); }
  void logError(const TCHAR *message) { log(DagorLogWindow::LogLevel::Error, message); }
  void clearLog() { DagorLogWindow::clearLog(); }

  Tab<TCHAR *> *getLogLines()
  {
    const std::vector<TSTR> lines = DagorLogWindow::getLogLines();

    Tab<TCHAR *> *resultLines = new Tab<TCHAR *>();
    for (int i = 0; i < lines.size(); ++i)
    {
      const TSTR &line = lines[i];
      TCHAR *resultLine = new TCHAR[line.length() + 1];
      memcpy(resultLine, line.data(), (line.length() + 1) * sizeof(TCHAR));
      resultLines->Append(1, &resultLine);
    }

    return resultLines;
  }

  bool hasWarningOrError() { return DagorLogWindow::getHasWarningOrError(); }

private:
  void log(DagorLogWindow::LogLevel level, const TCHAR *message)
  {
    TSTR str(message);
    // \n in MaxScript is actually \r\n, but handle everything.
    str.Replace(_T("\r\n"), _T("\n"));
    str.Replace(_T("\r"), _T("\n"));
    str.Replace(_T("\n"), _T("\r\n"));
    DagorLogWindow::addToLog(level, _T("%s\r\n"), str.data());
  }
};

// clang-format off
static DagorLogStaticInterface theDagorLogStaticInterface(
  Interface_ID(0x438b2512, 0xef263531), _T("dagorLog"), -1, 0, FP_CORE,

  DagorLogStaticInterface::FunctionId::ShowDialog, _T("showDialog"), -1, TYPE_VOID, FP_NO_REDRAW, 0,
  DagorLogStaticInterface::FunctionId::HideDialog, _T("hideDialog"), -1, TYPE_VOID, FP_NO_REDRAW, 0,
  DagorLogStaticInterface::FunctionId::LogNote, _T("logNote"), -1, TYPE_VOID, FP_NO_REDRAW, 1,
    _T("message"), -1, TYPE_STRING,
  DagorLogStaticInterface::FunctionId::LogWarning, _T("logWarning"), -1, TYPE_VOID, FP_NO_REDRAW, 1,
    _T("message"), -1, TYPE_STRING,
  DagorLogStaticInterface::FunctionId::LogError, _T("logError"), -1, TYPE_VOID, FP_NO_REDRAW, 1,
    _T("message"), -1, TYPE_STRING,
  DagorLogStaticInterface::FunctionId::ClearLog, _T("clearLog"), -1, TYPE_VOID, FP_NO_REDRAW, 0,
  DagorLogStaticInterface::FunctionId::GetLogLines, _T("getLogLines"), -1, TYPE_STRING_TAB, FP_NO_REDRAW, 0,
  DagorLogStaticInterface::FunctionId::HasWarningOrError, _T("hasWarningOrError"), -1, TYPE_BOOL, FP_NO_REDRAW, 0,

#if defined(MAX_RELEASE_R15) && MAX_RELEASE >= MAX_RELEASE_R15
  p_end
#else
  end
#endif
);
// clang-format on