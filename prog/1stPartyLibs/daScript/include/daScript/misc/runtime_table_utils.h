#pragma once
#include "daScript/misc/arraytype.h"
#include "daScript/simulate/aot_builtin.h"

namespace das
{
  class Context;
  struct LineInfoArg;

  template<typename T, typename CB>
  void for_each_table(const T & table, CB && callback, das::Context * context, das::LineInfoArg * at)
  {
    das::Sequence keysSequence;
    das::Sequence valueSequence;
    das::builtin_table_keys(keysSequence, table, sizeof(typename T::KeyType), context, at);
    das::builtin_table_values(valueSequence, table, sizeof(typename T::ValueType), context, at);

    typename T::KeyType keyData;
    typename T::ValueType * valueData;
    bool hasData = false;
    do
    {
      hasData = das::builtin_iterator_iterate(keysSequence, &keyData, context);
      hasData = das::builtin_iterator_iterate(valueSequence, &valueData, context) && hasData;
      if (hasData && !callback(keyData, *valueData))
      {
        break;
      }
    } while(hasData);
    das::builtin_iterator_close(keysSequence, &keyData, context);
    das::builtin_iterator_close(valueSequence, &valueData, context);
  }
}
