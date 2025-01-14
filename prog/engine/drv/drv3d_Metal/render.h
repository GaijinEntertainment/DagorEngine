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

#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_critSec.h>

#include <EASTL/unique_ptr.h>

struct RaytraceAccelerationStructure
{
  id<MTLResource> acceleration_struct = nil;
  int index = -1;
  bool was_built = false;
  RaytraceBuildFlags createFlags = RaytraceBuildFlags::NONE;
};

struct RaytraceTopAccelerationStructure : RaytraceAccelerationStructure
{};
struct RaytraceBottomAccelerationStructure : RaytraceAccelerationStructure
{};

namespace drv3d_metal
{

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
  Query = 1 << 6
};
}

struct TexCopyRegion
{
  id<MTLTexture> dst;
  id<MTLTexture> src;
  MTLPixelFormat src_format;
  MTLPixelFormat dst_format;
  uint16_t dest_x, dest_y, dest_z;
  uint16_t dest_level;
  uint16_t dest_levels;
  uint16_t src_level;
  uint8_t src_levels;
  uint8_t src_dxt;
  uint16_t src_x, src_y, src_z;
  uint16_t src_w, src_h, src_d;

  TexCopyRegion(Texture *dst, int dest_level, int dest_x, int dest_y, int dest_z, Texture *src, int src_level, int src_x, int src_y,
    int src_z, int src_w, int src_h, int src_d) :
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

class Render
{
public:
  static constexpr int MAX_FRAMES_TO_RENDER = 2;
  static constexpr int MAX_CBUFFER_SIZE = 64 * 1024;

  struct Caps
  {
    bool samplerHaveCmprFun = true;
    bool drawIndxWithBaseVertes = true;
    bool readWriteTextureTier1 = true;
    bool readWriteTextureTier2 = true;
  };

  struct ClearTexOnCreate
  {
    id<MTLTexture> metalTex = nil;
    uint32_t slices = 1;
    uint32_t depth = 1;
    uint32_t levels = 1;
    uint32_t width = 1;
    uint32_t height = 1;
    int base_format = 0;
    bool use_dxt = false;

