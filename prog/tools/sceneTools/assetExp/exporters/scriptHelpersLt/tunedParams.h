// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <ioSys/dag_dataBlock.h>
#include "scriptHelpers.h"
#include <libTools/util/makeBindump.h>

class ObjectParameters;
class IPropPanelCB;


namespace ScriptHelpers
{

//! MUST be binary compatible with the other declaration in prog\tools\sharedInclude\scriptHelpers\tunedParams.h
class TunedElement
{
public:
  TunedElement() {}
  virtual ~TunedElement() {}

  const char *getName() const { return name; }
  void setName(const char *n) { name = n; }

  virtual int subElemCount() const = 0;
  virtual TunedElement *getSubElem(int index) const = 0;

  virtual void fillPropPanel(ObjectParameters &, int &, IPropPanelCB &) {}
  virtual void getValues(ObjectParameters &, int &) {}
  virtual bool onPPButtonPressed(int, IPropPanelCB &) { return false; }

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

  ~TunedGroup()
  {
    for (int i = 0; i < subElem.size(); ++i)
      if (subElem[i])
        delete subElem[i];
  }

  virtual TunedElement *cloneElem() { return new TunedGroup(*this); }

  virtual int subElemCount() const { return subElem.size(); }

  virtual TunedElement *getSubElem(int index) const
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

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    cwr.writeInt32e(version);

    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i])
        continue;
      subElem[i]->saveData(cwr, save_cb);
    }
  }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
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

  virtual void loadValues(const DataBlock &blk)
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

TunedGroup *create_tuned_struct(const char *name, int version, dag::ConstSpan<TunedElement *> elems);

TunedGroup *create_tuned_group(const char *name, int version, dag::ConstSpan<TunedElement *> elems);
}; // namespace ScriptHelpers
