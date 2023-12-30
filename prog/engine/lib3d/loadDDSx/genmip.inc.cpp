#pragma once

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_C3
#include <sys/mman.h>
#endif

#if _TARGET_C1 | _TARGET_C2 | _TARGET_ANDROID | _TARGET_C3 | _TARGET_XBOX | (_TARGET_PC_WIN && _TARGET_64BIT)
#define SLOT_BASED_GENMIP_MEM 1
#else
#define SLOT_BASED_GENMIP_MEM 0
#endif

#include <memory/dag_physMem.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_spinlock.h>
#include <osApiWrappers/dag_critSec.h>
#include <math/dag_imageFunctions.h>
#include <image/dag_dxtCompress.h>
#include <generic/dag_smallTab.h>
#include <3d/tql.h>

extern void decompress_dxt1(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data);
extern void decompress_dxt3(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data);
extern void decompress_dxt5(unsigned char *decompressedData, int lw, int lh, int row_pitch, unsigned char *src_data);

extern void decompress_dxt1_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *temp_data);
extern void decompress_dxt3_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *temp_data);
extern void decompress_dxt5_downsample4x(unsigned char *downsampledData, int lw, int lh, int row_pitch, unsigned char *src_data,
  unsigned char *temp_data);

using namespace dagor_phys_memory;

#if _TARGET_XBOX
#include "genmip_xbox.h"
#else
#define PAGE_ALIGN_MASK 0xFFF
#endif

enum Filter
{
  FILTER_NEAREST,
  FILTER_BOX,
  FILTER_KAISER,
};

#ifndef GENMIP_ON_MIP_DONE
#define GENMIP_ON_MIP_DONE(tex, dst, pitch, src_pitch, src_lines, last, w, h)
#endif

class BoxFilter
{
public:
  void setTemp(void *) {}
  static bool needTemp() { return false; }
  // static bool canGammaInplace() {return true;}
  static bool canGammaInplace()
  {
    return false;
  } // inplace gamma is slower and of worser quality, and needed only if we don't have enough memory
  static inline void downsample4x(unsigned char *destData, unsigned char *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else
    {
      if (destW < 2)
        imagefunctions::downsample4x_simdu(destData, srcData, destW, destH);
      else
        imagefunctions::downsample4x_simda(destData, srcData, destW, destH);
    }
  }

  static inline void downsample4x_gamma_correct(unsigned char *destData, unsigned char *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else
    {
      if (destW < 2)
        imagefunctions::downsample4x_simdu(destData, srcData, destW, destH);
      else
        imagefunctions::downsample4x_simda_gamma_correct(destData, srcData, destW, destH);
    }
  }

  static inline void downsample4x(float *destData, float *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else
      imagefunctions::downsample4x_simda(destData, srcData, destW, destH);
  }
  void read_parameters(IGenLoad &crd, bool read_kaizer_always)
  {
    if (read_kaizer_always)
    {
      crd.readInt();
      crd.readInt();
    }
  }
};

class KaiserFilter
{
  float alpha, stretch;
  vec4f kernel[4];
  void *temp;

public:
  void setTemp(void *t) { temp = t; }
  static bool needTemp() { return true; }
  static bool canGammaInplace() { return false; }
  KaiserFilter(float alpha_, float stretch_) : alpha(alpha_), stretch(stretch_), temp(NULL) { memset(kernel, 0, sizeof(kernel)); }
  ~KaiserFilter() {}

  inline void downsample4x(unsigned char *destData, unsigned char *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else if (destW <= 8 && destH <= 8)
      imagefunctions::downsample4x_simdu(destData, srcData, destW, destH);
    else
    {
      imagefunctions::kaiser_downsample(destW, destH, srcData, destData, (float *)kernel, temp);
    }
  }

  static inline void downsample4x_gamma_correct(unsigned char *destData, unsigned char *srcData, int destW, int destH)
  {
    (void)destData;
    (void)srcData;
    (void)destW;
    (void)destH;
    G_ASSERT(0);
  }

  inline void downsample4x(float *destData, float *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else if (destW <= 8 && destH <= 8)
      imagefunctions::downsample4x_simda(destData, srcData, destW, destH);
    else
    {
      // fixme: kaiser temp could be allocated once for each thread, and re-allocated only if bigger size needed
      imagefunctions::kaiser_downsample(destW, destH, srcData, destData, (float *)kernel, temp);
    }
  }

  void read_parameters(IGenLoad &crd, bool /*read_kaizer_always*/)
  {
    alpha = crd.readReal();
    stretch = crd.readReal();
    imagefunctions::prepare_kaiser_kernel(alpha, stretch, (float *)&kernel[0]);
  }
};

