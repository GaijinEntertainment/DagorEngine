//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//

/**
 * @brief This file contains ring buffer template class and its derived classes.
 */
#pragma once

#include <3d/dag_drv3d.h>
#include <util/dag_globDef.h>

/**
 * @brief Ring dynamic buffer template class.
 *
 * @tparam BUF  Managed buffer type.
 * @tparam T    Type of the data stored in the buffer.
 * 
 * This class provides a mechanism for managing dynamic buffers in a ring buffer fashion.
 * It is expected, that user will implement initialization themselves, should they derive any classes.
 *
 */
template <class BUF, class T>
class RingDynamicBuffer
{
public:

  /**
   * @brief Default constructor.
   */
  RingDynamicBuffer() : buf(NULL), stride(0), pos(0), refCount(0), rounds(0), sPos(0) {}

  /**
   * @brief Destructor
   */
  ~RingDynamicBuffer() { close(); }

  /**
   * @brief Adds elements to the buffer.
   * 
   * @param [in] data   A pointer to the buffer storing the elements to add.
   * @param [in] count  Number of elements to add.
   * @return            Position of the first added element on success, -1 on failure.
   *
   * The data is placed continuously. In case the data does not fit after the last stored element,
   * the it is added at the start of the buffer and all overlapping elements get erased (i.e. the buffer is reset).
   * The function fails if \b count is greater than the size of the buffer.
   */
  int addData(const T *__restrict data, int count)
  {
    if (pos + count > size)
    {
      if (count > size)
        return -1;
      else
      {
        pos = 0;
        rounds++;
      }
    }
    buf->updateData(pos * stride, count * stride, data,
      VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (pos == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE));

    int cp = pos;
    pos += count;
    return cp;
  }

  /**
   * @brief Unlocks the managed buffer.
   * 
   * @param [in] used_count Number of elements to unlock memory for.
   * 
   * If the buffer has been locked, it should be unlocked before adding 
   * new elements. See \ref lockData for details.
   */
  void unlockData(int used_count)
  {
    pos += used_count;
    buf->unlock();
  }

  /**
   * @brief Locks memory block in the buffer.
   * 
   * @param [in] max_count  Number of elements to lock memory for.
   * @return                A pointer to the locked memory block on success, NULL on failure.
   *
   * @warning After the buffer has been locked, no data should be added using \ref addData before 
   *          the buffer is unlocked. Doing so will result in overwriting the locked memory.
   * 
   * The method locks a continuous memory block. If it does not fit after the last stored element,
   * the locked block is positioned at the start of the buffer (i.e. the buffer is reset). 
   * In that case writing to the locked memory will erase overlapping elements.
   */
  T *lockData(int max_count)
  {
    T *p;
    if (pos + max_count > size)
    {
      if (max_count > size)
        return (T *)NULL;
      else
      {
        pos = 0;
        rounds++;
      }
    }
    if (buf->lockEx(pos * stride, max_count * stride, (T **)&p, VBLOCK_WRITEONLY | (pos == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE)))
      return p;
    return (T *)NULL;
  }

  /**
   * @brief Returns the pointer to the managed buffer.
   * 
   * @return A pointer to the managed buffer.
   */
  BUF *getBuf() const { return buf; }

  /**
   * @brief Returns the size of the buffer.
   * 
   * @return Size of the buffer.
   */
  int bufSize() const { return size; }

  /**
   * @brief Evaluates the maximal number of elements that fit between the last stored item and the end of the buffer.
   * 
   * @return Evaluated number of elements.
   */
  int bufLeft() const { return size - pos; }

  /**
   * @brief Returns the buffer stride.
   * 
   * @return Buffer stride.
   */
  int getStride() const { return stride; }

  /**
   * @brief Returns the position of the virtual element following the last element of the buffer.
   * 
   * @return Position after the last stored element.
   */
  int getPos() const { return pos; }

