#include <util/dag_string.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/makeBindump.h>
#include "tunedParams.h"
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>

#include <debug/dag_debug.h>

using namespace ScriptHelpers;


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


namespace ScriptHelpers
{

class TunedStruct : public TunedGroup
{
public:
  TunedStruct(const char *nm, int ver, dag::ConstSpan<TunedElement *> elems) : TunedGroup(nm, ver, elems) {}

  virtual TunedElement *cloneElem() { return new TunedStruct(*this); }
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

  virtual TunedElement *cloneElem() { return new TunedRefSlot(*this); }

  virtual int subElemCount() const { return 0; }
  virtual TunedElement *getSubElem(int index) const { return NULL; }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    int id = -1;
    if (save_cb)
      id = save_cb->getRefSlotId(getName());

    cwr.writeInt32e(id);
  }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
  {
    if (save_cb)
      save_cb->addRefSlot(getName(), typeName);
  }

  virtual void loadValues(const DataBlock &blk) {}
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

  virtual int subElemCount() const { return 0; }
  virtual TunedElement *getSubElem(int index) const { return NULL; }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) = 0;

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb) = 0;
  virtual void loadValues(const DataBlock &blk) = 0;
};

}; // namespace ScriptHelpers


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


#define WRITE_DATA_SIMPLE   cwr.write32ex(&value, sizeof(value))
#define WRITE_DATA_E3DCOLOR cwr.write32ex(&value, sizeof(value))

#define SIMPLE_PARAM(type, add_method, get_method, blk_set, blk_get, write_stmnt)   \
  namespace ScriptHelpers                                                           \
  {                                                                                 \
  class TunedParam_##type : public TunedParam                                       \
  {                                                                                 \
  public:                                                                           \
    type value;                                                                     \
                                                                                    \
    TunedParam_##type(const char *nm, const type &val) : TunedParam(nm), value(val) \
    {}                                                                              \
                                                                                    \
    virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)       \
    {                                                                               \
      write_stmnt;                                                                  \
    }                                                                               \
                                                                                    \
    virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)                  \
    {                                                                               \
      blk.blk_set("value", value);                                                  \
    }                                                                               \
                                                                                    \
    virtual void loadValues(const DataBlock &blk)                                   \
    {                                                                               \
      value = blk.blk_get("value", value);                                          \
    }                                                                               \
                                                                                    \
    virtual TunedElement *cloneElem()                                               \
    {                                                                               \
      return new TunedParam_##type(*this);                                          \
    }                                                                               \
  };                                                                                \
                                                                                    \
  TunedElement *create_tuned_##type##_param(const char *name, const type &val)      \
  {                                                                                 \
    return new TunedParam_##type(name, val);                                        \
  }                                                                                 \
  }


SIMPLE_PARAM(real, addReal, getReal, setReal, getReal, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(int, addInt, getInt, setInt, getInt, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(bool, addCheck, getCheck, setBool, getBool, cwr.writeInt32e(value));
SIMPLE_PARAM(E3DCOLOR, addE3DCOLOR, getE3DCOLOR, setE3dcolor, getE3dcolor, WRITE_DATA_E3DCOLOR);
SIMPLE_PARAM(Point2, addPoint2, getPoint2, setPoint2, getPoint2, WRITE_DATA_SIMPLE);
SIMPLE_PARAM(Point3, addPoint3, getPoint3, setPoint3, getPoint3, WRITE_DATA_SIMPLE);


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

  virtual TunedElement *cloneElem() { return new TunedEnumParam(*this); }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb) { cwr.writeInt32e(value); }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
  {
    for (int i = 0; i < enumList.size(); ++i)
      if (enumList[i].value == value)
      {
        blk.setStr("value", enumList[i].name);
        break;
      }

    blk.setInt("valueInt", value);
  }

  virtual void loadValues(const DataBlock &blk)
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

  ~TunedArray()
  {
    clear();
    del_it(baseElement);
  }

  void clear() { clear_and_shrink(subElem); }

  virtual TunedElement *cloneElem() { return new TunedArray(*this); }

  virtual int subElemCount() const { return subElem.size(); }

  virtual TunedElement *getSubElem(int index) const
  {
    G_ASSERT(index >= 0 && index < subElem.size());
    return subElem[index].elem;
  }

  virtual void saveData(mkbindump::BinDumpSaveCB &cwr, SaveDataCB *save_cb)
  {
    cwr.writeInt32e(subElem.size());

    for (int i = 0; i < subElem.size(); ++i)
    {
      G_ASSERT(subElem[i].elem);

      subElem[i].elem->saveData(cwr, save_cb);
    }
  }

  virtual void saveValues(DataBlock &blk, SaveValuesCB *save_cb)
  {
    for (int i = 0; i < subElem.size(); ++i)
    {
      if (!subElem[i].elem)
        continue;

      DataBlock &sb = *blk.addNewBlock("elem");
      subElem[i].elem->saveValues(sb, save_cb);
    }
  }

  virtual void loadValues(const DataBlock &blk)
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


void set_tuned_array_member_to_show_in_caption(TunedElement &array, const char *member) {}

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
