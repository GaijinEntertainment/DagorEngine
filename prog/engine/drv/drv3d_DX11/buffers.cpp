// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <mutex>

#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_info.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>
#include <perfMon/dag_graphStat.h>

#include "drv_assert_defs.h"
#include "drv_log_defs.h"
#include "genericBuffer.h"
#include "pools.h"
#include "render_state.h"
#include "driver.h"
#include "drv_utils.h"
#include "statStr.h"

#include <generic/dag_sort.h>
#include "buffers.h"
#include <immediateConstStub.h>

#include <validation.h>

#include <validate_sbuf_flags.h>
#include "resource_size_info.h"

#define USE_NVAPI_MULTIDRAW 0 // GPU hangs on SLI and under the Nsight.
#if HAS_NVAPI && USE_NVAPI_MULTIDRAW
#include <nvapi.h>
#endif

#include <3d/dag_resourceDump.h>

using namespace drv3d_dx11;

namespace drv3d_dx11
{
template <typename BufferType, int N>
struct BufferList : drv3d_generic::ObjectPoolWithLock<drv3d_generic::ObjectProxyPtr<BufferType>, N>
{
  BufferList(const char *dbg_name) : drv3d_generic::ObjectPoolWithLock<drv3d_generic::ObjectProxyPtr<BufferType>, N>(dbg_name) {}

  bool createObj(BufferType *buf, UINT size, int flg, int bindFlags, const char *statName)
  {
    if (buf->create(size, flg, bindFlags, statName))
    {
      drv3d_generic::ObjectProxyPtr<BufferType> e;
      e.obj = buf;
      buf->handle = drv3d_generic::ObjectPoolWithLock<drv3d_generic::ObjectProxyPtr<BufferType>, N>::safeAllocAndSet(e);
      if (buf->handle != drv3d_generic::BAD_HANDLE)
        return true;
      D3D_ERROR("Not enough handles for IB/VB buffer");
    }
    return false;
  }

  bool createSObj(BufferType *buf, int struct_size, int elements, uint32_t flags, uint32_t format, const char *statName)
  {
    if (buf->createStructured(struct_size, elements, flags, format, statName))
    {
      drv3d_generic::ObjectProxyPtr<BufferType> e;
      e.obj = buf;
      buf->handle = drv3d_generic::ObjectPoolWithLock<drv3d_generic::ObjectProxyPtr<BufferType>, N>::safeAllocAndSet(e);
      if (buf->handle != drv3d_generic::BAD_HANDLE)
        return true;
      D3D_ERROR("Not enough handles for IB/VB buffer");
    }
    return false;
  }
};

static BufferList<GenericBuffer, 2048> g_buffers("buffers");
void clear_buffers_pool_garbage() { g_buffers.clearGarbage(); }

void removeHandleFromList(int handle) { g_buffers.safeReleaseEntry(handle); }

void advance_buffers()
{
#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  ITERATE_OVER_OBJECT_POOL(g_buffers, i)
    if (auto buf = g_buffers[i].obj)
    {
      if (!(buf->bufFlags & SBCF_DYNAMIC))
        continue;
      buf->updated = false;
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_buffers)
#endif
}

template <typename PtrType, int DataAlign, typename BufferType, UINT BindFlags>
struct InlineBuffer
{
  BufferType buf; // working with GenericBuffer directly
  uint32_t currOffset = 0;

  bool create(uint32_t size, int flags)
  {
    bool res = buf.create(size, flags | SBCF_DYNAMIC, BindFlags, "inline");
    currOffset = 0;
    return res;
  }

