// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#import <MetalFX/MetalFX.h>
#import <Metal/MTLAccelerationStructure.h>
#include <EASTL/atomic.h>
#include <EASTL/array.h>
#include <EASTL/unordered_map.h>
#include <drv/3d/dag_sampler.h>

#include <mutex>

#include "indicesManager.h"

#import "metalview.h"

#include "vdecl.h"
#include "buffers.h"
#include "shader.h"
#include "program.h"
#include "texture.h"
#include "shadersPreCache.h"
#include "bindless.h"

#include "buffBindPoints.h"

#include <generic/dag_carray.h>

#include <memory/dag_framemem.h>
#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_critSec.h>

#include <EASTL/unique_ptr.h>

#define HAVE_SAMPLE_QUERIES DAGOR_DBGLEVEL > 0

struct RaytraceAccelerationStructure : public drv3d_metal::HazardTracker
{
  id<MTLAccelerationStructure> acceleration_struct = nil;
  int index = -1;
  bool was_built = false;
  RaytraceBuildFlags createFlags = RaytraceBuildFlags::NONE;
};

struct RaytraceTopAccelerationStructure : RaytraceAccelerationStructure
{};
struct RaytraceBottomAccelerationStructure : RaytraceAccelerationStructure
{};

struct ResourceHeap
{
  struct Resource
  {
    enum class Access
    {
      Read = 0,
      Write
    };

    drv3d_metal::HazardEncoder enc;
    uint32_t offset = 0;
    uint32_t size = 0;
    Access type = Access::Read;
  };

  id<MTLHeap> heap = nil;
  struct ResourceHeapGroup *group = nullptr;
  size_t size = 0;
  eastl::vector<Resource> resources;

  void track_resource_read(drv3d_metal::HazardTracker &resource);
  void track_resource_write(drv3d_metal::HazardTracker &resource);
};

namespace drv3d_metal
{

enum
{
  // STAGE_CS, STAGE_PS, STAGE_VS
  // artificial stage to be able to bind STAGE_VS resources to mesh and object shaders
  STAGE_MS = 3,
  STAGE_OS = 4,
  STAGE_TOTAL
};

struct RenderAttachment
{
  Texture *texture = nullptr;
  Texture *resolve_target = nullptr;

  MTLLoadAction load_action = MTLLoadActionLoad;
  MTLStoreAction store_action = MTLStoreActionStore;
  uint32_t level = 0;
  uint32_t layer = 0;

  float clear_value[4]{};
};

namespace DirtyFlags
{
enum
{
  DepthState = 1 << 0,
  StencilRef = 1 << 1,
  DepthBias = 1 << 2,
  Viewport = 1 << 3,
  Scissor = 1 << 4,
  Cull = 1 << 5,
  Query = 1 << 6,
  BlendFactor = 1 << 7,
  DepthClip = 1 << 8,
  Fill = 1 << 9,
};
}

struct TexCopyRegion
{
  Texture *dst_ptr = nullptr;
  Texture *src_ptr = nullptr;
  id<MTLTexture> dst = nil;
  id<MTLTexture> src = nil;
  MTLPixelFormat src_format;
  MTLPixelFormat dst_format;
  uint16_t dest_x = 0, dest_y = 0, dest_z = 0;
  uint16_t dest_level = 0;
  uint16_t dest_levels = 0;
  uint16_t src_level = 0;
  uint8_t src_levels = 0;
  uint8_t src_dxt = 0;
  uint16_t src_x = 0, src_y = 0, src_z = 0;
  uint16_t src_w = 0, src_h = 0, src_d = 0;

  TexCopyRegion() = default;

  TexCopyRegion(Texture *dst, int dest_level, int dest_x, int dest_y, int dest_z, Texture *src, int src_level, int src_x, int src_y,
    int src_z, int src_w, int src_h, int src_d) :
    dst_ptr(dst),
    src_ptr(src),
    dst(dst->apiTex->texture),
    src(src->apiTex->texture),
    src_format(src->metal_format),
    dst_format(dst->metal_format),
    dest_x(dest_x),
    dest_y(dest_y),
    dest_z(dest_z),
    dest_level(dest_level),
    dest_levels(dst->mipLevels),
    src_level(src_level),
    src_levels(src->mipLevels),
    src_dxt(src->use_dxt),
    src_x(src_x),
    src_y(src_y),
    src_z(src_z),
    src_w(src_w),
    src_h(src_h),
    src_d(src_d)
  {}
};

struct SamplerState
{
  int addrU = TEXADDR_WRAP;
  int addrV = TEXADDR_WRAP;
  int addrW = TEXADDR_WRAP;
  d3d::FilterMode texFilter = d3d::FilterMode::Point;
  d3d::MipMapMode mipmap = d3d::MipMapMode::Point;
  int max_level = 1024;
  float lod = 0;
  int anisotropy = 1;
  E3DCOLOR border_color = E3DCOLOR(0, 0, 0, 0);

  id<MTLSamplerState> sampler = nil;

  friend inline bool operator==(const SamplerState &left, const SamplerState &right)
  {
    if (left.addrU == right.addrU && left.addrV == right.addrV && left.addrW == right.addrW && left.texFilter == right.texFilter &&
        left.mipmap == right.mipmap && left.max_level == right.max_level && left.anisotropy == right.anisotropy &&
        left.border_color == right.border_color)
    {
      return true;
    }

    return false;
  }
};

class Render
{
public:
  static constexpr int MAX_FRAMES_TO_RENDER = 2;
  static constexpr int MAX_CBUFFER_SIZE = 64 * 1024;

  // there's 32kb limit for query buffer so use it
  static constexpr int MAX_SAMPLE_SAMPLES = 32768 / sizeof(MTLCounterResultTimestamp);

  struct Caps
  {
    bool readWriteTextureTier1 = true;
    bool readWriteTextureTier2 = true;
  };

  enum class CommandType : uint8_t
  {
    BeginPass = 0,
    SetResources,
    SetPipelineState, // sets graphics pipeline state
    SetComputePipelineState,
    SetDepthState, // sets depth state
    SetStencilRef,
    SetBlendFactor,
    SetDepthBias,
    SetViewport,
    SetScissor,
    SetCull,
    SetFill,
    SetQuery,
    SetDepthClip,
    Draw,
    DrawIndexed,
    DrawIndirect,
    DrawIndexedIndirect,
    ClearBuffer,
    Dispatch,
    DispatchIndirect,
    GenerateMips,
    CopyTex,
    CopyBuf,
    ReadbackTexture,
    CopyTexRegion,
    TrackBindless,
    CopyAccelerationStruct,
    BuildAccelerationStructure,
    DoMetalFX,
    DoQuery,
    PresentDrawable,
    BeginEvent,
    EndEvent,
    DispatchMesh,
    DispatchMeshIndirect,
  };

