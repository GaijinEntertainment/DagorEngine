// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_math3d.h>
#include <util/dag_string.h>

#include <propPanel/commonWindow/dialogWindow.h>

class NewProjectDialog : public PropPanel::DialogWindow
{
public:
  NewProjectDialog(void *phandle, const char *caption, const char *name_label = NULL, const char *_note = NULL);

  const char *getName();
  const char *getLocation();

  void setName(const char *s);
  void setLocation(const char *s);

  void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel) override;
  bool onOk() override;

private:
  String mName, mLocation;
};
