// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shaderVariantSrc.h"
#include <generic/dag_tabUtils.h>
#include <generic/dag_smallTab.h>
#include <debug/dag_debug.h>
#include "globVar.h"
#include "intervals.h"
#include "shCompiler.h"
#include <shaders/dag_shaders.h>
#include "sh_stat.h"
#include "varMap.h"

namespace ShaderVariant
{
TypeTable *VariantSrc::defCtorTypeTable = NULL;

// return string info about variant
String VariantSrc::getVarStringInfo() const
{
  String out;
  for (int i = 0; i < types.getCount(); i++)
  {
    if (out.length())
      out.aprintf(2, " ");
    int val = (int)getNormalizedValue(i);
    out.aprintf(64, "\n%s = %s (%d)", (char *)types.getTypeName(i), types.getValueName(i, val), val);
  }

  return out;
}

/*********************************
 *
 * class VariantTableSrc
 *
 *********************************/
// generate all variants from avalible types
void VariantTableSrc::generateFromTypes(const TypeTable &type_list, const IntervalList &i_list, ShHardwareOptions *opt,
  bool enable_empty)
{
  types = type_list;
  intervals = i_list;
  types.setIntervalList(&intervals);

  clear_and_shrink(variants);
  if (!types.getCount())
  {
    if (enable_empty)
      variants.push_back(VariantSrc(types));
    return;
  }

  // check assume_static exist variables
  DataBlock *block = shc::getAssumedVarsBlock();
  if (block)
    for (int n = 0; n < block->paramCount(); n++)
    {
      const char *varname = block->getParamName(n);
      shc::registerAssumedVar(varname, VarMap::getVarId(varname) != -1);
    }


  // prepare assumes
  SmallTab<ValueType, TmpmemAlloc> assume_val;
  clear_and_resize(assume_val, types.getCount());
  mem_set_ff(assume_val);

  if (block)
    for (int i = 0; i < assume_val.size(); i++)
    {
      const VariantType &type = types.getType(i);
      const Interval *interval = nullptr;

      switch (type.type)
      {
        case VARTYPE_MODE: break;

        case VARTYPE_INTERVAL:
        case VARTYPE_GL_OVERRIDE_INTERVAL:
        {
          const IntervalList *iList = types.getIntervals();
          interval = iList ? iList->getInterval(type.extType) : nullptr;
        }
        break;

        case VARTYPE_GLOBAL_INTERVAL: interval = ShaderGlobal::getIntervalList().getInterval(type.extType); break;

        default: G_ASSERT(0);
      }

      if (interval)
      {
        const char *varname = interval->getNameStr();
        real value;

        for (int n = 0; n < block->paramCount(); n++)
          if (strcmp(varname, block->getParamName(n)) == 0)
          {
            switch (block->getParamType(n))
            {
              case DataBlock::TYPE_INT: value = (real)block->getInt(n); break;
              case DataBlock::TYPE_REAL: value = block->getReal(n); break;
              default: sh_debug(SHLOG_ERROR, "Assume variables: type in \"%s\" is neither \"real\" nor \"int\"", varname); continue;
            }

            assume_val[i] = interval->normalizeValue(value);
          }
      }
    }

  // fill variant table
  variants.reserve(512);
  VariantSrc::defCtorTypeTable = &types;
  VariantSrc result;
  //    debug(intervals.getStringInfo());
  //    debug(types.getStringInfo());
  processVariant(types.getCount() - 1, result, &assume_val[types.getCount() - 1]);
  VariantSrc::defCtorTypeTable = NULL;
  variants.shrink_to_fit();
}


// process flag recurse
void VariantTableSrc::processVariant(int index, VariantSrc &result, ValueType *assume_val)
{
  const VariantType &type = types.getType(index);
  ShaderCompilerStat::totalVariants += type.valRange.getMax() - type.valRange.getMin() + 1;

  for (ValueType r = type.valRange.getMin(); r <= type.valRange.getMax(); r++)
    if (*assume_val >= 0 && *assume_val != r)
      ShaderCompilerStat::droppedVariants.dueToAssumeVar++;
    else
    {
      result.setNormalizedValue(index, r);
      if (index <= 0)
        variants.push_back(VariantSrc(result));
      else
        processVariant(index - 1, result, assume_val - 1);
    }
}


// fill shader variant table
void VariantTableSrc::fillVariantTable(VariantTable &tab) const
{
  tab.types = types;
  tab.intervals = intervals;
  tab.variants.resize(variants.size());
  for (int i = 0; i < variants.size(); i++)
    variants[i].fillVariant(tab.variants[i]);
}
} // namespace ShaderVariant
