
extern bool g_ios_pause_rendering;

namespace drv3d_metal
{
#define DECLARE(type) Type_##type
enum CommandType
{
  DECLARE(Invalid),
  DECLARE(CommandClear),
  DECLARE(CommandClearTex),
  DECLARE(CommandClearTexture),
  DECLARE(CommandCopyTex),
  DECLARE(CommandSetViewport),
  DECLARE(CommandSetViewportNoZ),
  DECLARE(CommandSetScissor),
  DECLARE(CommandSetVDecl),
  DECLARE(CommandReadbackTex),
  DECLARE(CommandSetImmediate),
  DECLARE(CommandSetBuf),
  DECLARE(CommandSetIB),
  DECLARE(CommandBufDiscard),
  DECLARE(CommandSetConst),
  DECLARE(CommandSetTex),
  DECLARE(CommandSetProg),
  DECLARE(CommandSetTexMipLevel),
  DECLARE(CommandTexGenMips),
  DECLARE(CommandTexCopy),
  DECLARE(CommandTexCopyRegion),
  DECLARE(CommandStencilRef),
  DECLARE(CommandSetState),
  DECLARE(CommandSetRT),
  DECLARE(CommandSetDepth),
  DECLARE(CommandSetQuery),
  DECLARE(CommandSetFence),
  DECLARE(CommandSetTLAS),
  DECLARE(CommandBuildTLAS),
  DECLARE(CommandBuildBLAS),
  DECLARE(CommandAddBLASToList),
  DECLARE(CommandRemoveBLASFromList),
  DECLARE(CommandCopyBuffer),
  DECLARE(CommandUpdateBuffer),
  DECLARE(CommandDraw),
  DECLARE(CommandDrawIndirect),
  DECLARE(CommandDrawIndexed),
  DECLARE(CommandDrawIndexedIndirect),
  DECLARE(CommandDispatch),
  DECLARE(CommandDispatchIndirect),
  DECLARE(CommandClearRW),
  DECLARE(CommandClearBuffer),
  DECLARE(CommandSwapTextures),
  DECLARE(CommandSetBlendFactor),
  DECLARE(CommandSetTextureAddr),
  DECLARE(CommandSetTextureBorder),
  DECLARE(CommandSetTextureFilter),
  DECLARE(CommandSetTextureAniso),
  DECLARE(CommandSetMSAAPass),
  DECLARE(CommandSetDepthResolve),
  DECLARE(CommandDeleteNativeBuffer),
  DECLARE(CommandDeleteBuffer),
  DECLARE(CommandDeleteNativeTexture),
  DECLARE(CommandDeleteTexture),
  DECLARE(CommandBeginEvent),
  DECLARE(CommandEndEvent),
  DECLARE(CommandMtlfxUpscale),
  DECLARE(CommandCount)
};
#undef DECLARE

struct CommandClear
{
  uint8_t cid = Type_CommandClear;
  uint16_t mask;
  uint8_t stencil;
  E3DCOLOR color;
  float z;


  CommandClear(uint32_t mask, E3DCOLOR color, float z, uint8_t stencil) : mask(mask), stencil(stencil), color(color), z(z) {}

  __forceinline void Process(Render &render)
  {
    float c[4] = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
    if (!render.rt.vp.used)
      render.updateEncoder(mask, c, z, stencil);
    else
      render.doClear(nullptr, 0, 0, z, c, false, mask & CLEAR_TARGET, mask & CLEAR_ZBUFFER);
  }
};

struct CommandClearTex
{
  uint8_t cid = Type_CommandClearTex;
  uint8_t is_int : 1;
  uint8_t level : 7;
  uint16_t layer;

  Texture *tex;
  unsigned val[4];

  CommandClearTex(Texture *tex, const unsigned ival[4], int level, int layer, bool is_int) :
    is_int(is_int), level(level), layer(layer), tex(tex)
  {
    memcpy(val, ival, sizeof(val));
  }

  __forceinline void Process(Render &render) { render.doClear(tex, level, layer, 0.0f, (float *)val, is_int, true, false); }
};

struct CommandClearTexture
{
  uint8_t cid = Type_CommandClearTexture;
  uint8_t slices = 1;
  uint8_t depth = 1;
  uint8_t levels = 1;
  uint16_t width = 1;
  uint16_t height = 1;
  id<MTLTexture> metalTex = nil;
  int base_format = 0;
  bool use_dxt = false;

  CommandClearTexture(Texture *tex)
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

  __forceinline void Process(Render &render)
  {
    render.doClearTexture(width, height, slices, depth, levels, metalTex, base_format, use_dxt);
  }
};

struct CommandCopyTex
{
  uint32_t cid = Type_CommandCopyTex;

  Texture *src;
  float src_rect[4];
  Texture *dst;
  int dst_rect[4];

  CommandCopyTex(Texture *src, float *src_r, Texture *dst, int *dst_r) : src(src), dst(dst)
  {
    memcpy(src_rect, src_r, sizeof(src_rect));
    memcpy(dst_rect, dst_r, sizeof(dst_rect));
  }

  __forceinline void Process(Render &render) { render.doCopyTexture(dst, src, src_rect, dst_rect); }
};

struct CommandSetViewport
{
  uint32_t cid = Type_CommandSetViewport;
  int16_t x, y, w, h;
  float minz, maxz;

