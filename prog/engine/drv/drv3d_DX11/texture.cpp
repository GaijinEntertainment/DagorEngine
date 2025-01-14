// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <mutex>

#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_resetDevice.h>
#include <generic/dag_sort.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_atomic.h>
#include <image/dag_texPixel.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <generic/dag_smallTab.h>
#include <ioSys/dag_asyncIo.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_autoFuncProf.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <generic/dag_tab.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_watchdog.h>
#include <validation/texture.h>

#include <3d/dag_resourceDump.h>
#include "resource_size_info.h"
#include "driver.h"
#include <supp/dag_comPtr.h>
#include <d3d9types.h>
#if HAS_NVAPI && NVAPI_SLI_SYNC
#include <nvapi.h>
#endif

#if 0
#define VERBOSE_DEBUG debug
#else
#define VERBOSE_DEBUG(...)
#endif

#define BLACK_TEXTURES_WORKAROUND 1

using namespace drv3d_dx11;

namespace drv3d_dx11
{

#define DEBUG_SRGB_TEXTURES 0

extern bool ignore_resource_leaks_on_exit;
extern bool view_resizing_related_logging_enabled;

extern void dump_buffers(Tab<ResourceDumpInfo> &dump_info);

extern void get_shaders_mem_used(String &str);
extern void get_states_mem_used(String &str);

// extern CritSecStorage d3dCritSec;

volatile int is_reset_now = 0;
volatile int lock_cnt_now = 0;
void enter_lock_resources()
{
  if (!dx_device)
    return;

  for (;;)
  {
    interlocked_increment(lock_cnt_now);
    if (!is_reset_now)
      break;
    interlocked_decrement(lock_cnt_now);
    sleep_msec(10);
  }
}

void leave_lock_resources()
{
  if (!dx_device)
    interlocked_decrement(lock_cnt_now);
}

static bool is_texture_statistics_enabled = true;

#define D3D_LOG_MIS_PITCH 0

extern int get_ib_vb_mem_used(String *stat, int &out_sysmem);

// extern void add_vbo_stat(String& );

struct BitMaskFormat
{
  uint32_t bitCount;
  uint32_t alphaMask;
  uint32_t redMask;
  uint32_t greenMask;
  uint32_t blueMask;
  DXGI_FORMAT format;
};

BitMaskFormat bitMaskFormat[] = {
  {32, 0xff000000, 0xff0000, 0xff00, 0xff, DXGI_FORMAT_R8G8B8A8_UNORM},
};


extern char _need_zbdeqr, _limit_rt_size;
#define TRACE_TEX_ALLOC 0

#if TRACE_TEX_ALLOC
static int __tex_used = 0;
#endif

DXGI_FORMAT mode_pf;

typedef ObjectProxyPtr<BaseTex> TextureProxyPtr;

static ObjectPoolWithLock<TextureProxyPtr, 2048> g_textures("textures");
void clear_textures_pool_garbage() { g_textures.clearGarbage(); }

#define MAX_CLEAR_TEXTURE_SIZE 512

struct RtRec
{
  uint16_t w;
  uint16_t h;
  uint16_t used;
  uint16_t levels;

  DXGI_FORMAT fmt;
  ID3D11Texture2D *tex;
};

static Tab<RtRec> rtPool(inimem);

bool init_textures()
{
  if (resetting_device_now)
    return true;
  tql::initTexStubs();
  for (int i = 0; i < tql::texStub.size(); i++)
    ((BaseTex *)tql::texStub[i])->wasUsed = 1;
  return true;
}

void close_textures()
{
  //  if(d3dd) for(i=0;i<MAXSAMPLERS;++i)
  //    d3dd->SetTexture(i,NULL);

  if (resetting_device_now)
  {
    texture_sysmemcopy_usage = 0;
    ITERATE_OVER_OBJECT_POOL(g_textures, i)
      if (g_textures[i].obj != NULL)
      {
        if (tql::isTexStub(g_textures[i].obj))
          continue;
        g_textures[i].obj->releaseTex(false);
        texture_sysmemcopy_usage += data_size(g_textures[i].obj->texCopy);
      }
    ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures)
    ITERATE_OVER_OBJECT_POOL(g_textures, i)
      if (g_textures[i].obj != NULL)
        g_textures[i].obj->releaseTex(false);
    ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures)
    return;
  }
  bool thereAreLeakedTextures = false;

  // custom destroy and leak detection
  int numLeakedTextures = 0;
  ITERATE_OVER_OBJECT_POOL(g_textures, i)
    if (g_textures[i].obj != NULL)
    {
      if (tql::isTexStub(g_textures[i].obj))
        continue;
      numLeakedTextures++;
      LOGERR_CTX("leaked texture '%s', ptr = 0x%p", g_textures[i].obj->getResName(), g_textures[i].obj);
      g_textures[i].destroyObject();
    }
  ITERATE_OVER_OBJECT_POOL_DESTROY(g_textures)
  tql::termTexStubs();
  G_ASSERT_LOG(ignore_resource_leaks_on_exit || numLeakedTextures == 0, "Leaked %d textures on exit, see log for details",
    numLeakedTextures);
}

void gather_textures_to_recreate(FramememResourceSizeInfoCollection &collection)
{
  debug("gather_textures_to_recreate: %d", g_textures.totalUsed());
  ITERATE_OVER_OBJECT_POOL(g_textures, i)
    if (BaseTex *t = g_textures[i].obj)
      collection.push_back({(uint32_t)t->ressize(), (uint32_t)i, true, t->rld != nullptr});
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures)
}

void recreate_texture(uint32_t index)
{
  std::lock_guard lock(g_textures);
  if (!g_textures.isEntryUsed(index))
    return;
  if (BaseTex *t = g_textures[index].obj)
  {
    bool upd_samplers = t->releaseTex(true);
    if (t->rld)
    {
      int addrU = t->addrU, addrV = t->addrV, addrW = t->addrW;
      t->delayedCreate = true;
      t->rld->reloadD3dRes(t);
      t->texaddru(addrU);
      t->texaddrv(addrV);
      t->texaddrw(addrW);
    }
    else if ((t->cflg & TEXCF_SYSTEXCOPY) && data_size(t->texCopy))
    {
      ddsx::Header &hdr = *(ddsx::Header *)t->texCopy.data();
      int8_t sysCopyQualityId = hdr.hqPartLevels;
      unsigned flg = hdr.flags & ~(hdr.FLG_ADDRU_MASK | hdr.FLG_ADDRV_MASK);
      hdr.flags = flg | (t->addrU & hdr.FLG_ADDRU_MASK) | ((t->addrV << 4) & hdr.FLG_ADDRV_MASK);

      InPlaceMemLoadCB mcrd(t->texCopy.data() + sizeof(hdr), data_size(t->texCopy) - (int)sizeof(hdr));
      t->delayedCreate = true;
      VERBOSE_DEBUG("%s <%s> recreate %dx%dx%d (%s)", t->strLabel(), t->getResName(), hdr.w, hdr.h, hdr.depth, "TEXCF_SYSTEXCOPY");
      d3d::load_ddsx_tex_contents(t, hdr, mcrd, sysCopyQualityId);
    }
    else
      t->recreate();
  }
}

void reserve_tex(int max_tex) { g_textures.safeReserve(max_tex); }

void get_stat_tex(bool total, size_t &max_tex) { max_tex = total ? g_textures.size() : g_textures.totalUsed(); }

bool add_texture_to_list(BaseTex *t)
{
  TextureProxyPtr e;
  e.obj = t;
  t->id = g_textures.safeAllocAndSet(e);
  return t->id != BAD_HANDLE;
}

/*
Texture* DebugTexture[MAX_DEBUGTEX_POWER+1] =
{
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static unsigned int DebugTextureColor[MAX_DEBUGTEX_POWER+1] =
{
    0xFF00FF00,
    0xFF80FF00,
    0xFFFFFF00,
    0xFFFF8000,
    0xFFFF0000,
    0xFFFF0011,
    0xFFFF0033,
    0xFFFF0055,
    0xFFFF0077,
    0xFFFF00BB,
    0xFFFF00EE
};
*/

static inline void adjust_tex_size(int &w, int &h)
{
  DriverDesc &d = g_device_desc;
  w = clamp<int>(get_bigger_pow2(w), d.mintexw, d.maxtexw);
  h = clamp<int>(get_bigger_pow2(h), d.mintexh, d.maxtexh);
}

static inline void adjust_cubetex_size(int &s)
{
  DriverDesc &d = g_device_desc;
  s = clamp<int>(get_bigger_pow2(s), d.mincubesize, d.maxcubesize);
}

static inline void adjust_voltex_size(int &w, int &h, int &e)
{
  DriverDesc &d = g_device_desc;
  w = clamp<int>(get_bigger_pow2(w), d.minvolsize, d.maxvolsize);
  h = clamp<int>(get_bigger_pow2(h), d.minvolsize, d.maxvolsize);
  e = clamp<int>(get_bigger_pow2(e), d.minvolsize, d.maxvolsize);
}

