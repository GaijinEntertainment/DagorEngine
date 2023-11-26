// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/comWnd/dialog_window.h>
#include <propPanel2/comWnd/panel_window.h>

#include "../windowControls/w_simple_controls.h"
#include "../windowControls/w_window.h"
#include <winGuiWrapper/wgw_busy.h>

#include <windows.h>
#include <generic/dag_tab.h>

struct DialogDisableHandles
{
  DialogDisableHandles::DialogDisableHandles() : mDisabledHandles(midmem) {}

  Tab<void *> mDisabledHandles;
  void *mHandle;
};


///////////////////////////////////////////////////////////////////////////////


DialogButtonsHandler::DialogButtonsHandler(CDialogWindow *p_dialog) : mpDialog(p_dialog) {}


void DialogButtonsHandler::onClick(int pcb_id, PropertyContainerControlBase *panel)
{
  switch (pcb_id)
  {
    case DIALOG_ID_OK:

      if ((mpDialog) && (mpDialog->onOk()))
      {
        mpDialog->hide(pcb_id);
      }
      break;

    case DIALOG_ID_CANCEL:

      if ((mpDialog) && (mpDialog->onCancel()))
      {
        mpDialog->hide(pcb_id);
      }
      break;

    case DIALOG_ID_CLOSE:

      if ((mpDialog) && (mpDialog->onClose())) // push cancel by default
      {
        mpDialog->hide(mpDialog->closeReturn());
      }
      break;

    default: break;
  }
}

bool DialogButtonsHandler::getCanDelete() { return mpDialog->isDeleting(); }

//---------------------------------------------------------

class DialogWindowEventHandler : public WindowControlEventHandler
{
public:
  DialogWindowEventHandler(DialogButtonsHandler &dbh) : mDbh(dbh) {}

  virtual bool onWcClose(WindowBase *source)
  {
    mDbh.onClick(DIALOG_ID_CLOSE, NULL);
    return mDbh.getCanDelete();
  }

  virtual long onDropFiles(WindowBase *source, const dag::Vector<String> &files) override
  {
    CDialogWindow *dialog = mDbh.getDialog();
    return (dialog && dialog->onDropFiles(files)) ? 1 : 0;
  }

private:
  DialogButtonsHandler &mDbh;
};


//---------------------------------------------------------

CDialogWindow::CDialogWindow(void *phandle, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel) :

  mButtonsEventHandler(this),
  mDialogWindow(NULL),
  mPropertiesPanel(NULL),
  mButtonsPanel(NULL),
  mpHandle(phandle),
  mInitialFocusId(DIALOG_ID_OK),
  mIsModal(false),
  mDResult(DIALOG_ID_NONE),
  ceh(NULL),
  mIsVisible(false),
  mDeleting(false),
  mButtonsVisible(true)
{
  // creation

  mWindowHandler = new DialogWindowEventHandler(mButtonsEventHandler);
  mDialogWindow = new WWindow(mWindowHandler, 0, (GetSystemMetrics(SM_CXSCREEN) - _px(w)) / 2,
    (GetSystemMetrics(SM_CYSCREEN) - _px(h)) / 2, _px(w), _px(h), caption, true);

  create(_px(w), _px(h), hide_panel);
}


CDialogWindow::CDialogWindow(void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel) :

  mButtonsEventHandler(this),
  mDialogWindow(NULL),
  mPropertiesPanel(NULL),
  mButtonsPanel(NULL),
  mpHandle(phandle),
  mInitialFocusId(DIALOG_ID_OK),
  mIsModal(false),
  mDResult(DIALOG_ID_NONE),
  ceh(NULL),
  mIsVisible(false),
  mDeleting(false),
  mButtonsVisible(true)
{
  // creation

  mWindowHandler = new DialogWindowEventHandler(mButtonsEventHandler);
  mDialogWindow = new WWindow(mWindowHandler, 0, x, y, _px(w), _px(h), caption, true);

  create(_px(w), _px(h), hide_panel);
}