  // alloc and lock
  PtrType *alloc(uint32_t size, uint32_t &out_start_offset)
  {
    uint32_t startOffset = currOffset;
    uint32_t alignedSize = POW2_ALIGN(size, DataAlign);
    uint32_t nextOffset = currOffset + alignedSize;
    void *ret = nullptr;
    if (nextOffset < buf.getSize())
    {
      currOffset = nextOffset;
      out_start_offset = startOffset;
      buf.lock(startOffset, alignedSize, &ret, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
    }
    else
    {
      if (alignedSize > buf.getSize())
      {
        D3D_CONTRACT_ASSERT_FAIL("InlineBuffer overflow");
        D3D_CONTRACT_ERROR("InlineBuffer overflow");
      }
      else
      {
        currOffset = alignedSize;
        out_start_offset = 0;
        buf.lock(0, alignedSize, &ret, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
      }
    }
    return (PtrType *)ret;
  }

  void unlock() { buf.unlock(); }
};

static InitOnDemand<InlineBuffer<uint8_t, 16, GenericBuffer, D3D11_BIND_VERTEX_BUFFER>, false> g_inline_vertex_buffer;

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS
static int sort_buffers_by_size(GenericBuffer *const *left, GenericBuffer *const *right)
{
  if (int diff = ((*left)->bufSize >> 10) - ((*right)->bufSize >> 10))
    return -diff;
  else
    return strcmp((*left)->getName(), (*right)->getName());
}

template <class T>
int get_buffer_mem_used(String *stat, T &bufList, int &out_sysmem)
{
  Tab<GenericBuffer *> sortedBuffersList(tmpmem);
  bufList.lock();
  ITERATE_OVER_OBJECT_POOL(bufList, bufferNo)
    if (bufList[bufferNo].obj != NULL)
      sortedBuffersList.push_back(bufList[bufferNo].obj);
  ITERATE_OVER_OBJECT_POOL_RESTORE(bufList);

  if (stat)
    sort(sortedBuffersList, &sort_buffers_by_size);
  int totalSize = 0;
  STAT_STR_BEGIN(add);
  for (int i = 0; i < sortedBuffersList.size(); ++i)
  {
    totalSize += sortedBuffersList[i]->bufSize;
    if (stat)
    {
      add.printf(0, "%5dK %s '%s' buffer\n", sortedBuffersList[i]->bufSize >> 10,
        sortedBuffersList[i]->bufFlags & SBCF_DYNAMIC ? "dynamic" : "static", sortedBuffersList[i]->getName());
      STAT_STR_APPEND(add, *stat);
    }
  }
  if (stat)
    STAT_STR_END(*stat);
  bufList.unlock();

  return totalSize;
}


#if DAGOR_DBGLEVEL > 0

void dump_buffers(Tab<ResourceDumpInfo> &dump_info)
{
  ITERATE_OVER_OBJECT_POOL(g_buffers, bufferNo)
    if (g_buffers[bufferNo].obj != NULL)
    {
      const GenericBuffer *buff = g_buffers[bufferNo].obj;

      dump_info.emplace_back(ResourceDumpBuffer({(uint64_t)-1, (uint64_t)-1, (uint64_t)-1, (uint64_t)-1, (uint64_t)-1, buff->getSize(),
        (uint32_t)-1, buff->getFlags(), -1, -1, -1, -1, buff->getName(), ""}));
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_buffers);
}
#else
void dump_buffers(Tab<ResourceDumpInfo> &) {}
#endif


int get_ib_vb_mem_used(String *stat, int &out_sysmem)
{
  out_sysmem = 0;
  int inlineSize = g_inline_vertex_buffer->buf.getSize();
  if (stat)
  {
    stat->aprintf(128, "%5dK inline vertex buffer\n", g_inline_vertex_buffer->buf.getSize() >> 10);
  }

  inlineSize += get_buffer_mem_used(stat, g_buffers, out_sysmem);

  return inlineSize;
}
#else
int get_ib_vb_mem_used(String *, int &) { return 0; }
#endif
bool init_buffers(RenderState &rs, int imm_vb_size)
{
  bool res = true;
  res &= g_inline_vertex_buffer.demandInit()->create(imm_vb_size, SBCF_BIND_VERTEX); // INLINE_VB_SIZE
  debug("init imm buffer: %d", imm_vb_size);
  res &= init_immediate_cb();
  return res;
}

void flush_buffers(RenderState &rs)
{
  if (!rs.vertexInputModified)
    return;

  ID3D11Buffer *buffers[MAX_VERTEX_STREAMS];
  UINT offsets[MAX_VERTEX_STREAMS];
  UINT strides[MAX_VERTEX_STREAMS];

  int maxStream = -1;
  int minStream = MAX_VERTEX_STREAMS;
  for (int i = 0; i < MAX_VERTEX_STREAMS; i++)
  {
    const VertexStream &vs = rs.nextVertexInput.vertexStream[i];
    const VertexStream &os = rs.currVertexInput.vertexStream[i];

    if (vs.source != os.source || vs.offset != os.offset || vs.stride != os.stride)
    {
      maxStream = i;
      minStream = min<int>(i, minStream);
    }

    if (maxStream < 0) // found difference?
      continue;

    if (vs.source != NULL)
    {
      buffers[i] = vs.source->buffer;
      G_ASSERTF(vs.source->buffer != NULL, "vs[%d].source=%p (%s)", i, vs.source, vs.source->getName());
      offsets[i] = vs.offset;
      strides[i] = vs.stride;
    }
    else
    {
      buffers[i] = NULL;
      offsets[i] = 0;
      strides[i] = 0;
    }
  }

  ContextAutoLock contextLock;

  if (maxStream >= 0)
    dx_context->IASetVertexBuffers(minStream, 1 + maxStream - minStream, buffers + minStream, strides + minStream,
      offsets + minStream);

  const GenericBuffer *ib = rs.nextVertexInput.indexBuffer;
  if (ib != rs.currVertexInput.indexBuffer)
  {
    if (ib != NULL)
    {
      dx_context->IASetIndexBuffer(ib->buffer, (ib->bufFlags & SBCF_INDEX32) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, 0);
    }
    else
      dx_context->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
  }

  rs.currVertexInput = rs.nextVertexInput; // FIXME: copy less
  rs.vertexInputModified = false;
}

void close_buffers()
{
  if (resetting_device_now)
  {
    vbuffer_sysmemcopy_usage = 0;
    ibuffer_sysmemcopy_usage = 0;

    ITERATE_OVER_OBJECT_POOL(g_buffers, i)
      if (g_buffers[i].obj != NULL)
      {
        g_buffers[i].obj->release();
      }
    ITERATE_OVER_OBJECT_POOL_RESTORE(g_buffers)
  }
  else
  {
    term_immediate_cb();
    g_buffers.clear();
  }
  g_inline_vertex_buffer.demandDestroy();
}

void gather_buffers_to_recreate(FramememResourceSizeInfoCollection &collection)
{
  debug("gather_buffers_to_recreate: %d", g_buffers.totalUsed());
  ITERATE_OVER_OBJECT_POOL(g_buffers, i)
    if (auto buf = g_buffers[i].obj)
    {
      collection.push_back({buf->getSize(), (uint32_t)i, false, buf->rld != nullptr});
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_buffers)
}

void recreate_buffer(uint32_t index)
{
  std::lock_guard lock(g_buffers);
  if (!g_buffers.isEntryUsed(index))
    return;
  if (auto buf = g_buffers[index].obj)
    buf->recreateBuf(buf);
}

} // namespace drv3d_dx11

Vbuffer *d3d::create_vb(int size, int flg, const char *name, ResourceTagType)
{
  GenericBuffer *buf = new GenericBuffer();
  flg |= SBCF_BIND_VERTEX;
  validate_sbuffer_flags(flg, name);
  D3D_CONTRACT_ASSERT(((flg & SBCF_BIND_MASK) & ~(SBCF_BIND_VERTEX | SBCF_BIND_SHADER_RES)) == 0);
  bool res = g_buffers.createObj(buf, size, flg, (flg & SBCF_BIND_MASK) >> 16, name);
  if (!res)
  {
    delete buf;
    buf = NULL;
  }

  return buf;
}

Ibuffer *d3d::create_ib(int size, int flg, const char *stat_name, ResourceTagType)
{
  GenericBuffer *buf = new GenericBuffer();
  flg |= SBCF_BIND_INDEX;
  validate_sbuffer_flags(flg, stat_name);
  D3D_CONTRACT_ASSERT(((flg & SBCF_BIND_MASK) & ~(SBCF_BIND_INDEX | SBCF_BIND_SHADER_RES)) == 0);
  bool res = g_buffers.createObj(buf, size, flg, (flg & SBCF_BIND_MASK) >> 16, stat_name);
  if (!res)
  {
    delete buf;
    buf = NULL;
  }

  return buf;
}

Vbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned format, const char *name, ResourceTagType)
{
  validate_sbuffer_flags(flags, name);
  GenericBuffer *buf = new GenericBuffer();
  if (flags & SBCF_BIND_CONSTANT)
  {
    D3D_CONTRACT_ASSERTF((flags & SBCF_BIND_SHADER_RES) == 0, "constant buffers shouldn't be bound as shader resource.");
  }
  bool res = g_buffers.createSObj(buf, struct_size, elements, flags, format, name); //== fixme: |SBCF_BIND_VERTEX
  if (!res)
  {
    delete buf;
    buf = NULL;
  }

  if (flags & SBCF_BIND_UNORDERED && buf)
    buf->createUav();

  return buf;
}

bool d3d::setvsrc_ex(int slot, Sbuffer *vb, int ofs, int stride)
{
  RenderState &rs = g_render_state;
  rs.nextVertexInput.vertexStream[slot].source = (GenericBuffer *)vb;
  rs.nextVertexInput.vertexStream[slot].offset = ofs;
  rs.nextVertexInput.vertexStream[slot].stride = stride;
  rs.modified = rs.vertexInputModified = true;
  return true;
}

bool d3d::setind(Sbuffer *ib)
{
  D3D_CONTRACT_ASSERT_RETURN(!ib || ib->getFlags() & SBCF_BIND_INDEX, false);

  RenderState &rs = g_render_state;
  if (rs.nextVertexInput.indexBuffer != ib)
  {
    rs.nextVertexInput.indexBuffer = (GenericBuffer *)ib; // (GenericBuffer*)buff;
    rs.modified = rs.vertexInputModified = true;
  }
  return true;
}

static inline uint32_t nprim_to_nverts(uint32_t prim_type, uint32_t numprim)
{
  // table look-up: 4 bits per entry [2b mul 2bit add]
  static const uint64_t table = (0x0ULL << (4 * PRIM_POINTLIST))          //*1+0 00/00
                                | (0x4ULL << (4 * PRIM_LINELIST))         //*2+0 01/00
                                | (0x1ULL << (4 * PRIM_LINESTRIP))        //*1+1 00/01
                                | (0x8ULL << (4 * PRIM_TRILIST))          //*3+0 10/00
                                | (0x2ULL << (4 * PRIM_TRISTRIP))         //*1+2 00/10
                                | (0x8ULL << (4 * PRIM_TRIFAN))           //*1+2 00/10
                                | (0xcULL << (4 * PRIM_4_CONTROL_POINTS)) //*4+0 11/00
    //| (0x3LL << 4*PRIM_QUADSTRIP);   //*1+3 00/11
    ;

  const uint32_t code = uint32_t((table >> (prim_type * 4)) & 0x0f);
  return numprim * ((code >> 2) + 1) + (code & 3);
}

static inline void set_primitive_type_unsafe(RenderState &rs, uint32_t prim_type)
{
  if (rs.hullTopology)
    prim_type = rs.hullTopology;
  if (rs.currPrimType != prim_type)
  {
    rs.currPrimType = prim_type;
    uint32_t result_type = prim_type;
    if (prim_type == PRIM_TRIFAN)
      result_type = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    if (prim_type == PRIM_4_CONTROL_POINTS)
      result_type = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;

    if (!validate_primitive_topology(static_cast<D3D11_PRIMITIVE_TOPOLOGY>(result_type), (rs.hdgBits & HAS_GS) != 0,
          (rs.hdgBits & (HAS_HS | HAS_DS)) != 0))
    {
#if DAGOR_DBGLEVEL > 0
      D3D_CONTRACT_ERROR("Invalid primitive topology %u for draw call encountered. Geometry stage active %s, "
                         "Hull stage active %s, "
                         "Domain stage active %s, "
                         "Hull topology %u",
        result_type, ((rs.hdgBits & HAS_GS) != 0) ? "yes" : "no", ((rs.hdgBits & HAS_HS) != 0) ? "yes" : "no",
        ((rs.hdgBits & HAS_DS) != 0) ? "yes" : "no", (unsigned)rs.hullTopology);
#endif
    }

    dx_context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)result_type);
  }
}

