// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shCompiler.h"
#include "shaderVariant.h"
#include <debug/dag_debug.h>
#include "globVar.h"
#include <generic/dag_tabUtils.h>
#include <shaders/dag_shaders.h>
#include "shLog.h"

namespace ShaderVariant
{
// add type to a list
void TypeTable::addType(BaseType init_type, ExtType init_exttype, bool unique)
{
  if (unique && findType(init_type, init_exttype) >= 0)
    return;

  VariantType &vt = types.push_back();
  vt.type = init_type;
  vt.extType = init_exttype;
  vt.interval = NULL;

  if (iList)
    updateTypeValRange(vt);
}

void TypeTable::setIntervalList(IntervalList *ilist)
{
  iList = ilist;
  if (iList)
    for (int i = 0; i < types.size(); i++)
      updateTypeValRange(types[i]);
}

// find type from list
int TypeTable::findType(BaseType init_type, ExtType init_exttype) const
{
  for (int i = 0; i < types.size(); i++)
  {
    if (types[i].isEqual(init_type, init_exttype))
      return i;
  }
  return -1;
}

// remap value from some number to a inteval-index
ValueType TypeTable::normalizeValue(int index, real val) const
{
  G_ASSERT(tabutils::isCorrectIndex(types, index));
  const VariantType &t = types[index];

  if (t.type == VARTYPE_MODE)
  {
    ValueType v = (ValueType)val;
    if (!t.valRange.isInRange(v))
      t.valRange.rangeIt(v);
    return v;
  }
  else
  {
    if (!t.interval)
      sh_debug(SHLOG_FATAL, "normalize value %.4f: no interval ptr for interval type #%d in %s", val, index, (char *)getStringInfo());
    return t.interval->normalizeValue(val);
  }
}

// calculate type value range
void TypeTable::updateTypeValRange(VariantType &t)
{
  t.interval = NULL;
  switch (t.type)
  {
    case VARTYPE_MODE:
    {
      t.valRange.setBounds(0, 1);
    }
    break;
    case VARTYPE_INTERVAL:
    case VARTYPE_GL_OVERRIDE_INTERVAL:
    {
      if (!iList || (t.extType == INTERVAL_NOT_INIT))
        sh_debug(SHLOG_FATAL, "interval type not initialized!");

      t.interval = iList->getInterval(t.extType);
      if (!t.interval)
        sh_debug(SHLOG_FATAL, "interval not found!");
    }
    break;
    case VARTYPE_GLOBAL_INTERVAL:
    {
      if (t.extType == INTERVAL_NOT_INIT)
        sh_debug(SHLOG_FATAL, "interval type not initialized!");

      t.interval = ctx->globVars().getIntervalList().getInterval(t.extType);
      if (!t.interval)
        sh_debug(SHLOG_FATAL, "interval not found!");
    }
    break;
    default: G_ASSERTF(0, "unknown variant type - %d!", t.type);
  }

  if (t.interval)
    t.valRange = t.interval->getIndexRange();
}

// return string type name
String TypeTable::getTypeName(int i) const
{
  const VariantType &t = types[i];
  switch (t.type)
  {
    case VARTYPE_MODE:
    {
      switch (t.extType)
      {
        case TWO_SIDED: return String("2SIDED");
        case REAL_TWO_SIDED: return String("REAL_2SIDED");
      }
    }
    break;
    case VARTYPE_INTERVAL:
    {
      const Interval *interv = iList ? iList->getInterval(t.extType) : NULL;
      if (!interv)
        return String("IVAL[?]");

      return String(128, "IVAL[%s]", get_interval_name(*interv, *ctx));
    }
    case VARTYPE_GL_OVERRIDE_INTERVAL:
    {
      const Interval *interv = iList ? iList->getInterval(t.extType) : NULL;
      if (!interv)
        return String("G-o-IVAL[?]");

      return String(128, "G-o-IVAL[%s]", get_interval_name(*interv, *ctx));
    }
    case VARTYPE_GLOBAL_INTERVAL:
    {
      const Interval *interv = ctx->globVars().getIntervalList().getInterval(t.extType);
      if (!interv)
        return String("G-IVAL[?]");

      return String(128, "G-IVAL[%s]", get_interval_name(*interv, *ctx));
    }
    break;
  }

  return String("???");
}

String TypeTable::getValueName(int type_index, int value_index) const
{
  const VariantType &t = types[type_index];
  switch (t.type)
  {
    case VARTYPE_INTERVAL:
    case VARTYPE_GL_OVERRIDE_INTERVAL:
    {
      const Interval *interv = iList ? iList->getInterval(t.extType) : nullptr;
      if (!interv)
        break;

      return String(128, "%s", get_interval_value_name(*interv, value_index, *ctx));
    }
    case VARTYPE_GLOBAL_INTERVAL:
    {
      const Interval *interv = ctx->globVars().getIntervalList().getInterval(t.extType);
      if (!interv)
        break;

      return String(128, "%s", get_interval_value_name(*interv, value_index, *ctx));
    }
  }

  return String(0, "%i", value_index);
}

const char *TypeTable::getIntervalName(int i) const
{
  const VariantType &t = types[i];
  switch (t.type)
  {
    case VARTYPE_MODE:
    {
      switch (t.extType)
      {
        case TWO_SIDED: return "two_sided";
        case REAL_TWO_SIDED: return "real_two_sided";
      }
    }
    break;

    case VARTYPE_INTERVAL:
    case VARTYPE_GL_OVERRIDE_INTERVAL: return get_interval_name(*iList->getInterval(t.extType), *ctx);

    case VARTYPE_GLOBAL_INTERVAL: return get_interval_name(*ctx->globVars().getIntervalList().getInterval(t.extType), *ctx);
  }
  return "?t";
}

// dump to string info about types
String TypeTable::getStringInfo() const
{
  String out;

  for (int i = 0; i < types.size(); i++)
  {
    if (out.length())
      out.aprintf(2, " ");
    out.aprintf(64, "%s", (char *)getTypeName(i));
  }

  return "type=<" + out + ">";
}

void TypeTable::link(Tab<ExtType> &interval_link_table)
{
  for (unsigned int typeNo = 0; typeNo < types.size(); typeNo++)
  {
    if (types[typeNo].type == VARTYPE_GLOBAL_INTERVAL)
    {
      G_ASSERT(types[typeNo].extType != INTERVAL_NOT_INIT && types[typeNo].extType < interval_link_table.size());
      types[typeNo].extType = interval_link_table[(int)types[typeNo].extType];
    }
  }
}

VariantTable::VariantTable(shc::TargetContext &a_ctx) : types{a_ctx}, variants(midmem), cachedIndex(-1), ctx{&a_ctx} {}

// create search info values for this variants
SearchInfo *VariantTable::createSearchInfo(bool fill_with_defaults) { return new SearchInfo(types, fill_with_defaults); }

// get variant by index
Variant *VariantTable::getVariant(int index)
{
  if (tabutils::isCorrectIndex(variants, index))
    return &variants[index];
  else
    return NULL;
}

// find variant. return NULL, if not found
Variant *VariantTable::findVariant(const SearchInfo &si)
{
  if (cachedIndex >= 0 && variants[cachedIndex].isEqual(si))
    return &variants[cachedIndex];

  for (int i = 0; i < variants.size(); i++)
  {
    if (i != cachedIndex && variants[i].isEqual(si))
    {
      cachedIndex = i;
      return &variants[i];
    }
  }

  return NULL;
}

void VariantTable::linkIntervalList()
{
  types.setIntervalList(&intervals);
  cachedIndex = -1;
}

void VariantTable::link(Tab<ExtType> &interval_link_table)
{
  linkIntervalList();
  types.link(interval_link_table);
}
} // namespace ShaderVariant
