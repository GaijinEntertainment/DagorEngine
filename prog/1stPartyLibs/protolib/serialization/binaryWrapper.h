#pragma once
#ifndef __GAIJIN_PROTOIO_BINARYWRAPPER__
#define __GAIJIN_PROTOIO_BINARYWRAPPER__

#include "inputCodeStream.h"
#include "outputCodeStream.h"

namespace proto
{
  namespace io
  {
    template <typename T>
    struct BinaryWrapper
    {
      // default realization for enum, other types auto detected by templates specialization
      static void writeValue(OutputCodeStream & stream, TNumber num, T value)
      {
        stream.writeEnum(num, value);
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, T & value)
      {
        return stream.readEnum(tag, value);
      }
    };


    template <>
    struct BinaryWrapper<float>
    {
      static void writeValue(OutputCodeStream & stream, TNumber num, float value)
      {
        stream.writeFloat(num, value);
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, float & value)
      {
        return stream.readFloat(tag, value);
      }
    };


    template <>
    struct BinaryWrapper<double>
    {
      static void writeValue(OutputCodeStream & stream, TNumber num, double value)
      {
        stream.writeDouble(num, value);
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, double & value)
      {
        return stream.readDouble(tag, value);
      }
    };


    template <>
    struct BinaryWrapper<bool>
    {
      static void writeValue(OutputCodeStream & stream, TNumber num, bool value)
      {
        stream.writeBool(num, value);
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, bool & value)
      {
        return stream.readBool(tag, value);
      }
    };


    template <>
    struct BinaryWrapper<proto::string>
    {
      static void writeValue(OutputCodeStream & stream, TNumber num, const proto::string & value)
      {
        stream.writeString(num, str_cstr(value), str_size(value));
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, proto::string & value)
      {
        return readString(stream, tag, value);
      }
    };


    template <typename T>
    struct BinaryWrapperInt
    {
      static void writeValue(OutputCodeStream & stream, TNumber num, T value)
      {
        stream.writeVarInt(num, value);
      }


      static bool readValue(InputCodeStream & stream, StreamTag tag, T & value)
      {
        return stream.readVarInt(tag, value);
      }
    };

    template <> struct BinaryWrapper<proto::int32>  : BinaryWrapperInt<proto::int32>  {};
    template <> struct BinaryWrapper<proto::uint32> : BinaryWrapperInt<proto::uint32> {};
    template <> struct BinaryWrapper<proto::int64>  : BinaryWrapperInt<proto::int64>  {};
    template <> struct BinaryWrapper<proto::uint64> : BinaryWrapperInt<proto::uint64> {};
  }
}

#endif // __GAIJIN_PROTOIO_BINARYWRAPPER__
