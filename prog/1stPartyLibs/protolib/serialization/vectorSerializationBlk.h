#pragma once
#ifndef __GAIJIN_PROTOIO_VECTORSERIALIZATIONBLK__
#define __GAIJIN_PROTOIO_VECTORSERIALIZATIONBLK__

#include "blkSerialization.h"

namespace proto
{
  namespace io
  {
    template <typename T, typename V>
    inline void writeVectorBlk(const proto::BlkSerializator & ser,
      const char * name,
      const T & vector,
      void (V::*fn)(const proto::BlkSerializator & ser) const)
    {
      if (!ser.checkSerialize(vector))
      {
        return;
      }

      DataBlock * scopeBlk = ser.blk.addNewBlock(name);
      G_ASSERT(scopeBlk);
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        DataBlock * blk = scopeBlk->addNewBlock("item");
        G_ASSERT(blk);
        ((*it).*fn)(proto::BlkSerializator(ser, *blk));
      }
    }

    template <typename T, typename V>
    inline void readVectorBlk(const proto::BlkSerializator & ser,
      const char * name,
      T & vector,
      void (V::*fn)(const proto::BlkSerializator & ser))
    {
      DataBlock * scopeBlk = ser.blk.getBlockByName(name);
      if (!scopeBlk)
      {
        return;
      }

      int count = scopeBlk->blockCount();
      vector.resize(count);
      for (int i = 0; i < count; ++i)
      {
        DataBlock * blk = scopeBlk->getBlock(i);
        G_ASSERT(blk);
        (vector[i].*fn)(proto::BlkSerializator(ser, *blk));
      }
    }

    template <typename T>
    inline void writeVector(
      const proto::BlkSerializator & ser,
      const char * name,
      const T & vector)
    {
      if (!ser.checkSerialize(vector))
      {
        return;
      }

      DataBlock * blk = ser.blk.addNewBlock(name);
      G_ASSERT(blk);
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        BlkValueWrapper<typename T::value_type>::addValue(*blk, "item", *it);
      }
    }

    template <typename T>
    inline void readVector(
      const proto::BlkSerializator & ser,
      const char * name,
      T & vector)
    {
      DataBlock * blk = ser.blk.getBlockByName(name);
      if (blk)
      {
        vector.clear();

        for (int i = 0; i < blk->paramCount(); ++i)
        {
          typename T::value_type value;
          BlkValueWrapper<typename T::value_type>::getValue(*blk, i, value);
          vector.insert(vector.end(), value);
        }
      }
    }
  }
}

#endif // __GAIJIN_PROTOIO_VECTORSERIALIZATIONBLK__
