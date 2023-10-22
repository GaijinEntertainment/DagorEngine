//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <propPanel2/c_panel_base.h>

enum
{
  DIALOG_ID_NONE = 0,
  DIALOG_ID_OK,
  DIALOG_ID_CANCEL,
  DIALOG_ID_CLOSE,
  DIALOG_ID_YES = 6,
  DIALOG_ID_NO = 7,
};

class DialogWindowEventHandler;
class WWindow;
class CDialogWindow;
class CPanelWindow;
struct DialogDisableHandles;


class DialogButtonsHandler : public ControlEventHandler
{
public:
  DialogButtonsHandler(CDialogWindow *p_dialog);

  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  bool getCanDelete();
  CDialogWindow *getDialog() { return mpDialog; }

private:
  CDialogWindow *mpDialog;
};


class CDialogWindow : public ControlEventHandler
{
public:
  CDialogWindow(void *phandle, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel = false);

  CDialogWindow(void *phandle, int x, int y, hdpi::Px w, hdpi::Px h, const char caption[], bool hide_panel = false);

  virtual ~CDialogWindow();

  virtual PropertyContainerControlBase *getPanel();
  virtual void *getHandle();

  virtual int showDialog();
  virtual void show();
  virtual void hide(int result = DIALOG_ID_NONE);
  virtual void showButtonPanel(bool show = true);
  virtual bool isVisible() { return mIsVisible; }
  virtual bool isDeleting() { return mDeleting; } // for correct destruction

  virtual void moveWindow(int x, int y);
  virtual void resizeWindow(hdpi::Px w, hdpi::Px h, bool internal = false);
  virtual void center();
  virtual void autoSize();

  virtual int getScrollPos();
  virtual void setScrollPos(int pos);

  // events
  virtual bool onOk() { return true; }
  virtual bool onCancel() { return true; }
  virtual bool onClose() { return onCancel(); }

  virtual int closeReturn() { return DIALOG_ID_CANCEL; }

  void setDragAcceptFiles(bool accept = true);

  // Return true if it has been handled.
  virtual bool onDropFiles(const dag::Vector<String> &files) { return false; }

  // ControlEventHandler methods
  virtual long onChanging(int, PropPanel2 *) { return 0; }
  virtual void onChange(int, PropPanel2 *) {}
  virtual void onClick(int, PropPanel2 *) {}
  virtual void onResize(int, PropPanel2 *) {}

  inline void setCloseHandler(ControlEventHandler *_ceh) { ceh = _ceh; }

protected:
  void buttonsToWidth();
  void click(int id);

  DialogButtonsHandler mButtonsEventHandler;
  DialogWindowEventHandler *mWindowHandler;
  WWindow *mDialogWindow;
  CPanelWindow *mPropertiesPanel, *mButtonsPanel;
  DialogDisableHandles *mDHandles;
  ControlEventHandler *ceh;

  void *mpHandle;
  bool mIsModal;
  int mDResult;
  bool mIsVisible;
  bool mDeleting;
  bool mButtonsVisible;

private:
  void create(unsigned w, unsigned h, bool hide_panel);
};
