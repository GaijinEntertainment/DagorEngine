#include "buffers.h"
#include "context.h"

namespace dafx
{
bool create_gpu_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, uint32_t flags, uint fmt, const char *name)
{
  G_ASSERT_RETURN(elem_size % DAFX_ELEM_STRIDE == 0, false);
  DBG_OPT("create_gpu_res, elem_size:%d, elem_count:%d", elem_size, elem_count);

  Sbuffer *ptr = d3d::create_sbuffer(elem_size, elem_count, flags, fmt, name);

  if (!ptr)
  {
    logerr("dafx: create_gpu_res failed, name: %s", name);
    return false;
  }

  G_ASSERT(get_managed_res_id(name) == BAD_D3DRESID);
  D3DRESID id = register_managed_res(name, ptr);
  if (!id)
  {
    logerr("dafx: register_managed_res failed, name: %s", name);
    return false;
  }

  res.set(ptr, id);

  return true;
}

bool create_gpu_rb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name)
{
  return create_gpu_res(res, elem_size, elem_count, SBCF_UA_STRUCTURED_READBACK, 0, name);
}

bool create_gpu_sb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name)
{
  return create_gpu_res(res, elem_size, elem_count, SBCF_UA_SR_STRUCTURED, 0, name);
}

bool create_gpu_cb_res(GpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *name)
{
  return create_gpu_res(res, elem_size, elem_count, SBCF_CB_ONE_FRAME, 0, name);
}

bool create_cpu_res(CpuResourcePtr &res, unsigned int elem_size, unsigned int elem_count, const char *)
{
  G_ASSERT_RETURN(elem_size % DAFX_ELEM_STRIDE == 0, false);
  DBG_OPT("create_cpu_res, elem_size:%d, elem_count:%d", elem_size, elem_count);

  unsigned char *ptr = (unsigned char *)allocator_type::alloc(elem_size * elem_count);
  res.reset(ptr);
  G_ASSERT_RETURN(res, false);
  return true;
}

template <typename T, typename F>
typename T::Page *create_gen_buffer(T &dst, unsigned int sz, typename T::Buffer &out, F create_cb)
{
  G_ASSERT_RETURN(sz > 0, nullptr);
  sz = get_aligned_size(sz);
  G_ASSERT_RETURN(sz <= dst.pageSize, nullptr);

  typename T::PageId cid = typename T::PageId();
  for (int i = dst.pages.totalSize() - 1; i >= 0; --i)
  {
    const typename T::Page *page = dst.pages.getByIdx(i);
    if (page && page->available >= sz)
    {
      cid = dst.pages.createReferenceFromIdx(i);
      break;
    }
  }

  if (!cid)
  {
    eastl::string name;
    static int bufId = 0;
    name.append_sprintf("dafx_main_buffer_%d_%d", bufId++, dst.pages.totalSize());

    cid = dst.pages.emplaceOne();
    typename T::Page *page = dst.pages.get(cid);
    G_ASSERT_RETURN(page, nullptr);

    if (!create_cb(page->res, DAFX_ELEM_STRIDE, dst.pageSize / DAFX_ELEM_STRIDE, name.c_str()))
      return nullptr;

    page->size = dst.pageSize;
    page->available = dst.pageSize;
    page->garbage = 0;

    DBG_OPT("dafx: data buffer append, %s", name.c_str());
  }

  typename T::Page *page = dst.pages.get(cid);
  G_ASSERT_RETURN(page, nullptr);

  unsigned int offset = page->size - page->available;
  page->available -= sz;

  out.pageId = cid;
  out.size = sz;
  out.offset = offset;

  return page;
}

bool create_gpu_buffer(GpuBufferPool &dst, unsigned int sz, GpuBuffer &out)
{
  GpuPage *page = create_gen_buffer(dst, sz, out, create_gpu_sb_res);
  out.directPtr = page ? page->res.getBuf() : nullptr;
  out.resId = page ? page->res.getId() : BAD_D3DRESID;
  return page;
}

bool create_cpu_buffer(CpuBufferPool &dst, unsigned int sz, CpuBuffer &out)
{
  CpuPage *page = create_gen_buffer(dst, sz, out, create_cpu_res);
  out.directPtr = page ? page->res.get() : nullptr;
  return page;
}

