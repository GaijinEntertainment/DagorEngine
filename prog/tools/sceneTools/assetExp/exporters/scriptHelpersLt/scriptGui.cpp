// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_genIo.h>

#include "tunedParams.h"

#include <debug/dag_debug3d.h>
#include <debug/dag_debug.h>
#include <EASTL/unique_ptr.h>

using namespace ScriptHelpers;


namespace ScriptHelpers
{
eastl::unique_ptr<TunedElement> root_element;
TunedElement *selected_elem = NULL;
}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//

void ScriptHelpers::set_tuned_element(TunedElement *elem) { root_element.reset(elem); }


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
/*
  class TunedPosHelper:public TunedElement
  {
  public:
    Ptr<SimpleHelperObject> obj;


    TunedPosHelper(const char *n)
    {
      name=n;

      obj=new(midmem) SimpleHelperObject;
      obj->setName(getName());

      if (obj_editor)
        obj_editor->addObject(obj);
    }


    TunedPosHelper(const TunedPosHelper &from) : TunedElement(from)
    {
      obj=new(midmem) SimpleHelperObject(*from.obj);

      if (obj_editor)
        obj_editor->addObject(obj);
    }


    TunedElement* cloneElem() override
    {
      return new TunedPosHelper(*this);
    }

    int subElemCount() const override { return 0; }
    TunedElement* getSubElem(int index) const override { return NULL; }

    void fillPropPanel(ObjectParameters &op, int &pid, IPropPanelCB &ppcb) override {}

    void getValues(ObjectParameters &op, int &pid) override {}

    void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
    {
      Point3 pos=obj->getPos();
      cwr.write32ex(&pos, sizeof(pos));
    }

    void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
    {
      Point3 pos=obj->getPos();
      blk.setPoint3("pos", pos);
    }

    void loadValues(const DataBlock &blk) override
    {
      Point3 pos=obj->getPos();
      pos=blk.getPoint3("pos", pos);
      obj->setPos(pos);
    }
  };


  TunedElement* create_tuned_pos_helper(const char *name)
  {
    return new TunedPosHelper(name);
  }

*/
};