bool d3d::draw_base(int prim_type, int start_vertex, int numprim, uint32_t num_instances, uint32_t start_instance)
{
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERTF(prim_type != PRIM_TRIFAN, "vertex buffer can't contain tringle fan lists");
  D3D_CONTRACT_ASSERTF(rs.nextVertexShader != drv3d_generic::BAD_HANDLE, "vertex shader can't be null");
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != drv3d_generic::BAD_HANDLE);

  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  if (num_instances > 1 || start_instance > 0)
  {
    dx_context->DrawInstanced(nprim_to_nverts(prim_type, numprim), num_instances, start_vertex, start_instance);
    Stat3D::updateDrawPrim();
    Stat3D::updateTriangles(numprim * num_instances);
    Stat3D::updateInstances(num_instances);
  }
  else if (num_instances > 0)
  {
    dx_context->Draw(nprim_to_nverts(prim_type, numprim), start_vertex);
    Stat3D::updateDrawPrim();
    Stat3D::updateTriangles(numprim);
  }
  return true;
}

bool d3d::drawind_base(int prim_type, int start_index, int numprim, int base_vertex, uint32_t num_instances, uint32_t start_instance)
{
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERTF(prim_type != PRIM_TRIFAN, "vertex buffer can't contain tringle fan lists");
  D3D_CONTRACT_ASSERTF(rs.nextVertexShader != drv3d_generic::BAD_HANDLE, "vertex shader can't be null");
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != drv3d_generic::BAD_HANDLE);

  base_vertex = max(0, base_vertex);
  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  D3D_CONTRACT_ASSERT(num_instances > 0);
  if (num_instances > 1 || start_instance > 0) // may be we need to render as instanced in some other cases?
  {
    dx_context->DrawIndexedInstanced(nprim_to_nverts(prim_type, numprim), num_instances, start_index, base_vertex, start_instance);
    Stat3D::updateDrawPrim();
    Stat3D::updateTriangles(numprim * num_instances);
    Stat3D::updateInstances(num_instances);
  }
  else if (num_instances > 0)
  {
    dx_context->DrawIndexed(nprim_to_nverts(prim_type, numprim), start_index, base_vertex);
    Stat3D::updateDrawPrim();
    Stat3D::updateTriangles(numprim);
  }
  return true;
}