  CommandSetViewport(int x, int y, int w, int h, float minz, float maxz) : x(x), y(y), w(w), h(h), minz(minz), maxz(maxz) {}

  __forceinline void Process(Render &render)
  {
    render.rt.vp.used = true;

    render.rt.vp.x = x;
    render.rt.vp.y = y;
    render.rt.vp.w = w;
    render.rt.vp.h = h;
    render.rt.vp.minz = minz;
    render.rt.vp.maxz = maxz;
  }
};

struct CommandSetViewportNoZ
{
  uint64_t cid : 8;
  uint64_t x : 14;
  uint64_t y : 14;
  uint64_t w : 14;
  uint64_t h : 14;

  CommandSetViewportNoZ(int x, int y, int w, int h) : cid(Type_CommandSetViewportNoZ), x(x), y(y), w(w), h(h) {}

  __forceinline void Process(Render &render)
  {
    render.rt.vp.used = true;

    render.rt.vp.x = x;
    render.rt.vp.y = y;
    render.rt.vp.w = w;
    render.rt.vp.h = h;
    render.rt.vp.minz = 0.0f;
    render.rt.vp.maxz = 1.f;
  }
};

struct CommandSetScissor
{
  uint32_t cid = Type_CommandSetScissor;
  int x, y, w, h;

  CommandSetScissor(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}

  __forceinline void Process(Render &render)
  {
    render.sci_x = x;
    render.sci_y = y;
    render.sci_w = w;
    render.sci_h = h;

    if (render.scissor_on)
      render.need_set_scissor = true;
  }
};

struct CommandSetVDecl
{
  uint32_t cid = Type_CommandSetVDecl;
  int vdecl;

  CommandSetVDecl(int decl) : vdecl(decl) {}

  __forceinline void Process(Render &render)
  {
    render.vdecls.lock();
    render.vdecl = render.vdecls.getObj(vdecl);
    render.vdecls.unlock();
  }
};

struct CommandReadbackTex
{
  uint32_t cid = Type_CommandReadbackTex;
  id<MTLTexture> tex;
  unsigned layer;
  unsigned level;
  unsigned w, h, d;
  id<MTLBuffer> buf;
  unsigned buf_offset;
  unsigned pitch;
  unsigned image_size;

  CommandReadbackTex(id<MTLTexture> tex, unsigned layer, unsigned level, unsigned w, unsigned h, unsigned d, id<MTLBuffer> buf,
    unsigned buf_offset, unsigned pitch, unsigned image_size) :
    tex(tex), layer(layer), level(level), w(w), h(h), d(d), buf(buf), buf_offset(buf_offset), pitch(pitch), image_size(image_size)
  {}

  __forceinline void Process(Render &render)
  {
    render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

    MTLSize size = {(uint32_t)w, (uint32_t)h, (uint32_t)d};
    MTLOrigin origin = {0, 0, 0};

    [render.blitEncoder copyFromTexture:tex
                            sourceSlice:layer
                            sourceLevel:level
                           sourceOrigin:origin
                             sourceSize:size
                               toBuffer:buf
                      destinationOffset:buf_offset
                 destinationBytesPerRow:pitch
               destinationBytesPerImage:image_size];
  }
};

struct CommandSetImmediate
{
  uint8_t cid = Type_CommandSetImmediate;
  uint8_t rstage;
  uint16_t dword_count;
  unsigned dwords[4];

  CommandSetImmediate(unsigned stage, unsigned dword_count, unsigned *imm_dwords) : rstage(stage), dword_count(dword_count)
  {
    if (dword_count)
      memcpy(dwords, imm_dwords, dword_count * sizeof(unsigned));
  }

  __forceinline void Process(Render &render)
  {
    Render::StageBinding &stage = render.stages[rstage];
    if (dword_count)
      memcpy(stage.immediate_dwords, dwords, dword_count * sizeof(uint32_t));
    stage.immediate_dword_count = dword_count;
  }
};

struct CommandSetBuf
{
  uint64_t cid : 8;
  uint64_t rstage : 2;
  uint64_t slot : 7;
  uint64_t original_slot : 7;
  uint64_t offset : 32;
  uint64_t stride : 8;
  Buffer *buf;

  CommandSetBuf(unsigned stage, unsigned slot, unsigned original_slot, Buffer *buf, unsigned offset, unsigned stride) :
    offset(offset), buf(buf), stride(stride), rstage(stage), slot(slot), original_slot(original_slot)
  {
    cid = Type_CommandSetBuf;
  }

  __forceinline void Process(Render &render)
  {
    Render::StageBinding &stage = render.stages[rstage];
    if (rstage == STAGE_VS)
    {
      G_ASSERT(stride < 256);
      render.cur_rstate.vbuffer_stride[slot] = stride;
    }

    if (stage.buffers[slot] && stage.buffers[slot]->getTexture())
      stage.setTex(original_slot, nullptr);

    stage.setBuf(slot, buf, offset);

    if (buf && buf->getTexture())
      stage.setTex(original_slot, buf->getTexture());
  }
};

struct CommandSetIB
{
  uint32_t cid = Type_CommandSetIB;
  unsigned ib_type;
  Buffer *ib;

