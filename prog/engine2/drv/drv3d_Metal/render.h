#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#import <MetalKit/MetalKit.h>
#import <MetalFX/MetalFX.h>
#import <Metal/MTLAccelerationStructure.h>


#include <mutex>

#include "indicesManager.h"

#import "metalview.h"

#include "vdecl.h"
#include "buffers.h"
#include "shader.h"
#include "program.h"
#include "texture.h"
#include "shadersPreCache.h"
#include "acceleration_structure.h"

#include "buffBindPoints.h"

#include <generic/dag_carray.h>

#include <util/dag_generationReferencedData.h>
#include <osApiWrappers/dag_critSec.h>

#include <EASTL/unique_ptr.h>

#define DBREAK \
  {            \
    int k = 0; \
    k++;       \
  }

namespace drv3d_metal
{
class Render
{
public:
  static const int MAX_FRAMES_TO_RENDER = 3;

  struct Caps
  {
    bool samplerHaveCmprFun = true;
    bool drawIndxWithBaseVertes = true;
    bool readWriteTextureTier1 = true;
    bool readWriteTextureTier2 = true;
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
    uint32_t cull = 0;
    uint32_t depth_clip = 0;
  };

  static MTLDepthStencilDescriptor *depthStateDesc;
#if USE_METALFX_UPSCALE
  struct MTLFXUpscale
  {
    API_AVAILABLE(ios(16.0), macos(13.0))
    constexpr static MTLFXSpatialScalerColorProcessingMode color_modes[] = {MTLFXSpatialScalerColorProcessingModePerceptual,
      MTLFXSpatialScalerColorProcessingModeLinear, MTLFXSpatialScalerColorProcessingModeHDR};
    API_AVAILABLE(ios(16.0), macos(13.0)) MTLFXSpatialScalerDescriptor *spatialScalerDescriptor = NULL;
    API_AVAILABLE(ios(16.0), macos(13.0)) id<MTLFXSpatialScaler> spatialScaler = nil;
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
    int device_buffer_offset;
    id<MTLBuffer> is_binded = nil;
    int is_binded_offset = -1;

    int cbuffer_offset;
    uint8_t *cbuffer = nullptr;

    static int shared_cmd_offset;
    static int shared_cmd_size;
    static uint8_t *shared_cmd;

    int num_strored;

    void init();
    void destroy();
    void applyCmd(void *data, int reg_base, int num_regs);
    void *storeCmd(const float *data, int reg_num);
    void applyBuffer(unsigned stage, int num_reg);
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
    static const uint32_t max_size = 3 * 1024 * 1024;
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
    Tab<id<MTLAccelerationStructure>> acceleration_structures;
    Tab<RingBufferItem> constant_buffers;
  };

  MTLViewport cached_viewport;

  std::mutex constant_buffer_lock, delete_lock, copy_tex_lock, tlas_lock, blas_lock;
  FrameResources2Delete resources2delete;
  FrameResources2Delete resources2delete_frames[MAX_FRAMES_TO_RENDER];

  Tab<RingBufferItem> constant_buffers_free;
  RingBufferItem constant_buffer;

  Tab<UploadTex> textures2upload;
  Tab<CopyBuf> buffers2copy;

