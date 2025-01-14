// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_fixedBlockAllocator.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_resetDevice.h>
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
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_memIo.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_miscApi.h>
#include <3d/ddsFormat.h>
#include <generic/dag_tab.h>
#include <workCycle/dag_workCycle.h>
#include <util/dag_watchdog.h>
#include <math/dag_mathUtils.h>
#include <validateUpdateSubRegion.h>
#include <validation.h>

#include "driver.h"
#include <d3d9types.h>
#if HAS_NVAPI
#include <nvapi.h>
#endif


#if 0
#define VERBOSE_DEBUG debug
#else
#define VERBOSE_DEBUG(...)
#endif

namespace drv3d_dx11
{
struct LockingFixedBlockAllocator : protected FixedBlockAllocator
{
  void *allocateOne()
  {
    cs.lock();
    void *ret = FixedBlockAllocator::allocateOneBlock();
    cs.unlock();
    return ret;
  }
  bool freeOne(void *b)
  {
    cs.lock();
    bool ret = FixedBlockAllocator::freeOneBlock(b);
    cs.unlock();
    return ret;
  }
  LockingFixedBlockAllocator(uint32_t block_size, uint16_t initial_blocks) : FixedBlockAllocator(block_size, initial_blocks) {}

protected:
  WinCritSec cs;
};
static LockingFixedBlockAllocator texturesMemory(sizeof(BaseTex), 1024);
BaseTex *BaseTex::create_tex(uint32_t cflg_, int type_)
{
  void *ret = texturesMemory.allocateOne();
  if (!ret)
    return nullptr;
  return new (ret, _NEW_INPLACE) BaseTex(cflg_, type_);
}
void BaseTex::destroy_tex(BaseTex *t)
{
  if (!t)
    return;
  t->~BaseTex();
  G_VERIFY(texturesMemory.freeOne(t));
}

static inline BaseTex *getbasetex(BaseTexture *t) { return (BaseTex *)t; }

extern const char *dxgi_format_to_string(DXGI_FORMAT format);
extern DXGI_FORMAT dxgi_format_for_create(DXGI_FORMAT fmt);
extern DXGI_FORMAT dxgi_format_for_res(DXGI_FORMAT fmt, uint32_t cflg);
extern DXGI_FORMAT dxgi_format_for_res_depth(DXGI_FORMAT fmt, uint32_t cflg, bool read_stencil);
extern void set_tex_params(BaseTex *tex, int w, int h, int d, uint32_t flg, int levels, const char *stat_name);
extern bool create_tex2d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t levels, bool cube,
  D3D11_SUBRESOURCE_DATA *initial_data, int array_size = 1);
extern void clear_texture(BaseTex *tex);
extern bool mark_dirty_texture_in_states(const BaseTexture *tex);
extern bool create_tex3d(BaseTex::D3DTextures &tex, BaseTex *bt_in, uint32_t w, uint32_t h, uint32_t d, uint32_t flg, uint32_t levels,
  D3D11_SUBRESOURCE_DATA *initial_data);

extern bool remove_texture_from_states(BaseTexture *tex, bool recreate, TextureFetchState &state);
extern bool remove_texture_from_states(BaseTexture *tex, bool recreate);

extern uint32_t get_texture_res_size(const BaseTex *bt, int skip = 0);

extern HRESULT map_without_context_blocking(ID3D11Resource *resource, UINT subresource, D3D11_MAP maptype, bool nosyslock,
  D3D11_MAPPED_SUBRESOURCE *mapped);

template <class T>
inline void set_res_view_mip(T &d, uint32_t mip_level, int32_t mip_count)
{
  d.MostDetailedMip = mip_level;
  d.MipLevels = mip_count;
}

