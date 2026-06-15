// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/c_control_event_handler.h>
#include <propPanel/control/panelWindow.h>

// Dockable panel listing the GraphEditor canvas shortcuts. Keyboard rows pull their key text
// live from the EditorCommandSystem (so rebinds show through); mouse rows are fixed. Managed by
// GraphEditorPlg like the other panels; toggled from the toolbar button or the F1 hotkey.
class ShortcutsPanel final : public PropPanel::ControlEventHandler
{
public:
  ShortcutsPanel();
  ~ShortcutsPanel() override;

  PropPanel::PanelWindowPropertyControl *getPanelWindow() { return panelWindow; }

  void updateImgui();

private:
  PropPanel::PanelWindowPropertyControl *panelWindow = nullptr;
};
