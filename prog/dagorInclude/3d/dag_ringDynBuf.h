//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_drv3d.h>
#include <util/dag_globDef.h>


template <class BUF, class T>
class RingDynamicBuffer
{
public:
  RingDynamicBuffer() : buf(NULL), stride(0), pos(0), refCount(0), rounds(0), sPos(0) {}
  ~RingDynamicBuffer() { close(); }

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
  void unlockData(int used_count)
  {
    pos += used_count;
    buf->unlock();
  }
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

  BUF *getBuf() const { return buf; }
  int bufSize() const { return size; }
  int bufLeft() const { return size - pos; }
  int getStride() const { return stride; }
  int getPos() const { return pos; }

  int getProcessedCount() const { return rounds * size + pos - sPos; }

  void resetPos()
  {
    pos = 0;
    rounds++;
  }
  void resetCounters()
  {
    rounds = 0;
    sPos = pos;
  }

  void close()
  {
    destroy_it(buf);
    stride = 0;
    pos = 0;
  }

  void addRef() { refCount++; }
  void delRef()
  {
    refCount--;
    refCount == 0 ? delete this : (void)0;
  }

protected:
  BUF *buf;
  int stride, pos, size, sPos;
  int16_t refCount, rounds;
};


class RingDynamicVB : public RingDynamicBuffer<Vbuffer, void>
{
public:
  void init(int v_count, int v_stride, const char *stat_name = __FILE__)
  {
    close();
    buf = d3d::create_vb(v_count * v_stride, SBCF_DYNAMIC, stat_name);
    d3d_err(buf);
    stride = v_stride;
    size = v_count;
  }
};

class RingDynamicSB : public RingDynamicBuffer<Sbuffer, void>
{
public:
  ~RingDynamicSB() { close(); }
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
  void unlockData(int used_count) // not optimal, since we allocate in gpu memory too much
  {
    RingDynamicBuffer<Sbuffer, void>::unlockData(used_count);
    if (!renderBuf || !used_count)
      return;
    buf->copyTo(renderBuf, (pos - used_count) * stride, (pos - used_count) * stride, used_count * stride);
  }
  void close()
  {
    if (!bufOwned)
      (renderBuf ? renderBuf : buf) = nullptr;
    destroy_it(renderBuf);
    RingDynamicBuffer<Sbuffer, void>::close();
  }
  Sbuffer *getRenderBuf() const { return renderBuf ? renderBuf : buf; }
  Sbuffer *takeRenderBuf()
  {
    bufOwned = false;
    return renderBuf ? renderBuf : buf;
  }

protected:
  Sbuffer *renderBuf = 0;
  bool bufOwned = true;
  int addData(const void *__restrict, int) { return 0; } // not implemented
};

class RingDynamicIB : public RingDynamicBuffer<Ibuffer, uint16_t>
{
public:
  void init(int i_count)
  {
    close();
    buf = d3d::create_ib(i_count * 2, SBCF_DYNAMIC);
    d3d_err(buf);
    stride = 2;
    size = i_count;
  }
};
