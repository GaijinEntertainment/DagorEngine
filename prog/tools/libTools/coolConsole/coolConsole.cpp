#pragma comment(lib, "comctl32.lib")
#include <generic/dag_tab.h>
#include <coolConsole/coolConsole.h>
#include <generic/dag_sort.h>
#include <util/dag_console.h>
#include <osApiWrappers/dag_miscApi.h>
#include <winGuiWrapper/wgw_input.h>

#include <util/dag_globDef.h>
#include <workCycle/dag_workCycle.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_unicode.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <libTools/util/hash.h>

#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <limits.h>

#include <stdio.h>

#define DESTROY_WND(window)  \
  if (window)                \
  {                          \
    ::DestroyWindow(window); \
    window = NULL;           \
  }

#define ABOUT_MESSAGE "CoolConsole version 1.2"

#define PROGRESS_HEIGHT           20
#define DEF_CON_FONT_HEIGHT       9
#define DEF_INTERFACE_FONT_HEIGHT 8
#define EDIT_HEIGHT               (conFontHeight * 1.8)

#define CANCEL_BUTTON_WIDTH 50
#define CANCEL_BUTTON_ID    1000

#define CMD_COLOR     RGB(0, 0, 0)
#define PARAM_COLOR   RGB(128, 128, 0)
#define WARNING_COLOR RGB(0, 128, 0)
#define ERROR_COLOR   RGB(255, 0, 0)
#define FATAL_COLOR   RGB(255, 0, 0)
#define REMARK_COLOR  RGB(128, 128, 128)

#define DAGOR_IDLE_CYCLE_CALL_TIMEOUT 1000


#ifdef ERROR
#undef ERROR
#endif


// #define GET_CMD_HASH(cmd) ::get_hash(_strlwr(cmd), 0)

#define MAKE_CMD_HASH(cmd, hash) \
  String temp(cmd);              \
  hash = GET_CMD_HASH(temp.begin());


unsigned CoolConsole::wndClassId = 0;
void *CoolConsole::richEditDll = NULL;
int CoolConsole::conCount = 0;
HFONT CoolConsole::conFont = NULL;
int CoolConsole::conFontHeight = 11;
HFONT CoolConsole::interfaceFont = NULL;
// int       CoolConsole::lastDagorIdleCall = 0;

static void call_idle_cycle_seldom(bool important = false)
{
  static int lastDagorIdleCall = 0;

  int msec = get_time_msec();
  if (msec - lastDagorIdleCall > (important ? 50 : DAGOR_IDLE_CYCLE_CALL_TIMEOUT))
  {
    lastDagorIdleCall = msec;
    ::dagor_idle_cycle();
  }
}

static char *strtokLastToken = NULL;

// strtok analog excluding token-delimited qouted strings from tokens search
// for example string 'command "spaced parameter1" parameter2' will be split to
//'command', 'spaced parameter1', 'parameter2'
//==============================================================================
static char *conSttrok(char *str, const char *delim)
{
  if (!str)
    str = strtokLastToken;

  if (str)
  {
    if (*str == '"')
    {
      char *c = strchr(str + 1, '"');

      if (c)
      {
        bool quotedStr = false;

        if (!*(c + 1))
          quotedStr = true;
        else
          for (const char *tok = delim; *tok; ++tok)
            if (*(c + 1) == *tok)
            {
              quotedStr = true;
              break;
            }

        if (quotedStr)
        {
          *c = 0;
          strtokLastToken = c + 1;

          return str + 1;
        }
      }
    }

    for (char *c = str; *c; ++c)
    {
      for (const char *tok = delim; *tok; ++tok)
        if (*c == *tok)
        {
          *c = 0;
          strtokLastToken = c + 1;
          return str;
        }
    }

    strtokLastToken = NULL;
    return str;
  }

  return NULL;
}