  struct CommandEncoder
  {
    CommandEncoder()
    {
      // 64kb reserve should be a reasonable guesstimate
      data.reserve(64 << 10);
    }

    CommandEncoder &write(const void *ptr, size_t size)
    {
      uint8_t *ptr_uint = (uint8_t *)ptr;
      data.insert(data.end(), ptr_uint, ptr_uint + size);
      return *this;
    }

    template <typename T>
    CommandEncoder &write(const T &ptr)
    {
      return write(&ptr, sizeof(ptr));
    }

    CommandEncoder &read(void *ptr, size_t size)
    {
      memcpy(ptr, data.data() + read_offset, size);
      read_offset += size;
      return *this;
    }

    template <typename T>
    CommandEncoder &read(T &ptr)
    {
      return read(&ptr, sizeof(ptr));
    }

    void clear()
    {
      data.clear();
      read_offset = 0;
    }

    bool empty() const { return read_offset == data.size(); }

  private:
    eastl::vector<uint8_t> data;
    size_t read_offset = 0;
  };

  struct Resource
  {
    enum class Type : uint32_t
    {
      Buffer = 0,
      Texture,
      Sampler,
      AccelerationStructure
    };

    HazardTracker *resource = nullptr;
    id metal_resource = nil;
    uint32_t offset = 0;
    uint32_t stage : 2 = STAGE_VS;
    uint32_t uav : 1 = false;
    uint32_t is_from_vs : 1 = false;
    uint32_t slot : 6 = 0;
    uint32_t offset_only : 1 = 0;
    Type resource_type : 8 = Type::Buffer;
  };
  typedef eastl::vector<Render::Resource, framemem_allocator> ResourceArray;
  static_assert(sizeof(Resource) == 24);

  struct ClearTexOnCreate
  {
    Texture *tex = nullptr;
    id<MTLTexture> metalTex = nil;
    uint32_t slices = 1;
    uint32_t depth = 1;
    uint32_t levels = 1;
    uint32_t width = 1;
    uint32_t height = 1;
    int base_format = 0;
    bool use_dxt = false;

    ClearTexOnCreate(Texture *tex) : tex(tex)
    {
      G_ASSERT(tex);
      if (tex->type == D3DResourceType::ARRTEX)
        slices = tex->depth;
      else if (tex->type == D3DResourceType::CUBETEX)
        slices = 6;
      else if (tex->type == D3DResourceType::CUBEARRTEX)
        slices = 6 * tex->depth;

      if (tex->type == D3DResourceType::VOLTEX)
        depth = tex->depth;

      levels = tex->mipLevels;
      width = tex->width;
      height = tex->height;

      G_ASSERT(tex->apiTex);
      metalTex = tex->apiTex->texture;

      base_format = tex->base_format;
      use_dxt = tex->use_dxt;
    }
  };

  Caps caps;

  struct Viewport
  {
    float x, y;
    float w, h;
    float minz, maxz;

    bool used;

    Viewport()
    {
      x = y = w = h = 0;
      minz = 0.01f;
      maxz = 1.0f;

      used = false;
    }
  };

  struct DepthState
  {
    union
    {
      struct
      {
        uint64_t zenable : 1;
        uint64_t zfunc : 4;
        uint64_t depth_write_on : 1;
        uint64_t stencil_enable : 1;
        uint64_t stencil_func : 4;
        uint64_t stencil_read_mask : 8;
        uint64_t stencil_write_mask : 8;
        uint64_t sfail : 4;
        uint64_t szfail : 4;
        uint64_t spass : 4;
      };
      uint64_t depth_state = 0;
    };

    id<MTLDepthStencilState> depthState;

    friend inline bool operator==(const DepthState &left, const DepthState &right) { return left.depth_state == right.depth_state; }
  };

  struct CachedRenderState
  {
    Program::RenderState::RasterizerState raster_state;
    DepthState depth_state;
    bool scissor_on = false;
    float depth_bias = 0.f;
    float depth_slopebias = 0.f;
    uint32_t stencil_ref = 0;
    MTLCullMode cull = MTLCullModeNone;
    MTLDepthClipMode depth_clip = MTLDepthClipModeClip;
    uint32_t forcedSampleCount = 1;
  };

  static MTLDepthStencilDescriptor *depthStateDesc;
#if USE_METALFX_UPSCALE
  struct MTLFXUpscale
  {
    eastl::unordered_map<uint64_t, id> spatialScalers;
    bool supported = false;
  };
  MTLFXUpscale upscale;
#endif

  struct Query
  {
    int offset;
    int status;
    int64_t value;
    int used;

    Query()
    {
      offset = 0;
      status = 0;
      value = 0;
      used = 0;
    };
  };

  struct UploadTex
  {
    Texture *tex_ptr;
    id<MTLTexture> tex;
    id<MTLBuffer> buf;
    uint32_t pitch;
    uint32_t imageSize;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t w;
    uint16_t h;
    uint16_t d;
    uint8_t level;
    uint8_t layer;
  };

  struct CopyBuf
  {
    Buffer *dst_ptr = nullptr;
    id<MTLBuffer> src = nil;
    id<MTLBuffer> dst = nil;
    int src_offset = 0;
    int dst_offset = 0;
    int size = 0;
  };

  struct RingBufferItem
  {
    static const uint32_t max_dynamic_size = 2 * 1024 * 1024;
    static const uint32_t max_constant_size = 1024 * 1024;
    id<MTLBuffer> buf = nil;
    uint32_t offset = 0;
  };

  struct DeletedResource
  {
    enum class Type
    {
      None = 0,
      Texture,
      Buffer,
      VDecl,
      Shader,
      Program,
      NativeResource,
      Heap
    };
    Type type = Type::None;
    uint64_t submit = 0;
    union
    {
      Texture *texture;
      Buffer *buffer;
      int vdecl;
      int shader;
      int program;
      id<MTLResource> native_resource;
      id<MTLHeap> heap;
    };
  };

