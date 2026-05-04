// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

namespace PropPanel
{
class ContainerPropertyControl;
class ControlEventHandler;
} // namespace PropPanel

class CompositeEditorToolbar
{
public:
  void initUi(PropPanel::ControlEventHandler &event_handler, int toolbar_id);
  void closeUi();
  bool isInited() const;
  void updateGizmoToolbarButtons(bool canTransform);
  void updateSnapToolbarButtons();

private:
  void addCheckButton(PropPanel::ContainerPropertyControl &tb, int id, const char *editor_command_id, const char *bmp_name,
    const char *hint);
  void setButtonState(int id, bool checked, bool enabled);

  int toolBarId = -1;
};
