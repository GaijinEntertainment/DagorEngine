// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "scriptHelpersPanelUserData.h"
#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/blkUtil.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include <scriptHelpers/tunedParams.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

#include <debug/dag_debug.h>

#include <propPanel/control/container.h>

using namespace ScriptHelpers;


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedStruct : public TunedGroup
{
public:
  TunedStruct(const char *nm, int ver, dag::ConstSpan<TunedElement *> elems) : TunedGroup(nm, ver, elems) {}

  TunedElement *cloneElem() override { return new TunedStruct(*this); }


  void resetPropPanel() override
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        subElem[i]->resetPropPanel();
  }
  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    PropPanel::ContainerPropertyControl *op = panel.createGroup(++pid, getName());

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;
      subElem[i]->fillPropPanel(pid, *op);
    }

    op->setBoolValue(true);
  }


  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    ++pid;

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;
      subElem[i]->getValues(pid, panel);
    }
  }
};


TunedGroup *create_tuned_struct(const char *name, int version, dag::ConstSpan<TunedElement *> elems)
{
  return new TunedStruct(name, version, elems);
}

TunedGroup *create_tuned_group(const char *name, int version, dag::ConstSpan<TunedElement *> elems)
{
  return new TunedGroup(name, version, elems);
}

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedRefSlot : public TunedElement
{
public:
  String typeName;


  TunedRefSlot(const char *nm, const char *type_name)
  {
    name = nm;
    typeName = type_name;
  }

  TunedElement *cloneElem() override { return new TunedRefSlot(*this); }

  int subElemCount() const override { return 0; }
  TunedElement *getSubElem(int index) const override { return NULL; }

  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override {}

  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override {}


  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    int id = -1;
    if (save_cb)
      id = save_cb->getRefSlotId(getName());

    cwr.writeInt32e(id);
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    if (save_cb)
      save_cb->addRefSlot(getName(), typeName);
  }

  void loadValues(const DataBlock &blk) override {}
};


TunedElement *create_tuned_ref_slot(const char *name, const char *type_name) { return new TunedRefSlot(name, type_name); }

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedParam : public TunedElement
{
public:
  TunedParam(const char *nm) { name = nm; }

  int subElemCount() const override { return 0; }
  TunedElement *getSubElem(int index) const override { return NULL; }

  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override {}
  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override {}

  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override = 0;

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override = 0;
  void loadValues(const DataBlock &blk) override = 0;
};

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


#define WRITE_DATA_SIMPLE   cwr.write32ex(&value, sizeof(value))
#define WRITE_DATA_E3DCOLOR cwr.write32ex(&value, sizeof(value))


#define SIMPLE_PARAM(type, add_method, get_method, blk_set, blk_get, write_stmnt)                                                  \
  namespace ScriptHelpers                                                                                                          \
  {                                                                                                                                \
  class TunedParam_##type : public TunedParam                                                                                      \
  {                                                                                                                                \
  public:                                                                                                                          \
    type value;                                                                                                                    \
                                                                                                                                   \
    TunedParam_##type(const char *nm, const type &val) : TunedParam(nm), value(val) {}                                             \
                                                                                                                                   \
    void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panl) override { panl.add_method(++pid, getName(), value); } \
    void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override { value = panel.get_method(++pid); }             \
                                                                                                                                   \
    void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override { write_stmnt; }                                    \
    void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override { blk.blk_set("value", value); }                               \
    void loadValues(const DataBlock &blk) override { value = blk.blk_get("value", value); }                                        \
                                                                                                                                   \
    TunedElement *cloneElem() override { return new TunedParam_##type(*this); }                                                    \
  };                                                                                                                               \
                                                                                                                                   \
  TunedElement *create_tuned_##type##_param(const char *name, const type &val) { return new TunedParam_##type(name, val); }        \
  }