    ClearTexOnCreate(Texture *tex)
    {
      G_ASSERT(tex);
      if (tex->type == RES3D_ARRTEX)
        slices = tex->depth;
      else if (tex->type == RES3D_CUBETEX)
        slices = 6;
      else if (tex->type == RES3D_CUBEARRTEX)
        slices = 6 * tex->depth;

      if (tex->type == RES3D_VOLTEX)
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

  class ConstBuffer
  {
  public:
    id<MTLBuffer> is_binded = nil;
    int device_buffer_offset = 0;
    int is_binded_offset = -1;

    uint8_t cbuffer[MAX_CBUFFER_SIZE];
    uint8_t cbuffer_cache[MAX_CBUFFER_SIZE];

    int num_stored = 0;
    int num_last_bound = 0;

    void init();
    void destroy();
    void applyCmd(void *data, int reg_base, int num_regs);

    template <int stage>
    void applyBuffer(int num_reg);
  };

  struct UploadTex
  {
    id<MTLTexture> tex;
    id<MTLBuffer> buf;
    uint32_t pitch;
    uint32_t imageSize;
    uint16_t w;
    uint16_t h;
    uint16_t d;
    uint8_t level;
    uint8_t layer;
  };

  struct CopyBuf
  {
    id<MTLBuffer> src;
    id<MTLBuffer> dst;
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

  struct FrameResources2Delete
  {
    Tab<Texture *> textures;
    Tab<Buffer *> buffers;
    Tab<int> vdecls;
    Tab<int> shaders;
    Tab<int> programs;
    Tab<id<MTLResource>> native_resources;
    Tab<RaytraceAccelerationStructure *> acceleration_structs;

    Tab<RingBufferItem> constant_buffers;
    Tab<RingBufferItem> dynamic_buffers;
  };

  MTLViewport cached_viewport;

  std::mutex dynamic_buffer_lock, delete_lock, copy_tex_lock;
  FrameResources2Delete resources2delete;
  FrameResources2Delete resources2delete_frames[MAX_FRAMES_TO_RENDER];

  Tab<RingBufferItem> constant_buffers_free;
  RingBufferItem constant_buffer;

  Tab<RingBufferItem> dynamic_buffers_free;
  RingBufferItem dynamic_buffer;

  Tab<UploadTex> textures2upload;
  Tab<CopyBuf> buffers2copy;
  Tab<TexCopyRegion> regions2copy;
  Tab<ClearTexOnCreate> textures2clear;
  Tab<Buffer::BufTex *> buffers2clear;

  id<MTLBuffer> createBuffer(uint32_t size, uint32_t flags, const char *name = "");

  id<MTLBuffer> AllocateConstants(uint32_t size, int &offset, int sizeLeft = 0)
  {
    G_ASSERT(size <= RingBufferItem::max_constant_size);

    size = (size + 31) & ~31; // align size since we're padding anyways
    if (!constant_buffer.buf || constant_buffer.offset + size > RingBufferItem::max_constant_size ||
        constant_buffer.offset + sizeLeft > RingBufferItem::max_constant_size)
    {
      if (constant_buffer.buf)
        resources2delete.constant_buffers.push_back(constant_buffer);
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
        resources2delete.dynamic_buffers.push_back(dynamic_buffer);
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

  id<MTLCommandBuffer> commandBuffer;
  id<MTLRenderCommandEncoder> renderEncoder = nil;
  id<MTLBlitCommandEncoder> blitEncoder = nil;
  id<MTLComputeCommandEncoder> computeEncoder = nil;
  API_AVAILABLE(ios(15.0), macos(11.0)) id<MTLAccelerationStructureCommandEncoder> accelerationEncoder = nil;

  bool async_pso_compilation = false;
  bool using_async_pso_compilation = false;
  bool can_use_async_pso_compilation = false;
  Tab<bool> async_pso_compilation_stack;
  uint32_t async_pso_compilation_length = 0;
  void pushAsyncPsoCompilation(bool enable);
  void popAsyncPsoCompilation();

  id<MTLComputePipelineState> clear_cs_pipeline = nil;
  id<MTLComputePipelineState> copy_cs_pipeline = nil;

  id<MTLBuffer> query_buffer;
  id<MTLBuffer> stub_buffer = nil;
  int cur_query_offset = -1;

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

  struct RenderTargetConfig
  {
    RenderAttachment colors[Program::MAX_SIMRT];
    RenderAttachment depth;
    Viewport vp;

    bool isFullscreenViewport() const;
  };
  RenderTargetConfig rt;
  int need_change_rt;

  class StageBinding
  {
  public:
    Buffer *buffers[BUFFER_POINT_COUNT];
    int buffers_offset[BUFFER_POINT_COUNT];
    id<MTLBuffer> buffers_metal[BUFFER_POINT_COUNT];
    int buffers_metal_offset[BUFFER_POINT_COUNT];

    static_assert(drv3d_metal::BUFFER_POINT_COUNT <= 64, "not enough bits for buffers");
    uint64_t buffer_dirty_mask = 0;

    id<MTLResource> acceleration_structures[MAX_SHADER_ACCELERATION_STRUCTURES];

    uint32_t immediate_dwords_metal[4] = {~0u};
    int immediate_slot_metal = -1;
    uint32_t immediate_dword_count_metal = 0;

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
    };

    // first MAX_SHADER_TEXTURES is ordinary textures
    // second MAX_SHADER_TEXTURES are uav textures
    static constexpr uint32_t MAX_STAGE_TEXTURES = MAX_SHADER_TEXTURES * 2;
    TextureSlot textures[MAX_STAGE_TEXTURES];
    id<MTLTexture> textures_metal[MAX_STAGE_TEXTURES] = {};
    uint64_t texture_dirty_mask = 0;

    id<MTLSamplerState> samplers[MAX_STAGE_TEXTURES];
    id<MTLSamplerState> samplers_metal[MAX_STAGE_TEXTURES];
    uint64_t sampler_dirty_mask = 0;

    ConstBuffer cbuffer;

    __forceinline void setTex(int slot, Texture *tex, bool read_stencil = false, int mip = 0, bool as_uint = false,
      bool use_sampler = true)
    {
      G_ASSERT(slot < MAX_STAGE_TEXTURES);
      G_ASSERT(!as_uint || slot >= MAX_SHADER_TEXTURES);
      auto &cache = textures[slot];

      if (cache.texture != tex || cache.read_stencil != read_stencil || cache.as_uint != as_uint || cache.mip_level != mip)
        texture_dirty_mask |= 1ull << slot;

      cache.texture = tex;
      cache.read_stencil = read_stencil;
      cache.as_uint = as_uint;
      cache.mip_level = mip;
      if (tex && use_sampler)
      {
        cache.source = SamplerSource::Texture;

        id<MTLSamplerState> sampler = nil;
        tex->applySampler(sampler);

        if (sampler != samplers[slot])
          sampler_dirty_mask |= 1ull << slot;

        samplers[slot] = sampler;
      }
    }

    __forceinline void setSampler(int slot, id<MTLSamplerState> sampler)
    {
      if (sampler)
      {
        if (samplers[slot] != sampler)
          sampler_dirty_mask |= 1ull << slot;
        textures[slot].source = SamplerSource::Sampler;
      }
      samplers[slot] = sampler;
    }

    __forceinline void setBuf(int slot, Buffer *buf, int offset = 0)
    {
      G_ASSERT(slot < BUFFER_POINT_COUNT);
      if (buffers[slot])
        buffers[slot]->bound_slots &= 1ull << slot;

      int check_offset = buf ? buf->getDynamicOffset() + offset : offset;
      if (buffers[slot] != buf || buffers_offset[slot] != check_offset)
      {
        if (buf && buf->getTexture())
          texture_dirty_mask |= 1ull << slot;
        else
          buffer_dirty_mask |= 1ull << slot;
      }

      buffers[slot] = buf;
      buffers_offset[slot] = offset;

      if (buffers[slot])
        buffers[slot]->bound_slots |= 1ull << slot;
    }

    __forceinline void markDirty(Buffer *buf)
    {
      G_ASSERT(buf);
      buffer_dirty_mask |= buf->bound_slots;
    }

    __forceinline void setAccStruct(int slot, id<MTLResource> tlas)
    {
      G_ASSERT(slot < MAX_SHADER_ACCELERATION_STRUCTURES);
      acceleration_structures[slot] = tlas;
    }

    void bindVertexStream(uint32_t stream, uint32_t offset);

    template <int stage>
    void apply(Shader *shader);
    template <int stage>
    void apply_vertex_streams();
    template <int stage>
    void apply_buffers(Shader *shader);
    template <int stage>
    void apply_textures(Shader *shader);
    template <int stage>
    void apply_samplers(Shader *shader);
    template <int stage>
    void apply_acceleration_structs(Shader *shader);

    void reset(bool full = false);
    void resetBuffer(int buf);
    void resetBuffers();
    void removeBuf(Buffer *buf);
    void removeTex(Texture *tex);
  };

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
  BindlessManager bindlessManager;

  void markBufferDirty(Buffer *buf)
  {
    stages[STAGE_VS].markDirty(buf);
    stages[STAGE_PS].markDirty(buf);
    stages[STAGE_CS].markDirty(buf);
  }

  bool forceClearOnCreate = false;
  bool render_pass = false;
  bool has_image_blocks = false;

  MTLDepthClipMode depth_clip = MTLDepthClipModeClip;
  int stencil_ref = 0;

  float depth_bias;
  float depth_slopebias;

  MTLCullMode cur_cull = MTLCullModeBack;

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

  bool validate_framemem_bounds = false;

  uint32_t number_of_frames_to_skip_after_error = 0;
  uint32_t max_number_of_frames_to_skip_after_error = 0;
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
  static constexpr uint32_t BINDLESS_BUFFER_COUNT = 4096;
  static constexpr uint32_t BINDLESS_SAMPLER_COUNT = 512;

  Buffer *bindlessTextureIdBuffer = nullptr;
  eastl::array<Texture *, BINDLESS_TEXTURE_COUNT> bindlessTextures;
  uint32_t maxBindlessTextureId = 0;
  Buffer *bindlessBufferIdBuffer = nullptr;
  eastl::array<Buffer *, BINDLESS_BUFFER_COUNT> bindlessBuffers;
  uint32_t maxBindlessBufferId = 0;

  Buffer *bindlessSamplerIdBuffer = nullptr;
  eastl::array<id<MTLSamplerState>, BINDLESS_SAMPLER_COUNT> bindlessSamplers;
  uint32_t maxBindlessSamplerId = 0;

  eastl::vector<id<MTLResource>> resourcesToUse;

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

  void readbackTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int offset, int pitch,
    int imageSize);
  void uploadTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int pitch, int imageSize);
  void queueBufferForDeletion(id<MTLBuffer> buf);
  void queueTextureForDeletion(id<MTLTexture> tex);
  void queueCopyBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size);
  void queueUpdateBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size);

