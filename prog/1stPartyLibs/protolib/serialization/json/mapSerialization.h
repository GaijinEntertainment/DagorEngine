#ifndef __GAIJIN_PROTOLIB_SERIALIZATION_JSON_MAPSERIALIZATION_H__
#define __GAIJIN_PROTOLIB_SERIALIZATION_JSON_MAPSERIALIZATION_H__


#include "jsonSerializator.h"
#include <protolib/serialization/containersUtil.h>

namespace proto
{
  namespace io
  {
    template <typename T>
    inline void writeMap(
      const proto::JsonWriter & ser,
      const char * name,
      const T & map)
    {
      if (map.size())
      {
        util::Value2StrWrapper<typename T::key_type> keyWrapper;
        Json::Value & value = ser.getJson()[name];
        for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
        {
          const char * key = keyWrapper.valueToStr(it->first);
          writeValueToJson(ser, value[key], it->second);
        }
      }
    }

    template <typename T>
    inline void readMap(
      const proto::JsonReader & ser,
      const char * name,
      T & map)
    {
      map.clear();
      const Json::Value & jsonMap = ser.getJson()[name];
      typename T::key_type key;
      typename T::mapped_type value;
      if (jsonMap.isObject())
      {
        util::Value2StrWrapper<typename T::key_type> keyWrapper;
        for (Json::Value::const_iterator it = jsonMap.begin(); it != jsonMap.end(); ++it)
        {
          const Json::Value & item = *it;
          if (keyWrapper.strToValue(it.memberName(), key))
          {
            if (JsonValueReader<typename T::mapped_type>::read(item, value))
            {
              map[key] = value;
            }
          }
        }
      }
    }


    template <bool hasExtKey>
    struct MapEntryLoaderJson
    {
      template <typename T, typename K, typename F>
      static void load(const proto::JsonReader & ser,
                       T & map,
                       const K & key,
                       const Json::Value & json,
                       const F & fn)
      {
        typename T::mapped_type & item = map[key];
        fn(proto::JsonReader(ser, json, hasExtKey), item);
      }
    };


    template <>
    struct MapEntryLoaderJson<true>
    {
      template <typename T, typename K, typename F>
      static void load(const proto::JsonReader & ser,
                       T & map,
                       const K & key,
                       const Json::Value & json,
                       const F & fn)
      {
        typename T::mapped_type & item = map[key];
        item.setExtKey(key);
        fn(proto::JsonReader(ser, json, true), item);
      }
    };


    template <typename T>
    inline void writeMapJson(
        const proto::JsonWriter & ser,
        const char * name,
        const T & map,
        void (*fn)(const proto::JsonWriter & ser, const typename T::mapped_type &))
    {
      if (map.size())
      {
        unsigned counter = 0;
        Json::Value & value = ser.getJson()[name];
        for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
        {
          util::Value2StrWrapper<typename T::key_type> keyWrapper;
          const char * key = keyWrapper.valueToStr(it->first);

          fn(proto::JsonWriter(ser, value[key], proto::HasExtKey<typename T::mapped_type>::value), it->second);
        }
      }
    }


    template <typename T>
    inline void readMapJson(const proto::JsonReader & ser,
        const char * name,
        T & map,
        void (*fn)(const proto::JsonReader & ser, typename T::mapped_type &))
    {
      map.clear();
      const Json::Value & jsonMap = ser.getJson()[name];
      typename T::key_type key;
      if (jsonMap.isObject())
      {
        util::Value2StrWrapper<typename T::key_type> keyWrapper;
        for (Json::Value::const_iterator it = jsonMap.begin(); it != jsonMap.end(); ++it)
        {
          const Json::Value & item = *it;
          if (keyWrapper.strToValue(it.memberName(), key))
          {
            MapEntryLoaderJson<proto::HasExtKey<typename T::mapped_type>::value>::load(
                ser, map, key, item, fn);
          }
        }
      }
    }


    template <typename T>
    inline void writeMapJsonVersioned(
        const proto::JsonWriter & ser,
        const char * name,
        const T & map,
        void (*fn)(const proto::JsonWriter & ser, const typename T::mapped_type &))
    {
      writeMapJson(ser, name, map, fn);
    }

    template <typename T>
    inline void readMapJsonVersioned(const proto::JsonReader & ser,
        const char * name,
        T & map,
        void (*fn)(const proto::JsonReader & ser, typename T::mapped_type &))
    {
      readMapJson(ser, name, map, fn);
    }
  }
}


#endif /* __GAIJIN_PROTOLIB_SERIALIZATION_JSON_MAPSERIALIZATION_H__ */
