#pragma once
#include "common.h"
#include <3d/dag_drv3d.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_sbufferIDHolder.h>
#include <util/dag_generationReferencedData.h>
#include <3d/dag_gpuEventRingBuffer.h>

namespace dafx
{
struct Context;
using allocator_type = MidmemAlloc;

template <typename T>
struct CpuResourceDeleter
{
  void operator()(T *p)
  {
    if (p)
      allocator_type::free(p);
  }
};

using GpuResourcePtr = SbufferIDHolderWithVar;
using CpuResourcePtr = eastl::unique_ptr<unsigned char, CpuResourceDeleter<unsigned char>>;

using RefData = eastl::vector<unsigned char>;
using RefDataId = GenerationRefId<8, RefData>;

struct StagingRing
{
  StagingRing() : size(0), buffer(), ring(0), ofs(GPUEventRingBuffer::invalid_offset) {}

  unsigned int size;
  GPUEventRingBuffer::offset_type_t ofs;
  GpuResourcePtr buffer;
  GPUEventRingBuffer ring;
};

struct RenderBuffer
{
  int allocSize = 0;
  int usageSize = 0;
  int maxUsage = 0;
  GpuResourcePtr res;
};

template <typename Tx, typename Ty>
struct BufferPool
{
  using res_t = Tx;
  using element_t = Ty;

  struct Page
  {
    res_t res;
    unsigned int size;
    unsigned int garbage;
    unsigned int available;
  };
  using PageId = GenerationRefId<8, Page>;

  struct Buffer
  {
    Buffer() : pageId(PageId()), directPtr(nullptr), resId(BAD_D3DRESID), offset(0), size(0) {}
    PageId pageId;
    element_t *directPtr;
    D3DRESID resId;
    int offset;
    int size;
  };

  int pageSize;
  GenerationReferencedData<PageId, Page> pages;
};

using GpuBufferPool = BufferPool<GpuResourcePtr, Sbuffer>;
using GpuPage = GpuBufferPool::Page;
using GpuPageId = GpuBufferPool::PageId;
using GpuBuffer = GpuBufferPool::Buffer;

using CpuBufferPool = BufferPool<CpuResourcePtr, unsigned char>;
using CpuPage = CpuBufferPool::Page;
using CpuPageId = CpuBufferPool::PageId;
using CpuBuffer = CpuBufferPool::Buffer;

template <typename T>
bool init_buffer_pool(T &dst, size_t page_sz)
{
  G_ASSERT_RETURN(page_sz % DAFX_ELEM_STRIDE == 0, false);
  dst.pageSize = page_sz;
  return true;
}

bool create_gpu_buffer(GpuBufferPool &dst, unsigned int sz, GpuBuffer &out);
bool create_cpu_buffer(CpuBufferPool &dst, unsigned int sz, CpuBuffer &out);
void release_gpu_buffer(GpuBufferPool &dst, GpuBuffer &buf);
void release_cpu_buffer(CpuBufferPool &dst, CpuBuffer &buf);

bool create_gpu_rb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name);
bool create_gpu_sb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name);
bool create_gpu_cb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name);

bool create_cpu_res(CpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name);

bool update_gpu_cb_buffer(Sbuffer *buf, const void *src, unsigned int src_size);

unsigned char *start_updating_render_buffer(Context &ctx, int tag);
void stop_updating_render_buffer(RenderBuffer &buf);

unsigned char *start_updating_staging(Context &ctx, int size);
void stop_updating_staging(Context &ctx);

inline void validate_cpu_buffer(const CpuBuffer &buf, CpuBufferPool &buf_pool)
{
  CpuPage *page = buf_pool.pages.get(buf.pageId);
  G_FAST_ASSERT(page && page->res.get() == buf.directPtr);
  G_UNREFERENCED(page);
}

inline void validate_gpu_buffer(const GpuBuffer &buf, GpuBufferPool &buf_pool)
{
  GpuPage *page = buf_pool.pages.get(buf.pageId);
  G_FAST_ASSERT(page && page->res.getBuf() == buf.directPtr);
  G_UNREFERENCED(page);
}

void update_buffers_stats(Stats &stats, GpuBufferPool &gpu_pool, CpuBufferPool &cpu_pool,
  eastl::array<RenderBuffer, Config::max_render_tags> &ren_bufs, StagingRing &staging);
} // namespace dafx
