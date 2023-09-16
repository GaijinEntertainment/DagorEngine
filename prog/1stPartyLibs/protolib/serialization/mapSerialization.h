#pragma once
#ifndef __GAIJIN_PROTOIO_MAPSERIALIZATION__
#define __GAIJIN_PROTOIO_MAPSERIALIZATION__

#include "binaryWrapper.h"
#include "containersUtil.h"


namespace proto
{
  namespace io
  {
    template <typename T, bool versioned = true>
    class MapItemMarker
    {
      MapItemMarker(const MapItemMarker&);
      MapItemMarker & operator=(const MapItemMarker&);

    public:
      typedef T TMap;
      typedef typename TMap::mapped_type TValue;
      typedef typename TMap::iterator TMapIterator;

      static const int ADD_VER_CONST = DEFAULT_VERSION > 0 ? 0 : 1 - 2 * DEFAULT_VERSION;

      MapItemMarker(TMap & map)
        : map_(map)
      {
      }

      ~MapItemMarker()
      {
        unmarkAll();
      }

      void unmarkAll()
      {
        for (TMapIterator it = map_.begin(); it != map_.end(); ++it)
        {
          checkAndUnmark(it->second);
        }
      }

      bool mark(TValue & item)
      {
        if (item.getVersion() < DEFAULT_VERSION)
        {
          return false;
        }

        item.touch(-item.getVersion() - ADD_VER_CONST);
        return true;
      }

      bool checkAndUnmark(TValue & item)
      {
        version ver = item.getVersion();
        if (ver >= DEFAULT_VERSION)
        {
          return false;
        }

        item.touch(-ver - ADD_VER_CONST);
        return true;
      }

    private:
      TMap & map_;
    };


    // dummy class need for compilation only
    template <typename T>
    class MapItemMarker<T, false>
    {
    public:
      typedef T TMap;
      typedef typename TMap::mapped_type TValue;

      MapItemMarker(TMap &)
      {
        G_ASSERT(false);
      }

      void unmarkAll()
      {
        G_ASSERT(false);
      }

      bool mark(TValue &)
      {
        G_ASSERT(false);
        return false;
      }

      bool checkAndUnmark(TValue &)
      {
        G_ASSERT(false);
        return true;
      }
    };


    struct MapFormat
    {
      static const int NUM_KEY = 1;
      static const int NUM_VALUE = 2;
      static const int NUM_LIST_ALL = 3;
    };

    template <typename T, bool versioned>
    struct MapSerialization
    {
      typedef T TMap;
      typedef typename TMap::key_type TKey;
      typedef typename TMap::mapped_type TValue;
      typedef typename TMap::iterator TMapIterator;

      typedef bool (TValue::*TSerializationFunctor)(InputCodeStream &, StreamTag);

      static bool readMap(InputCodeStream & stream,
                          StreamTag tag,
                          TMap & map,
                          TSerializationFunctor serializeItem);

    private:
      static bool readMapAllListFallback(InputCodeStream & stream,
                                         TMap & map,
                                         TSerializationFunctor serializeItem);
    };

    template <typename T, bool versioned>
    inline bool MapSerialization<T, versioned>::readMap(
        InputCodeStream & stream,
        StreamTag tag,
        T & map,
        TSerializationFunctor serializeItem)
    {
      if (tag.type == StreamTag::EMPTY)
      {
        map.clear();
        return true;
      }

      PROTO_VALIDATE(tag.type == StreamTag::BLOCK_BEGIN);
      PROTO_VALIDATE(stream.readTag(tag));

      if (versioned)
      {
        if (tag.number == MapFormat::NUM_LIST_ALL)
        {
          return readMapAllListFallback(stream, map, serializeItem);
        }
      }
      else
      {
        map.clear();
      }

      TKey key;
      while (true)
      {
        if (tag.isBlockEnded())
        {
          return true;
        }

        PROTO_VALIDATE(tag.number == MapFormat::NUM_KEY);

        PROTO_VALIDATE(BinaryWrapper<typename T::key_type>::readValue(stream, tag, key));

        PROTO_VALIDATE(stream.readTag(tag));
        PROTO_VALIDATE(tag.number == MapFormat::NUM_VALUE);

        TValue & value = map[key];
        PROTO_VALIDATE((value.*serializeItem)(stream, tag));
        set_ext_key(value, key);
        PROTO_VALIDATE(stream.readTag(tag));
      }

      return true;
    }


    template <typename T, bool versioned>
    inline bool MapSerialization<T, versioned>::readMapAllListFallback(
        InputCodeStream & stream,
        T & map,
        TSerializationFunctor serializeItem)
    {
      StreamTag tag;
      MapItemMarker<T, versioned> marker(map);
      TKey key;

      PROTO_VALIDATE(stream.readTag(tag));
      while (true)
      {
        if (tag.isBlockEnded())
        {
          break;
        }

        PROTO_VALIDATE(tag.number == MapFormat::NUM_KEY);

        PROTO_VALIDATE(BinaryWrapper<typename T::key_type>::readValue(stream, tag, key));
        PROTO_VALIDATE(stream.readTag(tag));

        if (tag.number == MapFormat::NUM_KEY)
        {
          TMapIterator it = map.find(key);
          PROTO_VALIDATE(it != map.end());
          PROTO_VALIDATE(marker.mark(it->second));
          continue;
        }

        PROTO_VALIDATE(tag.number == MapFormat::NUM_VALUE)

        TValue & value = map[key];
        PROTO_VALIDATE((value.*serializeItem)(stream, tag));
        set_ext_key(value, key);
        PROTO_VALIDATE(marker.mark(value));
        PROTO_VALIDATE(stream.readTag(tag));
      }

      for (TMapIterator it = map.begin(); it != map.end();)
      {
        if (marker.checkAndUnmark(it->second))
        {
          ++it;
        }
        else
        {
          it = util::map_erase_and_get_next_iterator(map, it);
        }
      }

      return true;
    }


