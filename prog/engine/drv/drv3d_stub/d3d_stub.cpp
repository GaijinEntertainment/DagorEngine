// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <windows.h>
#include <3d/ddsFormat.h>
#endif

#include <drv/3d/dag_sampler.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_tiledResource.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_dispatch.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_shaderLibrary.h>
#include <drv/3d/dag_capture.h>
#include <3d/ddsxTex.h>
#include <3d/tql.h>
#include "pools.h"
#include <util/dag_string.h>
#include <image/dag_texPixel.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>

#include <generic/dag_span.h>

#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>

#include <ioSys/dag_memIo.h>
#include <ioSys/dag_genIo.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <startup/dag_globalSettings.h>
#include "frameStateTM.inc.h"
#include <basetexture.h>
#include <destroyEvent.h>

#include <EASTL/string.h>

static FrameStateTM g_frameState;

class ID3dDrawStatistic;
class DummyTexture;

#define RETURN_NULL_ON_ZERO_SIZE(cond) \
  if (cond)                            \
  {                                    \
    debug_dump_stack(stat_name);       \
    return NULL;                       \
  }

#define DECLARE_TQL_AND_MAKETMPRES(TEX_TYPE)                                            \
  DECLARE_TQL_TID_AND_STUB()                                                            \
  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override \
  {                                                                                     \
    auto *tmp = new TEX_TYPE;                                                           \
    tmp->ti = ti;                                                                       \
    tmp->ti.w = w, tmp->ti.h = h, tmp->ti.d = d, tmp->ti.mipLevels = l;                 \
    eastl::string prefixedName;                                                         \
    prefixedName.sprintf("%s:%s", staging_tex ? "sys" : "tmp", getResName());           \
    tmp->setResName(prefixedName.c_str());                                              \
    return tmp;                                                                         \
  }                                                                                     \
  void replaceTexResObject(BaseTexture *&new_tex) override                              \
  {                                                                                     \
    delete static_cast<TEX_TYPE *>(new_tex);                                            \
    new_tex = nullptr;                                                                  \
  }                                                                                     \
  inline bool isTexResEqual(BaseTexture *) const                                        \
  {                                                                                     \
    return false;                                                                       \
  }

enum LocalTm
{
  LOCAL_TM_WORLD = 0,
  LOCAL_TM_VIEW,
  LOCAL_TM_PROJ,

  LOCAL_TM__NUM
};

static bool vb_ib_data_mandatory_support = false; // always
static bool vb_ib_data_flag_support = true;       // if readable

struct ViewData
{
  int x, y, w, h;
  float minz, maxz;

  ViewData()
  {
    x = y = 0;
    w = h = 1;
    minz = maxz = 0;
  }
};

static thread_local int thread_idx = 0;
static constexpr int MAX_THREAD_NUM = 16;
static int next_thread_idx = 1;

static inline int get_bytes_per_pixel(unsigned fmt)
{
  switch (fmt & (unsigned)TEXFMT_MASK)
  {
    case TEXFMT_A16B16G16R16:
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_G32R32F: return 8;
    case TEXFMT_A32B32G32R32F: return 16;
    case TEXFMT_A8:
    case TEXFMT_R8: return 1;
    case TEXFMT_A1R5G5B5:
    case TEXFMT_A4R4G4B4:
    case TEXFMT_R5G6B5: return 2;
    // Block size in bytes for BC formats
    case TEXFMT_DXT1:
    case TEXFMT_ATI1N: return 8;
    case TEXFMT_DXT3:
    case TEXFMT_DXT5:
    case TEXFMT_ATI2N:
    case TEXFMT_BC6H:
    case TEXFMT_BC7: return 16;
  }
  return 4; // including lesser formats
}