// mip_count == -1 indicates all the mipmaps from MostDetailedMip
static bool createResView(TextureView &tv, BaseTex *tex, uint32_t face, uint32_t mip_level, int32_t mip_count)
{
  G_ASSERT(tex);
  if (!tex->tex.tex2D) // May become NULL later if recreated from another thread.
  {
    if (device_is_lost == S_OK)
      debug("DX11: createResView for NULL texture "
            "(cflg=0x%08X, lockFlags=0x%08X, delayedCreate=%d, name='%s')",
        tex->cflg, tex->lockFlags, (int)tex->delayedCreate, tex->getTexName());

    tv.view = NULL;
    return true; // NULL view is valid for NULL texture.
  }


  HRESULT hr = S_OK;
  const int type = tex->type;
  // G_ASSERT(type != RES3D_VOLTEX);
  G_ASSERT((int)mip_level >= 0);

  D3D11_SHADER_RESOURCE_VIEW_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  if (is_depth_format_flg(tex->cflg))
  {
    desc.Format = dxgi_format_for_res_depth(tex->format, tex->cflg, tex->read_stencil);
    mip_level = 0;
  }
  else
    desc.Format = dxgi_format_for_res(tex->format, tex->cflg);

  if ((tex->cflg & TEXCF_SAMPLECOUNT_MASK) == 0 || tex->tex.resolvedTex)
  {
    if (type == RES3D_TEX)
    {
      D3D11_TEX2D_SRV d;
      set_res_view_mip(d, mip_level, mip_count);
      desc.Texture2D = d;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      G_ASSERT(face == 0);
    }
    else if (type == RES3D_CUBETEX)
    {
      D3D11_TEXCUBE_SRV d;
      set_res_view_mip(d, mip_level, mip_count);
      desc.TextureCube = d;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    }
    else if (type == RES3D_VOLTEX)
    {
      D3D11_TEX3D_SRV d;
      set_res_view_mip(d, mip_level, mip_count);
      desc.Texture3D = d;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
      G_ASSERT(face == 0);
    }
    else if (type == RES3D_ARRTEX)
    {
      if (tex->cube_array)
      {
        D3D11_TEXCUBE_ARRAY_SRV d;
        set_res_view_mip(d, mip_level, mip_count);
        d.First2DArrayFace = face;
        d.NumCubes = (tex->depth - face) / 6;
        desc.TextureCubeArray = d;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
      }
      else
      {
        D3D11_TEX2D_ARRAY_SRV d;
        set_res_view_mip(d, mip_level, mip_count);
        d.FirstArraySlice = face;
        d.ArraySize = tex->depth - face;
        desc.Texture2DArray = d;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
      }
    }
  }
  else
  {
    if (type == RES3D_TEX)
    {
      G_ASSERT(mip_level == 0);
      // D3D11_TEX2DMS_SRV d; //dummy
      //        desc.Texture2DMS = d;
      desc.Texture2D.MostDetailedMip = 0;
      desc.Texture2D.MipLevels = 1;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
      G_ASSERT(face == 0 && mip_level == 0 && mip_count == -1);
    }
    else if (type == RES3D_CUBETEX)
    {
      G_ASSERT(mip_level == 0);
      D3D11_TEXCUBE_SRV d;
      d.MostDetailedMip = mip_level;
      d.MipLevels = mip_count;
      desc.TextureCube = d;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
      G_ASSERT(mip_level == 0 && mip_count == -1);
    }
    else if (type == RES3D_ARRTEX)
    {
      D3D11_TEX2D_ARRAY_SRV d;
      d.MostDetailedMip = mip_level;
      d.MipLevels = mip_count;
      d.FirstArraySlice = face;
      d.ArraySize = tex->depth - face;
      desc.Texture2DArray = d;
      desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
      G_ASSERT(mip_level == 0 && mip_count == -1);
    }
    else if (type == RES3D_VOLTEX)
    {
      G_ASSERT(0); // impossible
    }
  }

  bool nullDesc = (face == 0 && mip_level == 0 && mip_count == -1) && tex->format != DXGI_FORMAT_B8G8R8A8_TYPELESS &&
                  tex->format != DXGI_FORMAT_R8G8B8A8_TYPELESS;

  if (desc.Format != dxgi_format_for_create(tex->format))
  {
    nullDesc = false;
  }

  if (device_is_lost == S_OK)
  {
    if (!tex->tex.tex2D) // -V547 -V560 Already under ResAutoLock in getResView.
    {
      debug("DX11: createResView for NULL texture "
            "(cflg=0x%08X, lockFlags=0x%08X, delayedCreate=%d, name='%s')",
        tex->cflg, tex->lockFlags, (int)tex->delayedCreate, tex->getTexName());

      tv.view = NULL;
      return true; // NULL view is valid for NULL texture. NULL view should not be saved in viewCache.
    }

    hr = dx_device->CreateShaderResourceView(tex->tex.resolvedTex ? tex->tex.resolvedTex : tex->tex.tex2D, nullDesc ? NULL : &desc,
      &tv.shaderResView);

    if (FAILED(hr))
    {
      debug("CreateShaderResourceView: ITex=%p %p, nullDesc = %d, cflg=0x%08X, lockFlags=0x%08X, delayedCreate=%d, name='%s')"
            " face=%d mip_level=%d mip_count = %d texture %dx%dx, levels=%d resViewformat=%s (0x%X) nullDesc = %d tex format = %s",
        tex->tex.tex2D, tex->tex.resolvedTex, nullDesc, tex->cflg, tex->lockFlags, (int)tex->delayedCreate, tex->getTexName(), face,
        mip_level, mip_count, tex->width, tex->height, tex->mipLevels, dxgi_format_to_string(desc.Format), desc.Format, int(nullDesc),
        dxgi_format_to_string(tex->format));
      if (drv3d_dx11::device_should_reset(hr, "CreateShaderResourceView"))
      {
        tv.view = NULL;
        return false;
      }
    }
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
    if (tv.shaderResView)
      tv.shaderResView->SetPrivateData(WKPDID_D3DDebugObjectName, i_strlen(tex->getResName()), tex->getResName());
#endif

    DXFATAL(hr, "CreateShaderResourceView");
  }

  return SUCCEEDED(hr);
}
static bool createTexView(TextureView &tv, BaseTex *tex, uint32_t face, uint32_t mip_level, uint32_t mip_count, uint32_t cflg)
{
  const int type = tex->type;
  HRESULT hr = S_OK;
  // G_ASSERT(type != RES3D_VOLTEX);

  if (!is_depth_format_flg(cflg))
  {
    D3D11_RENDER_TARGET_VIEW_DESC desc;
    desc.Format = tex->format;

    if (tex->format == DXGI_FORMAT_B8G8R8A8_TYPELESS)
    {
      desc.Format = (cflg & TEXCF_SRGBWRITE) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
    }
    else if (tex->format == DXGI_FORMAT_R8G8B8A8_TYPELESS)
    {
      desc.Format = (cflg & TEXCF_SRGBWRITE) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    if ((cflg & TEXCF_SAMPLECOUNT_MASK) == 0)
    {
      if (type == RES3D_TEX)
      {
        D3D11_TEX2D_RTV d;
        d.MipSlice = mip_level;
        desc.Texture2D = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        G_ASSERT(face == 0 && mip_count == 1);
      }
      else if (type == RES3D_CUBETEX)
      {
        D3D11_TEX2D_ARRAY_RTV d;
        d.MipSlice = mip_level;
        d.FirstArraySlice = face < d3d::RENDER_TO_WHOLE_ARRAY ? face : 0;
        d.ArraySize = face < d3d::RENDER_TO_WHOLE_ARRAY ? 1 : 6;
        desc.Texture2DArray = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        G_ASSERT(mip_count == 1);
      }
      else if (type == RES3D_ARRTEX)
      {
        D3D11_TEX2D_ARRAY_RTV d;
        d.MipSlice = mip_level;
        d.FirstArraySlice = face < d3d::RENDER_TO_WHOLE_ARRAY ? face : 0;
        d.ArraySize = face < d3d::RENDER_TO_WHOLE_ARRAY ? 1 : tex->depth;
        desc.Texture2DArray = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        G_ASSERT(mip_count == 1);
      }
      else if (type == RES3D_VOLTEX)
      {
        D3D11_TEX3D_RTV d;
        d.MipSlice = mip_level;
        d.FirstWSlice = face < d3d::RENDER_TO_WHOLE_ARRAY ? face : 0;
        d.WSize = face < d3d::RENDER_TO_WHOLE_ARRAY ? 1 : -1;
        desc.Texture3D = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
        G_ASSERT(mip_count == 1);
      }
    }
    else
    {
      if (type == RES3D_TEX)
      {
        //          D3D11_TEX2DMS_RTV d; //dummy
        //          desc.Texture2DMS = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        G_ASSERT(face == 0 && mip_level == 0 && mip_count == 1);
      }
      else if (type == RES3D_CUBETEX)
      {
        D3D11_TEX2DMS_ARRAY_RTV d;
        d.FirstArraySlice = face;
        d.ArraySize = 1;
        desc.Texture2DMSArray = d;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
        G_ASSERT(mip_level == 0 && mip_count == 1);
      }
    }
    // same as tex2D/tex3D

    if (device_is_lost == S_OK)
    {
      hr = dx_device->CreateRenderTargetView(tex->tex.tex2D, &desc, &tv.targetView);
      if (!drv3d_dx11::device_should_reset(hr, "CreateRenderTargetView"))
      {
        DXFATAL(hr, "CreateRenderTargetView");
      }
    }
  }
  else
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    switch (tex->format)
    {
      case DXGI_FORMAT_R32_TYPELESS: desc.Format = DXGI_FORMAT_D32_FLOAT; break;
      case DXGI_FORMAT_R24G8_TYPELESS: desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
      case DXGI_FORMAT_R16_TYPELESS: desc.Format = DXGI_FORMAT_D16_UNORM; break;
      case DXGI_FORMAT_R32G8X24_TYPELESS: desc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; break;
      default: desc.Format = tex->format; break;
    }
    desc.Flags = (cflg & TEXCF_SRGBWRITE) ? (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL) : 0;
    if (!is_stencil_format_flg(cflg))
      desc.Flags &= ~D3D11_DSV_READ_ONLY_STENCIL;
    if (type == RES3D_ARRTEX)
    {
      G_ASSERT(mip_level == 0 && mip_count == 1);
      desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
      desc.Texture2DArray.MipSlice = 0;
      desc.Texture2DArray.FirstArraySlice = face;
      desc.Texture2DArray.ArraySize = 1;
    }
    else if (type == RES3D_CUBETEX)
    {
      G_ASSERT(mip_level == 0 && mip_count == 1);
      desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
      desc.Texture2DArray.MipSlice = 0;
      desc.Texture2DArray.FirstArraySlice = face;
      desc.Texture2DArray.ArraySize = 1;
    }
    else
    {
      G_ASSERT(face == 0 && mip_level == 0 && mip_count == 1);
      if ((tex->cflg & TEXCF_SAMPLECOUNT_MASK) == 0)
      {
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
      }
      else
      {
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
      }
    }

    if (device_is_lost == S_OK)
    {
      hr = dx_device->CreateDepthStencilView(tex->tex.tex2D, &desc, &tv.depthStencilView);
      if (!drv3d_dx11::device_should_reset(hr, "CreateDepthStencilView"))
      {
        DXFATAL(hr, "CreateDepthStencilView");
      }
    }
  }
  return SUCCEEDED(hr);
}

BaseTex::BaseTex(uint32_t cflg_, int type_) : BaseTextureImpl(cflg_, type_)
{
  modified = 0;
  delayedCreate = 0;
  read_stencil = 0;
  needs_clear = 0;
  cube_array = 0;
  wasCopiedToStage = 0;
  dirtyRt = 0;
  wasUsed = 0;

  lockMsr.pData = nullptr;
  lockMsr.RowPitch = 0;
  lockMsr.DepthPitch = 0;

  ddsxNotLoaded = 0;
  preallocBeforeLoad = false;
  rld = NULL;

  if (type == RES3D_CUBETEX)
  {
    addrU = addrV = TEXADDR_CLAMP;
  }
}

BaseTex::~BaseTex() { BaseTex::setReloadCallback(NULL); }

bool BaseTex::setReloadCallback(BaseTexture::IReloadData *_rld)
{
  if (rld && rld != _rld)
    rld->destroySelf();
  rld = _rld;
  return true;
}

int BaseTex::ressize() const { return isStub() ? 0 : get_texture_res_size(this); };

const char *BaseTex::strLabel() const { return "baseTex"; }

void BaseTex::release() { releaseTex(false); }

