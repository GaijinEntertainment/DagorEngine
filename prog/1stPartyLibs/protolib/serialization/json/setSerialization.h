#pragma once
#ifndef __GAIJIN_PROTOLIB_SERIALIZATION_JSON_SETSERIALIZATION_H__
#define __GAIJIN_PROTOLIB_SERIALIZATION_JSON_SETSERIALIZATION_H__

#include "jsonSerializator.h"
#include <protolib/serialization/containersUtil.h>

namespace proto
{
  namespace io
  {
    template <typename T>
    inline void writeSetJson(const proto::JsonWriter & ser,
      const char * name,
      const T & set,
      void (*fn)(const proto::JsonWriter & ser, const typename T::value_type &))
    {
      if (!set.size())
      {
        return;
      }

      unsigned counter = 0;
      Json::Value & value = ser.getJson()[name];
      for (typename T::const_iterator it = set.begin(); it != set.end(); ++it)
      {
        fn(proto::JsonWriter(ser, value[counter++]), *it);
      }
    }

    template <typename T>
    inline void readSetJson(const proto::JsonReader & ser,
      const char * name,
      T & set,
      void (*fn)(const proto::JsonReader & ser, typename T::value_type &))
    {
      set.clear();
      const Json::Value & json = ser.getJson()[name];
      if (json.isArray())
      {
        for (unsigned i = 0; i < json.size(); ++i)
        {
          typename T::value_type v;
          fn(proto::JsonReader(ser, json[i]), v);
          set.insert(std::move(v));
        }
      }
    }

    template <typename T>
    inline void writeSet(
      const proto::JsonWriter & ser,
      const char * name,
      const T & set)
    {
      if (!set.size())
      {
        return;
      }

      unsigned counter = 0;
      Json::Value & value = ser.getJson()[name];
      for (typename T::const_iterator it = set.begin(); it != set.end(); ++it)
      {
        writeValueToJson(ser, value[counter++], *it);
      }
    }

    template <typename T>
    inline void readSet(
      const proto::JsonReader & ser,
      const char * name,
      T & set)
    {
      set.clear();
      const Json::Value & value = ser.getJson()[name];
      if (value.isArray())
      {
        for (unsigned i = 0; i < value.size(); ++i)
        {
          typename T::value_type v;
          JsonValueReader<typename T::value_type>::read(value[i], v);
          set.insert(std::move(v));
        }
      }
    }
  }
}



#endif /* __GAIJIN_PROTOLIB_SERIALIZATION_JSON_SETSERIALIZATION_H__ */