  id<MTLBuffer> AllocateConstants(uint32_t size, int &offset, int sizeLeft = 0)
  {
    G_ASSERT(size <= RingBufferItem::max_size);

    std::lock_guard<std::mutex> scopedLock(constant_buffer_lock);

    size = (size + 255) & ~255; // align size since we're padding anyways
    if (!constant_buffer.buf || constant_buffer.offset + size > RingBufferItem::max_size ||
        constant_buffer.offset + sizeLeft > RingBufferItem::max_size)
    {
      if (constant_buffer.buf)
        resources2delete.constant_buffers.push_back(constant_buffer);
      if (constant_buffers_free.empty())
      {
        constant_buffer.buf = [device newBufferWithLength:RingBufferItem::max_size
                                                  options:MTLResourceCPUCacheModeWriteCombined | MTLResourceStorageModeShared];
#if DAGOR_DBGLEVEL > 0
        constant_buffer.buf.label = @"ring buffer";
#endif
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

  uint64_t cur_thread;
  uint64_t main_thread;
  int acquare_depth;

  int max_commands = 0;

  char device_name[128];

  MetalView *mainview;

  id<MTLDevice> device;
  id<MTLCommandQueue> commandQueue;

  id<MTLCommandBuffer> commandBuffer;
  id<MTLRenderCommandEncoder> renderEncoder;
  id<MTLBlitCommandEncoder> blitEncoder;
  id<MTLComputeCommandEncoder> computeEncoder;
  id<MTLAccelerationStructureCommandEncoder> accelerationEncoder;

  bool async_pso_compilation = false;
  bool using_async_pso_compilation = false;
  bool can_use_async_pso_compilation = false;
  uint32_t async_pso_compilation_length = 0;

  id<MTLComputePipelineState> clear_cs_pipeline = nil;
  id<MTLComputePipelineState> copy_cs_pipeline = nil;

  MTLRenderPassDescriptor *render_desc;

  id<MTLBuffer> query_buffer;
  id<MTLBuffer> stub_buffer = nil;
  int cur_query_offset;
  int need_set_qeury;

  id<MTLBuffer> tmp_copy_buff;
  bool drawable_aquared;

  bool start_capture;
  bool save_backBuffer;

  static constexpr int QUERIES_MAX = 8192;

  int used_query;
  carray<Query, QUERIES_MAX> queries;

  CritSecStorage aquareSec;
  CritSecStorage rcSec;

  int ibuffertype;
  Buffer *ibuffer;

  VDecl *vdecl;

  Texture *blank_tex[5];
  Texture *blank_tex_rw[5];

  struct RenderTargetConfig
  {
    Texture *rt[Program::MAX_SIMRT];
    int rt_levels[Program::MAX_SIMRT];
    int rt_layers[Program::MAX_SIMRT];
    Texture *depth;
    int depth_layer = 0;
    bool depth_readonly = false;
    Viewport vp;
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

    id<MTLAccelerationStructure> acceleration_structures[BUFFER_POINT_COUNT];

    uint32_t immediate_dwords_metal[4] = {~0u};
    int immediate_slot = -1;

    uint32_t immediate_dwords[4] = {0};
    uint32_t immediate_dword_count = 0;

    Texture *textures[MAX_SHADER_TEXTURES * 2];
    bool textures_read_stencil[MAX_SHADER_TEXTURES * 2];
    int textures_mip_level[MAX_SHADER_TEXTURES * 2];
    id<MTLSamplerState> samplers_metal[MAX_SHADER_TEXTURES * 2];
    id<MTLTexture> textures_metal[MAX_SHADER_TEXTURES * 2];

    ConstBuffer cbuffer;

    __forceinline void setTex(int slot, Texture *tex, bool read_stencil = false, int mip = 0)
    {
      G_ASSERT(slot < MAX_SHADER_TEXTURES * 2);
      textures[slot] = tex;
      textures_read_stencil[slot] = read_stencil;
      textures_mip_level[slot] = mip;
    }

    __forceinline void setBuf(int slot, Buffer *buf, int offset = 0)
    {
      G_ASSERT(slot < BUFFER_POINT_COUNT);
      buffers[slot] = buf;
      buffers_offset[slot] = offset;
    }

    __forceinline void setAccStruct(int slot, RaytraceTopAccelerationStructure *tlas)
    {
      G_ASSERT(slot < BUFFER_POINT_COUNT);
      if (tlas)
        acceleration_structures[slot] = tlas->instanceAccelerationStructure;
      else
        acceleration_structures[slot] = nil;
    }

    void apply(unsigned stage, Shader *shader);
    void reset(bool full = false);
    void resetBuffer(int buf);
    void resetBuffers(Shader *old_shader, Shader *shader);
    void removeBuf(Buffer *buf);
    void removeTex(Texture *tex);
  };

  StageBinding stages[3];

  bool msaa_pass = false;
  bool depth_resolve = false;
  bool forceClearOnCreate = false;

  int depth_clip;
  int stencil_ref = 0;

  float depth_bias;
  float depth_slopebias;
  int bias_changed;
  int stencil_ref_changed;

  int cur_cull = 2;
  int need_set_cull = 1;

  bool inited;

  int scr_wd;
  int scr_ht;

  int wnd_wd = 0;
  int wnd_ht = 0;

  int cur_wd;
  int cur_ht;

  int scr_bpp;
  bool cur_vsync;

  DepthState cur_depthstate;
  Tab<DepthState> dstate_cache;
  bool need_prepare_depth_state;

  bool scissor_on;
  bool need_set_scissor;
  int sci_x, sci_y, sci_w, sci_h;
  uint64_t frame = 0;

  uint32_t number_of_frames_to_skip_after_error = 0;
  uint32_t max_number_of_frames_to_skip_after_error = 0;
  std::atomic<int> hadError;

  uint64_t frame_completed = 0;
  uint64_t frame_scheduled = 0;
  uint64_t frames_completed[MAX_FRAMES_TO_RENDER] = {0, 0, 0};
  std::mutex frame_mutex;
  std::condition_variable frame_condition;

  Program::RenderState cur_rstate;
  id<MTLRenderPipelineState> cur_state;
  Program::RenderState last_rstate;

  Program *cur_prog;
  id<MTLComputePipelineState> current_cs_pipeline = nil;

  GenerationReferencedData<shaders::DriverRenderStateId, CachedRenderState> render_states;

  IndicesManager<Shader> shaders;
  IndicesManager<Program> progs;
  IndicesManager<VDecl> vdecls;

  ShadersPreCache shadersPreCache;

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

  void discardBuffer(Buffer *buffer, Buffer::BufTex *buftex, id<MTLBuffer> buf, int dynamic_offset);
  void readbackTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int offset, int pitch,
    int imageSize);
  void uploadTexture(id<MTLTexture> tex, int level, int layer, int w, int h, int d, id<MTLBuffer> buf, int pitch, int imageSize);
  void queueBufferForDeletion(id<MTLBuffer> buf);
  void queueTextureForDeletion(id<MTLTexture> tex);
  void queueCopyBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size);
  void queueUpdateBuffer(id<MTLBuffer> src, int src_offset, id<MTLBuffer> dst, int dst_offset, int size);

  void setTexture(unsigned stage, int slot, Texture *tex, int mip_level);

  void clearTex(Texture *tex, const unsigned val[4], int level, int layer);
  void clearTex(Texture *tex, const float val[4], int level, int layer);
  void clearTexture(Texture *tex);

  void swapTextures(Texture *src, BaseTexture *dst);

  void copyTex(Texture *src, Texture *dst, RectInt *rsrc, RectInt *rdst);
  void copyBuffer(Sbuffer *src, int srcOffset, Sbuffer *dst, int dstOffset, int size);

  void doTexCopyRegion(const struct CommandTexCopyRegion &cmd);
  void doCopyTexture(Texture *dst, Texture *src, float *src_rect, int *vp);
  void doClear(Texture *dst, int dst_level, int dst_layer, float z, float color[4], bool clear_int, bool color_write,
    bool depth_write);
  void doClearTexture(Texture *tex);
  void doTexCopy(Texture *dst, Texture *src);
  void doDispatch(Buffer *indirect_buffer, int offset, int tx, int ty, int tz);
  void doSetState(shaders::DriverRenderStateId state_id);
  void doSetProgram(int prog, bool is_async);

  bool draw(int prim_type, int start_vertex, int vertex_count, uint32_t num_inst, uint32_t start_inst = 0);
  bool drawIndexed(int prim_type, int start_index, int index_count, int base_vertex, uint32_t num_inst, uint32_t start_inst = 0);
  bool drawIndirect(int prim_type, Sbuffer *buffer, uint32_t offset);
  bool drawIndirectIndexed(int prim_type, Sbuffer *buffer, uint32_t offset);

  bool drawUP(int prim_type, int prim_count, const uint16_t *ind, const void *ptr, int numvert, int stride_bytes);

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

  void setMSAAPass();
  void setDepthResolve();

  int createComputeProgram(const uint8_t *code);
  void deleteComputeProgram(int cs);

  void clearBuffer(Buffer::BufTex *buff);
  void clearRwBuffer(Sbuffer *buff, const float val[4]);
  void clearRwBufferi(Sbuffer *tex, const unsigned val[4]);
  void dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z);
  void dispatch_indirect(Sbuffer *buffer, uint32_t offset);

