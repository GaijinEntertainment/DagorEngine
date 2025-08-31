// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shCompiler.h"
#include "shaderVariantSrc.h"
#include "shShaderContext.h"
#include <generic/dag_tabUtils.h>
#include <generic/dag_smallTab.h>
#include <debug/dag_debug.h>
#include "globVar.h"
#include "intervals.h"
#include "shCompiler.h"
#include <shaders/dag_shaders.h>
#include "sh_stat.h"
#include "varMap.h"
#include "commonUtils.h"

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
VariantTableSrc::VariantTableSrc(shc::TargetContext &a_ctx) : types{a_ctx}, variants(tmpmem), ctx{a_ctx} {}

void VariantTableSrc::generateFromTypes(const TypeTable &type_list, const IntervalList &i_list, bool enable_empty)
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

  // prepare assumes
  SmallTab<ValueType, TmpmemAlloc> assume_val;
  clear_and_resize(assume_val, types.getCount());
  mem_set_ff(assume_val);

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

      case VARTYPE_GLOBAL_INTERVAL: interval = ctx.globVars().getIntervalList().getInterval(type.extType); break;

      default: G_ASSERT(0);
    }

    if (interval)
    {
      const char *varname = get_interval_name(*interval, ctx);
      // @NOTE: Only global assumes are used here, as in the previous implementation
      if (auto valueMaybe = ctx.globAssumes().getAssumedVal(varname, true))
      {
        assume_val[i] = interval->normalizeValue(*valueMaybe);
      }
    }
  }

  // fill variant table
  variants.reserve(512);
  VariantSrc::defCtorTypeTable = &types;
  VariantSrc result;
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