static DXGI_FORMAT getFormat(const char *format_name)
{
  static const struct
  {
    const char *name;
    DXGI_FORMAT fmt;
  } fmtTable[] = {{"A16B16G16R16F", DXGI_FORMAT_R16G16B16A16_FLOAT}, {"A8R8G8B8", DXGI_FORMAT_B8G8R8A8_UNORM},
    {"R8G8B8A8", DXGI_FORMAT_R8G8B8A8_UNORM}, {"X8R8G8B8", DXGI_FORMAT_B8G8R8X8_UNORM}, {"R8", DXGI_FORMAT_R8_UNORM},
    {"L8", DXGI_FORMAT_R8_UNORM}, {"A1R5G5B5", DXGI_FORMAT_B5G5R5A1_UNORM}, {"R5G6B5", DXGI_FORMAT_B5G6R5_UNORM},
    {"R32F", DXGI_FORMAT_R32_FLOAT}, {"R32F", DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS}, {"R32UI", DXGI_FORMAT_R32_UINT},
    {"R16F", DXGI_FORMAT_R16_FLOAT}, {"NVD24", DXGI_FORMAT_D24_UNORM_S8_UINT}, {"NVD16", DXGI_FORMAT_D16_UNORM},
    {"D24", DXGI_FORMAT_D24_UNORM_S8_UINT}, {"D16", DXGI_FORMAT_D16_UNORM}, {"INTZ", DXGI_FORMAT_D24_UNORM_S8_UINT},
    {"D32", DXGI_FORMAT_D32_FLOAT}, {"D32_S8", DXGI_FORMAT_D32_FLOAT_S8X24_UINT}, {"G16R16", DXGI_FORMAT_R16G16_UNORM},
    {"G16R16F", DXGI_FORMAT_R16G16_FLOAT}, {"DXT1", DXGI_FORMAT_BC1_UNORM}, {"DXT5", DXGI_FORMAT_BC3_UNORM},
    {"ATI1", DXGI_FORMAT_BC4_UNORM}, {"ATI2", DXGI_FORMAT_BC5_UNORM}, {"BC6", DXGI_FORMAT_BC6H_UF16}, {"BC7", DXGI_FORMAT_BC7_UNORM}};

  for (int i = 0; i < c_countof(fmtTable); i++)
    if (strcmp(format_name, fmtTable[i].name) == 0)
      return fmtTable[i].fmt;
  return DXGI_FORMAT_UNKNOWN;
}

const char *dxgi_format_to_string(DXGI_FORMAT format)
{
  switch (format)
  {
      //    case D3DFMT_R8G8B8        : return "R8G8B8";

    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return "A8R8G8B8_typeless";
    case DXGI_FORMAT_B8G8R8A8_UNORM: return "A8R8G8B8_unorm";
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "A8R8G8B8_SRGB";
    case DXGI_FORMAT_B8G8R8X8_UNORM: return "X8R8G8B8";
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return "R8G8B8A8_typeless";
    case DXGI_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_unorm";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_SRGB";
    case DXGI_FORMAT_B5G6R5_UNORM: return "R5G6B5";
    case DXGI_FORMAT_B5G5R5A1_UNORM:
      return "A1R5G5B5";
      //    case DXGI_FORMAT_B4G4R4A4_UNORM     : return "A4R4G4B4"; //win8
    case DXGI_FORMAT_A8_UNORM: return "A8";
    case DXGI_FORMAT_R11G11B10_FLOAT: return "R11G11B10F";

    case DXGI_FORMAT_R10G10B10A2_UNORM:
      return "A2B10G10R10";
      //    case D3DFMT_X8B8G8R8      : return "X8B8G8R8";
    case DXGI_FORMAT_R16G16_UNORM:
      return "G16R16";
      //    case D3DFMT_A2R10G10B10   : return "A2R10G10B10";
    case DXGI_FORMAT_R16G16B16A16_UNORM: return "A16B16G16R16";
    case DXGI_FORMAT_R16G16B16A16_SNORM: return "A16B16G16R16S";
    case DXGI_FORMAT_R16G16B16A16_UINT: return "A16B16G16R16UI";
    case DXGI_FORMAT_R16G16B16A16_SINT:
      return "A16B16G16R16SI";

      //    case DXGI_FORMAT_A8P8               : return "A8P8"; //win8
      //    case DXGI_FORMAT_P8                 : return "P8";   //win8

    case DXGI_FORMAT_R8_UNORM: return "L8";
    case DXGI_FORMAT_R8G8_UNORM:
      return "R8G8";
      //    case D3DFMT_A4L4          : return "A4L4";
    case DXGI_FORMAT_R8G8_SNORM:
      return "V8U8";
      //    case D3DFMT_L6V5U5        : return "L6V5U5";
      //    case D3DFMT_X8L8V8U8      : return "X8L8V8U8";
      //    case D3DFMT_Q8W8V8U8      : return "Q8W8V8U8";
    case DXGI_FORMAT_R16G16_SNORM:
      return "V16U16";
      //    case D3DFMT_A2W10V10U10   : return "A2W10V10U10";
      //    case D3DFMT_UYVY          : return "UYVY";
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
      return "R8G8_B8G8";
      //    case DXGI_FORMAT_YUY2               : return "YUY2"; //win8
    case DXGI_FORMAT_G8R8_G8B8_UNORM: return "G8R8_G8B8";
    case DXGI_FORMAT_BC1_UNORM_SRGB: return "DXT1_SRGB";
    case DXGI_FORMAT_BC1_UNORM: return "DXT1";
    case DXGI_FORMAT_BC4_UNORM: return "ATI1N";
    case DXGI_FORMAT_BC5_UNORM: return "ATI2N";
    case DXGI_FORMAT_BC6H_UF16: return "BC6H";
    case DXGI_FORMAT_BC7_UNORM: return "BC7";
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return "BC7_SRGB";
      //    case D3DFMT_DXT2          : return "DXT2";
    case DXGI_FORMAT_BC2_UNORM_SRGB: return "DXT3_SRGB";
    case DXGI_FORMAT_BC2_UNORM:
      return "DXT3";
      //    case D3DFMT_DXT4          : return "DXT4";
    case DXGI_FORMAT_BC3_UNORM_SRGB: return "DXT5_SRGB";
    case DXGI_FORMAT_BC3_UNORM:
      return "DXT5";
      //    case D3DFMT_D16_LOCKABLE  : return "D16_LOCKABLE";
    case DXGI_FORMAT_D32_FLOAT: return "D32";
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return "D32_S8";
      //    case D3DFMT_D15S1         : return "D15S1";
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return "D24S8";
      //    case D3DFMT_D24X8         : return "D24X8";
      //    case D3DFMT_D24X4S4       : return "D24X4S4";
    case DXGI_FORMAT_D16_UNORM:
      return "D16";
      //    case D3DFMT_D32F_LOCKABLE : return "D32F_LOCKABLE";
      //    case D3DFMT_D24FS8        : return "D24FS8";
    case DXGI_FORMAT_R16_UNORM:
      return "L16";
      //    case D3DFMT_VERTEXDATA    : return "VERTEXDATA";
      //    case D3DFMT_INDEX16       : return "INDEX16";
      //    case D3DFMT_INDEX32       : return "INDEX32";
      //    case D3DFMT_Q16W16V16U16  : return "Q16W16V16U16";
      //    case D3DFMT_MULTI2_ARGB8  : return "MULTI2_ARGB8";
    case DXGI_FORMAT_R16_FLOAT: return "R16F";
    case DXGI_FORMAT_R16G16_FLOAT: return "G16R16F";
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return "A16B16G16R16F";
    case DXGI_FORMAT_R32_FLOAT: return "R32F";
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS: return "R32F_s8";
    case DXGI_FORMAT_R32_UINT: return "R32UI";
    case DXGI_FORMAT_R32G32_FLOAT: return "G32R32F";
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return "A32B32G32R32F";
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return "A32B32G32R32UI";
      /*    case D3DFMT_CxV8U8        : return "CxV8U8";
          case MAKE4C('D','F','1','6'): return "DF16";
          case MAKE4C('D','F','2','4'): return "DF24";
          case MAKE4C('I','N','T','Z'): return "INTZ";
          case MAKE4C('R','A','W','Z'): return "RAWZ";*/
  }

  return "Unknown";
}

DXGI_FORMAT dxgi_format_from_flags(uint32_t cflg)
{
  if (cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE))
  {
    switch (cflg & TEXFMT_MASK)
    {
      case TEXFMT_DXT1: return DXGI_FORMAT_BC1_UNORM_SRGB;
      case TEXFMT_DXT3: return DXGI_FORMAT_BC2_UNORM_SRGB;
      case TEXFMT_DXT5: return DXGI_FORMAT_BC3_UNORM_SRGB;
      case TEXFMT_BC7: return DXGI_FORMAT_BC7_UNORM_SRGB;
      case TEXFMT_A8R8G8B8: return DXGI_FORMAT_B8G8R8A8_TYPELESS; // -V1037
      case TEXFMT_R8G8B8A8: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
      case TEXFMT_A4R4G4B4: return DXGI_FORMAT_B8G8R8A8_TYPELESS; // FIXME: fake
    }
  }

  switch (cflg & TEXFMT_MASK)
  {
    case TEXFMT_A2R10G10B10: return DXGI_FORMAT_R10G10B10A2_UNORM; // -V1037 N/A fallback D3DFMT_A2R10G10B10;
    case TEXFMT_A2B10G10R10: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case TEXFMT_A16B16G16R16: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case TEXFMT_A16B16G16R16UI: return DXGI_FORMAT_R16G16B16A16_UINT;
    case TEXFMT_A16B16G16R16S: return DXGI_FORMAT_R16G16B16A16_SNORM;
    case TEXFMT_A16B16G16R16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case TEXFMT_A32B32G32R32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case TEXFMT_A32B32G32R32UI: return DXGI_FORMAT_R32G32B32A32_UINT;
    case TEXFMT_R32G32UI: return DXGI_FORMAT_R32G32_UINT;
    case TEXFMT_G16R16: return DXGI_FORMAT_R16G16_UNORM;
    case TEXFMT_G16R16F: return DXGI_FORMAT_R16G16_FLOAT;
    case TEXFMT_G32R32F: return DXGI_FORMAT_R32G32_FLOAT;
    case TEXFMT_R16F: return DXGI_FORMAT_R16_FLOAT;
    case TEXFMT_R32F: return DXGI_FORMAT_R32_FLOAT;
    case TEXFMT_R32UI: return DXGI_FORMAT_R32_UINT;
    case TEXFMT_R32SI: return DXGI_FORMAT_R32_SINT;
    case TEXFMT_DXT1: return DXGI_FORMAT_BC1_UNORM;
    case TEXFMT_DXT3: return DXGI_FORMAT_BC2_UNORM;
    case TEXFMT_DXT5: return DXGI_FORMAT_BC3_UNORM;
    case TEXFMT_ATI1N: return DXGI_FORMAT_BC4_UNORM;
    case TEXFMT_ATI2N: return DXGI_FORMAT_BC5_UNORM;
    case TEXFMT_BC6H: return DXGI_FORMAT_BC6H_UF16;
    case TEXFMT_BC7: return DXGI_FORMAT_BC7_UNORM;
    case TEXFMT_R8UI: return DXGI_FORMAT_R8_UINT;
    case TEXFMT_R16UI: return DXGI_FORMAT_R16_UINT;
    case TEXFMT_A8R8G8B8: return DXGI_FORMAT_B8G8R8A8_UNORM; // -V1037
    case TEXFMT_R8G8B8A8: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TEXFMT_L16: return DXGI_FORMAT_R16_UNORM;
    case TEXFMT_A8: return DXGI_FORMAT_A8_UNORM;
    case TEXFMT_R8: return DXGI_FORMAT_R8_UNORM;
    case TEXFMT_R8G8: return DXGI_FORMAT_R8G8_UNORM;
    case TEXFMT_R8G8S: return DXGI_FORMAT_R8G8_SNORM;
    case TEXFMT_A1R5G5B5: return DXGI_FORMAT_B5G5R5A1_UNORM;
    case TEXFMT_A4R4G4B4: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case TEXFMT_R5G6B5: return DXGI_FORMAT_B5G6R5_UNORM;
    case TEXFMT_R11G11B10F: return DXGI_FORMAT_R11G11B10_FLOAT;
    case TEXFMT_R9G9B9E5: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case TEXFMT_DEPTH24: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case TEXFMT_DEPTH16: return DXGI_FORMAT_D16_UNORM;
    case TEXFMT_DEPTH32_S8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case TEXFMT_DEPTH32: return DXGI_FORMAT_D32_FLOAT;
  }

  G_ASSERTF(0, "unknown texfmt %08x", cflg & TEXFMT_MASK);
  return DXGI_FORMAT_UNKNOWN;
}

