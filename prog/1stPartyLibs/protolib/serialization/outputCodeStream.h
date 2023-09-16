#pragma once
#ifndef __GAIJIN_PROTOIO_OUTPUTCODESTREAM__
#define __GAIJIN_PROTOIO_OUTPUTCODESTREAM__

#include "codeStream.h"
#include "binaryStream.h"

#include "types.h"

#include <debug/dag_assert.h>

#include <climits>
#include <cstring>


namespace proto
{
  namespace io
  {
    class OutputCodeStream
    {
    public:
      typedef OutputBinaryStream::TSize TSize;
      static const int STRING_DICTIONARY_START_SIZE = 11;
      OutputCodeStream(version prevVersion_ = SAVE_ALL_VERSION,
        Buffer::TSize dictionarySize = STRING_DICTIONARY_START_SIZE)
        : prevVersion(prevVersion_)
        , dictionary(
            dictionarySize,
            StringRecordHash(&stream.getBuffer()),
            StringRecordEqual(&stream.getBuffer()))
      {
      }

      const OutputBinaryStream & getBinaryStream() const
      {
        return stream;
      }

      OutputBinaryStream & getBinaryStream()
      {
        return stream;
      }

      void reserve(TSize size)
      {
        stream.reserve(size);
      }

      MoveBuffer moveBuffer()
      {
        dictionary.clear();
        return stream.moveBuffer();
      }

      void writeTag(StreamTag::Type type, TNumber number);

      template <typename T>
      void writeVarInt(T value);

      template <typename T>
      void writeVarInt(TNumber number, T value);

      template <typename T>
      void writeEnum(TNumber number, const T & value);

      void writeString(TNumber number, const char * data, size_t size);

      void writeBool(TNumber number, bool value);
      void writeFloat(TNumber number, float value);
      void writeDouble(TNumber number, double value);

      void writeRAW(TNumber number, const void * ptr, TSize size);

      template <typename T>
      void writeRAW(TNumber number, const T & v);

      void blockBegin(TNumber number);
      void blockEnd();

      bool checkVersion(version version) const
      {
        return version > prevVersion;
      }

      template <typename T>
      bool checkVersion(const T & obj) const
      {
        return checkVersionInternal(&obj);
      }

    private:

      bool checkVersionInternal(const SerializableVersioned * obj) const
      {
        return checkVersion(obj->getVersion());
      }

      bool checkVersionInternal(const void * /*obj*/) const
      {
        // serialize all nonversioned objects
        return true;
      }

      TSize findInDictionary(const char * data, int size)
      {
        StringRecord record;
        record.data = data;
        record.size = -size;
        TDictionary::const_iterator it = dictionary.find(record);
        return it == dictionary.end() ? 0 : it->second;
      }

      void addToDictionary(TSize parsePos, TSize dataPos, int size)
      {
        StringRecord record;
        record.position = dataPos;
        record.size = size;
        dictionary.insert(proto::make_pair(record, parsePos));
      }

      OutputBinaryStream stream;

      struct StringRecord
      {
        union
        {
          const char * data;
          TSize position;
        };

        int size;
      };

      struct StringRecordEqual
      {
        bool operator()(const StringRecord & value1, const StringRecord & value2) const
        {
          if (value1.size < 0)
          {
            G_ASSERT(value2.size > 0);
          }
          else if (value2.size < 0)
          {
            return operator()(value2, value1);
          }
          else
          {
            return false;
          }

          if (-value1.size != value2.size)
          {
            return false;
          }

          return !memcmp(value1.data, buffer->begin() + value2.position, value2.size);
        }

        StringRecordEqual(const Buffer * buffer_)
          : buffer(buffer_)
        {

        }

        const Buffer * buffer;
      };

      struct StringRecordHash
      {
        size_t operator()(const StringRecord & value) const
        {
          if (value.size < 0)
          {
            return hash_range(value.data, value.data - value.size);
          }

          const char * data = buffer->begin() + value.position;
          return hash_range(data, data + value.size);
        }

        StringRecordHash(const Buffer * buffer_)
          : buffer(buffer_)
        {

        }

        const Buffer * buffer;

        static size_t hash_range(const char * first, const char * last)
        {
          const unsigned char * p = (const unsigned char *)first;
          const unsigned char * e = (const unsigned char *)last;
          size_t result = 2166136261U;
          while (p != e)
          {
            size_t c = *p++;
            result = (result * 16777619) ^ c;
          }
          return (size_t)result;
        }
      };

