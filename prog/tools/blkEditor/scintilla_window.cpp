// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scintilla/include/scintilla.h"
#include "scintilla/include/sciLexer.h"

#include "scintilla_window.h"
#include <util/dag_simpleString.h>
#include <libTools/util/blkUtil.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_debug.h>
#include <winGuiWrapper/wgw_input.h>

#include "windows.h"
#include <commctrl.h>
#include <time.h>


enum
{
  IDM_FILE_SAVE = 32800,
  IDM_FILE_NEW,
  IDM_FILE_OPEN,

  CM_EDIT_UNDO,
  CM_EDIT_REDO,

  CM_FIND,
  CM_REPLACE,
  CM_GO_TO_LINE,
};

const int WM_SC_NOTYFY = WM_USER + 101;
const char SCPARENT_WINDOW_CLASS_NAME[] = "ScParentWindowClass";
const unsigned long UPDATE_IGNORE_DELAY_MS = 500;
const unsigned UPDATE_TIME_SHIFT_MS = 300;


static const int MARGIN_SCRIPT_FOLD_INDEX = 1;
void *CScintillaWindow::sciLexerDll = NULL;


const char nutKeyWords[] = "break case catch class clone continue \
  default delegate delete else extends for \
  function if in local null resume \
  return switch this throw try typeof \
  while parent yield constructor vargc vargv \
  instanceof true false static";