  void setTexture(unsigned stage, int slot, Texture *tex, bool use_sampler, int mip_level, bool as_uint);

  void clearTex(Texture *tex, const int val[4], int level, int layer);
  void clearTex(Texture *tex, const unsigned val[4], int level, int layer);
  void clearTex(Texture *tex, const float val[4], int level, int layer);
  void clearTexture(Texture *tex);

  bool setSrgbBackbuffer(bool set);

  void copyTex(Texture *src, Texture *dst, RectInt *rsrc, RectInt *rdst);
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

  bool drawUP(int prim_type, int prim_count, const uint16_t *ind, const void *ptr, int numvert, int stride_bytes);
  void flushQueuedCommands();

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

  void clearBuffer(Buffer::BufTex *buff);
  void clearRwBuffer(Sbuffer *buff, const float *val);
  void clearRwBufferi(Sbuffer *tex, const unsigned *val);
  void dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
  void dispatch_indirect(Sbuffer *buffer, uint32_t offset);

  void generateMips(Texture *texture);
  void copyTex(Texture *src, Texture *dest);
  void copyTexRegion(Texture *src, int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d, Texture *dest,
    int dest_level, int dest_x, int dest_y, int dest_z);
  void executeUpscale(Texture *color, Texture *output, uint32_t colorMode);

