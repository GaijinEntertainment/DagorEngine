// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

/************************************************************************
  shader variants for compilation
/************************************************************************/

#include "shaderVariant.h"
#include "shTargetContext.h"
#include <shaders/dag_shaderCommon.h>
#include "shLog.h"


namespace ShaderVariant
{
/*********************************
 *
 * class VariantSrc
 *
 *********************************/
class VariantSrc : public SearchInfo, public IVariantInfoProvider
{
protected:
  friend struct Variant;
  // DISABLE_EQUALITY(VariantSrc);
  inline bool operator==(const VariantSrc &v) const
  {
    G_ASSERT(false);
    return false;
  };
  inline bool operator!=(const VariantSrc &v) const
  {
    G_ASSERT(false);
    return false;
  };

public:
  int codeId;

  inline VariantSrc(const TypeTable &t) : SearchInfo(t, true), codeId(-1) {}
  inline VariantSrc() : SearchInfo(*defCtorTypeTable, true), codeId(-1) {}

  ~VariantSrc() override {}

  inline void fillVariant(Variant &v) const
  {
    v.codeId = codeId;
    v.combinedValues = combinedValues;

    // v.values = values;
  }

  inline VariantSrc(const VariantSrc &other) : SearchInfo(other.types, false) { operator=(other); }

  inline VariantSrc &operator=(const VariantSrc &other)
  {
    codeId = other.codeId;
    // values = other.values;
    combinedValues = other.combinedValues;
    getCount();
    return *this;
  }

  inline ValueType getValue(BaseType init_type, ExtType init_exttype, bool can_fail) const
  {
    int index = types.findType(init_type, init_exttype);
    if (index < 0)
      return can_fail ? -1 : 0;
    else
      return getNormalizedValue(index);
  }

  inline const IntervalList &getIntervalList() const
  {
    G_ASSERT(types.getIntervals());
    return *types.getIntervals();
  }

  // return string info about variant
  String getVarStringInfo() const override;

public:
  static TypeTable *defCtorTypeTable;
};

struct VariantInfo
{
  const VariantSrc &stat;
  const VariantSrc *dyn;
  const IntervalList &intervals;

  inline VariantInfo(const VariantSrc &st, const VariantSrc *dn) : stat(st), dyn(dn), intervals(st.getIntervalList()) {}

  inline ValueType getValue(BaseType init_type, ExtType init_exttype) const
  {
    ValueType v = stat.getValue(init_type, init_exttype, true);
    if (v < 0 && dyn)
      v = dyn->getValue(init_type, init_exttype, true);
    return v >= 0 ? v : -1;
  }
};


/*********************************
 *
 * class VariantTableSrc
 *
 *********************************/
class VariantTableSrc
{
public:
  explicit VariantTableSrc(shc::TargetContext &a_ctx);
  ~VariantTableSrc() {}

  NON_COPYABLE_TYPE(VariantTableSrc)

  // generate all variants from avalible types
  void generateFromTypes(const TypeTable &type_list, const IntervalList &i_list, bool enable_empty);

  // get types
  inline const TypeTable &getTypes() const { return types; };

  // get intervals
  inline const IntervalList &getIntervals() const { return intervals; };

  // get number of variants
  inline int getVarCount() const { return variants.size(); };

  // get variant by index
  VariantSrc *getVariant(int index) { return &variants[index]; }
  const VariantSrc *getVariant(int index) const { return &variants[index]; }

  // fill shader variant table
  void fillVariantTable(VariantTable &tab) const;

private:
  TypeTable types;
  IntervalList intervals;
  Tab<VariantSrc> variants;
  shc::TargetContext &ctx;

  // process flag recurse
  void processVariant(int index, VariantSrc &result, ValueType *assumed);
};
} // namespace ShaderVariant
DAG_DECLARE_RELOCATABLE(ShaderVariant::VariantSrc);
