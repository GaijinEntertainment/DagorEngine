#pragma once
#ifndef __GAIJIN_PROTOIO_MAPSERIALIZATIONBLK__
#define __GAIJIN_PROTOIO_MAPSERIALIZATIONBLK__

#include "blkSerialization.h"
#include "containersUtil.h"

namespace proto
{
  namespace io
  {
    template <typename T>
    inline void writeMap(
      const proto::BlkSerializator & ser,
      const char * name,
      const T & map)
    {
      if (!ser.checkSerialize(map))
      {
        return;
      }

      util::Value2StrWrapper<typename T::key_type> keyWrapper;
      DataBlock * blk = ser.blk.addNewBlock(name);
      G_ASSERT(blk);
      for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        const char * key = keyWrapper.valueToStr(it->first);
        BlkValueWrapper<typename T::mapped_type>::addValue(*blk, key, it->second);
      }
    }

    template <typename T>
    inline void readMap(
      const proto::BlkSerializator & ser,
      const char * name,
      T & map)
    {
      DataBlock * blk = ser.blk.getBlockByName(name);
      if (blk)
      {
        util::Value2StrWrapper<typename T::key_type> keyWrapper;

        map.clear();

        typename T::key_type key;
        for (int i = 0; i < blk->paramCount(); ++i)
        {
          const char * keyStr = blk->getParamName(i);
          G_ASSERT(keyStr != NULL);
          if (keyWrapper.strToValue(keyStr, key))
          {
            typename T::value_type value;
            BlkValueWrapper<typename T::mapped_type>::getValue(*blk, i, map[key]);
          }
        }
      }
    }

    template <typename T, typename V>
    inline void writeMapBlkVersioned(
      const proto::BlkSerializator & ser,
      const char * name,
      const T & map,
      void (V::*fn)(const proto::BlkSerializator & ser) const)
    {
      if (!ser.checkSerialize(map))
      {
        return;
      }

      const bool saveList = ser.checkSerialize(map.getStructureVersion());
      util::Value2StrWrapper<typename T::key_type> keyWrapper;
      if (saveList || !map.empty())
      {
        DataBlock * scopeBlk = ser.blk.addNewBlock(name);
        G_ASSERT(scopeBlk);
        for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
        {
          const typename T::mapped_type & item = it->second;
          const bool needSave = ser.checkSerialize(item.getVersion());
          if (!needSave)
          {
            if (saveList)
            {
              const char * key = keyWrapper.valueToStr(it->first);
              scopeBlk->setBool(key, true);
            }
            continue;
          }

          const char * key = keyWrapper.valueToStr(it->first);
          DataBlock * blk = scopeBlk->addNewBlock(key);
          G_ASSERT(blk);

          proto::BlkSerializator subSer(
            ser,
            *blk,
            proto::HasExtKey<typename T::mapped_type>::value);

          (item.*fn)(subSer);
        }

        if (saveList && !scopeBlk->paramCount())
        {
          scopeBlk->setBool("all", false);
        }
      }
    }


    template <bool hasExtKey>
    struct MapEntryLoaderBlk
    {
      template <typename T, typename V, typename K>
      static void load(const proto::BlkSerializator & ser,
                       T & map,
                       const K & key,
                       DataBlock & blk,
                       void (V::*fn)(const proto::BlkSerializator & ser))
      {
        V & item = map[key];
        (item.*fn)(proto::BlkSerializator(ser, blk, hasExtKey));
      }
    };


    template <>
    struct MapEntryLoaderBlk<true>
    {
      template <typename T, typename V, typename K>
      static void load(const proto::BlkSerializator & ser,
                       T & map,
                       const K & key,
                       DataBlock & blk,
                       void (V::*fn)(const proto::BlkSerializator & ser))
      {
        V & item = map[key];
        item.setExtKey(key);
        (item.*fn)(proto::BlkSerializator(ser, blk, true));
      }
    };


    template <typename T, typename V>
    inline void readMapBlkVersioned(const proto::BlkSerializator & ser,
      const char * name,
      T & map,
      void (V::*fn)(const proto::BlkSerializator & ser))
    {
      DataBlock * scopeBlk = ser.blk.getBlockByName(name);
      if (!scopeBlk)
      {
        return;
      }

      util::Value2StrWrapper<typename T::key_type> keyWrapper;
      if (scopeBlk->paramCount())
      {
        for (typename T::iterator it = map.begin(); it != map.end();)
        {
          int itemNameId = scopeBlk->getNameId(keyWrapper.valueToStr(it->first));
          if (itemNameId == -1 ||
            (!scopeBlk->getBlockByName(itemNameId) &&
              !scopeBlk->getBoolByNameId(itemNameId, false)))
          {
            it = util::map_erase_and_get_next_iterator(map, it);
          }
          else
          {
            ++it;
          }
        }
      }

      int count = scopeBlk->blockCount();
      typename T::key_type key;
      for (int i = 0; i < count; ++i)
      {
        DataBlock * blk = scopeBlk->getBlock(i);
        G_ASSERT(blk);
        const char * keyStr = blk->getBlockName();
        G_ASSERT(keyStr != NULL);
        if (keyWrapper.strToValue(keyStr, key))
        {
          MapEntryLoaderBlk<proto::HasExtKey<typename T::mapped_type>::value>::load(
              ser, map, key, *blk, fn);
        }
      }
    }


    template <typename T, typename V>
    inline void writeMapBlk(
      const proto::BlkSerializator & ser,
      const char * name,
      const T & map,
      void (V::*fn)(const proto::BlkSerializator & ser) const)
    {
      if (!ser.checkSerialize(map))
      {
        return;
      }

      util::Value2StrWrapper<typename T::key_type> keyWrapper;
      DataBlock * scopeBlk = ser.blk.addNewBlock(name);
      G_ASSERT(scopeBlk);
      for (typename T::const_iterator it = map.begin(); it != map.end(); ++it)
      {
        const typename T::mapped_type & item = it->second;
        const char * key = keyWrapper.valueToStr(it->first);
        DataBlock * blk = scopeBlk->addNewBlock(key);
        G_ASSERT(blk);
        proto::BlkSerializator subSer(
          ser,
          *blk,
          proto::HasExtKey<typename T::mapped_type>::value);

        (item.*fn)(subSer);
      }
    }


    template <typename T, typename V>
    inline void readMapBlk(const proto::BlkSerializator & ser,
      const char * name,
      T & map,
      void (V::*fn)(const proto::BlkSerializator & ser))
    {
      DataBlock * scopeBlk = ser.blk.getBlockByName(name);
      if (!scopeBlk)
      {
        return;
      }

      map.clear();

      util::Value2StrWrapper<typename T::key_type> keyWrapper;
      for (typename T::iterator it = map.begin(); it != map.end();)
      {
        if (!scopeBlk->getBlockByName(keyWrapper.valueToStr(it->first)))
        {
          it = map.erase(it);
        }
        else
        {
          ++it;
        }
      }

      int count = scopeBlk->blockCount();
      typename T::key_type key;
      for (int i = 0; i < count; ++i)
      {
        DataBlock * blk = scopeBlk->getBlock(i);
        G_ASSERT(blk);
        const char * keyStr = blk->getBlockName();
        G_ASSERT(keyStr != NULL);
        if (keyWrapper.strToValue(keyStr, key))
        {
          MapEntryLoaderBlk<proto::HasExtKey<typename T::mapped_type>::value>::load(
              ser, map, key, *blk, fn);
        }
      }
    }
  }
}

#endif // __GAIJIN_PROTOIO_MAPSERIALIZATIONBLK__