DXGI_FORMAT dxgi_format_for_create(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
  };
  return fmt;
}

DXGI_FORMAT dxgi_format_for_res_depth(DXGI_FORMAT fmt, uint32_t cflg, bool read_stencil)
{
  switch (fmt)
  {
    case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_D16_UNORM: return DXGI_FORMAT_R16_UNORM;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return read_stencil ? DXGI_FORMAT_X32_TYPELESS_G8X24_UINT : DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: return read_stencil ? DXGI_FORMAT_X24_TYPELESS_G8_UINT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    default: G_ASSERT(0);
  };
  return fmt;
}

DXGI_FORMAT dxgi_format_for_res(DXGI_FORMAT fmt, uint32_t cflg)
{
  switch (fmt)
  {
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D16_UNORM: G_ASSERT(0); return fmt;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return (cflg & TEXCF_SRGBREAD) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return (cflg & TEXCF_SRGBREAD) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
  };
  return fmt;
}

DXGI_FORMAT dxgi_format_for_res_auto(DXGI_FORMAT fmt, uint32_t cflg)
{
  if (is_depth_format_flg(cflg))
    return dxgi_format_for_res_depth(fmt, cflg, false);
  return dxgi_format_for_res(fmt, cflg);
}

int dxgi_format_to_bpp(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
      //    case D3DFMT_R8G8B8:           bpp=3; break; //n/a
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 32 + 32; // it is be 32+8 on ATI

    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R11G11B10_FLOAT:

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return 32;

    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
      //  case DXGI_FORMAT_B4G4R4A4_UNORM:      //dxgi 1.2
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16_TYPELESS: return 16;

    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT: return 8;

    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT: return 64;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT: return 128;
  }
  return 0;
}

void override_unsupported_bc(uint32_t &cflg)
{
  if ((cflg & TEXFMT_MASK) == TEXFMT_BC7 && g_device_desc.shaderModel < 5.0_sm)
    cflg = (cflg & ~TEXFMT_MASK) | TEXFMT_DXT5;
}

uint32_t calc_surface_size(uint32_t w, uint32_t h, DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM: return ((w + 3) / 4) * ((h + 3) / 4) * 8;

    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB: return ((w + 3) / 4) * ((h + 3) / 4) * 16;
  }
  return w * h * dxgi_format_to_bpp(fmt) / 8;
}

uint32_t calc_row_pitch(uint32_t w, DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM: return ((w + 3) / 4) * 8;

    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB: return ((w + 3) / 4) * 16;
  }
  return w * dxgi_format_to_bpp(fmt) / 8;
}

uint32_t get_min_tex_size(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB: return 4;
  }
  return 1;
}

bool is_float_format(uint32_t cflg)
{
  switch (cflg & TEXFMT_MASK)
  {
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_A32B32G32R32F:
    case TEXFMT_G16R16F:
    case TEXFMT_G32R32F:
    case TEXFMT_R16F:
    case TEXFMT_R32F:
    case TEXFMT_R11G11B10F:
    case TEXFMT_R9G9B9E5: return true;
  }
  return false;
}

ComPtr<ID3D11Texture2D> create_clear_texture(DXGI_FORMAT format, unsigned size)
{
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = size;
  desc.Height = size;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;
  desc.Format = format;
  eastl::vector<uint8_t> clearData(calc_surface_size(size, size, format), 0);
  D3D11_SUBRESOURCE_DATA idata;
  idata.SysMemPitch = calc_row_pitch(size, format);
  idata.pSysMem = clearData.data();
  idata.SysMemSlicePitch = 0;
  ComPtr<ID3D11Texture2D> tex;
  HRESULT hr = dx_device->CreateTexture2D(&desc, &idata, &tex);
  if (FAILED(hr))
  {
    D3D_ERROR("Failed to create clear texture for format %s", dxgi_format_to_string(format));
    return NULL;
  }
  return tex;
}

uint32_t get_texture_res_size(const BaseTex *bt, int skip = 0)
{
  if (bt == NULL)
    return 0;

  uint32_t w = bt->width;
  uint32_t h = bt->height;
  int resType = bt->restype();
  uint32_t d = (resType == RES3D_VOLTEX) ? bt->depth : 1;

  uint32_t total = 0;
  for (uint32_t i = 0; i < bt->mipLevels; i++)
  {
    if (i >= skip)
      total += calc_surface_size(w, h, bt->format) * d;
    if (w > 1)
      w >>= 1;
    if (h > 1)
      h >>= 1;
    if (d > 1)
      d >>= 1;
  };

  return ((resType == RES3D_ARRTEX) ? bt->depth : (resType == RES3D_CUBETEX ? 6 : 1)) * total;
}

static const char *stage_names[STAGE_MAX_EXT] = {"cs", "ps", "vs", "rt/csa"};
bool mark_dirty_texture_in_states(const BaseTexture *tex)
{
  bool found = false;

  // Mark all textures because the current number of used samplers can be less
  // than an index of a dirty texture
  // Resources CS must be locked.

  RenderState &rs = g_render_state;
  G_STATIC_ASSERT(countof(stage_names) == rs.texFetchState.resources.size());
  for (int stage = 0; stage < rs.texFetchState.resources.size(); ++stage)
  {
    for (int i = 0; i < rs.texFetchState.resources[stage].resources.size(); ++i)
    {
      if (rs.texFetchState.resources[stage].resources[i].texture == tex)
      {
        found = true;
        g_render_state.modified = true;
        g_render_state.texFetchState.resources[stage].modifiedMask |= 1 << i;
        debug("mark dirty tex=%p in %d stage [%s]  %s", tex, i, stage_names[stage], ((BaseTex *)tex)->getResName());
      }
    }
  }
  return found;
}

bool remove_texture_from_states(BaseTexture *tex, bool recreate, TextureFetchState &state)
{
  // Remove references to deleted texture from all states to avoid confusion with invalid pointers

  ResAutoLock resLock; // Thredsafe state.resources access.

  bool found = false;
  for (int stageI = 0; stageI < state.resources.size(); ++stageI)
  {
    TextureFetchState::Samplers &stage = state.resources[stageI];
    for (int i = 0; i < stage.resources.size(); ++i)
    {
      if (stage.resources[i].texture == tex)
      {
        found = true;
        stage.resources[i].setTex(NULL, stageI, false);
        if (recreate)
        {
          stage.resources[i].setTex(tex, stageI, true);
          g_render_state.modified = true;
          stage.modifiedMask |= 1 << i;
          debug("recreate tex=%p in %s[%d]  %s", tex, stage_names[stageI], i, ((BaseTex *)tex)->getResName());
        }
      }
    }
  }
  return found;
}

static void reset_depth()
{
  RenderState &rs = g_render_state;
  rs.modified = rs.rtModified = true;
  rs.viewModified = VIEWMOD_FULL;
  rs.nextRtState.removeDepth();
}

bool remove_texture_from_states(BaseTexture *tex, bool recreate = false)
{
  bool found = remove_texture_from_states(tex, recreate, g_render_state.texFetchState);
  RenderState &rs = g_render_state;

  if (rs.nextRtState.isDepthUsed() && rs.nextRtState.depth.tex == tex)
  {
    found = true;
    reset_depth();
    G_ASSERT(!recreate);
  }

  for (int i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (rs.nextRtState.isColorUsed(i) && rs.nextRtState.color[i].tex == tex)
    {
      found = true;
      d3d::set_render_target(i, NULL, 0);
      reset_depth();
      G_ASSERT(!recreate);
    }

    if (rs.currRtState.isColorUsed(i) && rs.currRtState.color[i].tex == tex)
    {
      found = true;
      rs.currRtState.color[i].tex = NULL;
      G_ASSERT(!recreate);
    }
  }

  return found;
}

