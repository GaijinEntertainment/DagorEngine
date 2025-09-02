// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include "listBoxStandalone.h"
#include "../scopedImguiBeginDisabled.h"

namespace PropPanel
{

class ListBoxPropertyControl : public PropertyControlBase
{
public:
  ListBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[], const Tab<String> &vals, int index) :
    PropertyControlBase(id, event_handler, parent, x, y, w, Constants::LISTBOX_DEFAULT_HEIGHT),
    controlCaption(caption),
    listBox(vals, index)
  {
    listBox.setEventHandler(this);
  }

  unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION | CONTROL_DATA_STRINGS; }
  unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_TYPE_STRING; }

  int getIntValue() const override { return listBox.getSelectedIndex(); }

  int getTextValue(char *buffer, int buflen) const override
  {
    const String *selectedText = listBox.getSelectedText();
    if (!selectedText)
      return 0;

    return ImguiHelper::getTextValueForString(*selectedText, buffer, buflen);
  }

  dag::ConstSpan<String> getStringsValue() override { return listBox.getValues(); }

  void setIntValue(int index) override { listBox.setSelectedIndex(index); }
  void setStringsValue(const Tab<String> &vals) override { listBox.setValues(vals); }
  void setCaptionValue(const char value[]) override { controlCaption = value; }
  void setTextValue(const char value[]) override { listBox.setSelectedValue(value); }
  void setListBoxEventHandlerValue(IListBoxControlEventHandler *handler) override { listBoxEventHandler = handler; }

  int addStringValue(const char *value) override { return listBox.addValue(value); }
  void removeStringValue(int idx) override { listBox.removeValueByIndex(idx); }

  void reset() override
  {
    listBox.setSelectedIndex(-1);

    PropertyControlBase::reset();
  }

  void setEnabled(bool enabled) override { listBox.setEnabled(enabled); }

  void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!listBox.isEnabled());

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    listBox.updateImgui(mW, mH);
  }

private:
  void onWcRightClick(WindowBase *source) override
  {
    if (listBoxEventHandler)
      listBoxEventHandler->onListBoxContextMenu(getID(), listBox);
  }

  String controlCaption;
  ListBoxControlStandalone listBox;
  IListBoxControlEventHandler *listBoxEventHandler = nullptr;
};

} // namespace PropPanel