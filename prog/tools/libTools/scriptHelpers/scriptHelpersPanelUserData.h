// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "debug/dag_assert.h"

namespace ScriptHelpers
{

class TunedElement;

class ScriptHelpersPanelUserData
{
public:
  enum class DataType
  {
    Unknown = 0,
    Array = 1,
    ArrayItem = 2,
  };

  static void *make(TunedElement *tuned_element, DataType data_type)
  {
    uintptr_t u = reinterpret_cast<uintptr_t>(tuned_element);
    G_ASSERT((u & 3) == 0);
    G_ASSERT(data_type == DataType::Array || data_type == DataType::ArrayItem);
    return reinterpret_cast<void *>(u | static_cast<unsigned>(data_type));
  }

  static TunedElement *retrieve(void *user_data, DataType &data_type)
  {
    uintptr_t u = reinterpret_cast<uintptr_t>(user_data);
    data_type = static_cast<DataType>(u & 3);
    if (data_type != DataType::Array && data_type != DataType::ArrayItem)
    {
      data_type = DataType::Unknown;
      return nullptr;
    }

    return reinterpret_cast<TunedElement *>(u & ~3);
  }
};

} // namespace ScriptHelpers