template <typename T>
void release_buffer(T &dst, typename T::Buffer &buf)
{
  G_ASSERT_RETURN(buf.size > 0, );

  typename T::Page *page = dst.pages.get(buf.pageId);
  G_ASSERT_RETURN(page, );

  page->garbage += buf.size;
  buf.directPtr = nullptr;
  buf.size = 0;
  buf.offset = 0;

  G_ASSERT_RETURN(page->garbage <= page->size, );
  if (page->garbage == page->size - page->available)
  {
    dst.pages.destroyReference(buf.pageId);
    DBG_OPT("free page");
  }

  buf.pageId = typename T::PageId();
}

void release_gpu_buffer(GpuBufferPool &dst, GpuBuffer &buf) { release_buffer(dst, buf); }

void release_cpu_buffer(CpuBufferPool &dst, CpuBuffer &buf) { release_buffer(dst, buf); }

bool update_gpu_cb_buffer(Sbuffer *res, const void *src, unsigned int src_size)
{
  G_FAST_ASSERT(src_size % DAFX_ELEM_STRIDE == 0);
  G_FAST_ASSERT(res->getNumElements() >= src_size / res->getElementSize());

  bool r = res->updateDataWithLock(0, src_size, src, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  if (!r)
  {
    logmessage(DAGOR_DBGLEVEL > 0 ? LOGLEVEL_ERR : LOGLEVEL_WARN,
      "dafx: update_gpu_cb_buffer failed, src_size:%d, dst_elem_size:%d, dst_elem_count:%d", src_size, res->getElementSize(),
      res->getNumElements());
  }

  return r;
}

unsigned char *start_updating_render_buffer(Context &ctx, int tag)
{
  RenderBuffer &buf = ctx.renderBuffers[tag];
  G_FAST_ASSERT(buf.allocSize % DAFX_ELEM_STRIDE == 0);
  G_FAST_ASSERT(buf.usageSize % DAFX_ELEM_STRIDE == 0);

  buf.maxUsage = max(buf.maxUsage, buf.usageSize);

  bool canShrink = false;
  if ((ctx.currentFrame % ctx.cfg.render_buffer_gc_tail) == 0 && buf.allocSize > ctx.cfg.render_buffer_gc_after)
  {
    canShrink = (float(buf.maxUsage) / float(buf.allocSize)) < ctx.cfg.render_buffer_shrink_ratio;
    buf.maxUsage = 0;
  }

  bool recreate = false;
  if (buf.usageSize > buf.allocSize) // grow
  {
    recreate = true;
    buf.allocSize += (buf.usageSize - buf.allocSize) * 2;
    buf.allocSize = max(buf.allocSize, ctx.cfg.render_buffer_start_size);
  }
  else if (canShrink) // shrink
  {
    recreate = true;
    buf.allocSize = max(buf.usageSize, ctx.cfg.render_buffer_gc_after);
  }

  G_ASSERT(buf.allocSize >= buf.usageSize);

  // workaound for DX12 spam D3DD Active texture buffer <dafx_cpu_render_buffer> (stage 2, slot 21) deleted...
  if (recreate && buf.res.getBuf())
  {
    const Driver3dDesc &desc = d3d::get_driver_desc();
    for (int i = 0; i < desc.maxsimtex; ++i)
    {
      d3d::set_buffer(STAGE_VS, i, nullptr);
      d3d::set_buffer(STAGE_PS, i, nullptr);
    }
    ShaderElement::invalidate_cached_state_block();
  }

  if (recreate)
  {
    uint flags = SBCF_DYNAMIC | SBCF_MAYBELOST | SBCF_CPU_ACCESS_WRITE | SBCF_BIND_SHADER_RES;
    uint fmt = TEXFMT_R32UI;

    if (ctx.cfg.use_render_sbuffer)
    {
      flags |= SBCF_MISC_STRUCTURED;
      fmt = 0;
    }

    buf.res.close();
    bool r = create_gpu_res(buf.res, DAFX_ELEM_STRIDE, buf.allocSize / DAFX_ELEM_STRIDE, flags, fmt,
      String(32, "dafx_cpu_render_buffer_tag%d", tag));
    if (!r)
    {
      buf.usageSize = 0;
      buf.allocSize = 0;
      return nullptr;
    }

    DBG_OPT("recreate render buffer tag: %d, new size: %d", tag, buf.allocSize);
  }

  unsigned char *data = nullptr;
  Sbuffer *res = buf.res.getBuf();
  bool r = res && res->lockEx(0, buf.usageSize, &data, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
  return (r && data) ? data : nullptr;
}

void stop_updating_render_buffer(RenderBuffer &buf)
{
  G_ASSERT_RETURN(buf.res.getBuf(), );
  buf.res.getBuf()->unlock();
}

unsigned char *start_updating_staging(Context &ctx, int size)
{
  G_ASSERT(size % DAFX_ELEM_STRIDE == 0);

  if (!ctx.supportsNoOverwrite)
    ctx.staging.ring.reset(ctx.staging.size);

  ctx.staging.ofs = ctx.staging.ring.allocate(size, DAFX_ELEM_STRIDE);
  if (ctx.staging.ofs == GPUEventRingBuffer::invalid_offset)
  {
    ctx.staging.buffer.close();
    ctx.staging.size = max(ctx.cfg.staging_buffer_size, ctx.staging.size + size);
    ctx.staging.ring.reset(ctx.staging.size);

    bool r = create_gpu_res(ctx.staging.buffer, DAFX_ELEM_STRIDE, ctx.staging.size / DAFX_ELEM_STRIDE,
      SBCF_DYNAMIC | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MAYBELOST, 0, "dafx_staging");
    debug("dafx staging reset: %d | %d", ctx.staging.size, r);

    if (!r)
    {
      ctx.staging.size = 0;
      ctx.staging.ring.reset(0);
      return nullptr;
    }

    ctx.staging.ofs = ctx.staging.ring.allocate(size, DAFX_ELEM_STRIDE);
    G_ASSERT(ctx.staging.ofs != GPUEventRingBuffer::invalid_offset);
  }

  unsigned char *data = nullptr;
  bool r = ctx.staging.buffer.getBuf()->lockEx(ctx.staging.ofs, size, &data,
    VBLOCK_WRITEONLY | VBLOCK_NOSYSLOCK | (ctx.supportsNoOverwrite ? VBLOCK_NOOVERWRITE : VBLOCK_DISCARD));

  return (r && data) ? data : nullptr;
}

void stop_updating_staging(Context &ctx)
{
  ctx.staging.buffer.getBuf()->unlock();

  ctx.staging.ring.releaseCompletedFrames();
  ctx.staging.ring.finishCurrentFrame();
}

template <typename T>
void get_pool_stats(T &pool, int &page_count, int &alloc_size, int &usage_size, int &garbage_size)
{
  page_count = 0;
  alloc_size = 0;
  usage_size = 0;
  garbage_size = 0;

  for (int i = 0; i < pool.pages.totalSize(); ++i)
  {
    const typename T::Page *page = pool.pages.getByIdx(i);
    if (!page)
      continue;

    page_count++;
    alloc_size += page->size;
    usage_size += page->size - page->available - page->garbage;
    garbage_size += page->garbage;
  }
}

void update_buffers_stats(Stats &stats, GpuBufferPool &gpu_pool, CpuBufferPool &cpu_pool,
  eastl::array<RenderBuffer, Config::max_render_tags> &ren_bufs, StagingRing &staging)
{
  stats.totalRenderBuffersAllocSize = 0;
  stats.totalRenderBuffersUsageSize = 0;
  for (int i = 0; i < Config::max_render_tags; ++i)
  {
    stats.totalRenderBuffersAllocSize += ren_bufs[i].allocSize;
    stats.totalRenderBuffersUsageSize += ren_bufs[i].usageSize;
  }

  stats.stagingSize = staging.size;

  get_pool_stats(cpu_pool, stats.cpuBufferPageCount, stats.cpuBufferAllocSize, stats.cpuBufferUsageSize, stats.cpuBufferGarbageSize);

  get_pool_stats(gpu_pool, stats.gpuBufferPageCount, stats.gpuBufferAllocSize, stats.gpuBufferUsageSize, stats.gpuBufferGarbageSize);
}

} // namespace dafx
