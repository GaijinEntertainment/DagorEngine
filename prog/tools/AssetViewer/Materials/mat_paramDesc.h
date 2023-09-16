#pragma once


#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point2.h>
#include <util/dag_string.h>
#include <generic/dag_tab.h>


class DataBlock;


class MaterialParamDescr
{
public:
  enum Type
  {
    PT_UNKNOWN,
    PT_TEXTURE,
    PT_TRIPLE_INT,
    PT_TRIPLE_REAL,
    PT_E3DCOLOR,
    PT_COMBO,
    PT_CUSTOM,
  };

  MaterialParamDescr(const DataBlock &blk) : comboVals(tmpmem) { loadFromBlk(blk); }

  Type getType() const { return type; }
  const char *getName() const { return name; }
  const char *getCaption() const { return caption; }
  int getSlot() const { return slot; }
  const IPoint2 &getIntConstains() const { return iConstaraints; }
  const Point2 &getRealConstains() const { return rConstaraints; }
  const Tab<String> &getComboVals() const { return comboVals; }
  const char *getDefStrVal() const { return defStrVal; }
  const char *getTexPath() const { return texPath; }

  void loadFromBlk(const DataBlock &blk);

private:
  Type type;
  Tab<String> comboVals;
  String name;
  String caption;
  IPoint2 iConstaraints;
  Point2 rConstaraints;
  String defStrVal;
  String texPath;
  int slot;

  static const char *typeToStr(Type type);
  static Type strToType(const char *type);
};