      typedef proto::UnorderedMap<
          StringRecord,
          TSize,
          StringRecordHash,
          StringRecordEqual>::TType TDictionary;

      TDictionary dictionary;
      version     prevVersion;
    };

    inline void OutputCodeStream::writeRAW(TNumber number, const void * ptr, TSize size)
    {
      if (size == 4)
      {
        writeTag(StreamTag::RAW32, number);
      }
      else
      {
        writeTag(StreamTag::VAR_LENGTH, number);
        writeVarInt(size);
      }

      stream.write(ptr, size);
    }

    template <typename T>
    inline void OutputCodeStream::writeRAW(TNumber number, const T & v)
    {
      writeRAW(number, &v, sizeof(v));
    }

    template <typename T>
    inline void OutputCodeStream::writeVarInt(T value)
    {
      uint64 unsignedValue(value);
      do
      {
        uint64 nextValue = unsignedValue >> 7;
        stream.put(nextValue ? (unsignedValue | 0x80) : unsignedValue );
        unsignedValue = nextValue;
      }
      while (unsignedValue);
    }

    template <typename T>
    inline void OutputCodeStream::writeVarInt(TNumber number, T value)
    {
      if (!value)
      {
        writeTag(StreamTag::EMPTY, number);
        return;
      }

      if (value >= INT_MIN  && value <= INT_MAX && (value <= -(1 << 21) || value >= (1 << 21)))
      {
        int32 uint32Value = value;
        writeTag(StreamTag::RAW32, number);
        stream.write(&uint32Value, 4);
        return;
      }

      bool minus = value < 0;
      writeTag(minus ? StreamTag::VAR_INT_MINUS : StreamTag::VAR_INT, number);
      writeVarInt(minus ?(0 - value) : value);
    }


    inline void OutputCodeStream::writeTag(StreamTag::Type type, TNumber number)
    {
      G_ASSERT(number > 0);
      char tag = type;

      int hiNumber = int(number) - StreamTag::SHORT_NUMBER_LIMIT;
      if (hiNumber < 0)
      {
        tag |= (number << StreamTag::TAG_BITS_COUNT);
        stream.put(tag);
      }
      else
      {
        stream.put(tag);
        writeVarInt(hiNumber);
      }
    }


    inline void OutputCodeStream::writeString(TNumber number, const char * data, size_t size)
    {
      const size_t MAX_DICTIONARY_STRING_LENGTH = 128;
      if (!size)
      {
        writeTag(StreamTag::EMPTY, number);
        return;
      }

      if (size > MAX_DICTIONARY_STRING_LENGTH)
      {
        writeTag(StreamTag::VAR_LENGTH, number);
        writeVarInt(size);
        stream.write(data, (Buffer::TSize)size);
        return;
      }

      Buffer::TSize pos = findInDictionary(data, (Buffer::TSize)size);
      if (pos > 0)
      {
        writeVarInt(number, pos);
      }
      else
      {
        writeTag(StreamTag::VAR_LENGTH, number);
        TSize parsePos = stream.tell();
        writeVarInt(size);
        TSize dataPos = stream.tell();
        stream.write(data, (Buffer::TSize)size);
        addToDictionary(parsePos, dataPos, (Buffer::TSize)size);
      }
    }


    inline void OutputCodeStream::writeBool(TNumber number, bool value)
    {
      writeTag(value ? StreamTag::EMPTY2 : StreamTag::EMPTY, number);
    }


    inline void OutputCodeStream::writeFloat(TNumber number, float value)
    {
      if (value == 0.0f)
      {
        writeTag(StreamTag::EMPTY, number);
      }
      else
      {
        writeTag(StreamTag::RAW32, number);
        stream.write(&value, 4);
      }
    }


    inline void OutputCodeStream::writeDouble(TNumber number, double value)
    {
      if (value == 0.0)
      {
        writeTag(StreamTag::EMPTY, number);
      }
      else
      {
        writeTag(StreamTag::VAR_LENGTH, number);
        writeVarInt(8);
        stream.write(&value, 8);
      }
    }


    template <typename T>
    inline void OutputCodeStream::writeEnum(TNumber number, const T & value)
    {
      G_ASSERT(validate_enum_value(value));
      writeVarInt(number, int64(value));
    }


    inline void OutputCodeStream::blockBegin(TNumber number)
    {
      writeTag(StreamTag::BLOCK_BEGIN, number);
    }


    inline void OutputCodeStream::blockEnd()
    {
      stream.put(StreamTag::TAG_BLOCK_END);
    }
  }
}

#endif // __GAIJIN_PROTOIO_OUTPUTCODESTREAM__
