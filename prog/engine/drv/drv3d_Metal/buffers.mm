// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix.h>
#include "render.h"
#include "drv_log_defs.h"

#include <osApiWrappers/dag_miscApi.h>

#include "pools.h"

static inline int DivideRoundingUp(int i, int j)
{
  return (i + (j - 1)) / j;
}

static int g_vb_memory_used = 0;
static int g_ib_memory_used = 0;
static int g_buffer_memory_used = 0;

int get_ib_vb_mem_used(String *stat, int &sysmem)
{
  sysmem = 0;
  if (stat)
  {
    stat->aprintf(128, "%5dK inline vertex buffer\n", g_vb_memory_used >> 10);
    stat->aprintf(128, "%5dK inline index buffer\n", g_ib_memory_used >> 10);
  }
  return g_vb_memory_used + g_ib_memory_used + g_buffer_memory_used;
}

namespace drv3d_metal
{

  typedef drv3d_generic::ObjectProxyPtr<drv3d_metal::Buffer::BufTex> BufferProxyPtr;

  static drv3d_generic::ObjectPoolWithLock<BufferProxyPtr> g_buffers("buffers");
  void clear_buffers_pool_garbage() { g_buffers.clearGarbage(); }

  void Buffer::BufTex::create(MTLResourceOptions storage, MTLTextureDescriptor* pTexDesc,
                              int aligment, int tex_format, const char* name)
  {
    String buf_name;
    buf_name.printf(0, "%s (%d)", name ? name : "(null)", render.frame);
    buffer = render.createBuffer(bufsize, storage, buf_name);

    if (flags & SBCF_BIND_VERTEX)
      g_vb_memory_used += bufsize;
    else if (flags & SBCF_BIND_INDEX)
      g_ib_memory_used += bufsize;
    else
      g_buffer_memory_used += bufsize;

    if (flags & SBCF_ZEROMEM)
      render.clearBuffer(this);
    else
      initialized = true;

    if (pTexDesc)
    {
      id<MTLTexture> tex = [buffer newTextureWithDescriptor : pTexDesc
                                                     offset : 0
                                                bytesPerRow : aligment];
      texture = new Texture(tex, tex_format, name);
    }
    else
    {
      texture = nullptr;
    }
    TEXQL_ON_BUF_ALLOC(this);

    BufferProxyPtr e;
    e.obj = this;
    this->tid = g_buffers.safeAllocAndSet(e);
  }

  void Buffer::BufTex::release()
  {
    if (texture)
    {
      texture->release();
    }

    if (buffer)
    {
      [buffer release];

      if (flags & SBCF_BIND_VERTEX)
        g_vb_memory_used -= bufsize;
      else if (flags & SBCF_BIND_INDEX)
        g_ib_memory_used -= bufsize;
      else
        g_buffer_memory_used -= bufsize;
    }
    TEXQL_ON_BUF_RELEASE(this);

    g_buffers.lock();
    if (g_buffers.isIndexValid(tid))
      g_buffers.safeReleaseEntry(tid);
    g_buffers.unlock();
    tid = drv3d_generic::BAD_HANDLE;
  }

  Buffer::Buffer()
  {
  }

