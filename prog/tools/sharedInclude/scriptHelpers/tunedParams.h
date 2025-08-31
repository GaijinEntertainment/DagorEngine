// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <scriptHelpers/scriptHelpers.h>
#include <libTools/util/makeBindump.h>


namespace PropPanel
{
class ContainerPropertyControl;
}

namespace ScriptHelpers
{
class IPropPanelCB
{
public:
  virtual void rebuildTreeList() = 0;
};


class TunedElement
{
public:
  TunedElement() {}
  virtual ~TunedElement() {}

  const char *getName() const { return name; }
  void setName(const char *n) { name = n; }

  virtual int subElemCount() const = 0;
  virtual TunedElement *getSubElem(int index) const = 0;

  virtual void resetPropPanel() {}
  virtual void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) = 0;
  virtual bool getPanelArrayItemCaption(String &caption, const TunedElement &array, const TunedElement &array_item) { return false; }
  virtual void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) = 0;

  virtual bool onClick(int pcb_id, PropPanel::ContainerPropertyControl &panel, IPropPanelCB &ppcb)
  {
    int num = subElemCount();
    for (int i = 0; i < num; ++i)
    {
      TunedElement *e = getSubElem(i);
      if (!e)
        continue;

      if (e->onClick(pcb_id, panel, ppcb))
        return true;
    }
    return false;
  }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) = 0;

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb) = 0;
  virtual void loadValues(const DataBlock &blk) = 0;

  virtual TunedElement *cloneElem() = 0;

protected:
  String name;
};

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedGroup : public TunedElement
{
public:
  Tab<TunedElement *> subElem;
  int version;


  TunedGroup(const char *nm, int ver, dag::ConstSpan<TunedElement *> elems) : subElem(midmem), version(ver)
  {
    name = nm;

    subElem.reserve(elems.size());

    for (int i = 0; i < elems.size(); ++i)
    {
      if (!elems[i])
        continue;
      subElem.push_back(elems[i]);
    }
  }

  TunedGroup(const TunedGroup &from) : TunedElement(from), subElem(from.subElem), version(from.version)
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        subElem[i] = subElem[i]->cloneElem();
  }

  ~TunedGroup() override
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        delete subElem[i];
  }

  TunedElement *cloneElem() override { return new TunedGroup(*this); }

  int subElemCount() const override { return subElem.size(); }

  TunedElement *getSubElem(int index) const override
  {
    G_ASSERT(index >= 0 && index < subElem.size());
    return subElem[index];
  }

  void addSubElem(TunedElement *e)
  {
    if (!e)
      return;
    subElem.push_back(e);
  }


  void resetPropPanel() override
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        subElem[i]->resetPropPanel();
  }
  void fillPropPanel(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        subElem[i]->fillPropPanel(pid, panel);
  }

  void getValues(int &pid, PropPanel::ContainerPropertyControl &panel) override
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        subElem[i]->getValues(pid, panel);
  }


  void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) override
  {
    cwr.writeInt32e(version);

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;
      subElem[i]->saveData(cwr, save_cb);
    }
  }

  void saveValues(DataBlock &blk, SaveValuesCB *save_cb) override
  {
    blk.setInt("version", version);

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;

      DataBlock &sb = *blk.addBlock(subElem[i]->getName());
      subElem[i]->saveValues(sb, save_cb);
    }
  }

  void loadValues(const DataBlock &blk) override
  {
    int blkVer = blk.getInt("version", 0);
    //== TODO: handle old versions

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;

      const DataBlock *sb = blk.getBlockByName(subElem[i]->getName());
      if (!sb)
        continue;

      subElem[i]->loadValues(*sb);
    }
  }
};

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{
struct EnumEntry
{
  String name;
  int value;

  EnumEntry() : value(0) {}
};


TunedElement *create_tuned_real_param(const char *name, const real &def_val);
TunedElement *create_tuned_int_param(const char *name, const int &def_val);
TunedElement *create_tuned_bool_param(const char *name, const bool &def_val);
TunedElement *create_tuned_E3DCOLOR_param(const char *name, const E3DCOLOR &def_val);
TunedElement *create_tuned_Point2_param(const char *name, const Point2 &def_val);
TunedElement *create_tuned_Point3_param(const char *name, const Point3 &def_val);
TunedElement *create_tuned_enum_param(const char *name, const Tab<EnumEntry> &list);

TunedElement *create_tuned_cubic_curve(const char *name, E3DCOLOR color);
TunedElement *create_tuned_gradient_box(const char *name);


TunedElement *create_tuned_ref_slot(const char *name, const char *type_name);

TunedElement *create_tuned_array(const char *name, TunedElement *base_element);
void set_tuned_array_member_to_show_in_caption(TunedElement &array, const char *member);

TunedGroup *create_tuned_struct(const char *name, int version, dag::ConstSpan<TunedElement *> elems);

TunedGroup *create_tuned_group(const char *name, int version, dag::ConstSpan<TunedElement *> elems);
}; // namespace ScriptHelpers
