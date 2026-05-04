// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "av_workCycleActRateDialog.h"

#include "av_cm.h"

#include <EditorCore/ec_modelessDialogWindowController.h>

class WorkCycleActRateDialogController : public ModelessDialogWindowController<WorkCycleActRateDialog>
{
public:
  const char *getWindowId() const override { return WindowIds::MAIN_SETTINGS_SET_ACT_RATE; }

  void createDialog() override
  {
    dialog.reset(new WorkCycleActRateDialog());
    dialog->setDialogButtonText(PropPanel::DIALOG_ID_OK, "Close");
    dialog->removeDialogButton(PropPanel::DIALOG_ID_CANCEL);
    dialog->fill();
  }
};

static WorkCycleActRateDialogController work_cycle_act_rate_dialog_controller;

IModelessWindowController *get_work_cycle_act_rate_dialog_controller() { return &work_cycle_act_rate_dialog_controller; }