    template <typename T, typename F>
    inline void writeMap(OutputCodeStream & stream,
                  int number,
                  const T & map,
                  const F & fn)
    {
      if (!stream.checkVersion(map))
      {
        return;
      }

      if (map.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        BinaryWrapper<typename T::key_type>::writeValue(stream, MapFormat::NUM_KEY, it->first);
        (it->second.*fn)(stream, BinarySaveOptions(MapFormat::NUM_VALUE, false));
      }

      stream.blockEnd();
    }

    template <typename T, typename F>
    inline void writeMapVersioned(OutputCodeStream & stream,
                          int number,
                          const T & map,
                          const F & fn)
    {
      if (!stream.checkVersion(map))
      {
        return;
      }

      if (map.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      const bool writeList = stream.checkVersion(map.getStructureVersion());
      if (writeList)
      {
        stream.writeBool(MapFormat::NUM_LIST_ALL, true);
      }

      for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        bool needSave = stream.checkVersion(it->second.getVersion());
        if (needSave || writeList)
        {
          BinaryWrapper<typename T::key_type>::writeValue(stream, MapFormat::NUM_KEY, it->first);
        }

        if (needSave)
        {
          (it->second.*fn)(stream, BinarySaveOptions(MapFormat::NUM_VALUE, false));
        }
      }

      stream.blockEnd();
    }


    template <typename T, typename V>
    inline void writeIndexedMap(OutputCodeStream & stream,
                  int number,
                  const T & map,
                  void (V::*fn)(OutputCodeStream &, int) const)
    {
      if (!stream.checkVersion(map))
      {
        return;
      }

      if (map.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      for (int i = 0; i < map.size(); ++i)
      {
        stream.writeString(MapFormat::NUM_KEY, map[i].first.c_str(), map[i].first.size());
        (map[i].second.*fn)(stream, MapFormat::NUM_VALUE);
      }

      stream.blockEnd();
    }


    template <typename T, typename V>
    inline void writeIndexedMapVersioned(OutputCodeStream & stream,
                          int number,
                          const T & map,
                          void (V::*fn)(OutputCodeStream &, int) const)
    {
      if (!stream.checkVersion(map))
      {
        return;
      }

      if (map.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      const bool writeList = stream.checkVersion(map.getStructureVersion());
      if (writeList)
      {
        stream.writeBool(MapFormat::NUM_LIST_ALL, true);
      }

      for (int i = 0; i < map.size(); ++i)
      {
        const V & value = map[i].second;
        bool needSave = stream.checkVersion(value.getVersion());
        if (needSave || writeList)
        {
          stream.writeString(MapFormat::NUM_KEY, to_cstr(map[i].first), str_size(map[i].first));
        }

        if (needSave)
        {
          (value.*fn)(stream, MapFormat::NUM_VALUE);
        }
      }

      stream.blockEnd();
    }

    template <typename T>
    inline void writeMap(OutputCodeStream & stream, int number, const T & map)
    {
      if (map.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        BinaryWrapper<typename T::key_type>::writeValue(stream, MapFormat::NUM_KEY, it->first);
        BinaryWrapper<typename T::mapped_type>::writeValue(stream, MapFormat::NUM_VALUE, it->second);
      }

      stream.blockEnd();
    }

    template <typename T>
    inline bool readMap(InputCodeStream & stream, StreamTag tag, T & map)
    {
      map.clear();

      if (tag.type == StreamTag::EMPTY)
      {
        return true;
      }

      PROTO_VALIDATE(tag.type == StreamTag::BLOCK_BEGIN);

      typename T::key_type key;
      while (true)
      {
        PROTO_VALIDATE(stream.readTag(tag));
        if (tag.isBlockEnded())
        {
          return true;
        }

        PROTO_VALIDATE(tag.number == MapFormat::NUM_KEY);
        PROTO_VALIDATE(BinaryWrapper<typename T::key_type>::readValue(stream, tag, key));

        PROTO_VALIDATE(stream.readTag(tag));
        PROTO_VALIDATE(tag.number == MapFormat::NUM_VALUE);
        PROTO_VALIDATE(BinaryWrapper<typename T::mapped_type>::readValue(stream, tag, map[key]));
      }

      return true;
    }


    template <typename T, typename F, bool VERSIONED>
    struct ReadMapHelper
    {
      static bool read(proto::io::InputCodeStream & stream, T & map, F & fn, int expectedTag)
      {
        proto::io::StreamTag tag;
        PROTO_VALIDATE(stream.readTag(tag));
        PROTO_VALIDATE(tag.number == expectedTag)
        PROTO_VALIDATE((proto::io::MapSerialization<T, VERSIONED>::readMap(
            stream, tag, map, fn)));
        return true;
      }
    };


    template <typename T, typename F>
    bool readMap(proto::io::InputCodeStream & stream, T & map, F fn, int expectedTag)
    {
      return ReadMapHelper<T, F, false>::read(stream, map, fn, expectedTag);
    }

    template <typename T, typename F>
    bool readMapVersioned(proto::io::InputCodeStream & stream, T & map, F fn, int expectedTag)
    {
      return ReadMapHelper<T, F, true>::read(stream, map, fn, expectedTag);
    }
  }
}

#endif // __GAIJIN_PROTOIO_MAPSERIALIZATION__
