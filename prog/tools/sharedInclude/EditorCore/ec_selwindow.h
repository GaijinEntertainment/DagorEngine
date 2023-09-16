#ifndef __GAIJIN_EDITORCORE_EC_SELWINDOW__
#define __GAIJIN_EDITORCORE_EC_SELWINDOW__
#pragma once

#include <EditorCore/ec_decl.h>
#include <EditorCore/ec_rect.h>
#include <EditorCore/ec_interface_ex.h>

#include <propPanel2/comWnd/dialog_window.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_tab.h>


/// Class for dialog window 'Select objects by name'.
/// (Called by key 'H')
/// @ingroup EditorCore
/// @ingroup Misc
/// @ingroup SelectByName
class SelWindow : public CDialogWindow
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
  void show();
  // void resizeControls();

  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onDoubleClick(int pcb_id, PropertyContainerControlBase *panel);
};


#endif //__GAIJIN_EDITORCORE_EC_SELWINDOW__