void BaseTex::resolve(ID3D11Resource *dst, DXGI_FORMAT dst_format)
{
  G_ASSERT(cflg & (TEXCF_SAMPLECOUNT_MASK));

  switch (dst_format)
  {
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      dst_format = (cflg & TEXCF_SRGBWRITE) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
      break;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      dst_format = (cflg & TEXCF_SRGBWRITE) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
      break;
  }

  ContextAutoLock contextLock;
  dx_context->ResolveSubresource(dst, 0, tex.texRes, 0, dst_format); // Skip disable_conditional_render_unsafe here, it may be intended
                                                                     // to be skipped.
}

void validate_copy_destination_flags(BaseTex *self, const char *what, int other_clfg)
{
  // we allow this for now, so other engine / driver components are still working without tripping this validation.
  // currently resource update buffer basic implementation uses texture to texture copy with a sys mem source.
  // (TODO we may be able to relax rules for copy dest in general)
  if (0 != (other_clfg & TEXCF_SYSMEM))
  {
    return;
  }
  if (0 != (self->cflg & (TEXCF_UPDATE_DESTINATION | TEXCF_RTARGET | TEXCF_UNORDERED)))
  {
    return;
  }
  D3D_ERROR(
    "used %s method on texture <%s> that does not support it, the texture needs either TEXCF_UPDATE_DESTINATION, TEXCF_RTARGET "
    "or TEXCF_UNORDERED create flags specified",
    what, self->getResName());
}

int BaseTex::update(BaseTexture *base_src)
{
  BaseTex *src = (BaseTex *)base_src;
  if (src->needs_clear)
    src->clear();
  if (needs_clear)
    clear();
  if (src->tex.texRes && tex.texRes)
  {
    validate_copy_destination_flags(this, "update", src->cflg);

#if DAGOR_DBGLEVEL > 0
    src->wasUsed = 1;
#endif
    ResAutoLock resLock;
    if (!src->tex.texRes || !tex.texRes) // -V560 can be changed from different thread
      return 0;

    // arr rect
    if (src->cflg & TEXCF_SAMPLECOUNT_MASK)
      src->resolve(tex.texRes, format);
    else
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: BaseTex::update uses CopyResource in an active generic render pass");
      dx_context->CopyResource(tex.texRes, src->tex.texRes);
    }
    return 1;
  }
  else
    return 0;
}


int BaseTex::updateSubRegionImpl(BaseTexture *base_src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h,
  int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z)
{
  BaseTex *src = (BaseTex *)base_src;
  if (needs_clear)
    clear();
  if (src->needs_clear)
    src->clear();
  ResAutoLock resLock;

  if (src->tex.texRes && tex.texRes)
  {
    if (is_bc_texformat(src->cflg))
    {
      src_x = src_x & ~3;
      src_y = src_y & ~3;
      src_w = ((src_w + 3 + src_x) & ~3) - src_x;
      src_h = ((src_h + 3 + src_y) & ~3) - src_y;
    }
    if (is_bc_texformat(cflg))
    {
      dest_x = dest_x & ~3;
      dest_y = dest_y & ~3;
    }

    if (!validate_update_sub_region_params(base_src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, this, dest_subres_idx,
          dest_x, dest_y, dest_z))
      return 0;

    D3D11_BOX box;
    box.left = src_x;
    box.top = src_y;
    box.front = src_z;
    box.right = src_x + src_w;
    box.bottom = src_y + src_h;
    box.back = src_z + src_d;

    ddsxNotLoaded = 0;
#if DAGOR_DBGLEVEL > 0
    src->wasUsed = 1;
#endif
    {
      /*
        If you use CopySubresourceRegion with a depth-stencil buffer or a multisampled resource, you must copy the
        whole subresource. In this situation, you must pass 0 to the DstX, DstY, and DstZ parameters and NULL to the
        pSrcBox parameter. In addition, source and destination resources, which are represented by the pSrcResource
        and pDstResource parameters, should have identical sample count values.
      */

      auto isDepthOrMS = [](const TextureInfo &info) {
        return is_depth_format_flg(info.cflg) || !!(info.cflg & TEXCF_SAMPLECOUNT_MASK);
      };

      TextureInfo srcInfo, dstInfo;
      base_src->getinfo(srcInfo, src_subres_idx);
      getinfo(dstInfo, dest_subres_idx);
      bool srcIsDepthOrMS = isDepthOrMS(srcInfo);
      bool dstIsDepthOrMS = isDepthOrMS(dstInfo);

      const D3D11_BOX *boxInput = &box;
      if (srcIsDepthOrMS || dstIsDepthOrMS)
      {
        G_ASSERT(dest_x == 0 && dest_y == 0 && dest_z == 0);
        G_ASSERT(box.left == 0 && box.top == 0 && box.front == 0);
        G_ASSERT(box.right == srcInfo.w && box.bottom == srcInfo.h && box.back == 1);

        boxInput = nullptr;
      }

      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: BaseTex::updateSubRegionImpl uses CopySubresourceRegion inside a generic render pass");
      dx_context->CopySubresourceRegion(tex.texRes, dest_subres_idx, dest_x, dest_y, dest_z, src->tex.texRes, src_subres_idx,
        boxInput);
    }
    return 1;
  }
  else
    D3D_ERROR("%s(tex.texRes=%p, src->tex.texRes=%p)", __FUNCTION__, tex.texRes, src->tex.texRes);


  return 0;
}

int BaseTex::updateSubRegion(BaseTexture *base_src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h,
  int src_d, int dest_subres_idx, int dest_x, int dest_y, int dest_z)
{
  validate_copy_destination_flags(this, "updateSubRegion", ((BaseTex *)base_src)->cflg);

  return updateSubRegionImpl(base_src, src_subres_idx, src_x, src_y, src_z, src_w, src_h, src_d, dest_subres_idx, dest_x, dest_y,
    dest_z);
}

BaseTexture *BaseTex::downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
  unsigned level_offset)
{
  auto rep = makeTmpTexResCopy(new_width, new_height, new_depth, new_mips, false);
  if (!rep)
    return nullptr;

  TextureInfo selfInfo;
  getinfo(selfInfo, 0);

  unsigned sourceLevel = max<unsigned>(level_offset, start_src_level);
  unsigned sourceLevelEnd = min<unsigned>(selfInfo.mipLevels, new_mips + level_offset);
  rep->texmiplevel(sourceLevel - level_offset, sourceLevelEnd - level_offset - 1);
  for (; sourceLevel < sourceLevelEnd; sourceLevel++)
  {
    for (int s = 0; s < selfInfo.a; s++)
    {
      ((BaseTex *)rep)
        ->updateSubRegionImpl(this, calcSubResIdx(sourceLevel, s, selfInfo.mipLevels), 0, 0, 0, max<int>(selfInfo.w >> sourceLevel, 1),
          max<int>(selfInfo.h >> sourceLevel, 1), max<int>(selfInfo.d >> sourceLevel, selfInfo.a),
          calcSubResIdx(sourceLevel - level_offset, s, new_mips), 0, 0, 0);
    }
  }

  return rep;
}

BaseTexture *BaseTex::upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
  unsigned level_offset)
{
  auto rep = makeTmpTexResCopy(new_width, new_height, new_depth, new_mips, false);
  if (!rep)
    return nullptr;

  TextureInfo selfInfo;
  getinfo(selfInfo, 0);

  unsigned destinationLevel = level_offset + start_src_level;
  unsigned destinationLevelEnd = min<unsigned>(selfInfo.mipLevels + level_offset, new_mips);
  rep->texmiplevel(destinationLevel, destinationLevelEnd - 1);
  for (; destinationLevel < destinationLevelEnd; destinationLevel++)
  {
    for (int s = 0; s < selfInfo.a; s++)
    {
      ((BaseTex *)rep)
        ->updateSubRegionImpl(this, calcSubResIdx(destinationLevel - level_offset, s, selfInfo.mipLevels), 0, 0, 0,
          max<int>(new_width >> destinationLevel, 1), max<int>(new_height >> destinationLevel, 1),
          max<int>(new_depth >> destinationLevel, selfInfo.a), calcSubResIdx(destinationLevel, s, new_mips), 0, 0, 0);
    }
  }

  return rep;
}

void BaseTex::clear()
{
  ResAutoLock resLock;
  if (!needs_clear)
    return;
  clear_texture(this);
  needs_clear = false;
}

