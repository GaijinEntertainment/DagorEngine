//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/de_object.h>
#include <util/dag_globDef.h>

#include <generic/dag_tab.h>
#include <generic/dag_ptrTab.h>
#include <generic/dag_smallTab.h>


enum
{
  // ObjectEditor commands. Put it between CM_OBJED_FIRST and CM_OBJED_LAST enum keys.
  CM_OBJED_FIRST,

  CM_OBJED_MODE_SELECT,
  CM_OBJED_MODE_MOVE,
  CM_OBJED_MODE_SURF_MOVE,
  CM_OBJED_MODE_ROTATE,
  CM_OBJED_MODE_SCALE,

  CM_OBJED_DROP,
  CM_OBJED_DROP_NORM,

  CM_OBJED_DELETE,

  CM_OBJED_RESET_SCALE,
  CM_OBJED_RESET_ROTATE,

  CM_OBJED_MODE_POINT_ACTION,

  CM_OBJED_LAST,
};


class IObjectCreator;
class PropertyContainerControlBase;
struct Frustum;


class ObjectEditor : public IGizmoClient, public IGenEventHandler, public IObjectsList
{
public:
  bool isGizmoValid = false;

  ObjectEditor() : objects(midmem), selection(midmem), cloneObjs(midmem) {}
  virtual ~ObjectEditor();

  virtual UndoSystem *getUndoSystem();

  virtual void updateToolbarButtons();

  virtual void dropObjects();
  virtual void dropObjectsNorm();
  virtual void resetScale();
  virtual void resetRotate();

  virtual void setWorkMode(const char *mode);
  virtual const char *getWorkMode() { return workMode; }

  virtual void setEditMode(int mode);

  virtual int getEditMode() { return editMode; }

  virtual void updateGizmo(int basis = IDaEditor4Engine::BASIS_none);
  virtual void setPointActionPreview(const char *shape, float param);

  virtual bool setUniqName(EditableObject *o, const char *n);
  void setSuffixDigitsCount(int c) { suffixDigitsCount = c; }


  virtual void addObjects(EditableObject **obj, int num, bool use_undo = true) { _addObjects(obj, num, use_undo); }
  virtual void removeObjects(EditableObject **obj, int num, bool use_undo = true) { _removeObjects(obj, num, use_undo); }
  virtual void removeAllObjects(bool use_undo = true);
  void addObject(EditableObject *obj, bool use_undo = true) { addObjects(&obj, 1, use_undo); }
  void removeObject(EditableObject *obj, bool use_undo = true) { removeObjects(&obj, 1, use_undo); }
  virtual void fillDeleteDepsObjects(Tab<EditableObject *> & /*list*/) const {}
  void removeInvalidObjects();

  virtual void update(float dt);
  virtual void beforeRender();
  virtual void render(const Frustum &, const Point3 &);
  virtual void renderTrans();

  int objectCount() const { return objects.size(); }
  EditableObject *getObject(int index) const
  {
    G_ASSERT(index >= 0 && index < objects.size());
    return objects[index];
  }

  EditableObject *getObjectByName(const char *name) const;
  EditableObject *pickObject(int x, int y);
  virtual bool pickObjects(int x, int y, Tab<EditableObject *> &objs);
  bool checkObjSelFilter(EditableObject &obj);

  void updateSelection();
  void setSelectionFocus(EditableObject *obj);

  int selectedCount() const { return selection.size(); }
  EditableObject *getSelected(int index) const
  {
    if (index < 0 || index >= selection.size())
      return NULL;
    return selection[index];
  }

  virtual EditableObject *cloneObject(EditableObject *o, bool use_undo);

  virtual void unselectAll();
  virtual void selectAll();
  virtual bool getSelectionBox(BBox3 &box) const;
  virtual bool getSelectionBoxFocused(BBox3 &box) const;
  virtual void deleteSelectedObjects(bool use_undo = true);


  virtual void onObjectFlagsChange(EditableObject *obj, int changed_flags);
  virtual void onObjectGeomChange(EditableObject *obj);
  virtual void selectionChanged();

  virtual void renameObject(EditableObject *obj, const char *new_name, bool use_undo = true);
  virtual void invalidateObjectProps();
  virtual void updateObjectProps() {}

  virtual void setCreateMode(IObjectCreator *creator = NULL);
  virtual void setCreateBySampleMode(EditableObject *sample = NULL);
  virtual void createObject(IObjectCreator *creator);
  virtual void createObjectBySample(EditableObject *sample);
  virtual bool cancelCreateMode();

  virtual void performPointAction(bool has_pos, int x, int y, const char *op);


  // IObjectsList
  virtual void getObjNames(Tab<SimpleString> &names, Tab<SimpleString> &sel_names, const Tab<int> &types);
  virtual void getTypeNames(Tab<SimpleString> & /*names*/) {}
  virtual void onSelectedNames(const Tab<SimpleString> &names);

