#ifndef __GAIJIN_EDITORCORE_EC_COMMON_DLG_H__
#define __GAIJIN_EDITORCORE_EC_COMMON_DLG_H__
#pragma once

#include <math/dag_math3d.h>
#include <util/dag_string.h>

#include <propPanel2/comWnd/dialog_window.h>

class NewProjectDialog : public CDialogWindow
{
public:
  NewProjectDialog(void *phandle, const char *caption, const char *name_label = NULL, const char *_note = NULL);

  const char *getName();
  const char *getLocation();

  void setName(const char *s);
  void setLocation(const char *s);

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual bool onOk();

private:
  String mName, mLocation;
};

#endif