void BaseTex::destroyObject()
{
  releaseTex(false);
  destroy_tex(this);
}

int BaseTex::texaddrImpl(int a)
{
  addrU = addrV = addrW = a;
  modified = 0b1111;
  return 1;
}

int BaseTex::texaddruImpl(int a)
{
  addrU = a;
  modified = 0b1111;
  return 1;
}

int BaseTex::texaddrvImpl(int a)
{
  addrV = a;
  modified = 0b1111;
  return 1;
}

int BaseTex::texaddrwImpl(int a)
{
  addrW = a;
  modified = 0b1111;
  return 1;
}

int BaseTex::texbordercolorImpl(E3DCOLOR c)
{
  borderColor = c;
  modified = 0b1111;
  return 1;
}

int BaseTex::texfilterImpl(int m)
{
  texFilter = m;
  modified = 0b1111;
  return 1;
}

int BaseTex::texmipmapImpl(int m)
{
  mipFilter = m;
  modified = 0b1111;
  return 1;
}

int BaseTex::texlodImpl(float mipmaplod)
{
  lodBias = mipmaplod;
  modified = 0b1111;
  return 1;
}

int BaseTex::texmiplevel(int minlev, int maxlev)
{
  const int topLevel = mipLevels - 1;
  if (minlev >= (int)mipLevels)
  {
    logerr("DX11: texmiplevel error: minlev=%d < mipLevel=%d failed in %s", minlev, (int)mipLevels, getTexName());
    minlev = topLevel;
  }
  if (maxlev >= (int)mipLevels)
  {
    logerr("DX11: texmiplevel error: maxlev=%d < mipLevel=%d failed in %s", maxlev, (int)mipLevels, getTexName());
    maxlev = topLevel;
  }
  G_ASSERTF(minlev < 0 || maxlev < 0 || minlev <= maxlev, "%d <= %d failed", minlev, maxlev);

  maxMipLevel = (minlev >= 0) ? minlev : 0;
  minMipLevel = (maxlev >= 0) ? maxlev : topLevel;
  modified = 0b1111;
  return 1;
}

int BaseTex::setAnisotropyImpl(int level)
{
  anisotropyLevel = clamp<int>(level, 1, 16);
  modified = 0b1111;
  return 1;
}

ID3D11View *BaseTex::getRtView(uint32_t face, uint32_t mip_level, uint32_t mip_count, bool srgb_write__depthreadonly)
{
  ResAutoLock resLock;
  if (!tex.texRes)
    return NULL;
  // target is not requested as RT
  G_ASSERT(cflg & TEXCF_RTARGET);

  TextureView tv;
  uint32_t srgb_cflg = cflg;
  if (srgb_write__depthreadonly)
    srgb_cflg |= TEXCF_SRGBWRITE;
  TextureView::ViewType vt = is_depth_format_flg(cflg) ? (srgb_write__depthreadonly ? TextureView::DEPTH_READONLY : TextureView::DEPTH)
                             : (srgb_cflg & TEXCF_SRGBWRITE) ? TextureView::SRGB_TARGET
                                                             : TextureView::NON_SRGB_TARGET;
  TextureView::Key texture_view_key = TextureView::key(vt, face, mip_level, 1);
  TextureView *ptv = viewCache.find(texture_view_key);
  if (ptv == NULL)
  {
    G_VERIFY(true == createTexView(tv, this, face, mip_level, 1, srgb_cflg) || device_is_lost != S_OK);
    if (!tv.view)
      return NULL;

    viewCache.addNoCheck(texture_view_key, tv, 1);
    ptv = &tv;
  }
  return ptv->view;
}

ID3D11ShaderResourceView *BaseTex::getResView(uint32_t face, uint32_t mip_level, uint32_t mip_count)
{
  ResAutoLock resLock;
  if (!tex.texRes)
    return NULL;
  if (isStub())
  {
    BaseTex *tx = getStubTex();
    return tx ? tx->getResView(face, 0, 0) : NULL;
  }

  // check and fixup
  int32_t baseMip = clamp<int32_t>(mip_level, 0, max(0, (int32_t)tex.realMipLevels - 1));
  int32_t mipCount = (int32_t)mip_count;
  if (mipCount <= 0 || baseMip + mipCount > tex.realMipLevels)
    mipCount = tex.realMipLevels - baseMip;

  if ((uint32_t)baseMip != mip_level || (mip_count != 0 && (uint32_t)mipCount != mip_count))
  {
    D3D_ERROR("fixed an invalid getResView call for texture '%s' baseMip=%u -> %d mipCount=%u -> %d minMipLevel=%u maxMipLevel=%u "
              "mipLevels=%u realMipLevels=%u format=%s",
      getTexName(), mip_level, baseMip, mip_count, mipCount, minMipLevel, maxMipLevel, mipLevels, tex.realMipLevels,
      dxgi_format_to_string(format));

    G_ASSERTF(false, "BaseTex::getResView called with invalid parameters, see error log for details");
  }

  if (baseMip + mipCount == tex.realMipLevels)
    mipCount = 0;

  TextureView tv;
  TextureView::Key texture_view_key =
    TextureView::key(read_stencil ? TextureView::STENCIL_SHADER_RES : TextureView::SHADER_RES, face, baseMip, mipCount);

  TextureView *ptv = viewCache.find(texture_view_key);
  if (ptv == NULL)
  {
    G_VERIFY(true == createResView(tv, this, face, baseMip, mipCount ? mipCount : -1) || device_is_lost != S_OK);
    if (!tv.view)
      return NULL;

    viewCache.addNoCheck(texture_view_key, tv, 1);
    ptv = &tv;
  }
  return ptv->shaderResView;
}

ID3D11UnorderedAccessView *BaseTex::getExistingUaView(uint32_t face, uint32_t mip_level, bool as_uint)
{
  ResAutoLock resLock;
  if (!tex.texRes)
    return NULL;
  TextureView tv;
  TextureView::Key texture_view_key = TextureView::key(as_uint ? TextureView::UAV_UINT : TextureView::UAV, face, mip_level, 0);
  TextureView *ptv = viewCache.find(texture_view_key);
  return ptv ? ptv->uaView : nullptr;
}