  int setTextureAddr(Texture *tex, int u, int v, int w);
  int setTextureBorder(Texture *tex, E3DCOLOR color);
  int setTextureFilter(Texture *tex, int filter);
  int setTextureMipmapFilter(Texture *tex, int filter);
  int setTextureAnisotropy(Texture *tex, int level);
  void setTextureMipLevel(Texture *texture, int minlevel, int maxlevel);
  void generateMips(Texture *texture);
  void copyTex(Texture *src, Texture *dest);
  void copyTexRegion(Texture *src, int src_level, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d, Texture *dest,
    int dest_level, int dest_x, int dest_y, int dest_z);
  void executeUpscale(Texture *color, Texture *output, uint32_t colorMode);

  void setCull(int cull)
  {
    cur_cull = cull;
    need_set_cull = 1;
  }

  /*
   * Return better metal feature set for initialized device
   * look details at developer.apple.com/documentation/metal/mtlfeatureset
   * mtl_version will be an int[4] variable,
   * mtl_version[1-2] for device id,
   * mtl_version[3-4] for metal version
   */
  int getSupportedMTLVersion(void *mtl_version);

  void setRT(int index, Texture *rt, int level, int layer);
  void setDepth(Texture *depth, int layer, bool ro);

  void restoreRT();
  void restoreDepth();