  CommandSetIB(Buffer *ib, unsigned ib_type) : ib_type(ib_type), ib(ib) {}

  __forceinline void Process(Render &render)
  {
    render.ibuffer = ib;
    render.ibuffertype = ib_type;
  }
};

struct CommandBufDiscard
{
  uint32_t cid = Type_CommandBufDiscard;
  Buffer *buf;
  Buffer::BufTex *buf_tex;
  id<MTLBuffer> buf_mtl;
  unsigned offset;

  CommandBufDiscard(Buffer *buf, Buffer::BufTex *buf_tex, id<MTLBuffer> buf_mtl, unsigned offset) :
    buf(buf), buf_tex(buf_tex), buf_mtl(buf_mtl), offset(offset)
  {}

  __forceinline void Process(Render &render) { buf->discard(buf_tex, buf_mtl, offset); }
};

struct CommandSetConst
{
  uint64_t cid : 8;
  uint64_t stage : 2;
  uint64_t reg_base : 16;
  uint64_t reg_num : 16;
  uint64_t need_free : 22;
  void *data;

  CommandSetConst(unsigned stage, const float *in_data, unsigned reg_base, unsigned reg_num) :
    stage(stage), reg_base(reg_base), reg_num(reg_num)
  {
    cid = Type_CommandSetConst;
    if ((data = render.stages[stage].cbuffer.storeCmd(in_data, reg_num)))
      need_free = false;
    else
    {
      data = midmem_ptr()->alloc(reg_num);
      memcpy(data, in_data, reg_num);
      need_free = true;
    }
  }

  __forceinline void Process(Render &render)
  {
    render.stages[stage].cbuffer.applyCmd(data, reg_base, reg_num);
    if (need_free)
      midmem_ptr()->free(data);
  }
};

struct CommandSetTex
{
  uint32_t cid = Type_CommandSetTex;
  uint8_t rstage;
  uint8_t slot;
  uint8_t read_stencil;
  uint8_t mip_level;
  Texture *tex;

  CommandSetTex(unsigned stage, unsigned slot, Texture *tex, unsigned read_stencil, unsigned mip_level) :
    rstage(stage), slot(slot), read_stencil(read_stencil), mip_level(mip_level), tex(tex)
  {}

  __forceinline void Process(Render &render)
  {
    Render::StageBinding &stage = render.stages[rstage];
    stage.setTex(slot, tex, read_stencil, mip_level);
  }
};

struct CommandSetProg
{
  uint16_t cid = Type_CommandSetProg;
  uint16_t is_async = 0;
  int prog;

  CommandSetProg(int prog) : is_async(render.async_pso_compilation), prog(prog) {}

  __forceinline void Process(Render &render) { render.doSetProgram(prog, is_async); }
};

struct CommandSetTexMipLevel
{
  uint32_t cid = Type_CommandSetTexMipLevel;
  uint16_t min_level;
  uint16_t max_level;
  Texture *tex;

  CommandSetTexMipLevel(Texture *tex, int min_level, int max_level) : min_level(min_level), max_level(max_level), tex(tex) {}

  __forceinline void Process(Render &render) { tex->setMiplevelImpl(min_level, max_level); }
};

struct CommandTexGenMips
{
  uint32_t cid = Type_CommandTexGenMips;
  Texture *tex;

  CommandTexGenMips(Texture *tex) : tex(tex) {}

  __forceinline void Process(Render &render)
  {
    render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

    G_ASSERT(tex);
    [render.blitEncoder generateMipmapsForTexture:tex->apiTex->texture];
  }
};

struct CommandTexCopy
{
  uint32_t cid = Type_CommandTexCopy;
  Texture *dst;
  Texture *src;

  CommandTexCopy(Texture *dst, Texture *src) : dst(dst), src(src) {}

  __forceinline void Process(Render &render) { render.doTexCopy(dst, src); }
};

struct CommandTexCopyRegion
{
  uint16_t cid = Type_CommandTexCopyRegion;
  uint16_t dest_x, dest_y, dest_z;
  id<MTLTexture> dst;
  id<MTLTexture> src;
  MTLPixelFormat src_format;
  MTLPixelFormat dst_format;
  uint16_t dest_level;
  uint16_t dest_levels;
  uint16_t src_level;
  uint8_t src_levels;
  uint8_t src_dxt;
  uint16_t src_x, src_y, src_z;
  uint16_t src_w, src_h, src_d;

  CommandTexCopyRegion(Texture *dst, int dest_level, int dest_x, int dest_y, int dest_z, Texture *src, int src_level, int src_x,
    int src_y, int src_z, int src_w, int src_h, int src_d) :
    dest_x(dest_x),
    dest_y(dest_y),
    dest_z(dest_z),
    dst(dst->apiTex->texture),
    src(src->apiTex->texture),
    src_format(src->metal_format),
    dst_format(dst->metal_format),
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
  {
    G_ASSERT(!dst->isStub());
    int dst_mip = dest_level % dst->mipLevels;
    G_ASSERT(src_w <= max(dst->width >> dst_mip, 1));
    G_ASSERT(src_h <= max(dst->height >> dst_mip, 1));
    G_ASSERT(!src_dxt || (src_x & 3) == 0);
    G_ASSERT(!src_dxt || (src_y & 3) == 0);
    G_ASSERT(!src_dxt || (dest_x & 3) == 0);
    G_ASSERT(!src_dxt || (dest_y & 3) == 0);
  }

