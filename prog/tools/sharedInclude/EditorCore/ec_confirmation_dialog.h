// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/hdpiUtil.h>
#include <propPanel/commonWindow/dialogWindow.h>
#include <propPanel/control/container.h>

class ConfirmationDialog final
{
public:
  explicit ConfirmationDialog(const char caption[], const char msg[]) :
    dialog(nullptr, hdpi::_pxScaled(100), hdpi::_pxScaled(25), caption)
  {
    dialog.getPanel()->createStatic(0, msg);

    const float windowWidth =
      ImGui::CalcTextSize(msg).x + ImGui::GetStyle().ItemSpacing.x + (ImGui::GetStyle().WindowPadding.x * 2.0f);
    dialog.setWindowSize(hdpi::Px(windowWidth), hdpi::Px(0));
    dialog.autoSize();
  }

  ~ConfirmationDialog() noexcept = default;

  int showDialog() { return dialog.showDialog(); }

private:
  PropPanel::DialogWindow dialog;
};