  // IGizmoClient
  virtual void release() {}
  virtual Point3 getPt() { return gizmoPt; }
  virtual bool getRot(Point3 &p);
  virtual bool getScl(Point3 &p);
  virtual bool getAxes(Point3 &ax, Point3 &ay, Point3 &az);
  virtual void changed(const Point3 &delta);
  virtual bool canStartChangeAt(int x, int y, int gizmo_sel);
  virtual void gizmoStarted();
  virtual void gizmoEnded(bool apply);
  virtual int getAvailableTypes();


  // IGenEventHandler
  virtual void handleKeyPress(int dkey, int modif);
  virtual void handleKeyRelease(int /*dkey*/, int /*modif*/);

  virtual bool handleMouseMove(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBPress(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseLBRelease(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBPress(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseRBRelease(int x, int y, bool inside, int buttons, int key_modif);
  virtual bool handleMouseCBPress(int /*x*/, int /*y*/, bool /*inside*/, int /*buttons*/, int /*key_modif*/) { return false; }
  virtual bool handleMouseCBRelease(int /*x*/, int /*y*/, bool /*inside*/, int /*buttons*/, int /*key_modif*/) { return false; }
  virtual bool handleMouseWheel(int /*wheel_d*/, int /*x*/, int /*y*/, int /*key_modif*/) { return false; }

protected:
  SimpleString objListOwnerName;
  PtrTab<EditableObject> objects, selection;
  EditableObject *selectionFocus = nullptr;

  Point3 gizmoPt = ZERO<Point3>(), gizmoOrigin = ZERO<Point3>(), gizmoScl = ZERO<Point3>(), gizmoRot = ZERO<Point3>(),
         gizmoSclO = ZERO<Point3>(), gizmoRotO = ZERO<Point3>(), gizmoRotPoint = ZERO<Point3>();
  bool isGizmoStarted = false, areObjectPropsValid = false;

  EditableObject *sample = nullptr;
  IObjectCreator *creator = nullptr;

  bool cloneMode = false;
  Point3 cloneDelta = ZERO<Point3>();
  String cloneName;
  PtrTab<EditableObject> cloneObjs;

  virtual void _addObjects(EditableObject **obj, int num, bool use_undo);
  virtual void _removeObjects(EditableObject **obj, int num, bool use_undo);
  virtual bool canSelectObj(EditableObject *o);
  virtual bool canCloneSelection() { return false; }
  virtual int getCloneCount() { return 1; }
  virtual void cloneSelection();
  virtual Point3 getSurfMoveGizmoPos(const Point3 &obj_pos) const;

  // ControlEventHandler
  virtual void onClick(int pcb_id, PropertyContainerControlBase *panel);

  class UndoAddObjects;
  class UndoRemoveObjects;
  class UndoObjectEditorRename;

protected:
  static Point2 cmp_obj_z_pt;
  static int cmp_obj_z(EditableObject *const *a, EditableObject *const *b);

private:
  // int toolBarId;

  int editMode = CM_OBJED_MODE_SELECT;
  int lastY = 0;
  int rotDy = 0;
  int scaleDy = 0;
  float createRot = 0;
  float createScale = 1;
  bool canTransformOnCreate = false;
  bool justCreated = false;

  int suffixDigitsCount = 2;
  String filterString;

  String workMode;
  int mouseX = 0;
  int mouseY = 0;

  int pointActionPreviewShape = -1;
  float pointActionPreviewParam = 0.0f;
  void renderPointActionPreview();
  virtual int parsePointActionPreviewShape(const char *) { return -1; }
  virtual void renderPointActionPreviewShape(int, const Point3 &, float) {}
};


class ObjectEditor::UndoAddObjects : public UndoRedoObject
{
public:
  ObjectEditor *objEd;
  PtrTab<EditableObject> objects;

  UndoAddObjects(ObjectEditor *oe, int num) : objEd(oe), objects(midmem) { objects.reserve(num); }

  virtual void restore(bool /*save_redo*/) { objEd->_removeObjects((EditableObject **)(void *)objects.data(), objects.size(), false); }
  virtual void redo() { objEd->_addObjects((EditableObject **)(void *)objects.data(), objects.size(), false); }

  virtual size_t size() { return sizeof(*this) + data_size(objects) + objects.size() * sizeof(EditableObject); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoAddObjects"; }
};

class ObjectEditor::UndoRemoveObjects : public UndoRedoObject
{
public:
  ObjectEditor *objEd;
  PtrTab<EditableObject> objects;

  UndoRemoveObjects(ObjectEditor *oe, int num) : objEd(oe), objects(midmem) { objects.reserve(num); }

  virtual void restore(bool /*save_redo*/) { objEd->_addObjects((EditableObject **)(void *)objects.data(), objects.size(), false); }
  virtual void redo() { objEd->_removeObjects((EditableObject **)(void *)objects.data(), objects.size(), false); }

  virtual size_t size() { return sizeof(*this) + data_size(objects) + objects.size() * sizeof(EditableObject); }

  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoRemoveObjects"; }
};

class ObjectEditor::UndoObjectEditorRename : public UndoRedoObject
{
public:
  ObjectEditor *objEd;
  Ptr<EditableObject> object;
  String undoName, redoName;


  UndoObjectEditorRename(ObjectEditor *oe, EditableObject *obj) : objEd(oe), object(obj)
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
