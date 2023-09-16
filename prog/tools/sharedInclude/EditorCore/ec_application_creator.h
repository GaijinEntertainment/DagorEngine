#ifndef __GAIJIN_EDITORCORE_APPLICATION_CREATOR__
#define __GAIJIN_EDITORCORE_APPLICATION_CREATOR__
#pragma once

#include <propPanel2/comWnd/dialog_window.h>

class EditorWorkspace;


class ApplicationCreator : public CDialogWindow
{
public:
  ApplicationCreator(void *phandle, EditorWorkspace &wsp);

  // CDialogWindow interface

  virtual bool onOk();
  virtual bool onCancel() { return true; };

  // ControlEventHandler methods from CDialogWindow

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);

private:
  EditorWorkspace &wsp;

  void correctAppPath(PropertyContainerControlBase &panel);
};


#endif //__GAIJIN_EDITORCORE_APPLICATION_CREATOR__