SIMPLE_PARAM(real, createEditFloat, getFloat, setReal, getReal, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(int, createEditInt, getInt, setInt, getInt, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(bool, createCheckBox, getBool, setBool, getBool, cwr.writeInt32e(value));
SIMPLE_PARAM(E3DCOLOR, createColorBox, getColor, setE3dcolor, getE3dcolor, WRITE_DATA_E3DCOLOR);
SIMPLE_PARAM(Point2, createPoint2, getPoint2, setPoint2, getPoint2, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(Point3, createPoint3, getPoint3, setPoint3, getPoint3, WRITE_DATA_SIMPLE);


#undef SIMPLE_PARAM
#undef WRITE_DATA_SIMPLE


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedEnumParam : public TunedParam
{
public:
  Tab<EnumEntry> enumList;
  int value;


  TunedEnumParam(const char *nm, const Tab<EnumEntry> &list) : TunedParam(nm), enumList(list)
  {
    value = 0;
    if (enumList.size())
      value = enumList[0].value;
  }

  TunedElement *cloneElem() override { return new TunedEnumParam(*this); }

  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    PropPanel::ContainerPropertyControl *rgrp = panel.createRadioGroup(++pid, getName());
    int selPid = 0;

    for (int i = 0; i < enumList.size(); ++i)
    {
      rgrp->createRadio(i, enumList[i].name);
      if (value == enumList[i].value)
        selPid = i;
    }

    rgrp->setIntValue(selPid);
  }


  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    int index = panel.getInt(++pid);
    if ((index > -1) && (index < enumList.size()))
    {
      value = enumList[index].value;
    }
  }


  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override { cwr.writeInt32e(value); }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    for (int i = 0; i < enumList.size(); ++i)
      if (enumList[i].value == value)
      {
        blk.setStr("value", enumList[i].name);
        break;
      }

    blk.setInt("valueInt", value);
  }

  void loadValues(const DataBlock &blk) override
  {
    value = blk.getInt("valueInt", value);

    const char *s = blk.getStr("value", NULL);
    if (s)
    {
      for (int i = 0; i < enumList.size(); ++i)
        if (stricmp(enumList[i].name, s) == 0)
        {
          value = enumList[i].value;
          break;
        }
    }
  }
};


TunedElement *create_tuned_enum_param(const char *name, const Tab<EnumEntry> &list) { return new TunedEnumParam(name, list); }

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