  MTLViewport cached_viewport;

  std::mutex dynamic_buffer_lock, delete_lock, copy_tex_lock;
  Tab<DeletedResource> resources2delete;

  Tab<RingBufferItem> constant_buffers_free;
  RingBufferItem constant_buffer;

  Tab<RingBufferItem> dynamic_buffers_free;
  RingBufferItem dynamic_buffer;

  Tab<UploadTex> textures2upload;
  Tab<CopyBuf> buffers2copy;
  Tab<TexCopyRegion> regions2copy;
  Tab<ClearTexOnCreate> textures2clear;
  Tab<eastl::pair<Buffer *, Buffer::BufTex *>> buffers2clear;

  Tab<SamplerState> sampler_states;
  std::mutex samplerStatesMutex;

  int getSampler(SamplerState sampler_state);

  CommandEncoder command_encoder;

  void freeResources();

  id<MTLBuffer> createBuffer(uint32_t size, uint32_t flags, const char *name = "");

  id<MTLBuffer> AllocateConstants(uint32_t size, int &offset, int sizeLeft = 0)
  {
    G_ASSERT(size <= RingBufferItem::max_constant_size);

    size = (size + 31) & ~31; // align size since we're padding anyways
    if (!constant_buffer.buf || constant_buffer.offset + size > RingBufferItem::max_constant_size ||
        constant_buffer.offset + sizeLeft > RingBufferItem::max_constant_size)
    {
      if (constant_buffer.buf)
      {
        std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);
        frame_callback.push_back([this, cb = constant_buffer] {
          std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);
          constant_buffers_free.push_back(cb);

          constexpr uint32_t max_const_buffers = 4;
          if (constant_buffers_free.size() > max_const_buffers)
          {
            for (int i = max_const_buffers; i < constant_buffers_free.size(); i++)
              [constant_buffers_free[i].buf release];
            constant_buffers_free.resize(max_const_buffers);
          }
        });
      }
      if (constant_buffers_free.empty())
      {
        constant_buffer.buf = createBuffer(RingBufferItem::max_constant_size,
          MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared, "ring buffer");
      }
      else
      {
        constant_buffer = constant_buffers_free.back();
        constant_buffers_free.pop_back();
      }
      constant_buffer.offset = 0;
    }

    offset = constant_buffer.offset;
    constant_buffer.offset += size;

    return constant_buffer.buf;
  }

  id<MTLBuffer> AllocateDynamicBuffer(uint32_t size, int &offset, int sizeLeft = 0)
  {
    G_ASSERT(size <= RingBufferItem::max_dynamic_size);

    std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);

    size = (size + 31) & ~31; // align size since we're padding anyways
    if (!dynamic_buffer.buf || dynamic_buffer.offset + size > RingBufferItem::max_dynamic_size ||
        dynamic_buffer.offset + sizeLeft > RingBufferItem::max_dynamic_size)
    {
      if (dynamic_buffer.buf)
      {
        frame_callback.push_back([this, db = dynamic_buffer] {
          std::lock_guard<std::mutex> scopedLock(dynamic_buffer_lock);
          dynamic_buffers_free.push_back(db);

          constexpr uint32_t max_dynamic_buffers = 3;
          if (dynamic_buffers_free.size() > max_dynamic_buffers)
          {
            for (int i = max_dynamic_buffers; i < dynamic_buffers_free.size(); i++)
              [dynamic_buffers_free[i].buf release];
            dynamic_buffers_free.resize(max_dynamic_buffers);
          }
        });
      }
      if (dynamic_buffers_free.empty())
      {
        dynamic_buffer.buf = createBuffer(RingBufferItem::max_dynamic_size,
          MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared, "ring dynamic buffer");
      }
      else
      {
        dynamic_buffer = dynamic_buffers_free.back();
        dynamic_buffers_free.pop_back();
      }
      dynamic_buffer.offset = 0;
    }

    offset = dynamic_buffer.offset;
    dynamic_buffer.offset += size;

    return dynamic_buffer.buf;
  }

  eastl::atomic<uint64_t> cur_thread;
  uint64_t main_thread;
  eastl::atomic<int> acquire_depth;

  int max_commands = 0;

  char device_name[128];

  MetalView *mainview;

  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;

  id<MTLRenderCommandEncoder> renderEncoder = nil;
  id<MTLBlitCommandEncoder> blitEncoder = nil;
  id<MTLComputeCommandEncoder> computeEncoder = nil;
  id<MTLAccelerationStructureCommandEncoder> accelerationEncoder = nil;
#if DAGOR_DBGLEVEL > 0
  bool force_sync_encoding = false;