  int setSampler(unsigned stage, int slot, id<MTLSamplerState> smp);

  int updateBindlessResource(uint32_t index, D3dResource *res);
  int copyBindlessResources(uint32_t resourceType, uint32_t src, uint32_t dst, uint32_t count);
  int updateBindlessResourcesToNull(uint32_t resourceType, uint32_t index, uint32_t count);

  void prepareBindlessResources(eastl::vector<id<MTLResource>> &resourcesToUse);
  void prepareBindlessResourcesGraphics(id<MTLRenderCommandEncoder> renderCommandEncoder);
  void prepareBindlessResourcesCompute(id<MTLComputeCommandEncoder> renderComputeEncoder);
  void setBindlessResources();

  bool updateProgram();
  void updateStates();

  void setCull(MTLCullMode cull)
  {
    if (cur_cull != cull)
      dirty_state |= DirtyFlags::Cull;
    cur_cull = cull;
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

  void acquireOwnerShip();
  void releaseOwnerShip();
  int tryReleaseOwnerShip();

  shaders::DriverRenderStateId createRenderState(const shaders::RenderState &state);
  bool setRenderState(shaders::DriverRenderStateId id);
  void clearRenderStates();

  void MetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);
  void PrepareMetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);

#if DAGOR_DBGLEVEL > 0
  void put_encoder_label(const char *label);
  id<MTLCommandBuffer> wait_for_encoder();
#else
  void put_encoder_label(const char *label) {}
  id<MTLCommandBuffer> wait_for_encoder() { return commandBuffer; }
#endif

