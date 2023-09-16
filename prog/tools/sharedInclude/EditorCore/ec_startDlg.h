#ifndef __GAIJIN_EDITORCORE_START_DIALOG__
#define __GAIJIN_EDITORCORE_START_DIALOG__
#pragma once

#include <propPanel2/comWnd/dialog_window.h>

class EditorWorkspace;


// dialog control IDs

enum
{
  ID_START_DIALOG_COMBO = 400,
  ID_START_DIALOG_BUTTON_ADD,
  ID_START_DIALOG_BUTTON_EDIT,
};


class EditorStartDialog : public CDialogWindow
{
public:
  EditorStartDialog(void *phandle, const char *caption, EditorWorkspace &wsp, const char *wsp_blk, const char *select_wsp);
  virtual ~EditorStartDialog();

  void editWorkspace();

  // CDialogWindow interface

  virtual bool onOk();
  virtual bool onCancel() { return true; };

  // ControlEventHandler methods from CDialogWindow

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);

  virtual void onCustomFillPanel(PropertyContainerControlBase &panel) {}
  virtual bool onCustomSettings(PropertyContainerControlBase &panel) { return true; }
  virtual int getWspDialogHeight();

protected:
  EditorWorkspace &wsp;

  inline int getCustomTop() const { return customTop; }
  inline bool isWspEditing() const { return editWsp; }

  virtual void createCustom() {}

  virtual void onChangeWorkspace(const char *name);

  virtual void onAddWorkspace();
  virtual void onEditWorkspace();

  void reloadWsp();

private:
  String startWsp;
  String blkName;

  bool editWsp;
  bool wspInited;

  int customTop;

  void initWsp(bool init_combo);
};


class WorkspaceDialog : public CDialogWindow
{
public:
  WorkspaceDialog(void *phandle, EditorStartDialog *esd, const char *caption, EditorWorkspace &wsp, bool is_editing);

  // CDialogWindow interface

  virtual bool onOk();
  virtual bool onCancel() { return true; };

  // ControlEventHandler methods from CDialogWindow

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);

protected:
  enum
  {
    PID_COMMON_PARAMETERS_GRP,
    PID_WSP_NAME,
    PID_APP_FOLDER,
    PID_NEW_APPLICATION,
    PID_CUSTOM_PARAMETERS
  };

  EditorWorkspace &mWsp;
  bool mEditing;
  EditorStartDialog *mEsd;
};


#endif //__GAIJIN_EDITORCORE_START_DIALOG__