bool draw_up(int prim_type, int numprim, const void *ptr, int stride_bytes)
{
  CHECK_THREAD;

  RenderState &rs = g_render_state;
  G_ASSERT(rs.nextVdecl != drv3d_generic::BAD_HANDLE);
  uint32_t numVerts = nprim_to_nverts(prim_type, numprim);
  uint32_t offset;
  uint8_t *dst = g_inline_vertex_buffer->alloc(numVerts * stride_bytes, offset);
  if (dst != NULL)
  {
    if (prim_type == PRIM_TRIFAN)
    {
      uint8_t *cptr = ((uint8_t *)ptr) + stride_bytes;
      for (int i = 0; i < numprim; ++i, dst += stride_bytes * 3, cptr += stride_bytes)
      {
        memcpy(dst, ptr, stride_bytes);
        memcpy(dst + stride_bytes, cptr, stride_bytes);
        memcpy(dst + stride_bytes * 2, cptr + stride_bytes, stride_bytes);
      }
    }
    else
      memcpy(dst, ptr, numVerts * stride_bytes);
    g_inline_vertex_buffer->unlock();
  }
  else
    return false;

  GenericBuffer *oldb = rs.nextVertexInput.vertexStream[0].source;
  uint32_t oldOffset = rs.nextVertexInput.vertexStream[0].offset;
  uint32_t oldStride = rs.nextVertexInput.vertexStream[0].stride;

  d3d::setvsrc_ex(0, &g_inline_vertex_buffer->buf, offset, stride_bytes);

  if (!flush_all())
    return true;
  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  dx_context->Draw(numVerts, 0);

  d3d::setvsrc_ex(0, oldb, oldOffset, oldStride);
  Stat3D::updateDrawPrim();
  Stat3D::updateTriangles(numprim);
  return true;
}