//==============================================================================
CoolConsole::CoolConsole(const char *capt, HWND parent_wnd) :
  hWnd(NULL),
  caption(capt),
  parentWnd(parent_wnd),
  logWnd(NULL),
  cancelBtn(NULL),
  cmdWnd(NULL),
  progressWnd(NULL),
  commands(midmem),
  cmdNames(midmem),
  progressCb(NULL),
  cmdHistoryLastUnfinished(false),
  cmdHistoryPos(0),
  lastVisible(false),
  notesCnt(0),
  warningsCnt(0),
  errorsCnt(0),
  fatalsCnt(0),
  globWarnCnt(0),
  globErrCnt(0),
  globFatalCnt(0)
{
  ++conCount;

  flags = CC_CAN_CLOSE | CC_CAN_MINIMIZE;

  HDC dc = ::GetDC(NULL);

  if (!conFont)
  {
    conFontHeight = MulDiv(DEF_CON_FONT_HEIGHT, GetDeviceCaps(dc, LOGPIXELSY), 72);

    conFont = ::CreateFont(-conFontHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY, 0, "Courier New");
  }

  if (!interfaceFont)
  {
    int fontHeight = MulDiv(DEF_INTERFACE_FONT_HEIGHT, GetDeviceCaps(dc, LOGPIXELSY), 72);

    interfaceFont = ::CreateFont(-fontHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, PROOF_QUALITY, 0, "Tahoma");
  }

  ::ReleaseDC(NULL, dc);

  registerCommand("help", this);
  registerCommand("list", this);
  registerCommand("clear", this);
  console::set_visual_driver_raw(this);
}


//==============================================================================
CoolConsole::~CoolConsole()
{
  console::set_visual_driver_raw(nullptr);
  --conCount;

  destroyConsole();

  if (conCount <= 0)
  {
    ::DeleteObject(conFont);
    conFont = NULL;

    ::DeleteObject(interfaceFont);
    interfaceFont = NULL;

    ::FreeLibrary((HMODULE)richEditDll);
    richEditDll = NULL;
  }
}


//==============================================================================
int CoolConsole::compareCmd(const String *s1, const String *s2) { return stricmp((const char *)*s1, (const char *)*s2); }


//==============================================================================
bool CoolConsole::registerWndClass()
{
  if (wndClassId)
    return true;

  richEditDll = ::LoadLibrary("riched20.dll");

  if (!richEditDll)
  {
    debug("[CoolConsole::registerWndClass] Unable to load \"riched20.dll\"");
    return false;
  }

  INITCOMMONCONTROLSEX initRec;

  initRec.dwSize = sizeof(INITCOMMONCONTROLSEX);
  initRec.dwICC = ICC_PROGRESS_CLASS;

  if (!InitCommonControlsEx(&initRec))
  {
    debug("[CoolConsole::registerWndClass] Unable to init common controls");
    return false;
  }

  WNDCLASSEXW wcex;

  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = 0;
  wcex.lpfnWndProc = (WNDPROC)wndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = ::GetModuleHandle(NULL);
  wcex.hIcon = ::LoadIcon(NULL, IDI_INFORMATION);
  wcex.hIconSm = ::LoadIcon(NULL, IDI_INFORMATION);
  wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = L"CoolConsole";

  wndClassId = RegisterClassExW(&wcex);

  return (bool)wndClassId;
}


//==============================================================================
intptr_t CALLBACK CoolConsole::wndProc(void *hwnd, unsigned msg, intptr_t w_param, intptr_t l_param)
{
  HWND wnd = (HWND)hwnd;
  PAINTSTRUCT ps;
  HDC hdc;
  CoolConsole *console = (CoolConsole *)::GetWindowLongPtr(wnd, GWLP_USERDATA);

  switch (msg)
  {
    case WM_PAINT:
      hdc = ::BeginPaint(wnd, &ps);
      EndPaint(wnd, &ps);
      break;

    case WM_SIZE: console->alignControls(); break;

    case WM_SYSCOMMAND:
      switch (w_param)
      {
        case SC_CLOSE:
          if (!console->isProgress() && console->isCanClose())
          {
            if (auto *phwnd = (HWND)console->parentWnd)
            {
              ::BringWindowToTop(phwnd);
              ::SetFocus(phwnd);
            }
            console->hideConsole();
          }
          break;

        case SC_MINIMIZE:
          if (!console->isProgress() && console->isCanMinimize())
            return ::DefWindowProcW(wnd, msg, w_param, l_param);
          break;

        default: return ::DefWindowProcW(wnd, msg, w_param, l_param);
      }
      break;

    case WM_COMMAND:
      if ((HWND)l_param == console->cmdWnd)
      {
        switch (HIWORD(w_param))
        {
          case EN_MAXTEXT: console->runCommand(); return 0;

          case EN_CHANGE: console->showCmdList(); break;
        }
      }
      else if ((HWND)l_param == console->cancelBtn && LOWORD(w_param) == CANCEL_BUTTON_ID && console->progressCb)
      {
        console->progressCb->onCancel();
        return 0;
      }

      return ::DefWindowProcW(wnd, msg, w_param, l_param);

    case WM_KEYDOWN:
      if (w_param == VK_ESCAPE)
      {
        ShowWindow(wnd, SW_HIDE /*SW_MINIMIZE*/);
        return 0;
      }
      return ::DefWindowProcW(wnd, msg, w_param, l_param);

    default: return ::DefWindowProcW(wnd, msg, w_param, l_param);
  }

  return 0;
}