  __forceinline void Process(Render &render) { render.doTexCopyRegion(*this); }
};

struct CommandStencilRef
{
  uint16_t cid = Type_CommandStencilRef;
  uint16_t ref;

  CommandStencilRef(unsigned ref) : ref(ref) {}

  __forceinline void Process(Render &render)
  {
    if (render.stencil_ref != ref)
    {
      render.stencil_ref = ref;
      render.stencil_ref_changed = true;
    }
  }
};

struct CommandSetState
{
  uint32_t cid = Type_CommandSetState;
  shaders::DriverRenderStateId state_id;

  CommandSetState(shaders::DriverRenderStateId state_id) : state_id(state_id) {}

  __forceinline void Process(Render &render) { render.doSetState(state_id); }
};

struct CommandSetRT
{
  uint8_t cid = Type_CommandSetRT;
  uint8_t slot;
  uint16_t level;
  unsigned layer;
  Texture *rt;

  CommandSetRT(unsigned slot, Texture *rt, unsigned level, unsigned layer) : slot(slot), level(level), layer(layer), rt(rt) {}

  __forceinline void Process(Render &render)
  {
    render.rt.rt[slot] = rt;
    render.rt.rt_levels[slot] = level;
    render.rt.rt_layers[slot] = layer;

    render.need_change_rt = 1;

    render.rt.vp.used = false;

    if (rt)
    {
      render.sci_x = 0;
      render.sci_y = 0;
      render.sci_w = rt == (Texture *)-1 ? render.wnd_wd : max(1, rt->width >> level);
      render.sci_h = rt == (Texture *)-1 ? render.wnd_ht : max(1, rt->height >> level);
    }
  }
};

struct CommandSetDepth
{
  uint32_t cid = Type_CommandSetDepth;
  int16_t layer = 0;
  int16_t ro = 0;
  Texture *rt;

  CommandSetDepth(Texture *rt, int layer, bool ro) : layer(layer), ro(ro), rt(rt) {}

  __forceinline void Process(Render &render)
  {
    render.rt.depth = rt;
    render.rt.depth_layer = layer;
    render.rt.depth_readonly = ro > 0;
    render.need_change_rt = 1;

    if (rt)
    {
      render.sci_x = 0;
      render.sci_y = 0;
      render.sci_w = rt == (Texture *)-1 ? render.wnd_wd : rt->width;
      render.sci_h = rt == (Texture *)-1 ? render.wnd_ht : rt->height;
    }
  }
};

struct CommandSetQuery
{
  uint32_t cid = Type_CommandSetQuery;
  Render::Query *query;

  CommandSetQuery(Render::Query *query) : query(query) {}

  __forceinline void Process(Render &render)
  {
    if (query)
    {
      Render::Query *local_query = query;
      render.cur_query_offset = local_query->offset;
      [render.commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        local_query->value = *((uint64_t *)((uint8_t *)[render.query_buffer contents] + local_query->offset));
        local_query->status = 0;
      }];
    }
    else
    {
      render.cur_query_offset = -1;
    }

    render.need_set_qeury = 1;
  }
};

struct CommandSetMSAAPass
{
  uint32_t cid = Type_CommandSetMSAAPass;

  __forceinline void Process(Render &render) { render.msaa_pass = true; }
};

struct CommandSetDepthResolve
{
  uint32_t cid = Type_CommandSetDepthResolve;

  __forceinline void Process(Render &render) { render.depth_resolve = true; }
};

struct CommandSetFence
{
  uint32_t cid = Type_CommandSetFence;
  Render::Query *query;

  CommandSetFence(Render::Query *query) : query(query) {}

  __forceinline void Process(Render &render)
  {
    Render::Query *local_query = query;
    [render.commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) { local_query->status = 0; }];
  }
};

struct CommandSetTLAS
{
  uint32_t cid = Type_CommandSetTLAS;
  RaytraceTopAccelerationStructure *accStruct;
  ShaderStage targetStage = STAGE_CS;
  int setSlot = -1;

  CommandSetTLAS(RaytraceTopAccelerationStructure *as, ShaderStage stage, int slot) :
    accStruct(as), targetStage(stage), setSlot(slot){};

  __forceinline void Process(Render &render)
  {
    G_ASSERT(targetStage == STAGE_CS); // For now metal supports only CS
    Render::StageBinding &stage = render.stages[targetStage];
    stage.setAccStruct(setSlot, accStruct);
  }
};


struct CommandBuildTLAS
{
  uint32_t cid = Type_CommandBuildTLAS;
  RaytraceTopAccelerationStructure *accStruct;

  API_AVAILABLE(ios(15.0), macos(11.0))
  MTLInstanceAccelerationStructureDescriptor *accDesc;
  bool shouldUpdate;

  API_AVAILABLE(ios(15.0), macos(11.0))
  CommandBuildTLAS(RaytraceTopAccelerationStructure *tlas, MTLInstanceAccelerationStructureDescriptor *desc, bool update) :
    accStruct(tlas), accDesc(desc), shouldUpdate(update){};

