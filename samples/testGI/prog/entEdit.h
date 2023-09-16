#pragma once

#include <daEditor4/de4_objEditor.h>

enum
{
  CID_EntityObj = 0x354A2D0Bu
}; // EntityObj

//
// Entity object placed on landscape
//
class EntityObj : public EditableObject
{
public:
  EO_IMPLEMENT_RTTI(CID_EntityObj, EditableObject)

  EntityObj(const char *ent_name)
  {
    props.entityName = ent_name;
    props.placeOnCollision = true;
    props.useCollisionNorm = false;
  }

  virtual bool isValid() const { return true; }

  // EntityObj *clone();

  virtual void update(float) {}
  virtual void beforeRender() {}
  virtual void render() { renderBox(getLbb()); }
  virtual void renderTrans() {}

  virtual bool isSelectedByRectangle(const IBBox2 &rect) const { return isBoxSelectedByRectangle(getLbb(), rect); }
  virtual bool isSelectedByPointClick(int x, int y) const { return isBoxSelectedByPointClick(getLbb(), x, y); }
  virtual bool getWorldBox(BBox3 &box) const
  {
    box = matrix * BSphere3(Point3(0, 0.5, 0), 0.5);
    return true;
  }
  BBox3 getLbb() const { return BBox3(Point3(-0.5, 0, -0.5), Point3(0.5, 1, 0.5)); }

  /*
    virtual void fillProps(PropPanel2 &panel,
      DClassID for_class_id, dag::ConstSpan<EditableObject*> objects);

    virtual void onPPChange(int pid, bool edit_finished,
      PropPanel2 &panel,
      dag::ConstSpan<EditableObject*> objects);

    virtual void onPPBtnPressed(int pid, PropPanel2 &panel,
      dag::ConstSpan<EditableObject*> objects);
  */

  virtual bool mayRename() { return true; }
  virtual bool mayDelete() { return true; }
  virtual void setWtm(const TMatrix &wtm)
  {
    EditableObject::setWtm(wtm);
    updateEntityPosition(true);
  }

  virtual void onRemove(ObjectEditor *) {}
  virtual void onAdd(ObjectEditor *objEditor)
  {
    propsChanged();

    if (name.empty())
      objEditor->setUniqName(this, props.entityName);
  }

  virtual bool setPos(const Point3 &p)
  {
    if (!EditableObject::setPos(p))
      return false;
    updateEntityPosition(true);
    return true;
  }
  virtual void onObjectNameChange(EditableObject *, const char *, const char *) { objectPropsChanged(); }

  UndoRedoObject *makePropsUndoObj() { return new UndoPropsChange(this); }

  void setPlaceOnCollision(bool) {}
  void updateEntityPosition(bool = false) {}

  struct Props
  {
    SimpleString entityName;
    bool placeOnCollision;
    bool useCollisionNorm;

    String notes;
  };

  const Props &getProps() const { return props; }
  void setProps(const Props &p)
  {
    props = p;
    propsChanged();
  }

  void propsChanged() {}
  void objectPropsChanged() {}

protected:
  Props props;

  ~EntityObj() {}

  void setPosOnCollision(Point3 pos);

  void rePlaceAllEntities();

  class UndoPropsChange : public UndoRedoObject
  {
    Ptr<EntityObj> obj;
    EntityObj::Props oldProps, redoProps;

  public:
    UndoPropsChange(EntityObj *o) : obj(o) { oldProps = redoProps = obj->props; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoProps = obj->props;
      obj->setProps(oldProps);
    }
    virtual void redo() { obj->setProps(redoProps); }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoEntityPropsChange"; }
  };
};

class EntityObjEditor : public ObjectEditor
{
public:
  EntityObjEditor()
  {
    EntityObj *o;

    o = new EntityObj(NULL);
    addObject(o, false);
    o->setPos(Point3(-10, 2, -10));

    o = new EntityObj(NULL);
    addObject(o, false);
    o->setPos(Point3(-12, 5, -12));

    // selectAll();
    // setEditMode(CM_OBJED_MODE_ROTATE);
  }
};