ID3D11UnorderedAccessView *BaseTex::getUaView(uint32_t face, uint32_t mip_level, bool as_uint)
{
  ResAutoLock resLock;
  if (!tex.texRes)
    return NULL;
  TextureView tv;
  TextureView::Key texture_view_key = TextureView::key(as_uint ? TextureView::UAV_UINT : TextureView::UAV, face, mip_level, 0);
  TextureView *ptv = viewCache.find(texture_view_key);
  if (ptv == NULL)
  {
    D3D11_UNORDERED_ACCESS_VIEW_DESC descView;
    ZeroMemory(&descView, sizeof(descView));
    // D3D11_TEXTURE2D_DESC descBuf;
    // tex.tex2D->GetDesc( &descBuf );

    descView.Format = dxgi_format_for_res(format, cflg & (~(TEXCF_SRGBREAD | TEXCF_SRGBWRITE)));

    if (type == RES3D_TEX)
    {
      descView.Texture2D.MipSlice = mip_level;
      descView.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
      G_ASSERT(face == 0);
    }
    else if (type == RES3D_CUBETEX)
    {
      descView.Texture2DArray.MipSlice = mip_level;
      descView.Texture2DArray.FirstArraySlice = face;
      descView.Texture2DArray.ArraySize = 6 - face;
      descView.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    else if (type == RES3D_VOLTEX)
    {
      descView.Texture3D.MipSlice = mip_level;
      descView.Texture3D.FirstWSlice = 0;
      descView.Texture3D.WSize = depth >> mip_level;
      descView.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
      G_ASSERT(face == 0);
    }
    else if (type == RES3D_ARRTEX)
    {
      descView.Texture2DArray.MipSlice = mip_level;
      descView.Texture2DArray.FirstArraySlice = face;
      descView.Texture2DArray.ArraySize = depth - face;
      descView.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }

    if (as_uint)
    {
      G_ASSERT(dxgi_format_to_bpp(descView.Format) == 32);
      descView.Format = DXGI_FORMAT_R32_UINT;
    }

    HRESULT hr = dx_device->CreateUnorderedAccessView(tex.texRes, &descView, &tv.uaView);
    if (FAILED(hr))
    {
      if (!device_should_reset(hr, "CreateUnorderedAccessView"))
      {
        D3D11_RESOURCE_DIMENSION dim;
        tex.texRes->GetType(&dim);
        switch (dim)
        {
          case D3D11_RESOURCE_DIMENSION_BUFFER:
          {
            D3D11_BUFFER_DESC desc;
            static_cast<ID3D11Buffer *>(tex.texRes)->GetDesc(&desc);
            DAG_FATAL("CreateUnorderedAccessView failed on ID3D11Buffer with %d."
                      " (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
              hr, desc.ByteWidth, desc.Usage, desc.BindFlags, desc.CPUAccessFlags, desc.MiscFlags, desc.StructureByteStride,
              descView.Format, descView.ViewDimension, descView.Buffer.FirstElement, descView.Buffer.NumElements,
              descView.Buffer.Flags);
            break;
          }
          case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
          {
            D3D11_TEXTURE1D_DESC desc;
            static_cast<ID3D11Texture1D *>(tex.texRes)->GetDesc(&desc);
            DAG_FATAL("CreateUnorderedAccessView failed on ID3D11Texture1D with %d."
                      " (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
              hr, desc.Width, desc.MipLevels, desc.ArraySize, desc.Format, desc.Usage, desc.BindFlags, desc.CPUAccessFlags,
              desc.MiscFlags, descView.Format, descView.ViewDimension, descView.Texture1DArray.MipSlice,
              descView.Texture1DArray.FirstArraySlice, descView.Texture1DArray.ArraySize);
            break;
          }
          case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
          {
            D3D11_TEXTURE2D_DESC desc;
            static_cast<ID3D11Texture2D *>(tex.texRes)->GetDesc(&desc);
            DAG_FATAL("CreateUnorderedAccessView failed on ID3D11Texture2D with %d."
                      " (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
              hr, desc.Width, desc.Height, desc.MipLevels, desc.ArraySize, desc.Format, desc.SampleDesc.Count, desc.SampleDesc.Quality,
              desc.Usage, desc.BindFlags, desc.CPUAccessFlags, desc.MiscFlags, descView.Format, descView.ViewDimension,
              descView.Texture2DArray.MipSlice, descView.Texture2DArray.FirstArraySlice, descView.Texture2DArray.ArraySize);
            break;
          }
          case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
          {
            D3D11_TEXTURE3D_DESC desc;
            static_cast<ID3D11Texture3D *>(tex.texRes)->GetDesc(&desc);
            DAG_FATAL("CreateUnorderedAccessView failed on ID3D11Texture3D with %d."
                      " (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
              hr, desc.Width, desc.Height, desc.Depth, desc.MipLevels, desc.Format, desc.Usage, desc.BindFlags, desc.CPUAccessFlags,
              desc.MiscFlags, descView.Format, descView.ViewDimension, descView.Texture3D.MipSlice, descView.Texture3D.FirstWSlice,
              descView.Texture3D.WSize);
            break;
          }
          default: DXFATAL(hr, "CreateUnorderedAccessView");
        }
      }
      return NULL;
    }

    viewCache.addNoCheck(texture_view_key, tv);
    ptv = &tv;
  }

  return ptv->uaView;
}

BaseTexture *BaseTex::makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex)
{
  if (type != RES3D_ARRTEX && type != RES3D_VOLTEX && type != RES3D_CUBEARRTEX)
    d = 1;
  BaseTex *clonedTex = BaseTex::create_tex(cflg | (staging_tex ? TEXCF_SYSMEM : 0), type);
  if (!clonedTex)
    return nullptr;
  if (!staging_tex)
    clonedTex->tidXored = tidXored, clonedTex->stubTexIdx = stubTexIdx;

  clonedTex->key = key;
  set_tex_params(clonedTex, w, h, d, clonedTex->cflg, l, String::mk_str_cat(staging_tex ? "stg:" : "tmp:", getTexName()));
  clonedTex->preallocBeforeLoad = clonedTex->delayedCreate = true;

  if (!clonedTex->allocateTex())
    del_d3dres(clonedTex);
  return clonedTex;
}
void BaseTex::replaceTexResObject(BaseTexture *&other_tex)
{
  if (this == other_tex)
    return;

  {
    ResAutoLock resLock;
    BaseTex *other = (BaseTex *)other_tex;

    if (!other->isStub())
      other->tex.setPrivateData(getTexName());

    // swap texture objects
    char tmp[sizeof(tex)];
    memcpy(tmp, &tex, sizeof(tmp));
    memcpy(&tex, &other->tex, sizeof(tmp));
    memcpy(&other->tex, tmp, sizeof(tmp));

    // swap dimensions
    eastl::swap(width, other->width);
    eastl::swap(height, other->height);
    eastl::swap(depth, other->depth);
    eastl::swap(mipLevels, other->mipLevels);
    eastl::swap(key, other->key);

    // swap wasUsed flag
    wasUsed = other->wasUsed;
    other->wasUsed = true;
    if (!other->ddsxNotLoaded)
      ddsxNotLoaded = false;

    // clear view cache to switch to new texture object
    viewCache.clear();
    other->viewCache.clear();
  }
  del_d3dres(other_tex);
}

bool BaseTex::allocateTex()
{
  if (tex.texRes && !isStub())
    return true;
  if (tex.texRes)
    tex.release();

  switch (type)
  {
    case RES3D_VOLTEX: return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, nullptr);
    case RES3D_TEX:
    case RES3D_CUBETEX: return create_tex2d(tex, this, width, height, mipLevels, type == RES3D_CUBETEX, nullptr, 1);
    case RES3D_ARRTEX: return create_tex2d(tex, this, width, height, mipLevels, cube_array, nullptr, depth);
  }
  return false;
}
void BaseTex::discardTex()
{
  if (stubTexIdx >= 0 && dx_res_cs.tryLock())
  {
    releaseTex(true);
    recreate();
    dx_res_cs.unlock();
  }
}

bool BaseTex::releaseTex(bool recreate_after)
{
  if (type == RES3D_ARRTEX)
  {
    G_ASSERT(lockFlags == 0); // locked?
  }
  else if (type == RES3D_TEX || type == RES3D_CUBETEX)
  {
    G_ASSERTF((lockFlags & ~(TEX_COPIED | TEXLOCK_COPY_STAGING)) == 0, "texture <%s>", getResName()); // locked?
  }

  ResAutoLock resLock;
  bool found;
  if (recreate_after)
    found = mark_dirty_texture_in_states(this);
  else
    found = remove_texture_from_states(this, recreate_after);

  viewCache.clear();
  //   if (!(cflg & TEXCF_RTARGET) || !releasePreallocatedRenderTarget(tex))
  if (tex.texRes && !isStub())
    TEXQL_ON_RELEASE(this);
  tex.release();
  return found;
}