#endif

  bool async_pso_compilation = false;
  bool using_async_pso_compilation = false;
  bool can_use_async_pso_compilation = false;
  Tab<bool> async_pso_compilation_stack;
  uint32_t async_pso_compilation_length = 0;
  void pushAsyncPsoCompilation(bool enable);
  void popAsyncPsoCompilation();

  id<MTLComputePipelineState> clear_cs_pipeline = nil;

  id<MTLBuffer> query_buffer;
  id<MTLBuffer> stub_buffer = nil;
  int cur_query_offset = -1;

  struct Encoder
  {
    id<MTLFence> fence = nil;
    uint64_t submit_index : 48 = 0;
    uint64_t index : 16 = 0;
    uint64_t last_used_submit = 0;
  };

  eastl::deque<Encoder> encoders;
  eastl::deque<Encoder *> free_encoders;
  bool manual_hazard_tracking = true;

  Encoder *current_encoder = nil;
  Encoder *current_enqueued_encoder = nullptr;
  eastl::vector<eastl::pair<uint32_t, uint64_t>> last_encoders;
  eastl::vector<eastl::pair<uint32_t, uint64_t>> last_encoders_current;
  uint32_t current_enqueued_resources = 0;
  eastl::vector<bool> encoders_to_wait;
  eastl::vector<bool> encoders_to_wait_vs;

  Encoder *allocate_encoder()
  {
    G_ASSERT(current_encoder == nullptr);
    G_ASSERT(manual_hazard_tracking);
    auto it = eastl::find_if(free_encoders.begin(), free_encoders.end(),
      [this](const Encoder *enc) { return enc->last_used_submit <= submits_completed; });
    Encoder *new_encoder = nullptr;
    if (it != free_encoders.end())
    {
      new_encoder = *it;
      free_encoders.erase(it);
    }
    else
    {
      encoders.push_back();
      encoders_to_wait.resize(encoders.size());
      encoders_to_wait_vs.resize(encoders.size());
      new_encoder = &encoders.back();
      new_encoder->fence = [device newFence];
      new_encoder->index = encoders.size() - 1;
#if DAGOR_DBGLEVEL > 0
      new_encoder->fence.label = [NSString stringWithFormat:@"Fence index %d", int(new_encoder->index)];
#endif
    }
    G_ASSERT(new_encoder);
    G_ASSERT(new_encoder->fence);
    new_encoder->submit_index = submits_scheduled;
    new_encoder->last_used_submit = submits_scheduled;
    return new_encoder;
  }

  void submit_encoder(Encoder *&enc)
  {
    if (!manual_hazard_tracking)
      return;

    G_ASSERT(enc);
    free_encoders.push_back(enc);

    if (renderEncoder)
      [renderEncoder updateFence:enc->fence afterStages:MTLRenderStageFragment];
    else if (blitEncoder)
      [blitEncoder updateFence:enc->fence];
    else if (computeEncoder)
      [computeEncoder updateFence:enc->fence];
    else if (accelerationEncoder)
      [accelerationEncoder updateFence:enc->fence];
    else
      G_ASSERT(0 && "Can't insert fence when there's no encoder");
    enc = nullptr;
  }

  void track_resource_write_enqueued(HazardTracker &resource)
  {
    if (!manual_hazard_tracking)
      return;
    G_ASSERT(resource.heap == nullptr);
    G_ASSERT(current_enqueued_encoder);
    current_enqueued_resources++;
  }

  // marks that resource is being set as rw/rt in current encoder
  void track_resource_write(HazardTracker &resource, bool is_from_vs = false)
  {
    if (!manual_hazard_tracking)
      return;

    G_ASSERT(current_encoder);

    if (resource.heap)
      return resource.heap->track_resource_write(resource);

    // remove reads that are already executed and we don't care about
    resource.encoders_read_in.erase(eastl::remove_if(resource.encoders_read_in.begin(), resource.encoders_read_in.end(),
                                      [this](const HazardEncoder &enc) { return enc.submit <= submits_completed; }),
      resource.encoders_read_in.end());

    // first make sure all reads are done
    for (auto &enc : resource.encoders_read_in)
    {
      track_resource_read_impl(enc, is_from_vs);
      // no need to track reads anymore, encoder_written_in gonna handle dependencies
      enc.submit = 0;
    }

    // make sure we're done with previous write
    track_resource_read_impl(resource.encoder_written_in, is_from_vs);

    resource.encoder_written_in.encoder = current_encoder->index;
    resource.encoder_written_in.submit = submits_scheduled;
  }

  void track_resource_read_impl(const HazardEncoder &enc, bool is_from_vs = false)
  {
    if (enc.encoder == 0 || enc.submit <= submits_completed)
      return;

    if (encoders_to_wait[enc.encoder] && encoders_to_wait_vs[enc.encoder] >= is_from_vs)
      return;

    id<MTLFence> fence = encoders[enc.encoder].fence;
    encoders[enc.encoder].last_used_submit = submits_scheduled;
    if (renderEncoder)
      [renderEncoder waitForFence:fence beforeStages:is_from_vs ? MTLRenderStageVertex : MTLRenderStageFragment];
    else if (blitEncoder)
      [blitEncoder waitForFence:fence];
    else if (computeEncoder)
      [computeEncoder waitForFence:fence];
    else if (accelerationEncoder)
      [accelerationEncoder waitForFence:fence];
    else
      G_ASSERT(0 && "Can't insert fence when there's no encoder");

    encoders_to_wait[enc.encoder] = 1;
    encoders_to_wait_vs[enc.encoder] |= is_from_vs;
  }

  // checks if resource was written previously and waits for encoder that did the writes
  void track_resource_read(HazardTracker &resource, bool is_from_vs = false)
  {
    if (!manual_hazard_tracking)
      return;

    if (resource.heap)
      return resource.heap->track_resource_read(resource);

    // remove reads that are already executed and we don't care about
    resource.encoders_read_in.erase(eastl::remove_if(resource.encoders_read_in.begin(), resource.encoders_read_in.end(),
                                      [this](const HazardEncoder &enc) { return enc.submit <= submits_completed; }),
      resource.encoders_read_in.end());

    // make sure we're done with previous write
    track_resource_read_impl(resource.encoder_written_in, is_from_vs);

    G_ASSERT(current_encoder);
    HazardEncoder enc = {submits_scheduled, current_encoder->index};
    if (resource.encoders_read_in.empty() || resource.encoders_read_in.back() != enc)
      resource.encoders_read_in.push_back(enc);
  }

#if HAVE_SAMPLE_QUERIES
  id<MTLCounterSampleBuffer> sampleBuffer = nil;
  uint64_t sampleSamples[MAX_SAMPLE_SAMPLES] = {};
  uint64_t processedSample = 0;
