//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <util/dag_globDef.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_resPtr.h>


template <class BUF, class T>
class RingDynamicBuffer
{
public:
  RingDynamicBuffer() : stride(0), pos(0), refCount(0), rounds(0), sPos(0) {}
  RingDynamicBuffer(RingDynamicBuffer &&) = default;
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

  template <typename U = T>
  LockedBuffer<U> lockBufferAs(uint32_t max_count, uint32_t &prev_pos)
  {
    G_ASSERT(sizeof(U) % stride == 0);
    const uint32_t maxCountInInnerElems = max_count * sizeof(U) / stride;
    if (pos + maxCountInInnerElems > size)
    {
      if (maxCountInInnerElems > size)
        return LockedBuffer<U>();
      else
      {
        pos = 0;
        rounds++;
      }
    }
    prev_pos = pos;
    pos += maxCountInInnerElems;
    return lock_sbuffer<U>(buf.getBuf(), prev_pos * stride, max_count,
      VBLOCK_WRITEONLY | (prev_pos == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE));
  }

  Sbuffer *getBuf() const { return buf.getBuf(); }
  D3DRESID getBufId() const { return buf.getBufId(); }
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
    buf.close();
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
  BUF buf;
  int stride, pos, size, sPos;
  int16_t refCount, rounds;
};


class RingDynamicVB : public RingDynamicBuffer<UniqueBuf, void>
{
public:
  void init(int v_count, int v_stride, const char *stat_name)
  {
    close();
    buf = dag::create_vb(v_count * v_stride, SBCF_DYNAMIC, stat_name);
    stride = v_stride;
    size = v_count;
  }
};

class RingDynamicSB : public RingDynamicBuffer<UniqueBuf, void>
{
public:
  RingDynamicSB() = default;
  RingDynamicSB(RingDynamicSB &&) = default;
  ~RingDynamicSB() { close(); }
  void init(int v_count, int v_stride, int elem_size, uint32_t flags, uint32_t format, const char *stat_name)
  {
    close();
    G_ASSERT(v_stride % elem_size == 0);
    UniqueBuf stagingBuf;
    if (d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers &&
        !(d3d::get_driver_code().is(d3d::dx11) && (flags & SBCF_MISC_DRAWINDIRECT)))
      flags |= SBCF_DYNAMIC | SBCF_CPU_ACCESS_WRITE;
    else // not optimal, since we allocate in gpu memory too much. todo: optimize
    {
      String stagingName(0, "%s_staging", stat_name);
      stagingBuf = dag::create_sbuffer(elem_size, v_count * (v_stride / elem_size),
        SBCF_DYNAMIC | SBCF_BIND_VERTEX | SBCF_CPU_ACCESS_WRITE, 0, stagingName.c_str()); // we don't need SBCF_BIND_VERTEX, but
                                                                                          // DX driver demands it
    }
    buf = dag::create_sbuffer(elem_size, v_count * (v_stride / elem_size), flags, format, stat_name);
    if (stagingBuf)
    {
      renderBuf = eastl::move(buf);
      buf = eastl::move(stagingBuf);
    }
    stride = v_stride;
    size = v_count;
  }
  void unlockData(int used_count) // not optimal, since we allocate in gpu memory too much
  {
    RingDynamicBuffer<UniqueBuf, void>::unlockData(used_count);
    if (!renderBuf || !used_count)
      return;
    buf.getBuf()->copyTo(renderBuf.getBuf(), (pos - used_count) * stride, (pos - used_count) * stride, used_count * stride);
  }
  void close()
  {
    renderBuf.close();
    RingDynamicBuffer<UniqueBuf, void>::close();
  }
  Sbuffer *getRenderBuf() const { return (renderBuf ? renderBuf : buf).getBuf(); }
  int addData(const void *__restrict data, int count)
  {
    if (count <= 0)
      return -1;
    int previousPos = RingDynamicBuffer<UniqueBuf, void>::addData(data, count);
    if (previousPos == -1)
      return -1;
    if (renderBuf)
      buf.getBuf()->copyTo(renderBuf.getBuf(), previousPos * stride, previousPos * stride, count * stride);
    return pos - count;
  }
  D3DRESID getBufId() const
  {
    if (renderBuf)
      return renderBuf.getBufId();
    return RingDynamicBuffer<UniqueBuf, void>::getBufId();
  }

protected:
  UniqueBuf renderBuf;
};

class RingDynamicIB : public RingDynamicBuffer<UniqueBuf, uint16_t>
{
public:
  void init(int i_count, const char *name)
  {
    close();
    buf = dag::create_ib(i_count * 2, SBCF_DYNAMIC, name);
    stride = 2;
    size = i_count;
  }
};

class RingDynamicIB32 : public RingDynamicBuffer<UniqueBuf, uint32_t>
{
public:
  void init(int i_count, const char *name)
  {
    close();
    buf = dag::create_ib(i_count * sizeof(uint32_t), SBCF_DYNAMIC | SBCF_INDEX32, name);
    stride = sizeof(uint32_t);
    size = i_count;
  }
};
