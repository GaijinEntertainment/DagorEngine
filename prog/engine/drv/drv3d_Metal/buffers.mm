
#include <math/dag_TMatrix.h>
#include "render.h"

#include <osApiWrappers/dag_miscApi.h>

#include "pools.h"

#define HAS_SMALL_BUFFER_CACHE 0

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

#if HAS_SMALL_BUFFER_CACHE
  struct BufCache
  {
    static const uint32_t max_size = 32 * 1024;

    std::mutex mutex;
    id<MTLHeap> heap_shared = nil;
    id<MTLHeap> heap_private = nil;

    id<MTLBuffer> allocate(MTLResourceOptions storage, uint32_t size)
    {
      std::lock_guard<std::mutex> scopedLock(mutex);

      id<MTLHeap> & heap = storage == MTLResourceStorageModeShared ? heap_shared : heap_private;
      if (heap == nil)
      {
        // todo: make setting
        int cache_size = storage == MTLResourceStorageModeShared ? 1*1024*1024 : 7*1024*1024;
        MTLHeapDescriptor* heapDescriptor = [MTLHeapDescriptor new];
        heapDescriptor.cpuCacheMode = MTLCPUCacheModeDefaultCache;
        heapDescriptor.storageMode = storage == MTLResourceStorageModeShared ? MTLStorageModeShared : MTLStorageModePrivate;
        heapDescriptor.size = cache_size;
        heap = [render.device newHeapWithDescriptor:heapDescriptor];
      }
      id<MTLBuffer> buf = [heap newBufferWithLength : size options : storage];
      G_ASSERT(buf != nil);
      return buf;
    }
  };

  BufCache global_cache;
