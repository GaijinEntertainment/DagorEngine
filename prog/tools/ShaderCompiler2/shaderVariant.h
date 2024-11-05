// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "intervals.h"
#include "shaderSave.h"
#include <debug/dag_debug.h>
#include "varTypes.h"
#include <generic/dag_tab.h>
#include <util/dag_stdint.h>
#include <EASTL/array.h>

namespace ShaderVariant
{
struct VariantType
{
public:
  VariantType() = default;

  BaseType type;
  ExtType extType;
  ValueRange valRange{0, 0};
  BINDUMP_NON_SERIALIZABLE(const Interval *interval = nullptr);

  inline VariantType(BaseType init_type, ExtType init_exttype) : type(init_type), extType(init_exttype), valRange(0, 0), interval(NULL)
  {}

  inline bool isEqual(BaseType init_type, ExtType init_exttype) const { return type == init_type && extType == init_exttype; }
};

class TypeTable
{
public:
  inline int getCount() const { return types.size(); };
  inline void shrinkTo(int cnt) { return types.resize(cnt); }

  inline const VariantType &getType(int i) const { return types[i]; }

  void addType(BaseType init_type, ExtType init_exttype, bool unique);

  // find type from list. return type index or -1, if not found
  int findType(BaseType init_type, ExtType init_exttype) const;

  String getTypeName(int i) const;
  String getValueName(int type_index, int value_index) const;

  // return unified interval name (variable_name or MODE:<name>)
  const char *getIntervalName(int i) const;

  // dump to string info about types
  String getStringInfo() const;

  // remap value from some number to a inteval-index
  ValueType normalizeValue(int index, real val) const;

  void setIntervalList(IntervalList *ilist);
  inline const IntervalList *getIntervals() const { return iList; }

  void link(Tab<ExtType> &interval_link_table);

  void reset() { types.clear(); }

private:
  SerializableTab<VariantType> types;
  BINDUMP_NON_SERIALIZABLE(const IntervalList *iList = nullptr);

  // calculate type value range
  void updateTypeValRange(VariantType &t);
};

class PartialSearchInfo
{
  friend class SearchInfo;

public:
  inline PartialSearchInfo(IMemAlloc *m = tmpmem) : values(m) {}

  inline int getCount() const { return values.size(); }
  inline void clear() { clear_and_shrink(values); }

  // set unnormalized value
  inline void setValue(BaseType init_type, ExtType init_exttype, real val)
  {
    for (int i = 0; i < values.size(); i++)
    {
      ValInfo &vi = values[i];
      if (vi.type == init_type && vi.extType == init_exttype)
      {
        vi.val = val;
        return;
      }
    }

    ValInfo &vi = values.push_back();
    vi.type = init_type;
    vi.extType = init_exttype;
    vi.val = val;
  }

private:
  struct ValInfo
  {
    BaseType type;
    ExtType extType;
    real val;
  };

  Tab<ValInfo> values;
};

class CombinedValues
{
public:
  CombinedValues() { mValues.fill(0); }
  bool operator==(const CombinedValues &right) const { return mCount == right.mCount && right.mValues == mValues; }
  inline void set(uint32_t idx, ValueType value)
  {
    G_ASSERT(idx < mCount);
    mValues[idx] = value;
  }
  inline ValueType get(uint32_t idx) const
  {
    G_ASSERT(idx < mCount);
    return mValues[idx];
  }
  inline void setCount(uint32_t count)
  {
    G_ASSERT(count < MAX_VALUES);
    mCount = count;
    eastl::fill(mValues.begin() + mCount, mValues.end(), 0);
  }
  inline uint32_t getCount() const { return mCount; }

private:
  static constexpr uint32_t MAX_VALUES = 32;
  eastl::array<ValueType, MAX_VALUES> mValues;
  uint32_t mCount = 0;
};

class SearchInfo
{
protected:
  friend struct Variant;

  SearchInfo(const SearchInfo &from) : types(from.types) { G_ASSERT(false); }
  SearchInfo &operator=(const SearchInfo &from)
  {
    G_ASSERT(false);
    return *this;
  }

public:
  inline SearchInfo(const TypeTable &t, bool fill_with_defaults) : types(t)
  {
    combinedValues = {};
    combinedValues.setCount(types.getCount());

    if (fill_with_defaults)
    {
      for (int i = 0; i < types.getCount(); i++)
      {
        setNormalizedValue(i, types.getType(i).valRange.getMin());
      }
    }
  }

  inline ~SearchInfo() {}

  // fill from partial search info table (slow)
  inline void fillFromPartialSearchInfo(const PartialSearchInfo &psi)
  {
    combinedValues.setCount(psi.values.size());
    for (int i = 0; i < psi.values.size(); i++)
    {
      const PartialSearchInfo::ValInfo &vi = psi.values[i];
      int index = types.findType(vi.type, vi.extType);

      if (index >= 0 && index < getCount())
        setValue(index, vi.val);
    }
  }

  inline uint32_t getCount() const { return combinedValues.getCount(); }

  inline void setNormalizedValue(uint32_t index, ValueType val)
  {
    G_ASSERT(index < getCount());
    combinedValues.set(index, val);
  }

  inline uint32_t getNormalizedValue(uint32_t index) const
  {
    G_ASSERT(index < getCount());
    return combinedValues.get(index);
  }

  // set unnormalized value
  inline void setValue(uint32_t index, real val)
  {
    G_ASSERT(index < getCount());
    combinedValues.set(index, types.normalizeValue(index, val));
  }

  const TypeTable &getTypes() const { return types; }

protected:
  const TypeTable &types;
  CombinedValues combinedValues;
};

class VariantSrc;
struct Variant
{
protected:
  friend class VariantSrc;
  CombinedValues combinedValues;

public:
  int codeId;

  inline Variant() : codeId(-1) {}

  // check equality
  inline bool isEqual(const SearchInfo &other) const { return other.combinedValues == combinedValues; }
  inline uint32_t getValue(uint32_t index) const
  {
    G_ASSERT(index < getCount());
    return combinedValues.get(index);
  }

  inline uint32_t getCount() const { return combinedValues.getCount(); }
};

class VariantTable
{
protected:
  friend class VariantTableSrc;

public:
  VariantTable();

  // create search info values for this variants
  SearchInfo *createSearchInfo(bool fill_with_defaults);

  // find variant. return NULL, if not found
  Variant *findVariant(const SearchInfo &si);
  const Variant *findVariant(const SearchInfo &si) const { return const_cast<VariantTable *>(this)->findVariant(si); }

  // get variant by index
  Variant *getVariant(int index);
  const Variant *getVariant(int index) const { return const_cast<VariantTable *>(this)->getVariant(index); }

  inline int getVarCount() const { return variants.size(); }
  inline const TypeTable &getTypes() const { return types; }
  dag::ConstSpan<Variant> getVariants() const { return variants; }

  void linkIntervalList();
  void link(Tab<ExtType> &interval_link_table);

  void reset()
  {
    variants.clear();
    intervals.clear();
    types.reset();
  }

private:
  TypeTable types;
  IntervalList intervals;
  SerializableTab<Variant> variants;
  BINDUMP_NON_SERIALIZABLE(mutable int cachedIndex);
};
} // namespace ShaderVariant
