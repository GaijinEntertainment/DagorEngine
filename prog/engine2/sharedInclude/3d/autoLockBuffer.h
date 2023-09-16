/************************************************************************
  vertex & index buffers - automatically lock when add data &
  unlock before set to driver. you can define static & dynamic
  buffers. for Vbuffer you must define vertex format struct
  or ANY_VERTEX if you want to add void* data
************************************************************************/
// Copyright 2023 by Gaijin Games KFT, All rights reserved.
#ifndef __AUTOBUF_H
#define __AUTOBUF_H

#include <3d/dag_drv3d.h>
#include <util/dag_globDef.h>

template <bool DYNAMIC>
class AutoLockIBuffer
{
public:
  AutoLockIBuffer(int ind_count) : ib(NULL), data(NULL), curInd(0), size(0) { create(ind_count); }

  ~AutoLockIBuffer() { close(); }

  // create or re-create buffer
  void create(int ind_count)
  {
    close();
    d3d_err(ib = d3d::create_ib(ind_count * sizeof(uint32_t), SBCF_INDEX32 | (DYNAMIC ? SBCF_DYNAMIC : 0)));
    curInd = 0;
    size = ind_count;
  }

  // close buffer
  void close()
  {
    unlock();
    del_d3dres(ib);
    ib = NULL;
    curInd = 0;
    size = 0;
  }

  // add data to buffer. return false, if overfill
  bool addData(const uint32_t *ind, int ind_count)
  {
    if (ind_count + curInd > size || !ib)
      return false;
    lock();
    if (!data)
      return false;
    memcpy(&data[curInd], ind, ind_count * sizeof(uint32_t));
    curInd += ind_count;
    return true;
  }

  // reserve space in buffer. fill manually. return NULL, if overfill
  uint32_t *reserveData(int ind_count)
  {
    if (ind_count + curInd > size || !ib)
      return NULL;
    lock();
    if (!data)
      return NULL;
    int from = curInd;
    curInd += ind_count;
    return &data[from];
  }

  // set buffer to driver. return number of indices
  int setToDriver(bool reset = true)
  {
    if (!ib)
      return 0;
    unlock();
    d3d_err(d3d::setind(ib));
    int i = curInd;
    if (reset)
      curInd = 0;
    return i;
  }

  // manually lock buffer
  void lock()
  {
    if (data || !ib)
      return;

    if (DYNAMIC)
    {
      d3d_err(ib->lock32(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (curInd == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE)));
    }
    else
    {
      d3d_err(ib->lock32(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK));
    }
  }

  // manually unlock buffer
  void unlock()
  {
    if (!data || !ib)
      return;
    d3d_err(ib->unlock());
    data = NULL;
  }

  // call it after rendering for dynamic buffers
  void processDynamic()
  {
    // if (DYNAMIC && ib) ib->swap();
  }

protected:
  Ibuffer *ib;
  int curInd;
  int size;
  uint32_t *data;
};


// struct for overloading functions, becouse typedef int VDECL and int are some types
struct VDecl
{
  VDECL vdecl;
  inline explicit VDecl(VDECL vd) : vdecl(vd) {}
  inline operator VDECL() const { return vdecl; }
};

template <bool DYNAMIC, typename VERTEX_FMT>
class AutoLockVBuffer
{
public:
  AutoLockVBuffer(int vertex_count, const VDecl &init_vdecl) : vb(NULL), data(NULL), vdecl(init_vdecl) { create(vertex_count); }

  ~AutoLockVBuffer() { close(); }

  // create or re-create buffer
  void create(int vertex_count)
  {
    close();
    d3d_err(vb = d3d::create_vb(vertex_count * sizeof(VERTEX_FMT), DYNAMIC ? SBCF_DYNAMIC : 0, __FILE__));
    curVert = 0;
    size = vertex_count;
  }

  // close buffer
  void close()
  {
    unlock();
    del_d3dres(vb);
    vb = NULL;
    curVert = 0;
    size = 0;
  }

  // add data to buffer. return false, if overfill
  bool addData(const VERTEX_FMT *v, int vertex_count)
  {
    if (vertex_count + curVert > size || !vb)
      return false;
    lock();
    if (!data)
      return false;
    memcpy(&data[curVert], v, vertex_count * sizeof(VERTEX_FMT));
    curVert += vertex_count;
    return true;
  }