static inline unsigned calc_surface_sz(const TextureInfo &ti, unsigned l)
{
  const uint32_t BC_BLOCK_SIZE = 4;
  unsigned w = max<unsigned>(ti.w >> l, 1), h = max<unsigned>(ti.h >> l, 1);
  bool is_bc = is_bc_texformat(ti.cflg);
  unsigned stride_bytes = (is_bc ? ((w + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE) : w) * get_bytes_per_pixel(ti.cflg);
  unsigned heightInBlocks = (is_bc ? ((h + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE) : h);
  return stride_bytes * heightInBlocks;
}

struct DrvBuffer
{
  DrvBuffer()
  {
    memset(buf, 0, sizeof(buf));
    initBufSize(32 << 20);
    autoRealloc = false;
  }
  ~DrvBuffer()
  {
    for (int i = 0; i < MAX_THREAD_NUM; i++)
      del_it(buf[i]);
    memset(bufSz, 0, sizeof(bufSz));
  }

  inline void *getBuffer(int size, bool writeonly)
  {
    if (thread_idx <= 0)
    {
      const int count = interlocked_acquire_load(next_thread_idx);
      if (count >= MAX_THREAD_NUM)
        return nullptr;
      thread_idx = count + 1;
      const int newThreadIdx = interlocked_increment(next_thread_idx);
      DEBUG_CTX("one more resource creator/locker thread, %d total", newThreadIdx);
      //      debug_dump_stack();
    }
    int ti = thread_idx - 1;

    if (bufSz[ti] < size)
    {
      if (autoRealloc)
        setBufSize(size, ti);
      else
        DAG_FATAL("need buffer size = %dK | exist buffer size = %dK", size >> 10, bufSz[ti] >> 10);
    }
    if (!buf[ti])
      buf[ti] = new char[bufSz[ti]];
    if (!writeonly)
      memset(buf[ti], 0x00, size);
    return buf[ti];
  }

  inline void setBufSize(int size, int ti)
  {
    bufSz[ti] = size;
    del_it(buf[ti]);
  }
  inline void initBufSize(int size)
  {
    for (int i = 0; i < MAX_THREAD_NUM; i++)
      bufSz[i] = size;
  }

  inline void autoReallocSet(bool auto_realloc) { autoRealloc = auto_realloc; }

protected:
  char *buf[MAX_THREAD_NUM];
  int bufSz[MAX_THREAD_NUM];
  bool autoRealloc;
};


static bool useTexDynPool = true;
static DrvBuffer drvBuffer;

class DummyBufferImpl : public Sbuffer
{
public:
  DummyBufferImpl()
  {
    size = 1;
    flags = 0;
    bufData = NULL;
  }

  void allocInternal()
  {
    bufData = memalloc(size, midmem);
    memset(bufData, 0, size);
  }
  void allocBuf(bool allocate)
  {
    if (vb_ib_data_mandatory_support || (allocate && vb_ib_data_flag_support))
      allocInternal();
  }
  void freeBuf()
  {
    if (bufData)
      memfree(bufData, midmem);
    bufData = NULL;
  }

  virtual int lock(unsigned ofs_bytes, unsigned size_bytes, void **ptr, int options)
  {
    if (!ptr) // delayed lock
      return 1;

    *ptr = NULL;
    if (size_bytes == 0 && ofs_bytes != 0)
      return 0;
    if (ofs_bytes + size_bytes > size)
      return 0;

    if (bufData)
      *ptr = (char *)bufData + ofs_bytes;
    else
    {
      *ptr = drvBuffer.getBuffer(size, (options & (VBLOCK_WRITEONLY | VBLOCK_DISCARD)));
      if (!*ptr)
      {
        allocInternal();
        return lock(ofs_bytes, size_bytes, ptr, options);
      }
    }
    return 1;
  }

  int getFlags() const override { return flags; }
  virtual int unlock() { return 1; }

  virtual bool copyTo(Sbuffer *destBase)
  {
    auto *dest = static_cast<DummyBufferImpl *>(destBase);

    if (dest->size != size)
      return false;

    // Since the contents of the buffers do not matter for stub driver, no copy is necessary.
    // Otherwise, provided both pointers are valid (and they currently might not be), it would be:
    // memcpy(dest->bufData, bufData, size);

    return true;
  }

  virtual bool copyTo(Sbuffer *destBase, uint32_t dst_ofs_bytes, uint32_t src_ofs_bytes, uint32_t size_bytes)
  {
    auto *dest = static_cast<DummyBufferImpl *>(destBase);

    if (dst_ofs_bytes + size_bytes > dest->size)
      return false;

    if (src_ofs_bytes + size_bytes > size)
      return false;

    // Since the contents of the buffers do not matter for stub driver, no copy is necessary.
    // Otherwise, provided both pointers are valid (and they currently might not be), it would be:
    // memcpy(reinterpret_cast<char *>(dest->bufData) + dst_ofs_bytes, reinterpret_cast<char *>(bufData) + src_ofs_bytes, size_bytes);

    return true;
  }

  // D3DResource
  virtual void preload() {}
  int ressize() const override { return size; }

protected:
  int size, flags;
  void *bufData;
};

class DummyVBuffer final : public DummyBufferImpl
{
public:
  DummyVBuffer() : DummyBufferImpl() { elemSz = 1; }
  void setElemSz(int esz) { elemSz = esz; }

  virtual int getElementSize() const { return elemSz; }
  virtual int getNumElements() const { return size / getElementSize(); };
  virtual void destroy() { dispose(this); }

  DECLARE_BUF_CREATOR(DummyVBuffer);

private:
  int elemSz;
};

class DummyIBuffer final : public DummyBufferImpl
{
public:
  DummyIBuffer() : DummyBufferImpl() {}

  virtual int getElementSize() const { return (flags & SBCF_INDEX32) ? 4 : 2; }
  virtual int getNumElements() const { return size / getElementSize(); };
  virtual void destroy() { dispose(this); }

  DECLARE_BUF_CREATOR(DummyIBuffer);
};

class DummyVolTexture final : public VolTexture
{
public:
  DummyVolTexture() { setSize(4, 4, 4, 1); }

  // load color texture

  int getinfo(TextureInfo &i, int level = 0) const override
  {
    if (!ti.mipLevels)
      debug("E-- %s %dx%d,D%d,A%d,L%d", getTexName(), ti.w, ti.h, ti.d, ti.a, ti.mipLevels);
    if (level >= ti.mipLevels)
      level = ti.mipLevels - 1;

    i = ti;
    i.w = max<uint32_t>(1u, ti.w >> level);
    i.h = max<uint32_t>(1u, ti.h >> level);
    i.d = max<uint32_t>(1u, ti.d >> level);
    return 1;
  }

  virtual int lockimg(void **, int &, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };
  virtual int lockimg(void **, int &, int, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };

  virtual int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags)
  {
    row_pitch = (ti.w >> level) * get_bytes_per_pixel(ti.cflg);
    slice_pitch = (ti.h >> level) * row_pitch;
    *data = drvBuffer.getBuffer(slice_pitch * (ti.d >> level), !(flags & TEXLOCK_READ));
    return 1;
  }
  virtual int unlockbox() { return 1; }

  //// BaseTexture ////
  virtual int generateMips() { return 1; }
  int level_count() const override { return ti.mipLevels; }
  virtual int texmiplevel(int /*minlevel*/, int /*maxlevel*/) { return 0; }
  virtual int update(BaseTexture * /*src*/) { return 1; }
  virtual int updateSubRegion(BaseTexture * /*src*/, int /*src_subres_idx*/, int /*src_x*/, int /*src_y*/, int /*src_z*/,
    int /*src_w*/, int /*src_h*/, int /*src_d*/, int /*dest_subres_idx*/, int /*dest_x*/, int /*dest_y*/, int /*dest_z*/)
  {
    return 1;
  }

  //// D3dResource ////
  virtual void destroy() { dispose(this); }
  virtual void preload() {}
  int restype() const override { return RES3D_VOLTEX; }
  int ressize() const override { return memSz; }

  inline void setSize(int w, int h, int d, int lev, int cflg = 0)
  {
    ti.w = w;
    ti.h = h;
    ti.d = d;
    ti.a = 1;
    ti.mipLevels = lev;
    ti.resType = RES3D_VOLTEX;
    ti.cflg = cflg;
    memSz = 0;
    for (unsigned l = 0; l < ti.mipLevels; l++)
      memSz += calc_surface_sz(ti, l) * max<unsigned>(ti.d >> l, 1);
  }
  inline void setTexName(const char *stat_name) { setResName(stat_name); }

  DECLARE_TEX_CREATOR(DummyVolTexture);

public:
  TextureInfo ti;
  unsigned memSz = 0;
  DECLARE_TQL_AND_MAKETMPRES(DummyVolTexture)
protected:
  virtual int texaddrImpl(int /*addrmode*/) { return 1; }
  virtual int texaddruImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texaddrvImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texaddrwImpl(int /*addrmode*/) { return TEXADDR_WRAP; } // default is TEXADDR_WRAP
  virtual int texbordercolorImpl(E3DCOLOR /*color*/) { return 1; }
  virtual int texfilterImpl(int /*filtermode*/) { return TEXFILTER_DEFAULT; }
  virtual int texmipmapImpl(int /*mipmapmode*/) { return TEXMIPMAP_DEFAULT; }
  virtual int texlodImpl(float /*mipmaplod*/) { return 0; }
  virtual int setAnisotropyImpl(int /*level*/) { return 1; }
};

class DummyArrTexture final : public ArrayTexture
{
public:
  DummyArrTexture() { setSize(4, 4, 4, 1); }

  // load color texture
  int getinfo(TextureInfo &i, int level = 0) const override
  {
    if (!ti.mipLevels)
      debug("E-- %s %dx%d,D%d,A%d,L%d", getTexName(), ti.w, ti.h, ti.d, ti.a, ti.mipLevels);
    if (level >= ti.mipLevels)
      level = ti.mipLevels - 1;

    i = ti;
    i.w = max<uint32_t>(1u, ti.w >> level);
    i.h = max<uint32_t>(1u, ti.h >> level);
    return 1;
  }

  virtual int lockimg(void **, int &, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };
  virtual int lockimg(void **, int &, int, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };

  //// BaseTexture ////
  virtual int generateMips() { return 1; }
  int level_count() const override { return ti.mipLevels; }
  virtual int texmiplevel(int /*minlevel*/, int /*maxlevel*/) { return 0; }
  virtual int update(BaseTexture * /*src*/) { return 1; }
  virtual int updateSubRegion(BaseTexture * /*src*/, int /*src_subres_idx*/, int /*src_x*/, int /*src_y*/, int /*src_z*/,
    int /*src_w*/, int /*src_h*/, int /*src_d*/, int /*dest_subres_idx*/, int /*dest_x*/, int /*dest_y*/, int /*dest_z*/)
  {
    return 1;
  }
  bool isCubeArray() const override { return ti.resType == RES3D_CUBEARRTEX; }

  //// D3dResource ////
  virtual void destroy() { dispose(this); }
  virtual void preload() {}
  int restype() const override { return ti.resType; }
  int ressize() const override { return memSz; }

  inline void setSize(int w, int h, int d, int lev, int cflg = 0)
  {
    ti.w = w;
    ti.h = h;
    ti.d = 1;
    ti.a = d;
    ti.mipLevels = lev;
    ti.resType = RES3D_ARRTEX;
    ti.cflg = cflg;
    memSz = 0;
    for (unsigned l = 0; l < ti.mipLevels; l++)
      memSz += calc_surface_sz(ti, l) * ti.a;
  }
  inline void setTexName(const char *stat_name) { setResName(stat_name); }

  DECLARE_TEX_CREATOR(DummyArrTexture);

public:
  TextureInfo ti;
  unsigned memSz = 0;
  DECLARE_TQL_AND_MAKETMPRES(DummyArrTexture)
protected:
  virtual int texaddrImpl(int /*addrmode*/) { return 1; }
  virtual int texaddruImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texaddrvImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texbordercolorImpl(E3DCOLOR /*color*/) { return 1; }
  virtual int texfilterImpl(int /*filtermode*/) { return TEXFILTER_DEFAULT; }
  virtual int texmipmapImpl(int /*mipmapmode*/) { return TEXMIPMAP_DEFAULT; }
  virtual int texlodImpl(float /*mipmaplod*/) { return 0; }
  virtual int setAnisotropyImpl(int /*level*/) { return 1; }
};

class DummyCubeTexture final : public CubeTexture
{
public:
  DummyCubeTexture() { setSize(4, 1); }

  //// CubeTexture ////
  // load color texture

  int getinfo(TextureInfo &i, int level = 0) const override
  {
    if (!ti.mipLevels)
      debug("E-- %s %dx%d,D%d,A%d,L%d", getTexName(), ti.w, ti.h, ti.d, ti.a, ti.mipLevels);
    if (level >= ti.mipLevels)
      level = ti.mipLevels - 1;

    i = ti;
    i.w = i.h = max<uint32_t>(1u, ti.w >> level);
    return 1;
  }

  virtual int lockimg(void **, int &, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };
  virtual int lockimg(void **p, int &stride_bytes, int /*face*/, int /*level*/, unsigned flags)
  {
    const uint32_t BC_BLOCK_SIZE = 4;
    stride_bytes = (is_bc_texformat(ti.cflg) ? ((ti.w + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE) : ti.w) * get_bytes_per_pixel(ti.cflg);
    uint32_t heightInBlocks = (is_bc_texformat(ti.cflg) ? ((ti.h + BC_BLOCK_SIZE - 1) / BC_BLOCK_SIZE) : ti.h);
    *p = drvBuffer.getBuffer(heightInBlocks * stride_bytes, !(flags & TEXLOCK_READ));
    return 1;
  }
  virtual int unlockimg() { return 1; }

  //// BaseTexture ////
  virtual int generateMips() { return 1; }
  int level_count() const override { return ti.mipLevels; }
  virtual int texmiplevel(int /*min_lev*/, int /*max_lev*/) { return 0; }
  virtual int update(BaseTexture * /*src*/) { return 1; }
  virtual int updateSubRegion(BaseTexture * /*src*/, int /*src_subres_idx*/, int /*src_x*/, int /*src_y*/, int /*src_z*/,
    int /*src_w*/, int /*src_h*/, int /*src_d*/, int /*dest_subres_idx*/, int /*dest_x*/, int /*dest_y*/, int /*dest_z*/)
  {
    return 1;
  }

  //// D3dResource ////
  virtual void destroy() { dispose(this); }
  virtual void preload() {}
  int restype() const override { return RES3D_CUBETEX; }
  int ressize() const override { return memSz; }

  inline void setSize(int sz, int lev, int cflg = 0)
  {
    ti.w = sz;
    ti.h = sz;
    ti.d = 1;
    ti.a = 6;
    ti.mipLevels = lev;
    ti.resType = RES3D_CUBETEX;
    ti.cflg = cflg;
    memSz = 0;
    for (unsigned l = 0; l < ti.mipLevels; l++)
      memSz += calc_surface_sz(ti, l) * 6;
  }
  inline void setTexName(const char *stat_name) { setResName(stat_name); }

  DECLARE_TEX_CREATOR(DummyCubeTexture);

public:
  TextureInfo ti;
  unsigned memSz = 0;
  DECLARE_TQL_AND_MAKETMPRES(DummyCubeTexture)
protected:
  virtual int texaddrImpl(int /*addrmode*/) { return 1; }
  virtual int texaddruImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texaddrvImpl(int /*addrmode*/) { return TEXADDR_WRAP; }
  virtual int texbordercolorImpl(E3DCOLOR /*color*/) { return 1; }
  virtual int texfilterImpl(int /*filtermode*/) { return TEXFILTER_DEFAULT; }
  virtual int texmipmapImpl(int /*mipmapmode*/) { return TEXMIPMAP_DEFAULT; }
  virtual int texlodImpl(float /*mipmaplod*/) { return 0; }
  virtual int setAnisotropyImpl(int /*level*/) { return 1; }
};

class DummyTexture final : public Texture
{
public:
  DummyTexture() { setSize(4, 4, 1); }

  // from  D3dResource
  virtual void destroy() { dispose(this); }

  int restype() const override { return RES3D_TEX; }
  int ressize() const override { return memSz; }
  virtual void preload() {}

  // from  BaseTexture
  virtual int generateMips() { return 1; }
  virtual int update(BaseTexture * /*src*/) { return 1; } // update texture from
  virtual int updateSubRegion(BaseTexture * /*src*/, int /*src_subres_idx*/, int /*src_x*/, int /*src_y*/, int /*src_z*/,
    int /*src_w*/, int /*src_h*/, int /*src_d*/, int /*dest_subres_idx*/, int /*dest_x*/, int /*dest_y*/, int /*dest_z*/)
  {
    return 1;
  }
  int level_count() const override { return ti.mipLevels; }
  virtual int texmiplevel(int /*min_lev*/, int /*max_lev*/) { return 0; }

  virtual int lockimg(void **p, int &stride_bytes, int /*level*/, unsigned flags)
  {
    stride_bytes = ti.w * get_bytes_per_pixel(ti.cflg);
    if (!p) // delayed lock
      return 1;
    *p = drvBuffer.getBuffer(ti.w * ti.h * get_bytes_per_pixel(ti.cflg), !(flags & TEXLOCK_READ));
    return 1;
  }
  virtual int lockimg(void **, int &, int, int = 0, unsigned = TEXLOCK_DEFAULT) { return 0; };

  virtual int unlockimg() { return 1; }

  int getinfo(TextureInfo &i, int level = 0) const override
  {
    if (!ti.mipLevels)
      debug("E-- %s %dx%d,D%d,A%d,L%d", getTexName(), ti.w, ti.h, ti.d, ti.a, ti.mipLevels);
    if (level >= ti.mipLevels)
      level = ti.mipLevels - 1;

    i = ti;
    i.w = max<uint32_t>(1u, ti.w >> level);
    i.h = max<uint32_t>(1u, ti.h >> level);
    return 1;
  }

  inline void setSize(int w, int h, int lev, int cflg = 0)
  {
    ti.w = w;
    ti.h = h;
    ti.d = 1;
    ti.a = 1;
    ti.mipLevels = lev;
    ti.resType = RES3D_TEX;
    ti.cflg = cflg;
    memSz = 0;
    for (unsigned l = 0; l < ti.mipLevels; l++)
      memSz += calc_surface_sz(ti, l);
  }
  inline void setTexName(const char *stat_name) { setResName(stat_name); }

  inline void getSize(IPoint2 &sz)
  {
    sz.x = ti.w;
    sz.y = ti.h;
  }

  DECLARE_TEX_CREATOR(DummyTexture);

public:
  TextureInfo ti;
  unsigned memSz = 0;
  DECLARE_TQL_AND_MAKETMPRES(DummyTexture)
protected:
  virtual int texaddrImpl(int /*addrmode*/) { return 1; }             // set texaddru,texaddrv,...
  virtual int texaddruImpl(int /*addrmode*/) { return TEXADDR_WRAP; } // default is TEXADDR_WRAP
  virtual int texaddrvImpl(int /*addrmode*/) { return TEXADDR_WRAP; } // default is TEXADDR_WRAP
  virtual int texbordercolorImpl(E3DCOLOR) { return 1; }
  virtual int texfilterImpl(int /*filtermode*/) { return TEXFILTER_DEFAULT; } // default is TEXFILTER_DEFAULT
  virtual int texmipmapImpl(int /*mipmapmode*/) { return TEXMIPMAP_DEFAULT; } // default is TEXMIPMAP_DEFAULT
  virtual int texlodImpl(float /*mipmaplod*/) { return 0; }                   // default is zero. Sets texture lod bias
  virtual int setAnisotropyImpl(int /*level*/) { return 1; }                  // default is 1.
};

static DummyTexture backBufTex, backBufDepthTex;
static ViewData viewData;
static float screen_aspect_ratio = 1.0;


static bool drv_inited = false;
static Driver3dDesc stub_desc = {
  0, 0, // int zcmpfunc,acmpfunc;
  0, 0, // int sblend,dblend;

  1, 1,         // int mintexw,mintexh;
  16384, 16384, // int maxtexw,maxtexh;
  1, 16384,     // int mincubesize,maxcubesize;
  1, 16384,     // int minvolsize,maxvolsize;
  0,            // int maxtexaspect; ///< 1 means texture should be square, 0 means no limit
  65536,        // int maxtexcoord;
  16,           // int maxsimtex;
  8,            // int maxlights;
  8,            // int maxclipplanes;
  64,           // int maxstreams;
  64,           // int maxstreamstr;
  1024,         // int maxvpconsts;


  65536 * 4, 65536 * 4, // int maxprims,maxvertind;
  0.5f, 0.5f,           // float upixofs,vpixofs;

  //! fragment shader version flags

  8,    // int maxSimRT;
  true, // bool is20ArbitrarySwizzleAvailable;
  32,   // minWarpSize
  64    // maxWarpSize
};

static Driver3dRenderTarget currentRtState;

bool d3d::init_driver()
{
  drv_inited = true;
  g_frameState.init();

  viewData.x = 0;
  viewData.y = 0;

  viewData.w = 1;
  viewData.h = 1;

  viewData.minz = 0.0;
  viewData.maxz = 1.0;

  dgs_set_window_mode(WindowMode::WINDOWED);

#if _TARGET_ANDROID
  stub_desc.caps.hasAnisotropicFilter = true;
#endif

#if _TARGET_PC_WIN
  stub_desc.caps.hasDepthReadOnly = true;
  stub_desc.caps.hasStructuredBuffers = true;
  stub_desc.caps.hasNoOverwriteOnShaderResourceBuffers = true;
  stub_desc.caps.hasForcedSamplerCount = true;
  stub_desc.caps.hasVolMipMap = true;
  stub_desc.caps.hasOcclusionQuery = true;
  stub_desc.caps.hasConstBufferOffset = true;
  stub_desc.caps.hasResourceCopyConversion = true;
  stub_desc.caps.hasReadMultisampledDepth = true;
  stub_desc.caps.hasQuadTessellation = true;
  stub_desc.caps.hasGather4 = true;
  stub_desc.caps.hasUAVOnEveryStage = true;
#endif

#if _TARGET_PC_WIN || _TARGET_C1 || _TARGET_C1
  stub_desc.caps.hasAsyncCompute = true;
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID
  stub_desc.caps.hasDepthBoundsTest = true;
  stub_desc.caps.hasInstanceID = true;
  stub_desc.caps.hasConservativeRassterization = true;
  stub_desc.caps.hasWellSupportedIndirect = true;
#endif

#if _TARGET_PC_WIN || _TARGET_PC_LINUX || _TARGET_ANDROID || _TARGET_C3
  stub_desc.caps.hasConditionalRender = true;
#endif

  stub_desc.shaderModel = 5.0_sm;

  return true;
}

bool d3d::is_inited() { return drv_inited; }

#if _TARGET_PC_WIN
static main_wnd_f *main_wnd_proc = NULL;

static LRESULT FAR PASCAL WindowProcProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_CREATE: break;
    case WM_DESTROY: break;
    case WM_ERASEBKGND:
    case WM_PAINT: return DefWindowProcW(hWnd, message, wParam, lParam);
  }

  if (main_wnd_proc)
    return main_wnd_proc(hWnd, message, wParam, lParam);
  return DefWindowProcW(hWnd, message, wParam, lParam);
}
#endif

