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
    PropertyControlBase(id, event_handler, parent, x, y, w, hdpi::Px(0)), controlCaption(caption), listBox(vals, index)
  {
    listBox.setEventHandler(this);
  }

  virtual unsigned getTypeMaskForSet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_CAPTION | CONTROL_DATA_STRINGS; }
  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_INT | CONTROL_DATA_TYPE_STRING; }

  virtual int getIntValue() const override { return listBox.getSelectedIndex(); }

  virtual int getTextValue(char *buffer, int buflen) const override
  {
    const String *selectedText = listBox.getSelectedText();
    if (!selectedText)
      return 0;

    return ImguiHelper::getTextValueForString(*selectedText, buffer, buflen);
  }

  virtual void setIntValue(int index) override { listBox.setSelectedIndex(index); }
  virtual void setStringsValue(const Tab<String> &vals) override { listBox.setValues(vals); }
  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }
  virtual void setTextValue(const char value[]) override { listBox.setSelectedValue(value); }

  virtual int addStringValue(const char *value) override { return listBox.addValue(value); }
  virtual void removeStringValue(int idx) override { listBox.removeValueByIndex(idx); }

  virtual void reset() override
  {
    listBox.setSelectedIndex(-1);

    PropertyControlBase::reset();
  }

  virtual void setEnabled(bool enabled) override { listBox.setEnabled(enabled); }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!listBox.isEnabled());

    ImguiHelper::separateLineLabel(controlCaption);
    setFocusToNextImGuiControlIfRequested();

    listBox.updateImgui(mW, mH);
  }

private:
  String controlCaption;
  ListBoxControlStandalone listBox;
};

} // namespace PropPanel