  enum class EncoderType
  {
    None = 0,
    Render,
    Blit,
    Compute,
    Acceleration
  };
  inline void ensureHaveEncoderExceptRender(id<MTLCommandBuffer> cmdBuf, EncoderType type, bool wait = true,
    const char *label = nullptr)
  {
#define KILL_ENCODER(t, enc)         \
  if (type != t)                     \
  {                                  \
    if (enc != nil)                  \
    {                                \
      [enc endEncoding];             \
      enc = nil;                     \
      if (wait)                      \
        cmdBuf = wait_for_encoder(); \
    }                                \
  }
    KILL_ENCODER(EncoderType::Render, renderEncoder);
    KILL_ENCODER(EncoderType::Compute, computeEncoder);
    KILL_ENCODER(EncoderType::Blit, blitEncoder);
    if (@available(iOS 15.0, macos 11.0, *))
    {
      KILL_ENCODER(EncoderType::Acceleration, accelerationEncoder);
    }
#undef KILL_ENCODER

    if (type != EncoderType::Render)
    {
      if (type != EncoderType::None)
        checkNoRenderPass();
      need_change_rt = 1;
      cached_viewport.originX = -FLT_MAX;
    }
    if (type == EncoderType::Blit && !blitEncoder)
    {
      blitEncoder = [cmdBuf blitCommandEncoder];
#if DAGOR_DBGLEVEL > 0
      if (label)
        blitEncoder.label = [NSString stringWithUTF8String:label];
#endif
    }
    if (type == EncoderType::Compute && !computeEncoder)
    {
      computeEncoder = [cmdBuf computeCommandEncoder];
#if DAGOR_DBGLEVEL > 0
      if (label)
        computeEncoder.label = [NSString stringWithUTF8String:label];
#endif
      stages[STAGE_CS].reset();
      prepareBindlessResourcesCompute(computeEncoder);
    }
    if (@available(iOS 15.0, macos 11.0, *))
    {
      if (type == EncoderType::Acceleration && !accelerationEncoder)
        accelerationEncoder = [cmdBuf accelerationStructureCommandEncoder];
    }
  }

  // need to keep list of those to be able to use persistent indices
  IndicesManager<RaytraceAccelerationStructure> blases;
  id<MTLResource> defaultBlas = nil;
  NSMutableArray *bottomLevelAccelerationStructures = nil;

  RaytraceAccelerationStructure *createAccelerationStructure(RaytraceBuildFlags flags, bool blas);

  void deleteAccelerationStructure(RaytraceAccelerationStructure *as);

  void setTLAS(RaytraceTopAccelerationStructure *tlas, ShaderStage stage, int slot);

  API_AVAILABLE(ios(15.0), macos(11.0))
  void buildAccelerationStructure(RaytraceAccelerationStructure *as, MTLAccelerationStructureDescriptor *desc, Sbuffer *space_buffer,
    uint32_t space_buffer_offset, bool refit);

  void buildBLAS(RaytraceBottomAccelerationStructure *as, const ::raytrace::BottomAccelerationStructureBuildInfo &basbi);
  void buildTLAS(RaytraceTopAccelerationStructure *as, const ::raytrace::TopAccelerationStructureBuildInfo &tasbi);

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
  uint64_t getGPUTimeStampFreq();
};

extern Render render;

struct RCAutoLock
{
  RCAutoLock() { ::enter_critical_section(render.rcSec); }
  ~RCAutoLock() { ::leave_critical_section(render.rcSec); }
};

inline void hash_combine(uint64_t &seed, uint64_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); }
} // namespace drv3d_metal
