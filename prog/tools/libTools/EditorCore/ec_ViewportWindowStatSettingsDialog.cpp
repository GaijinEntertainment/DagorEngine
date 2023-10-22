#include "ec_ViewportWindowStatSettingsDialog.h"
#include <EditorCore/ec_ViewportWindow.h>
#include <EditorCore/ec_cm.h>

ViewportWindowStatSettingsDialog::ViewportWindowStatSettingsDialog(ViewportWindow &_viewport, hdpi::Px width, hdpi::Px height) :
  CDialogWindow(_viewport.getParentHandle(), width, height, "Viewport stat display settings"), viewport(_viewport)
{}

PropertyContainerControlBase *ViewportWindowStatSettingsDialog::createTabPanel()
{
  PropertyContainerControlBase *panel = getPanel();
  G_ASSERT(panel);
  return panel->createTabPanel(CM_STATS_SETTINGS_TAB_PANEL, "");
}

void ViewportWindowStatSettingsDialog::onChange(int pcb_id, PropPanel2 *panel) { viewport.handleStatSettingsDialogChange(pcb_id); }