static WNDPROC origEditboxWndProc = NULL;
static intptr_t CALLBACK editboxWndProc(void *hwnd, unsigned msg, intptr_t w_param, intptr_t l_param)
{
  HWND wnd = (HWND)hwnd;

  switch (msg)
  {
    case WM_KEYDOWN:
      switch (w_param)
      {
        case VK_UP:
          ((CoolConsole *)::GetWindowLongPtr(wnd, GWLP_USERDATA))->onCmdArrowKey(true, wingw::is_key_pressed(VK_CONTROL));
          return 0;
        case VK_DOWN:
          ((CoolConsole *)::GetWindowLongPtr(wnd, GWLP_USERDATA))->onCmdArrowKey(false, wingw::is_key_pressed(VK_CONTROL));
          return 0;
        case VK_ESCAPE: ShowWindow(GetParent(wnd), SW_HIDE /*SW_MINIMIZE*/); return 0;
      }
      break;
  }

  return origEditboxWndProc(wnd, msg, w_param, l_param);
}

static WNDPROC origLogWndProc = NULL;
static intptr_t CALLBACK logWndProc(void *hwnd, unsigned msg, intptr_t w_param, intptr_t l_param)
{
  HWND wnd = (HWND)hwnd;

  if (msg == WM_KEYDOWN && w_param == VK_ESCAPE)
  {
    ShowWindow(GetParent(wnd), SW_HIDE /*SW_MINIMIZE*/);
    return 0;
  }
  return origLogWndProc(wnd, msg, w_param, l_param);
}

static void moveEditCaretToEnd(HWND wnd)
{
  DWORD textSize = ::GetWindowTextLength(wnd);
  ::SendMessage(wnd, EM_SETSEL, textSize, textSize);
  ::SendMessage(wnd, EM_SCROLLCARET, 0, 0);
}

void CoolConsole::onCmdArrowKey(bool up, bool /*ctrl_modif*/)
{
  String command;
  const int cmdLen = (int)::SendMessage(cmdWnd, WM_GETTEXTLENGTH, 0, 0) + 1;
  command.resize(cmdLen);
  ::GetWindowText(cmdWnd, command.begin(), cmdLen);
  command[cmdLen - 1] = 0;

  if (up && cmdHistoryPos > 0)
  {
    if (cmdHistoryPos == cmdHistory.size() && !cmdHistoryLastUnfinished && !command.empty())
    {
      cmdHistory.push_back() = command;
      cmdHistoryLastUnfinished = true;
    }
    cmdHistoryPos--;
    ::SetWindowText(cmdWnd, cmdHistory[cmdHistoryPos]);
    moveEditCaretToEnd(cmdWnd);
  }
  else if (!up && cmdHistoryPos < cmdHistory.size())
  {
    cmdHistoryPos++;
    ::SetWindowText(cmdWnd, cmdHistoryPos < cmdHistory.size() ? cmdHistory[cmdHistoryPos].str() : NULL);
    if (cmdHistoryLastUnfinished && cmdHistoryPos + 1 == cmdHistory.size())
    {
      cmdHistoryLastUnfinished = false;
      cmdHistory.pop_back();
      cmdHistoryPos = cmdHistory.size();
    }
    moveEditCaretToEnd(cmdWnd);
  }
}
void CoolConsole::saveCmdHistory(DataBlock &blk) const
{
  blk.removeBlock("cmdHistory");
  DataBlock &b = *blk.addBlock("cmdHistory");
  for (int i = 0; i < cmdHistory.size(); i++)
    b.addStr("cmd", cmdHistory[i]);
}
void CoolConsole::loadCmdHistory(const DataBlock &blk)
{
  const DataBlock &b = *blk.getBlockByNameEx("cmdHistory");
  cmdHistory.clear();
  cmdHistoryLastUnfinished = false;

  int nid = blk.getNameId("cmd");
  for (int i = 0; i < b.paramCount(); i++)
    if (b.getParamNameId(i) == nid && b.getParamType(i) == b.TYPE_STRING)
      cmdHistory.push_back() = b.getStr(i);
  cmdHistoryPos = cmdHistory.size();
}

//==============================================================================
void CoolConsole::setCaption(const char *capt)
{
  caption = capt;

  if (hWnd)
    ::SetWindowText(hWnd, capt);
}