  /**
   * @brief Evaluates the number of elements added since the last reset 
   * (or since the buffer initialization, if no reset has been done so far).
   * 
   * @return The evaluated number of elements.
   */
  int getProcessedCount() const { return rounds * size + pos - sPos; }

  /**
   * @brief Resets the buffer but leaves the counters intact.
   */
  void resetPos()
  {
    pos = 0;
    rounds++;
  }

  /**
   * @brief Resets buffer processed elements counter.
   * 
   * The buffer is not reset.
   */
  void resetCounters()
  {
    rounds = 0;
    sPos = pos;
  }

  /**
   * @brief Destroys the managed buffer.
   * 
   * The buffer is reset as well.
   */
  void close()
  {
    destroy_it(buf);
    stride = 0;
    pos = 0;
  }

  /**
   * @brief Increments the reference counter.
   */
  void addRef() { refCount++; }

  /**
   * @brief Decrements the reference counter. Provided that it has reached 0, deletes the buffer.
   */
  void delRef()
  {
    refCount--;
    refCount == 0 ? delete this : (void)0;
  }

protected:

  /**
   * @brief Pointer to the stored buffer.
   */
  BUF *buf;

  /**
   * @brief Determines the number of bytes reserved for a single element.
   * Must be larger or equal to its actual size.
   */
  int stride;
  
  /**
   * @brief Determines the position of the virtual element
   * following the last element of the buffer.
   */
  int pos;
  
  /**
   * @brief Determines the number of the elements stored within the buffer.
   */
  int size;

  /**
   * @brief Determines the value of \c pos by the
   * moment the counters have been reset last time.
   */
  int sPos;

  /**
   * @brief Determines the number of references to the buffer.
   */
  int16_t refCount;
  
  /**
   * @brief Determines how many times the buffer has
   * been reset so far.
   */
  int16_t rounds;
};

/** @brief Ring dynamic Vertex Buffer
 *
 * Class for managing ring dynamic vertex buffer.
 *
 */
class RingDynamicVB : public RingDynamicBuffer<Vbuffer, void>
{
public:

  /**
   * @brief Initializes the vertex buffer.
   *
   * @param [in] v_count    Number of vertices.
   * @param [in] v_stride   Vertex stride.
   * @param [in] stat_name  Name of the buffer (optional, default is the current file name).
   */
  void init(int v_count, int v_stride, const char *stat_name = __FILE__)
  {
    close();
    buf = d3d::create_vb(v_count * v_stride, SBCF_DYNAMIC, stat_name);
    d3d_err(buf);
    stride = v_stride;
    size = v_count;
  }
};

/** 
 * @brief Class for managing ring dynamic shader buffer.
 */
class RingDynamicSB : public RingDynamicBuffer<Sbuffer, void>
{
public:

  /**
   * @brief Destructor.
   */
  ~RingDynamicSB() { close(); }