static int page_align(int sz) { return (sz + PAGE_ALIGN_MASK) & ~PAGE_ALIGN_MASK; }

static CritSecStorage memCritSec;

// PS4 and Android uses multiple individual regions for muiltithreaded processing
// while other versions work in single region (genmip_mem, genmip_memsz)
// pc/xbox dynamically commit/release memory from virtually allocated region
// mac/linux  mmap maximum possible region

static void *genmip_mem = NULL;
static unsigned genmip_memsz = 0;
static int max_gen_gamma_texture_size = 0;
int max_keep_systexture_size = 1024; // could be readed from config

#define MAX_FALLBACKS      (2)
#define MEMORY_TO_FALLBACK (2048ull << 20)

#if SLOT_BASED_GENMIP_MEM
static constexpr int MAX_GENMIP_CTX = 8;
namespace
{
struct GenmipMemContext
{
  void *memSlots[2] = {nullptr, nullptr}; // src_data | temp_data

  static unsigned freeMask;
  static OSSpinlock maskSL;
  static unsigned allocCtx()
  {
    OSSpinlockScopedLock lock(maskSL);
    if (!freeMask)
      return 0;
    unsigned idx = __bsf(freeMask);
    freeMask &= ~(1u << idx);
    return idx + 1;
  }
  static void freeCtx(unsigned idx)
  {
    if (!idx)
      return;
    idx--;
    OSSpinlockScopedLock lock(maskSL);
    G_ASSERTF(!(freeMask & (1u << idx)), "freeMask=0x%x idx=%d", freeMask, idx);
    freeMask |= 1u << idx;
  }
};
unsigned GenmipMemContext::freeMask = (1 << MAX_GENMIP_CTX) - 1;

OSSpinlock GenmipMemContext::maskSL;
} // namespace
static GenmipMemContext genmip_ctx[MAX_GENMIP_CTX];
static thread_local unsigned genmip_ctx_idx = 0;

static void *try_alloc(unsigned sz, int slot)
{
  if (!genmip_ctx_idx)
    if ((genmip_ctx_idx = GenmipMemContext::allocCtx()) == 0)
      return nullptr;
  GenmipMemContext &ctx = genmip_ctx[genmip_ctx_idx - 1];
  sz = POW2_ALIGN(sz, 16384);
#if _TARGET_C1 | _TARGET_C2

#else
  ctx.memSlots[slot] = tmpmem->tryAlloc(sz);
#endif
  // debug("-- genmip_alloc[%d] %dK -> %p", slot, sz>>10, genmip_mem_slots[slot]);
  return ctx.memSlots[slot];
}

// wait until enough memory will be available
static void *genmip_alloc(unsigned sz, int slot)
{
  G_ASSERT(genmip_memsz); // check if initialized
  G_ASSERT(genmip_mem);
  const int stepMs = 50;
  for (int timeout = 20 * 1000; timeout > 0; timeout -= stepMs)
  {
    void *ptr = try_alloc(sz, slot);
    if (ptr != NULL)
      return ptr;
    sleep_msec(stepMs);
  }
  return NULL;
}

static void genmip_free(int slot)
{
  if (!genmip_ctx_idx)
    return;
  GenmipMemContext &ctx = genmip_ctx[genmip_ctx_idx - 1];
  if (ctx.memSlots[slot])
  {
    // debug("-- genmip_free[%d]  %p", slot, genmip_mem_slots[slot]);
#if _TARGET_C1 | _TARGET_C2

#else
    memfree(ctx.memSlots[slot], tmpmem);
#endif
    ctx.memSlots[slot] = 0;
  }
}
#endif