//==============================================================================
bool CoolConsole::isVisible() const { return ::GetWindowLongPtr(hWnd, GWL_STYLE) & WS_VISIBLE; }


//==============================================================================
bool CoolConsole::isTopmost() const
{
  return false; //::GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
}


//==============================================================================
void CoolConsole::addMessageFmt(MessageType type, const char *fmt, const DagorSafeArg *arg, int anum)
{
  if (!hWnd)
    return;

  if (!fmt || !*fmt)
    return;

  unsigned color = 0;
  String con_fmt(0, "CON: %s", fmt);

  switch (type)
  {
    case REMARK:
      color = REMARK_COLOR;
      if (isCountMessages())
        ++notesCnt;
      break;

    case WARNING:
      ++globWarnCnt;
      color = WARNING_COLOR;
      if (isCountMessages())
        ++warningsCnt;
      logmessage_fmt(LOGLEVEL_WARN, con_fmt, arg, anum);
      break;

    case ERROR:
      ++globErrCnt;
      color = ERROR_COLOR;
      if (isCountMessages())
        ++errorsCnt;
      logmessage_fmt(LOGLEVEL_ERR, con_fmt, arg, anum);
      break;

    case FATAL:
      ++globFatalCnt;
      color = FATAL_COLOR;
      if (isCountMessages())
        ++fatalsCnt;
      logmessage_fmt(LOGLEVEL_ERR, con_fmt, arg, anum);
      break;

    default:
      logmessage_fmt(LOGLEVEL_DEBUG, con_fmt, arg, anum);
      if (isCountMessages())
        ++notesCnt;
  }
  if (!is_main_thread())
    return;

  unsigned logEnd = ::GetWindowTextLength(logWnd);

  if (logEnd)
  {
    addTextToLog("\n", color, &logEnd);
    ++logEnd;
  }

  String msg;
  msg.vprintf(0, fmt, arg, anum);
  addTextToLog(msg, color, &logEnd);

  if (isProgress())
    ::UpdateWindow(logWnd);
}
void CoolConsole::puts(const char *str, console::LineType type)
{
  if (limitOutputLinesLeft == 0)
    return;

  unsigned color = 0;
  switch (type)
  {
    case console::CONSOLE_INFO: color = REMARK_COLOR; break;
    case console::CONSOLE_WARNING: color = WARNING_COLOR; break;
    case console::CONSOLE_ERROR: color = ERROR_COLOR; break;
  }

  unsigned logEnd = ::GetWindowTextLength(logWnd);
  if (logEnd)
  {
    addTextToLog("\n", color, &logEnd);
    ++logEnd;
  }
  addTextToLog(str, color, &logEnd);
  if (--limitOutputLinesLeft == 0)
    addTextToLog("\n... too many lines, output omitted (see debug for full log)...\n", WARNING_COLOR);
}

//==============================================================================
bool CoolConsole::initConsole()
{
  if (hWnd)
    return true;

  if (!registerWndClass())
    return false;

  const int sw = ::GetSystemMetrics(SM_CXSCREEN);
  const int sh = ::GetSystemMetrics(SM_CYSCREEN);
  const HINSTANCE inst = ::GetModuleHandle(NULL);

  wchar_t wcs_tmp[256];
  hWnd = ::CreateWindowExW(/*WS_EX_TOPMOST*/ 0, (const wchar_t *)(uintptr_t)MAKELONG(wndClassId, 0),
    utf8_to_wcs(caption, wcs_tmp, countof(wcs_tmp)), WS_OVERLAPPEDWINDOW, sw / 2, 100, sw / 2 - 10, sh / 2, parentWnd, NULL, inst,
    NULL);

  if (!hWnd)
    return false;

  ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

  logWnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, NULL,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | ES_READONLY | ES_MULTILINE, 0, 0, 10, 10, hWnd, NULL, inst, NULL);
  ::SetWindowLongPtr(logWnd, GWLP_USERDATA, (LONG_PTR)this);
  origLogWndProc = (WNDPROC)SetWindowLongPtr(logWnd, GWLP_WNDPROC, (LONG_PTR)logWndProc);

  if (!logWnd)
  {
    destroyConsole();
    return false;
  }

  ::SendMessage(logWnd, WM_SETFONT, (WPARAM)conFont, 0);

  progressWnd = ::CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 10, 10, hWnd, NULL, inst, NULL);

  if (!progressWnd)
  {
    destroyConsole();
    return false;
  }

  cancelBtn = ::CreateWindow("BUTTON", "Stop", WS_CHILD, 0, 0, 10, 10, hWnd, (HMENU)CANCEL_BUTTON_ID, inst, NULL);

  if (!cancelBtn)
  {
    destroyConsole();
    return false;
  }

  ::SendMessage(cancelBtn, WM_SETFONT, (WPARAM)interfaceFont, 0);

  cmdWnd = ::CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ES_AUTOHSCROLL | ES_MULTILINE | ES_WANTRETURN, 0, 0, 10, 10, hWnd, NULL, inst, NULL);

  if (!cmdWnd)
  {
    destroyConsole();
    return false;
  }

  ::SendMessage(cmdWnd, WM_SETFONT, (WPARAM)conFont, 0);

  cmdListWnd = ::CreateWindow("LISTBOX", NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LBS_HASSTRINGS | LBS_NOTIFY, 0, 0, 10, 10,
    hWnd, NULL, inst, NULL);

  if (!cmdListWnd)
  {
    destroyConsole();
    return false;
  }

  ::SendMessage(cmdListWnd, WM_SETFONT, (WPARAM)conFont, 0);

  alignControls();
  addMessage(REMARK, ABOUT_MESSAGE);

  ::SetWindowLongPtr(cmdWnd, GWLP_USERDATA, (LONG_PTR)this);
  origEditboxWndProc = (WNDPROC)SetWindowLongPtr(cmdWnd, GWLP_WNDPROC, (LONG_PTR)editboxWndProc);
  return true;
}


