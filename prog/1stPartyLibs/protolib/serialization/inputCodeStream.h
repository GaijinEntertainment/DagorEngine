#pragma once
#ifndef __GAIJIN_PROTOIO_INPUTCODESTREAM__
#define __GAIJIN_PROTOIO_INPUTCODESTREAM__

#include "types.h"
#include "binaryStream.h"
#include "codeStream.h"
#include <protoConfig.h>
#include <util/dag_baseDef.h>
#include <debug/dag_assert.h>


#define PROTO_ERROR_INTERNAL(msg)            \
{                                            \
  logError(__FILE__, __LINE__, msg);         \
  return false;                              \
}

#define PROTO_VALIDATE_INTERNAL(expr)\
if (!(expr))                         \
{                                    \
  PROTO_ERROR_INTERNAL(#expr);        \
}

#define PROTO_ERROR(msg)                     \
{                                            \
  stream.logError(__FILE__, __LINE__, msg);  \
  return false;                              \
}

#define PROTO_VALIDATE(expr) \
if (!(expr))                 \
{                            \
  PROTO_ERROR(#expr);        \
}

namespace proto
{
  namespace io
  {
    class InputCodeStream
    {
    public:
      typedef InputBinaryStream::TSize TSize;

      explicit InputCodeStream(const ConstBuffer & buffer)
        : stream(buffer)
      {
      }

      const InputBinaryStream & getBinaryStream() const
      {
        return stream;
      }

      void logError(const char * file, int line, const char * msg);
      const char * errorMessage() const;

      template <typename T>
      bool readVarInt(StreamTag tag, T & value);

      template <typename T>
      bool readVarInt(T & value);

      template <typename T>
      bool readEnum(StreamTag tag, T & value);

      bool readString(StreamTag tag, ConstBuffer & stringData);
      bool readBool(StreamTag tag, bool & value);
      bool readTag(StreamTag & tag);

      bool readFloat(StreamTag tag, float & value);
      bool readDouble(StreamTag tag, double & value);

      bool readRAW(StreamTag tag, void * ptr, TSize size);

      template <typename T>
      bool readRAW(StreamTag tag, T & value);

      bool skip(StreamTag tag);

    private:
      InputBinaryStream stream;
      StreamTag         lastTag;
      Buffer            errorBuffer;
    };

    inline void InputCodeStream::logError(const char * file, int line, const char * msg)
    {
      const int reserveSize = strlen(file) + strlen(msg) + 128;
      errorBuffer.reserve(errorBuffer.size() + reserveSize);
      if (errorBuffer.size() == 0)
      {
        SNPRINTF(errorBuffer.end(), errorBuffer.getAllocated() - errorBuffer.size(),
            "Error in stream, last tag=%d, number=%d\n",
            lastTag.type, lastTag.number);

        int writen = (int)strlen(errorBuffer.end()) + 1;
        errorBuffer.resize(errorBuffer.size() + writen);
      }
      char * startWrite = errorBuffer.end() - 1;
      SNPRINTF(startWrite, errorBuffer.getAllocated() - errorBuffer.size() + 1,
          "%s:%d - %s\n", file, line, msg);

      int writen = (int)strlen(errorBuffer.end()) + 1;
      errorBuffer.resize(errorBuffer.size() + writen);
    }

    inline const char * InputCodeStream::errorMessage() const
    {
      return errorBuffer.begin() ? errorBuffer.begin() : "";
    }

    inline bool InputCodeStream::skip(StreamTag tag)
    {
      switch (tag.type)
      {
      case StreamTag::EMPTY:
      case StreamTag::EMPTY2:
        break;
      case StreamTag::RAW32:
        PROTO_VALIDATE_INTERNAL(stream.seekCurrent(4));
        break;

      case StreamTag::VAR_INT:
      case StreamTag::VAR_INT_MINUS:
        {
          int64 dummy = 0;
          PROTO_VALIDATE_INTERNAL(readVarInt(dummy));
          break;
        }
      case StreamTag::VAR_LENGTH:
        {
          InputBinaryStream::TSize size = 0;
          PROTO_VALIDATE_INTERNAL(readVarInt(size));
          PROTO_VALIDATE_INTERNAL(size >= 0);
          PROTO_VALIDATE_INTERNAL(stream.seekCurrent(size));
          break;
        }
      case StreamTag::BLOCK_BEGIN:
        {
          StreamTag childTag;
          while (true)
          {
            PROTO_VALIDATE_INTERNAL(readTag(childTag));
            if (childTag.isBlockEnded())
            {
              break;
            }

            PROTO_VALIDATE_INTERNAL(skip(childTag));
          }

          break;
        }
      default:
        PROTO_ERROR_INTERNAL("Can't skip invalid tag");
      }

      return true;
    }

    inline bool InputCodeStream::readRAW(StreamTag tag, void * ptr, TSize size)
    {
      TSize sizeToRead = 0;
      if (tag.type == proto::io::StreamTag::RAW32)
      {
        sizeToRead = 4;
      }
      else
      {
        PROTO_VALIDATE_INTERNAL(tag.type == proto::io::StreamTag::VAR_LENGTH);
        readVarInt(sizeToRead);
      }

      PROTO_VALIDATE_INTERNAL(size == sizeToRead);
      PROTO_VALIDATE_INTERNAL(stream.read(ptr, size));
      return true;
    }

    template <typename T>
    inline bool InputCodeStream::readRAW(StreamTag tag, T & value)
    {
      return readRAW(tag, &value, sizeof(value));
    }


    inline bool InputCodeStream::readBool(StreamTag tag, bool & value)
    {
      switch (tag.type)
      {
      case StreamTag::EMPTY:
        value = false;
        break;
      case StreamTag::EMPTY2:
        value = true;
        break;
      default:
        PROTO_ERROR_INTERNAL("Unexpected tag");
      }

      return true;
    }


    inline bool InputCodeStream::readFloat(StreamTag tag, float & value)
    {
      switch (tag.type)
      {
      case StreamTag::EMPTY:
        value = 0.0f;
        break;
      case StreamTag::RAW32:
        stream.read(&value, 4);
        break;
      default:
        PROTO_ERROR_INTERNAL("Unexpected tag");
      }

      return true;
    }


    inline bool InputCodeStream::readDouble(StreamTag tag, double & value)
    {
      switch (tag.type)
      {
      case StreamTag::EMPTY:
        value = 0.0;
        break;
      case StreamTag::VAR_LENGTH:
        {
          int size = 0;
          PROTO_VALIDATE_INTERNAL(readVarInt(size));
          PROTO_VALIDATE_INTERNAL(size == 8);

          stream.read(&value, 8);
        }
        break;
      default:
        PROTO_ERROR_INTERNAL("Unexpected tag");
      }

      return true;
    }


    inline bool InputCodeStream::readTag(StreamTag & tag)
    {
      char ch;
      PROTO_VALIDATE_INTERNAL(stream.get(ch));

      tag.type = StreamTag::Type(ch & StreamTag::TAG_MASK);
      tag.number = ((unsigned char)ch) >> StreamTag::TAG_BITS_COUNT;
      if (!tag.number)
      {
        PROTO_VALIDATE_INTERNAL(readVarInt(tag.number));
        tag.number += StreamTag::SHORT_NUMBER_LIMIT;
      }

      lastTag = tag;
      return true;
    }

    template <typename T>
    inline bool InputCodeStream::readVarInt(T & value)
    {
      char token;
      int counter = 0;
      uint64 unsignedValue = 0;
      while (true)
      {
        PROTO_VALIDATE_INTERNAL(stream.get(token));
        PROTO_VALIDATE_INTERNAL(counter <= 64);
        unsignedValue |= (uint64(token & 0x7F) << counter);
        if (!(token & 0x80))
        {
          value = unsignedValue;
          break;
        }

        counter += 7;
      }

      return true;
    }

    template <typename T>
    inline bool InputCodeStream::readVarInt(StreamTag tag, T & value)
    {
      switch (tag.type)
      {
      case StreamTag::EMPTY:
        value = 0;
        break;
      case StreamTag::RAW32:
        {
          int32 int32Value = 0;
          PROTO_VALIDATE_INTERNAL(stream.read(&int32Value, 4));
          value = int32Value;
        }
        break;
      case StreamTag::VAR_INT:
        PROTO_VALIDATE_INTERNAL(readVarInt(value));
        break;
      case StreamTag::VAR_INT_MINUS:
        PROTO_VALIDATE_INTERNAL(readVarInt(value));
        value *= -1;
        break;
      default:
        PROTO_ERROR_INTERNAL("Unexpected tag");
      }

      return true;
    }

    inline bool InputCodeStream::readString(StreamTag tag, ConstBuffer & value)
    {
      value = ConstBuffer();
      TSize restorePosition = 0;
      switch (tag.type)
      {
      case StreamTag::EMPTY:
          break;
      case StreamTag::RAW32:
      case StreamTag::VAR_INT:
          Buffer::TSize position;
          PROTO_VALIDATE_INTERNAL(readVarInt(tag, position));
          restorePosition = stream.tell();
          PROTO_VALIDATE_INTERNAL(position > 0);
          PROTO_VALIDATE_INTERNAL(position < restorePosition);

          stream.seekBegin(position);
      case StreamTag::VAR_LENGTH:
          {
            ConstBuffer::TSize size;
            PROTO_VALIDATE_INTERNAL(readVarInt(size));
            PROTO_VALIDATE_INTERNAL(size > 0);
            PROTO_VALIDATE_INTERNAL(stream.getBuffer(value, size));
          }

          if (restorePosition > 0)
          {
            stream.seekBegin(restorePosition);
          }
          break;
      default:
          PROTO_ERROR_INTERNAL("Unexpected tag");
      }

      return true;
    }

    template <typename T>
    inline bool InputCodeStream::readEnum(StreamTag tag, T & value)
    {
      int64 intValue;
      PROTO_VALIDATE_INTERNAL(readVarInt(tag, intValue));
      T newValue = T(intValue);
      PROTO_VALIDATE_INTERNAL(validate_enum_value(newValue));
      value = newValue;
      return true;
    }


    template <typename T>
    inline bool readString(InputCodeStream & stream, StreamTag tag, T & value)
    {
      ConstBuffer stringData;
      PROTO_VALIDATE(stream.readString(tag, stringData));
      str_assign(value, stringData.begin(), stringData.size());
      return true;
    }


    template <typename T, typename F>
    bool readObject(proto::io::InputCodeStream & stream, T & obj, F fn, int expectedTag)
    {
      proto::io::StreamTag tag;
      PROTO_VALIDATE(stream.readTag(tag));
      PROTO_VALIDATE(tag.number == expectedTag)
      PROTO_VALIDATE((obj.*fn)(stream, tag));
      return true;
    }
  }
}

#undef PROTO_ERROR_INTERNAL
#undef PROTO_VALIDATE_INTERNAL

#endif // __GAIJIN_PROTOIO_INPUTCODESTREAM__