int BaseTex::recreate()
{
  ResAutoLock resLock;
  ddsxNotLoaded = 0;

  if (type == RES3D_TEX)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), width, height, "rt|dyn");
      return create_tex2d(tex, this, width, height, mipLevels, false, NULL, 1);
    }

    if (!preallocBeforeLoad)
    {
      debug("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), width, height, "empty");
      return create_tex2d(tex, this, width, height, mipLevels, false, NULL, 1);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      if (BaseTex *tx = getStubTex())
      {
        tex.texRes = tx->tex.texRes;
        tex.texRes->AddRef();
      }
      else
        tex.texRes = nullptr;
      return 1;
    }
    return create_tex2d(tex, this, 4, 4, 1, false, NULL, 1);
  }

  if (type == RES3D_CUBETEX)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), width, height, "rt|dyn");
      return create_tex2d(tex, this, width, height, mipLevels, true, NULL, 1);
    }

    if (!preallocBeforeLoad)
    {
      debug("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), width, height, "empty");
      return create_tex2d(tex, this, width, height, mipLevels, true, NULL, 1);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      if (BaseTex *tx = getStubTex())
      {
        tex.texRes = tx->tex.texRes;
        tex.texRes->AddRef();
      }
      else
        tex.texRes = nullptr;
      return 1;
    }
    return create_tex2d(tex, this, 4, 4, 1, true, NULL, 1);
  }

  if (type == RES3D_VOLTEX)
  {
    if (cflg & (TEXCF_RTARGET | TEXCF_DYNAMIC))
    {
      VERBOSE_DEBUG("%s <%s> recreate %dx%dx%d (%s)", strLabel(), getResName(), width, height, depth, "rt|dyn");
      return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, NULL);
    }

    if (!preallocBeforeLoad)
    {
      debug("%s <%s> recreate %dx%dx%d (%s)", strLabel(), getResName(), width, height, depth, "empty");
      return create_tex3d(tex, this, width, height, depth, cflg, mipLevels, NULL);
    }

    delayedCreate = true;
    VERBOSE_DEBUG("%s <%s> recreate %dx%d (%s)", strLabel(), getResName(), 4, 4, "placeholder");
    if (stubTexIdx >= 0)
    {
      if (BaseTex *tx = getStubTex())
      {
        tex.texRes = tx->tex.texRes;
        tex.texRes->AddRef();
      }
      else
        tex.texRes = nullptr;
      return 1;
    }
    return create_tex3d(tex, this, 4, 4, 1, cflg, 1, NULL);
  }

  if (type == RES3D_ARRTEX)
  {
    VERBOSE_DEBUG("%s <%s> recreate %dx%dx%d (%s)", strLabel(), getResName(), width, height, depth, "rt|dyn");
    return create_tex2d(tex, this, width, height, mipLevels, cube_array, NULL, depth);
  }

  return 0;
}


static const int GUARD = 64 * 1024;

int BaseTex::lockimg(void **p, int &stride, int lev, unsigned flags)
{
  if (device_is_lost != S_OK)
  {
    D3D_ERROR("lockimg during reset");
    if (p)
      *p = NULL;
    return 0;
  }

  if (isStub())
  {
    if (p)
      *p = NULL;
    stride = 0;
    return 0;
  }

  if (!tex.tex2D && (flags & TEXLOCK_WRITE) && !(flags & TEXLOCK_READ))
    if (!create_tex2d(tex, this, width, height, mipLevels, false, NULL))
    {
      D3D_ERROR("failed to auto-create tex.tex2D on lockImg");
      return 0;
    }
  stride = 0;

  uint32_t prevFlags = lockFlags;
  if ((cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)) || (flags & TEXLOCK_COPY_STAGING))
  {
#if DAGOR_DBGLEVEL > 0
    wasUsed = 1;
#endif
    bool resourceCopied = (lockFlags == TEX_COPIED);
    lockFlags = 0;
    if (tex.stagingTex2D == NULL)
    {
      resourceCopied = false;
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = mipLevels;
      desc.ArraySize = (type == RES3D_CUBETEX) ? 6 : 1;
      desc.Format = format;
      desc.SampleDesc.Quality = 0;
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;

      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      desc.MiscFlags = (type == RES3D_CUBETEX) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

      if (device_is_lost == S_OK)
      {
        HRESULT hr = dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D);
        RETRY_CREATE_STAGING_TEX(hr, ressize(), dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D));
        if (FAILED(hr))
        {
          D3D_ERROR("can't create staging tex");
          if (!device_should_reset(hr, "CreateTexture2D(rtarget stagingTex2D)"))
          {
            DXFATAL(hr, "CreateTexture2D(stagingTex2D)");
            return 0;
          }
        }
        G_ASSERT(tex.stagingTex2D != NULL);
      }
    }

    if (device_is_lost == S_OK && tex.stagingTex2D)
    {
      if (flags & TEXLOCK_COPY_STAGING)
      {
        ContextAutoLock contextLock;
        disable_conditional_render_unsafe();
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: BaseTex::lockimg uses CopyResource in an active generic render pass");
        dx_context->CopyResource(tex.stagingTex2D, tex.tex2D);
        lockFlags = TEXLOCK_COPY_STAGING;
        return 1;
      }
      if (!resourceCopied || !p)
      {
        ContextAutoLock contextLock;
        disable_conditional_render_unsafe();
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: BaseTex::lockimg uses CopyResource in an active generic render pass");
        dx_context->CopyResource(tex.stagingTex2D, tex.tex2D);
      }
    }
    if (!p)
    {
      lockFlags = TEX_COPIED;
      stride = 0;
      return 1;
    }
  }
  else
  {
    lockFlags = flags;
    if (p == NULL)
    {
      if (flags & TEXLOCK_RWMASK)
      {
        D3D_ERROR("NULL in lockimg");
        return 0;
      }
    }
    else
      *p = 0;

    if (tex.texUsage == D3D11_USAGE_IMMUTABLE)
    {
      D3D_ERROR("can't lock immutable tex");
      return 0;
    }
  }

  if (flags & TEXLOCK_RWMASK)
  {
    if (tex.stagingTex2D == NULL && (cflg & (TEXCF_DYNAMIC | TEXCF_SYSMEM)) == 0)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = mipLevels;
      desc.ArraySize = (type == RES3D_CUBETEX) ? 6 : 1;
      desc.Format = format;
      desc.SampleDesc.Quality = 0;
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;

      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = (type == RES3D_CUBETEX) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

      if (device_is_lost == S_OK)
      {
        HRESULT hr = dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D);
        RETRY_CREATE_STAGING_TEX(hr, ressize(), dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D));
        if (FAILED(hr))
        {
          D3D_ERROR("can't create staging tex");
          if (!device_should_reset(hr, "CreateTexture2D(lock stagingTex2D)"))
          {
            DXFATAL(hr, "CreateTexture2D(stagingTex2D)");
            return 0;
          }
        }
      }
    }

    ID3D11Resource *res = tex.stagingTex2D ? tex.stagingTex2D : tex.tex2D;
    if (!res)
      return 0;

    //    enter_lock_resources();

    mappedSubresource = D3D11CalcSubresource(lev, 0, 1);
    D3D11_MAP mapAccess = D3D11_MAP_READ;
    if (!(cflg & TEXCF_RTARGET))
    {
      if (!tex.stagingTex2D && !(cflg & TEXCF_SYSMEM))
        mapAccess = (lockFlags & TEXLOCK_NOOVERWRITE) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
      else if (!(lockFlags & (TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY | TEXLOCK_UPDATEFROMSYSTEX)))
        mapAccess = D3D11_MAP_READ; // -V1048
      else if ((lockFlags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) == TEXLOCK_WRITE)
        mapAccess = D3D11_MAP_WRITE;
      else
        mapAccess = D3D11_MAP_READ_WRITE;
    }

    if (device_is_lost == S_OK)
      last_hres = map_without_context_blocking(res, mappedSubresource, mapAccess, flags & TEXLOCK_NOSYSLOCK, &lockMsr);
    else
      last_hres = device_is_lost;

    if (FAILED(last_hres))
    {
      if (prevFlags == TEX_COPIED && last_hres == DXGI_ERROR_WAS_STILL_DRAWING)
        lockFlags = prevFlags;
      return 0;
    }

#ifdef GUARD_TEX_LOCK
    G_ASSERT(!debugLockData); //...DX11: Guard locked memory (copy from staging to mem!).
    int debugLockDataSize = GUARD + lockMsr.DepthPitch + GUARD;
    debugLockData = new char[debugLockDataSize];
    memset(debugLockData, 42, debugLockDataSize);
    *p = debugLockData + GUARD;
#else
    *p = lockMsr.pData;
#endif

    stride = lockMsr.RowPitch;
    lockFlags = flags;
    lockedLevel = lev;
    return 1;
  }
  lockFlags = flags;
  return 1;
}

