#pragma once

class ControlEventHandler;
class PropertyContainerControlBase;

class CompositeEditorToolbar
{
public:
  void initUi(ControlEventHandler &event_handler, int toolbar_id);
  void closeUi();
  void updateToolbarButtons(bool canTransform);

private:
  void addCheckButton(PropertyContainerControlBase &tb, int id, const char *bmp_name, const char *hint);
  void setButtonState(int id, bool checked, bool enabled);

  int toolBarId = -1;
};