  Buffer::Buffer(uint32_t bsize, int ssize,int buf_flags, int set_tex_format, const char* name)
  {
    setResName(name);

    if (buf_flags & SBCF_BIND_CONSTANT || ((buf_flags & SBCF_DYNAMIC) && set_tex_format))
    {
      allow_resize_ring_buffer = true;
    }

    bufSize = 0;
    bufFlags = 0;
    structSize = buf_flags & SBCF_MISC_ALLOW_RAW ? 4 : ssize;

    locked_offset = -1;
    locked_size = -1;

    bufSize = ((ssize > 0 ? bsize * ssize : bsize) + 3) & ~3;
    bufFlags = buf_flags;

    G_ASSERTF(set_tex_format == 0 || ((bufFlags&(SBCF_BIND_VERTEX|SBCF_BIND_INDEX|SBCF_MISC_STRUCTURED|SBCF_MISC_ALLOW_RAW)) == 0),
            "can't create buffer with texture format which is Structured, Vertex, Index or Raw");
    tex_format = set_tex_format;

    if (tex_format != 0)
    {
      MTLPixelFormat metal_format = Texture::format2Metal(tex_format);

      aligment = [render.device minimumLinearTextureAlignmentForPixelFormat:metal_format];
      bufSize = fmax(bufSize, aligment);

      int widBytes, nBytes;
      Texture::getStride(tex_format, 1, 1, 0, widBytes, nBytes);
      int w = bufSize / widBytes;
      int h = 1;

      if (w > 4096)
      {
        w = 4096;
        h = DivideRoundingUp(bufSize, 4096 * widBytes);
        aligment = w * widBytes;
        bufSize = h * aligment;
      }
      else
      {
        if (bufSize % aligment != 0)
        {
          bufSize = DivideRoundingUp(bufSize, aligment) * aligment;
        }

        aligment = bufSize;
      }

      pTexDesc = [[MTLTextureDescriptor texture2DDescriptorWithPixelFormat : metal_format
                                                                    width : w
                                                                   height : h
                                                                mipmapped : NO] retain];

      pTexDesc.usage = MTLTextureUsageShaderRead;
      if (bufFlags & SBCF_BIND_UNORDERED)
        pTexDesc.usage |= MTLTextureUsageShaderWrite;
    }

    checkLockParams(0, bufSize, VBLOCK_DISCARD, bufFlags);
    isDynamic = bufFlags & SBCF_DYNAMIC;
    fast_discard = bufFlags & SBCF_FRAMEMEM;

    max_buffers = Render::MAX_FRAMES_TO_RENDER;

    use_upload_buffer = (!(buf_flags & SBCF_USAGE_READ_BACK) && !isDynamic) || pTexDesc;
    // hack, staging buffer
    if ((bufFlags & (SBCF_CPU_ACCESS_READ | SBCF_CPU_ACCESS_WRITE)) == (SBCF_CPU_ACCESS_READ | SBCF_CPU_ACCESS_WRITE))
    {
      fast_discard = true;
      use_upload_buffer = false;
    }
    if (set_tex_format)
      fast_discard = false;

    storage = !use_upload_buffer ? MTLResourceStorageModeShared : MTLResourceStorageModePrivate;
    if (buf_flags & SBCF_USAGE_READ_BACK)
      storage = MTLResourceStorageModeShared;
    if (pTexDesc)
      pTexDesc.resourceOptions = storage;

    if (bufFlags & SBCF_ZEROMEM)
      G_ASSERT(fast_discard == false);

    if (isDynamic && !fast_discard)
    {
      BufTex* prev_buf = nullptr;

      for (int i = 0; i < max_buffers; i++)
      {
        BufTex* buf = new BufTex(bufSize, bufFlags);
        buf->index = i;
        buf->create(storage, pTexDesc, aligment, tex_format, name);

        if (i==0)
        {
          ring_buffer = buf;
        }
        else
        {
          prev_buf->next = buf;
        }

        prev_buf = buf;
      }

      prev_buf->next = ring_buffer;

      cur_buffer = prev_buf;
      cur_render_buffer = prev_buf;
    }
    else if (fast_discard)
    {
      cur_buffer = nullptr;
      cur_render_buffer = nullptr;
    }
    else
    {
      cur_buffer = new BufTex(bufSize, bufFlags);
      cur_buffer->create(storage, pTexDesc, aligment, tex_format, name);
      cur_render_buffer = cur_buffer;
    }
  }

  Buffer::~Buffer()
  {
    if (pTexDesc)
      [pTexDesc release];
  }