template <class T>
inline void set_res_view_mip(T &d, uint32_t mip_level, int32_t mip_count)
{
  d.MostDetailedMip = mip_level;
  d.MipLevels = mip_count;
}
static void fixup_tex_params(int w, int h, int32_t &flg, int &levels)
{
  if (is_depth_format_flg(flg))
    flg |= TEXCF_RTARGET;
  const bool rt = flg & TEXCF_RTARGET;

  if (rt)
  {
    if (flg & TEXCF_SRGBWRITE)
    {
      if ((flg & TEXFMT_MASK) != TEXFMT_DEFAULT && (flg & TEXFMT_MASK) != TEXFMT_R8G8B8A8)
        flg |= TEXCF_SRGBREAD; // only supports srgbwrite from srgb surfaces!
    }
  }

  if (rt && (flg & TEXFMT_MASK) == TEXFMT_R5G6B5)
    flg = (flg & ~TEXFMT_MASK) | TEXFMT_A8R8G8B8;
  if ((flg & TEXFMT_MASK) == TEXFMT_DEFAULT)
    flg = (flg & ~TEXFMT_MASK) | TEXFMT_A8R8G8B8;

  if (levels == 0)
    levels = auto_mip_levels_count(w, h, rt ? 1 : 4);

  if (flg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    flg &= ~TEXCF_SYSTEXCOPY;
}

static const int DX11_MIP_LEVELS_LIMIT = 16;

void set_tex_params(BaseTex *tex, int w, int h, int d, uint32_t flg, int levels, const char *stat_name)
{
  ResAutoLock resLock; // Writing to a bitfield isn't atomic, must protect getResView.

  DXGI_FORMAT fmt = dxgi_format_from_flags(flg);
  G_ASSERT(fmt != DXGI_FORMAT_UNKNOWN);
  G_ASSERTF(levels > 0, "(%s).levels=%d", tex->getResName(), levels);

  tex->format = fmt;
  G_ASSERTF(levels < DX11_MIP_LEVELS_LIMIT && levels > 0, "DX11: %d is invalid value for mipLevels", levels);
  tex->mipLevels = levels;
  tex->width = w;
  tex->height = h;
  tex->depth = d ? d : 1;
  tex->maxMipLevel = 0;
  tex->minMipLevel = levels - 1;

  if (flg & TEXCF_CLEAR_ON_CREATE)
    tex->needs_clear = true;

  tex->setTexName(stat_name);
}

bool create_tex2d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t levels, bool cube,
  D3D11_SUBRESOURCE_DATA *initial_data, int array_size = 1)
{
  uint32_t &flg = bt_in->cflg;
  G_ASSERT(!((flg & TEXCF_SAMPLECOUNT_MASK) && initial_data != NULL));
  G_ASSERT(!((flg & TEXCF_SAMPLECOUNT_MASK) && (flg & TEXCF_CLEAR_ON_CREATE)));
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & (TEXCF_DYNAMIC | TEXCF_RTARGET))));

  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = w;
  desc.Height = h;
  G_ASSERTF(levels < DX11_MIP_LEVELS_LIMIT && levels > 0, "DX11: %d is invalid value for realMipLevels", levels);
  desc.MipLevels = levels;
  tex.realMipLevels = levels;
  desc.ArraySize = (cube ? 6 : 1) * array_size;
  desc.Format = dxgi_format_for_create(dxgi_format_from_flags(flg));
  desc.SampleDesc.Quality = 0;
  desc.SampleDesc.Count = get_sample_count(flg);
  G_ASSERT(desc.Format != DXGI_FORMAT_UNKNOWN);

  D3D11_USAGE usage = D3D11_USAGE_DEFAULT; // GPU R/W

  if ((flg & (TEXCF_DYNAMIC | TEXCF_READABLE)) == TEXCF_DYNAMIC)
  {
    usage = D3D11_USAGE_DYNAMIC; // GPU RO, CPU WO
  }

  if ((flg & TEXCF_LOADONCE) && initial_data)
    usage = D3D11_USAGE_IMMUTABLE; // GPU RO

  desc.Usage = usage;

  const bool isDepth = is_depth_format_flg(flg);
  const bool isRT = flg & TEXCF_RTARGET;
  // The depth texture is intended to be multisampled only as a pair to a multisampled color texture.
  // There is no need to sample from depth in that case. DX10.0 HW does not support it anyway.
  desc.BindFlags =
    (isDepth && (flg & TEXCF_SAMPLECOUNT_MASK) && !g_device_desc.caps.hasReadMultisampledDepth) ? 0 : D3D11_BIND_SHADER_RESOURCE;
  desc.BindFlags |= (isRT ? (isDepth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET) : 0);
  desc.BindFlags |= (flg & TEXCF_UNORDERED) ? D3D11_BIND_UNORDERED_ACCESS : 0;

  G_ASSERT(!(isDepth && (flg & TEXCF_READABLE)));

  uint32_t access = 0;
  if (usage == D3D11_USAGE_DYNAMIC)
    access |= D3D11_CPU_ACCESS_WRITE;

  desc.CPUAccessFlags = (D3D11_CPU_ACCESS_FLAG)access;

  if (usage == D3D11_USAGE_IMMUTABLE)
    desc.CPUAccessFlags = 0;

  if (flg & TEXCF_SYSMEM)
  {
    usage = desc.Usage = D3D11_USAGE_STAGING;
    access = desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = 0;
    G_ASSERT(!isRT && !isDepth);
  }

  desc.MiscFlags = (cube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0) |
                   (((flg & TEXCF_RTARGET) && (flg & (TEXCF_GENERATEMIPS))) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);

  tex.texUsage = usage;
  tex.texAccess = (D3D11_CPU_ACCESS_FLAG)access;

  if ((flg & TEXCF_SAMPLECOUNT_MASK))
  {
    uint32_t numQualities;
    if (FAILED(dx_device->CheckMultisampleQualityLevels(desc.Format, desc.SampleDesc.Count, &numQualities)) || numQualities == 0)
      return false;
  }

  {
    HRESULT hr = dx_device->CreateTexture2D(&desc, initial_data, &tex.tex2D);
    if (FAILED(hr))
    {
      if (hr == E_OUTOFMEMORY && (flg & TEXCF_SYSMEM) && !*bt_in->getTexName())
        return false;
      debug("DX11: CreateTexture2D failed "
            "(hr=0x%08X, ITex=%p, cflg=0x%08X, lockFlags=0x%08X, delayedCreate=%d, name='%s', %dx%d,L%d[%d] "
            "GetDeviceRemovedReason=0x%08X)",
        hr, tex.tex2D, bt_in->cflg, bt_in->lockFlags, (int)bt_in->delayedCreate, bt_in->getTexName(), w, h, levels, desc.ArraySize,
        dx_device->GetDeviceRemovedReason());
      if (bt_in->getTID() != BAD_TEXTUREID)
        tql::dump_tex_state(bt_in->getTID());
      if (hr != E_OUTOFMEMORY || bt_in->getTID() == BAD_TEXTUREID)
        D3D_ERROR("CreateTexture2D D3D_ERROR %x, %s", hr, dx11_error(hr));
      if (!device_should_reset(hr, "CreateTexture2D"))
      {
        if (bt_in->getTID() == BAD_TEXTUREID)
          DXFATAL(hr, "CreateTexture2D(tex2D)");
        return false;
      }
    }

#if NVAPI_SLI_SYNC
    if ((flg & TEXCF_RTARGET) && is_nvapi_initialized && has_sli())
    {
      NVDX_ObjectHandle handle;
      NvAPI_Status res = NvAPI_D3D_GetObjectHandleForResource(dx_device, tex.tex2D, &handle);
      if (res == NVAPI_OK)
      {
        NvU32 one = 1;
        res = NvAPI_D3D_SetResourceHint(dx_device, handle, NVAPI_D3D_SRH_CATEGORY_SLI,
          NVAPI_D3D_SRH_SLI_APP_CONTROLLED_INTERFRAME_CONTENT_SYNC, &one);
        if (res != NVAPI_OK)
          debug("NvAPI_D3D_SetResourceHint(\"%s\") = %d", bt_in->getTexName(), res);
      }
    }
#endif
  }

  if ((flg & (TEXCF_READABLE | TEXCF_SYSMEM)) == TEXCF_READABLE)
  {
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = 0;

    {
      HRESULT hr = dx_device->CreateTexture2D(&desc, initial_data, &tex.stagingTex2D);
      RETRY_CREATE_STAGING_TEX(hr, bt_in->ressize(), dx_device->CreateTexture2D(&desc, initial_data, &tex.stagingTex2D));
      if (FAILED(hr))
      {
        if (tex.tex2D)
          tex.tex2D->Release();
        tex.tex2D = NULL;

        D3D_ERROR("CreateTexture2D D3D_ERROR %s", dx11_error(hr));
        if (!device_should_reset(hr, "CreateTexture2D(stagingTex2D)"))
        {
          DXFATAL(hr, "CreateTexture2D(stagingTex2D)");
          return false;
        }
      }
    }
  }
  tex.setPrivateData(bt_in->getResName());
  tex.memSize = bt_in->ressize();
  TEXQL_ON_ALLOC(bt_in);

  return true;
}

bool create_tex3d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t d, uint32_t flg, uint32_t levels,
  D3D11_SUBRESOURCE_DATA *initial_data)
{
  G_ASSERT((flg & TEXCF_SAMPLECOUNT_MASK) == 0);
  G_ASSERT(!((flg & TEXCF_LOADONCE) && (flg & TEXCF_DYNAMIC)));

  D3D11_TEXTURE3D_DESC desc = {0};
  desc.Width = w;
  desc.Height = h;
  desc.Depth = d;
  G_ASSERTF(levels < DX11_MIP_LEVELS_LIMIT && levels > 0, "DX11: %d is invalid value for realMipLevels", levels);
  desc.MipLevels = levels;
  tex.realMipLevels = levels;
  desc.Format = dxgi_format_for_create(dxgi_format_from_flags(flg));

  D3D11_USAGE usage = D3D11_USAGE_DEFAULT; // GPU R/W
  if (flg & TEXCF_DYNAMIC)
    usage = D3D11_USAGE_DYNAMIC; // GPU RO, CPU WO
  if ((flg & TEXCF_LOADONCE) && initial_data)
    usage = D3D11_USAGE_IMMUTABLE; // GPU RO

  //_STAGING
  desc.Usage = usage;

  const bool isDepth = is_depth_format_flg(flg);
  const bool isRT = flg & TEXCF_RTARGET;

  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (isRT ? (isDepth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET) : 0);
  desc.BindFlags |= (flg & TEXCF_UNORDERED) ? D3D11_BIND_UNORDERED_ACCESS : 0;
  desc.CPUAccessFlags = (flg & TEXCF_DYNAMIC) ? D3D11_CPU_ACCESS_WRITE : 0;

  desc.MiscFlags = ((flg & TEXCF_RTARGET) && (flg & (TEXCF_GENERATEMIPS))) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

  tex.texUsage = usage;
  tex.texAccess = (D3D11_CPU_ACCESS_FLAG)desc.CPUAccessFlags;

  {
    HRESULT hr = dx_device->CreateTexture3D(&desc, initial_data, &tex.tex3D);
    // HRESULT hr = dx_device->CreateTexture3D(&desc, initial_data, &tex.tex3D);
    if (FAILED(hr))
    {
      if (hr == E_OUTOFMEMORY && (flg & TEXCF_SYSMEM) && !*bt_in->getTexName())
        return false;
      if (hr != E_OUTOFMEMORY || bt_in->getTID() == BAD_TEXTUREID)
        D3D_ERROR("CreateTexture3D D3D_ERROR %s", dx11_error(hr));
      debug(
        "DX11: CreateTexture3D failed "
        "(hr=0x%08X, ITex=%p, cflg=0x%08X, lockFlags=0x%08X, delayedCreate=%d, name='%s', %dx%dx%d,L%d GetDeviceRemovedReason=0x%08X)",
        hr, tex.tex3D, bt_in->cflg, bt_in->lockFlags, (int)bt_in->delayedCreate, bt_in->getTexName(), w, h, d, levels,
        dx_device->GetDeviceRemovedReason());
      if (!device_should_reset(hr, "CreateTexture3D"))
      {
        if (bt_in->getTID() == BAD_TEXTUREID)
          DXFATAL(hr, "CreateTexture3D(tex3D)");
        return false;
      }
    }
  }

  tex.setPrivateData(bt_in->getResName());
  tex.memSize = bt_in->ressize();
  TEXQL_ON_ALLOC(bt_in);

  return true;
}

