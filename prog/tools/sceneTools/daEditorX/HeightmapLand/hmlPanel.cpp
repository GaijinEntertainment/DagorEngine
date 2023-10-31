#include "hmlPanel.h"
#include <dllPluginCore/core.h>
#include "hmlCm.h"


//==============================================================================

HmapLandPlugin::HmapLandPanel::HmapLandPanel(HmapLandPlugin &p) : plugin(p), mPanelWindow(NULL) {}

bool HmapLandPlugin::HmapLandPanel::showPropPanel(bool show)
{
  if (show != (bool)(mPanelWindow))
  {
    if (show)
      EDITORCORE->addPropPanel(PROPBAR_EDITOR_WTYPE, hdpi::_pxScaled(PROPBAR_WIDTH));
    else
      EDITORCORE->removePropPanel(mPanelWindow->getParentWindowHandle());
  }

  return true;
}


//==============================================================================

void HmapLandPlugin::HmapLandPanel::fillPanel(bool refill, bool schedule_regen, bool hold_pos)
{
  if (mPanelWindow)
    mPanelWindow->setPostEvent(refill ? ((schedule_regen ? 2 : 1) | (hold_pos ? 4 : 0)) : 0);
}


void HmapLandPlugin::HmapLandPanel::onPostEvent(int pcb_id, PropPanel2 *panel)
{
  if (pcb_id) // refill
  {
    plugin.mainPanelState.reset();
    panel->saveState(plugin.mainPanelState);
    if (pcb_id & 4)
      plugin.mainPanelState.setInt("pOffset", mPanelWindow->getScrollPos());
  }

  mPanelWindow->showPanel(false);
  plugin.fillPanel(*panel);
  if (pcb_id & 3)
    panel->loadState(plugin.mainPanelState);
  if (pcb_id & 4)
    mPanelWindow->setScrollPos(plugin.mainPanelState.getInt("pOffset", 0));

  mPanelWindow->showPanel(true);
  if (pcb_id & 2) // schedule_regen
    plugin.onPluginMenuClick(CM_BUILD_COLORMAP);
}


void HmapLandPlugin::HmapLandPanel::updateLightGroup()
{
  if (mPanelWindow)
    plugin.updateLightGroup(*mPanelWindow);
}


//==============================================================================

void HmapLandPlugin::HmapLandPanel::onChange(int pcb_id, PropPanel2 *panel) { plugin.onChange(pcb_id, panel); }


//==============================================================================

void HmapLandPlugin::HmapLandPanel::onClick(int pcb_id, PropPanel2 *panel) { plugin.onClick(pcb_id, panel); }


//==============================================================================