bool d3d::draw_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  if (!args)
    return false;
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERT(rs.nextVertexShader != drv3d_generic::BAD_HANDLE);
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != BAD_HANDLE);

  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  GenericBuffer *vb = NULL;
  D3D_CONTRACT_ASSERT(args->getFlags() & SBCF_BIND_UNORDERED);
  vb = (GenericBuffer *)args;
  if (!(vb->bufFlags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("can not draw from non drawindirect buffer");
    return false;
  }
  D3D_CONTRACT_ASSERT(vb->buffer);

  dx_context->DrawInstancedIndirect(vb->buffer, byte_offset);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t byte_offset)
{
  if (!args)
    return false;
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERT(rs.nextVertexShader != drv3d_generic::BAD_HANDLE);
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != BAD_HANDLE);

  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  GenericBuffer *vb = NULL;
  D3D_CONTRACT_ASSERT(args->getFlags() & SBCF_BIND_UNORDERED);
  vb = (GenericBuffer *)args;
  if (!(vb->bufFlags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("can not draw from non drawindirect buffer");
    return false;
  }
  D3D_CONTRACT_ASSERT(vb->buffer);

  dx_context->DrawIndexedInstancedIndirect(vb->buffer, byte_offset);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::multi_draw_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  if (!args || draw_count == 0)
    return false;
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERT(rs.nextVertexShader != drv3d_generic::BAD_HANDLE);
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != BAD_HANDLE);

  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  GenericBuffer *vb = NULL;
  vb = (GenericBuffer *)args;
  if (!(vb->bufFlags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("can not draw from non drawindirect buffer");
    return false;
  }
  D3D_CONTRACT_ASSERT(vb->buffer);

#if USE_NVAPI_MULTIDRAW
  if (is_nvapi_initialized)
    NvAPI_D3D11_MultiDrawInstancedIndirect(dx_context, draw_count, vb->buffer, byte_offset, stride_bytes);
  else
#endif
    for (int i = 0; i < draw_count; ++i, byte_offset += stride_bytes)
      dx_context->DrawInstancedIndirect(vb->buffer, byte_offset);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::multi_draw_indexed_indirect(int prim_type, Sbuffer *args, uint32_t draw_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  if (!args || draw_count == 0)
    return false;
  CHECK_THREAD;

  if (!flush_all())
    return true;
  RenderState &rs = g_render_state;

  D3D_CONTRACT_ASSERT(rs.nextVertexShader != drv3d_generic::BAD_HANDLE);
  // D3D_CONTRACT_ASSERT(rs.nextPixelShader != BAD_HANDLE);

  ContextAutoLock contextLock;
  set_primitive_type_unsafe(rs, prim_type);
  GenericBuffer *vb = NULL;
  vb = (GenericBuffer *)args;
  if (!(vb->bufFlags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("can not draw from non drawindirect buffer");
    return false;
  }
  D3D_CONTRACT_ASSERT(vb->buffer);

#if USE_NVAPI_MULTIDRAW
  if (is_nvapi_initialized)
    NvAPI_D3D11_MultiDrawIndexedInstancedIndirect(dx_context, draw_count, vb->buffer, byte_offset, stride_bytes);
  else
#endif
    for (int i = 0; i < draw_count; ++i, byte_offset += stride_bytes)
      dx_context->DrawIndexedInstancedIndirect(vb->buffer, byte_offset);

  Stat3D::updateDrawPrim();
  return true;
}

bool d3d::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z, GpuPipeline gpu_pipeline)
{
  D3D_CONTRACT_ASSERT(thread_group_x <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
  D3D_CONTRACT_ASSERT(thread_group_y <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);
  D3D_CONTRACT_ASSERT(thread_group_z <= D3D11_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION);

  CHECK_THREAD;

  bool async = gpu_pipeline == GpuPipeline::ASYNC_COMPUTE && (d3d::get_driver_desc().caps.hasAsyncCompute);

  RenderState &rs = g_render_state;
  D3D_CONTRACT_ASSERT(rs.nextComputeShader != drv3d_generic::BAD_HANDLE);

  if (!flush_cs_all(true, async))
    return true;

  {
    ContextAutoLock contextLock;
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!rs.isGenericRenderPassActive || async,
      "DX11: Dispatch indirect inside a generic render pass");
    dx_context->Dispatch(thread_group_x, thread_group_y, thread_group_z);
  }

  return true;
}

bool d3d::dispatch_indirect(Sbuffer *args, uint32_t offset, GpuPipeline gpu_pipeline)
{
  if (!args)
    return false;
  CHECK_THREAD;

  bool async = gpu_pipeline == GpuPipeline::ASYNC_COMPUTE && (d3d::get_driver_desc().caps.hasAsyncCompute);

  RenderState &rs = g_render_state;
  D3D_CONTRACT_ASSERT(rs.nextComputeShader != drv3d_generic::BAD_HANDLE);
  if (!flush_cs_all(true, async))
    return true;

  D3D_CONTRACT_ASSERT(args->getFlags() & SBCF_BIND_UNORDERED);
  GenericBuffer *vb = (GenericBuffer *)args;
  D3D_CONTRACT_ASSERT(vb->buffer);
  if (!(vb->bufFlags & SBCF_MISC_DRAWINDIRECT))
  {
    D3D_CONTRACT_ERROR("can not dispatch from non drawindirect buffer");
    return false;
  }

  {
    ContextAutoLock contextLock;
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!rs.isGenericRenderPassActive || async,
      "DX11: Dispatch indirect inside a generic render pass");
    dx_context->DispatchIndirect(vb->buffer, offset);
  }

  return true;
}

