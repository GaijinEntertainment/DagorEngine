// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentTypes.h>
#include <EASTL/array.h>


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
    data[0] = data[1] = Point4(0, 0, 0, 0);
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
}; // namespace animchar_additional_data