#endif
  uint64_t nextSubmittedSample = 0;
  uint64_t nextSample = 0;

  id<MTLBuffer> tmp_copy_buff;
  bool save_backBuffer = false;
  bool drawable_acquired = false;

  static constexpr int QUERIES_MAX = 8192;

  int used_query;
  carray<Query, QUERIES_MAX> queries;

  CritSecStorage acquireSec;
  CritSecStorage rcSec;

  MTLIndexType ibuffertype = MTLIndexTypeUInt16;
  Buffer *ibuffer = nullptr;

  VDecl *vdecl;

  // if srgb backbuffer write is enabled
  bool is_rgb_backbuffer = false;

  Texture *blank_tex[int(MetalImageType::Count)];
  Texture *blank_tex_uint[int(MetalImageType::Count)];
  Texture *blank_tex_rw[int(MetalImageType::Count)];
  id<MTLTexture> blank_tex_2d = 0;
  id<MTLTexture> blank_tex_2dArray = 0;
  id<MTLTexture> blank_tex_cube = 0;

  struct RenderTargetConfig
  {
    RenderAttachment colors[Program::MAX_SIMRT];
    RenderAttachment depth;
    Viewport vp;

    bool isFullscreenViewport() const;
  };
  RenderTargetConfig rt;
  int need_change_rt;

  // cs maps to cs
  // ps maps to ps
  // vs maps to vs/ms/os
  struct StageStorage
  {
    static constexpr uint32_t MAX_SAMPLERS = 16;

    id<MTLBuffer> buffers[BUFFER_POINT_COUNT] = {};
    int buffers_offset[BUFFER_POINT_COUNT] = {};

    static_assert(drv3d_metal::BUFFER_POINT_COUNT <= 64, "not enough bits for buffers");
    uint64_t buffer_dirty_mask = 0;

    id<MTLAccelerationStructure> acceleration_structures[MAX_SHADER_ACCELERATION_STRUCTURES];

    uint32_t immediate_dwords[4] = {~0u};
    uint32_t immediate_dword_count = 0;
    int immediate_slot = -1;

    id<MTLTexture> textures[MAX_SHADER_TEXTURES] = {};
    uint64_t texture_dirty_mask = 0;

    id<MTLSamplerState> samplers[MAX_SAMPLERS];
    uint64_t sampler_dirty_mask = 0;

    uint8_t cbuffer[MAX_CBUFFER_SIZE];
    int cbuffer_num_bound = 0;

    uint32_t stage = STAGE_TOTAL;

    bool is_vertex() const { return stage == STAGE_VS || stage == STAGE_MS || stage == STAGE_OS; }

    void reset()
    {
      for (auto &buf : buffers)
        buf = nil;
      for (auto &tex : textures)
        tex = nil;
      for (auto &smp : samplers)
        smp = nil;

      cbuffer_num_bound = 0;
      immediate_slot = -1;
      immediate_dword_count = 0;

      buffer_dirty_mask = ~0ull;
      sampler_dirty_mask = ~0ull;
      texture_dirty_mask = ~0ull;
    }

    void bindBuffer(uint32_t slot, Buffer *resource, id<MTLBuffer> buffer, unsigned offset, bool uav, ResourceArray &resources)
    {
      G_ASSERT(stage != STAGE_TOTAL);
      if (buffer && buffers[slot] == buffer && buffers_offset[slot] == offset)
        return;
      if (buffer)
      {
        resources.push_back({
          .resource = resource,
          .metal_resource = buffer,
          .offset = offset,
          .stage = stage,
          .uav = uav,
          .is_from_vs = is_vertex(),
          .slot = slot,
          .offset_only = buffers[slot] == buffer,
          .resource_type = Render::Resource::Type::Buffer,
        });
        buffers[slot] = buffer;
        buffers_offset[slot] = offset;
      }
    }
  };

  class ConstBuffer
  {
  public:
    uint8_t cbuffer[MAX_CBUFFER_SIZE];
    int num_stored = 0;

    void init();
    void destroy();
    void applyCmd(void *data, int reg_base, int num_regs);

    void applyBuffer(StageStorage &storage, int num_reg, ResourceArray &resources);
  };

  class StageBinding
  {
  public:
    Buffer *buffers[BUFFER_POINT_COUNT];
    int buffers_offset[BUFFER_POINT_COUNT];

    RaytraceTopAccelerationStructure *acceleration_structures[MAX_SHADER_ACCELERATION_STRUCTURES];

    uint32_t immediate_dwords[4] = {0};
    uint32_t immediate_dword_count = 0;

    enum class SamplerSource : uint8_t
    {
      None = 0,
      Texture,
      Sampler
    };

    struct TextureSlot
    {
      Texture *texture = nullptr;
      bool read_stencil = false;
      bool as_uint = false;
      SamplerSource source = SamplerSource::None;
      uint8_t mip_level = 0;
      uint8_t slice = 0;
    };

    // first MAX_SHADER_TEXTURES is ordinary textures
    // second MAX_SHADER_TEXTURES are uav textures
    static constexpr uint32_t MAX_STAGE_TEXTURES = MAX_SHADER_TEXTURES * 2;
    TextureSlot textures[MAX_STAGE_TEXTURES];

    id<MTLSamplerState> samplers[MAX_STAGE_TEXTURES];

    ConstBuffer cbuffer;

    __forceinline void setTex(StageStorage &storage, int slot, Texture *tex, bool read_stencil = false, int mip = 0, int slice = 0,
      bool as_uint = false)
    {
      G_ASSERT(slot < MAX_STAGE_TEXTURES);
      G_ASSERT(!as_uint || slot >= MAX_SHADER_TEXTURES);
      auto &cache = textures[slot];

      if (cache.texture != tex || cache.read_stencil != read_stencil || cache.as_uint != as_uint || cache.mip_level != mip ||
          cache.slice != slice)
        storage.texture_dirty_mask |= 1ull << slot;

      cache.texture = tex;
      cache.read_stencil = read_stencil;
      cache.as_uint = as_uint;
      cache.mip_level = mip;
      cache.slice = slice;
    }

    __forceinline void setSampler(StageStorage &storage, int slot, id<MTLSamplerState> sampler, float bias)
    {
      G_ASSERT(slot < 16);
      if (sampler)
      {
        if (samplers[slot] != sampler)
          storage.sampler_dirty_mask |= 1ull << slot;
        textures[slot].source = SamplerSource::Sampler;
      }
      samplers[slot] = sampler;
    }

    __forceinline void setBuf(StageStorage &storage, int slot, Buffer *buf, int offset = 0)
    {
      G_ASSERT(slot < BUFFER_POINT_COUNT);
      if (buffers[slot])
        buffers[slot]->bound_slots &= ~(1ull << slot);

      int check_offset = buf ? buf->getDynamicOffset() + offset : offset;
      if (buffers[slot] != buf || buffers_offset[slot] != check_offset)
      {
        if (buf && buf->getTexture())
          storage.texture_dirty_mask |= 1ull << slot;
        else
          storage.buffer_dirty_mask |= 1ull << slot;
      }

      buffers[slot] = buf;
      buffers_offset[slot] = offset;

      if (buffers[slot])
        buffers[slot]->bound_slots |= 1ull << slot;
    }

    __forceinline void setAccStruct(int slot, RaytraceTopAccelerationStructure *tlas)
    {
      G_ASSERT(slot < MAX_SHADER_ACCELERATION_STRUCTURES);
      acceleration_structures[slot] = tlas;
    }

    void bindVertexStream(StageStorage &storage, uint32_t stream, uint32_t offset, ResourceArray &resources);
    void apply(StageStorage &storage, Shader *shader, ResourceArray &resources);
    void apply_vertex_streams(StageStorage &storage, ResourceArray &resources);
    void apply_buffers(StageStorage &storage, Shader *shader, ResourceArray &resources);
    void apply_textures(StageStorage &storage, Shader *shader, ResourceArray &resources);
    void apply_samplers(StageStorage &storage, Shader *shader, ResourceArray &resources);
    void apply_acceleration_structs(StageStorage &storage, Shader *shader, ResourceArray &resources);

    void reset();
    void removeBuf(Buffer *buf);
    void removeTex(Texture *tex);
  };

  struct BufferUP : public Buffer
  {
    BufferUP()
    {
      isDynamic = true;
      fast_discard = true;
    }
  };
  BufferUP upBufferVB, upBufferIB;

  // this is used for sync debugging of gpu side errors
