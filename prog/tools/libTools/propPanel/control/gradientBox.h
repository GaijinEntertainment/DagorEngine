// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <propPanel/control/propertyControlBase.h>
#include <propPanel/imguiHelper.h>
#include "gradientControlStandalone.h"
#include "../scopedImguiBeginDisabled.h"
#include <ioSys/dag_dataBlock.h>
#include <winGuiWrapper/wgw_dialogs.h>

namespace PropPanel
{

class GradientBoxPropertyControl : public PropertyControlBase
{
public:
  GradientBoxPropertyControl(ControlEventHandler *event_handler, ContainerPropertyControl *parent, int id, int x, int y, hdpi::Px w,
    const char caption[]) :
    PropertyControlBase(id, event_handler, parent, x, y, w,
      hdpi::_pxScaled(GRADIENT_HEIGHT) + hdpi::_pxScaled(TRACK_GRADIENT_BUTTON_HEIGHT)),
    gradientControl(this, hdpi::_px(w) - hdpi::_pxS(TRACK_GRADIENT_BUTTON_WIDTH))
  {
    G_ASSERT(mH > 0);
  }

  virtual unsigned getTypeMaskForSet() const override
  {
    return CONTROL_DATA_TYPE_GRADIENT | CONTROL_CAPTION | CONTROL_DATA_MIN_MAX_STEP;
  }

  virtual unsigned getTypeMaskForGet() const override { return CONTROL_DATA_TYPE_GRADIENT; }

  virtual void setGradientValue(PGradient value) override { gradientControl.setValue(value); }
  virtual void getGradientValue(PGradient destGradient) const override { gradientControl.getValue(destGradient); }

  virtual void setCaptionValue(const char value[]) override { controlCaption = value; }

  // current X value line
  virtual void setFloatValue(float value) override { gradientControl.setCurValue(value); }

  // 2 byte - cycle
  virtual void setIntValue(int value) override { gradientControl.setCycled(value & CONTROL_CYCLED_VALUES); }

  virtual void setMinMaxStepValue(float min, float max, float step) override { gradientControl.setMinMax(min, max); }

  virtual void setEnabled(bool enabled) override { controlEnabled = enabled; }

  virtual void reset() override { gradientControl.reset(); }

  virtual long onWcClipboardCopy(WindowBase *source, DataBlock &blk) override
  {
    blk.reset();
    blk.setStr("DataType", "Gradient");

    Gradient value(tmpmem);
    getGradientValue(&value);
    for (int i = 0; i < value.size(); ++i)
    {
      DataBlock *key_blk = blk.addNewBlock("key");
      if (key_blk)
      {
        key_blk->setReal("pos", value[i].position);
        key_blk->setE3dcolor("color", value[i].color);
      }
    }
    return 1;
  }

  virtual long onWcClipboardPaste(WindowBase *source, const DataBlock &blk) override
  {
    if (strcmp(blk.getStr("DataType", ""), "Gradient") != 0)
      return 0;

    int result = 0;
    gradientControl.setSelected(true);

    if (wingw::message_box(wingw::MBS_YESNO | wingw::MBS_QUEST, "Confirmation", "Replace gradient keys for \"%s\"?", controlCaption) ==
        wingw::MB_ID_YES)
    {
      Gradient value(tmpmem);

      int i = -1;
      while (const DataBlock *key_blk = blk.getBlockByName("key", i))
      {
        GradientKey key;
        key.position = key_blk->getReal("pos");
        key.color = key_blk->getE3dcolor("color");
        value.push_back(key);
        ++i;
      }

      setGradientValue(&value);
      onWcChange(nullptr);
      result = 1;
    }
    gradientControl.setSelected(false);
    return result;
  }

  virtual void updateImgui() override
  {
    ScopedImguiBeginDisabled scopedDisabled(!controlEnabled);

    ImguiHelper::separateLineLabel(controlCaption);

    const int width =
      mW > 0 ? min((int)mW, (int)floorf(ImGui::GetContentRegionAvail().x)) : (int)floorf(ImGui::GetContentRegionAvail().x);
    gradientControl.updateImgui(width, mH);
  }

private:
  String controlCaption;
  bool controlEnabled = true;
  GradientControlStandalone gradientControl;
};

} // namespace PropPanel