#define DEF_W 1280
#define DEF_H 720

bool d3d::init_video(void *hinst, main_wnd_f *wnd_proc, const char *wcname, int /*ncmdshow*/, void *&mainwnd, void *renderwnd,
  void *hicon, const char * /*title*/, Driver3dInitCallback * /*cb*/)
{
  DataBlock empty;
  bool has_stg = (dgs_get_settings && dgs_get_settings());
  const DataBlock &blk_video = has_stg ? *dgs_get_settings()->getBlockByNameEx("stub3d") : empty;

  thread_idx = 0 + 1;
  int drvBufSz = blk_video.getInt("bufferSize", 64 << 20); // default is enough for 4096x4096 RGBA
  drvBuffer.initBufSize(drvBufSz);
  drvBuffer.autoReallocSet(blk_video.getBool("autoBufferReallocate", false));
  useTexDynPool = blk_video.getBool("useDynamicTexturePool", true);

  IPoint2 rtTexSizeCur;
  rtTexSizeCur.x = blk_video.getInt("screen_width", DEF_W);
  rtTexSizeCur.y = blk_video.getInt("screen_height", DEF_H);
  screen_aspect_ratio = rtTexSizeCur.y ? float(rtTexSizeCur.x) / float(rtTexSizeCur.y) : 1.0f;

  backBufTex.setSize(rtTexSizeCur.x, rtTexSizeCur.y, 1);
  backBufTex.setTexName("back_buffer");
  backBufDepthTex.setTexName("back_buffer_depth");
  backBufDepthTex.setSize(rtTexSizeCur.x, rtTexSizeCur.y, 1, TEXFMT_DEPTH32);

  const int poolSz = blk_video.getInt("maxTexturePoolSize", 4096);

  DummyTexture::initPool(poolSz);
  DummyCubeTexture::initPool(poolSz);
  DummyVolTexture::initPool(poolSz);
  DummyArrTexture::initPool(poolSz);

  vb_ib_data_mandatory_support = blk_video.getBool("vb_ib_dataSupport", has_stg ? false : true);
  debug("vb_ib_data_mandatory_support=%d", vb_ib_data_mandatory_support);

  vb_ib_data_flag_support = blk_video.getBool("vb_ib_dataFlagSupport", true);
  debug("vb_ib_data_flag_support=%d", vb_ib_data_flag_support);

#if _TARGET_PC_WIN
  main_wnd_proc = wnd_proc;
  if (wcname)
  {
    // Register a window class
    WNDCLASSW wndClass;
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndClass.lpfnWndProc = WindowProcProxy;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = (HINSTANCE)hinst;
    wndClass.hIcon = (HICON)hicon;
    wndClass.hCursor = LoadCursor(NULL, IDC_CROSS);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = L"DagorStubD3DWndClass";

    if (!RegisterClassW(&wndClass))
      return false;
  }

  // Create the render window
  if (!renderwnd && wcname)
  {
    mainwnd = CreateWindowExW(WS_EX_APPWINDOW, L"DagorStubD3DWndClass", L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 320,
      240, NULL, NULL, (HINSTANCE)hinst, NULL);

    if (!mainwnd)
      return false;
  }
  else
    mainwnd = renderwnd;

  // Show the window
  if (mainwnd)
  {
    ShowWindow((HWND)mainwnd, SW_SHOW);
    UpdateWindow((HWND)mainwnd);
  }
#else
  (void)hinst;
  (void)wnd_proc;
  (void)wcname;
  (void)mainwnd;
  (void)renderwnd;
  (void)hicon;
#endif

  debug("inited video: %dx%d stub3d [bufferSize=%dK, preallocateTexCount=%d useDynTexPool=%d]", rtTexSizeCur.x, rtTexSizeCur.y,
    drvBufSz >> 10, poolSz, useTexDynPool);
  d3d::set_render_target();
  return true;
}

