#pragma once
#ifndef __GAIJIN_PROTOIO_BINARYSTREAM__
#define __GAIJIN_PROTOIO_BINARYSTREAM__

#include "buffer.h"

#include <debug/dag_assert.h>


namespace proto
{
  namespace io
  {
    class InputBinaryStream
    {
    public:
      typedef ConstBuffer::TSize TSize;
      typedef const char * TIter;

      InputBinaryStream(const ConstBuffer & buffer_)
        : buffer(buffer_)
        , position(buffer.begin())
        , bufferEnd(buffer.end())
      {
      }

      Buffer::TSize tell() const
      {
        return position - buffer.begin();
      }

      TIter cursor() const
      {
        return position;
      }

      bool eof() const
      {
        return position == bufferEnd;
      }

      bool getBuffer(ConstBuffer & buf, TSize size)
      {
        TIter nextPosition = position + size;
        if (nextPosition > bufferEnd)
        {
          return false;
        }

        buf = ConstBuffer(position, size);

        position = nextPosition;
        return true;
      }

      bool read(void * ptr, TSize size)
      {
        TIter nextPosition = position + size;
        if (nextPosition > bufferEnd)
        {
          return false;
        }

        memcpy(ptr, position, size);
        position = nextPosition;
        return true;
      }

      bool get(char & value)
      {
        TIter nextPosition = position + 1;
        if (nextPosition > bufferEnd)
        {
          return false;
        }

        value = *position;
        position = nextPosition;
        return true;
      }

      bool seekBegin(int delta)
      {
        return seek(buffer.begin(), delta);
      }

      bool seekEnd(int delta)
      {
        return seek(buffer.end(), delta);
      }

      bool seekCurrent(int delta)
      {
        return seek(position, delta);
      }

    private:
      bool seek(TIter iter, int delta)
      {
        TIter newPosition = iter + delta;
        if (newPosition < buffer.begin() || newPosition > bufferEnd)
        {
          return false;
        }

        position = newPosition;
        return true;
      }

      ConstBuffer   buffer;
      TIter         position;
      TIter const   bufferEnd;
    };


    class OutputBinaryStream
    {
    public:
      typedef Buffer::TSize TSize;
      typedef char * TIter;

      OutputBinaryStream()
        : buffer()
        , position(buffer.begin())
      {
      }

      OutputBinaryStream(const MoveBuffer & buffer_)
        : buffer(buffer_)
        , position(buffer.begin())
      {
      }

      void reserve(TSize size)
      {
        TSize pos = tell();
        buffer.reserve(size);
        seekBegin(pos);
      }

      TIter cursor() const
      {
        return position;
      }

      const Buffer & getBuffer() const
      {
        return buffer;
      }

      MoveBuffer moveBuffer()
      {
        MoveBuffer result = buffer.move();
        position = buffer.begin();
        return result;
      }

      void put(char value)
      {
        checkResize(1);
        *position++ = value;
      }

      void write(const void * data, TSize size)
      {
        checkResize(size);
        memcpy(position, data, size);
        position += size;
      }

      TSize tell() const
      {
        return position - buffer.begin();
      }

      void seekBegin(int delta)
      {
        seek(buffer.begin(), delta);
      }

      void seekEnd(int delta)
      {
        seek(buffer.end(), delta);
      }

      void seekCurrent(int delta)
      {
        seek(position, delta);
      }

      void checkResize(int wantWriteSize)
      {
        Buffer::TSize offset = position - buffer.begin();
        Buffer::TSize newSize = offset + wantWriteSize;
        if (newSize > buffer.size())
        {
          buffer.resize(newSize);
          position = buffer.begin() + offset;
        }
      }

    private:
      void seek(TIter iter, int delta)
      {
        TIter newPosition = iter + delta;
        G_ASSERT(newPosition >= buffer.begin() && newPosition <= buffer.end());
        position = newPosition;
      }

      Buffer        buffer;
      TIter         position;
    };
  }
}

#endif // __GAIJIN_PROTOIO_BINARYSTREAM__