#endif

  void Buffer::BufTex::create(MTLResourceOptions storage, MTLTextureDescriptor* pTexDesc,
                              int aligment, int tex_format, const char* name)
  {
#if HAS_SMALL_BUFFER_CACHE
    if (bufsize <= BufCache::max_size && pTexDesc == nil)
    {
      buffer = global_cache.allocate(storage, bufsize);
    }
    else
#endif
      buffer = [render.device newBufferWithLength : bufsize options : storage];

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

#if DAGOR_DBGLEVEL > 0
    if (name)
    {
      buffer.label = [NSString stringWithFormat:@"%s (%d)", name, index];
    }
#endif

    if (pTexDesc)
    {
      if (@available(macos 10.13, *))
      {
        id<MTLTexture> tex = [buffer newTextureWithDescriptor : pTexDesc
                                                       offset : 0
                                                  bytesPerRow : aligment];
        texture = new Texture(tex, tex_format, name);
      }
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
      if (@available(macos 10.13, *))
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
    }

    checkLockParams(0, bufSize, VBLOCK_DISCARD, bufFlags);
    isDynamic = bufFlags & SBCF_DYNAMIC;
    fast_discard = bufFlags & SBCF_FRAMEMEM;

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
        buf->frame = -1;

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
      last_locked_frame = render.frame;
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
      dynamic_buffer = render.AllocateConstants(bufSize, dynamic_offset, bufSize);
      G_ASSERT(dynamic_buffer);
      dynamic_frame = render.frame;

      render.discardBuffer(this, cur_buffer, dynamic_buffer, dynamic_offset);

      uint8_t *ret = (uint8_t*)dynamic_buffer.contents + dynamic_offset + offset_bytes;
#if _TARGET_IOS
      G_ASSERT(offset_bytes == 0);
      if (size_bytes != bufSize)
        memset(ret + size_bytes, 0, bufSize - size_bytes);
#endif
      return ret;
    }

    if (isDynamic)
    {
      constexpr uint32_t maxFrames = 3; // Render::MAX_FRAMES_TO_RENDER;
      if (flags & VBLOCK_DISCARD)
      {
        G_ASSERT(cur_buffer);

        BufTex* prev_buffer = cur_buffer;
        cur_buffer = cur_buffer->next;
        if (allow_resize_ring_buffer && fabs(render.frame - cur_buffer->frame) < maxFrames)
        {
          #if DAGOR_DBGLEVEL > 0
          if (!pTexDesc) // Currently SBCF_FRAMEMEM + tex_format treated as allow_resize_ring_buffer
            logerr("METAL reallocating dynamic buffer %s, consider using SBCF_FRAMEMEM",
                [ring_buffer->buffer.label UTF8String]);
          #endif

          max_buffers++;

          BufTex* buf = new BufTex(bufSize, bufFlags);
          buf->create(storage, pTexDesc, aligment, tex_format, "");
          buf->index = max_buffers - 1;

          prev_buffer->next = buf;
          buf->next = cur_buffer;

          cur_buffer = buf;
        }
        else if (fabs(render.frame - cur_buffer->frame) < maxFrames)
        {
          #if DAGOR_DBGLEVEL > 0
          logerr("METAL locking dynamic buffer %s with VBLOCK_DISCARD multiple times during last %d frames",
              [ring_buffer->buffer.label UTF8String], maxFrames);
          #endif
          render.aquareOwnerShip();
          render.flush(true);
          render.releaseOwnerShip();
        }
      }

      cur_buffer->frame = render.frame;
      render.discardBuffer(this, cur_buffer, nil, 0);
    }

    G_ASSERT(!use_upload_buffer || !(flags & VBLOCK_READONLY) || (bufFlags & SBCF_USAGE_READ_BACK));
    if ((flags & VBLOCK_READONLY) && (last_locked_frame == ~0ull || (render.frame - last_locked_frame < Render::MAX_FRAMES_TO_RENDER)))
    {
      render.aquareOwnerShip();
      @autoreleasepool
      {
        render.flush(true);
      }
      render.releaseOwnerShip();
      last_locked_frame = render.frame;
    }

    if (use_upload_buffer && !(flags & VBLOCK_READONLY))
    {
      locked_size = (locked_size + 3) & ~3;
      @autoreleasepool
      {
        if (locked_size > 128 * 1024)
        {
          upload_buffer = [render.device newBufferWithLength : locked_size
                                                     options : MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared];
#if DAGOR_DBGLEVEL > 0
          upload_buffer.label = @"upload buffer";
#endif
        }
        else
          upload_buffer = [render.AllocateConstants(locked_size, upload_buffer_offset) retain];
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

    bool from_thread = render.acquare_depth == 0 || cur_thread != render.cur_thread;
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
        tempBuffer = [render.device newBufferWithLength : size_bytes
                                                   options : MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared];
#if DAGOR_DBGLEVEL > 0
        tempBuffer.label = @"copy buffer";
#endif
      }
      else
        tempBuffer = [render.AllocateConstants(size_bytes, tempOffset) retain];
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
    return false;
  }

  bool Buffer::copyTo(Sbuffer * dest, uint32_t dst_ofs_bytes, uint32_t src_ofs_bytes, uint32_t size_bytes)
  {
    render.copyBuffer(this, src_ofs_bytes, dest, dst_ofs_bytes, size_bytes);
    return false;
  }

  void Buffer::discard(BufTex* render_buffer, id<MTLBuffer> dyn_buffer, int dyn_offset)
  {
    if (!isDynamic)
    {
      return;
    }

    dynamic_buffer = dyn_buffer;
    dynamic_offset = dyn_offset;
    cur_render_buffer = render_buffer;
  }

  id<MTLBuffer> Buffer::getBuffer()
  {
    G_ASSERTF(dynamic_buffer || cur_render_buffer, "probably <%s> fastdiscard buffer is used and not updated this frame", getResName());
    if (dynamic_buffer && dynamic_frame != render.frame)
    {
#if DAGOR_DBGLEVEL > 0
      logerr("METAL setting buffer <%s> which has SBCF_FRAMEMEM and wasn't locked this frame",
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

  void Buffer::apply(id<MTLBuffer> buffer, int stage, int slot, int offset)
  {
    if (buffer == nil)
      offset = 0;
    if (stage == STAGE_VS)
    {
      [render.renderEncoder setVertexBuffer : buffer offset : offset atIndex : slot];
    }
    else
    if (stage == STAGE_PS)
    {
      [render.renderEncoder setFragmentBuffer : buffer offset : offset atIndex : slot];
    }
    else
    if (stage == STAGE_CS)
    {
      [render.computeEncoder setBuffer : buffer offset : offset atIndex : slot];
    }
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