void d3d::release_driver() { drv_inited = false; }

#include "driver_code.h"
DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::stub); }
const char *d3d::get_driver_name() { return "d3d/dummy"; }
const char *d3d::get_device_name() { return "device/dummy"; }
const char *d3d::get_last_error() { return "n/a"; }
uint32_t d3d::get_last_error_code() { return 0; }
const char *d3d::get_device_driver_version() { return "1.0"; }

void d3d::prepare_for_destroy() { on_before_window_destroyed(); }
void d3d::window_destroyed(void * /*hwnd*/) {}

void *d3d::get_device() { return NULL; }

/// returns driver description (pointer to static object)
const Driver3dDesc &d3d::get_driver_desc() { return stub_desc; }

int d3d::driver_command(Drv3dCommand command, void * /*par1*/, void * /*par2*/, void * /*par3*/)
{
  // of course we can actually track when we need mt sync or not (just like in drv3d_DX9, see needMtSync var),
  // but much simpler just always lock & unlock
  if (command == Drv3dCommand::ACQUIRE_OWNERSHIP)
    dummy_crit.lock();
  else if (command == Drv3dCommand::RELEASE_OWNERSHIP)
    dummy_crit.unlock();
  return 0;
}

bool d3d::device_lost(bool * /*can_reset_now*/) { return false; }
bool d3d::reset_device() { return true; }
bool d3d::is_in_device_reset_now() { return false; }
bool d3d::is_window_occluded() { return false; }

