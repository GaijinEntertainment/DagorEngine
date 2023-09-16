#pragma once
#include <propPanel2/comWnd/dialog_window.h>

class ViewportWindow;

class ViewportWindowStatSettingsDialog : public CDialogWindow
{
public:
  ViewportWindowStatSettingsDialog(ViewportWindow &_viewport, unsigned width, unsigned height);

  PropertyContainerControlBase *createTabPanel();

private:
  virtual void onChange(int pcb_id, PropPanel2 *panel) override;

  ViewportWindow &viewport;
};