#if DAGOR_DBGLEVEL > 0
  std::mutex signpost_mutex;
  std::condition_variable signpost_condition;
  uint64_t signpost = 0;
  id<MTLSharedEvent> signpost_event;
  MTLSharedEventListener *signpost_listener = nil;
  dispatch_queue_t signpost_queue;
  bool use_signposts = false;
#endif

  StageBinding stages[STAGE_MAX];
  StageStorage storages[STAGE_TOTAL] = {
    {.stage = STAGE_CS}, {.stage = STAGE_PS}, {.stage = STAGE_VS}, {.stage = STAGE_MS}, {.stage = STAGE_OS}};
  BindlessManager bindlessManager;

  __forceinline void markDirty(StageStorage &storage, Buffer *buf)
  {
    G_ASSERT(buf);
    if (buf && buf->getTexture())
      storage.texture_dirty_mask |= buf->bound_slots;
    else
      storage.buffer_dirty_mask |= buf->bound_slots;
  }

  void markBufferDirty(Buffer *buf)
  {
    markDirty(storages[STAGE_VS], buf);
    markDirty(storages[STAGE_MS], buf);
    markDirty(storages[STAGE_OS], buf);
    markDirty(storages[STAGE_PS], buf);
    markDirty(storages[STAGE_CS], buf);
  }

  bool forceClearOnCreate = false;
  bool render_pass = false;
  bool has_image_blocks = false;

  MTLDepthClipMode depth_clip = MTLDepthClipModeClip;
  int stencil_ref = 0;

  float depth_bias;
  float depth_slopebias;

  MTLCullMode cur_cull = MTLCullModeBack;
  MTLTriangleFillMode cur_fill = MTLTriangleFillModeFill;

  bool inited;

  int scr_wd;
  int scr_ht;

  int wnd_wd = 0;
  int wnd_ht = 0;

  int cur_wd;
  int cur_ht;

  int scr_bpp;
  bool cur_vsync;

  uint32_t dirty_state = 0;

  DepthState cur_depthstate;
  Tab<DepthState> dstate_cache;

  bool scissor_on;
  int sci_x, sci_y, sci_w, sci_h;
  uint64_t frame = 0;

  float blend_factor[4] = {};

  bool validate_framemem_bounds = false;

  uint32_t number_of_frames_to_skip_after_error = 0;
  uint32_t max_number_of_frames_to_skip_after_error = 0;
  bool report_gpu_errors = false;
  std::atomic<int> hadError;

  std::atomic<bool> report_stalls;

  // us
  std::atomic<uint64_t> current_gpu_time;
  uint64_t last_gpu_time = 8300;

  std::atomic<uint64_t> current_command_gpu_time;
  bool capture_command_gpu_time = false;

  Texture *backbuffer = nullptr;

  uint64_t submits_completed = 0;
  uint64_t submits_scheduled = 1;
  uint64_t frames_completed[MAX_FRAMES_TO_RENDER] = {0, 0};
  eastl::vector<eastl::function<void()>> frame_callbacks[MAX_FRAMES_TO_RENDER];
  eastl::vector<eastl::function<void()>> frame_callback;
  std::mutex frame_mutex;
  std::condition_variable frame_condition;

  Program::RenderState cur_rstate;
  id<MTLRenderPipelineState> cur_state;
  Program::RenderState last_rstate;
  uint32_t forcedSampleCount = 1;

  Program *cur_prog;
  id<MTLComputePipelineState> current_cs_pipeline = nil;

  GenerationReferencedData<shaders::DriverRenderStateId, CachedRenderState> render_states;

  IndicesManager<Shader> shaders;
  IndicesManager<Program> progs;
  IndicesManager<VDecl> vdecls;

  ShadersPreCache shadersPreCache;

  static constexpr uint32_t BINDLESS_TEXTURE_COUNT = 4096;
  static constexpr uint32_t BINDLESS_BUFFER_COUNT = 16384;
  static constexpr uint32_t BINDLESS_SAMPLER_COUNT = 512;

  struct BindlessTextureCache
  {
    struct Cache
    {
      Texture *tex = nullptr;
      id<MTLTexture> metal_tex = nil;
    };
    eastl::vector<Cache> cache;
    eastl::vector<uint64_t> id_cache;
    uint64_t default_tex = 0;

    void init(uint64_t tex) { default_tex = tex; }

    bool update(uint32_t index, D3dResource *res);

    void resize(uint32_t size)
    {
      if (cache.size() >= size)
        return;
      cache.resize(size);
      id_cache.resize(size, default_tex);
    }
  };

  struct BindlessBufferCache
  {
    struct Cache
    {
      Buffer *buf = nullptr;
      id<MTLBuffer> metal_buf = nil;
    };
    eastl::vector<Cache> cache;
    eastl::vector<uint64_t> id_cache;
    uint64_t default_buf = 0;

    void init(uint64_t buf) { default_buf = buf; }

    bool update(uint32_t index, D3dResource *res);

    void resize(uint32_t size)
    {
      if (cache.size() >= size)
        return;
      cache.resize(size);
      id_cache.resize(size, default_buf);
    }
  };

  Buffer *bindlessTexture2DIdBuffer = nullptr;
  Buffer *bindlessTextureCubeIdBuffer = nullptr;
  Buffer *bindlessTexture2DArrayIdBuffer = nullptr;
  Buffer *bindlessBufferIdBuffer = nullptr;

  BindlessTextureCache bindlessTextures2D;
  BindlessTextureCache bindlessTexturesCube;
  BindlessTextureCache bindlessTextures2DArray;
  BindlessBufferCache bindlessBuffers;

  Buffer *bindlessSamplerIdBuffer = nullptr;
  eastl::vector<id<MTLSamplerState>> bindlessSamplers;
  eastl::vector<uint64_t> bindlessSamplersCache;

  API_AVAILABLE(ios(18.0), macos(15.0)) id<MTLResidencySet> residencySet = nil;
  eastl::unordered_map<id<MTLResource>, uint32_t> resource_residency;
  uint32_t bindless_resources_bound = 0;
  bool bindless_set_dirty = true;

  void removeResource(id<MTLResource> res);
  void addResource(id<MTLResource> res);

  Render();

  bool init();

  bool isInited();

  void endFrame();
  void cleanupFrame();
  void flush(bool wait, bool present = false);

  void flushTexture(Texture *tex);

  void clearView(int what, E3DCOLOR c, float z, uint32_t stencil);

  void setViewport(int x, int y, int w, int h, float minz, float maxz);

  void setScissor(int x, int y, int w, int h);

  void setBuff(unsigned stage, BufferType bf_type, int slot, Buffer *buffer, int offset, int stride);
  void setImmediateConst(unsigned stage, const uint32_t *data, unsigned num_words);

  void setIBuff(Buffer *buffer);

  void readbackTexture(Texture *tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int offset, int pitch,
    int imageSize);
  void uploadTexture(Texture *tex_ptr, id<MTLTexture> tex, int level, int layer, int x, int y, int z, int w, int h, int d,
    id<MTLBuffer> buf, int pitch, int imageSize);
  void queueResourceForDeletion(id<MTLResource> buf);
  void queueHeapForDeletion(id<MTLHeap> heap);
  void queueCopyBuffer(Buffer *dst_buf, id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size);
  void queueUpdateBuffer(id<MTLBuffer> src, int src_offset, Buffer *dst_buf, int dst_offset, int size);

  void setTexture(unsigned stage, int slot, Texture *tex, int mip_level, int slice, bool as_uint);

  void clearTex(Texture *tex, const int val[4], int level, int layer);
  void clearTex(Texture *tex, const unsigned val[4], int level, int layer);
  void clearTex(Texture *tex, const float val[4], int level, int layer);
  void clearTexture(Texture *tex);

  bool setSrgbBackbuffer(bool set);

  void copyTex(Texture *src, Texture *dst, const RectInt *rsrc, const RectInt *rdst);
  void copyBuffer(Sbuffer *src, int srcOffset, Sbuffer *dst, int dstOffset, int size);

  void doTexCopyRegion(const TexCopyRegion &cmd);
  void doClear(Texture *dst, int dst_level, int dst_layer, float z, float color[4], bool clear_int, bool color_write,
    bool depth_write);
  void doClearTexture(uint16_t width, uint16_t height, uint8_t slices, uint8_t depth, uint8_t levels, id<MTLTexture> tex,
    int base_format, bool use_dxt);
  void doDispatch(Buffer *indirect_buffer, int offset, int tx, int ty, int tz);

  bool draw(int prim_type, int start_vertex, int vertex_count, uint32_t num_inst, uint32_t start_inst = 0);
  bool drawIndexed(int prim_type, int start_index, int index_count, int base_vertex, uint32_t num_inst, uint32_t start_inst = 0);
  bool drawIndirect(int prim_type, Sbuffer *buffer, uint32_t offset);
  bool drawIndirectIndexed(int prim_type, Sbuffer *buffer, uint32_t offset);

  bool dispatchMesh(uint32_t x, uint32_t y, uint32_t z);
  bool dispatchMeshIndirect(Buffer *args, uint32_t dispatch_count, uint32_t stride_bytes, uint32_t byte_offset);

  bool drawUP(int prim_type, int prim_count, const uint16_t *ind, const void *ptr, int numvert, int stride_bytes);

  uint64_t getTimestampResult(uint64_t timestamp);

  int createVertexShader(const uint8_t *code);
  void deleteVertexShader(int vs);

  int createPixelShader(const uint8_t *code);
  void deletePixelShader(int ps);

  int createVDdecl(VSDTYPE *d);
  void setVDecl(VDECL vdecl);
  void deleteVDecl(VDECL vdecl);

  int createProgram(int vp, int ps, VDECL vdecl, unsigned *strides, unsigned streams);
  void setProgram(int prog);
  void deleteProgram(int prog);

  void setRenderPass(bool set);

  int createComputeProgram(const uint8_t *code);
  void deleteComputeProgram(int cs);

  void clearBuffer(Buffer *buf, Buffer::BufTex *buff);
  void clearRwBuffer(Sbuffer *buff, const float *val);
  void clearRwBufferi(Sbuffer *tex, const unsigned *val);
  void dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
  void dispatch_indirect(Sbuffer *buffer, uint32_t offset);

  void generateMips(Texture *texture);
  void copyTex(Texture *src, Texture *dest);
  void copyTexRegion(Texture *src, int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d, Texture *dest,
    int dest_level, int dest_x, int dest_y, int dest_z);
  void executeUpscale(Texture *color, Texture *output, uint32_t colorMode);

  int setSampler(unsigned stage, int slot, int sampler, float bias = 0.f);

  bool updateBindlessResource(D3DResourceType range_type, uint32_t index, D3dResource *res);
  int copyBindlessResources(D3DResourceType type, uint32_t src, uint32_t dst, uint32_t count);
  int updateBindlessResourcesToNull(D3DResourceType type, uint32_t index, uint32_t count);

  void prepareBindlessResources(uint32_t requestedTypes);

  bool updateProgram();
  void updateStates();

  void executeCommands(bool wait, bool present);
  bool copyBackbuffer(id<MTLCommandBuffer> commandBuffer);

  inline void checkRenderAcquired()
  {
#if DAGOR_DBGLEVEL > 0
    G_ASSERTF(acquire_depth > 0, "depth %d", (int)acquire_depth);
#endif
  }

  void setCull(MTLCullMode cull)
  {
    if (cur_cull != cull)
      dirty_state |= DirtyFlags::Cull;
    cur_cull = cull;
  }

  void setFill(MTLTriangleFillMode fill)
  {
    if (cur_fill != fill)
      dirty_state |= DirtyFlags::Fill;
    cur_fill = fill;
  }

  /*
   * Return better metal feature set for initialized device
   * look details at developer.apple.com/documentation/metal/mtlfeatureset
   * mtl_version will be an int[4] variable,
   * mtl_version[1-2] for device id,
   * mtl_version[3-4] for metal version
   */
  int getSupportedMTLVersion(void *mtl_version);

  void setRT(int index, const RenderAttachment &attach);
  void setDepth(const RenderAttachment &attach);

  void restoreRT();
  void restoreDepth();

  bool setBlendFactor(E3DCOLOR color);

  void setConst(unsigned stage, unsigned int reg_base, const float *data, unsigned int num_regs);

  bool setStencilRef(uint32_t ref);

  bool applyStates();
  void updateEncoder(unsigned mask = 0, float *color = nullptr, float z = 0.f, uint8_t stencil = 0);
  void updateDepthState();

  void acquireOwnership();
  void releaseOwnership();
  int tryReleaseOwnerShip();

  shaders::DriverRenderStateId createRenderState(const shaders::RenderState &state);
  bool setRenderState(shaders::DriverRenderStateId id);
  void clearRenderStates();

  void MetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);
  void PrepareMetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);

  enum class EncoderType
  {
    None = 0,
    Render,
    Blit,
    Compute,
    Acceleration
  };

  EncoderType encoder_type = EncoderType::None;

  inline void ensureHaveEncoderExceptRenderFrontend(EncoderType type)
  {
    if (type != EncoderType::Render)
    {
      if (type != EncoderType::None)
        checkNoRenderPass();
      need_change_rt = 1;
      cached_viewport.originX = -FLT_MAX;
    }

    if (type == EncoderType::Compute && encoder_type != EncoderType::Compute)
    {
      storages[STAGE_CS].reset();
      current_cs_pipeline = nullptr;
    }
    encoder_type = type;
  }

  inline void ensureHaveEncoderExceptRender(id<MTLCommandBuffer> commandBuffer, EncoderType type, const char *label = nullptr)
  {
#define KILL_ENCODER(t, enc)           \
  if (type != t)                       \
  {                                    \
    if (enc != nil)                    \
    {                                  \
      submit_encoder(current_encoder); \
      [enc endEncoding];               \
      enc = nil;                       \
    }                                  \
  }
    KILL_ENCODER(EncoderType::Render, renderEncoder);
    KILL_ENCODER(EncoderType::Compute, computeEncoder);
    KILL_ENCODER(EncoderType::Blit, blitEncoder);
    KILL_ENCODER(EncoderType::Acceleration, accelerationEncoder);
#undef KILL_ENCODER

    bool created = false;
    if (type != EncoderType::Render)
    {
      if (type != EncoderType::None)
        checkNoRenderPass();
      need_change_rt = 1;
      cached_viewport.originX = -FLT_MAX;

      if (type == EncoderType::Blit)
        created = blitEncoder == nil;
      else if (type == EncoderType::Compute)
        created = computeEncoder == nil;
      else if (type == EncoderType::Acceleration)
        created = accelerationEncoder == nil;
    }
    else
      created = renderEncoder == nil;

    if (type == EncoderType::Blit && !blitEncoder)
    {
      blitEncoder = [commandBuffer blitCommandEncoder];
#if DAGOR_DBGLEVEL > 0
      blitEncoder.label = [NSString stringWithUTF8String:(label ? label : "default_blit")];
#endif
      if (blitEncoder == nil)
        DAG_FATAL("Failed to allocate blit encoder");
    }
    if (type == EncoderType::Compute && !computeEncoder)
    {
      computeEncoder = [commandBuffer computeCommandEncoder];
#if DAGOR_DBGLEVEL > 0
      computeEncoder.label = [NSString stringWithUTF8String:(label ? label : "default_compute")];
#endif
      if (computeEncoder == nil)
        DAG_FATAL("Failed to allocate compute encoder");
    }
    if (type == EncoderType::Acceleration && !accelerationEncoder)
    {
      accelerationEncoder = [commandBuffer accelerationStructureCommandEncoder];
#if DAGOR_DBGLEVEL > 0
      accelerationEncoder.label = [NSString stringWithUTF8String:(label ? label : "default_accel")];
#endif
      if (accelerationEncoder == nil)
        DAG_FATAL("Failed to allocate compute encoder");
    }
    if (created && manual_hazard_tracking)
    {
      G_ASSERT(current_encoder == nullptr);
      current_encoder = allocate_encoder();

      eastl::fill(encoders_to_wait.begin(), encoders_to_wait.end(), 0);
      eastl::fill(encoders_to_wait_vs.begin(), encoders_to_wait_vs.end(), 0);
    }
  }

  // need to keep list of those to be able to use persistent indices
  IndicesManager<RaytraceAccelerationStructure> blases;
  id<MTLAccelerationStructure> defaultBlas = nil;
  id<MTLAccelerationStructure> defaultTlas = nil;
  NSMutableArray *nativeBlases = nil;

  RaytraceAccelerationStructure *createAccelerationStructure(RaytraceBuildFlags flags, uint32_t size, bool blas);

  void deleteAccelerationStructure(RaytraceAccelerationStructure *as);

  void setTLAS(RaytraceTopAccelerationStructure *tlas, ShaderStage stage, int slot);

  void buildAccelerationStructure(RaytraceAccelerationStructure *as, MTLAccelerationStructureDescriptor *desc, Sbuffer *space_buffer,
    uint32_t space_buffer_offset, bool refit);

  void buildBLAS(RaytraceBottomAccelerationStructure *as, const ::raytrace::BottomAccelerationStructureBuildInfo &basbi);
  void buildTLAS(RaytraceTopAccelerationStructure *as, const ::raytrace::TopAccelerationStructureBuildInfo &tasbi);
  void copyAccelerationStruct(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src);

  void checkNoRenderPass();

  Query *createQuery();
  void startFence(Query *query);
  void startQuery(Query *query);
  void finishQuery(Query *query);
  uint64_t getQueryResult(Query *query, bool force_flush);
  void releaseQuery(Query *query);

  void deleteBuffer(Buffer *buff);
  void deleteTexture(Texture *tex);

  void release();
  void beginEvent(const char *name);
  void endEvent();
};

extern Render render;

struct RCAutoLock
{
  RCAutoLock() { ::enter_critical_section(render.rcSec); }
  ~RCAutoLock() { ::leave_critical_section(render.rcSec); }
};

inline void hash_combine(uint64_t &seed, uint64_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); }
} // namespace drv3d_metal
