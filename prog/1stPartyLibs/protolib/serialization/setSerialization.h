#pragma once
#ifndef __GAIJIN_PROTOIO_SETSERIALIZATION__
#define __GAIJIN_PROTOIO_SETSERIALIZATION__

#include "vectorSerialization.h"


// protobuffer set serialization is identical to that of vector


namespace proto
{
  namespace io
  {
    template <typename T>
    inline void writeSet(
        OutputCodeStream & stream,
        int number,
        const T & set)
    {
      writeVector(stream, number, set);
    }


    template <typename T>
    inline bool readSet(
        InputCodeStream & stream,
        StreamTag tag,
        T & set)
    {
      return readVector(stream, tag, set);
    }


    template <typename T, typename F>
    inline void writeSet(OutputCodeStream & stream,
      int number,
      const T & set,
      const F & fn)
    {
      writeVector(stream, number, set, fn);
    }


    template <typename T, typename V>
    inline bool readSet(
      InputCodeStream & stream,
      StreamTag tag,
      T & set,
      bool (V::*serializeItem)(InputCodeStream &, StreamTag))
    {
      return readVector(stream, tag, set, serializeItem);
    }


    template <typename T, typename F>
    inline void writeSetVersioned(OutputCodeStream & stream,
      int number,
      const T & set,
      const F & fn)
    {
      writeVectorVersioned(stream, number, set, fn);
    }

    template <typename T, typename V>
    inline bool readSetVersioned(
      InputCodeStream & stream,
      StreamTag tag,
      T & set,
      bool (V::*serializeItem)(InputCodeStream &, StreamTag))
    {
      return readVectorVersioned(stream, tag, set, serializeItem);
    }
  }
}

#endif // __GAIJIN_PROTOIO_SETSERIALIZATION__
