#pragma once
#ifndef __GAIJIN_PROTOIO_SETSERIALIZATIONBLK__
#define __GAIJIN_PROTOIO_SETSERIALIZATIONBLK__

#include "vectorSerializationBlk.h"


// datablock set serialization is identical to that of vector


namespace proto
{
  namespace io
  {
    template <typename T, typename V>
    inline void writeSetBlk(const proto::BlkSerializator & ser,
      const char * name,
      const T & set,
      void (V::*fn)(const proto::BlkSerializator & ser) const)
    {
      writeVectorBlk(ser, name, set, fn);
    }

    template <typename T, typename V>
    inline void readSetBlk(const proto::BlkSerializator & ser,
      const char * name,
      T & set,
      void (V::*fn)(const proto::BlkSerializator & ser))
    {
      readVectorBlk(ser, name, set, fn);
    }

    template <typename T>
    inline void writeSet(
      const proto::BlkSerializator & ser,
      const char * name,
      const T & set)
    {
      writeVector(ser, name, set);
    }

    template <typename T>
    inline void readSet(
      const proto::BlkSerializator & ser,
      const char * name,
      T & set)
    {
      readVector(ser, name, set);
    }
  }
}


#endif // __GAIJIN_PROTOIO_SETSERIALIZATIONBLK__