void clear_texture(BaseTex *tex)
{
  unsigned mip_levels = tex->mipLevels;
  unsigned d = tex->depth;
  if ((tex->cflg & TEXCF_UNORDERED) && (d == 1 || !tex->cube_array))
  {
    bool as_float = is_float_format(tex->cflg);
    float clearValF[4] = {0.0f};
    uint32_t clearValI[4] = {0};
    for (int level = 0; level < mip_levels; level++)
      if (as_float)
        d3d::clear_rwtexf(tex, clearValF, 0, level);
      else
        d3d::clear_rwtexi(tex, clearValI, 0, level);
    return;
  }
  bool is3d = tex->type == RES3D_VOLTEX;
  if (tex->cflg & TEXCF_RTARGET)
  {
    for (int level = 0; level < mip_levels; level++)
    {
      for (int layer = 0; layer < (is3d ? max(1u, d >> level) : d * (tex->cube_array ? 6 : 1)); layer++)
      {
        ID3D11View *view = tex->getRtView(layer, level, 1, false);
        if (is_depth_format_flg(tex->cflg))
        {
          ContextAutoLock contextLock;
          dx_context->ClearDepthStencilView((ID3D11DepthStencilView *)view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0, 0);
        }
        else
        {
          ContextAutoLock contextLock;
          float color[4] = {0, 0, 0, 0};
          dx_context->ClearRenderTargetView((ID3D11RenderTargetView *)view, color);
        }
      }
    }
    return;
  }
  unsigned w = tex->width;
  unsigned h = tex->height;
  unsigned clear_size = min(max(w, h), (unsigned)MAX_CLEAR_TEXTURE_SIZE);
  ComPtr<ID3D11Texture2D> clear_src = create_clear_texture(tex->format, clear_size);
  if (clear_src)
  {
    // can't copy block of size less than 4 for BC-compressed textures
    unsigned min_box_size = get_min_tex_size(tex->format);
    D3D11_BOX box;
    box.left = 0;
    box.top = 0;
    box.front = 0;
    box.back = 1;
    ContextAutoLock contextLock;
    disable_conditional_render_unsafe();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "DX11: clear_texture uses CopySubresourceRegion inside a generic render pass");
    if (tex->type == RES3D_CUBETEX || tex->cube_array)
      d *= 6;
    for (int level = 0; level < mip_levels; level++)
    {
      for (int layer = 0; layer < (is3d ? 1 : d); layer++)
      {
        unsigned width = max(1u, w >> level);
        unsigned height = max(1u, h >> level);
        unsigned depth = max(1u, d >> level);
        unsigned subresource = layer * mip_levels + level;
        for (int x = 0; x < (width + clear_size - 1) / clear_size; x++)
          for (int y = 0; y < (height + clear_size - 1) / clear_size; y++)
          {
            box.right = max(min_box_size, min((unsigned)clear_size, width - x * clear_size));
            box.bottom = max(min_box_size, min((unsigned)clear_size, height - y * clear_size));
            if (is3d)
              for (int z = 0; z < depth; z++)
                dx_context->CopySubresourceRegion(tex->tex.texRes, level, x * clear_size, y * clear_size, z, clear_src.Get(), 0, &box);
            else
              dx_context->CopySubresourceRegion(tex->tex.texRes, subresource, x * clear_size, y * clear_size, 0, clear_src.Get(), 0,
                &box);
          }
      }
    }
  }
}

/*

unsigned int get_texture_size(IDirect3DBaseTexture9 *texture)
{
  return 0;
  G_ASSERT(texture);

  D3DSURFACE_DESC surfaceDesc;
  D3DVOLUME_DESC volDesc;

  if (texture->GetType() == D3DRTYPE_TEXTURE)
    ((IDirect3DTexture9 *)texture)->GetLevelDesc(0, &surfaceDesc);
  else if (texture->GetType() == D3DRTYPE_CUBETEXTURE)
    ((IDirect3DCubeTexture9 *)texture)->GetLevelDesc(0, &surfaceDesc);
  else if (texture->GetType() == D3DRTYPE_VOLUMETEXTURE)
  {
    ((IDirect3DVolumeTexture9 *)texture)->GetLevelDesc(0, &volDesc);
    surfaceDesc.Format = volDesc.Format;
    surfaceDesc.Type = volDesc.Type;
    surfaceDesc.Usage = volDesc.Usage;
    surfaceDesc.Pool = volDesc.Pool;
    surfaceDesc.MultiSampleType = D3DMULTISAMPLE_NONE;
    surfaceDesc.MultiSampleQuality = 0;
    surfaceDesc.Width = volDesc.Width;
    surfaceDesc.Height = volDesc.Height;
  }
  else
    G_ASSERT(0);

  unsigned int surfaceSize = calc_surface_size(surfaceDesc.Width,  surfaceDesc.Height);
   if (surfaceSize == 0)
     DAG_FATAL("unsupported fmt: %08X", surfaceDesc.Format);

  unsigned int size = 0;
  for (unsigned int mipNo = 0; mipNo < texture->GetLevelCount(); mipNo++)
    size += surfaceSize / (1 << (2 * mipNo));

  if (texture->GetType() == D3DRTYPE_CUBETEXTURE)
    size *= 6;

  if (texture->GetType() == D3DRTYPE_VOLUMETEXTURE)
    size *= volDesc.Depth;

  return size;
}
*/

void BaseTex::destroy()
{
#if DAGOR_DBGLEVEL > 1
  if (!wasUsed && view_resizing_related_logging_enabled)
    logwarn("texture %p, of size %dx%dx%d total=%dbytes, name=%s was destroyed but was never used in rendering", this, width, height,
      depth, tex.memSize, getResName());
#elif DAGOR_DBGLEVEL > 0
  if (!wasUsed && view_resizing_related_logging_enabled)
    debug("texture %p, of size %dx%dx%d total=%dbytes, name=%s was destroyed but was never used in rendering", this, width, height,
      depth, tex.memSize, getResName());
#endif

  g_textures.lock();
  if (g_textures.isIndexValid(id))
    g_textures.safeReleaseEntry(id);
  g_textures.unlock();
  id = BAD_HANDLE;
  destroyObject();
}

#define LOCK_IN_THREAD_TIMEOUT_MS (500)

HRESULT map_without_context_blocking(ID3D11Resource *resource, UINT subresource, D3D11_MAP maptype, bool nosyslock,
  D3D11_MAPPED_SUBRESOURCE *mapped)
{
  HRESULT hres;
  if (nosyslock || is_main_thread() || get_current_thread_id() == gpuThreadId)
  {
    ContextAutoLock contextLock;
    hres = dx_context->Map(resource, subresource, maptype, nosyslock ? D3D11_MAP_FLAG_DO_NOT_WAIT : 0, mapped);
  }
  else
  {
    int lockStarted = get_time_msec();
    for (;;)
    {
      bool timedout = get_time_msec() - lockStarted >= LOCK_IN_THREAD_TIMEOUT_MS;
      int startIteration = get_time_msec();
      {
        ContextAutoLock contextLock;
        hres = dx_context->Map(resource, subresource, maptype, timedout ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT, mapped);
      }
      if (get_time_msec() - startIteration > 50)
        logwarn("Map (iteration): %dms", get_time_msec() - startIteration);

      if (hres != DXGI_ERROR_WAS_STILL_DRAWING || timedout)
        break;

      sleep_msec(1);
    }
  }
  return hres;
}
} // namespace drv3d_dx11