//==============================================================================
void CoolConsole::destroyConsole()
{
  if (cmdWnd)
    SetWindowLongPtr(cmdWnd, GWLP_WNDPROC, (LONG_PTR)origEditboxWndProc);
  DESTROY_WND(hWnd);
  DESTROY_WND(logWnd);
  DESTROY_WND(progressWnd);
  DESTROY_WND(cancelBtn);
  DESTROY_WND(cmdWnd);
  DESTROY_WND(cmdListWnd);
}


//==============================================================================
void CoolConsole::showConsole(bool activate)
{
  if (!hWnd)
    return;

  //::ShowWindow(hWnd, activate ? SW_SHOW : ::IsWindowVisible(hWnd) ? SW_SHOWNA : SW_SHOWNOACTIVATE);
  ::ShowWindow(hWnd, SW_RESTORE | SW_SHOWNORMAL);
  ::ShowWindow(hWnd, activate ? SW_SHOW : SW_SHOWNA);
  ::BringWindowToTop(hWnd);
  ::SetFocus(cmdWnd);
}


//==============================================================================
void CoolConsole::hideConsole() const
{
  if (!hWnd)
    return;

  ::ShowWindow(hWnd, SW_HIDE);
}


//==============================================================================
void CoolConsole::clearConsole() const
{
  if (!hWnd)
    return;

  ::SetWindowText(logWnd, NULL);
}

void CoolConsole::saveToFile(const char *file_name) const
{
  if (!hWnd)
    return;
  FILE *fp = fopen(file_name, "wb");
  if (fp)
  {
    unsigned textLen = ::GetWindowTextLength(logWnd);
    String data;
    data.resize(textLen + 1);
    ::GetWindowText(logWnd, data.begin(), textLen + 1);
    fwrite((char *)data, textLen, 1, fp);
    fclose(fp);
  }
  else
  {
    char msg[MAX_PATH + 100];
    strcpy(msg, "Could not save log to file \"");
    strcat(msg, file_name);
    strcat(msg, "\".");
    MessageBox(hWnd, msg, "Save log error", MB_OK);
  }
}

//==============================================================================
void CoolConsole::moveOnBottom() const
{
  if (!parentWnd)
    hideConsole();
}


