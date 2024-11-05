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


    virtual TunedElement* cloneElem()
    {
      return new TunedPosHelper(*this);
    }

    virtual int subElemCount() const { return 0; }
    virtual TunedElement* getSubElem(int index) const { return NULL; }

    virtual void fillPropPanel(ObjectParameters &op, int &pid, IPropPanelCB &ppcb)
    {
    }

    virtual void getValues(ObjectParameters &op, int &pid)
    {
    }

    virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
    {
      Point3 pos=obj->getPos();
      cwr.write32ex(&pos, sizeof(pos));
    }

    virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
    {
      Point3 pos=obj->getPos();
      blk.setPoint3("pos", pos);
    }

    virtual void loadValues(const DataBlock &blk)
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