  bool setBlendFactor(E3DCOLOR color);

  void setConst(unsigned stage, unsigned int reg_base, const float *data, unsigned int num_regs);

  bool setStencilRef(uint32_t ref);

  bool applyStates();
  void updateEncoder(unsigned mask = 0, float *color = nullptr, float z = 0.f, uint8_t stencil = 0);
  void updateDepthState();

  void aquareOwnerShip();
  void releaseOwnerShip();
  int tryReleaseOwnerShip();

  shaders::DriverRenderStateId createRenderState(const shaders::RenderState &state);
  bool setRenderState(shaders::DriverRenderStateId id);
  void clearRenderStates();

  void MetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);
  void PrepareMetalfxUpscale(Texture *color, Texture *output, uint32_t colorMode);

  inline void EndRenderEncoding(id<MTLRenderCommandEncoder> &renderEncoder)
  {
    if (renderEncoder != nil)
    {
      [renderEncoder endEncoding];
      renderEncoder = nil;
    }
  }

  inline void EndBlitEncoding(id<MTLBlitCommandEncoder> &blitEncoder)
  {
    if (blitEncoder != nil)
    {
      [blitEncoder endEncoding];
      blitEncoder = nil;
    }
  }

  inline void EndComputeEncoding(id<MTLComputeCommandEncoder> &computeEncoder)
  {
    if (computeEncoder != nil)
    {
      [computeEncoder endEncoding];
      computeEncoder = nil;
    }
  }

  inline void EndAccelerationEncoding(id<MTLAccelerationStructureCommandEncoder> &accelerationEncoder)
  {
    if (accelerationEncoder != nil)
    {
      [accelerationEncoder endEncoding];
      accelerationEncoder = nil;
    }
  }

  RaytraceTopAccelerationStructure *createTopAccelerationStructure();
  RaytraceBottomAccelerationStructure *createBottomAccelerationStructure();

  eastl::vector<eastl::unique_ptr<RaytraceTopAccelerationStructure>> topStructs{};

  // So vulkan and dx both have a different approach to metal for binding tlas with its blases.
  // In vulkan and dx instance is basically a transform + POINTER to blas.
  // But in metal it is a transform + INDEX IN ARRAY.
  // So blases should be stored in the separate NSMutableArray that is fed to tlas.
  // But our API has write_raytrace_index_entries_to_memory that has to happen in main thread
  // during instance buffer lock. Which means all indexes should be set at that moment.
  // The idea is simple: vector is updated on main thread and NSMutableArray is updated
  // in a deferred way on command buffer thread via commands.
  // That way every tlas build will share the array but will reference only required blases.
  // It is unknown if it is a faster method and should be tested with multiple raytracing systems,
  // but there is only 1 at the time of writing this code, and it is written in 1 file with statics
  // which makes it impossible to dublicate for testing.
  eastl::vector<eastl::unique_ptr<RaytraceBottomAccelerationStructure>> bottomStructs{};
  NSMutableArray *bottomLevelAccelerationStructures;

  void scheduleTLASSet(RaytraceTopAccelerationStructure *tlas, ShaderStage stage, int slot);

  void deleteTLAS(RaytraceTopAccelerationStructure *tlas);
  void deleteBLAS(RaytraceBottomAccelerationStructure *blas);

  void addBLASStubToList();
  void removeBLASFromList(int delete_index);

  id<MTLAccelerationStructure> createAccelerationStructure(MTLAccelerationStructureDescriptor *desc);

  void scheduleBLASBuild(RaytraceBottomAccelerationStructure *blas, RaytraceGeometryDescription *desc);
  void buildBLAS(RaytraceBottomAccelerationStructure *blas, RaytraceGeometryDescription *desc);

  void scheduleTLASBuild(RaytraceTopAccelerationStructure *tlas, Sbuffer *instance_descriptor_buffer, uint32_t instance_count);
  void buildTLAS(RaytraceTopAccelerationStructure *blas, Sbuffer *instance_descriptor_buffer, uint32_t instance_count);

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