//==============================================================================
void CoolConsole::alignControls(bool align_progress_only)
{
  RECT clientRect;
  ::GetClientRect(hWnd, &clientRect);

  const int cw = clientRect.right;
  const int ch = clientRect.bottom;

  const int logH = ch - PROGRESS_HEIGHT - conFontHeight * 2;

  if (!align_progress_only)
  {
    ::MoveWindow(logWnd, 0, 0, cw, logH, FALSE);
    ::MoveWindow(progressWnd, 0, logH, cw, PROGRESS_HEIGHT, FALSE);
    ::MoveWindow(cmdWnd, 0, ch - EDIT_HEIGHT, cw, EDIT_HEIGHT, FALSE);

    const unsigned newLineCnt = ::SendMessage(logWnd, EM_GETLINECOUNT, 0, 0);
    const unsigned visLine = ::SendMessage(logWnd, EM_GETFIRSTVISIBLELINE, 0, 0);

    RECT logEditRect;
    ::SendMessage(logWnd, EM_GETRECT, 0, (LPARAM)&logEditRect);

    const unsigned windowLineCnt = (logEditRect.bottom - logEditRect.top) / conFontHeight;

    const int scrollCnt = newLineCnt - visLine - windowLineCnt + 1;

    ::SendMessage(logWnd, EM_LINESCROLL, 0, scrollCnt);
  }
  else
  {
    if (isProgress() && progressCb)
    {
      ::MoveWindow(progressWnd, 0, logH, cw - CANCEL_BUTTON_WIDTH - 1, PROGRESS_HEIGHT, FALSE);
      ::MoveWindow(cancelBtn, cw - CANCEL_BUTTON_WIDTH, logH, CANCEL_BUTTON_WIDTH, PROGRESS_HEIGHT, FALSE);
    }
    else
      ::MoveWindow(progressWnd, 0, logH, cw, PROGRESS_HEIGHT, FALSE);
  }

  ::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
}


//==============================================================================
bool CoolConsole::runCommand(const char *cmd)
{
  String command(tmpmem);

  if (cmd)
    command = cmd;
  else
  {
    const int cmdLen = (int)::SendMessage(cmdWnd, WM_GETTEXTLENGTH, 0, 0) + 1;

    command.resize(cmdLen);
    ::GetWindowText(cmdWnd, command.begin(), cmdLen);
    command[cmdLen - 1] = 0;

    ::SetWindowText(cmdWnd, NULL);
  }

  if (cmdHistoryLastUnfinished)
    cmdHistory.pop_back();
  for (int i = cmdHistory.size() - 1; i >= 0; i--)
    if (strcmp(cmdHistory[i], command) == 0)
      erase_items(cmdHistory, i, 1);
  cmdHistory.push_back() = command;
  cmdHistoryLastUnfinished = false;
  cmdHistoryPos = cmdHistory.size();

  if (strcmp(command, "clrhist") == 0)
  {
    cmdHistory.clear();
    cmdHistoryLastUnfinished = false;
    cmdHistoryPos = cmdHistory.size();
    addMessage(NOTE, "command history cleared!");
    return true;
  }
  Tab<const char *> params(tmpmem);
  const char *cmdName = NULL;

  const char *token = ::conSttrok(command, " \t");

  while (token)
  {
    if (*token)
    {
      if (cmdName)
        params.push_back(token);
      else
        cmdName = token;
    }

    token = ::conSttrok(NULL, " \t");
  }

  if (!cmdName)
    return false;

  unsigned logEnd = ::GetWindowTextLength(logWnd);

  if (logEnd)
  {
    addTextToLog("\n", 0, &logEnd);
    ++logEnd;
  }

  addTextToLog(cmdName, CMD_COLOR, &logEnd, true);
  logEnd += i_strlen(cmdName);

  for (int i = 0; i < params.size(); ++i)
  {
    addTextToLog(" ", PARAM_COLOR, &logEnd, true);
    ++logEnd;

    addTextToLog(params[i], PARAM_COLOR, &logEnd, true);
    logEnd += i_strlen(params[i]);
  }

  IConsoleCmd *handler = NULL;
  commands.get(String(cmdName), handler);

  limitOutputLinesLeft = 1024;
  if (handler && handler->onConsoleCommand(cmdName, params))
  {
    limitOutputLinesLeft = -1;
    return true;
  }
  for (auto &p : params)
    command.aprintf(0, " %s", p);
  if (console::command(command.c_str()))
  {
    limitOutputLinesLeft = -1;
    return true;
  }
  addTextToLog(String(32, "\nUnknown command \"%s\"", command.c_str()), ERROR_COLOR, &logEnd);
  limitOutputLinesLeft = -1;
  return false;
}


//==============================================================================
void CoolConsole::runHelp(const char *command)
{
  IConsoleCmd *handler = NULL;

  if (commands.get(String(command), handler))
  {
    const char *help = handler->onConsoleCommandHelp(command);
    if (help)
    {
      addTextToLog("\n");
      addTextToLog(help);
      return;
    }
  }

  addTextToLog(String(32, "\nUnknown command '%s'", command));
}


