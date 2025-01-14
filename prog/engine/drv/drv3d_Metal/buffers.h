// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#import "texture.h"
#include <drv/3d/dag_buffers.h>

namespace drv3d_metal
{
class Buffer : public Sbuffer
{
public:
  struct BufTex
  {
    uint64_t last_submit = 0;
    int index = 0;
    id<MTLBuffer> buffer;
    Texture *texture;
    BufTex *next;
    int bufsize = 0;
    int tid = -1;
    int flags = 0;
    bool initialized = false;

    BufTex(int bufsize, int flags) : bufsize(bufsize), flags(flags) {}

    void create(MTLResourceOptions storage, MTLTextureDescriptor *pTexDesc, int aligment, int tex_format, const char *name);
    void release();

    int ressize() { return bufsize; }

    void destroyObject() {}
  };

protected:
  int max_buffers = 2;
  bool isDynamic = false;
  BufTex *cur_buffer;
  BufTex *cur_render_buffer;
  BufTex *ring_buffer = nullptr;
  bool allow_resize_ring_buffer = false;
  bool fast_discard = false;

  id<MTLBuffer> upload_buffer = nil;
  int upload_buffer_offset = 0;
  bool use_upload_buffer = false;

  int sync_offset;
  int sync_size;

  MTLTextureDescriptor *pTexDesc = nullptr;
  MTLResourceOptions storage;
  int aligment = 0;
  int tex_format = 0;

  Buffer();

public:
  uint32_t bufSize;
  uint32_t bufFlags;
  uint16_t structSize;

  // we use this to temporary suballocate main cbuffer
  // and commit changes to cur_buffer at the end of frame
  id<MTLBuffer> dynamic_buffer = nil;
  int dynamic_offset = 0;
  uint64_t dynamic_frame = 0;

  // slot mask where buffer is bound
  uint64_t bound_slots = 0;

  int locked_offset;
  int locked_size;
  uint64_t last_locked_submit = ~0ull;

  Buffer(uint32_t bufsize, int ssize, int buf_flags, int tex_format, const char *name);
  ~Buffer();

  int ressize() const override { return bufSize; }
  int getFlags() const override { return bufFlags; }

  virtual int getElementSize() const { return structSize; }
  virtual int getNumElements() const { return (int)(structSize > 0 ? bufSize / structSize : 0); };

  virtual void setResApiName(const char *name) const;

  virtual int lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int flags);
  void *lock(uint32_t offset_bytes, uint32_t size_bytes, int flags);
  int unlock();

  bool updateData(uint32_t ofs_bytes, uint32_t size_bytes, const void *__restrict src, uint32_t lockFlags);

  virtual bool copyTo(Sbuffer * /*dest*/);
  virtual bool copyTo(Sbuffer * /*dest*/, uint32_t /*dst_ofs_bytes*/, uint32_t /*src_ofs_bytes*/, uint32_t /*size_bytes*/);

  int getDynamicOffset() const;

  id<MTLBuffer> getBuffer();
  Texture *getTexture();

  virtual void destroy();
  void release();

  static void cleanup();
};

void clear_buffers_pool_garbage();
} // namespace drv3d_metal