LRESULT CALLBACK scintilla_accel_wnd_proc(HWND wnd, unsigned msg, WPARAM w_param, LPARAM l_param)
{
  HWND owner = GetParent(wnd);
  CScintillaWindow *editor = (CScintillaWindow *)GetWindowLongPtr((HWND)wnd, GWLP_USERDATA);
  if (!editor)
    return DefWindowProc(wnd, msg, w_param, l_param);

  if ((msg == WM_KEYDOWN || msg == WM_KEYUP))
  {
    int cmdId = -1;
    if (wingw::is_key_pressed(VK_CONTROL))
    {
      int keyMap[] = {'S', IDM_FILE_SAVE, 'O', IDM_FILE_OPEN, 'N', IDM_FILE_NEW, 'F', CM_FIND, 'H', CM_REPLACE, 'Z', CM_EDIT_UNDO, 'Y',
        CM_EDIT_REDO, 'G', CM_GO_TO_LINE};

      for (int i = 0; i < sizeof(keyMap) / sizeof(keyMap[0]); i += 2)
      {
        if (w_param == keyMap[i])
        {
          cmdId = keyMap[i + 1];
          break;
        }
      }
    }
    else
    {
      // if (w_param == VK_F9)
      //   cmdId = CM_BUILD_COMPILE;
    }

    if (cmdId >= 0)
    {
      MSG _msg;
      while (PeekMessage(&_msg, wnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) {}

      if (msg == WM_KEYDOWN || w_param == VK_F9)
        SendMessage(owner, WM_COMMAND, /*MAKEWORD(cmdId, 0)*/ cmdId, NULL);
      return 0;
    }
  }
  if (msg == WM_SC_NOTYFY)
  {
    editor->notify(reinterpret_cast<SCNotification *>(l_param));
    return 0;
  }
  return CallWindowProcW((WNDPROC)editor->scintilaDefProc, wnd, msg, w_param, l_param);
}


LRESULT CALLBACK parent_window_proc(HWND hwnd, unsigned message, WPARAM wparam, LPARAM lparam)
{
  NMHDR *lpnmhdr;
  if (message == WM_NOTIFY)
  {
    lpnmhdr = (LPNMHDR)lparam;
    SendMessage(lpnmhdr->hwndFrom, WM_SC_NOTYFY, lpnmhdr->code, lparam);
  }
  return DefWindowProc(hwnd, message, wparam, lparam);
}


void register_class()
{
  static bool wnd_classes_initialized = false;
  if (wnd_classes_initialized == true)
    return;

  WNDCLASSEX scWndCls = {0};

  scWndCls.cbSize = sizeof(WNDCLASSEX);
  scWndCls.hCursor = LoadCursor(NULL, IDC_ARROW);
  scWndCls.hIcon = LoadIcon(NULL, IDI_INFORMATION);
  scWndCls.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
  scWndCls.lpfnWndProc = (WNDPROC)parent_window_proc;
  scWndCls.hbrBackground = (HBRUSH)(COLOR_BTNSHADOW);
  scWndCls.hInstance = (HMODULE)GetModuleHandle(NULL);
  scWndCls.lpszClassName = TEXT(SCPARENT_WINDOW_CLASS_NAME);
  scWndCls.style = CS_PARENTDC;
  scWndCls.cbClsExtra = 0;
  scWndCls.cbWndExtra = 0;
  scWndCls.lpszMenuName = NULL;

  if (!RegisterClassEx(&scWndCls))
    G_ASSERT(false && "sc parent: Window Class not registered!");

  wnd_classes_initialized = true;
}


CScintillaWindow::CScintillaWindow(ScintillaEH *event_handler, void *phandle, int x, int y, hdpi::Px w, hdpi::Px h,
  const char caption[]) :
  hwndScintilla(NULL),
  hwndScParent(NULL),
  mParentHandle(phandle),
  scintilaDefProc(NULL),
  eventHandler(event_handler),
  lastSetTime(0),
  needCallModify(false)
{
  initScintilla();
  updateTimer = new WinTimer(this, UPDATE_TIME_SHIFT_MS, true);
}


CScintillaWindow::~CScintillaWindow()
{
  if (updateTimer)
    del_it(updateTimer);
}


void CScintillaWindow::initScintilla()
{
#if _TARGET_64BIT
  sciLexerDll = ::LoadLibrary("SciLexer64.DLL");
#else
  sciLexerDll = ::LoadLibrary("SciLexer.DLL");
#endif
  if (sciLexerDll == NULL)
    debug("[ScriptEditor::registerWndClass]Scintilla DLL \
    could not be loaded.");

  INITCOMMONCONTROLSEX initRec;

  initRec.dwSize = sizeof(INITCOMMONCONTROLSEX);
  initRec.dwICC = ICC_PROGRESS_CLASS;

  if (!InitCommonControlsEx(&initRec))
    debug("[ScriptEditor::registerWndClass] Unable to init common controls");

  const HINSTANCE inst = ::GetModuleHandle(NULL);

  register_class();
  hwndScParent = ::CreateWindowEx(0, SCPARENT_WINDOW_CLASS_NAME, "", WS_CHILD | WS_VISIBLE, 100, 100, 100, 100, (HWND)mParentHandle,
    NULL, inst, NULL);
  if (!hwndScParent)
    DAG_FATAL("Failed to create scintilla parent window");

  hwndScintilla = ::CreateWindowExW(0, L"Scintilla", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN, 100, 100, 100, 100,
    (HWND)hwndScParent, NULL, inst, NULL);
  if (!hwndScintilla)
    DAG_FATAL("Failed to create scintilla window");

  ::SetWindowLongPtr((HWND)hwndScintilla, GWLP_USERDATA, (LONG_PTR)this);

  scintilaDefProc = (void *)GetWindowLongPtr((HWND)hwndScintilla, GWLP_WNDPROC);
  G_ASSERT(scintilaDefProc);
  SetWindowLongPtrW((HWND)hwndScintilla, GWLP_WNDPROC, (LONG_PTR)(WNDPROC)scintilla_accel_wnd_proc);
  setupScintilla();
  ::SetFocus((HWND)hwndScintilla);
}


int CScintillaWindow::sendEditor(unsigned Msg, uintptr_t wParam, intptr_t lParam)
{
  return ::SendMessage((HWND)hwndScintilla, Msg, wParam, lParam);
}


void CScintillaWindow::setAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
{
  sendEditor(SCI_STYLESETFORE, style, fore);
  sendEditor(SCI_STYLESETBACK, style, back);
  if (size >= 1)
    sendEditor(SCI_STYLESETSIZE, style, size);
  if (face)
    sendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}


void CScintillaWindow::setupScintilla()
{
  COLORREF black = 0;

  sendEditor(SCI_SETLEXER, SCLEX_RUBY);
  // sendEditor(SCI_SETSTYLEBITS, 7);
  sendEditor(SCI_SETSTYLEBITS, 5);
  sendEditor(SCI_SETTABWIDTH, 2);
  sendEditor(SCI_SETINDENT, 0); //< same as tab width
  sendEditor(SCI_SETUSETABS, 0);
  // sendEditor(SCI_SETINDENTATIONGUIDES, 1);

  // folding
  sendEditor(SCI_SETPROPERTY, (WPARAM) "fold", (LPARAM) "1");
  sendEditor(SCI_SETPROPERTY, (WPARAM) "fold.compact", (LPARAM) "0");
  sendEditor(SCI_SETPROPERTY, (WPARAM) "fold.comment", (LPARAM) "1");
  sendEditor(SCI_SETPROPERTY, (WPARAM) "fold.preprocessor", (LPARAM) "1");
  // Now resize all the margins to zero
  //(This will be done in a RecalcLineMargin method...)
  sendEditor(SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 0);
  // Then set the margin type and margin mask and resize it...
  sendEditor(SCI_SETMARGINTYPEN, MARGIN_SCRIPT_FOLD_INDEX, SC_MARGIN_SYMBOL);
  sendEditor(SCI_SETMARGINMASKN, MARGIN_SCRIPT_FOLD_INDEX, SC_MASK_FOLDERS);
  sendEditor(SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 20);
  // Set some visual preferences
  const COLORREF foldCol = RGB(0xC0, 0xD0, 0xF0);
  const COLORREF red = RGB(0xFF, 0x0, 0x0);


  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_CIRCLEPLUS);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_CIRCLEMINUS);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNERCURVE);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_CIRCLEPLUSCONNECTED);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_CIRCLEMINUSCONNECTED);
  sendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNERCURVE);


  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDER, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERSUB, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERMIDTAIL, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEREND, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPENMID, foldCol);
  sendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERTAIL, foldCol);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDER, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEREND, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPENMID, black);
  sendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, black);
  // sendEditor(SCI_MARKERSETBACK, int colour)


  sendEditor(SCI_SETFOLDFLAGS, 16, 0); // 16  Draw line below if not expanded
  // Tell scintilla we want to be notified about mouse clicks in the margin
  sendEditor(SCI_SETMARGINSENSITIVEN, MARGIN_SCRIPT_FOLD_INDEX, 1);

  //  String sceneKeyWords;
  //  for (int i=0; i<names.size(); i++)
  //  {
  //    sceneKeyWords += names[i] + " ";
  //  }
  sendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(nutKeyWords));
  //  sendEditor(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>((char*)sceneKeyWords));

  setAStyle(STYLE_DEFAULT, RGB(0, 0, 0), RGB(0xFF, 0xFF, 0xFF), 10, "Courier New");
  sendEditor(SCI_STYLECLEARALL);

  const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
  const COLORREF darkGreen = RGB(0, 0x80, 0);
  const COLORREF darkBlue = RGB(0, 0, 0x80);
  const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);

  // c default is used for all the document's text
  setAStyle(SCE_C_DEFAULT, black, RGB(255, 255, 255), 11, "Courier New");
  setAStyle(SCE_C_COMMENT, darkGreen);
  setAStyle(SCE_C_COMMENTLINE, darkGreen);
  setAStyle(SCE_C_COMMENTDOC, RGB(0x99, 0xDD, 0));
  setAStyle(SCE_C_NUMBER, RGB(0, 0x80, 0x80));
  setAStyle(SCE_C_WORD, RGB(0, 0, 0xFF));
  setAStyle(SCE_C_STRING, lightBlue);
  setAStyle(SCE_C_CHARACTER, lightBlue);
  setAStyle(SCE_C_UUID, red);
  setAStyle(SCE_C_PREPROCESSOR, darkBlue);
  setAStyle(SCE_C_OPERATOR, red);
  setAStyle(SCE_C_IDENTIFIER, darkBlue);
  setAStyle(SCE_C_STRINGEOL, lightBlue);
  setAStyle(SCE_C_VERBATIM, lightBlue);
  setAStyle(SCE_C_REGEX, darkBlue);
  setAStyle(SCE_C_COMMENTLINEDOC, darkGreen);
  setAStyle(SCE_C_WORD2, RGB(0xDD, 0x99, 0));
  setAStyle(SCE_C_COMMENTDOCKEYWORD, lightBlue);
  setAStyle(SCE_C_COMMENTDOCKEYWORDERROR, RGB(0x99, 0xDD, 0));
  setAStyle(SCE_C_GLOBALCLASS, darkGreen);

  sendEditor(SCI_STYLESETHOTSPOT, SCE_C_WORD2, 1);
  sendEditor(SCI_STYLESETHOTSPOT, SCE_C_COMMENTDOC, 1);
  sendEditor(SCI_SETHOTSPOTACTIVEFORE, 1, RGB(0x80, 0x80, 0));
  sendEditor(SCI_SETHOTSPOTACTIVEBACK, 1, RGB(0xFF, 0xFF, 0));
  sendEditor(SCI_SETHOTSPOTACTIVEUNDERLINE, 1);
  sendEditor(SCI_SETHOTSPOTSINGLELINE, 1);

  sendEditor(SCI_AUTOCSETIGNORECASE, 1);
  sendEditor(SCI_AUTOCSETMAXHEIGHT, 20);
}