  __forceinline void Process(Render &render)
  {
    if (@available(iOS 15.0, macos 11.0, *))
      render.buildTLAS(accStruct, accDesc, shouldUpdate);
  }
};

struct CommandBuildBLAS
{
  uint32_t cid = Type_CommandBuildBLAS;

  RaytraceBottomAccelerationStructure *accStruct;

  API_AVAILABLE(ios(15.0), macos(11.0))
  MTLPrimitiveAccelerationStructureDescriptor *accDesc;
  bool shouldUpdate;

  API_AVAILABLE(ios(15.0), macos(11.0))
  CommandBuildBLAS(RaytraceBottomAccelerationStructure *as, MTLPrimitiveAccelerationStructureDescriptor *desc, bool update) :
    accStruct(as), accDesc(desc), shouldUpdate(update){};

  __forceinline void Process(Render &render)
  {
    if (@available(iOS 15.0, macos 11.0, *))
      render.buildBLAS(accStruct, accDesc, shouldUpdate);
  }
};


struct CommandAddBLASToList
{
  uint32_t cid = Type_CommandAddBLASToList;
  CommandAddBLASToList(){};

  __forceinline void Process(Render &render) { render.addBLASStubToList(); }
};

struct CommandRemoveBLASFromList
{
  uint32_t cid = Type_CommandRemoveBLASFromList;
  int deleteIndex = -1;

  CommandRemoveBLASFromList(int delete_index) : deleteIndex(delete_index){};

  __forceinline void Process(Render &render) { render.removeBLASFromList(deleteIndex); }
};

struct CommandCopyBuffer
{
  uint32_t cid = Type_CommandCopyBuffer;
  unsigned dst_offset;
  Buffer *dst;
  id<MTLBuffer> src;
  unsigned src_offset;
  unsigned size;

  CommandCopyBuffer(Buffer *dst, unsigned dst_offset, Buffer *src, unsigned src_offset, unsigned size) :
    dst_offset(dst_offset), dst(dst), src(src->getBuffer()), src_offset(src_offset + src->getDynamicOffset()), size(size)
  {}

  __forceinline void Process(Render &render)
  {
    render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);

    G_ASSERT(dst->getDynamicOffset() == 0);
    [render.blitEncoder copyFromBuffer:src sourceOffset:src_offset toBuffer:dst->getBuffer() destinationOffset:dst_offset size:size];
  }
};

struct CommandUpdateBuffer
{
  uint32_t cid = Type_CommandUpdateBuffer;
  unsigned dst_offset;
  id<MTLBuffer> dst;
  id<MTLBuffer> src;
  unsigned src_offset;
  unsigned size;

  CommandUpdateBuffer(id<MTLBuffer> dst, unsigned dst_offset, id<MTLBuffer> src, unsigned src_offset, unsigned size) :
    dst_offset(dst_offset), dst(dst), src(src), src_offset(src_offset), size(size)
  {}

  __forceinline void Process(Render &render)
  {
    render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);
    [render.blitEncoder copyFromBuffer:src sourceOffset:src_offset toBuffer:dst destinationOffset:dst_offset size:size];
  }
};

struct CommandBeginEvent
{
  uint16_t cid = Type_CommandBeginEvent;
  String name;
  CommandBeginEvent(const char *name) : name(0, name) {}
  __forceinline void Process(Render &render)
  {
    if (render.renderEncoder)
      [render.renderEncoder pushDebugGroup:[NSString stringWithUTF8String:name.c_str()]];
    else if (render.computeEncoder)
      [render.computeEncoder pushDebugGroup:[NSString stringWithUTF8String:name.c_str()]];
    else if (render.blitEncoder)
      [render.blitEncoder pushDebugGroup:[NSString stringWithUTF8String:name.c_str()]];
    else
      [render.commandBuffer pushDebugGroup:[NSString stringWithUTF8String:name.c_str()]];
  }
};

struct CommandEndEvent
{
  uint16_t cid = Type_CommandEndEvent;
  __forceinline void Process(Render &render)
  {
    if (render.renderEncoder)
      [render.renderEncoder popDebugGroup];
    else if (render.computeEncoder)
      [render.computeEncoder popDebugGroup];
    else if (render.blitEncoder)
      [render.blitEncoder popDebugGroup];
    else
      [render.commandBuffer popDebugGroup];
  }
};

struct CommandDraw
{
  uint16_t cid = Type_CommandDraw;
  uint16_t prim_type;
  unsigned start_vertex;
  unsigned prim_count;
  unsigned num_instances;
  unsigned start_instance;

  CommandDraw(unsigned prim_type, unsigned start_vertex, unsigned prim_count, unsigned num_instances, unsigned start_instance) :
    prim_type(prim_type),
    start_vertex(start_vertex),
    prim_count(prim_count),
    num_instances(num_instances),
    start_instance(start_instance)
  {}

