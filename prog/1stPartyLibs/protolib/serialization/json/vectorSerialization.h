#ifndef __GAIJIN_PROTOLIB_SERIALIZATION_JSON_VECTORSERIALIZATION_H__
#define __GAIJIN_PROTOLIB_SERIALIZATION_JSON_VECTORSERIALIZATION_H__

#include "jsonSerializator.h"
#include <protolib/serialization/containersUtil.h>

namespace proto
{
  namespace io
  {
    template <typename T>
    inline void writeVectorJson(const proto::JsonWriter & ser,
      const char * name,
      const T & vector,
      void (*fn)(const proto::JsonWriter & ser, const typename T::value_type &))
    {
      if (!vector.size())
      {
        return;
      }

      unsigned counter = 0;
      Json::Value & value = ser.getJson()[name];
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        fn(proto::JsonWriter(ser, value[counter++]), *it);
      }
    }

    template <typename T>
    inline void readVectorJson(const proto::JsonReader & ser,
      const char * name,
      T & vector,
      void (*fn)(const proto::JsonReader & ser, typename T::value_type &))
    {
      vector.clear();
      const Json::Value & json = ser.getJson()[name];
      if (json.isArray())
      {
        vector.resize(json.size());
        for (unsigned i = 0; i < json.size(); ++i)
        {
          fn(proto::JsonReader(ser, json[i]), vector[i]);
        }
      }
    }

    template <typename T>
    inline void writeVector(
      const proto::JsonWriter & ser,
      const char * name,
      const T & vector)
    {
      if (vector.empty())
      {
        return;
      }

      unsigned counter = 0;
      Json::Value & value = ser.getJson()[name];
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        writeValueToJson(ser, value[counter++], *it);
      }
    }

    template <typename T>
    inline void readVector(
      const proto::JsonReader & ser,
      const char * name,
      T & vector)
    {
      vector.clear();
      const Json::Value & value = ser.getJson()[name];
      if (value.isArray())
      {
        vector.resize(value.size());
        for (unsigned i = 0; i < vector.size(); ++i)
        {
          JsonValueReader<typename T::value_type>::read(value[i], vector[i]);
        }
      }

    }
  }
}



#endif /* __GAIJIN_PROTOLIB_SERIALIZATION_JSON_VECTORSERIALIZATION_H__ */
