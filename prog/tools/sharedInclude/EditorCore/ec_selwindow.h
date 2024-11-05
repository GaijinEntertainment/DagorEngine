// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_decl.h>
#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_interface_ex.h>

#include <propPanel/commonWindow/dialogWindow.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>


/// Class for dialog window 'Select objects by name'.
/// (Called by key 'H')
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup SelectByName
class SelWindow : public PropPanel::DialogWindow
{
public:
  /// Constructor 1.
  /// @param p - pointer to parent window (may be NULL)
  /// @param obj - pointer to interface IObjectsList
  SelWindow(void *phandle, IObjectsList *obj, const char *obj_list_owner_name = NULL);

  /// Constructor 2.
  /// @param p - pointer to parent window (may be NULL)
  /// @param[in] rect - dialog window size
  /// @param obj - pointer to interface IObjectsList
  SelWindow(void *phandle, const EcRect &rect, IObjectsList *obj, const char *obj_list_owner_name = NULL);

  /// Destructor.
  ~SelWindow();

  virtual int showDialog() override;

  /// Get list of selected objects.
  /// @param[out] names - objects list
  void getSelectedNames(Tab<String> &names);

private:
  const char *objListName;
  IObjectsList *objects;

  Tab<String> typeNames;
  // Tab<CtlCheckBox*> objTypes;

  DataBlock *typesBlk;

  void ctorInit();

  void fillNames();
  // void resizeControls();

  virtual void onChange(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
  virtual void onDoubleClick(int pcb_id, PropPanel::ContainerPropertyControl *panel);
};
