#pragma once
#include <propPanel2/comWnd/dialog_window.h>
#include <propPanel2/c_window_event_handler.h>
#include <generic/dag_tab.h>
#include <string>

class ObjPropDialog : public CDialogWindow
{
public:
  ObjPropDialog(const char *caption, hdpi::Px width, hdpi::Px height, const Tab<String> &nodes, const Tab<String> &scripts);

  bool onOk() override;

  const Tab<String> &getScripts() const { return scripts; }

protected:
  void onClick(int pcb_id, PropertyContainerControlBase *panel) override;
  void onChange(int pcb_id, PropertyContainerControlBase *panel) override;

  void applyChanges(const Tab<int> &sels);

private:
  Tab<String> scripts;
  Tab<int> prevSels;
  bool changed = false, shouldSave = false;
};