  // reserve space in buffer. fill manually. return NULL, if overfill
  VERTEX_FMT *reserveData(int vertex_count)
  {
    if (vertex_count + curVert > size || !vb)
      return NULL;
    lock();
    if (!data)
      return NULL;
    int from = curVert;
    curVert += vertex_count;
    return &data[from];
  }

  // set buffer to driver. return number of vertices
  int setToDriver(bool reset = true)
  {
    if (!vb)
      return 0;
    unlock();
    d3d_err(d3d::setvsrc(0, vb, sizeof(VERTEX_FMT)));
    int v = curVert;
    if (reset)
      curVert = 0;
    return v;
  }

  inline VDECL getVDecl() const { return vdecl; }

  // set vertex format or VDECL to driver
  void setVertexFormat() const { d3d_err(d3d::setvdecl(vdecl)); }

  // manually lock buffer
  void lock()
  {
    if (data || !vb)
      return;

    if (DYNAMIC)
    {
      d3d_err(vb->lockEx(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (curVert == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE)));
    }
    else
    {
      d3d_err(vb->lockEx(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK));
    }
  }

  // manually unlock buffer
  void unlock()
  {
    if (!data || !vb)
      return;
    d3d_err(vb->unlock());
    data = NULL;
  }

  // call it after rendering for dynamic buffers
  void processDynamic()
  {
    // if (DYNAMIC && vb) vb->swap();
  }

protected:
  Vbuffer *vb;
  int curVert;
  int size;
  VDECL vdecl;
  VERTEX_FMT *data;
};


struct ANY_VERTEX
{};
template <bool DYNAMIC>
class AutoLockVBuffer<DYNAMIC, ANY_VERTEX>
{
public:
  AutoLockVBuffer(int vertex_count, VDECL init_vdecl, int _stride) : vb(NULL), data(NULL), vdecl(init_vdecl), stride(_stride)
  {
    create(vertex_count);
  }

  ~AutoLockVBuffer() { close(); }

  // create or re-create buffer
  void create(int vertex_count)
  {
    close();
    d3d_err(vb = d3d::create_vb(vertex_count * stride, DYNAMIC ? SBCF_DYNAMIC : 0, __FILE__));
    curVert = 0;
    size = vertex_count;
  }

  // close buffer
  void close()
  {
    unlock();
    del_d3dres(vb);
    vb = NULL;
    curVert = 0;
    size = 0;
  }

  // add data to buffer. return false, if overfill
  bool addData(const void *v, int vertex_count)
  {
    if (vertex_count + curVert > size || !vb)
      return false;
    lock();
    if (!data)
      return false;
    memcpy(&data[curVert * stride], v, vertex_count * stride);
    curVert += vertex_count;
    return true;
  }

  // reserve space in buffer. fill manually. return NULL, if overfill
  uint8_t *reserveData(int vertex_count)
  {
    if (vertex_count + curVert > size || !vb)
      return NULL;
    lock();
    if (!data)
      return NULL;
    int from = curVert;
    curVert += vertex_count;
    return &data[from * stride];
  }

  // set buffer to driver. return number of vertices
  int setToDriver(bool reset = true)
  {
    if (!vb)
      return 0;
    unlock();
    d3d_err(d3d::setvsrc(0, vb, stride));
    int v = curVert;
    if (reset)
      curVert = 0;
    return v;
  }

  // get vertex format
  inline VDECL getVDecl() const { return vdecl; }

  // set vertex format or VDECL to driver
  void setVertexFormat() const
  {
    G_ASSERT(vdecl != BAD_VDECL);
    d3d_err(d3d::setvdecl(vdecl));
  }

  // manually lock buffer
  void lock()
  {
    if (data || !vb)
      return;

    if (DYNAMIC)
    {
      d3d_err(vb->lockEx(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (curVert == 0 ? VBLOCK_DISCARD : VBLOCK_NOOVERWRITE)));
    }
    else
    {
      d3d_err(vb->lockEx(0, 0, &data, VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK));
    }
  }

  // manually unlock buffer
  void unlock()
  {
    if (!data || !vb)
      return;
    d3d_err(vb->unlock());
    data = NULL;
  }

  // call it after rendering for dynamic buffers
  void processDynamic()
  {
    // if (DYNAMIC && vb) vb->swap();
  }

protected:
  Vbuffer *vb;
  int curVert;
  int size;
  int stride;
  VDECL vdecl;
  uint8_t *data;
};

#endif //__AUTOBUF_H