bool d3d::should_use_compute_for_image_processing(std::initializer_list<unsigned>) { return false; }

static bool is_depth_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg >= TEXFMT_FIRST_DEPTH && cflg <= TEXFMT_LAST_DEPTH;
}

unsigned d3d::get_texformat_usage(int cflg, int /*restype*/)
{
  uint32_t ret =
    USAGE_FILTER | USAGE_VERTEXTEXTURE | USAGE_BLEND | USAGE_RTARGET | USAGE_TEXTURE | USAGE_PIXREADWRITE | USAGE_UNORDERED;
  if (is_depth_format_flg(cflg))
    ret |= USAGE_DEPTH;
  return ret;
}
bool d3d::check_texformat(int /*cflg*/) { return true; }
int d3d::get_max_sample_count(int) { return 1; }
bool d3d::issame_texformat(int cflg1, int cflg2) { return cflg1 == cflg2; }
bool d3d::check_cubetexformat(int /*cflg*/) { return true; }

bool d3d::check_voltexformat(int /*cflg*/) { return true; }

Texture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  if (img)
  {
    if (!w)
      w = img->w;
    if (!h)
      h = img->h;
  }
  RETURN_NULL_ON_ZERO_SIZE(!w || !h);
  DummyTexture *tex = DummyTexture::create();
  if (!levels)
    levels = get_log2i(max(w, h));
  tex->setSize(w, h, levels, flg);
  tex->setTexName(stat_name);
  return tex;
}

BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int quality_id, int levels, const char *stat_name, int)
{
  flg = implant_d3dformat(flg, hdr.d3dFormat);
  G_ASSERT((flg & TEXCF_TYPEMASK) != TEXCF_RTARGET);

  if (hdr.flags & ddsx::Header::FLG_ARRTEX)
    return create_array_tex(hdr.w, hdr.h, hdr.depth, flg, hdr.levels, stat_name);
  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    return create_cubetex(hdr.w, flg, hdr.levels, stat_name);
  if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    return create_voltex(hdr.w, hdr.h, hdr.depth, flg, hdr.levels, stat_name);

  RETURN_NULL_ON_ZERO_SIZE(!hdr.w || !hdr.h);
  int skip_levels = hdr.getSkipLevels(hdr.getSkipLevelsFromQ(quality_id), levels);
  int w = hdr.w >> skip_levels, h = hdr.h >> skip_levels;
  if (!w)
    w = 1;
  if (!h)
    h = 1;

  return create_tex(NULL, w, h, flg, levels, stat_name);
}

BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *stat_name)
{
  ddsx::Header hdr;
  if (!crd.readExact(&hdr, sizeof(hdr)) || !hdr.checkLabel())
  {
    debug("expected ddsx format");
    return 0;
  }

  BaseTexture *bt = alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name);

  if (!bt)
    return NULL;

  if (load_ddsx_tex_contents(bt, hdr, crd, quality_id) != TexLoadRes::OK)
  {
    del_d3dres(bt);
    return NULL;
  }
  return bt;
}


CubeTexture *d3d::create_cubetex(int size, int flg, int levels, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!size);
  DummyCubeTexture *tex = DummyCubeTexture::create();
  if (!levels)
    levels = get_log2i(size);
  tex->setSize(size, levels, flg);
  tex->setTexName(stat_name);
  return tex;
}

VolTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!w || !h || !d);
  DummyVolTexture *tex = DummyVolTexture::create();
  if (!levels)
    levels = get_log2i(max(max(w, h), d));
  tex->setSize(w, h, d, levels, flg);
  tex->setTexName(stat_name);
  return tex;
}

ArrayTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!w || !h || !d);
  DummyArrTexture *tex = DummyArrTexture::create();
  if (!levels)
    levels = get_log2i(max(w, h));
  tex->setSize(w, h, d, levels, flg);
  tex->setTexName(stat_name);
  return tex;
}

ArrayTexture *d3d::create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!side || !d);
  DummyArrTexture *tex = DummyArrTexture::create();
  if (!levels)
    levels = get_log2i(side);
  tex->setSize(side, side, d * 6, levels, flg);
  tex->setTexName(stat_name);
  tex->ti.resType = RES3D_CUBEARRTEX;
  return tex;
}

bool d3d::stretch_rect(BaseTexture * /*src*/, BaseTexture * /*dst*/, RectInt * /*rsrc*/, RectInt * /*rdst*/) { return true; }
bool d3d::copy_from_current_render_target(BaseTexture * /*to_tex*/) { return true; }