  /**
   * @brief Initializes the ring buffer.
   *
   * @param [in] v_count    Number of elements in the buffer.
   * @param [in] v_stride   Element stride.
   * @param [in] elem_size  Element size. Must be a multiple of \b v_stride.
   * @param [in] flags      Buffer flags.
   * @param [in] format     Buffer format.
   * @param [in] stat_name  Name of the buffer (optional, default is the current file name).
   * 
   * @todo Staging buffer creation is not optimal due to large GPU memory allocation. Requires optimization.
   * 
   * The function initializes managed buffer as dynamic and CPU-writable, if the following conditions are met:
   *    \li The driver supports locking of shader buffers with the no-overwrite method.
   *    \li The driver used is not DirectX11.
   *    \li The buffer is designed as stream output for indirect draw (indicated by \c SBCF_MISC_DRAWINDIRECT flag).
   * 
   * Otherwise, a staging (dynamic, CPU-writable) and render buffers are created. 
   * The ring buffer instance possesses exclusive ownership of the render buffer.
   */
  void init(int v_count, int v_stride, int elem_size, uint32_t flags, uint32_t format, const char *stat_name = __FILE__)
  {
    close();
    G_ASSERT(v_stride % elem_size == 0);
    Sbuffer *stagingBuf = 0;
    if (d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers &&
        !(d3d::get_driver_code().is(d3d::dx11) && (flags & SBCF_MISC_DRAWINDIRECT)))
      flags |= SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE;
    else // not optimal, since we allocate in gpu memory too much. todo: optimize
    {
      stagingBuf = d3d::create_sbuffer(elem_size, v_count * (v_stride / elem_size),
        SBCF_DYNAMIC | SBCF_BIND_VERTEX | SBCF_CPU_ACCESS_WRITE, 0, stat_name); // we don't need SBCF_BIND_VERTEX, but
                                                                                // DX driver demands it
      d3d_err(stagingBuf);
    }
    buf = d3d::create_sbuffer(elem_size, v_count * (v_stride / elem_size), flags, format, stat_name);
    d3d_err(buf);
    if (stagingBuf)
    {
      renderBuf = buf;
      buf = stagingBuf;
    }
    stride = v_stride;
    size = v_count;
  }

  /**
   * @brief Unlocks the managed buffer.
   *
   * @param [in] used_count Number of elements to unlock memory for.
   *
   * In case staging buffer was created, the function copies unlocked memory from it to the render buffer.
   */
  void unlockData(int used_count) // not optimal, since we allocate in gpu memory too much
  {
    RingDynamicBuffer<Sbuffer, void>::unlockData(used_count);
    if (!renderBuf || !used_count)
      return;
    buf->copyTo(renderBuf, (pos - used_count) * stride, (pos - used_count) * stride, used_count * stride);
  }

  /**
   * @brief Destroys the managed buffer.
   *
   * Provided that the instance exclusively owns the render buffer, destroys it.
   * The counters and the buffer are reset as well.
   */
  void close()
  {
    if (!bufOwned)
      (renderBuf ? renderBuf : buf) = nullptr;
    destroy_it(renderBuf);
    RingDynamicBuffer<Sbuffer, void>::close();
  }

  /**
   * @brief Retrives the pointer to the render buffer without terminating its exclusive ownership by the ring buffer.
   * 
   * @return A pointer to the render buffer.
   * 
   * See \ref takeRenderBuf if you want to terminate the exclusive ownership.
   */
  Sbuffer *getRenderBuf() const { return renderBuf ? renderBuf : buf; }

  /**
   * @brief Retrieves the pointer to the render buffer and terminates its exclusive ownership by the ring buffer.
   *
   * @return A pointer to the render buffer.
   * 
   * Not being the exclusive owner of the render buffer, the ring buffer is not responsible for its memory deallocating.
   * See \ref getRenderBuf if terminating the exclusive ownership is undesirable.
   */
  Sbuffer *takeRenderBuf()
  {
    bufOwned = false;
    return renderBuf ? renderBuf : buf;
  }

protected:
  /**
   * @brief A pointer to the render buffer, which is used by GPU.
   */
  Sbuffer *renderBuf = 0;

  /**
   * @brief Determines whether the class exclusively owns the render buffer.
   * 
   * Not being the exclusive owner of the render buffer, the ring buffer is not responsible for its memory deallocating.
   */
  bool bufOwned = true;

  int addData(const void *__restrict, int) { return 0; } // not implemented
};

/**
 * @brief Class for managing ring dynamic index buffer.
 */
class RingDynamicIB : public RingDynamicBuffer<Ibuffer, uint16_t>
{
public:

  /**
   * @brief Initializes the ring buffer.
   * 
   * @param [in] i_count Number of indices.
   */
  void init(int i_count)
  {
    close();
    buf = d3d::create_ib(i_count * 2, SBCF_DYNAMIC);
    d3d_err(buf);
    stride = 2;
    size = i_count;
  }
};