unsigned d3d::get_texformat_usage(int cflg, int restype)
{
  int format = cflg & TEXFMT_MASK;
  if (format == TEXFMT_A4R4G4B4)
    return 0;
  if (format == TEXFMT_ETC2_RGBA || format == TEXFMT_ETC2_RG)
    return 0;

  UINT flags = 0;
  DXGI_FORMAT dxgiFormat = dxgi_format_from_flags(format);
  if (dx_device->CheckFormatSupport(dxgiFormat, &flags) != S_OK)
    return 0;
  if ((restype == RES3D_TEX) && !(flags & D3D11_FORMAT_SUPPORT_TEXTURE2D))
    return 0;
  if ((restype == RES3D_VOLTEX) && !(flags & D3D11_FORMAT_SUPPORT_TEXTURE3D))
    return 0;
  if ((restype == RES3D_CUBETEX) && !(flags & D3D11_FORMAT_SUPPORT_TEXTURECUBE))
    return 0;

  bool supportSrgb = false;
  switch (format)
  {
    case TEXFMT_DXT1:
    case TEXFMT_DXT3:
    case TEXFMT_DXT5:
    case TEXFMT_A8R8G8B8:
    case TEXFMT_R8G8B8A8: supportSrgb = true; break;
  };
  uint32_t ret =
    (flags & D3D11_FORMAT_SUPPORT_TEXTURE2D) ? USAGE_TEXTURE | USAGE_VERTEXTEXTURE | (supportSrgb ? USAGE_SRGBREAD : 0) : 0;

  if (flags & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
    ret |= USAGE_FILTER;
  if (flags & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON)
    ret |= USAGE_SAMPLECMP;

  if (flags & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)
    ret |= USAGE_AUTOGENMIPS;
  if (flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET)
    ret |= USAGE_RTARGET | (supportSrgb ? USAGE_SRGBWRITE : 0);
  if (flags & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
    ret |= USAGE_DEPTH;
  if (flags & D3D11_FORMAT_SUPPORT_BLENDABLE)
    ret |= USAGE_BLEND;
  if (flags & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)
  {
    ret |= USAGE_UNORDERED;
    D3D11_FEATURE_DATA_FORMAT_SUPPORT2 feature2 = {dxgiFormat, 0};
    if (S_OK == dx_device->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT2, &feature2, sizeof(feature2)))
    {
      if (feature2.OutFormatSupport2 & D3D11_FORMAT_SUPPORT2_UAV_TYPED_LOAD)
      {
        ret |= USAGE_UNORDERED_LOAD;
      }
    }
  }

  return ret | USAGE_PIXREADWRITE;
  // return (format == TEXFMT_A4R4G4B4) ? 0 : (unsigned)-1;
}

namespace d3d
{
static bool check_texformat(int cflg, int resType)
{
  unsigned flags = d3d::get_texformat_usage(cflg, resType);
  unsigned mask = USAGE_TEXTURE;
  if (is_depth_format_flg(cflg))
    mask |= USAGE_DEPTH;
  else
  {
    if (cflg & TEXCF_RTARGET)
      mask |= USAGE_RTARGET;
    if (cflg & TEXCF_UNORDERED)
      mask |= USAGE_UNORDERED;
  }
  return (flags & mask) == mask ? true : false;
}
} // namespace d3d

bool d3d::check_texformat(int cflg) { return check_texformat(cflg, RES3D_TEX); }

int d3d::get_max_sample_count(int cflg)
{
  DXGI_FORMAT dxgiFormat = dxgi_format_from_flags(cflg);
  for (int samples = get_sample_count(TEXCF_SAMPLECOUNT_MAX); samples; samples >>= 1)
  {
    uint32_t numQualityLevels = 0;
    if (SUCCEEDED(dx_device->CheckMultisampleQualityLevels(dxgiFormat, samples, &numQualityLevels)) && numQualityLevels > 0)
      return samples;
  }

  return 1;
}

bool d3d::issame_texformat(int f1, int f2) { return dxgi_format_from_flags(f1) == dxgi_format_from_flags(f2); }

bool d3d::check_cubetexformat(int f) { return check_texformat(f, RES3D_CUBETEX); }

bool d3d::check_voltexformat(int f) { return check_texformat(f, RES3D_VOLTEX); }


Texture *drv3d_dx11::create_d3d_tex(ID3D11Texture2D *tex_res, const char *name, int flg)
{
  D3D11_TEXTURE2D_DESC desc;
  tex_res->GetDesc(&desc);

  BaseTex *tex = BaseTex::create_tex(flg, desc.ArraySize > 1 ? RES3D_ARRTEX : RES3D_TEX);
  tex->tex.tex2D = tex_res;
  G_ASSERTF(desc.MipLevels < DX11_MIP_LEVELS_LIMIT && desc.MipLevels > 0, "DX11: %d is invalid value for mipLevels", desc.MipLevels);
  tex->mipLevels = desc.MipLevels;
  tex->minMipLevel = tex->mipLevels - 1;
  tex->tex.realMipLevels = desc.MipLevels;
  tex->width = desc.Width;
  tex->height = desc.Height;
  tex->depth = desc.ArraySize;
  if (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
    tex->format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
  else if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
    tex->format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
  else
    tex->format = desc.Format;
  tex->wasUsed = 1;
  tex->setTexName(name);
  TEXQL_ON_ALLOC(tex);
  return tex;
}

Texture *drv3d_dx11::create_backbuffer_tex(int id, IDXGI_SWAP_CHAIN *swap_chain)
{
  G_ASSERT(swap_chain);

  ID3D11Texture2D *texRes;
  DXFATAL(swap_chain->GetBuffer(id, __uuidof(ID3D11Texture2D), (void **)&texRes), "SCC");

  return create_d3d_tex(texRes, "backbuffer", TEXFMT_DEFAULT | TEXCF_RTARGET);
}

BaseTexture *d3d::create_tex(TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  check_texture_creation_args(w, h, flg, stat_name);
  G_ASSERT_RETURN(d3d::check_texformat(flg), nullptr);

  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    D3D_ERROR("create_tex: can not create dynamic render target");
    return NULL;
  }
  if (img)
  {
    w = img->w;
    h = img->h;
  }
  int imgW = w;
  int imgH = h;

  DriverDesc &dd = g_device_desc;
  // TODO: remove clamp entirely and replace logerr with assert once all projects satisfy this requirement
  if ((w < dd.mintexw) || (w > dd.maxtexw) || (h < dd.mintexh) || (h > dd.maxtexh))
    logerr("create_tex: texture <%s> size %d x %d is out of bounds [%d, %d] x [%d, %d]", stat_name, w, h, dd.mintexw, dd.maxtexw,
      dd.mintexh, dd.maxtexh);
  w = clamp<int>(w, dd.mintexw, dd.maxtexw);
  h = clamp<int>(h, dd.mintexh, dd.maxtexh);

  fixup_tex_params(w, h, flg, levels);

  if (img != NULL)
  {
    levels = 1;

    if (w == 0 && h == 0)
    {
      w = img->w;
      h = img->h;
    }

    if (((w ^ img->w) | (h ^ img->h)) != 0)
    {
      D3D_ERROR("create_tex: image size differs from texture size (%dx%d != %dx%d)", img->w, img->h, w, h);
      img = NULL; // abort copying
    }

    if (dxgi_format_to_bpp(dxgi_format_from_flags(flg)) != 32)
      img = NULL;
  }

  // TODO: check for preallocated RT (with requested, not adjusted tex dimensions)

  BaseTex *tex = BaseTex::create_tex(flg, RES3D_TEX);

  set_tex_params(tex, w, h, 1, flg, levels, stat_name);
  if (tex->cflg & TEXCF_SYSTEXCOPY)
  {
    if (img)
    {
      int memSz = w * h * 4;
      clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + memSz);

      ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
      memset(&hdr, 0, sizeof(hdr));
      hdr.label = _MAKE4C('DDSx');
      hdr.d3dFormat = D3DFMT_A8R8G8B8;
      hdr.flags |= ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0) ? hdr.FLG_GAMMA_EQ_1 : 0;
      hdr.w = w;
      hdr.h = h;
      hdr.levels = 1;
      hdr.bitsPerPixel = 32;
      hdr.memSz = memSz;
      /*sysCopyQualityId*/ hdr.hqPartLevels = 0;

      memcpy(tex->texCopy.data() + sizeof(hdr), img + 1, memSz);
      VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
    }
    else if (tex->cflg & TEXCF_LOADONCE)
    {
      int memSz = tex->ressize();
      clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + memSz);
      mem_set_0(tex->texCopy);

      ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
      hdr.label = _MAKE4C('DDSx');
      hdr.d3dFormat = texfmt_to_d3dformat(tex->cflg & TEXFMT_MASK);
      hdr.flags |= ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0) ? hdr.FLG_GAMMA_EQ_1 : 0;
      hdr.w = w;
      hdr.h = h;
      hdr.levels = levels;
      hdr.bitsPerPixel = dxgi_format_to_bpp(tex->format);
      if (hdr.d3dFormat == D3DFMT_DXT1)
        hdr.dxtShift = 3;
      else if (hdr.d3dFormat == D3DFMT_DXT3 || hdr.d3dFormat == D3DFMT_DXT5)
        hdr.dxtShift = 4;
      if (hdr.dxtShift)
        hdr.bitsPerPixel = 0;
      hdr.memSz = memSz;
      /*sysCopyQualityId*/ hdr.hqPartLevels = 0;

      VERBOSE_DEBUG("%s %dx%d reserved DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
    }
    else
      tex->cflg &= ~TEXCF_SYSTEXCOPY;
  }

  //  tex->basetex=t->tex;

  // only 1 mip
  D3D11_SUBRESOURCE_DATA idata[1];

  if (img != NULL)
  {
    idata[0].pSysMem = (const void *)(img + 1);
    idata[0].SysMemPitch = w * 4;
    idata[0].SysMemSlicePitch = 0;
  }

  bool res = create_tex2d(tex->tex, tex, w, h, levels, false, img ? idata : NULL);
  if (!res)
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  tex->tex.memSize = tex->ressize();

  if (!add_texture_to_list(tex))
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  return tex;
}

BaseTexture *d3d::create_cubetex(int size, int flg, int levels, const char *stat_name)
{
  G_ASSERT_RETURN(d3d::check_cubetexformat(flg), nullptr);

  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    D3D_ERROR("create_cubtex: can not create dynamic render target");
    return NULL;
  }

  DriverDesc &dd = g_device_desc;
  // TODO: remove clamp entirely and replace logerr with assert once all projects satisfy this requirement
  if ((size < dd.mincubesize) || (size > dd.maxcubesize))
    logerr("create_cubetex: texture <%s> size %d is out of bounds [%d, %d]", stat_name, size, dd.mincubesize, dd.maxcubesize);
  size = get_bigger_pow2(clamp<int>(size, dd.mincubesize, dd.maxcubesize));

  fixup_tex_params(size, size, flg, levels);

  BaseTex *tex = BaseTex::create_tex(flg, RES3D_CUBETEX);
  set_tex_params(tex, size, size, 1, flg, levels, stat_name);

  bool res = create_tex2d(tex->tex, tex, size, size, levels, true, NULL);
  if (!res)
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  tex->tex.memSize = tex->ressize();

  if (!add_texture_to_list(tex))
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  return tex;
}