void d3d::dispatch_mesh(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
  G_UNUSED(thread_group_x);
  G_UNUSED(thread_group_y);
  G_UNUSED(thread_group_z);
}

void d3d::dispatch_mesh_indirect(Sbuffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset)
{
  G_UNUSED(args);
  G_UNUSED(dispatch_count);
  G_UNUSED(stride_bytes);
  G_UNUSED(byte_offset);
}

void d3d::dispatch_mesh_indirect_count(Sbuffer *args, uint32_t args_stride_bytes, uint32_t args_byte_offset, Sbuffer *count,
  uint32_t count_byte_offset, uint32_t max_count)
{
  G_UNUSED(args);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(args_stride_bytes);
  G_UNUSED(count);
  G_UNUSED(count_byte_offset);
  G_UNUSED(max_count);
}

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline gpu_pipeline) { return BAD_GPUFENCEHANDLE; }

void d3d::insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline) {}

bool d3d::set_const_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer, uint32_t consts_offset, uint32_t consts_size)
{
  D3D_CONTRACT_ASSERT(slot < MAX_CONST_BUFFERS);
  D3D_CONTRACT_ASSERT(shader_stage < STAGE_MAX);
  if (slot >= MAX_CONST_BUFFERS || shader_stage >= STAGE_MAX)
    return false;
  if (!buffer)
  {
    if (g_render_state.constants.customBuffers[shader_stage][slot])
    {
      g_render_state.constants.customBuffers[shader_stage][slot] = NULL;
      g_render_state.constants.constantsBufferChanged[shader_stage] |= 1 << slot;
      g_render_state.modified = true;
    }
  }
  else
  {
    GenericBuffer *vb = (GenericBuffer *)buffer;
    D3D_CONTRACT_ASSERT(vb->bufFlags & SBCF_BIND_CONSTANT);
    if (!(vb->bufFlags & SBCF_BIND_CONSTANT))
      return false;

    D3D_CONTRACT_ASSERTF_RETURN(g_device_desc.caps.hasConstBufferOffset || (consts_offset == 0 && consts_size == 0), false,
      "Const buffer with offset and size doesn't support in this DX version. "
      "Check DeviceDriverCapabilities::hasConstBufferOffset.");
    D3D_CONTRACT_ASSERTF_RETURN(!(consts_offset & 15) && !(consts_size & 15), false,
      "consts_offset and consts_size should be aligned by 16");
    // debug("Slot %d; Vb bufSize %d; Vb structSize %d", slot, vb->bufSize, vb->structSize);
    consts_size = consts_size ? consts_size : 4096;
    if (g_render_state.constants.customBuffers[shader_stage][slot] != vb->buffer ||
        g_render_state.constants.buffersFirstConstants[shader_stage][slot] != consts_offset ||
        g_render_state.constants.buffersNumConstants[shader_stage][slot] != consts_size)
    {
      g_render_state.constants.customBuffers[shader_stage][slot] = vb->buffer;
      g_render_state.constants.constantsBufferChanged[shader_stage] |= 1 << slot;
      g_render_state.constants.buffersFirstConstants[shader_stage][slot] = consts_offset;
      g_render_state.constants.buffersNumConstants[shader_stage][slot] = consts_size;
      g_render_state.modified = true;
    }
    if (g_render_state.constants.customBuffersMaxSlotUsed[shader_stage] < slot)
      g_render_state.constants.customBuffersMaxSlotUsed[shader_stage] = slot;
  }
  return true;
}