void d3d_genmip_reserve(int sz)
{
  debug("d3d_genmip_reserve(%d)", sz);
  if (!genmip_mem && !sz)
    return;

  if (genmip_mem)
  {
    debug("release(%p, %dK)", genmip_mem, genmip_memsz >> 10);
#if SLOT_BASED_GENMIP_MEM
    // do nothing
#elif _TARGET_PC_WIN | _TARGET_XBOX
    ::VirtualFree(genmip_mem, 0, MEM_RELEASE);
#else
    munmap(genmip_mem, genmip_memsz);
#endif
    genmip_mem = NULL;
    genmip_memsz = 0;
  }

  if (sz)
  {
    int maxFallbacks = MAX_FALLBACKS;
    bool decode_downsample = false;

#if !SLOT_BASED_GENMIP_MEM && _TARGET_PC_WIN
    OSVERSIONINFOEX osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.dwMajorVersion = 6;
    GetVersionEx((OSVERSIONINFO *)&osvi);
    MEMORYSTATUSEX statex;
    memset(&statex, 255, sizeof(statex)); // set mem to max
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    const char *reason = NULL;
    if (statex.ullTotalVirtual <= MEMORY_TO_FALLBACK)
      reason = "low virtual memory";
    else if (statex.ullTotalPhys <= MEMORY_TO_FALLBACK)
      reason = "low physical memory";
    else if (osvi.dwMajorVersion < 6)
      reason = "low winver";
    if (reason)
    {
      sz /= 2;
      maxFallbacks = 1;
      debug("reduce max float genmip size due to %s to %d", reason, sz);

      decode_downsample = true; // always make footprint as low as possible
    }
#endif

    for (int pass = 0; pass < maxFallbacks; ++pass)
    {
      if (decode_downsample) //-V547
      {
        max_gen_gamma_texture_size = sz / 2;
        const int dxt_sz = sz * sz * 4;                 // original dxt (4x) | original dxt (1x)
        const int rgba_sz = sz * sz * sizeof(uint32_t); // downsampled rgba  | original rgba
        const int tmp_sz = sz * 2 * 4 * 4;              // one stripe of 4x4 rgba blocks of source (2x) texture.
        genmip_memsz = page_align(dxt_sz) + page_align(rgba_sz) + page_align(tmp_sz);
      }
      else
      {
        max_gen_gamma_texture_size = sz;
        genmip_memsz = page_align(sz * sz * 16) + page_align(sz * sz * 8);
      }

#if SLOT_BASED_GENMIP_MEM
      genmip_mem = (void *)(intptr_t)1;
#elif _TARGET_PC_WIN | _TARGET_XBOX
      for (int i = 0; i < 2 && !genmip_mem; ++i)
      {
#if _TARGET_XBOX
        genmip_mem = xbox_virtual_alloc(0, genmip_memsz, MEM_RESERVE | (!i ? MEM_TOP_DOWN : 0));
#else
        genmip_mem = ::VirtualAlloc(NULL, genmip_memsz, MEM_RESERVE | (i == 0 ? MEM_TOP_DOWN : 0), PAGE_READWRITE);
#endif
        debug("reserve(%dK)=%p, err=0x%x", genmip_memsz >> 10, genmip_mem, ::GetLastError());
      }
#else
      genmip_mem = mmap(0, genmip_memsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
      if (genmip_mem) //-V547
        break;
      else if (pass != (MAX_FALLBACKS - 1))
      {
        debug("not enough memory for gamma corrected textures of size = %d, try = %d", sz, sz / 2);
        sz /= 2;
      }
    }

    if (!genmip_mem)
      DAG_FATAL("Not enough memory - needed %dK", genmip_memsz >> 10);

    ::create_critical_section(memCritSec);
  }
  else
    ::destroy_critical_section(memCritSec);
}

#if !SLOT_BASED_GENMIP_MEM
// commit memory at region (offset, end-offset)
// size = total area size
// returns base addr (not counting offset)
static void *genmip_commit_phys_mem(uint32_t mem_offset, int size)
{
  G_UNREFERENCED(mem_offset);
  G_UNREFERENCED(size);
  G_ASSERT(genmip_memsz);
  G_ASSERT(genmip_mem);
  if (!genmip_mem)
    return NULL;
#if _TARGET_PC_WIN | _TARGET_XBOX
#if _TARGET_XBOX
  if (xbox_virtual_alloc((char *)genmip_mem + mem_offset, size, MEM_COMMIT) == NULL)
#else
  if (VirtualAlloc((char *)genmip_mem + mem_offset, size, MEM_COMMIT, PAGE_READWRITE) == NULL)
#endif
  {
    debug("VirtualAlloc(%p, %dK) failed with %d", genmip_mem, size >> 10, GetLastError());
    return NULL;
  }
#endif
  return genmip_mem;
}
#endif

// fix size, so it points to the next byte of sequential area on pc, or 0 on ps4/android
static uint8_t *genmip_commit_source(int size, int &offset_out)
{
  size = page_align(size);
#if SLOT_BASED_GENMIP_MEM
  offset_out = 0;
  return (uint8_t *)genmip_alloc(size, 0);
#else
  offset_out = size;
  return (uint8_t *)genmip_commit_phys_mem(0, size);
#endif
}

static uint8_t *genmip_commit_data(int offset, int size)
{
#if SLOT_BASED_GENMIP_MEM
  (void)offset;
  return (uint8_t *)genmip_alloc(size, 1);
#else
  return (uint8_t *)genmip_commit_phys_mem(offset, size);
#endif
}

static void genmip_release_phys_mem()
{
#if SLOT_BASED_GENMIP_MEM
  genmip_free(1);
  genmip_free(0);
  GenmipMemContext::freeCtx(genmip_ctx_idx);
  genmip_ctx_idx = 0;
#elif _TARGET_PC_WIN | _TARGET_XBOX
  ::VirtualFree(genmip_mem, genmip_memsz, MEM_DECOMMIT);
#elif _TARGET_APPLE
  madvise(genmip_mem, genmip_memsz, MADV_FREE);
#else
  madvise(genmip_mem, genmip_memsz, MADV_DONTNEED);
#endif
}

// ps4/android:
//    temp [2*dxt_size](opt) | [w * quad](opt)
//    rgba [rgba_sz]
// pc: (overlap memory to save space)
//    dxt   [dxt_size]
//    temp  [dxt_size](opt)  | [w * quad](opt) <- offset
//    rgba  [rgba_size]
//  kaiser filter uses dxt+temp as one continuous field
//
// dxt->rgba. rgba -> temp. temp -> rgba. temp should not intersect with rgba, and dxt with rgba.
//  rgba_w can be 1x or 0.5x of w
static void genmip_commit_phys_mem_char_dxt(int offset, int w, int h, int rgba_w, int rgba_h, bool need_temp, uint8_t *&rgba,
  void *&temp)
{
  int dxt_sz = page_align(w * h);
  int rgba_sz = page_align(rgba_w * rgba_h * 4); // normal or downsampled rgba
  int tmp_sz = rgba_w != w ? page_align(w * 16)  // low memory, decode+downsample
                           : page_align(need_temp ? dxt_sz * 2 : dxt_sz) - offset;
  uint8_t *data = genmip_commit_data(offset, tmp_sz + rgba_sz);
  temp = nullptr;
  rgba = nullptr;
  G_ASSERTF_RETURN(data, , "genmip_commit_data(%d, %dK) failed", offset, (tmp_sz + rgba_sz) >> 10);
  temp = data + offset;
  rgba = data + offset + tmp_sz;
  // debug("char_dxt temp=%p rgba=%p dxt=%0x rgba=%x tmp=%x ofs=%x", temp, rgba, dxt_sz, rgba_sz, tmp_sz, offset);
}

// ps4/android:
//   gamma         [float_sz]
//   rgba = temp   [tmp_sz]
// pc:
//   dxt | gamma   [dxt_sz]
//                 [float_sz - dxt_sz] <- offset
//   rgba = temp   [tmp_sz]
// can overlap dxt and gamma. dxt->rgba. rgba->gamma.
static bool genmip_commit_phys_mem_float_dxt(int offset, int w, int h, bool need_temp, uint8_t *&rgba, float *&gamma, void *&temp)
{
  int float_sz = page_align(w * h * 16);
  int tmp_sz = page_align(need_temp ? w * h * 8 : w * h * 4);
  gamma = (float *)genmip_commit_data(offset, float_sz - offset + tmp_sz);
  if (!gamma)
    return false;
  rgba = (uint8_t *)gamma + float_sz;
  temp = need_temp ? rgba : NULL;
  return true;
}

// ps4/android:
//   temp   [tmp_sz]
// pc:
//   rgba   [rgba_sz]
//   temp   [tmp_sz]   <- offset
// rgba -> temp. temp -> rgba. temp should not intersect with rgba
static void genmip_commit_phys_mem_char_rgba(int offset, int w, int h, bool need_temp, void *&temp)
{
  if (!need_temp)
    return;
  int tmp_sz = page_align(w * h * 2); // rgba downsampled 2X
  uint8_t *data = genmip_commit_data(offset, tmp_sz);
  temp = nullptr;
  G_ASSERTF_RETURN(data, , "genmip_commit_data(%d, %dK) failed", offset, tmp_sz >> 10);
  temp = (uint8_t *)data + offset;
}

// ps4/android:
//   temp   [tmp_sz]
//   gamma  [float_sz]
// pc:
//   rgba | temp  [rgba_sz] (rgba)
//                [tmp_sz - rgba_sz] <- offset
//   gamma        [float_sz]
//     rgba+temp overlapped
// rgba->gamma. gamma -> temp. temp -> gamma. gamma->rgba. temp should not intersect with gamma, and gamma with rgba.tmp_sz
static bool genmip_commit_phys_mem_float_rgba(int offset, int w, int h, bool need_temp, float *&gamma, void *&temp)
{
  int float_sz = page_align(w * h * 16);
  int tmp_sz = page_align(need_temp ? w * h * 8 : w * h * 4);
  uint8_t *data = genmip_commit_data(offset, tmp_sz - offset + float_sz);
  if (!data)
    return false;
  gamma = (float *)((uint8_t *)data + tmp_sz);
  temp = need_temp ? data : NULL;
  return true;
}

#if SLOT_BASED_GENMIP_MEM
struct AutoAcquireMem
{
  AutoAcquireMem() {}
  ~AutoAcquireMem() {}
};

#else
static void acquireMem()
{
  G_ASSERTF(genmip_mem, "d3d_genmip_reserve() not called");
  ::enter_critical_section(memCritSec);
}
static void releaseMem()
{
  G_ASSERTF(genmip_mem, "d3d_genmip_reserve() not called");
  ::leave_critical_section(memCritSec);
}

struct AutoAcquireMem
{
  AutoAcquireMem() { acquireMem(); }
  ~AutoAcquireMem() { releaseMem(); }
};
#endif

#if _TARGET_PC | _TARGET_XBOX
#define GEN_DELSYSMEMCOPY      TEXLOCK_DELSYSMEMCOPY
#define GEN_DONOTUPDATE        TEXLOCK_DONOTUPDATEON9EXBYDEFAULT
#define GEN_DONT_DELSYSMEMCOPY TEXLOCK_UPDATEFROMSYSTEX
#else
#define GEN_DELSYSMEMCOPY      0
#define GEN_DONOTUPDATE        0
#define GEN_DONT_DELSYSMEMCOPY 0
#endif

template <class Filter, bool force_gamma_space = true>
bool create_dxt_mip_chain(Texture *tex, TEXTUREID tid, unsigned char *base_tex, int base_tex_fmt, const ddsx::Header &hdr,
  IGenLoad &crd, unsigned fmt, int start_level, int levels, int skip_levels, int dest_slice, Filter &f, int dxt_alg)
{
  G_ASSERT(tex || (hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY));
  int offset;
  uint8_t *dxtData = genmip_commit_source(hdr.w * hdr.h, offset);
  if (dxtData == NULL)
    return false;
  struct ReleasePhysMem
  {
    ~ReleasePhysMem() { genmip_release_phys_mem(); }
  } genmip_auto_release_phys_mem;

  bool diff_tex = (hdr.flags & hdr.FLG_NEED_PAIRED_BASETEX) ? true : false;
  if (!diff_tex)
    if (!crd.readExact(dxtData, hdr.getSurfaceSz(0)))
      return false;
  const float gamma = crd.readReal();
  f.read_parameters(crd, diff_tex);

  if (diff_tex)
  {
    struct Dxt1ColorBlock
    {
      unsigned c0r : 5, c0g : 6, c0b : 5;
      unsigned c1r : 5, c1g : 6, c1b : 5;
      unsigned idx;
    };
    struct Dxt5AlphaBlock
    {
      unsigned char a0, a1;
      unsigned short idx[3];
    };

    int blk_total = (hdr.w / 4) * (hdr.h / 4);
    int blk_empty = crd.readInt();

    Tab<unsigned> map(tmpmem);
    map.resize((blk_total + 31) / 32);
    if (blk_empty == 0)
      mem_set_ff(map);
    else if (blk_empty == blk_total)
      mem_set_0(map);
    else if (!crd.readExact(map.data(), data_size(map)))
      return false;

    int dxt_blk_sz = (fmt == D3DFMT_DXT1) ? 8 : 16;
    if (base_tex_fmt == fmt)
      memcpy(dxtData, base_tex, blk_total * dxt_blk_sz);
    else if (base_tex_fmt == D3DFMT_DXT1)
      for (int i = 0; i < blk_total; i++)
      {
        memset(dxtData + i * 16, 0xFF, 2);
        memset(dxtData + i * 16 + 2, 0, 6);
        memcpy(dxtData + i * 16 + 8, base_tex + i * 8, 8);
      }
    else if (base_tex_fmt == D3DFMT_DXT5)
      for (int i = 0; i < blk_total; i++)
        memcpy(dxtData + i * 8, base_tex + i * 16 + 8, 8);

    if (fmt == D3DFMT_DXT5)
    {
      for (int i = 0; i < map.size(); i++)
      {
        if (!map[i])
          continue;
        for (unsigned j = 0, w = map[i]; w; j++, w >>= 1)
        {
          if (!(w & 1))
            continue;

          Dxt5AlphaBlock pa, &ba = *(Dxt5AlphaBlock *)(dxtData + (i * 32 + j) * dxt_blk_sz);
          if (!crd.readExact(&pa, sizeof(pa)))
            return false;

          if (base_tex_fmt == D3DFMT_DXT5)
          {
            pa.a0 += ba.a0;
            pa.a1 += ba.a1;
            pa.idx[0] = ba.idx[0] ^ pa.idx[0];
            pa.idx[1] = ba.idx[1] ^ pa.idx[1];
            pa.idx[2] = ba.idx[2] ^ pa.idx[2];
          }
          memcpy(&ba, &pa, sizeof(pa));
        }
      }
    }

    for (int i = 0; i < map.size(); i++)
    {
      if (!map[i])
        continue;
      for (unsigned j = 0, w = map[i]; w; j++, w >>= 1)
      {
        if (!(w & 1))
          continue;

        Dxt1ColorBlock pc, &bc = *(Dxt1ColorBlock *)(dxtData + (i * 32 + j) * dxt_blk_sz + (fmt == D3DFMT_DXT5 ? 8 : 0));
        if (!crd.readExact(&pc, sizeof(pc)))
          return false;

        pc.c0r += bc.c0r;
        pc.c0g += bc.c0g;
        pc.c0b += bc.c0b;
        pc.c1r += bc.c1r;
        pc.c1g += bc.c1g;
        pc.c1b += bc.c1b;
        pc.idx = bc.idx ^ pc.idx;
        memcpy(&bc, &pc, sizeof(pc));
      }
    }
  }

  if (hdr.flags & hdr.FLG_HOLD_SYSMEM_COPY)
  {
    d3d_genmip_store_sysmem_copy(tid, hdr, dxtData);
    if (!tex)
      return true;
  }

  uint32_t w = hdr.w;
  uint32_t h = hdr.h;
  int mode = MODE_DXT1;

  bool skip_first_downsample = false;
  float *floatData = 0;
  void *kaiserTemp = 0;
  unsigned char *rgbaData = 0;

  if (!skip_levels)
  {
    bool res = true;
    if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, start_level, dest_slice))
    {
      char *dst = d3d::get_update_buffer_addr_for_write(rub);
      int dst_pitch = d3d::get_update_buffer_pitch(rub), src_pitch = hdr.getSurfacePitch(skip_levels);
      if (!dst)
        res = false;
      else if (dst_pitch == src_pitch)
        memcpy(dst, dxtData, hdr.getSurfaceSz(skip_levels));
      else if (dst_pitch > src_pitch)
      {
        int src_h = hdr.getSurfaceScanlines(skip_levels);
        for (const uint8_t *src = dxtData, *src_e = src + src_h * src_pitch; src < src_e; src += src_pitch, dst += dst_pitch)
          memcpy(dst, src, src_pitch);
        dst -= dst_pitch * src_h;
      }
      else if (dst_pitch)
      {
        TextureInfo ti;
        tex->getinfo(ti, start_level);
        logerr("tex(%s) mip=%dx%d src=%dx%d lockimg(%d) gives dst_pitch=%d < src_pitch=%d", tex->getTexName(), ti.w, ti.h, w, h,
          start_level, dst_pitch, src_pitch);
        d3d::release_update_buffer(rub);
        return false;
      }
      GENMIP_ON_MIP_DONE(tex, dst, src_pitch, dst_pitch, hdr.getSurfaceScanlines(0), levels == 1, w, h);
      if (!d3d::update_texture_and_release_update_buffer(rub))
        res = false;
      d3d::release_update_buffer(rub);
    }
    else
      res = false;

    if (!res)
      return false;
  }

  if (w > 4 && h > 4 && w * h * (4 + 2) > genmip_memsz) // rgba(4)+dxt(2)
  {
    // debug("%dx%d goes low skip_levels=%d", w,h, skip_levels);
    skip_first_downsample = true;
    G_ASSERT(w * h * 2 + w * 16 <= genmip_memsz); // Source DXT5 + downscaled RGBA + temp buffer for one decoded stripe.
  }

  if (force_gamma_space)
  {
    if (skip_first_downsample)
      genmip_commit_phys_mem_char_dxt(offset, w, h, w / 2, h / 2, f.needTemp(), rgbaData, kaiserTemp);
    else
      genmip_commit_phys_mem_char_dxt(offset, w, h, w, h, f.needTemp(), rgbaData, kaiserTemp);
  }
  else
  {
    if (!genmip_commit_phys_mem_float_dxt(offset, hdr.w, hdr.h, f.needTemp(), rgbaData, floatData, kaiserTemp))
      genmip_commit_phys_mem_char_dxt(offset, hdr.w, hdr.h, hdr.w, hdr.h, f.needTemp(), rgbaData, kaiserTemp);
  }
  if (!rgbaData)
    return false;

  if (skip_first_downsample)
  {
    // dxt->rgba via temp. require temp [w * 16]
    switch (fmt)
    {
      case D3DFMT_DXT1: decompress_dxt1_downsample4x(rgbaData, hdr.w, hdr.h, hdr.w * 4, dxtData, (unsigned char *)kaiserTemp); break;
      case D3DFMT_DXT3:
        decompress_dxt3_downsample4x(rgbaData, hdr.w, hdr.h, hdr.w * 4, dxtData, (unsigned char *)kaiserTemp);
        mode = MODE_DXT3;
        break;
      case D3DFMT_DXT5:
        decompress_dxt5_downsample4x(rgbaData, hdr.w, hdr.h, hdr.w * 4, dxtData, (unsigned char *)kaiserTemp);
        mode = MODE_DXT5;
        break;
      default: G_ASSERT(0);
    };
    f.setTemp(dxtData); // w*h
  }
  else
  {
    switch (fmt)
    {
      case D3DFMT_DXT1: decompress_dxt1(rgbaData, w, h, w * 4, dxtData); break;
      case D3DFMT_DXT3:
        decompress_dxt3(rgbaData, w, h, w * 4, dxtData);
        mode = MODE_DXT3;
        break;
      case D3DFMT_DXT5:
        decompress_dxt5(rgbaData, w, h, w * 4, dxtData);
        mode = MODE_DXT5;
        break;
      default: G_ASSERT(0);
    };
#if SLOT_BASED_GENMIP_MEM
    f.setTemp(kaiserTemp); // Cannot reuse because the genmip memory is not continuous there.
#else
    f.setTemp(force_gamma_space ? dxtData : kaiserTemp); // Reuse the source dxt as filter target because the source
#endif // will be unpacked to rgba at the moment.
  }

  bool canGammaInplace = f.canGammaInplace() && (fabsf(gamma - 2.2f) < 0.01f);
  if (!force_gamma_space && !canGammaInplace)
    imagefunctions::exponentiate4_c(rgbaData, floatData, w * h, gamma);
  float invgamma = 1.0f / gamma;

  for (int i = 0; i < skip_levels; ++i)
  {
    w >>= 1;
    h >>= 1;
    if (force_gamma_space)
    {
      if (i != 0 || !skip_first_downsample)
        f.downsample4x(rgbaData, rgbaData, w, h);
    }
    else
    {
      if (!canGammaInplace)
        f.downsample4x(floatData, floatData, w, h);
      else
        f.downsample4x_gamma_correct(rgbaData, rgbaData, w, h);
    }
  }

  bool res = true;
  if (skip_levels > 0)
  {
    skip_first_downsample = false;

    if (!force_gamma_space && !canGammaInplace)
      imagefunctions::convert_to_linear_simda(floatData, rgbaData, (w && h) ? w * h : w + h, invgamma);

    if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, start_level, dest_slice))
    {
      char *dst = d3d::get_update_buffer_addr_for_write(rub);
      int dst_pitch = d3d::get_update_buffer_pitch(rub);
      ManualDXT(mode, (TexPixel32 *)rgbaData, w, h, dst_pitch, dst, dxt_alg);
      GENMIP_ON_MIP_DONE(tex, dst, (w / 4) << hdr.dxtShift, dst_pitch, h / 4, levels == 1, w, h);
      if (!d3d::update_texture_and_release_update_buffer(rub))
        res = false;
      d3d::release_update_buffer(rub);
    }
    else
      res = false;
  }

  for (int i = 1; i < levels && res; i++)
  {
    w >>= 1;
    h >>= 1;
    if (force_gamma_space)
    {
      if (i != 1 || !skip_first_downsample)
        f.downsample4x(rgbaData, rgbaData, w, h);
    }
    else
    {
      if (!canGammaInplace)
        f.downsample4x(floatData, floatData, w, h);
      else
        f.downsample4x_gamma_correct(rgbaData, rgbaData, w, h);
    }
    // G_ASSERT(w && h);
    w = max(w, 1U);
    h = max(h, 1U);

    if (!force_gamma_space && !canGammaInplace)
      imagefunctions::convert_to_linear_simda(floatData, rgbaData, w * h, invgamma);

    if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, start_level + i, dest_slice))
    {
      char *dst = d3d::get_update_buffer_addr_for_write(rub);
      int dst_pitch = d3d::get_update_buffer_pitch(rub);
      ManualDXT(mode, (TexPixel32 *)rgbaData, w, h, dst_pitch, dst, dxt_alg);
      GENMIP_ON_MIP_DONE(tex, dst, (w / 4) << hdr.dxtShift, dst_pitch, h / 4, i + 1 == levels, w, h);
      if (!d3d::update_texture_and_release_update_buffer(rub))
        res = false;
      d3d::release_update_buffer(rub);
    }
    else
      res = false;
  }
  return res;
}