  __forceinline void Process(Render &render)
  {
    if (num_instances > 0 && render.applyStates())
    {
      G_ASSERT(render.caps.drawIndxWithBaseVertes || start_instance == 0);
      if (render.caps.drawIndxWithBaseVertes)
        [render.renderEncoder drawPrimitives:(MTLPrimitiveType)prim_type
                                 vertexStart:start_vertex
                                 vertexCount:prim_count
                               instanceCount:num_instances
                                baseInstance:start_instance];
      else
        [render.renderEncoder drawPrimitives:(MTLPrimitiveType)prim_type
                                 vertexStart:start_vertex
                                 vertexCount:prim_count
                               instanceCount:num_instances];
    }
  }
};

struct CommandDrawIndirect
{
  uint16_t cid = Type_CommandDrawIndirect;
  uint16_t prim_type;
  unsigned offset;
  Buffer *buf;

  CommandDrawIndirect(unsigned prim_type, Buffer *buf, unsigned offset) : prim_type(prim_type), offset(offset), buf(buf) {}

  __forceinline void Process(Render &render)
  {
    if (render.applyStates())
      [render.renderEncoder drawPrimitives:(MTLPrimitiveType)prim_type
                            indirectBuffer:buf->getBuffer()
                      indirectBufferOffset:offset + buf->getDynamicOffset()];
  }
};

struct CommandDrawIndexed
{
  uint16_t cid = Type_CommandDrawIndexed;
  uint16_t prim_type;
  int base_vertex;
  unsigned start_vertex;
  unsigned prim_count;
  unsigned num_instances;
  unsigned start_instance;

  CommandDrawIndexed(unsigned prim_type, int base_vertex, unsigned start_vertex, unsigned prim_count, unsigned num_instances,
    unsigned start_instance) :
    prim_type(prim_type),
    base_vertex(base_vertex),
    start_vertex(start_vertex),
    prim_count(prim_count),
    num_instances(num_instances),
    start_instance(start_instance)
  {}

  __forceinline void Process(Render &render)
  {
    if (num_instances > 0 && render.applyStates())
    {
      int offset = render.ibuffertype == 1 ? start_vertex * 4 : start_vertex * 2;
      base_vertex = max(0, base_vertex);

      if (render.caps.drawIndxWithBaseVertes)
      {
        [render.renderEncoder drawIndexedPrimitives:(MTLPrimitiveType)prim_type
                                         indexCount:prim_count
                                          indexType:(MTLIndexType)render.ibuffertype
                                        indexBuffer:render.ibuffer->getBuffer()
                                  indexBufferOffset:offset + render.ibuffer->getDynamicOffset()
                                      instanceCount:num_instances
                                         baseVertex:base_vertex
                                       baseInstance:start_instance];
      }
      else
      {
        if (base_vertex > 0)
        {
          Render::StageBinding &vs_stage = render.stages[STAGE_VS];
          vs_stage.bindVertexStream(0, render.cur_rstate.vbuffer_stride[0] * base_vertex);

          auto vdecl = render.vdecl ? render.vdecl : render.cur_prog->vdecl;
          constexpr uint32_t stream2 = 1;
          if (vdecl->num_streams > 1 && (vdecl->instanced_mask & (1 << stream2)) == 0)
            vs_stage.bindVertexStream(stream2, render.cur_rstate.vbuffer_stride[stream2] * base_vertex);
        }

        [render.renderEncoder drawIndexedPrimitives:(MTLPrimitiveType)prim_type
                                         indexCount:prim_count
                                          indexType:(MTLIndexType)render.ibuffertype
                                        indexBuffer:render.ibuffer->getBuffer()
                                  indexBufferOffset:offset + render.ibuffer->getDynamicOffset()
                                      instanceCount:num_instances];
      }
    }
  }
};

struct CommandDrawIndexedIndirect
{
  uint16_t cid = Type_CommandDrawIndexedIndirect;
  uint16_t prim_type;
  unsigned offset;
  Buffer *buf;

  CommandDrawIndexedIndirect(unsigned prim_type, Buffer *buf, unsigned offset) : prim_type(prim_type), offset(offset), buf(buf) {}

  __forceinline void Process(Render &render)
  {
    if (render.applyStates())
    {
      [render.renderEncoder drawIndexedPrimitives:(MTLPrimitiveType)prim_type
                                        indexType:(MTLIndexType)render.ibuffertype
                                      indexBuffer:render.ibuffer->getBuffer()
                                indexBufferOffset:render.ibuffer->getDynamicOffset()
                                   indirectBuffer:buf->getBuffer()
                             indirectBufferOffset:offset + buf->getDynamicOffset()];
    }
  }
};

struct CommandDispatch
{
  uint16_t cid = Type_CommandDispatch;
  uint16_t x, y, z;

  CommandDispatch(int x, int y, int z) : x(x), y(y), z(z) {}

  __forceinline void Process(Render &render) { render.doDispatch(nullptr, 0, x, y, z); }
};

struct CommandDispatchIndirect
{
  uint32_t cid = Type_CommandDispatchIndirect;
  Buffer *buf;
  unsigned offset;

  CommandDispatchIndirect(Buffer *buf, unsigned offset) : buf(buf), offset(offset) {}

  __forceinline void Process(Render &render) { render.doDispatch(buf, offset, 0, 0, 0); }
};

struct CommandClearRW
{
  uint32_t cid = Type_CommandClearRW;
  Buffer *buf;
  unsigned val[4];
  unsigned size[4];