int BaseTex::lockimg(void **p, int &stride, int face, int level, unsigned flags)
{
  if (device_is_lost != S_OK)
  {
    D3D_ERROR("lockimg during reset");
    if (p)
      *p = NULL;
    return 0;
  }

  stride = 0;

  uint32_t prevFlags = lockFlags;
  if ((flags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) != TEXLOCK_READ && p)
  {
    D3D_ERROR("cannot lock cube tex image for write - n/a!");
    return 0;
  }
  if (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED))
  {
#if DAGOR_DBGLEVEL > 0
    wasUsed = 1;
#endif
    bool resourceCopied = (lockFlags == TEX_COPIED);
    lockFlags = 0;
    if (is_depth_format_flg(cflg))
    {
      D3D_ERROR("can't lock depth format");
      return 0;
    }
    if (tex.stagingTex2D == NULL)
    {
      wasCopiedToStage = 0;
      resourceCopied = false;
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = mipLevels;
      desc.ArraySize = (type == RES3D_CUBETEX) ? 6 : 1;
      desc.Format = format;
      desc.SampleDesc.Quality = 0;
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;

      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      desc.MiscFlags = (type == RES3D_CUBETEX) ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

      {
        HRESULT hr = dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D);
        RETRY_CREATE_STAGING_TEX(hr, ressize(), dx_device->CreateTexture2D(&desc, NULL, &tex.stagingTex2D));
        if (FAILED(hr))
        {
          D3D_ERROR("can't create staging tex");
          if (!device_should_reset(hr, "CreateTexture2D(rtarget stagingTex2D)"))
            DXFATAL(hr, "CreateTexture2D(stagingTex2D)");
          if (p)
            *p = NULL;
          return 0;
        }
      }

      G_ASSERT(tex.stagingTex2D != NULL);
    }

    if (flags & TEXLOCK_COPY_STAGING)
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: BaseTex::lockimg uses CopyResource in an active generic render pass");
      dx_context->CopyResource(tex.stagingTex2D, tex.tex2D);
      lockFlags = TEXLOCK_COPY_STAGING;
      return 1;
    }
    if (((!resourceCopied || !p) && !wasCopiedToStage))
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: BaseTex::lockimg uses CopyResource in an active generic render pass");
      dx_context->CopyResource(tex.stagingTex2D, tex.tex2D);
      wasCopiedToStage = 1;
    }
    if (!p)
    {
      lockFlags = TEX_COPIED;
      stride = 0;
      return 1;
    }
    ID3D11Resource *res = tex.stagingTex2D ? tex.stagingTex2D : tex.tex2D;
    G_ASSERT(tex.stagingTex2D);

    //    enter_lock_resources();
    mappedSubresource = D3D11CalcSubresource(level, face, mipLevels);
    D3D11_MAP mapAccess = D3D11_MAP_READ;
    if (!(cflg & TEXCF_RTARGET))
    {
      if (!tex.stagingTex2D && !(cflg & TEXCF_SYSMEM))
        mapAccess = (lockFlags & TEXLOCK_NOOVERWRITE) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
      else if (!(lockFlags & (TEXLOCK_WRITE | TEXLOCK_DELSYSMEMCOPY | TEXLOCK_UPDATEFROMSYSTEX)))
        mapAccess = D3D11_MAP_READ; // -V1048
      else if ((lockFlags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) == TEXLOCK_WRITE)
        mapAccess = D3D11_MAP_WRITE;
      else
        mapAccess = D3D11_MAP_READ_WRITE;
    }

    last_hres = map_without_context_blocking(res, mappedSubresource, mapAccess, flags & TEXLOCK_NOSYSLOCK, &lockMsr);

    if (FAILED(last_hres))
    {
      if (prevFlags == TEX_COPIED && last_hres == DXGI_ERROR_WAS_STILL_DRAWING)
        lockFlags = prevFlags;
      return 0;
    }
    *p = lockMsr.pData;

    stride = lockMsr.RowPitch;
    lockFlags = flags;
    lockedLevel = level;
    return 1;
  }
  else
  {
    D3D_ERROR("cannot lock non rtarget cube tex image - n/a!");
    return 0;
  }
  return 0;
}

int BaseTex::unlockimg()
{
  if (type == RES3D_TEX)
  {
    if (lockFlags == TEXLOCK_COPY_STAGING)
    {
      if (tex.stagingTex2D != NULL && tex.tex2D != NULL)
      {
        ContextAutoLock contextLock;
        disable_conditional_render_unsafe();
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: BaseTex::unlockimg uses CopyResource in an active generic render pass");
        dx_context->CopyResource(tex.tex2D, tex.stagingTex2D);
      }

      return 1;
    }

    if ((cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)) && (lockFlags == TEX_COPIED))
      return 1;

#ifdef GUARD_TEX_LOCK
    G_ASSERT(debugLockData);

    for (unsigned int offset = 0; offset < GUARD; offset++)
    {
      G_ASSERT(debugLockData[offset] == 42);
      G_ASSERT(debugLockData[GUARD + lockMsr.DepthPitch + offset] == 42);
    }

    memcpy(lockMsr.pData, debugLockData + GUARD, lockMsr.DepthPitch);

    delete[] debugLockData;
    debugLockData = NULL;
#endif

    ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
    if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy) && lockMsr.pData && !hdr.compressionType())
    {
      int rpitch = hdr.getSurfacePitch(lockedLevel);
      int h = hdr.getSurfaceScanlines(lockedLevel);
      uint8_t *src = (uint8_t *)lockMsr.pData;
      uint8_t *dest = texCopy.data() + sizeof(ddsx::Header);

      for (int i = 0; i < lockedLevel; i++)
        dest += hdr.getSurfacePitch(i) * hdr.getSurfaceScanlines(i);
      G_ASSERT(dest < texCopy.data() + sizeof(ddsx::Header) + hdr.memSz);

      G_ASSERTF(rpitch <= lockMsr.RowPitch, "%dx%d: tex.pitch=%d copy.pitch=%d, lev=%d", width, height, lockMsr.RowPitch, rpitch,
        lockedLevel);
      for (int y = 0; y < h; y++, dest += rpitch)
        memcpy(dest, src + y * lockMsr.RowPitch, rpitch);
      VERBOSE_DEBUG("%s %dx%d updated DDSx for TEXCF_SYSTEXCOPY", getResName(), hdr.w, hdr.h, data_size(texCopy));
    }


    if (device_is_lost == S_OK)
    {
      if ((lockFlags & TEXLOCK_RWMASK) && (tex.stagingTex2D || tex.tex2D))
      {
        ContextAutoLock contextLock;
        dx_context->Unmap(tex.stagingTex2D ? tex.stagingTex2D : tex.tex2D, mappedSubresource);
        //  leave_lock_resources();
      }

      if (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED))
        ; // no-op
      else if (tex.stagingTex2D != NULL && tex.tex2D != NULL && (lockFlags & (TEXLOCK_RWMASK | TEXLOCK_UPDATEFROMSYSTEX)) != 0 &&
               !(lockFlags & TEXLOCK_DONOTUPDATEON9EXBYDEFAULT))
      {
        ContextAutoLock contextLock;
        disable_conditional_render_unsafe();
#if BLACK_TEXTURES_WORKAROUND
        for (int mimNo = 0; mimNo < mipLevels; mimNo++)
        {
          mappedSubresource = D3D11CalcSubresource(mimNo, 0, mipLevels);
          dx_context->CopySubresourceRegion(tex.tex2D, mappedSubresource, 0, 0, 0, tex.stagingTex2D, mappedSubresource, NULL);
        }
#else
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: BaseTex::unlockimg uses CopyResource in an active generic render pass");
        dx_context->CopyResource(tex.tex2D, tex.stagingTex2D);
#endif
      }
    }

    if ((lockFlags & TEXLOCK_DELSYSMEMCOPY) && tex.stagingTex2D != NULL)
    {
      tex.stagingTex2D->Release();
      tex.stagingTex2D = NULL;
    }

    lockFlags = 0;
    return 1;
  }

  if (type == RES3D_CUBETEX)
  {
    if (lockFlags == TEXLOCK_COPY_STAGING)
    {
      if (tex.stagingTex2D != NULL && tex.tex2D != NULL)
      {
        ContextAutoLock contextLock;
        disable_conditional_render_unsafe();
        VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
          "DX11: BaseTex::unlockimg uses CopyResource in an active generic render pass");
        dx_context->CopyResource(tex.tex2D, tex.stagingTex2D);
      }

      return 1;
    }

    if ((cflg & TEXCF_RTARGET) && (lockFlags == TEX_COPIED))
      return 1;
    if (lockFlags & TEXLOCK_RWMASK)
    {
      ContextAutoLock contextLock;
      dx_context->Unmap(tex.stagingTex2D ? tex.stagingTex2D : tex.tex2D, mappedSubresource);
    }

    lockFlags = 0;
    return 1;
  }

  return 0;
}