template <class Filter, bool force_gamma_space = true>
bool create_rgba8_mip_chain(Texture *tex, const ddsx::Header &hdr, IGenLoad &crd, int start_level, int levels, int skip_levels,
  int dest_slice, Filter &f)
{
  G_ASSERT(tex);
  int offset;
  uint8_t *rgbaData = genmip_commit_source(hdr.w * hdr.h * sizeof(uint32_t), offset);
  if (rgbaData == NULL)
    return false;
  struct ReleasePhysMem
  {
    ~ReleasePhysMem() { genmip_release_phys_mem(); }
  } genmip_auto_release_phys_mem;

  if (!crd.readExact(rgbaData, hdr.getSurfaceSz(0)))
    return false;
  const float gamma = crd.readReal();
  f.read_parameters(crd, false);

  float *floatData = 0;
  void *kaiserTemp = 0;
  if (force_gamma_space)
    genmip_commit_phys_mem_char_rgba(offset, hdr.w, hdr.h, f.needTemp(), kaiserTemp);
  else
  {
    if (!genmip_commit_phys_mem_float_rgba(offset, hdr.w, hdr.h, f.needTemp(), floatData, kaiserTemp))
      genmip_commit_phys_mem_char_rgba(offset, hdr.w, hdr.h, f.needTemp(), kaiserTemp);
  }
  if (!kaiserTemp)
    return false;

  f.setTemp(kaiserTemp);

  uint32_t w = hdr.w;
  uint32_t h = hdr.h;

  if (!force_gamma_space)
    imagefunctions::exponentiate4_c(rgbaData, floatData, w * h, gamma);

  for (int i = 0; i < skip_levels; i++)
  {
    w >>= 1;
    h >>= 1;
    f.downsample4x(rgbaData, rgbaData, w, h);
  }

  bool res = true;
  for (int i = 0; i < levels && res; i++)
  {
    if (!force_gamma_space)
      if (skip_levels || i) // split for pvs studio
        imagefunctions::convert_to_linear_simda(floatData, rgbaData, (w && h) ? w * h : w + h, 1.0f / gamma);

    if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(tex, start_level + i, dest_slice))
    {
      char *dst = d3d::get_update_buffer_addr_for_write(rub);
      int dst_pitch = d3d::get_update_buffer_pitch(rub), src_pitch = w * 4;
      if (!dst)
        res = false;
      else if (dst_pitch == src_pitch)
        memcpy(dst, rgbaData, h * src_pitch);
      else if (dst_pitch > src_pitch)
      {
        for (const uint8_t *src = rgbaData, *src_e = src + h * src_pitch; src < src_e; src += src_pitch, dst += dst_pitch)
          memcpy(dst, src, src_pitch);
        dst -= dst_pitch * h;
      }
      else if (dst_pitch)
      {
        TextureInfo ti;
        tex->getinfo(ti, start_level + i);
        logerr("tex(%s) mip=%dx%d src=%dx%d lockimg(%d) gives dst_pitch=%d < src_pitch=%d", tex->getTexName(), ti.w, ti.h, w, h,
          start_level, dst_pitch, src_pitch);
        d3d::release_update_buffer(rub);
        res = false;
        break;
      }
      GENMIP_ON_MIP_DONE(tex, dst, w * 4, dst_pitch, h, i + 1 == levels, w, h);
      if (!d3d::update_texture_and_release_update_buffer(rub))
        res = false;
      d3d::release_update_buffer(rub);
    }
    else
      res = false;

    if (i < levels - 1 && res)
    {
      w >>= 1;
      h >>= 1;
      if (!force_gamma_space)
        f.downsample4x(floatData, floatData, w, h);
      else
        f.downsample4x(rgbaData, rgbaData, w, h);
      w = max(w, 1U);
      h = max(h, 1U);
    }
  }

  return res;
}