  CommandClearRW(Buffer *buf, unsigned *clear_val) : buf(buf)
  {
    G_ASSERT(buf);
    memcpy(val, clear_val, sizeof(val));
  }

  __forceinline void Process(Render &render)
  {
    // handle only this special case for now. i searched through the code and it seems we don't use anything else
    if ((val[0] == 0 || val[0] == 0xffffffff))
    {
      render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);
      G_ASSERT(buf->getDynamicOffset() == 0);
      [render.blitEncoder fillBuffer:buf->getBuffer() range:NSMakeRange(0, buf->bufSize) value:(val[0] & 0xff)];
    }
    else
    {
      render.ensureHaveEncoderExceptRender(Render::EncoderType::Compute);

      [render.computeEncoder setComputePipelineState:render.clear_cs_pipeline];

      size[0] = buf->bufSize / 4;
      G_ASSERT(buf->getDynamicOffset() == 0);
      [render.computeEncoder setBytes:val length:32 atIndex:0];
      [render.computeEncoder setBuffer:buf->getBuffer() offset:0 atIndex:1];
      [render.computeEncoder dispatchThreadgroups:MTLSizeMake((size[0] + 63) >> 6, 1, 1) threadsPerThreadgroup:MTLSizeMake(64, 1, 1)];

      render.stages[STAGE_CS].reset();
      render.current_cs_pipeline = nullptr;
    }
  }
};

struct CommandClearBuffer
{
  uint32_t cid = Type_CommandClearBuffer;
  Buffer::BufTex *buf;

  CommandClearBuffer(Buffer::BufTex *buf) : buf(buf) {}

  __forceinline void Process(Render &render)
  {
    render.ensureHaveEncoderExceptRender(Render::EncoderType::Blit);
    [render.blitEncoder fillBuffer:buf->buffer range:NSMakeRange(0, buf->bufsize) value:0];

    buf->initialized = true;
  }
};

struct CommandSwapTextures
{
  uint32_t cid = Type_CommandSwapTextures;
  Texture *src;
  BaseTexture *dst;

  CommandSwapTextures(Texture *src, BaseTexture *dst) : src(src), dst(dst) {}

  __forceinline void Process(Render &render) { src->replaceTexResObjectImpl(dst); }
};

struct CommandSetBlendFactor
{
  uint32_t cid = Type_CommandSetBlendFactor;
  E3DCOLOR factor;


  CommandSetBlendFactor(E3DCOLOR factor) : factor(factor) {}

  __forceinline void Process(Render &render)
  {
    [render.renderEncoder setBlendColorRed:factor.r / 255.f green:factor.g / 255.f blue:factor.b / 255.f alpha:factor.a / 255.f];
  }
};

struct CommandSetTextureAddr
{
  uint8_t cid = Type_CommandSetTextureAddr;
  int8_t u;
  int8_t v;
  int8_t w;
  Texture *tex;

  CommandSetTextureAddr(Texture *tex, int u, int v, int w) : u(u), v(v), w(w), tex(tex) {}

  __forceinline void Process(Render &render) { tex->doSetAddressing(u, v, w); }
};

struct CommandSetTextureBorder
{
  uint32_t cid = Type_CommandSetTextureBorder;
  E3DCOLOR color;
  Texture *tex;

  CommandSetTextureBorder(Texture *tex, E3DCOLOR color) : color(color), tex(tex) {}

  __forceinline void Process(Render &render) { tex->doSetBorder(color); }
};

struct CommandSetTextureFilter
{
  uint32_t cid = Type_CommandSetTextureFilter;
  int16_t filter;
  int16_t mipfilter;
  Texture *tex;

  CommandSetTextureFilter(Texture *tex, int filter, int mipfilter) : filter(filter), mipfilter(mipfilter), tex(tex) {}

  __forceinline void Process(Render &render) { tex->doSetFilter(filter, mipfilter); }
};

struct CommandSetTextureAniso
{
  uint32_t cid = Type_CommandSetTextureAniso;
  int level;
  Texture *tex;

  CommandSetTextureAniso(Texture *tex, int level) : level(level), tex(tex) {}

  __forceinline void Process(Render &render) { tex->doSetAnisotropy(level); }
};

struct CommandDeleteNativeBuffer
{
  uint32_t cid = Type_CommandDeleteNativeBuffer;
  id<MTLBuffer> buf;

  CommandDeleteNativeBuffer(id<MTLBuffer> buf) : buf(buf) {}

  __forceinline void Process(Render &render) { render.resources2delete.native_resources.push_back(buf); }
};

struct CommandDeleteBuffer
{
  uint32_t cid = Type_CommandDeleteBuffer;
  Buffer *buf;

  CommandDeleteBuffer(Buffer *buf) : buf(buf) {}

  __forceinline void Process(Render &render) { render.resources2delete.buffers.push_back(buf); }
};

struct CommandDeleteNativeTexture
{
  uint32_t cid = Type_CommandDeleteNativeTexture;
  id<MTLTexture> tex;

  CommandDeleteNativeTexture(id<MTLTexture> tex) : tex(tex) {}

  __forceinline void Process(Render &render) { render.resources2delete.native_resources.push_back(tex); }
};

struct CommandDeleteTexture
{
  uint32_t cid = Type_CommandDeleteTexture;
  Texture *tex;