// Texture states setup
VPROG d3d::create_vertex_shader(const uint32_t * /*native_code*/) { return 1; }
void d3d::delete_vertex_shader(VPROG /*vs*/) {}

bool d3d::set_const(unsigned, unsigned /*reg_base*/, const void * /*data*/, unsigned /*num_regs*/) { return true; }
bool d3d::set_immediate_const(unsigned, const uint32_t *, unsigned) { return true; }

int d3d::set_cs_constbuffer_size(int required_size) { return required_size; }
int d3d::set_vs_constbuffer_size(int required_size) { return required_size > 0 ? required_size : 256; }
FSHADER d3d::create_pixel_shader(const uint32_t * /*native_code*/) { return 1; }
void d3d::delete_pixel_shader(FSHADER /*ps*/) {}

#if _TARGET_PC_WIN
VPROG d3d::create_vertex_shader_hlsl(const char *, unsigned, const char *, const char *, String *) { return 1; }
FSHADER d3d::create_pixel_shader_hlsl(const char *, unsigned, const char *, const char *, String *) { return 1; }
bool d3d::compile_compute_shader_hlsl(const char *, unsigned, const char *, const char *, Tab<uint32_t> &, String &) { return true; }
#endif
PROGRAM d3d::get_debug_program() { return 1; }
#if (_TARGET_PC | _TARGET_XBOX)
VPROG d3d::create_vertex_shader_dagor(const VPRTYPE * /*tokens*/, int /*len*/) { return 1; }
VPROG d3d::create_vertex_shader_asm(const char * /*asm_text*/) { return 1; }
FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE * /*tokens*/, int /*len*/) { return 1; }
FSHADER d3d::create_pixel_shader_asm(const char * /*asm_text*/) { return 1; }
#endif
PROGRAM d3d::create_program(VPROG, FSHADER, VDECL, unsigned *, unsigned) { return 1; }
// if strides & streams are unset, will get them from VDECL
//  should be deleted externally

PROGRAM d3d::create_program(const uint32_t *, const uint32_t *, VDECL, unsigned *, unsigned) { return 1; }

PROGRAM d3d::create_program_cs(const uint32_t * /*cs_native*/, CSPreloaded) { return 1; }

bool d3d::set_program(PROGRAM) { return true; }
void d3d::delete_program(PROGRAM) {}
#if (_TARGET_PC | _TARGET_XBOX)
bool d3d::set_pixel_shader(FSHADER) { return true; }
bool d3d::set_vertex_shader(VPROG) { return true; }
VDECL d3d::get_program_vdecl(PROGRAM) { return 1; }
#endif

Vbuffer *d3d::create_vb(int b_size, int flags, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!b_size);
  DummyVBuffer *vb = DummyVBuffer::create(b_size, 0, flags | SBCF_BIND_VERTEX, stat_name);
  return vb;
}
Ibuffer *d3d::create_ib(int b_size, int flags, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(!b_size);
  return DummyIBuffer::create(b_size, 0, flags | SBCF_BIND_INDEX, stat_name);
}
Sbuffer *create_sbuffer(int struct_size, int elements, unsigned flags, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(struct_size * elements == 0);
  if (DummyVBuffer *buf = DummyVBuffer::create(struct_size * elements, 0, flags, stat_name))
  {
    buf->setElemSz(struct_size);
    return buf;
  }
  return NULL;
}

Vbuffer *d3d::create_sbuffer(int struct_size, int elements, unsigned flags, unsigned /*format*/, const char *stat_name)
{
  RETURN_NULL_ON_ZERO_SIZE(struct_size * elements == 0);
  return DummyVBuffer::create(struct_size * elements, 0, flags, stat_name);
}

bool d3d::draw_indirect(int, Sbuffer *, uint32_t) { return true; }
bool d3d::draw_indexed_indirect(int, Sbuffer *, uint32_t) { return true; }
bool d3d::multi_draw_indirect(int, Sbuffer *, uint32_t, uint32_t, uint32_t) { return true; }
bool d3d::multi_draw_indexed_indirect(int, Sbuffer *, uint32_t, uint32_t, uint32_t) { return true; }
bool d3d::dispatch_indirect(Sbuffer *, uint32_t, GpuPipeline) { return true; }
void d3d::dispatch_mesh(uint32_t, uint32_t, uint32_t) {}
void d3d::dispatch_mesh_indirect(Sbuffer *, uint32_t, uint32_t, uint32_t) {}
void d3d::dispatch_mesh_indirect_count(Sbuffer *, uint32_t, uint32_t, Sbuffer *, uint32_t, uint32_t) {}
bool d3d::set_const_buffer(uint32_t, uint32_t, Sbuffer *, uint32_t, uint32_t) { return true; }
bool d3d::set_const_buffer(unsigned, unsigned, const float *, unsigned) { return true; }

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline /*gpu_pipeline*/) { return BAD_GPUFENCEHANDLE; }
void d3d::insert_wait_on_fence(GPUFENCEHANDLE & /*fence*/, GpuPipeline /*gpu_pipeline*/) {}

bool d3d::set_render_target()
{
  currentRtState.setBackbufColor();
  currentRtState.removeDepth();
  return true;
}

bool d3d::set_depth(BaseTexture *tex, DepthAccess access)
{
  if (!tex)
    currentRtState.removeDepth();
  else
    currentRtState.setDepth(tex, access == DepthAccess::SampledRO);
  return true;
}

bool d3d::set_depth(BaseTexture *tex, int face, DepthAccess access)
{
  if (!tex)
    currentRtState.removeDepth();
  else
    currentRtState.setDepth(tex, face, access == DepthAccess::SampledRO);
  return true;
}

bool d3d::set_render_target(int ri, Texture *tex, uint8_t level)
{
  if (!tex)
    currentRtState.removeColor(ri);
  else
    currentRtState.setColor(ri, tex, level, 0);
  return true;
}

bool d3d::set_render_target(int ri, BaseTexture *ctex, int fc, uint8_t level)
{
  if (!ctex)
    currentRtState.removeColor(ri);
  else
    currentRtState.setColor(ri, ctex, level, fc);
  return true;
}

bool d3d::set_render_target(const Driver3dRenderTarget &rt)
{
  currentRtState = rt;
  return true;
}

void d3d::get_render_target(Driver3dRenderTarget &out_rt) { out_rt = currentRtState; }

bool d3d::get_target_size(int &w, int &h)
{
  if (currentRtState.isBackBufferColor())
    return d3d::get_render_target_size(w, h, NULL, 0);
  else if (currentRtState.isColorUsed(0))
    return d3d::get_render_target_size(w, h, currentRtState.color[0].tex, currentRtState.color[0].level);
  else if (currentRtState.isDepthUsed() && currentRtState.depth.tex)
    return d3d::get_render_target_size(w, h, currentRtState.depth.tex, currentRtState.depth.level);
  return false;
}

bool d3d::get_render_target_size(int &w, int &h, BaseTexture *rt_tex, uint8_t level)
{
  w = h = 0;
  if (!rt_tex)
    return get_render_target_size(w, h, &backBufTex, 0);
  else if (rt_tex->restype() == RES3D_TEX || rt_tex->restype() == RES3D_CUBETEX || rt_tex->restype() == RES3D_ARRTEX)
  {
    TextureInfo i;
    rt_tex->getinfo(i, level);
    w = i.w;
    h = i.h;
    return true;
  }
  return false;
}