bool d3d::set_buffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  if (buffer)
  {
    D3D_CONTRACT_ASSERT(buffer->getFlags() & (SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES)); // todo: remove SBCF_BIND_UNORDERED check!
#if DAGOR_DBGLEVEL > 0
                                                                                            // todo: this check to be removed
    if ((buffer->getFlags() & (SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES)) == SBCF_BIND_UNORDERED)
      D3D_CONTRACT_ERROR("buffer %s is without SBCF_BIND_SHADER_RES flag and can't be used in SRV. Deprecated, fixme!",
        buffer->getBufName());
#endif
  }

  if (slot >= MAX_RESOURCES) // these are not samplers!
  {
    D3D_CONTRACT_ERROR("invalid slot number %d", slot);
    return false;
  }

  if (buffer)
    remove_view_from_uav(((GenericBuffer *)buffer)->uav);

#ifdef ADDITIONAL_DYNAMIC_BUFFERS_VALIDATION
  if (
    buffer && (buffer->getFlags() & SBCF_DYNAMIC) && !(buffer->getFlags() & (SBCF_BIND_CONSTANT | SBCF_BIND_VERTEX | SBCF_BIND_INDEX)))
  {
    D3D_CONTRACT_ASSERTF(((GenericBuffer *)buffer)->updated, "Dynamic buffer %s is used before update this frame.",
      buffer->getBufName());
  }
