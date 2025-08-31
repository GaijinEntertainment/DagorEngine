// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <EASTL/array.h>
#include <generic/dag_span.h>
#include "render/animchar_additional_data_types.hlsli"


namespace animchar_additional_data
{

using AdditionalMetadata = eastl::array<uint16_t, 8>;

// return index in additional_data for this DataType, this function invalidate results previous request_space calls
template <int type>
int request_space(ecs::Point4List &data, int size)
{
  G_STATIC_ASSERT(type >= 0 && type < 8);
  if (data.size() < 2)
  {
    data.resize(2);
    data[0] = data[1] = Point4::ZERO;
  }
  int oldSize = data.size();
  const AdditionalMetadata count = bitwise_cast<AdditionalMetadata>(data[oldSize - 1]);
  const AdditionalMetadata offset = bitwise_cast<AdditionalMetadata>(data[oldSize - 2]);
  int oldDataSize = count[type];
  // offset in additional_data stores offset for shader, in shader we read additional_data inverted, need invert on cpu
  int oldDataOffset = oldSize - 2 - (int)(offset[type]) - oldDataSize;
  G_ASSERT_RETURN(oldDataOffset >= 0, 0);
  int sizeDelta = size - oldDataSize;
  if (sizeDelta != 0)
  {
    int newDataSize = oldSize + sizeDelta;
    int copySize = oldSize - oldDataOffset - oldDataSize;
    int i = 0;
    if (sizeDelta > 0) // increase vector
    {
      data.resize(newDataSize);
      for (auto src = data.begin() + oldSize - 1, dst = data.end() - 1; i < copySize; i++, dst--, src--)
        *dst = *src;
    }
    else
    {
      for (auto src = data.begin() + oldDataOffset + oldDataSize, dst = src + sizeDelta; i < copySize; i++, dst++, src++)
        *dst = *src;
      data.resize(newDataSize);
    }
    Point4 &newCountF = data[newDataSize - 1];
    Point4 &newOffsetF = data[newDataSize - 2];
    AdditionalMetadata newCount = bitwise_cast<AdditionalMetadata>(newCountF);
    AdditionalMetadata newOffset = bitwise_cast<AdditionalMetadata>(newOffsetF);
    newCount[type] += sizeDelta;
    for (int i = type - 1; i >= 0; i--)
      newOffset[i] += sizeDelta;
    newCountF = bitwise_cast<Point4>(newCount);
    newOffsetF = bitwise_cast<Point4>(newOffset);
  }
  return oldDataOffset;
}


// helper class to store additional data on its own, and provide view interface
class AnimcharAdditionalDataVec : public dag::Vector<Point4>
{};


// lightweight view of additional data, doesn't store anything directly
class AnimcharAdditionalDataView : public dag::ConstSpan<Point4>
{
private:
  static const Point4 NULL_METADATA_RAW[];
  static const AnimcharAdditionalDataView NULL_METADATA;
  static const Point4 ZERO_METADATA_RAW[];
  static const AnimcharAdditionalDataView ZERO_METADATA;

public:
  // we make it explicit to avoid accidental conversion from raw data, without metadata
  explicit AnimcharAdditionalDataView(const dag::ConstSpan<Point4> &s) : dag::ConstSpan<Point4>(s)
  {
    G_ASSERTF(s.size() >= 2, "AnimcharAdditionalDataView must have at least 2 elements for metadata! It has %d elements", s.size());
  }

  // implicit conversion is safe here
  AnimcharAdditionalDataView(const AnimcharAdditionalDataVec &data) :
    AnimcharAdditionalDataView(static_cast<const dag::ConstSpan<Point4>>(data))
  {}

  // null is when only metadata exists, no real additional data
  static AnimcharAdditionalDataView get_null() { return NULL_METADATA; }

  static AnimcharAdditionalDataView get_optional_data(const ecs::Point4List *additional_data)
  {
    return additional_data && !additional_data->empty()
             ? AnimcharAdditionalDataView(make_span_const<Point4>(additional_data->data(), additional_data->size()))
             : get_null();
  }
};


template <int type>
AnimcharAdditionalDataVec prepare_fixed_space(dag::ConstSpan<Point4> additional_data)
{
  G_STATIC_ASSERT(type >= 0 && type < 8);
  AnimcharAdditionalDataVec data;
  data.resize(additional_data.size() + 2);
  if (additional_data.empty())
  {
    data[0] = data[1] = Point4::ZERO;
    return data;
  }

  for (int i = 0; i < additional_data.size(); ++i)
    data[i] = additional_data[i];

  AdditionalMetadata count = bitwise_cast<AdditionalMetadata>(Point4::ZERO);
  AdditionalMetadata offset = bitwise_cast<AdditionalMetadata>(Point4::ZERO);
  count[type] += additional_data.size();
  for (int i = type - 1; i >= 0; i--)
    offset[i] += additional_data.size();
  data[data.size() - 1] = bitwise_cast<Point4>(count);
  data[data.size() - 2] = bitwise_cast<Point4>(offset);
  return data;
}


inline AnimcharAdditionalDataView get_null_data() { return AnimcharAdditionalDataView::get_null(); }

inline AnimcharAdditionalDataView get_optional_data(const ecs::Point4List *additional_data)
{
  return AnimcharAdditionalDataView::get_optional_data(additional_data);
}


}; // namespace animchar_additional_data