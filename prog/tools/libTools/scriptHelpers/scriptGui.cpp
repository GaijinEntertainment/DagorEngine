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


  void update(real dt) override {}

  void beforeRender() override {}

  E3DCOLOR getRenderColor() const
  {
    if (isSelected())
      return E3DCOLOR(255, 255, 255);
    if (isFrozen())
      return E3DCOLOR(120, 120, 120);
    return E3DCOLOR(255, 200, 10);
  }

  void render() override
  {
    set_cached_debug_lines_wtm(getWtm());
    draw_cached_debug_sphere(Point3(0, 0, 0), radius, getRenderColor());
  }

  void renderTrans() override {}

  bool isSelectedByPointClick(IGenViewportWnd *vp, int x, int y) const override
  {
    return ::is_sphere_hit_by_point(getPos(), radius, vp, x, y);
  }

  bool isSelectedByRectangle(IGenViewportWnd *vp, const EcRect &rect) const override
  {
    return ::is_sphere_hit_by_rect(getPos(), radius, vp, rect);
  }

  bool getWorldBox(BBox3 &box) const override
  {
    box.makecube(getPos(), radius * 2);
    return true;
  }


  void scaleObject(const Point3 &delta, const Point3 &origin, IEditorCoreEngine::BasisType basis) override
  {
    real dr = delta.x * delta.y * delta.z;
    radius *= dr;
  }

  void putScaleUndo() override
  {
    /*
    if (objEditor)
      objEditor->getUndoSystem()->put(new(midmem) UndoRadius(this));
    */
  }

  void onPPChange(int pid, bool edit_finished, PropPanel::ContainerPropertyControl &panel,
    dag::ConstSpan<RenderableEditableObject *> objects) override
  {}

protected:
  class UndoRadius : public UndoRedoObject
  {
  public:
    Ptr<SimpleHelperObject> obj;
    real oldRadius, redoRadius;


    UndoRadius(SimpleHelperObject *o) : obj(o) { oldRadius = redoRadius = obj->radius; }

    void restore(bool save_redo) override
    {
      if (save_redo)
        redoRadius = obj->radius;
      obj->radius = oldRadius;
    }

    void redo() override { obj->radius = redoRadius; }

    size_t size() override { return sizeof(*this); }
    void accepted() override {}
    void get_description(String &s) override { s = "UndoRadius"; }
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


  TunedElement *cloneElem() override { return new TunedPosHelper(*this); }

  int subElemCount() const override { return 0; }
  TunedElement *getSubElem(int index) const override { return NULL; }


  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override {}

  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override {}

  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    Point3 pos = obj->getPos();
    cwr.write32ex(&pos, sizeof(pos));
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    Point3 pos = obj->getPos();
    blk.setPoint3("pos", pos);
  }

  void loadValues(const DataBlock &blk) override
  {
    Point3 pos = obj->getPos();
    pos = blk.getPoint3("pos", pos);
    obj->setPos(pos);
  }
};


TunedElement *create_tuned_pos_helper(const char *name) { return new TunedPosHelper(name); }

}; // namespace ScriptHelpers
