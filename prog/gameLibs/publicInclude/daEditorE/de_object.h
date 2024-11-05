//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daEditorE/de_interface.h>
#include <../../tools/sharedInclude/libTools/util/undo.h>
#include <math/dag_math3d.h>
#include <math/integer/dag_IBBox2.h>
#include <util/dag_simpleString.h>
#include <generic/dag_DObject.h>
#include <generic/dag_tab.h>

class ObjectEditor;

#define EO_IMPLEMENT_RTTI(id, BASE)           \
  static DClassID getStaticClassId()          \
  {                                           \
    return id;                                \
  }                                           \
  virtual bool isSubOf(DClassID cid)          \
  {                                           \
    return cid == (id) || BASE::isSubOf(cid); \
  }                                           \
  virtual DClassID getClassId()               \
  {                                           \
    return id;                                \
  }


// EditableObject class ID
static constexpr unsigned CID_EditableObject = 0x14ACB0C1u; // EditableObject


class EditableObject : public DObject
{
public:
  enum EditableObjectFlags
  {
    FLG_SELECTED = 1 << 0,
    FLG_HIDDEN = 1 << 1,
    FLG_FROZEN = 1 << 2,
    FLG_HIGHLIGHTED = 1 << 3,
    FLG_WANTRESELECT = 1 << 4,
  };

  inline EditableObject() : matrix(TMatrix::IDENT), objFlags(0), surfaceDist(0), objEditor(NULL) {}
  inline EditableObject(const EditableObject &o) :
    name(o.name), matrix(o.matrix), objFlags(o.objFlags), surfaceDist(o.surfaceDist), objEditor(NULL)
  {}
  virtual ~EditableObject();

  virtual EditableObject *cloneObject() { return NULL; }
  virtual const char *getName() const { return name; }
  virtual bool setName(const char *nm)
  {
    name = nm;
    return true;
  }

  virtual Point3 getPos() const { return matrix.getcol(3); }
  virtual TMatrix getTm() const { return matrix; }
  virtual bool setPos(const Point3 &p);

  virtual bool isValid() const = 0;
  virtual bool isSelectedByPointClick(int x, int y) const = 0;
  virtual bool isSelectedByRectangle(const IBBox2 &rect) const = 0;

  virtual void update(float dt) = 0;
  virtual void beforeRender() = 0;
  virtual void render() = 0;
  virtual void renderTrans() = 0;

  inline bool isHidden() const { return objFlags & FLG_HIDDEN; }
  inline bool isFrozen() const { return objFlags & FLG_FROZEN; }
  inline bool isSelected() const { return objFlags & FLG_SELECTED; }
  inline bool isHighlighted() const { return objFlags & FLG_HIGHLIGHTED; }

  int getFlags() const { return objFlags; }
  virtual void setFlags(int value, int mask, bool use_undo = true);

  virtual void hideObject(bool hide = true) { setFlags(hide ? FLG_HIDDEN : 0, FLG_HIDDEN | (hide ? FLG_SELECTED : 0)); }
  virtual void freezeObject(bool freeze = true) { setFlags(freeze ? FLG_FROZEN : 0, FLG_FROZEN | (freeze ? FLG_SELECTED : 0)); }
  virtual void highlightObject(bool highlight = true) { setFlags(highlight ? FLG_HIGHLIGHTED : 0, FLG_HIGHLIGHTED); }
  virtual void selectObject(bool select = true) { setFlags(select ? FLG_SELECTED : 0, FLG_SELECTED); }

  virtual bool getWorldBox(BBox3 &box) const = 0;
  virtual BBox3 getLbb() const = 0;
  virtual bool setSize(const Point3 &p);
  virtual Point3 getSize() const;
  virtual bool setRotM3(const Matrix3 &tm);
  virtual bool setRotTm(const TMatrix &tm);
  virtual Matrix3 getRotM3() const;
  virtual TMatrix getRotTm() const;
  virtual Matrix3 getMatrix() const;
  virtual void setMatrix(const Matrix3 &tm);
  virtual TMatrix getWtm() const { return matrix; }
  virtual void setWtm(const TMatrix &wtm);