//==============================================================================
void CoolConsole::runCmdList()
{
  addTextToLog("\nRegistered commands:");

  unsigned logEnd = ::GetWindowTextLength(logWnd);

  for (int i = 0; i < cmdNames.size(); ++i)
  {
    addTextToLog("\n", 0, &logEnd);
    ++logEnd;

    addTextToLog(cmdNames[i], 0, &logEnd);
    logEnd += cmdNames[i].length();
  }
  console::CommandList mlist;
  console::get_command_list(mlist);
  char cmdText[384];
  debug("mlist.size() = %d", mlist.size());
  for (int i = 0; i < mlist.size(); ++i)
  {
    const console::CommandStruct &cmd = mlist[i];
    _snprintf(cmdText, sizeof(cmdText), "\n%s  ", cmd.command.str());
    cmdText[sizeof(cmdText) - 1] = 0;

    if (!cmd.argsDescription.empty())
    {
      int argDescLen = cmd.argsDescription.length();
      int leftFreeSpace = sizeof(cmdText) - strlen(cmdText) - 1;
      strncat(cmdText, cmd.argsDescription, min(argDescLen, leftFreeSpace));
    }
    else
    {
      for (int k = 1; k < cmd.minArgs; k++)
        strncat(cmdText, "<x> ", min(4, int(sizeof(cmdText) - strlen(cmdText) - 1)));
      for (int k = cmd.minArgs; k < cmd.maxArgs; k++)
        strncat(cmdText, "[x] ", min(4, int(sizeof(cmdText) - strlen(cmdText) - 1)));
    }
    if (cmd.description.length())
    {
      const char *separator = " -- ";
      int sepLen = strlen(separator);
      const char *clipEnd = "...";
      int clipLen = strlen(clipEnd);

      int descLen = cmd.description.length();
      int textLen = strlen(cmdText);
      int freeCells = sizeof(cmdText) - textLen - 1;

      if (freeCells >= sepLen + descLen)
      {
        strncat(cmdText, separator, sepLen);
        strncat(cmdText, cmd.description, descLen);
      }
      else if (freeCells > sepLen + clipLen)
      {
        strncat(cmdText, separator, sepLen);
        int clippedLen = freeCells - sepLen - clipLen;
        strncat(cmdText, cmd.description.c_str(), clippedLen);
        strncat(cmdText, clipEnd, clipLen);
      }
    }

    addTextToLog(cmdText, 0, &logEnd);
    logEnd += strlen(cmdText);
  }
}


//==============================================================================
void CoolConsole::addTextToLog(const char *text, unsigned color, const unsigned *pos, bool bold)
{
  CHARFORMAT fmt;
  memset(&fmt, 0, sizeof(fmt));

  fmt.cbSize = sizeof(fmt);
  fmt.dwMask = CFM_COLOR | CFM_BOLD;
  fmt.crTextColor = color;
  if (bold)
    fmt.dwEffects = CFE_BOLD;

  ::SendMessage(logWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&fmt);

  CHARRANGE cr;
  cr.cpMin = pos ? *pos : ::GetWindowTextLength(logWnd);
  cr.cpMax = cr.cpMin;

  ::SendMessage(logWnd, EM_EXSETSEL, 0, (LPARAM)&cr);
  ::SendMessage(logWnd, EM_REPLACESEL, 0, (LPARAM)text);
  ::SendMessage(logWnd, EM_SCROLL, SB_BOTTOM, 0);
}


//==============================================================================
void CoolConsole::startProgress(IProgressCB *progress_cb)
{
  if (!hWnd)
    return;

  progressCb = progress_cb;

  if (!isProgress())
  {
    lastVisible = isVisible();
    flags |= CC_PROGRESS;

    ::EnableWindow(cmdWnd, FALSE);
    ::EnableWindow(progressWnd, TRUE);
  }

  showConsole(false);

  ::ShowWindow(cancelBtn, progressCb ? SW_SHOW : SW_HIDE);
  alignControls(true);

  call_idle_cycle_seldom(true);
}


//==============================================================================
void CoolConsole::endProgress()
{
  if (!hWnd)
    return;

  flags &= ~CC_PROGRESS;

  ::SendMessage(progressWnd, PBM_SETRANGE, 0, 0);

  ::EnableWindow(cmdWnd, TRUE);
  ::EnableWindow(progressWnd, FALSE);

  if (progressCb)
  {
    ::ShowWindow(cancelBtn, SW_HIDE);
    alignControls(true);
  }

  progressCb = NULL;

  if (lastVisible)
    ::UpdateWindow(hWnd);
  else
  {
    hideConsole();
    call_idle_cycle_seldom(true);
  }
}