bool d3d::setview(int x, int y, int w, int h, float minz, float maxz)
{
  viewData.x = x;
  viewData.y = y;

  viewData.w = w;
  viewData.h = h;

  viewData.minz = minz;
  viewData.maxz = maxz;

  return true;
}

bool d3d::setviews(dag::ConstSpan<Viewport> viewports)
{
  G_UNUSED(viewports);
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::setscissors(dag::ConstSpan<ScissorRect> scissorRects)
{
  G_UNUSED(scissorRects);
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::getview(int &x, int &y, int &w, int &h, float &minz, float &maxz)
{
  x = viewData.x;
  y = viewData.y;
  w = viewData.w;
  h = viewData.h;
  minz = viewData.minz;
  maxz = viewData.maxz;

  return true;
}

bool d3d::setscissor(int /*x*/, int /*y*/, int /*w*/, int /*h*/) { return true; }

bool d3d::clearview(int /*what*/, E3DCOLOR, float /*z*/, uint32_t /*stencil*/) { return true; }

bool d3d::update_screen(bool /*app_active*/)
{
  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

#if _TARGET_IOS | _TARGET_ANDROID | _TARGET_XBOX
  static int lastFpsTime = 0;
  static unsigned int lastFrameNo = 0;
  if (::get_time_msec() > lastFpsTime + 3000)
  {
    debug("%5d: FPS=%.1f", ::dagor_frame_no(), 1000.f * (::dagor_frame_no() - lastFrameNo) / (float)(::get_time_msec() - lastFpsTime));

    lastFpsTime = ::get_time_msec();
    lastFrameNo = ::dagor_frame_no();
  }
#endif
  return true;
}

void d3d::wait_for_async_present(bool) {}
void d3d::gpu_latency_wait() {}

/// set vertex stream source
bool d3d::setvsrc_ex(int /*stream*/, Vbuffer *, int /*ofs*/, int /*stride_bytes*/) { return true; }

/// set indices
bool d3d::setind(Ibuffer * /*ib*/) { return true; }

VDECL d3d::create_vdecl(VSDTYPE * /*vsd*/) { return 1; }
bool d3d::setvdecl(VDECL /*vdecl*/) { return true; }
void d3d::delete_vdecl(VDECL /*vdecl*/) {}

bool d3d::draw_base(int /*type*/, int /*start*/, int /*numprim*/, uint32_t /*num_inst*/, uint32_t /*start_instance*/) { return true; }
bool d3d::drawind_base(int /*type*/, int /*startind*/, int /*numprim*/, int /*base_vertex*/, uint32_t /*num_inst*/,
  uint32_t /*start_instance*/)
{
  return true;
}
bool d3d::draw_up(int /*type*/, int /*numprim*/, const void * /*ptr*/, int /*stride_bytes*/) { return true; }
bool d3d::drawind_up(int /*type*/, int /*minvert*/, int /*numvert*/, int /*numprim*/, const uint16_t * /*ind*/, const void * /*ptr*/,
  int /*stride_bytes*/)
{
  return true;
}

bool d3d::dispatch(uint32_t /*thread_group_x*/, uint32_t /*thread_group_y*/, uint32_t /*thread_group_z*/, GpuPipeline /*gpu_pipeline*/)
{
  return false;
}

shaders::DriverRenderStateId d3d::create_render_state(const shaders::RenderState &)
{
  return shaders::DriverRenderStateId(shaders::DriverRenderStateId::INVALID_ID + 1);
}
bool d3d::set_render_state(shaders::DriverRenderStateId) { return true; }
void d3d::clear_render_states() {}

bool d3d::setstencil(uint32_t /*ref*/) { return true; }

bool d3d::setwire(bool /*in*/) { return true; }

bool d3d::setgamma(float) { return true; }

float d3d::get_screen_aspect_ratio() { return screen_aspect_ratio; }
void d3d::change_screen_aspect_ratio(float) {}

void *d3d::fast_capture_screen(int & /*w*/, int & /*h*/, int & /*stride_bytes*/, int & /*format*/) { return NULL; }
void d3d::end_fast_capture_screen() {}

TexPixel32 *d3d::capture_screen(int & /*w*/, int & /*h*/, int & /*stride_bytes*/) { return NULL; }
void d3d::release_capture_buffer() {}

void d3d::get_screen_size(int &w, int &h)
{
  IPoint2 size;
  backBufTex.getSize(size);
  w = size.x;
  h = size.y;
}

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *)
{
  if (num_textures)
    *num_textures = DummyTexture::entCnt() + DummyCubeTexture::entCnt() + DummyVolTexture::entCnt();
  if (total_mem)
    *total_mem = 100;
}

bool d3d::set_blend_factor(E3DCOLOR) { return true; }

d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &) { return d3d::SamplerHandle(0); }
void d3d::set_sampler(unsigned, unsigned, d3d::SamplerHandle) {}
uint32_t d3d::register_bindless_sampler(BaseTexture *) { return 0; }
uint32_t d3d::register_bindless_sampler(SamplerHandle) { return 0; }

uint32_t d3d::allocate_bindless_resource_range(uint32_t, uint32_t) { return 0; }
uint32_t d3d::resize_bindless_resource_range(uint32_t, uint32_t, uint32_t, uint32_t) { return 0; }
void d3d::free_bindless_resource_range(uint32_t, uint32_t, uint32_t) {}
void d3d::update_bindless_resource(uint32_t, D3dResource *) {}
void d3d::update_bindless_resources_to_null(uint32_t, uint32_t, uint32_t) {}

bool d3d::set_tex(unsigned, unsigned, BaseTexture *, bool) { return true; }
bool d3d::set_rwtex(unsigned, unsigned, BaseTexture *, uint32_t, uint32_t, bool) { return true; }
bool d3d::clear_rwtexi(BaseTexture *, const unsigned[4], uint32_t, uint32_t) { return true; }
bool d3d::clear_rwtexf(BaseTexture *, const float[4], uint32_t, uint32_t) { return true; }
bool d3d::clear_rwbufi(Sbuffer *, const unsigned[4]) { return true; }
bool d3d::clear_rwbuff(Sbuffer *, const float[4]) { return true; }

bool d3d::clear_rt(const RenderTarget &, const ResourceClearValue &) { return true; }

bool d3d::set_buffer(unsigned, unsigned, Sbuffer *) { return true; }
bool d3d::set_rwbuffer(unsigned, unsigned, Sbuffer *) { return true; }

STATIC_TEX_POOL_DECL(DummyVolTexture);
STATIC_TEX_POOL_DECL(DummyCubeTexture);
STATIC_TEX_POOL_DECL(DummyArrTexture);
STATIC_TEX_POOL_DECL(DummyTexture);
STATIC_BUF_POOL_DECL(DummyVBuffer);
STATIC_BUF_POOL_DECL(DummyIBuffer);


void d3d::reserve_res_entries(bool /*strict_max*/, int max_tex, int /*max_vs*/, int /*max_ps*/, int /*max_vdecl*/, int max_vb,
  int max_ib, int /*max_stblk*/)
{
  DummyTexture::reserve(max_tex);
  DummyCubeTexture::reserve(max_tex / 4); // heuristic
  DummyVolTexture::reserve(max_tex / 8);  // heuristic
  DummyVBuffer::reserve(max_vb);
  DummyIBuffer::reserve(max_ib);
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = DummyTexture::entMax() + DummyCubeTexture::entMax() + DummyVolTexture::entMax();
  max_vb = DummyVBuffer::entMax();
  max_ib = DummyIBuffer::entMax();
  max_vs = max_ps = max_vdecl = max_stblk = 0;
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = DummyTexture::entCnt() + DummyCubeTexture::entCnt() + DummyVolTexture::entCnt();
  max_vb = DummyVBuffer::entCnt();
  max_ib = DummyIBuffer::entCnt();
  max_vs = max_ps = max_vdecl = max_stblk = 0;
}

