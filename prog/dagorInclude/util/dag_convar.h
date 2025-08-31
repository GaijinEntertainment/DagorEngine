//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdio.h>
#include <debug/dag_assert.h>
#include <string.h>
#include <util/dag_globDef.h> // min/max

enum ConVarFlags
{
  CVF_DEFAULT = 0,
  // This convar won't be added to linked list (i.e. can't be discovered or changed)
  CVF_UNREGISTERED = 1 << 0
};

enum class ConVarType
{
  CVT_BOOL,
  CVT_INT,
  CVT_FLOAT
};

class ConVarBase
{
protected:
  ConVarBase(const char *name, const char *help_tip, int flags = CVF_DEFAULT);
  ~ConVarBase();

public:
  bool isFlagSet(int f) const { return (flags & f) != 0; }

  const char *getName() const { return name; }
  const char *getHelpTip() const { return helpTip; }
  ConVarBase *getNextVar() const { return nextVar; }

  virtual void revert() = 0; // set default value
  virtual void describeSelf(char *buf, size_t buf_size) const = 0;
  virtual void describeValue(char *buf, size_t buf_size) const = 0;
  virtual void parseString(const char *input) = 0;
  virtual bool imguiWidget(const char *label_override = nullptr) = 0;

  virtual ConVarType getType() const = 0;
  virtual float getBaseValue() const = 0;

private:
  const char *name;
  const char *helpTip;
  ConVarBase *nextVar = nullptr;
  int flags;
};

template <typename T, bool hasRange>
class ConVarT : public ConVarBase
{
public:
  ConVarT(const char *name, T def_val, const char *help_tip = nullptr, int flags = CVF_DEFAULT) :
    ConVarBase(name, help_tip, flags), value(def_val), prevValue(def_val), defValue(def_val)
  {}

  void revert() override { value = defValue; }
  void describeSelf(char *buf, size_t buf_size) const override
  {
    if (const char *tip = getHelpTip())
      snprintf(buf, buf_size, "%s - %s", getName(), tip);
    else
      strncpy(buf, getName(), buf_size);
  }
  void describeValue(char *buf, size_t buf_size) const override;
  void parseString(const char *input) override;
  bool imguiWidget(const char *label_override) override;
  ConVarType getType() const override;
  float getBaseValue() const override { return static_cast<float>(value); }

  virtual void set(T val) { value = val; }
  T get() const { return value; }
  T getDefault() const { return defValue; }
  operator T() const { return get(); }
  T &operator=(const T &val)
  {
    set(val);
    return value;
  }

  bool pullValueChange()
  {
    bool changed = value != prevValue;
    prevValue = value;
    return changed;
  }

protected:
  T value;
  T prevValue;
  T defValue;
};

template <typename T>
class ConVarT<T, true> : public ConVarT<T, false> // partial specialization for range case
{
public:
  using ConVarT<T, false>::operator=;

  ConVarT(const char *name, T def_val, T min_value, T max_value, const char *help_tip = nullptr, int flags = CVF_DEFAULT) :
    ConVarT<T, false>(name, def_val, help_tip, flags), minValue(min_value), maxValue(max_value)
  {
    G_ASSERT(this->defValue >= minValue);
    G_ASSERT(this->defValue <= maxValue);
  }

  void describeValue(char *buf, size_t buf_size) const override;
  void parseString(const char *input) override;
  bool imguiWidget(const char *label_override) override;
  ConVarType getType() const override;
  float getBaseValue() const override { return static_cast<float>(this->value); }

  void set(T val) override { this->value = min(max(val, minValue), maxValue); }
  T getMin() const { return minValue; }
  T getMax() const { return maxValue; }

  void setMinMax(T min, T max)
  {
    minValue = min;
    maxValue = max;
    set(this->value);
  }

private:
  T minValue, maxValue;
};

template <>
void ConVarT<int, true>::describeValue(char *buf, size_t buf_size) const;
template <>
void ConVarT<int, false>::describeValue(char *buf, size_t buf_size) const;
template <>
void ConVarT<float, true>::describeValue(char *buf, size_t buf_size) const;
template <>
void ConVarT<float, false>::describeValue(char *buf, size_t buf_size) const;
template <>
void ConVarT<bool, false>::describeValue(char *buf, size_t buf_size) const;
template <>
void ConVarT<int, true>::parseString(const char *buf);
template <>
void ConVarT<int, false>::parseString(const char *buf);
template <>
void ConVarT<float, true>::parseString(const char *buf);
template <>
void ConVarT<float, false>::parseString(const char *buf);
template <>
void ConVarT<bool, false>::parseString(const char *buf);
template <>
bool ConVarT<int, true>::imguiWidget(const char *label_override);
template <>
bool ConVarT<int, false>::imguiWidget(const char *label_override);
template <>
bool ConVarT<float, true>::imguiWidget(const char *label_override);
template <>
bool ConVarT<float, false>::imguiWidget(const char *label_override);
template <>
bool ConVarT<bool, false>::imguiWidget(const char *label_override);

template <>
inline ConVarType ConVarT<int, true>::getType() const
{
  return ConVarType::CVT_INT;
}
template <>
inline ConVarType ConVarT<int, false>::getType() const
{
  return ConVarType::CVT_INT;
}
template <>
inline ConVarType ConVarT<float, true>::getType() const
{
  return ConVarType::CVT_FLOAT;
}
template <>
inline ConVarType ConVarT<float, false>::getType() const
{
  return ConVarType::CVT_FLOAT;
}
template <>
inline ConVarType ConVarT<bool, false>::getType() const
{
  return ConVarType::CVT_BOOL;
}

extern template class ConVarT<int, false>;
extern template class ConVarT<int, true>;
extern template class ConVarT<float, false>;
extern template class ConVarT<float, true>;
extern template class ConVarT<bool, false>;

class ConVarIterator
{
public:
  ConVarIterator();
  ConVarBase *nextVar();

private:
  ConVarBase *next = nullptr;
};

typedef ConVarT<bool, false> ConVarB;
typedef ConVarT<float, true> ConVarF;
typedef ConVarT<int, true> ConVarI;

#define CONSOLE_BOOL_VAL(domain, name, defVal, ...) ConVarT<bool, false> name(domain "." #name, defVal, ##__VA_ARGS__)
#define CONSOLE_INT_VAL(domain, name, defVal, minVal, maxVal, ...) \
  ConVarT<int, true> name(domain "." #name, defVal, minVal, maxVal, ##__VA_ARGS__)
#define CONSOLE_FLOAT_VAL_MINMAX(domain, name, defVal, minVal, maxVal, ...) \
  ConVarT<float, true> name(domain "." #name, defVal, minVal, maxVal, ##__VA_ARGS__)
#define CONSOLE_FLOAT_VAL(domain, name, defVal, ...) ConVarT<float, false> name(domain "." #name, defVal, ##__VA_ARGS__)