#endif

  ResAutoLock resLock;
  TextureFetchState::Resources &res = g_render_state.texFetchState.resources[shader_stage];
  res.resources.resize(max(res.resources.size(), slot + 1));
  if (res.resources[slot].setBuffer((GenericBuffer *)buffer))
  {
    res.modifiedMask |= 1 << slot;
    g_render_state.modified = true;
  }

  if (shader_stage == STAGE_CS && d3d::get_driver_desc().caps.hasAsyncCompute)
  {
    TextureFetchState::Resources &res = g_render_state.texFetchState.resources[STAGE_CS_ASYNC_STATE];
    res.resources.resize(max(res.resources.size(), slot + 1));
    if (res.resources[slot].setBuffer((GenericBuffer *)buffer))
    {
      res.modifiedMask |= 1 << slot;
      g_render_state.modified = true;
    }
  }

  return true;
}

bool d3d::set_rwbuffer(unsigned shader_stage, unsigned slot, Sbuffer *buffer)
{
  if (featureLevelsSupported < D3D_FEATURE_LEVEL_11_1 && shader_stage != STAGE_CS && shader_stage != STAGE_PS)
  {
    D3D_CONTRACT_ERROR("currently unsupported set buffers to other stages than Cs and Ps");
    return false;
  }
  if (shader_stage == STAGE_VS)
    shader_stage = STAGE_PS;
  GenericBuffer *vb = NULL;
  if (buffer)
  {
    D3D_CONTRACT_ASSERT(buffer->getFlags() & (SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES)); // todo: remove SBCF_BIND_SHADER_RES check!
#if DAGOR_DBGLEVEL > 0
                                                                                            // todo: this check to be removed
    if ((buffer->getFlags() & (SBCF_BIND_UNORDERED | SBCF_BIND_SHADER_RES)) == SBCF_BIND_SHADER_RES)
      D3D_CONTRACT_ERROR("buffer %s is without SBCF_BIND_UNORDERED flag and can't be used in UAV. Deprecated, fixme!",
        buffer->getBufName());
#endif
    remove_buffer_from_slot(buffer);
    vb = (GenericBuffer *)buffer;
    G_ASSERT(vb->uav);
    remove_view_from_uav_ignore_slot(shader_stage, slot, vb->uav);
  }

  if (vb)
    g_render_state.texFetchState.uavState[shader_stage].uavsUsed = true;
  if (g_render_state.texFetchState.setUav(shader_stage, slot, vb ? vb->uav : NULL))
  {
    g_render_state.texFetchState.uavState[shader_stage].uavModifiedMask |= 1 << slot;
    g_render_state.modified = true;
  }
  return true;
}