  CommandDeleteTexture(Texture *tex) : tex(tex) {}

  __forceinline void Process(Render &render)
  {
    render.resources2delete.textures.push_back(tex);
    if (tex)
      render.flushTexture(tex);
  }
};

struct CommandMtlfxUpscale
{
  uint32_t cid = Type_CommandMtlfxUpscale;
  Texture *color;
  Texture *output;
  uint32_t colorMode;

  CommandMtlfxUpscale(Texture *col, Texture *out, uint32_t mode) : color(col), output(out), colorMode(mode) {}

  __forceinline void Process(Render &render) { render.MetalfxUpscale(color, output, colorMode); }
};

#define CMD_USE_ATOMICS 0

struct CommandBuffer
{
  static const uint32_t max_size = 800 * 1024;
  uint32_t offset = 0;
#if CMD_USE_ATOMICS
  uint8_t data[max_size];
#else
  Tab<uint8_t> data;
#endif
  uint32_t size_stats[Type_CommandCount] = {0};

  CommandBuffer()
  {
#if !CMD_USE_ATOMICS
    data.resize(max_size);
#endif
  }

  template <typename T, typename... Params>
  __forceinline T *push(Params... params)
  {
    if (g_ios_pause_rendering && is_main_thread())
      return nullptr;
#if CMD_USE_ATOMICS
    uint32_t current = __atomic_add_fetch(&offset, sizeof(T), __ATOMIC_SEQ_CST) - sizeof(T);
    G_ASSERT(current + sizeof(T) < max_size);
#else
    RCAutoLock autoclock;
    uint32_t current = offset;
    offset += sizeof(T);
    if (offset > data.size())
      data.resize(int(offset * 1.5f));
#endif
    T *result = new (&data[0] + current) T(params...);
    size_stats[result->cid] += sizeof(T);
    return result;
  }

  void execute(Render &render)
  {
    TIME_PROFILE(execute)
#define PROCESS(Type)                              \
  case Type_##Type:                                \
  {                                                \
    Type *cmd = (Type *)(&data[0] + local_offset); \
    cmd_size = sizeof(Type);                       \
    cmd->Process(render);                          \
  }                                                \
  break;

    uint32_t local_offset = 0;
    while (local_offset < offset)
    {
      uint32_t cmd_size = 0;
      uint32_t id = data[local_offset];
      switch (id)
      {
        PROCESS(CommandClear)
        PROCESS(CommandClearTex)
        PROCESS(CommandClearTexture)
        PROCESS(CommandCopyTex)
        PROCESS(CommandSetViewport)
        PROCESS(CommandSetViewportNoZ)
        PROCESS(CommandSetScissor)
        PROCESS(CommandSetVDecl)
        PROCESS(CommandReadbackTex)
        PROCESS(CommandSetImmediate)
        PROCESS(CommandSetBuf)
        PROCESS(CommandSetIB)
        PROCESS(CommandBufDiscard)
        PROCESS(CommandSetConst)
        PROCESS(CommandSetTex)
        PROCESS(CommandSetProg)
        PROCESS(CommandSetTexMipLevel)
        PROCESS(CommandTexGenMips)
        PROCESS(CommandTexCopy)
        PROCESS(CommandTexCopyRegion)
        PROCESS(CommandStencilRef)
        PROCESS(CommandSetState)
        PROCESS(CommandSetRT)
        PROCESS(CommandSetDepth)
        PROCESS(CommandSetQuery)
        PROCESS(CommandSetFence)
        PROCESS(CommandSetTLAS)
        PROCESS(CommandBuildTLAS)
        PROCESS(CommandBuildBLAS)
        PROCESS(CommandAddBLASToList)
        PROCESS(CommandRemoveBLASFromList)
        PROCESS(CommandCopyBuffer)
        PROCESS(CommandUpdateBuffer)
        PROCESS(CommandDraw)
        PROCESS(CommandDrawIndirect)
        PROCESS(CommandDrawIndexed)
        PROCESS(CommandDrawIndexedIndirect)
        PROCESS(CommandDispatch)
        PROCESS(CommandDispatchIndirect)
        PROCESS(CommandClearRW)
        PROCESS(CommandClearBuffer)
        PROCESS(CommandSwapTextures)
        PROCESS(CommandSetBlendFactor)
        PROCESS(CommandSetTextureAddr)
        PROCESS(CommandSetTextureBorder)
        PROCESS(CommandSetTextureFilter)
        PROCESS(CommandSetTextureAniso)
        PROCESS(CommandSetMSAAPass)
        PROCESS(CommandSetDepthResolve)
        PROCESS(CommandDeleteNativeBuffer)
        PROCESS(CommandDeleteBuffer)
        PROCESS(CommandDeleteNativeTexture)
        PROCESS(CommandDeleteTexture)
        PROCESS(CommandBeginEvent)
        PROCESS(CommandEndEvent)
        PROCESS(CommandMtlfxUpscale)
        default: G_ASSERT("unknown command" && 0);
      }
      local_offset += cmd_size;
    }
    G_ASSERT(local_offset == offset);
    offset = 0;
    memset(size_stats, 0, sizeof(size_stats));

#undef PROCESS
  }
};
} // namespace drv3d_metal