/*
namespace ScriptHelpers
{

  class TunedData:public TunedElement
  {
  public:
    TunedElement *structElem;


    TunedData(Declaration &decl)
      : structElem(NULL)
    {
      name=String(decl.getName())+"_data";
      structElem=decl.createTunedElement();
    }

    TunedData(const TunedData &from) : TunedElement(from), structElem(NULL)
    {
      if (from.structElem) structElem=from.structElem->cloneElem();
    }

    TunedElement* cloneElem() override
    {
      return new TunedData(*this);
    }

    void fillPropPanel(ObjectParameters &op, int &pid, IPropPanelCB &ppcb) override
    {
      if (structElem) structElem->fillPropPanel(op, pid, ppcb);
    }

    void getValues(ObjectParameters &op, int &pid) override
    {
      if (structElem) structElem->getValues(op, pid);
    }

    int subElemCount() const override { return structElem?structElem->subElemCount():0; }

    TunedElement* getSubElem(int index) const override
    {
      return structElem?structElem->getSubElem(index):NULL;
    }

    void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
    {
      if (structElem) structElem->saveData(cb, save_cb);
    }

    void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
    {
      if (structElem) structElem->saveValues(blk, save_cb);
    }

    void loadValues(const DataBlock &blk) override
    {
      if (structElem) structElem->loadValues(blk);
    }
  };

};
//*/


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedArray : public TunedElement
{
public:
  TunedElement *baseElement;

  struct Elem
  {
    TunedElement *elem;
    int pid;

    Elem(TunedElement *e = NULL) : elem(e), pid(-1) {}

    ~Elem() { del_it(elem); }
  };

  Tab<Elem> subElem;

  int basePid;

  String memberToShowInCaption;


  TunedArray(const char *nm, TunedElement *be) : baseElement(be), subElem(midmem), basePid(-1) { name = nm; }

  TunedArray(const TunedArray &from) : TunedElement(from), baseElement(NULL), subElem(from.subElem), basePid(-1)
  {
    if (from.baseElement)
      baseElement = from.baseElement->cloneElem();

    for (int i = 0; i < subElem.size(); ++i)
    {
      subElem[i].pid = -1;
      if (subElem[i].elem)
        subElem[i].elem = subElem[i].elem->cloneElem();
    }
  }

  ~TunedArray() override
  {
    clear();
    del_it(baseElement);
  }

  void clear() { clear_and_shrink(subElem); }

  TunedElement *cloneElem() override { return new TunedArray(*this); }

  int subElemCount() const override { return subElem.size(); }

  TunedElement *getSubElem(int index) const override
  {
    G_ASSERT(index >= 0 && index < subElem.size());
    return subElem[index].elem;
  }

  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (!baseElement)
      return;

    PropPanel::ContainerPropertyControl *op = panel.createGroup(++pid, getName());
    // op->minimize();

    op->createButton(++pid, String("add ") + baseElement->getName());
    basePid = pid;

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i].elem)
        continue;

      String caption(100, "%d", i + 1);
      const bool useCustomCaption = !memberToShowInCaption.empty() && getPanelArrayItemCaptionUsingIndex(caption, *subElem[i].elem, i);

      PropPanel::ContainerPropertyControl *eop = op->createGroup(++pid, caption);
      G_ASSERT(eop && "fillPropPanel: Create group failed");
      // eop->minimize();

      if (useCustomCaption)
      {
        op->setUserDataValue(ScriptHelpersPanelUserData::make(this, ScriptHelpersPanelUserData::DataType::Array));
        eop->setUserDataValue(ScriptHelpersPanelUserData::make(subElem[i].elem, ScriptHelpersPanelUserData::DataType::ArrayItem));
      }

      subElem[i].pid = pid + 1;

      eop->createButton(++pid, "up");
      eop->createButton(++pid, "down", true, false);
      eop->createButton(++pid, "remove");
      eop->createButton(++pid, "clone", true, false);

      subElem[i].elem->fillPropPanel(pid, *eop);
    }
  }

  bool getPanelArrayItemCaptionUsingIndex(String &caption, const TunedElement &array_item, int array_item_index)
  {
    const int structMemberCount = array_item.subElemCount();
    for (int i = 0; i < structMemberCount; ++i)
    {
      TunedElement *member = array_item.getSubElem(i);
      if (strcmp(member->getName(), memberToShowInCaption) == 0)
      {
        DataBlock block;
        member->saveValues(block, nullptr);

        const int paramIndex = block.findParam("value");
        if (paramIndex >= 0)
        {
          const bool withParamName = false;
          const SimpleString paramStr = blk_util::paramStrValue(block, paramIndex, withParamName);

          caption.printf(memberToShowInCaption.size() + paramStr.length() + 10, "%d (%s=%s)", array_item_index + 1,
            memberToShowInCaption.c_str(), paramStr.c_str());
        }

        return true;
      }
    }

    return false;
  }

  bool getPanelArrayItemCaption(String &caption, const TunedElement &array, const TunedElement &array_item) override
  {
    if (memberToShowInCaption.empty())
      return false;

    const int subElemCount = array.subElemCount();
    for (int i = 0; i < subElemCount; ++i)
    {
      const TunedElement *current_item = array.getSubElem(i);
      if (current_item == &array_item)
        return getPanelArrayItemCaptionUsingIndex(caption, array_item, i);
    }

    return false;
  }

  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    if (!baseElement)
      return;

    pid++;
    pid++; // add button

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i].elem)
        continue;

      pid++;

      pid++; // up
      pid++; // down
      pid++; // remove
      pid++; // clone

      subElem[i].elem->getValues(pid, panel);
    }
  }


  bool onClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, IPropPanelCB &ppcb) override
  {
    if (TunedElement::onClick(pcb_id, panel, ppcb))
      return true;

    if (basePid != -1 && pcb_id == basePid)
    {
      TunedElement *e = baseElement->cloneElem();
      if (!e)
        return true;

      subElem.push_back().elem = e;

      ppcb.rebuildTreeList();
      return true;
    }
    else
    {
      for (int i = 0; i < subElem.size(); ++i)
      {
        if (!subElem[i].elem || subElem[i].pid == -1)
          continue;

        if (pcb_id == subElem[i].pid + 0) // up
        {
          if (i == 0)
            return true;

          TunedElement *e = subElem[i - 1].elem;
          subElem[i - 1].elem = subElem[i].elem;
          subElem[i].elem = e;

          ppcb.rebuildTreeList();
          return true;
        }
        else if (pcb_id == subElem[i].pid + 1) // down
        {
          if (i + 1 >= subElem.size())
            return true;

          TunedElement *e = subElem[i + 1].elem;
          subElem[i + 1].elem = subElem[i].elem;
          subElem[i].elem = e;

          ppcb.rebuildTreeList();
          return true;
        }
        else if (pcb_id == subElem[i].pid + 2) // remove
        {
          erase_items(subElem, i, 1);

          ppcb.rebuildTreeList();
          return true;
        }
        else if (pcb_id == subElem[i].pid + 3) // clone
        {
          TunedElement *e = subElem[i].elem->cloneElem();
          if (!e)
            return true;

          subElem[insert_items(subElem, i, 1)].elem = e;

          ppcb.rebuildTreeList();
          return true;
        }
      }
    }

    return false;
  }


  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    cwr.writeInt32e(subElem.size());

    for (int i = 0; i < subElem.size(); ++i)
    {
      G_ASSERT(subElem[i].elem);

      subElem[i].elem->saveData(cwr, save_cb);
    }
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i].elem)
        continue;

      DataBlock &sb = *blk.addNewBlock("elem");
      subElem[i].elem->saveValues(sb, save_cb);
    }
  }

  void loadValues(const DataBlock &blk) override
  {
    clear();

    if (!baseElement)
      return;

    int nameId = blk.getNameId("elem");

    for (int i = 0; i < blk.blockCount(); ++i)
    {
      const DataBlock &sb = *blk.getBlock(i);
      if (sb.getBlockNameId() != nameId)
        continue;

      TunedElement *e = baseElement->cloneElem();
      if (!e)
        continue;

      e->loadValues(sb);

      subElem.push_back().elem = e;
    }
  }
};


TunedElement *create_tuned_array(const char *name, TunedElement *base_elem) { return new TunedArray(name, base_elem); }


void set_tuned_array_member_to_show_in_caption(TunedElement &array, const char *member)
{
  static_cast<TunedArray &>(array).memberToShowInCaption = member;
}

}; // namespace ScriptHelpers
DAG_DECLARE_RELOCATABLE(ScriptHelpers::TunedArray::Elem);


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


ScriptHelpers::Context::Context() : rootElem(NULL) {}


ScriptHelpers::Context::~Context() { del_it(rootElem); }


void ScriptHelpers::Context::setTunedElement(TunedElement *elem)
{
  del_it(rootElem);
  rootElem = elem;
}


void ScriptHelpers::Context::loadParams(const DataBlock &blk)
{
  if (rootElem)
  {
    rootElem->loadValues(blk);
  }
}


void ScriptHelpers::Context::saveParams(DataBlock &blk, SaveValuesCB *save_cb)
{
  if (rootElem)
    rootElem->saveValues(blk, save_cb);
}


void ScriptHelpers::Context::saveParamsData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
{
  if (rootElem)
    rootElem->saveData(cwr, save_cb);
}