BaseTexture *d3d::create_voltex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  G_ASSERT_RETURN(d3d::check_voltexformat(flg), nullptr);

  if ((flg & (TEXCF_RTARGET | TEXCF_DYNAMIC)) == (TEXCF_RTARGET | TEXCF_DYNAMIC))
  {
    D3D_ERROR("create_voltex: can not create dynamic render target");
    return NULL;
  }

  fixup_tex_params(w, h, flg, levels);

  BaseTex *tex = BaseTex::create_tex(flg, RES3D_VOLTEX);
  set_tex_params(tex, w, h, d, flg, levels, stat_name);

  bool res = create_tex3d(tex->tex, tex, w, h, d, flg, levels, NULL);
  if (!res)
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  tex->tex.memSize = tex->ressize();
  if (tex->cflg & TEXCF_SYSTEXCOPY)
  {
    clear_and_resize(tex->texCopy, sizeof(ddsx::Header) + tex->tex.memSize);
    mem_set_0(tex->texCopy);

    ddsx::Header &hdr = *(ddsx::Header *)tex->texCopy.data();
    hdr.label = _MAKE4C('DDSx');
    hdr.d3dFormat = texfmt_to_d3dformat(tex->cflg & TEXFMT_MASK);
    hdr.w = w;
    hdr.h = h;
    hdr.depth = d;
    hdr.levels = levels;
    hdr.bitsPerPixel = dxgi_format_to_bpp(tex->format);
    if (hdr.d3dFormat == D3DFMT_DXT1)
      hdr.dxtShift = 3;
    else if (hdr.d3dFormat == D3DFMT_DXT3 || hdr.d3dFormat == D3DFMT_DXT5)
      hdr.dxtShift = 4;
    if (hdr.dxtShift)
      hdr.bitsPerPixel = 0;
    hdr.flags = hdr.FLG_VOLTEX;
    if ((tex->cflg & (TEXCF_SRGBREAD | TEXCF_SRGBWRITE)) == 0)
      hdr.flags |= hdr.FLG_GAMMA_EQ_1;
    hdr.memSz = tex->tex.memSize;
    /*sysCopyQualityId*/ hdr.hqPartLevels = 0;
    VERBOSE_DEBUG("%s %dx%d reserved DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(tex->texCopy));
  }

  if (!add_texture_to_list(tex))
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  return tex;
}

BaseTexture *d3d::create_array_tex(int w, int h, int d, int flg, int levels, const char *stat_name)
{
  G_ASSERT_RETURN(d3d::check_texformat(flg), nullptr);

  fixup_tex_params(w, h, flg, levels);

  BaseTex *tex = BaseTex::create_tex(flg, RES3D_ARRTEX);
  set_tex_params(tex, w, h, d, flg, levels, stat_name);

  bool res = create_tex2d(tex->tex, tex, w, h, levels, tex->cube_array = false, NULL, d);
  if (!res)
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  tex->tex.memSize = tex->ressize();

  if (!add_texture_to_list(tex))
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  return tex;
}

BaseTexture *d3d::create_cube_array_tex(int side, int d, int flg, int levels, const char *stat_name)
{
  G_ASSERT_RETURN(d3d::check_cubetexformat(flg), nullptr);

  fixup_tex_params(side, side, flg, levels);

  BaseTex *tex = BaseTex::create_tex(flg, RES3D_ARRTEX);
  set_tex_params(tex, side, side, d, flg, levels, stat_name);

  bool res = create_tex2d(tex->tex, tex, side, side, levels, tex->cube_array = true, NULL, d);
  if (!res)
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  tex->tex.memSize = tex->ressize();

  if (!add_texture_to_list(tex))
  {
    BaseTex::destroy_tex(tex);
    return NULL;
  }

  return tex;
}

// load compressed texture
BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *stat_name)
{
  ddsx::Header hdr;
  if (!crd.readExact(&hdr, sizeof(hdr)) || !hdr.checkLabel())
  {
    debug("invalid DDSx format");
    return 0;
  }

  BaseTexture *tex = alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name);
  if (tex != NULL)
  {
    BaseTex *bt = (BaseTex *)tex;
    if (bt->cflg & TEXCF_SYSTEXCOPY)
      G_ASSERTF_AND_DO(hdr.hqPartLevels == 0, bt->cflg &= ~TEXCF_SYSTEXCOPY,
        "cannot use TEXCF_SYSTEXCOPY with base part of split texture!");
    if (bt->cflg & TEXCF_SYSTEXCOPY)
    {
      int data_sz = hdr.packedSz ? hdr.packedSz : hdr.memSz;
      clear_and_resize(bt->texCopy, sizeof(hdr) + data_sz);
      memcpy(bt->texCopy.data(), &hdr, sizeof(hdr));
      /*sysCopyQualityId*/ ((ddsx::Header *)bt->texCopy.data())->hqPartLevels = 0;
      if (!crd.readExact(bt->texCopy.data() + sizeof(hdr), data_sz))
      {
        LOGERR_CTX("inconsistent input tex data, data_sz=%d tex=%s", data_sz, stat_name);
        del_d3dres(tex);
        return NULL;
      }
      VERBOSE_DEBUG("%s %dx%d stored DDSx (%d bytes) for TEXCF_SYSTEXCOPY", stat_name, hdr.w, hdr.h, data_size(bt->texCopy));
      InPlaceMemLoadCB mcrd(bt->texCopy.data() + sizeof(hdr), data_sz);
      if (load_ddsx_tex_contents(tex, hdr, mcrd, quality_id) == TexLoadRes::OK)
        return tex;
    }
    else if (load_ddsx_tex_contents(tex, hdr, crd, quality_id) == TexLoadRes::OK)
      return tex;
    del_d3dres(tex);
  }
  return NULL;
}

BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int q_id, int levels, const char *stat_name, int stub_tex_idx)
{
  flg = implant_d3dformat(flg, hdr.d3dFormat);
  if (hdr.d3dFormat == D3DFMT_A4R4G4B4 || hdr.d3dFormat == D3DFMT_X4R4G4B4 || hdr.d3dFormat == D3DFMT_R5G6B5)
    flg = implant_d3dformat(flg, D3DFMT_A8R8G8B8);
  override_unsupported_bc((uint32_t &)flg);
  G_ASSERT((flg & TEXCF_RTARGET) == 0);
  flg |= (hdr.flags & hdr.FLG_GAMMA_EQ_1) ? 0 : TEXCF_SRGBREAD;

  if (levels <= 0)
    levels = hdr.levels;

  BaseTex *bt = nullptr;
  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    bt = BaseTex::create_tex(flg, RES3D_CUBETEX);
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    bt = BaseTex::create_tex(flg, RES3D_VOLTEX);
  else if (hdr.flags & ddsx::Header::FLG_ARRTEX)
    bt = BaseTex::create_tex(flg, RES3D_ARRTEX);
  else
    bt = BaseTex::create_tex(flg, RES3D_TEX);

  int skip_levels = hdr.getSkipLevels(hdr.getSkipLevelsFromQ(q_id), levels);
  int w = max(hdr.w >> skip_levels, 1), h = max(hdr.h >> skip_levels, 1), d = max(hdr.depth >> skip_levels, 1);
  if (!(hdr.flags & hdr.FLG_VOLTEX))
    d = (hdr.flags & hdr.FLG_ARRTEX) ? hdr.depth : 1;

  set_tex_params(bt, w, h, d, flg | (drv3d_dx11::immutable_textures ? TEXCF_LOADONCE : 0), levels, stat_name);
  bt->stubTexIdx = stub_tex_idx;
  bt->preallocBeforeLoad = bt->delayedCreate = true;
  bt->texaddru(hdr.getAddrU());
  bt->texaddrv(hdr.getAddrV());

  if (!add_texture_to_list(bt))
  {
    BaseTex::destroy_tex(bt);
    return NULL;
  }

  if (stub_tex_idx >= 0)
  {
    bt->tex.tex2D = bt->getStubTex()->tex.tex2D;
    bt->tex.tex2D->AddRef();
    bt->ddsxNotLoaded = 0;
    return bt;
  }
  bt->ddsxNotLoaded = 1;

  return bt;
}

unsigned d3d::pcwin32::get_texture_format(BaseTexture *tex)
{
  BaseTex *bt = (BaseTex *)tex;
  if (!bt)
    return 0;
  return bt->format;
}
const char *d3d::pcwin32::get_texture_format_str(BaseTexture *tex)
{
  BaseTex *bt = (BaseTex *)tex;
  if (!bt)
    return NULL;
  return dxgi_format_to_string(bt->format);
}

void *d3d::pcwin32::get_native_surface(BaseTexture *tex) { return ((BaseTex *)tex)->tex.tex2D; }

struct DX11BtSortRec
{
  BaseTex *t;
  int szKb, lfu;

  DX11BtSortRec(BaseTex *_t)
  {
    t = _t;
    szKb = t->isStub() ? 0 : tql::sizeInKb(t->ressize());
    lfu = tql::get_tex_lfu(t->getTID());
  }
  BaseTex *operator->() { return t; }
};
static int sort_textures_by_size(const DX11BtSortRec *left, const DX11BtSortRec *right)
{
  if (int diff = left->szKb - right->szKb)
    return -diff;
  if (int diff = left->t->type - right->t->type)
    return diff;
  if (int diff = strcmp(left->t->getResName(), right->t->getResName()))
    return diff;
  return right->lfu - left->lfu;
}

const char *tex_mipfilter_string(uint32_t mipFilter)
{
  static const char *filterMode[5] = {"default", "point", "linear", "unknown"};
  return filterMode[min(mipFilter, (uint32_t)4)];
}
const char *tex_filter_string(uint32_t filter)
{
  static const char *filterMode[6] = {"default", "point", "smooth", "best", "none", "unknown"};
  return filterMode[min(filter, (uint32_t)5)];
}

