#pragma once
#ifndef __GAIJIN_PROTOIO_VECTORSERIALIZATION__
#define __GAIJIN_PROTOIO_VECTORSERIALIZATION__

#include "binaryWrapper.h"

namespace proto
{
  namespace io
  {
    struct VectorFormat
    {
      static const int NUM_SIZE = 1;
      static const int NUM_VALUE = 2;
      static const int NUM_INDEX = 3;
    };

    template <typename T>
    inline void writeVector(
        OutputCodeStream & stream,
        int number,
        const T & vector)
    {
      if (!stream.checkVersion(vector))
      {
        return;
      }

      if (vector.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        BinaryWrapper<typename T::value_type>::writeValue(stream, VectorFormat::NUM_VALUE, *it);
      }

      stream.blockEnd();
    }

    template <typename T>
    inline bool readVector(
        InputCodeStream & stream,
        StreamTag tag,
        T & vector)
    {
      vector.clear();

      if (tag.type == StreamTag::EMPTY)
      {
        return true;
      }

      PROTO_VALIDATE(tag.type == StreamTag::BLOCK_BEGIN);
      PROTO_VALIDATE(stream.readTag(tag));

      typename T::value_type value;
      while (!tag.isBlockEnded())
      {
        PROTO_VALIDATE(tag.number == VectorFormat::NUM_VALUE);
        BinaryWrapper<typename T::value_type>::readValue(stream, tag, value);
        PROTO_VALIDATE(stream.readTag(tag));
        vector.insert(vector.end(), value);
      }

      return true;
    }


    template <typename T, typename F>
    inline void writeVector(OutputCodeStream & stream,
      int number,
      const T & vector,
      const F & fn)
    {
      if (!stream.checkVersion(vector))
      {
        return;
      }

      if (vector.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      for (typename T::const_iterator it = vector.begin(); it != vector.end(); ++it)
      {
        ((*it).*fn)(stream, BinarySaveOptions(VectorFormat::NUM_VALUE));
      }

      stream.blockEnd();
    }

    template <typename T, typename V>
    inline bool readVector(
      InputCodeStream & stream,
      StreamTag tag,
      T & vector,
      bool (V::*serializeItem)(InputCodeStream &, StreamTag))
    {
      vector.clear();
      if (tag.type == StreamTag::EMPTY)
      {
        return true;
      }

      PROTO_VALIDATE(tag.type == StreamTag::BLOCK_BEGIN);
      PROTO_VALIDATE(stream.readTag(tag));

      while (!tag.isBlockEnded())
      {
        PROTO_VALIDATE(tag.number == VectorFormat::NUM_VALUE);

        V value; // TODO optimization
        PROTO_VALIDATE((value.*serializeItem)(stream, tag));
        vector.push_back(value);
        PROTO_VALIDATE(stream.readTag(tag));
      }

      return true;
    }


    template <typename T, typename F>
    inline void writeVectorVersioned(OutputCodeStream & stream,
      int number,
      const T & vector,
      const F & fn)
    {
      if (!stream.checkVersion(vector))
      {
        return;
      }

      if (vector.empty())
      {
        stream.writeTag(proto::io::StreamTag::EMPTY, number);
        return;
      }

      stream.blockBegin(number);
      if (stream.checkVersion(vector.getStructureVersion()))
      {
        stream.writeVarInt(VectorFormat::NUM_SIZE, (int)vector.size());
      }

      int lastIndex = -1;
      for (int i = 0; i < vector.size(); ++i)
      {
        const typename T::value_type & value = vector[i];
        if (stream.checkVersion(value))
        {
          if (lastIndex + 1 != i)
          {
            stream.writeVarInt(VectorFormat::NUM_INDEX, i);
          }

          (value.*fn)(stream, BinarySaveOptions(VectorFormat::NUM_VALUE));
          lastIndex = i;
        }
      }

      stream.blockEnd();
    }

    template <typename T, typename V>
    inline bool readVectorVersioned(
      InputCodeStream & stream,
      StreamTag tag,
      T & vector,
      bool (V::*serializeItem)(InputCodeStream &, StreamTag))
    {
      if (tag.type == StreamTag::EMPTY)
      {
        vector.clear();
        return true;
      }

      PROTO_VALIDATE(tag.type == StreamTag::BLOCK_BEGIN);
      PROTO_VALIDATE(stream.readTag(tag));

      if (tag.isBlockEnded())
        return true;

      int size = 0;
      if (tag.number == VectorFormat::NUM_SIZE)
      {
        stream.readVarInt(tag, size);
        PROTO_VALIDATE(size > 0);
        vector.resize(size);
        PROTO_VALIDATE(stream.readTag(tag));
      }

      int index = -1;
      while (!tag.isBlockEnded())
      {
        if (tag.number == VectorFormat::NUM_INDEX)
        {
          stream.readVarInt(tag, index);
          PROTO_VALIDATE(index > 0);
          PROTO_VALIDATE(stream.readTag(tag));
        }
        else
        {
          ++index;
        }

        PROTO_VALIDATE(tag.number == VectorFormat::NUM_VALUE);

        if (index >= vector.size())
        {
          PROTO_VALIDATE(!size || index < size);
          vector.resize(index + 1);
        }

        PROTO_VALIDATE((vector[index].*serializeItem)(stream, tag));
        PROTO_VALIDATE(stream.readTag(tag));
      }

      return true;
    }
  }
}

#endif // __GAIJIN_PROTOIO_VECTORSERIALIZATION__