d3d::EventQuery *d3d::create_event_query() { return (EventQuery *)1; }

void d3d::release_event_query(EventQuery *) {}
bool d3d::issue_event_query(EventQuery *query) { return query ? true : false; }
bool d3d::get_event_query_status(d3d::EventQuery *, bool) { return true; }

int d3d::create_predicate() { return -1; }
void d3d::free_predicate(int) {}
bool d3d::begin_survey(int) { return false; }
void d3d::end_survey(int) {}
void d3d::begin_conditional_render(int) {}
void d3d::end_conditional_render(int) {}
bool d3d::set_depth_bounds(float, float) { return true; }

#if _TARGET_PC_WIN
bool d3d::pcwin32::set_capture_full_frame_buffer(bool /*ison*/) { return false; }
void d3d::pcwin32::set_present_wnd(void *) {}
unsigned d3d::pcwin32::get_texture_format(BaseTexture *) { return 0; }
const char *d3d::pcwin32::get_texture_format_str(BaseTexture *) { return "n/a"; }
void *d3d::pcwin32::get_native_surface(BaseTexture *) { return nullptr; }

#endif

#if _TARGET_PC | _TARGET_XBOX
bool d3d::get_vrr_supported() { return false; }
bool d3d::get_vsync_enabled() { return false; }
bool d3d::enable_vsync(bool enable) { return !enable; }
void d3d::get_video_modes_list(Tab<String> &list) { clear_and_shrink(list); }
#endif

#if _TARGET_ANDROID
void android_d3d_reinit(void *) {}
void android_update_window_size(void *) {}
#endif

static bool srgb_bb_on = false;
bool d3d::set_srgb_backbuffer_write(bool on)
{
  bool old = srgb_bb_on;
  srgb_bb_on = on;
  return old;
}

void d3d::beginEvent(const char *) {}
void d3d::endEvent() {}

Texture *d3d::get_backbuffer_tex() { return &backBufTex; }
Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }

#include "../drv3d_commonCode/rayTracingStub.inc.cpp"

void d3d::set_variable_rate_shading(unsigned, unsigned, VariableRateShadingCombiner, VariableRateShadingCombiner) {}

void d3d::set_variable_rate_shading_texture(BaseTexture *) {}

void d3d::resource_barrier(ResourceBarrierDesc, GpuPipeline) {}

Texture *d3d::alias_tex(Texture *, TexImage32 *, int, int, int, int, const char *) { return nullptr; }

CubeTexture *d3d::alias_cubetex(CubeTexture *, int, int, int, const char *) { return nullptr; }

VolTexture *d3d::alias_voltex(VolTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_array_tex(ArrayTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *, int, int, int, int, const char *) { return nullptr; }

ResourceAllocationProperties d3d::get_resource_allocation_properties(const ResourceDescription &desc)
{
  G_UNUSED(desc);
  return {};
}
ResourceHeap *d3d::create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  G_UNUSED(heap_group);
  G_UNUSED(size);
  G_UNUSED(flags);
  return nullptr;
}
void d3d::destroy_resource_heap(ResourceHeap *heap) { G_UNUSED(heap); }
Sbuffer *d3d::place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_UNUSED(heap);
  G_UNUSED(desc);
  G_UNUSED(offset);
  G_UNUSED(alloc_info);
  G_UNUSED(name);
  return nullptr;
}
BaseTexture *d3d::place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_UNUSED(heap);
  G_UNUSED(desc);
  G_UNUSED(offset);
  G_UNUSED(alloc_info);
  G_UNUSED(name);
  return nullptr;
}
ResourceHeapGroupProperties d3d::get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  G_UNUSED(heap_group);
  return {};
};
void d3d::map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  G_UNUSED(tex);
  G_UNUSED(mapping);
  G_UNUSED(mapping_count);
  G_UNUSED(heap);
}
TextureTilingInfo d3d::get_texture_tiling_info(BaseTexture *tex, size_t subresource)
{
  G_UNUSED(tex);
  G_UNUSED(subresource);
  return TextureTilingInfo{};
}
void d3d::activate_buffer(Sbuffer *buf, ResourceActivationAction action, const ResourceClearValue &value,
  GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  G_UNUSED(buf);
  G_UNUSED(gpu_pipeline);
  G_UNUSED(action);
  G_UNUSED(value);
}
void d3d::activate_texture(BaseTexture *tex, ResourceActivationAction action, const ResourceClearValue &value,
  GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  G_UNUSED(tex);
  G_UNUSED(action);
  G_UNUSED(value);
  G_UNUSED(gpu_pipeline);
}
void d3d::deactivate_buffer(Sbuffer *buf, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  G_UNUSED(buf);
  G_UNUSED(gpu_pipeline);
}
void d3d::deactivate_texture(BaseTexture *tex, GpuPipeline gpu_pipeline /*= GpuPipeline::GRAPHICS*/)
{
  G_UNUSED(tex);
  G_UNUSED(gpu_pipeline);
}
d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex_region(BaseTexture *, unsigned, unsigned, unsigned, unsigned, unsigned,
  unsigned, unsigned, unsigned)
{
  return nullptr;
}
d3d::ResUpdateBuffer *d3d::allocate_update_buffer_for_tex(BaseTexture *, int, int) { return nullptr; }
void d3d::release_update_buffer(d3d::ResUpdateBuffer *&rub) { rub = nullptr; }
char *d3d::get_update_buffer_addr_for_write(d3d::ResUpdateBuffer *) { return nullptr; }
size_t d3d::get_update_buffer_size(d3d::ResUpdateBuffer *) { return 0; }
size_t d3d::get_update_buffer_pitch(d3d::ResUpdateBuffer *) { return 0; }
size_t d3d::get_update_buffer_slice_pitch(d3d::ResUpdateBuffer *) { return 0; }
bool d3d::update_texture_and_release_update_buffer(d3d::ResUpdateBuffer *&rub)
{
  rub = nullptr;
  return false;
}

d3d::RenderPass *d3d::create_render_pass(const RenderPassDesc &rp_desc)
{
  G_UNUSED(rp_desc);
  return nullptr;
}

void d3d::delete_render_pass(d3d::RenderPass *rp) { G_UNUSED(rp); }

void d3d::begin_render_pass(d3d::RenderPass *rp, const RenderPassArea area, const RenderPassTarget *targets)
{
  G_UNUSED(rp);
  G_UNUSED(area);
  G_UNUSED(targets);
}

void d3d::next_subpass() {}

void d3d::end_render_pass() {}

#if DAGOR_DBGLEVEL > 0
void d3d::allow_render_pass_target_load() {}
#endif

ShaderLibrary d3d::create_shader_library(const ShaderLibraryCreateInfo &) { return InvalidShaderLibrary; }

void d3d::destroy_shader_library(ShaderLibrary) {}

bool d3d::start_capture(const char *, const char *) { return false; }
void d3d::stop_capture(){};

#define CHECK_MAIN_THREAD()
#include "frameStateTM.inc.cpp"