  virtual void moveObject(const Point3 &delta, IDaEditor4Engine::BasisType basis);
  virtual void moveSurfObject(const Point3 &delta, IDaEditor4Engine::BasisType basis);
  virtual void rotateObject(const Point3 &delta, const Point3 &origin, IDaEditor4Engine::BasisType basis);
  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IDaEditor4Engine::BasisType basis);

  virtual void putMoveUndo();
  virtual void putRotateUndo();
  virtual void putScaleUndo();

  virtual bool mayDelete() { return true; }
  virtual bool mayRename() { return true; }

  virtual void onObjectNameChange(EditableObject * /*obj*/, const char * /*old_name*/, const char * /*new_name*/) {}

  ObjectEditor *getObjEditor() const { return objEditor; }

  virtual DClassID getCommonClassId(EditableObject **objects, int num);

  virtual void onRemove(ObjectEditor * /*objEditor*/) {}
  virtual void onAdd(ObjectEditor * /*objEditor*/) {}

  virtual void rememberSurfaceDist();
  virtual void zeroSurfaceDist() { surfaceDist = 0.0; }
  virtual float getSurfaceDist() const { return surfaceDist; }

  static DClassID getStaticClassId() { return CID_EditableObject; }
  virtual bool isSubOf(DClassID cid) { return cid == CID_EditableObject; }
  virtual DClassID getClassId() { return CID_EditableObject; }

  void removeFromEditor();
  void renderBox(const BBox3 &lbb) const;
  bool isPivotSelectedByPointClick(int x, int y, int max_scr_dist = 3) const;
  bool isPivotSelectedByRectangle(const IBBox2 &rect) const;
  bool isBoxSelectedByPointClick(const BBox3 &lbb, int x, int y) const;
  bool isBoxSelectedByRectangle(const BBox3 &lbb, const IBBox2 &rect) const;
  bool isSphSelectedByPointClick(const BSphere3 &lbs, int x, int y) const;
  bool isSphSelectedByRectangle(const BSphere3 &lbs, const IBBox2 &rect) const;

  static bool is_sphere_hit_by_point(const Point3 &world_center, float world_rad, int x, int y);
  static bool is_sphere_hit_by_rect(const Point3 &world_center, float world_rad, const IBBox2 &rect);
  static float get_world_rad(const Point3 &size);

protected:
  friend class ObjectEditor;

  ObjectEditor *objEditor;
  SimpleString name;
  TMatrix matrix;
  int objFlags;
  float surfaceDist;

  class UndoObjFlags;
  class UndoMove;
  class UndoMatrix;
};

template <class T>
T *RTTI_cast(EditableObject *source)
{
  return (source && source->isSubOf(T::getStaticClassId())) ? (T *)source : NULL;
}


class EditableObject::UndoObjFlags : public UndoRedoObject
{
public:
  Ptr<EditableObject> obj;
  int prevFlags, redoFlags;

  UndoObjFlags(EditableObject *o) : obj(o), prevFlags(o->getFlags()) { redoFlags = prevFlags; }

  virtual void restore(bool save_redo)
  {
    if (save_redo)
      redoFlags = obj->getFlags();
    obj->setFlags(prevFlags, ~0);
  }
  virtual void redo() { obj->setFlags(redoFlags, ~0); }

  virtual size_t size() { return sizeof(*this); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoObjFlags"; }
};

class EditableObject::UndoMove : public UndoRedoObject
{
public:
  Ptr<EditableObject> obj;
  Point3 oldPos, redoPos;

  UndoMove(EditableObject *o) : obj(o) { oldPos = redoPos = obj->getPos(); }

  virtual void restore(bool save_redo)
  {
    if (save_redo)
      redoPos = obj->getPos();
    obj->setPos(oldPos);
  }
  virtual void redo() { obj->setPos(redoPos); }

  virtual size_t size() { return sizeof(*this); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoMove"; }
};

class EditableObject::UndoMatrix : public UndoRedoObject
{
public:
  Ptr<EditableObject> obj;
  Matrix3 oldMatrix, redoMatrix;

  UndoMatrix(EditableObject *o) : obj(o) { oldMatrix = redoMatrix = obj->getMatrix(); }

  virtual void restore(bool save_redo)
  {
    if (save_redo)
      redoMatrix = obj->getMatrix();
    obj->setMatrix(oldMatrix);
  }
  virtual void redo() { obj->setMatrix(redoMatrix); }

  virtual size_t size() { return sizeof(*this); }
  virtual void accepted() {}
  virtual void get_description(String &s) { s = "UndoMatrix"; }
};

class IObjectsList
{
public:
  virtual void getObjNames(Tab<SimpleString> &names, Tab<SimpleString> &sel_names, const Tab<int> &types) = 0;
  virtual void getTypeNames(Tab<SimpleString> &names) = 0;
  virtual void onSelectedNames(const Tab<SimpleString> &names) = 0;
};