  int Buffer::lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags)
  {
    checkLockParams(ofs_bytes, size_bytes, flags, bufFlags);
    if (!ptr)
    {
      last_locked_submit = render.submits_scheduled;
      return 1;
    }
    else
    {
      *ptr = lock(ofs_bytes, size_bytes, flags);
      return *ptr ? 1 : 0;
    }
  }

  void* Buffer::lock(uint32_t offset_bytes, uint32_t size_bytes, int flags)
  {
    locked_offset = offset_bytes;
    locked_size = size_bytes;

    G_ASSERT(size_bytes <= bufSize);
    if (size_bytes == 0)
    {
      locked_size = bufSize - offset_bytes;
    }

    if (fast_discard)
    {
      G_ASSERT(offset_bytes == 0);
#if DAGOR_DBGLEVEL > 0
      if (render.validate_framemem_bounds)
      {
        uint32_t s = bufFlags & SBCF_BIND_CONSTANT ? bufSize : locked_size;
        dynamic_buffer = [render.device newBufferWithLength : s
                                                    options : MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared];
        dynamic_offset = 0;
        render.queueBufferForDeletion(dynamic_buffer);
      }
      else
#endif
        dynamic_buffer = render.AllocateDynamicBuffer(locked_size, dynamic_offset, bufSize);
      G_ASSERT(dynamic_buffer);
      dynamic_frame = render.frame;

      if (render.acquire_depth.load())
        render.markBufferDirty(this);

      cur_render_buffer = cur_buffer;

      uint8_t *ret = (uint8_t*)dynamic_buffer.contents + dynamic_offset + offset_bytes;
      return ret;
    }

    if (isDynamic)
    {
      if (flags & VBLOCK_DISCARD)
      {
        G_ASSERT(cur_buffer);

        BufTex* prev_buffer = cur_buffer;
        cur_buffer = cur_buffer->next;
        if (allow_resize_ring_buffer && render.submits_completed < cur_buffer->last_submit)
        {
          #if DAGOR_DBGLEVEL > 0
          if (!pTexDesc) // Currently SBCF_FRAMEMEM + tex_format treated as allow_resize_ring_buffer
            D3D_ERROR("METAL reallocating dynamic buffer %s, consider using SBCF_FRAMEMEM",
                [ring_buffer->buffer.label UTF8String]);
          #endif

          max_buffers++;

          BufTex* buf = new BufTex(bufSize, bufFlags);
          buf->create(storage, pTexDesc, aligment, tex_format, "");
          buf->index = max_buffers - 1;

          prev_buffer->next = buf;
          buf->next = cur_buffer;

          cur_buffer = buf;

          if (render.acquire_depth.load())
            render.markBufferDirty(this);
        }
        else if (render.submits_completed < cur_buffer->last_submit)
        {
          #if DAGOR_DBGLEVEL > 0
          D3D_ERROR("METAL locking dynamic buffer %s with VBLOCK_DISCARD multiple times during last %d frames",
              [ring_buffer->buffer.label UTF8String], max_buffers);
          #endif
          render.acquireOwnerShip();
          render.flush(true);
          render.releaseOwnerShip();
        }
        else if (render.acquire_depth.load())
          render.markBufferDirty(this);
      }

      cur_buffer->last_submit = render.submits_scheduled;
      cur_render_buffer = cur_buffer;
    }

    G_ASSERT(!use_upload_buffer || !(flags & VBLOCK_READONLY) || (bufFlags & SBCF_USAGE_READ_BACK));
    if ((flags & VBLOCK_READONLY) && (last_locked_submit == ~0ull || (render.submits_completed < last_locked_submit)))
    {
#if DAGOR_DBGLEVEL > 0
      if (render.report_stalls)
        D3D_ERROR("[METAL_BUFFER] flushing to readback buffer %s, frame %llu (%llu)", getResName(), render.frame, render.submits_completed);
#endif
      render.acquireOwnerShip();
      @autoreleasepool
      {
        render.flush(true);
      }
      last_locked_submit = render.submits_scheduled;
      render.releaseOwnerShip();
    }

    if (use_upload_buffer && !(flags & VBLOCK_READONLY))
    {
      locked_size = (locked_size + 3) & ~3;
      @autoreleasepool
      {
        uint64_t cur_thread = 0;
        pthread_threadid_np(NULL, &cur_thread);

        bool from_thread = render.acquire_depth == 0 || cur_thread != render.cur_thread;
        if (from_thread || locked_size > 256 * 1024)
        {
          String name;
          name.printf(0, "upload for %s %llu", getResName(), render.frame);
          upload_buffer = render.createBuffer(locked_size, MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared, name);
          upload_buffer_offset = 0;
        }
        else
          upload_buffer = [render.AllocateDynamicBuffer(locked_size, upload_buffer_offset, locked_size) retain];
      }
      G_ASSERT((upload_buffer_offset & 3) == 0);
      G_ASSERT((offset_bytes & 3) == 0);
      G_ASSERT((locked_size & 3) == 0);

      return (void*)((uint8_t*)[upload_buffer contents] + upload_buffer_offset);
    }

    return (void*)((uint8_t*)[cur_buffer->buffer contents] + offset_bytes);
  }

  int Buffer::getDynamicOffset() const
  {
    G_ASSERT(cur_render_buffer || fast_discard);
    return dynamic_buffer ? dynamic_offset : 0;
  }

  int Buffer::unlock()
  {
    if (locked_offset == -1)
    {
      return 1;
    }

    if (use_upload_buffer && upload_buffer)
    {
      G_ASSERT(upload_buffer);
      G_ASSERT(cur_buffer->initialized);
      render.queueCopyBuffer(upload_buffer, upload_buffer_offset, cur_buffer->buffer, locked_offset, locked_size);
      render.queueBufferForDeletion(upload_buffer);
      upload_buffer = nil;
      upload_buffer_offset = 0;
    }

    locked_offset = -1;
    locked_size = -1;

    return 1;
  }

  bool Buffer::updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags)
  {
    G_ASSERT_RETURN(size_bytes != 0, false);

    uint64_t cur_thread = 0;
    pthread_threadid_np(NULL, &cur_thread);

    bool from_thread = render.acquire_depth == 0 || cur_thread != render.cur_thread;
    if ((lockFlags & (VBLOCK_DISCARD | VBLOCK_NOOVERWRITE)) || isDynamic || fast_discard || from_thread)
    {
      // in those cases we need to use locking to keep everything consistent
      return updateDataWithLock(ofs_bytes, size_bytes, src, lockFlags);
    }

    G_ASSERT(dynamic_offset == 0);
    G_ASSERT(ofs_bytes + size_bytes <= bufSize);

    id<MTLBuffer> tempBuffer = nil;
    int tempOffset = 0;
    @autoreleasepool
    {
      if (size_bytes > 128 * 1024)
      {
        String name;
        name.printf(0, "upload for %s %llu", getResName(), render.frame);
        tempBuffer = render.createBuffer(size_bytes, MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared, name);
      }
      else
        tempBuffer = [render.AllocateDynamicBuffer(size_bytes, tempOffset) retain];
    }

    G_ASSERT(tempBuffer);
    memcpy((uint8_t*)[tempBuffer contents] + tempOffset, src, size_bytes);

    render.queueUpdateBuffer(tempBuffer, tempOffset, cur_buffer->buffer, ofs_bytes, size_bytes);
    render.queueBufferForDeletion(tempBuffer);

    return true;
  }

  bool Buffer::copyTo(Sbuffer * dest)
  {
    render.copyBuffer(this, 0, dest, 0, 0);
    return true;
  }

  bool Buffer::copyTo(Sbuffer * dest, uint32_t dst_ofs_bytes, uint32_t src_ofs_bytes, uint32_t size_bytes)
  {
    render.acquireOwnerShip();
    render.copyBuffer(this, src_ofs_bytes, dest, dst_ofs_bytes, size_bytes);
    render.releaseOwnerShip();
    return true;
  }

  id<MTLBuffer> Buffer::getBuffer()
  {
    G_ASSERTF(dynamic_buffer || cur_render_buffer, "probably <%s> fastdiscard buffer is used and not updated this frame", getResName());
    if (dynamic_buffer && dynamic_frame != render.frame)
    {
#if DAGOR_DBGLEVEL > 0
      D3D_ERROR("METAL setting buffer <%s> which has SBCF_FRAMEMEM and wasn't locked this frame",
               getResName());
#endif
      return nullptr;
    }
    return dynamic_buffer ? dynamic_buffer : cur_render_buffer->buffer;
  }

  Texture* Buffer::getTexture()
  {
    return cur_render_buffer ? cur_render_buffer->texture : nullptr;
  }

  void Buffer::destroy()
  {
    render.deleteBuffer(this);
  }

  void Buffer::release()
  {
    if (!isDynamic || fast_discard)
    {
      if (cur_buffer)
      {
        cur_buffer->release();
        delete cur_buffer;
      }
    }
    else
    {
      BufTex* buf = ring_buffer;
      for (int i = 0; i < max_buffers; i++)
      {
        BufTex* prev_buf = buf;
        buf = buf->next;

        prev_buf->release();
        delete prev_buf;
      }
    }
    delete this;
  }

  void Buffer::setResApiName(const char *name) const
  {
#if DAGOR_DBGLEVEL > 0
    if (ring_buffer == nullptr)
      return;
    BufTex* buf = ring_buffer;
    for (int i = 0; i < max_buffers; i++)
    {
      BufTex* prev_buf = buf;
      buf = buf->next;
      if (prev_buf->buffer)
        prev_buf->buffer.label = [NSString stringWithUTF8String : name];
    }
#endif
  }

  void Buffer::cleanup()
  {
    Tab<Buffer::BufTex*> buffers;
    g_buffers.lock();
    ITERATE_OVER_OBJECT_POOL(g_buffers, textureNo)
      if (g_buffers[textureNo].obj != NULL)
        buffers.push_back(g_buffers[textureNo].obj);
    ITERATE_OVER_OBJECT_POOL_RESTORE(g_buffers);
    g_buffers.unlock();
    for (int i = 0; i < buffers.size(); ++i)
      buffers[i]->release();
  }
}
