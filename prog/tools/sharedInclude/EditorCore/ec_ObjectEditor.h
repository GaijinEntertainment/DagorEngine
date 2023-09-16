#ifndef __GAIJIN_EDITORCORE_EC_OBJECT_EDITOR_H__
#define __GAIJIN_EDITORCORE_EC_OBJECT_EDITOR_H__
#pragma once


#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_interface_ex.h>
#include <EditorCore/ec_rendEdObject.h>

#include <propPanel2/c_panel_base.h>
#include <propPanel2/comWnd/panel_window.h>
#include <sepGui/wndPublic.h>

#include <util/dag_globDef.h>

#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_smallTab.h>

#include <libTools/util/undo.h>


class RenderableEditableObject;
class ObjectEditorPropPanelBar;
class IObjectCreator;


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


/// 3D objects Editor.
/// Used in conjunction with RenderableEditableObject. May be fairly simply
/// built into different editors.
/// @ingroup EditorCore
/// @ingroup Misc
class ObjectEditor : public IGizmoClient,
                     public IGenEventHandler,
                     public IObjectsList,
                     public IWndManagerWindowHandler,
                     public ControlEventHandler
{
  friend class ObjectEditorWrap;

public:
  bool isGizmoValid;

  /// Constructor.
  ObjectEditor();
  /// Destructor.
  virtual ~ObjectEditor();

  //*****************************************************************
  /// @name Implementation of IObjectsList methods for selecting objects by name
  //@{
  virtual void getObjNames(Tab<String> &names, Tab<String> &sel_names, const Tab<int> &types);
  virtual void getTypeNames(Tab<String> &names) {}
  virtual void onSelectedNames(const Tab<String> &names);
  //@}


  /// Get Undo system.
  /// Get ObjectEditor's Undo system. Default implementation returns global
  /// Undo system of the Editor. Override this method if you need local
  /// Undo system.
  /// @return pointer to ObjectEditor's Undo system
  virtual UndoSystem *getUndoSystem();


  //*****************************************************************
  /// @name ToolBar methods, useful button functions.
  //@{
  /// Initialize User Interface of an Object Editor.
  /// @param[in] toolbar_id - toolbar's id to use.
  virtual void initUi(int toolbar_id);
  /// Close User Interface of an Object Editor.
  virtual void closeUi();

  /// Fill ToolBar with buttons (called from initUi()).
  /// Override to add custom buttons to toolbar.
  virtual void fillToolBar(PropertyContainerControlBase *toolbar);

  /// Add button to toolbar.
  /// @param[in] tb - pointer to toolbar
  /// @param[in] cmd - button command id
  /// @param[in] bmp_name - pointer to button's bitmap name
  /// @param[in] hint - pointer to hint string
  /// @param[in] check - is check button
  virtual void addButton(PropertyContainerControlBase *tb, int id, const char *bmp_name, const char *hint, bool check = false);

  /// Enable button.
  /// @param[in] id - button ID
  /// @param[in] state - <b>true / false</b>: enabled / disabled
  virtual void enableButton(int id, bool state);

  /// Set check-like button state.
  /// @param[in] id - button ID
  /// @param[in] state - <b>true / false</b>: checked / not checked
  virtual void setButton(int id, bool state);

  /// Set Radio Button state (calls setButton(tb, id, value_id==id)).
  /// @param[in] id - button ID
  /// @param[in] value_id - if value_id==id set specified radio button on
  ///                       (other buttons of button's groop are set to off)
  virtual void setRadioButton(int id, int value_id);

  /// Update toolbar buttons (check-like and radio buttons) state.
  /// Override if custom buttons are added to toolbar.
  virtual void updateToolbarButtons();
  //@}


  /// Open 'Select by name' dialog window to select objects from
  /// objects list by name.
  virtual void selectByNameDlg();

  /// Drop selected objects on surface
  virtual void dropObjects();

  /// Set Edit Mode.
  /// @param[in] mode - Edit mode:
  ///\n               - CM_OBJED_MODE_SELECT - 'Select' mode
  ///\n               - CM_OBJED_MODE_MOVE - 'Move' mode
  ///\n               - CM_OBJED_MODE_SURF_MOVE - 'Surface move' mode
  ///\n               - CM_OBJED_MODE_ROTATE - 'Rotate' mode
  ///\n               - CM_OBJED_MODE_SCALE - 'Scale' mode
  virtual void setEditMode(int mode);

  /// Show/hide Property Panel
  virtual void showPanel();

  /// Get Edit Mode.
  virtual int getEditMode() { return editMode; }

  /// Update Gizmo state.
  virtual void updateGizmo(int basis = IEditorCoreEngine::BASIS_None);

  /// Make from n (if needed) and set uniq name for object o
  virtual bool setUniqName(RenderableEditableObject *o, const char *n);
  void setSuffixDigitsCount(int c) { suffixDigitsCount = c; }


  //*****************************************************************
  /// @name Editing objects (add, remove, render).
  //@{
  /// Add objects to Editor.
  /// @param[in] obj - array of pointers to objects to add to
  /// @param[in] num - number of objects to add to
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  virtual void addObjects(RenderableEditableObject **obj, int num, bool use_undo = true) { _addObjects(obj, num, use_undo); }

  /// Remove objects from Editor.
  /// @param[in] obj - array of pointers to objects to remove
  /// @param[in] num - number of objects to remove
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  virtual void removeObjects(RenderableEditableObject **obj, int num, bool use_undo = true) { _removeObjects(obj, num, use_undo); }

  /// Remove all objects from Editor.
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  virtual void removeAllObjects(bool use_undo = true);

  /// Add one object to Editor.
  /// @param[in] obj - pointer to object to add to
  void addObject(RenderableEditableObject *obj, bool use_undo = true) { addObjects(&obj, 1, use_undo); }

  /// Remove one object from Editor.
  /// @param[in] obj - pointer to object to remove
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  void removeObject(RenderableEditableObject *obj, bool use_undo = true) { removeObjects(&obj, 1, use_undo); }

  /// Update objects, Gizmo, etc in accordance with time specified.
  /// Should be called from outside the ObjectEditor in order it
  /// may properly function.
  /// @param[in] dt - time passed
  virtual void update(real dt);

  /// Prepare objects for rendering.
  /// Should be called from outside the ObjectEditor in order it
  /// may properly function.
  virtual void beforeRender();

  /// Render opaque objects parts.
  /// Should be called from outside the ObjectEditor in order it
  /// may properly function.
  virtual void render();

  /// Render transparent objects parts.
  /// Should be called from outside the ObjectEditor in order it
  /// may properly function.
  virtual void renderTrans();
  //@}


  //*****************************************************************
  /// @name Editing objects (object count, get objects, etc).
  //@{
  /// Get object count.
  /// @return object count
  int objectCount() const { return objects.size(); }

  /// Get object from objects list by index.
  /// @param[in] index - object index
  /// @return pointer to object
  RenderableEditableObject *getObject(int index) const
  {
    G_ASSERT(index >= 0 && index < objects.size());
    return objects[index];
  }

  /// Get object by name (case insensitive).
  /// @param[in] name - pointer to name string
  /// @return pointer to object. If there are many appropriate objects then
  ///         function returns pointer to the first object.
  RenderableEditableObject *getObjectByName(const char *name) const;

  /// Pick one object in viewport point having specified coordinates.
  /// Hidden and frozen objects are ignored.
  /// @param[in] wnd - pointer to viewport window
  /// @param[in] x,y - x,y point coordinates
  /// @return pointer to the first object found
  RenderableEditableObject *pickObject(IGenViewportWnd *wnd, int x, int y);

  /// Pick objects in viewport point having specified coordinates.
  /// Hidden and frozen objects are ignored.
  /// @param[in] wnd - pointer to viewport window
  /// @param[in] x,y - x,y point coordinates
  /// @param[out] objs - reference to array of pointers to objects found
  /// @return @b true if one or many objects were picked, @b false in other case
  bool pickObjects(IGenViewportWnd *wnd, int x, int y, Tab<RenderableEditableObject *> &objs);
  bool checkObjSelFilter(RenderableEditableObject &obj);
  //@}


  //*****************************************************************
  /// @name Editing objects (edit selected objects).
  //@{
  /// Update object selection list in accordance with their FLG_SELECTED flags.
  void updateSelection();

  /// Get selected objects count.
  /// @return selected objects count
  int selectedCount() const { return selection.size(); }

  /// Get selected object from objects list by index.
  /// @param[in] index - object index
  /// @return pointer to object selected
  RenderableEditableObject *getSelected(int index) const
  {
    if (index < 0 || index >= selection.size())
      return NULL;
    return selection[index];
  }

  /// Unselect all objects.
  virtual void unselectAll();

  /// Select all objects.
  virtual void selectAll();

  /// Get bounding box for objects selected.
  /// @param[out] box - reference to bounding box
  /// @return @b true if operation successful, @b false in other case
  virtual bool getSelectionBox(BBox3 &box) const;

  /// Delete selected objects.
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  virtual void deleteSelectedObjects(bool use_undo = true);
  //@}


  //*****************************************************************
  /// @name Editing objects (methods called by events).
  //@{
  /// Called when object flags are changed.
  /// @param[in] obj - pointer to object whose flags are changed
  /// @param[in] changed_flags - FLG_SELECTED,FLG_HIDDEN,
  ///                            FLG_FROZEN,FLG_HIGHLIGHTED
  virtual void onObjectFlagsChange(RenderableEditableObject *obj, int changed_flags);

  /// Called when object geometry is changed.
  /// @param[in] obj - pointer to object whose geometry is changed
  virtual void onObjectGeomChange(RenderableEditableObject *obj);

  /// Called each time an object is selected or deselected, so it should only
  /// set flags etc, and not perform long actions.
  virtual void selectionChanged();
  //@}

  void saveNormalOnSelection(const Point3 &n) override;
  void setCollisionIgnoredOnSelection() override;
  void resetCollisionIgnoredOnSelection() override;

  //*****************************************************************
  /// @name Editing objects (renaming, object properties).
  //@{
  /// Rename object.
  /// The function tests if renaming is possible and if specified name is unique.
  /// @b NOTE: If new name is not unique, it will be changed and resulting name
  /// will be different.
  /// @param[in] obj - pointer to object
  /// @param[in] new_name - pointer to string with new name
  /// @param[in] use_undo - <b>true / false: use / not use</b> undo system
  virtual void renameObject(RenderableEditableObject *obj, const char *new_name, bool use_undo = true);

  /// Indicate the need for updating object properties (on Property Panel).
  virtual void invalidateObjectProps();

  /// Update object properties on Property Panel.
  /// Commonly called implicitly after some time since call to
  /// invalidateObjectProps()
  virtual void updateObjectProps();
  //@}


  //*****************************************************************
  /// @name Methods inherited from IGizmoClient
  //@{
  virtual void release() {}
  virtual Point3 getPt();
  virtual bool getRot(Point3 &p);
  virtual bool getScl(Point3 &p);
  virtual bool getAxes(Point3 &ax, Point3 &ay, Point3 &az);
  virtual void changed(const Point3 &delta);
  virtual bool canStartChangeAt(IGenViewportWnd *wnd, int x, int y, int gizmo_sel);
  virtual void gizmoStarted();
  virtual void gizmoEnded(bool apply);

  virtual int getAvailableTypes();
  //@}


  //*****************************************************************
  /// @name Methods inherited from IGenEventHandler
  //@{
  virtual void handleKeyPress(IGenViewportWnd *wnd, int vk, int modif);
  virtual void handleKeyRelease(IGenViewportWnd *wnd, int vk, int modif) {}

  virtual bool handleMouseMove(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBPress(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseCBRelease(IGenViewportWnd *wnd, int x, int y, bool inside, int buttons, int key_modif) { return false; }
  virtual bool handleMouseWheel(IGenViewportWnd *wnd, int wheel_d, int x, int y, int key_modif) { return false; }
  virtual bool handleMouseDoubleClick(IGenViewportWnd *wnd, int x, int y, int key_modif) { return false; }

  virtual void handleViewportPaint(IGenViewportWnd *wnd) {}
  virtual void handleViewChange(IGenViewportWnd *wnd) {}
  //@}

  //*****************************************************************
  /// @name Methods for creating new objects
  //@{

  /// Called to start or stop object creation mode.
  /// @param[in] creator - creator to use
  virtual void setCreateMode(IObjectCreator *creator = NULL);
  /// Called to start or stop object creation mode.
  /// @param[in] sample - object to clone
  virtual void setCreateBySampleMode(RenderableEditableObject *sample = NULL);

  /// this method can be overrided to create objects
  /// default implementation does nothing, but finishes creation process
  /// @param[in] creator - creator to use
  virtual void createObject(IObjectCreator *creator);

  /// this method can be overrided to create objects
  /// default implementation does nothing
  /// @param[in] sample - object to clone
  virtual void createObjectBySample(RenderableEditableObject *sample);
  //@}

protected:
  String objListOwnerName;

  PtrTab<RenderableEditableObject> objects, selection;

  Point3 gizmoPt, gizmoOrigin, gizmoScl, gizmoRot, gizmoSclO, gizmoRotO, gizmoRotPoint;
  bool isGizmoStarted, areObjectPropsValid;

  enum class PlacementRotation
  {
    NO_ROTATION = -1,
    X_TO_NORMAL = 0,
    Y_TO_NORMAL = 1,
    Z_TO_NORMAL = 2
  };
  PlacementRotation selectedPlacementRotation = PlacementRotation::NO_ROTATION;
  TMatrix getPlacementRotationMatrix();

  RenderableEditableObject *sample;
  IObjectCreator *creator;
  ObjectEditorPropPanelBar *objectPropBar;

  virtual void _addObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual void _removeObjects(RenderableEditableObject **obj, int num, bool use_undo);
  virtual bool canSelectObj(RenderableEditableObject *o);

  virtual Point3 getSurfMoveGizmoPos(const Point3 &obj_pos) const;

  virtual ObjectEditorPropPanelBar *createEditorPropBar(void *handle);

  // IWndManagerWindowHandler
  virtual IWndEmbeddedWindow *onWmCreateWindow(void *handle, int type);
  virtual bool onWmDestroyWindow(void *handle);

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);

  virtual void moveObjects(PtrTab<RenderableEditableObject> &obj, const Point3 &delta, IEditorCoreEngine::BasisType basis);

  class UndoAddObjects : public UndoRedoObject
  {
  public:
    ObjectEditor *objEd;
    PtrTab<RenderableEditableObject> objects;

    UndoAddObjects(ObjectEditor *oe, int num) : objEd(oe), objects(midmem) { objects.reserve(num); }

    virtual void restore(bool save_redo) { objEd->_removeObjects((RenderableEditableObject **)&objects[0], objects.size(), false); }

    virtual void redo() { objEd->_addObjects((RenderableEditableObject **)&objects[0], objects.size(), false); }

    virtual size_t size() { return sizeof(*this) + data_size(objects) + objects.size() * sizeof(RenderableEditableObject); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoAddObjects"; }
  };

  class UndoRemoveObjects : public UndoRedoObject
  {
  public:
    ObjectEditor *objEd;
    PtrTab<RenderableEditableObject> objects;

    UndoRemoveObjects(ObjectEditor *oe, int num) : objEd(oe), objects(midmem) { objects.reserve(num); }

    virtual void restore(bool save_redo) { objEd->_addObjects((RenderableEditableObject **)&objects[0], objects.size(), false); }

    virtual void redo() { objEd->_removeObjects((RenderableEditableObject **)&objects[0], objects.size(), false); }

    virtual size_t size() { return sizeof(*this) + data_size(objects) + objects.size() * sizeof(RenderableEditableObject); }

    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoRemoveObjects"; }
  };

  class UndoObjectEditorRename : public UndoRedoObject
  {
  public:
    ObjectEditor *objEd;
    Ptr<RenderableEditableObject> object;
    String undoName, redoName;


    UndoObjectEditorRename(ObjectEditor *oe, RenderableEditableObject *obj) : objEd(oe), object(obj)
    {
      undoName = object->getName();
      redoName = object->getName();
    }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoName = object->getName();
      objEd->renameObject(object, undoName, false);
    }

    virtual void redo() { objEd->renameObject(object, redoName, false); }

    virtual size_t size() { return sizeof(*this) + data_size(undoName) + data_size(redoName); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoObjectEditorRename"; }
  };

private:
  int toolBarId;

  int editMode;
  int lastY;
  int rotDy;
  int scaleDy;
  real createRot;
  real createScale;
  bool canTransformOnCreate;
  bool justCreated;

  int suffixDigitsCount;
  String filterString;
  Tab<SimpleString> filterStrings;
  bool invFilter = false;
};


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

class ObjectEditorPropPanelBar : public ControlEventHandler
{
public:
  ObjectEditorPropPanelBar(ObjectEditor *obj_ed, void *hwnd, const char *caption);
  ~ObjectEditorPropPanelBar();

  // ControlEventHandler
  virtual void onChange(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);
  virtual void onPostEvent(int pcb_id, PropPanel2 *panel);

  virtual void fillPanel();
  virtual void refillPanel();
  virtual void updateName(const char *name);

  CPanelWindow *getPanel()
  {
    G_ASSERT(propPanel);
    return propPanel;
  }

protected:
  ObjectEditor *objEd;
  PtrTab<RenderableEditableObject> objects;

  CPanelWindow *propPanel;

  void getObjects();
};

// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

#endif