CDialogWindow::~CDialogWindow()
{
  mDeleting = true;
  del_it(mPropertiesPanel);
  del_it(mButtonsPanel);
  del_it(mDialogWindow);
  del_it(mWindowHandler);
  del_it(mDHandles);
}


void CDialogWindow::create(unsigned w, unsigned h, bool hide_panel)
{
  if (!mpHandle)
    mpHandle = GetFocus();

  if (!mpHandle)
    mpHandle = GetForegroundWindow();

  mPropertiesPanel = new CPanelWindow(this, mDialogWindow->getHandle(), 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO, "");

  if (hide_panel)
    ShowWindow((HWND)mPropertiesPanel->getWindowHandle(), FALSE);
  // mPropertiesPanel->showPanel(!hide_panel);

  mButtonsPanel = new CPanelWindow(&mButtonsEventHandler, mDialogWindow->getHandle(), 0, 0, hdpi::Px::ZERO, hdpi::Px::ZERO, "");

  // styles

  // SetWindowLong((HWND) mPropertiesPanel->getWindowHandle(),
  //   GWL_EXSTYLE, WS_EX_STATICEDGE);

  SetWindowLong((HWND)mDialogWindow->getHandle(), GWL_STYLE, GetWindowLong((HWND)mDialogWindow->getHandle(), GWL_STYLE) | WS_SYSMENU);

  mDialogWindow->hide();

  // sizing and move

  resizeWindow(_pxActual(w), _pxActual(h), true);

  // controls

  mButtonsPanel->createButton(DIALOG_ID_OK, "Ok");
  mButtonsPanel->createButton(DIALOG_ID_CANCEL, "Cancel", true, false);

  // for windows disabling

  mDHandles = new DialogDisableHandles();
  mDHandles->mHandle = mDialogWindow->getHandle();
}


void CDialogWindow::showButtonPanel(bool show)
{
  mButtonsPanel->showPanel(show);
  mButtonsVisible = show;
  CDialogWindow::resizeWindow(_pxActual(mDialogWindow->getWidth()), _pxActual(mDialogWindow->getHeight()), true);
}


void CDialogWindow::moveWindow(int x, int y) { mDialogWindow->moveWindow(x, y); }


void CDialogWindow::resizeWindow(hdpi::Px w, hdpi::Px h, bool internal)
{
  if (!internal)
    mDialogWindow->resizeWindow(_px(w), _px(h));

  int cw, ch;
  mDialogWindow->getClientSize(cw, ch);

  mPropertiesPanel->setWidth(_pxActual(cw));
  mPropertiesPanel->setHeight(_pxActual((mButtonsVisible) ? ch - _pxS(DIALOG_BUTTONS_PANEL_HEIGHT) : ch));

  mButtonsPanel->moveTo((cw - _pxS(DIALOG_BUTTONS_PANEL_WIDTH)) / 2, (mButtonsVisible) ? ch - _pxS(DIALOG_BUTTONS_PANEL_HEIGHT) : ch);
  mButtonsPanel->setWidth(_pxScaled(DIALOG_BUTTONS_PANEL_WIDTH));
  mButtonsPanel->setHeight(_pxScaled(DIALOG_BUTTONS_PANEL_HEIGHT));
}


void CDialogWindow::center()
{
  int _x = (GetSystemMetrics(SM_CXSCREEN) - mDialogWindow->getWidth()) / 2;
  int _y = (GetSystemMetrics(SM_CYSCREEN) - mDialogWindow->getHeight()) / 2;

  mDialogWindow->moveWindow(_x, _y);
}