void CScintillaWindow::enableWindow(bool enable)
{
  ::EnableWindow((HWND)hwndScParent, enable);
  ::EnableWindow((HWND)hwndScintilla, enable);
}


void CScintillaWindow::moveWindow(int x, int y, int width, int height)
{
  ::MoveWindow((HWND)hwndScParent, x, y, width, height, TRUE);
  ::MoveWindow((HWND)hwndScintilla, 0, 0, width, height, TRUE);
}


bool CScintillaWindow::isWindowFocused() const
{
  const HWND focused = ::GetFocus();
  return focused == (HWND)hwndScParent || focused == (HWND)hwndScintilla;
}


void CScintillaWindow::loadFromDataBlock(const DataBlock &blk)
{
  SimpleString str = blk_util::blkTextData(blk);
  setText(str.str());
}


bool CScintillaWindow::saveToDataBlock(DataBlock *blk)
{
  SimpleString text;
  getText(text);
  bool res = blk->loadText(text.str(), text.length());
  return res;
}


void CScintillaWindow::getText(SimpleString &text)
{
  if (!hwndScintilla)
    return;

  int len = sendEditor(SCI_GETLENGTH);
  char *data = new char[len + 1];

  TextRange tr;
  tr.chrg.cpMin = 0;
  tr.chrg.cpMax = len;
  tr.lpstrText = data;

  SendMessage((HWND)hwndScintilla, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
  text = data;
  delete[] data;
}


void CScintillaWindow::setText(const char *text)
{
  lastSetTime = GetTickCount();
  sendEditor(SCI_SETTEXT, 0, (LPARAM)text);
}


void CScintillaWindow::notify(SCNotification *notification)
{
  switch (notification->nmhdr.code)
  {
    case SCN_MARGINCLICK:
    {
      const int modifiers = notification->modifiers;
      const int position = notification->position;
      const int margin = notification->margin;
      const int line_number = sendEditor(SCI_LINEFROMPOSITION, position, 0);
      switch (margin)
      {
        case MARGIN_SCRIPT_FOLD_INDEX: sendEditor(SCI_TOGGLEFOLD, line_number, 0); break;
      }
      break;
    }
    case SCN_CHARADDED:
      if (notification->ch == '\n')
      {
        int pos = sendEditor(SCI_GETCURRENTPOS, 0, 0);
        int cur_line_index = sendEditor(SCI_LINEFROMPOSITION, pos, 0);
        if (cur_line_index > 0)
        {
          int ident = sendEditor(SCI_GETLINEINDENTATION, cur_line_index - 1, 0);
          if (ident < 32)
          {
            char itext[32];
            memset(itext, ' ', ident);
            itext[ident] = '\0';
            sendEditor(SCI_INSERTTEXT, pos, (uintptr_t)itext);
            sendEditor(SCI_SETCURRENTPOS, pos + ident, 0);
            sendEditor(SCI_SETANCHOR, pos + ident, 0);
          }
        }
      }
      break;

    case SCN_MODIFIED: onEditorChange(); break;
  }
}


void CScintillaWindow::onEditorChange()
{
  if (eventHandler)
  {
    if (GetTickCount() - lastSetTime < UPDATE_IGNORE_DELAY_MS)
      return;
    needCallModify = true;
  }
}


void CScintillaWindow::update()
{
  if (needCallModify)
  {
    eventHandler->onModify();
    needCallModify = false;
  }
}


int CScintillaWindow::getCursorPos() { return sendEditor(SCI_GETCURRENTPOS); }


void CScintillaWindow::setCursorPos(int new_pos) { sendEditor(SCI_GOTOPOS, new_pos); }


int CScintillaWindow::getScrollPos() { return sendEditor(SCI_GETFIRSTVISIBLELINE); }


void CScintillaWindow::setScrollPos(int new_pos) { sendEditor(SCI_LINESCROLL, 0, new_pos); }


void CScintillaWindow::getFoldingInfo(DataBlock &blk)
{
  blk.reset();
  int fold_level = SC_FOLDLEVELBASE;
  int line_count = sendEditor(SCI_GETLINECOUNT);
  DataBlock *cur_block = &blk;
  Tab<DataBlock *> blk_stack(tmpmem);
  blk_stack.push_back(&blk);

  for (int line = 0; line < line_count; ++line)
  {
    int is_folder = sendEditor(SCI_GETFOLDLEVEL, line) & SC_FOLDLEVELHEADERFLAG;
    int line_fold_level = sendEditor(SCI_GETFOLDLEVEL, line) & SC_FOLDLEVELNUMBERMASK;

    if (is_folder)
    {
      while (line_fold_level < fold_level && blk_stack.size() > 0)
      {
        cur_block = blk_stack.back();
        blk_stack.pop_back();
        --fold_level;
      }

      if (line_fold_level > fold_level)
      {
        blk_stack.push_back(cur_block);
        fold_level = line_fold_level;
      }
      else if (line_fold_level == fold_level && blk_stack.size() > 0)
        cur_block = blk_stack.back();

      cur_block = cur_block->addNewBlock("fold");
      cur_block->setBool("exp", sendEditor(SCI_GETFOLDEXPANDED, line));
    }
  }
}


void CScintillaWindow::setFoldingInfo(const DataBlock &blk)
{
  sendEditor(SCI_COLOURISE, 0, -1);
  int fold_level = SC_FOLDLEVELBASE;
  int line_count = sendEditor(SCI_GETLINECOUNT);
  const DataBlock *cur_block = &blk;
  Tab<const DataBlock *> blk_stack(tmpmem);
  Tab<int> blk_counters(tmpmem);
  blk_stack.push_back(&blk);
  blk_counters.push_back(-1);

  for (int line = 0; line < line_count; ++line)
  {
    int is_folder = sendEditor(SCI_GETFOLDLEVEL, line) & SC_FOLDLEVELHEADERFLAG;
    int line_fold_level = sendEditor(SCI_GETFOLDLEVEL, line) & SC_FOLDLEVELNUMBERMASK;

    if (is_folder)
    {
      while (line_fold_level < fold_level && blk_stack.size() > 0 && blk_counters.size() > 0)
      {
        cur_block = blk_stack.back();
        blk_stack.pop_back();
        blk_counters.pop_back();
        --fold_level;
      }

      int start_block = -1;
      if (line_fold_level > fold_level)
      {
        blk_stack.push_back(cur_block);
        blk_counters.push_back(0);
        fold_level = line_fold_level;
      }
      else if (line_fold_level == fold_level && blk_stack.size() > 0 && blk_counters.size() > 0)
      {
        cur_block = blk_stack.back();
        start_block = blk_counters.back();
        ++blk_counters.back();
      }

      if (cur_block)
        cur_block = cur_block->getBlockByName("fold", start_block);
      if (cur_block && !cur_block->getBool("exp", true))
        sendEditor(SCI_TOGGLEFOLD, line);
    }
  }
}