int BaseTex::generateMips()
{
  ID3D11ShaderResourceView *resView = getResView(0, 0, 0);
  G_ASSERT(resView);
  if (resView)
  {
    ContextAutoLock lock;
    disable_conditional_render_unsafe();
    dx_context->GenerateMips(resView);
  }
  return 1;
}

int BaseTex::lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags)
{
  if (device_is_lost != S_OK)
  {
    D3D_ERROR("lockbox during reset");
    if (data)
      *data = NULL;
    return 0;
  }

  ResAutoLock resLock;

  if (!data)
  {
    D3D_ERROR("NULL in lockbox");
    return 0;
  }

  *data = NULL;
  row_pitch = 0;
  slice_pitch = 0;
  lockFlags = 0;

  if (cflg & TEXCF_RTARGET)
  {
    G_ASSERTF(!(flags & TEXLOCK_WRITE), "can not write to render target");
  }

  if (flags & TEXLOCK_RWMASK)
  {
    if ((flags & TEXLOCK_READ) && !(cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)))
    {
      G_ASSERTF(flags & TEXLOCK_DELSYSMEMCOPY, "TEXLOCK_DELSYSMEMCOPY required together with TEXLOCK_READ");
    }

    //    enter_lock_resources();

    if (tex.stagingTex3D == NULL && (cflg & TEXCF_DYNAMIC) == 0)
    {
      D3D11_TEXTURE3D_DESC desc;
      desc.Width = width;
      desc.Height = height;
      desc.Depth = depth;
      desc.MipLevels = mipLevels;
      desc.Format = format;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      {
        HRESULT hr = dx_device->CreateTexture3D(&desc, NULL, &tex.stagingTex3D);
        if (FAILED(hr))
        {
          D3D_ERROR("can't create staging tex");
          if (!device_should_reset(last_hres, "CreateTexture3D"))
            DXFATAL(hr, "CreateTexture3D(stagingTex3D)");
          *data = NULL; // -V1048
          return 0;
        }
        if ((flags & TEXLOCK_READ) && !(cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)))
        {
          ContextAutoLock contextLock;
          disable_conditional_render_unsafe();
          VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
            "DX11: BaseTex::lockbox uses CopyResource in an active generic render pass");
          dx_context->CopyResource(tex.stagingTex3D, tex.tex3D);
        }
      }
    }

    if ((flags & TEXLOCK_READ) && (cflg & (TEXCF_RTARGET | TEXCF_UNORDERED)))
    {
      ContextAutoLock contextLock;
      disable_conditional_render_unsafe();
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
        "DX11: BaseTex::lockbox uses CopyResource in an active generic render pass");
      dx_context->CopyResource(tex.stagingTex3D, tex.tex3D);
    }

    ID3D11Resource *res = (cflg & TEXCF_DYNAMIC) ? tex.tex3D : tex.stagingTex3D;

    //    enter_lock_resources();

    mappedSubresource = D3D11CalcSubresource(level, 0, 1);

    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = map_without_context_blocking(res, mappedSubresource,
      (flags & TEXLOCK_DISCARD) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_READ_WRITE, flags & TEXLOCK_NOSYSLOCK, &msr);
    if (FAILED(hr))
      return 0;

    if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy))
    {
      lockMsr = msr;
      lockedLevel = level;
    }
    *data = msr.pData;
    row_pitch = msr.RowPitch;
    slice_pitch = msr.DepthPitch;
    lockFlags = flags;
    return 1;
  }
  return 0;
}

int BaseTex::unlockbox()
{
  if ((cflg & TEXCF_SYSTEXCOPY) && data_size(texCopy) && lockMsr.pData)
  {
    ddsx::Header &hdr = *(ddsx::Header *)texCopy.data();
    G_ASSERT(!hdr.compressionType());

    int rpitch = hdr.getSurfacePitch(lockedLevel);
    int h = hdr.getSurfaceScanlines(lockedLevel);
    int d = max(depth >> lockedLevel, 1);
    uint8_t *src = (uint8_t *)lockMsr.pData;
    uint8_t *dest = texCopy.data() + sizeof(ddsx::Header);

    for (int i = 0; i < lockedLevel; i++)
      dest += hdr.getSurfacePitch(i) * hdr.getSurfaceScanlines(i) * max(depth >> i, 1);
    G_ASSERT(dest < texCopy.data() + sizeof(ddsx::Header) + hdr.memSz);

    G_ASSERTF(rpitch <= lockMsr.RowPitch && rpitch * h <= lockMsr.DepthPitch, "%dx%dx%d: tex.pitch=%d,%d copy.pitch=%d,%d, lev=%d",
      width, height, depth, lockMsr.RowPitch, lockMsr.DepthPitch, rpitch, rpitch * h, lockedLevel);
    for (int di = 0; di < d; di++, src += lockMsr.DepthPitch)
      for (int y = 0; y < h; y++, dest += rpitch)
        memcpy(dest, src + y * lockMsr.RowPitch, rpitch);
    VERBOSE_DEBUG("%s %dx%dx%d updated DDSx for TEXCF_SYSTEXCOPY", getResName(), hdr.w, hdr.h, hdr.depth, data_size(texCopy));
  }
  memset(&lockMsr, 0, sizeof(lockMsr));

  if (lockFlags & TEXLOCK_RWMASK)
  {
    ContextAutoLock contextLock;
    dx_context->Unmap(tex.stagingTex3D, mappedSubresource);
    //  leave_lock_resources();
  }

  if (tex.stagingTex3D != NULL && (lockFlags & TEXLOCK_WRITE))
  {
    ContextAutoLock contextLock;
    disable_conditional_render_unsafe();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "BaseTex::unlockbox uses CopyResource in an active generic render pass");
    dx_context->CopyResource(tex.tex3D, tex.stagingTex3D);
  }

  if ((lockFlags & TEXLOCK_DELSYSMEMCOPY) && tex.stagingTex3D != NULL)
  {
    tex.stagingTex3D->Release();
    tex.stagingTex3D = NULL;
  }

  lockFlags = 0;
  return 1;
}

bool BaseTex::isSampledAsFloat() const
{
  switch (cflg & TEXFMT_MASK)
  {
    case TEXFMT_DEFAULT:
    case TEXFMT_A2R10G10B10:
    case TEXFMT_A2B10G10R10:
    case TEXFMT_A16B16G16R16:
    case TEXFMT_A16B16G16R16F:
    case TEXFMT_A32B32G32R32F:
    case TEXFMT_G16R16:
    case TEXFMT_G16R16F:
    case TEXFMT_G32R32F:
    case TEXFMT_R16F:
    case TEXFMT_R32F:
    case TEXFMT_DXT1:
    case TEXFMT_DXT3:
    case TEXFMT_DXT5:
    case TEXFMT_L16:
    case TEXFMT_A8:
    case TEXFMT_R8:
    case TEXFMT_A1R5G5B5:
    case TEXFMT_A4R4G4B4:
    case TEXFMT_R5G6B5:
    case TEXFMT_A16B16G16R16S:
    case TEXFMT_ATI1N:
    case TEXFMT_ATI2N:
    case TEXFMT_R8G8B8A8:
    case TEXFMT_R11G11B10F:
    case TEXFMT_R9G9B9E5:
    case TEXFMT_R8G8:
    case TEXFMT_R8G8S:
    case TEXFMT_DEPTH24:
    case TEXFMT_DEPTH16:
    case TEXFMT_DEPTH32:
    case TEXFMT_DEPTH32_S8: return true;
  }
  return false;
}

void BaseTex::D3DTextures::release()
{
  if (stagingTex != NULL)
  {
    stagingTex->Release();
    stagingTex = NULL;
  }
  if (texRes != NULL)
  {
    texRes->Release();
    texRes = NULL;
  }

  if (resolvedTex)
  {
    resolvedTex->Release();
  }
  resolvedTex = NULL;
}

void BaseTex::D3DTextures::setPrivateData(const char *name) const
{
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC_WIN
  if (!name)
  {
    return;
  }
  if (texRes)
  {
    texRes->SetPrivateData(WKPDID_D3DDebugObjectName, i_strlen(name), name);
  }
  if (stagingTex)
  {
    stagingTex->SetPrivateData(WKPDID_D3DDebugObjectName, i_strlen(name), name);
  }
#endif
}
} // namespace drv3d_dx11
