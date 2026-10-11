//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/string.h>
#include <dag/dag_vector.h>
#include <charconv>

struct VPValue
{
  enum class Widget
  {
    Separator,
    Checkbox,
    FloatValue,
    IntValue,
    FloatSlider,
    FloatSliderLog,
    IntSlider,
    IntSliderLog,
    Combo,
  };

  using GetCallback = eastl::function<double()>;
  using SetCallback = eastl::function<void(double)>;

  template <typename CBG, typename CBS>
  VPValue(const char *name, const char *tooltip, Widget widget, double minValue, double maxValue, CBG get_callback, CBS set_callback,
    const char *comma_separated_items = "") :
    name(name),
    tooltip(tooltip),
    minValue(minValue),
    maxValue(maxValue),
    widget(widget),
    getCallback(get_callback),
    setCallback(set_callback),
    selected(false)
  {
    for (eastl::string rest(comma_separated_items); !rest.empty();)
    {
      const auto comma = rest.find(',');
      items.push_back(rest.substr(0, comma));
      if (comma == eastl::string::npos)
        break;
      rest.erase(0, comma + 1);
    }
  }

  void setValue(double v) { setCallback(v); }

  double getValue() const { return getCallback(); }

  eastl::string name;
  eastl::string tooltip;
  double minValue;
  double maxValue;
  Widget widget;
  GetCallback getCallback;
  SetCallback setCallback;
  dag::Vector<eastl::string> items;
  bool selected;
};

void add_video_production_value(VPValue *value);

#define VP_SEPARATOR() add_video_production_value(new VPValue("", "", VPValue::Widget::Separator, 0, 0, nullptr, nullptr))
#define VP_CHECKBOX(name, tooltip, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::Checkbox, 0, 0, get, set))
#define VP_FLOAT_SLIDER(name, tooltip, min, max, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::FloatSlider, min, max, get, set))
#define VP_FLOAT_SLIDER_LOG(name, tooltip, min, max, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::FloatSliderLog, min, max, get, set))
#define VP_INT_SLIDER(name, tooltip, min, max, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::IntSlider, min, max, get, set))
#define VP_INT_SLIDER_LOG(name, tooltip, min, max, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::IntSliderLog, min, max, get, set))
#define VP_INT_VALUE(name, tooltip, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::IntValue, 0, 0, get, set))
#define VP_COMBO(name, tooltip, comma_separated_items, get, set) \
  add_video_production_value(new VPValue(name, tooltip, VPValue::Widget::Combo, 0, 0, get, set, comma_separated_items))
