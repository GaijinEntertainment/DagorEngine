// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/functional.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include "levelProfilerFilter.h"
#include "levelProfilerWidgets.h"

namespace levelprofiler
{

// The filter of one table column. The table owns it, asks it about each row item and rebinds it after each item rebuild.
class ILpColumnFilter
{
public:
  virtual ~ILpColumnFilter() = default;

  virtual bool isActive() const = 0;
  virtual void reset() = 0;
  // A filter of a column with views is active when the filter of any value is; these tell the values of the shown view from
  // the others.
  virtual bool isShownViewActive() const { return isActive(); }
  bool isOtherViewActive() const { return getActiveOtherView() >= 0; }
  virtual void resetShownView() { reset(); }
  virtual void resetOtherViews() {}
  // A view, not the shown one, with an active filter, or -1.
  virtual int getActiveOtherView() const { return -1; }
  virtual bool pass(const void *item) const = 0;
  // Draws the popup content. Returns true when the filter changed.
  virtual bool drawPopup() = 0;

  // The table calls beginRebind, then rebindItem for each item, then endRebind. A filter takes its data bounds from the items and
  // keeps what the user set; a check filter drops the exclusions of values that the items no longer have.
  virtual void beginRebind() {}
  virtual void rebindItem(const void * /* item */) {}
  virtual void endRebind() {}

  // A non-null hint gives the column a search field in the filter row; the field sends its text to setSearchText.
  virtual const char *getSearchHint() const { return nullptr; }
  virtual void setSearchText(const char * /* text */) {}
};

// A range of one number of a row. The getter returns false when the row has no value: such a row gives no bound, and passes
// unless the user clears the check box of the missing text (the popup has it when the text is not empty).
template <typename Row, typename T>
class LpRangeColumnFilter final : public ILpColumnFilter
{
public:
  using Getter = eastl::function<bool(const Row &, T &)>;

  LpRangeColumnFilter(Getter getter, const ProfilerString &label_text, const char *format_min, const char *format_max,
    float speed_value, const char *missing_text = nullptr) :
    getValue(eastl::move(getter)),
    label(label_text),
    formatMin(format_min),
    formatMax(format_max),
    speed(speed_value),
    missingText(missing_text ? missing_text : "")
  {}

  bool isActive() const override { return filter.isActive() || !passMissing; }
  void reset() override
  {
    filter.reset();
    passMissing = true;
  }

  bool pass(const void *item) const override
  {
    T value;
    if (!getValue(*static_cast<const Row *>(item), value))
      return passMissing;
    return filter.pass(value);
  }

  bool drawPopup() override
  {
    LpRangeFilterController<T> controller(&filter);
    bool cleared = false;
    bool changed = drawRangeFilter<T>("Range", label.c_str(), controller, formatMin, formatMax, speed, 0, &cleared);
    if (cleared)
      passMissing = true;
    if (!missingText.empty())
    {
      changed |= ImGui::Checkbox(missingText.c_str(), &passMissing);
      ImGui::SetItemTooltip("Rows without a value: the cell shows %s", missingText.c_str());
    }
    return changed;
  }

  void beginRebind() override { hasValues = false; }

  void scale(T factor) { filter.scale(factor); }

  void rebindItem(const void *item) override
  {
    T value;
    if (!getValue(*static_cast<const Row *>(item), value))
      return;
    minValue = hasValues ? eastl::min(minValue, value) : value;
    maxValue = hasValues ? eastl::max(maxValue, value) : value;
    hasValues = true;
  }

  void endRebind() override
  {
    if (hasValues)
      filter.rebind(minValue, maxValue);
    else
      filter.rebind(T(0), T(0));
  }

private:
  Getter getValue;
  LpRangeFilter<T> filter;
  ProfilerString label;
  const char *formatMin = nullptr;
  const char *formatMax = nullptr;
  float speed = 1.0f;
  ProfilerString missingText;
  bool passMissing = true;

  bool hasValues = false;
  T minValue = T(0), maxValue = T(0);
};

// A set of excluded values of one key of a row. The popup lists the keys found in the items, in Compare order, under the label
// when it is not empty.
template <typename Row, typename Key, typename Compare = eastl::less<Key>>
class LpCheckColumnFilter final : public ILpColumnFilter
{
public:
  using Getter = eastl::function<Key(const Row &)>;
  using LabelFunc = typename LpMultiSelectWidget<Key>::LabelFunc;

