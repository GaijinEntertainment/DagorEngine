#pragma once
#ifndef __GAIJIN_PROTOIO_BUFFER__
#define __GAIJIN_PROTOIO_BUFFER__

#include <debug/dag_assert.h>

#include <limits>


namespace proto
{
  namespace io
  {
    class ConstBuffer
    {
    public:
      typedef int TSize;

      ConstBuffer()
        : data_(NULL)
        , size_(0)
      {
      }

      ConstBuffer(const char * data, TSize size)
        : data_(data)
        , size_(size)
      {
      }

      const char * begin() const
      {
        return data_;
      }

      const char * end() const
      {
        return data_ + size_;
      }

      TSize size() const
      {
        return size_;
      }

    private:
      const char *  data_;
      TSize         size_;
    };


    class MoveBuffer
    {
    public:
      typedef int TSize;

      MoveBuffer(const MoveBuffer & src)
      {
        data_ = NULL;
        *this = src;
      }

      MoveBuffer & operator=(const MoveBuffer & src)
      {
        if (this == &src)
          return *this;

        delete[] data_;

        data_ = src.data_;
        size_ = src.size_;
        allocated_ = src.allocated_;
        bigStep_ = src.bigStep_;

        src.data_ = NULL;
        src.size_ = 0;
        src.allocated_ = 0;
        return *this;
      }

      void clear()
      {
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
        allocated_ = 0;
      }

      ~MoveBuffer()
      {
        clear();
      }

    protected:

      MoveBuffer(TSize bigStep)
        : data_(NULL)
        , size_(0)
        , allocated_(0)
        , bigStep_(bigStep)
      {
      }

      // hack with mutable create for make constructors with argumet - const MoveBuffer &
      // if we use MoveBuffer & instead then we can't compile buffer(srcBuffer.move()) in gcc 4.4
      // because in gcc 4.4 we can't pass temporary object in function that expect non const reference
      mutable char *  data_;
      mutable TSize   size_;
      mutable TSize   allocated_;
      TSize           bigStep_;
    };


    class Buffer : private MoveBuffer
    {
      Buffer(const Buffer &);
      Buffer& operator=(const Buffer &);

    public:
      typedef MoveBuffer::TSize TSize;

      Buffer(TSize bigStep = 1 << 20)
        : MoveBuffer(bigStep)
      {
      }

      Buffer(const MoveBuffer & src)
        : MoveBuffer(src)
      {
      }

      Buffer & operator=(const MoveBuffer & src)
      {
        MoveBuffer::operator=(src);
        return *this;
      }

      operator ConstBuffer() const
      {
        return ConstBuffer(begin(), size());
      }

      const char * begin() const
      {
        return data_;
      }

      const char * end() const
      {
        return data_ + size_;
      }

      char * begin()
      {
        return data_;
      }

      char * end()
      {
        return data_ + size_;
      }

      TSize size() const
      {
        return size_;
      }

      void resize(TSize new_size)
      {
        G_ASSERT(new_size >= 0);
        if (new_size > allocated_)
        {
          reserve(new_size);
        }

        size_ = new_size;
      }

      void reserve(TSize new_size)
      {
        if (new_size <= allocated_)
        {
          return;
        }

        TSize newAllocated = calcAllocated(new_size);
        char * newData = new char[newAllocated];
        memcpy(newData, data_, size_);
        delete[] data_;
        data_ = newData;
        allocated_ = newAllocated;
      }

      TSize getAllocated()
      {
        return allocated_;
      }

      void clear()
      {
        MoveBuffer::clear();
      }

      MoveBuffer move()
      {
        return *this;
      }

      void swap(Buffer & buffer)
      {
        MoveBuffer tmp(move());
        *this = buffer;
        buffer.move() = tmp;
      }

    private:

      TSize calcAllocated(TSize new_size)
      {
        G_ASSERT(new_size > 0);

        if (new_size >= bigStep_)
        {
          return ((new_size - 1) / bigStep_ + 1) * bigStep_;
        }

        const TSize MIN_SIZE = 1 << 7;

        TSize result = allocated_ ? allocated_ : MIN_SIZE;
        while (result < new_size)
        {
          result <<= 1;
        }

        return result;
      }
    };
  }
}

#endif // __GAIJIN_PROTOIO_BUFFER__
