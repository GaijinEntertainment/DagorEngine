// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/messageQueue.h>

class EditorWorkspace;


// dialog control IDs

enum
{
  ID_START_DIALOG_COMBO = 400,
  ID_START_DIALOG_ERROR_MESSAGE,
  ID_START_DIALOG_BUTTON_ADD,
  ID_START_DIALOG_BUTTON_EDIT,
};


class EditorStartDialog : public PropPanel::DialogWindow, public PropPanel::IDelayedCallbackHandler
{
public:
  EditorStartDialog(const char *caption, EditorWorkspace &wsp, const char *wsp_blk, const char *select_wsp);
  ~EditorStartDialog() override;

  void editWorkspace();

  const String &getStartWorkspace() const { return startWsp; }
  int getWorkspaceIndex(const char *name) const;

  // DialogWindow interface

  bool onOk() override;
  bool onCancel() override { return true; };

  // ControlEventHandler methods from CDialogWindow

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

  virtual void onCustomFillPanel(PropPanel::ContainerPropertyControl &panel) {}
  virtual bool onCustomSettings(PropPanel::ContainerPropertyControl &panel) { return true; }

protected:
  EditorWorkspace &wsp;

  inline int getCustomTop() const { return customTop; }
  inline bool isWspEditing() const { return editWsp; }

  // IDelayedCallbackHandler
  void onImguiDelayedCallback(void *user_data) override;

  void updateImguiDialog() override;

  virtual void createCustom() {}

  virtual void onChangeWorkspace(const char *name);

  virtual void onAddWorkspace();
  virtual void onEditWorkspace();

  void reloadWsp();
  void updateOkButtonState();

private:
  String startWsp;
  String blkName;

  bool editWsp;
  bool wspInited;
  bool showAddWorkspaceDialog = false;
  bool selectedWorkspaceValid = false;

  int customTop;

  void initWsp(bool init_combo);
};


class WorkspaceDialog : public PropPanel::DialogWindow
{
public:
  WorkspaceDialog(EditorStartDialog *esd, const char *caption, EditorWorkspace &wsp, bool is_editing);

  // DialogWindow interface

  bool onOk() override;
  bool onCancel() override { return true; };

  // ControlEventHandler methods from CDialogWindow

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;

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
