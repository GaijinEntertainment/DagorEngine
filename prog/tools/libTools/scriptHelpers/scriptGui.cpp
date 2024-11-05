// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>

#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_rendEdObject.h>

#include <scriptHelpers/tunedParams.h>

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>


using namespace ScriptHelpers;


namespace ScriptHelpers
{
TunedElement *root_element = NULL;
TunedElement *selected_elem = NULL;

ObjectEditor *obj_editor = NULL;

ParamChangeCB *param_change_cb = NULL;
}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


void ScriptHelpers::set_tuned_element(TunedElement *elem)
{
  del_it(root_element);
  root_element = elem;
}


void ScriptHelpers::load_helpers(const DataBlock &blk)
{
  if (root_element)
    root_element->loadValues(blk);
}


void ScriptHelpers::save_helpers(DataBlock &blk, SaveValuesCB *save_cb)
{
  if (root_element)
    root_element->saveValues(blk, save_cb);
}


void ScriptHelpers::save_params_data(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
{
  if (root_element)
    root_element->saveData(cwr, save_cb);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class SimpleHelperObject : public RenderableEditableObject
{
public:
  real radius;


  SimpleHelperObject() : radius(0.5f) {}


  virtual void update(real dt) {}

  virtual void beforeRender() {}

  E3DCOLOR getRenderColor() const
  {
    if (isSelected())
      return E3DCOLOR(255, 255, 255);
    if (isFrozen())
      return E3DCOLOR(120, 120, 120);
    return E3DCOLOR(255, 200, 10);
  }

  virtual void render()
  {
    set_cached_debug_lines_wtm(getWtm());
    draw_cached_debug_sphere(Point3(0, 0, 0), radius, getRenderColor());
  }

  virtual void renderTrans() {}

  virtual bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const
  {
    return ::is_sphere_hit_by_point(getPos(), radius, vp, x, y);
  }

  virtual bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const
  {
    return ::is_sphere_hit_by_rect(getPos(), radius, vp, rect);
  }

  virtual bool getWorldBox(BBox3 &box) const
  {
    box.makecube(getPos(), radius * 2);
    return true;
  }


  virtual void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis)
  {
    real dr = delta.x * delta.y * delta.z;
    radius *= dr;
  }

  virtual void putScaleUndo()
  {
    /*
    if (objEditor)
      objEditor->getUndoSystem()->put(new(midmem) UndoRadius(this));
    */
  }

  virtual void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects)
  {}

protected:
  class UndoRadius : public UndoRedoObject
  {
  public:
    Ptr<SimpleHelperObject> obj;
    real oldRadius, redoRadius;


    UndoRadius(SimpleHelperObject *o) : obj(o) { oldRadius = redoRadius = obj->radius; }

    virtual void restore(bool save_redo)
    {
      if (save_redo)
        redoRadius = obj->radius;
      obj->radius = oldRadius;
    }

    virtual void redo() { obj->radius = redoRadius; }

    virtual size_t size() { return sizeof(*this); }
    virtual void accepted() {}
    virtual void get_description(String &s) { s = "UndoRadius"; }
  };
};


class TunedPosHelper : public TunedElement
{
public:
  Ptr<SimpleHelperObject> obj;


  TunedPosHelper(const char *n)
  {
    name = n;

    obj = new (midmem) SimpleHelperObject;
    obj->setName(getName());

    if (obj_editor)
      obj_editor->addObject(obj);
  }


  TunedPosHelper(const TunedPosHelper &from) : TunedElement(from)
  {
    obj = new (midmem) SimpleHelperObject(*from.obj);

    if (obj_editor)
      obj_editor->addObject(obj);
  }


  virtual TunedElement *cloneElem() { return new TunedPosHelper(*this); }

  virtual int subElemCount() const { return 0; }
  virtual TunedElement *getSubElem(int index) const { return NULL; }


  virtual void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) {}

  virtual void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) {}

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    Point3 pos = obj->getPos();
    cwr.write32ex(&pos, sizeof(pos));
  }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
  {
    Point3 pos = obj->getPos();
    blk.setPoint3("pos", pos);
  }

  virtual void loadValues(const DataBlock &blk)
  {
    Point3 pos = obj->getPos();
    pos = blk.getPoint3("pos", pos);
    obj->setPos(pos);
  }
};


TunedElement *create_tuned_pos_helper(const char *name) { return new TunedPosHelper(name); }

}; // namespace ScriptHelpers