  LpCheckColumnFilter(Getter getter, LabelFunc label_func, const char *label_text = nullptr) :
    getKey(eastl::move(getter)), labelFunc(label_func), label(label_text ? label_text : "")
  {}

  bool isActive() const override { return filter.isActive(); }
  void reset() override { filter.reset(); }
  bool pass(const void *item) const override { return filter.pass(getKey(*static_cast<const Row *>(item))); }

  bool drawPopup() override
  {
    if (!label.empty())
      ImGui::TextUnformatted(label.c_str());
    bool changed = false;
    LpMultiSelectWidget<Key> widget(labelFunc);
    widget.Draw(values, filter, changed);
    return changed;
  }

  void beginRebind() override { values.clear(); }
  void rebindItem(const void *item) override { values.push_back(getKey(*static_cast<const Row *>(item))); }

  void endRebind() override
  {
    eastl::sort(values.begin(), values.end(), Compare());
    values.erase(eastl::unique(values.begin(), values.end()), values.end());
    filter.retainExcludedIn(values);
  }

private:
  Getter getKey;
  LabelFunc labelFunc;
  ProfilerString label;
  CheckFilter<Key> filter;
  eastl::vector<Key> values;
};

// A set of excluded values of a list of keys of a row. A row passes when the filter passes each of its keys; a row with an empty
// list has the none key. The popup lists the keys found in the items in Compare order, then the none key.
template <typename Row, typename Key, typename Compare = eastl::less<Key>>
class LpMultiCheckColumnFilter final : public ILpColumnFilter
{
public:
  using KeyVisitor = eastl::function<bool(const Key &)>;
  // Calls the visitor for each key of the row; it can stop when the visitor returns false.
  using ForEachKey = eastl::function<void(const Row &, const KeyVisitor &)>;
  using LabelFunc = typename LpMultiSelectWidget<Key>::LabelFunc;

  LpMultiCheckColumnFilter(ForEachKey for_each_key, Key none_key, LabelFunc label_func) :
    forEachKey(eastl::move(for_each_key)), noneKey(eastl::move(none_key)), labelFunc(label_func)
  {}

  bool isActive() const override { return filter.isActive(); }
  void reset() override { filter.reset(); }

  bool pass(const void *item) const override
  {
    // One struct, so the capture is two pointers and fits the inline storage of eastl::function: no heap block per row.
    struct
    {
      bool hasKeys = false;
      bool allPass = true;
    } walk;
    forEachKey(*static_cast<const Row *>(item), [this, &walk](const Key &key) {
      walk.hasKeys = true;
      walk.allPass = walk.allPass && filter.pass(key);
      return walk.allPass;
    });
    return walk.hasKeys ? walk.allPass : filter.pass(noneKey);
  }

  bool drawPopup() override
  {
    bool changed = false;
    LpMultiSelectWidget<Key> widget(labelFunc);
    widget.Draw(values, filter, changed);
    return changed;
  }

  void beginRebind() override
  {
    values.clear();
    hasNone = false;
  }

  void rebindItem(const void *item) override
  {
    bool hasKeys = false;
    forEachKey(*static_cast<const Row *>(item), [this, &hasKeys](const Key &key) {
      hasKeys = true;
      values.push_back(key);
      return true;
    });
    hasNone |= !hasKeys;
  }

  void endRebind() override
  {
    eastl::sort(values.begin(), values.end(), Compare());
    values.erase(eastl::unique(values.begin(), values.end()), values.end());
    if (hasNone)
      values.push_back(noneKey);
    filter.retainExcludedIn(values);
  }

private:
  ForEachKey forEachKey;
  Key noneKey;
  LabelFunc labelFunc;
  CheckFilter<Key> filter;
  eastl::vector<Key> values;
  bool hasNone = false;
};

// A text search and include/exclude masks on one string of a row.
template <typename Row>
class LpTextColumnFilter final : public ILpColumnFilter
{
public:
  using Getter = eastl::function<const ProfilerString &(const Row &)>;

  LpTextColumnFilter(Getter getter, const char *search_hint) : getText(eastl::move(getter)), hint(search_hint) {}

  bool isActive() const override { return filter.isActive(); }
  void reset() override { filter.reset(); }
  bool pass(const void *item) const override { return filter.pass(getText(*static_cast<const Row *>(item))); }
  bool drawPopup() override { return filter.drawInline(); }

  const char *getSearchHint() const override { return hint; }
  void setSearchText(const char *text) override { filter.setSearchText(text); }

private:
  Getter getText;
  LpIncludeExcludeFilter filter;
  const char *hint = nullptr;
};

} // namespace levelprofiler