#if DAGOR_DBGLEVEL > 0
void drv3d_dx11::dump_resources(Tab<ResourceDumpInfo> &dump_info)
{
  ITERATE_OVER_OBJECT_POOL(g_textures, textureNo)
    if (g_textures[textureNo].obj != NULL)
    {
      BaseTex *tex = g_textures[textureNo].obj;

      resource_dump_types::TextureTypes type = resource_dump_types::TextureTypes::TEX2D;
      switch (tex->type)
      {
        case RES3D_TEX: type = resource_dump_types::TextureTypes::TEX2D; break;
        case RES3D_CUBETEX: type = resource_dump_types::TextureTypes::CUBETEX; break;
        case RES3D_VOLTEX: type = resource_dump_types::TextureTypes::VOLTEX; break;
        case RES3D_ARRTEX: type = resource_dump_types::TextureTypes::ARRTEX; break;
        case RES3D_CUBEARRTEX: type = resource_dump_types::TextureTypes::CUBEARRTEX; break;
      }

      dump_info.emplace_back(ResourceDumpTexture({(uint64_t)-1, (uint64_t)-1, (uint64_t)-1, tex->width, tex->height,
        tex->level_count(),
        (type == resource_dump_types::TextureTypes::VOLTEX || type == resource_dump_types::TextureTypes::CUBEARRTEX) ? tex->depth : -1,
        !(type == resource_dump_types::TextureTypes::ARRTEX)
          ? ((type == resource_dump_types::TextureTypes::CUBETEX || type == resource_dump_types::TextureTypes::CUBEARRTEX) ? 6 : -1)
          : tex->depth,
        tex->ressize(), tex->cflg, (uint32_t)tex->base_format, type, !is_depth_format_flg((uint32_t)tex->base_format),
        tex->getResName(), ""}));
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures);

  dump_buffers(dump_info);
}
#else
void drv3d_dx11::dump_resources(Tab<ResourceDumpInfo> &) {}
#endif

#include "statStr.h"

void d3d::get_texture_statistics(uint32_t *num_textures, uint64_t *total_mem, String *out_dump)
{
  g_textures.lock();

  int totalTextures = g_textures.totalUsed();
  if (num_textures)
    *num_textures = totalTextures;
  if (total_mem)
    *total_mem = 0;

  Tab<DX11BtSortRec> sortedTexturesList(tmpmem);
  sortedTexturesList.reserve(totalTextures);

  ITERATE_OVER_OBJECT_POOL(g_textures, textureNo)
    if (g_textures[textureNo].obj != NULL)
      sortedTexturesList.push_back(g_textures[textureNo].obj);
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures);

  sort(sortedTexturesList, &sort_textures_by_size);

  uint64_t totalSize2d = 0;
  uint64_t totalSizeCube = 0;
  uint64_t totalSizeVol = 0;
  uint64_t totalSizeArr = 0;
  uint32_t numUnknownSize = 0;
  uint64_t texCopySz = 0;
  uint64_t stagingTexSz = 0;
  for (unsigned int textureNo = 0; textureNo < sortedTexturesList.size(); textureNo++)
  {
    texCopySz += data_size(sortedTexturesList[textureNo]->texCopy);
    uint64_t sz = sortedTexturesList[textureNo]->ressize();
    if (sz == 0)
    {
      numUnknownSize++;
    }
    else
    {
      if (sortedTexturesList[textureNo]->tex.stagingTex2D)
        stagingTexSz += sz;
      if (sortedTexturesList[textureNo]->type == RES3D_TEX)
        totalSize2d += sz;
      else if (sortedTexturesList[textureNo]->type == RES3D_CUBETEX)
        totalSizeCube += sz;
      else if (sortedTexturesList[textureNo]->type == RES3D_VOLTEX)
        totalSizeVol += sz;
      else if (sortedTexturesList[textureNo]->type == RES3D_ARRTEX)
        totalSizeArr += sz;
    }
  }

  uint64_t texturesMem = totalSize2d + totalSizeCube + totalSizeVol + totalSizeArr;
  int vb_sysmem = 0;
  String vbibmem;
  String tmpStor;
  uint64_t vb_ib_mem = get_ib_vb_mem_used(&vbibmem, vb_sysmem);

  if (total_mem)
    *total_mem = totalSize2d + totalSizeCube + totalSizeVol;

  if (out_dump)
  {
    *out_dump += String(200,
      "TOTAL: %dM\n"
      "%d textures: %dM (%llu), 2d: %dM (%llu), cube: %dM (%llu), vol: %dM (%llu), arr: %dM (%llu), staging(%dM), %d stubs/unknown\n"
      "sysmem for drvReset: %dM + %dM (%dK),   frame=%d\n"
      "vb/ib mem: %dM\n",
      (vb_ib_mem >> 20) + texturesMem / 1024 / 1024, totalTextures, texturesMem / 1024 / 1024, texturesMem, totalSize2d / 1024 / 1024,
      totalSize2d, totalSizeCube / 1024 / 1024, totalSizeCube, totalSizeVol / 1024 / 1024, totalSizeVol, totalSizeArr / 1024 / 1024,
      totalSizeArr, stagingTexSz / 1024 / 1024, numUnknownSize, texCopySz >> 20, vb_sysmem >> 20, vb_sysmem >> 10, dagor_frame_no(),
      vb_ib_mem >> 20);

    *out_dump += String(0, "\n");

    STAT_STR_BEGIN(add);
    String stagings(128, "Staging textures: %dMb:\n", stagingTexSz / 1024 / 1024);
    for (unsigned int textureNo = 0; textureNo < sortedTexturesList.size(); textureNo++)
    {
      BaseTex *tex = (BaseTex *)sortedTexturesList[textureNo].t;
      uint64_t sz = sortedTexturesList[textureNo].szKb;
      int ql = tql::get_tex_cur_ql(tex->getTID());
      ql = ql == 15 ? '-' : '0' + ql;
      if (tex->tex.stagingTex2D)
        stagings.aprintf(128, "%5dK %s\n", sz, tex->getResName());
      if (tex->type == RES3D_TEX)
      {
        add.printf(0, "%5dK q%c  2d  %4dx%-4d, m%2d, aniso=%-2d filter=%7s mip=%s(%d..%d), %s - '%s'", sz, ql, tex->width, tex->height,
          tex->mipLevels, tex->anisotropyLevel, tex_filter_string(tex->texFilter), tex_mipfilter_string(tex->mipFilter),
          tex->maxMipLevel, tex->minMipLevel, dxgi_format_to_string(dxgi_format_for_res_auto(tex->format, tex->cflg)),
          tex->getResName());
      }
      else if (tex->type == RES3D_CUBETEX)
      {
        add.printf(0, "%5dK q%c  cube %4d, m%2d, %s - '%s'", sz, ql, tex->width, tex->mipLevels,
          dxgi_format_to_string(dxgi_format_for_res_auto(tex->format, tex->cflg)), tex->getResName());
      }
      else
      {
        add.printf(0, "%5dK q%c  %s %4dx%-4dx%3d, m%2d, aniso=%-2d filter=%7s mip=%s, %s - '%s'", sz, ql,
          (tex->type == RES3D_VOLTEX) ? "vol" : ((tex->type == RES3D_ARRTEX) ? "arr" : " ? "), tex->width, tex->height, tex->depth,
          tex->mipLevels, tex->anisotropyLevel, tex_filter_string(tex->texFilter), tex_mipfilter_string(tex->mipFilter),
          dxgi_format_to_string(dxgi_format_for_res_auto(tex->format, tex->cflg)), tex->getResName());
      }

      add += num_textures ? "\n"
                          : String(0, " cache=%d%s\n", tex->viewCache.size(), tql::get_tex_info(tex->getTID(), false, tmpStor)).str();

      STAT_STR_APPEND(add, *out_dump);
    }
    stagings += "       ---\n";
    STAT_STR_APPEND(stagings, *out_dump);
    STAT_STR_END(*out_dump);
    *out_dump += vbibmem;

    get_shaders_mem_used(vbibmem);
    *out_dump += vbibmem;
    get_states_mem_used(vbibmem);
    *out_dump += vbibmem;
  }

  g_textures.unlock();

  //  add_vbo_stat(statistics);
}


void get_mem_stat(String *out_str)
{
  g_textures.lock();

  int totalTextures = g_textures.totalUsed();

  uint64_t totalSizeRt = 0;
  uint64_t totalSize2d = 0;
  uint64_t totalSizeCube = 0;
  uint64_t totalSizeVol = 0;
  uint64_t totalSizeArr = 0;
  uint64_t stagingTexSz = 0;

  ITERATE_OVER_OBJECT_POOL(g_textures, textureNo)
    if (g_textures[textureNo].obj)
    {
      BaseTex *tex = (BaseTex *)g_textures[textureNo].obj;
      uint64_t sz = tex->ressize();

      if (tex->tex.stagingTex2D)
        stagingTexSz += sz;

      if (tex->cflg & TEXCF_RTARGET)
        totalSizeRt += sz;
      else if (tex->type == RES3D_TEX)
        totalSize2d += sz;
      else if (tex->type == RES3D_CUBETEX)
        totalSizeCube += sz;
      else if (tex->type == RES3D_VOLTEX)
        totalSizeVol += sz;
      else if (tex->type == RES3D_ARRTEX)
        totalSizeArr += sz;
    }
  ITERATE_OVER_OBJECT_POOL_RESTORE(g_textures);

  int vbSysMem = 0;
  uint64_t vbIbMem = get_ib_vb_mem_used(NULL, vbSysMem);

  *out_str = String(0, "r:%d 2:%d c:%d v:%d a:%d s:%d v:%d", totalSizeRt >> 20, totalSize2d >> 20, totalSizeCube >> 20,
    totalSizeVol >> 20, totalSizeArr >> 20, stagingTexSz >> 20, vbIbMem >> 20);

  g_textures.unlock();
}


BaseTexture *d3d::alias_tex(BaseTexture *baseTexture, TexImage32 *img, int w, int h, int flg, int levels, const char *stat_name)
{
  return nullptr;
}

BaseTexture *d3d::alias_cubetex(BaseTexture *baseTexture, int size, int flg, int levels, const char *stat_name) { return nullptr; }

BaseTexture *d3d::alias_voltex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}

BaseTexture *d3d::alias_array_tex(BaseTexture *baseTexture, int w, int h, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}

BaseTexture *d3d::alias_cube_array_tex(BaseTexture *baseTexture, int side, int d, int flg, int levels, const char *stat_name)
{
  return nullptr;
}