//==============================================================================
bool CoolConsole::registerCommand(const char *cmd, IConsoleCmd *handler)
{
  if (!cmd || !*cmd || !handler)
    return false;

  String command(cmd);
  _strlwr((char *)command);

  const bool ret = commands.add(command, handler);

  if (ret)
  {
    cmdNames.push_back(command);
    sort(cmdNames, &compareCmd);
  }
  else
    debug("[CoolConsole] Couldn't register command \"%s\" due to hash collision.", cmd);

  return ret;
}


//==============================================================================
bool CoolConsole::unregisterCommand(const char *cmd, IConsoleCmd *handler)
{
  if (!cmd || !*cmd)
    return false;

  IConsoleCmd *regHandler = NULL;
  String s_cmd(cmd);

  if (commands.get(s_cmd, regHandler))
  {
    if (regHandler == handler)
    {
      commands.erase(s_cmd);
      for (int i = 0; i < cmdNames.size(); ++i)
        if (!stricmp(cmdNames[i], cmd))
        {
          erase_items(cmdNames, i, 1);
          break;
        }
    }
    else
    {
      debug("[CoolConsole] Couldn't unregister command \"%s\" due to handlers collision.", cmd);
      return false;
    }
  }

  return true;
}


//==============================================================================
bool CoolConsole::onConsoleCommand(const char *cmd, dag::ConstSpan<const char *> params)
{
  if (!stricmp("help", cmd))
  {
    runHelp(params.size() ? params[0] : "help");
    return true;
  }
  else if (!stricmp("list", cmd))
  {
    if (params.empty())
    {
      runCmdList();
      return true;
    }
  }
  else if (!stricmp("clear", cmd))
  {
    clearConsole();
    return true;
  }

  return false;
}


//==============================================================================
const char *CoolConsole::onConsoleCommandHelp(const char *cmd)
{
  if (!stricmp("help", cmd))
    return "Type 'help <command name>' to take command help\n"
           "Type 'list' to take available commands list";
  else if (!stricmp("list", cmd))
    return "Type 'list' to take list of all known commands";
  else if (!stricmp("clear", cmd))
    return "Type 'clear' to completely clear console";

  return NULL;
}


//==============================================================================
void CoolConsole::getCmds(const char *prefix, Tab<String> &cmds) const
{
  if (prefix)
  {
    const int prefLen = i_strlen(prefix);
    const bool startSegment = false;
    for (int i = 0; i < cmdNames.size(); ++i)
    {
      if (cmdNames[i].length() >= prefLen)
      {
        if (!memcmp(prefix, cmdNames[i].begin(), prefLen))
          cmds.push_back(cmdNames[i]);
        else if (startSegment)
          return;
      }
      else if (startSegment)
        return;
    }
  }
  else
    cmds = cmdNames;
}


//==============================================================================
void CoolConsole::showCmdList()
{
  const int textLen = ::GetWindowTextLength(cmdWnd);
  String prefix;

  prefix.resize(textLen + 1);

  ::GetWindowText(cmdWnd, prefix.begin(), textLen);

  int i = prefix.length() - 1;

  if (i < 0 || prefix[i] != '\t')
    return;

  debug("prefix = %s", (const char *)prefix);

  while (i >= 0 && prefix[i] == '\t')
    prefix[i--] = 0;

  ::SetWindowText(cmdWnd, prefix);

  Tab<String> cmds(tmpmem);

  getCmds(prefix, cmds);

  RECT rect;
  ::GetWindowRect(cmdWnd, &rect);

  const int w = rect.right - rect.left;

  ::MoveWindow(cmdListWnd, rect.left, rect.bottom, w, 30, TRUE);
  ::ShowWindow(cmdListWnd, SW_SHOW);
}


//==============================================================================
void CoolConsole::setActionDescFmt(const char *desc, const DagorSafeArg *arg, int anum)
{
  addMessageFmt(NOTE, desc, arg, anum);
  ::UpdateWindow(hWnd);
  call_idle_cycle_seldom();
}


//==============================================================================
void CoolConsole::setTotal(int total_cnt)
{
  ::SendMessage(progressWnd, PBM_SETRANGE, 0, MAKELPARAM(0, total_cnt));
  ::UpdateWindow(progressWnd);
  call_idle_cycle_seldom(true);
}


//==============================================================================
void CoolConsole::setDone(int done_cnt)
{
  ::SendMessage(progressWnd, PBM_SETPOS, done_cnt, 0);
  ::UpdateWindow(progressWnd);
  call_idle_cycle_seldom(done_cnt == 0);
}


//==============================================================================
void CoolConsole::incDone(int inc)
{
  ::SendMessage(progressWnd, PBM_DELTAPOS, inc, 0);
  ::UpdateWindow(progressWnd);
  call_idle_cycle_seldom();
}