void CDialogWindow::autoSize(bool auto_center)
{
  if (!mPropertiesPanel)
    return;

  const int MAX_HT = GetSystemMetrics(SM_CYSCREEN) - _pxS(120);

  int delta = _px(mPropertiesPanel->getClientHeight()) - mPropertiesPanel->getHeight();
  if (delta != 0)
  {
    int new_ht = mDialogWindow->getHeight() + delta;
    new_ht = (new_ht > MAX_HT) ? MAX_HT : new_ht;
    resizeWindow(_pxActual(mDialogWindow->getWidth()), _pxActual(new_ht));
  }

  if (auto_center)
    center();
}

int CDialogWindow::getScrollPos() { return mPropertiesPanel->getScrollPos(); }


void CDialogWindow::setScrollPos(int pos) { mPropertiesPanel->setScrollPos(pos); }

void CDialogWindow::buttonsToWidth()
{
  int cw, ch;
  mDialogWindow->getClientSize(cw, ch);

  mButtonsPanel->moveTo(0, ch - _pxS(DIALOG_BUTTONS_PANEL_HEIGHT));
  mButtonsPanel->setWidth(_pxActual(cw));
  mButtonsPanel->setHeight(_pxScaled(DIALOG_BUTTONS_PANEL_HEIGHT));
}


PropertyContainerControlBase *CDialogWindow::getPanel() { return mPropertiesPanel->getContainer(); }


void *CDialogWindow::getHandle() { return mDialogWindow->getHandle(); }


BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  if (!lParam)
    return false;

  DialogDisableHandles *ddh = (DialogDisableHandles *)lParam;

  if (((HWND)ddh->mHandle != hwnd) && (IsWindowEnabled(hwnd)) && (IsWindowVisible(hwnd)))
  {
    EnableWindow(hwnd, false);
    ddh->mDisabledHandles.push_back(hwnd);
  }

  return true;
}


void CDialogWindow::click(int id) { mButtonsEventHandler.onClick(id, NULL); }


int CDialogWindow::showDialog()
{
  bool was_busy = wingw::set_busy(false);
  clear_and_shrink(mDHandles->mDisabledHandles);
  EnumThreadWindows(GetCurrentThreadId(), &EnumWindowsProc, (LPARAM)mDHandles);

  show();

  mIsModal = true;
  MSG msg;
  mDResult = DIALOG_ID_NONE;

  while ((mDResult == DIALOG_ID_NONE) && (GetMessageW(&msg, NULL, 0, 0)))
  {
    // process ENTER, ESC

    if (msg.message == WM_KEYDOWN)
    {
      switch (msg.wParam)
      {
        case VK_ESCAPE: click(DIALOG_ID_CLOSE); break;

        case VK_RETURN:
          LONG props = ::GetWindowLong(msg.hwnd, GWL_STYLE);
          if (!(props & ES_WANTRETURN))
            click(DIALOG_ID_OK);
          break;
      }
    }

    // stop program path here until dialog hide

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  wingw::set_busy(was_busy);
  return mDResult;
}


void CDialogWindow::show()
{
  mDialogWindow->show();

  if (mInitialFocusId == DIALOG_ID_OK || mInitialFocusId == DIALOG_ID_CANCEL)
  {
    SetFocus((HWND)mPropertiesPanel->getWindowHandle());
    mButtonsPanel->setFocusById(mInitialFocusId);
  }

  mIsModal = false;
  mIsVisible = true;
}


void CDialogWindow::hide(int result)
{
  if (result != DIALOG_ID_NONE)
  {
    mDResult = result;
  }
  if (ceh)
    ceh->onClick(-DIALOG_ID_CLOSE, NULL);

  if (mIsModal)
  {
    int c = mDHandles->mDisabledHandles.size();
    for (int i = 0; i < c; ++i)
    {
      EnableWindow((HWND)mDHandles->mDisabledHandles[i], true);
    }

    clear_and_shrink(mDHandles->mDisabledHandles);
  }

  mDialogWindow->hide();
  mIsVisible = false;
  SetFocus((HWND)mpHandle);
}

void CDialogWindow::setDragAcceptFiles(bool accept) { mDialogWindow->setDragAcceptFiles(accept); }