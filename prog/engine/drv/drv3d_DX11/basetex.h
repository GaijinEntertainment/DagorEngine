// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_globDef.h>
#include <generic/dag_smallTab.h>
#include <3d/ddsxTex.h>
#include <DXGIFormat.h>
#include "generic/dag_tabExt.h"
#include <3d/tql.h>
#include "basetexture.h"

#define MAX_STAT_NAME 64
#define MAX_MIPLEVELS 16


namespace ddsx
{
struct Header;
}

namespace drv3d_dx11
{
static bool is_depth_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return cflg >= TEXFMT_FIRST_DEPTH && cflg <= TEXFMT_LAST_DEPTH;
}

static bool is_stencil_format_flg(uint32_t cflg)
{
  cflg &= TEXFMT_MASK;
  return is_depth_format_flg(cflg) && cflg != TEXFMT_DEPTH16 && cflg != TEXFMT_DEPTH32;
}

static bool is_signed_format(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R8G8_SNORM: return true;
  }
  return false;
}

bool is_float_format(uint32_t cflg);

struct TextureView
{
  typedef uint32_t Key;

  enum ViewType
  {
    UNKNOWN = 0,
    DEPTH = 1,
    SHADER_RES = 2,
    UAV = 3,
    SRGB_TARGET = 4,
    NON_SRGB_TARGET = 5,
    DEPTH_READONLY = 6,
    UAV_UINT = 7,
    STENCIL_SHADER_RES = 8,
  };

  union
  {
    ID3D11View *view;
    ID3D11RenderTargetView *targetView;
    ID3D11DepthStencilView *depthStencilView;
    ID3D11ShaderResourceView *shaderResView;
    ID3D11UnorderedAccessView *uaView;
  };

  TextureView() : view(NULL) {}

  void destroyObject()
  {
    if (view)
      view->Release();
    view = NULL;
  }

  static Key key(ViewType type, uint32_t face, uint32_t mip_level, uint32_t mip_count = 1)
  {
    return ((Key)(type << 24) | (face << 8) | (mip_level << 4) | (mip_count << 0)) + 1;
  }
};

class BaseTex final : public BaseTextureImpl
{
public:
  struct D3DTextures
  {
    union
    {
      ID3D11Resource *texRes;
      ID3D11Texture2D *tex2D;
      ID3D11Texture3D *tex3D;
    };

    union
    {
      // for staging textures
      ID3D11Resource *stagingTex;
      ID3D11Texture2D *stagingTex2D;
      ID3D11Texture3D *stagingTex3D;
    };
    D3D11_USAGE texUsage;
    UINT texAccess; // D3D11_CPU_ACCESS_FLAG
    uint32_t memSize : 28;
    uint32_t realMipLevels : 4;

    void release();
    void setPrivateData(const char *) const;

    ID3D11Texture2D *resolvedTex;
    D3DTextures() :
      resolvedTex(NULL), stagingTex(NULL), texRes(NULL), texUsage(D3D11_USAGE_DEFAULT), texAccess(0), memSize(0), realMipLevels(0){};
  };
  D3DTextures tex;

  KeyMap<TextureView, TextureView::Key, 2> viewCache;

  char *debugLockData = nullptr;
  D3D11_MAPPED_SUBRESOURCE lockMsr;

  DECLARE_TQL_TID_AND_STUB()
  SmallTab<uint8_t, MidmemAlloc> texCopy; //< ddsx::Header + ddsx data; sysCopyQualityId stored in hdr.hqPartLevel
  BaseTexture::IReloadData *rld;

  UINT mappedSubresource = 0;
  int id = -1;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

  G_STATIC_ASSERT(STAGE_MAX_EXT == 4);
  uint16_t modified : STAGE_MAX_EXT;
  uint16_t delayedCreate : 1;
  uint16_t read_stencil : 1;
  uint16_t preallocBeforeLoad : 1;
  uint16_t needs_clear : 1;
  uint16_t ddsxNotLoaded : 1;
  uint16_t cube_array : 1;
  uint16_t wasCopiedToStage : 1;
  uint16_t dirtyRt : 1;
  uint16_t wasUsed : 1;

  static BaseTex *create_tex(uint32_t cflg_, int type_);
  static void destroy_tex(BaseTex *);

  virtual void setResApiName(const char *name) const override { tex.setPrivateData(name); }

  bool setReloadCallback(BaseTexture::IReloadData *_rld);

  int ressize() const override;

  const char *strLabel() const;

  void release();
  int recreate();
  void destroy() override;
  bool releaseTex(bool recreate_after);

  void resolve(ID3D11Resource *dst, DXGI_FORMAT dst_format);
  int update(BaseTexture *src) override;
  int updateSubRegionImpl(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z);
  int updateSubRegion(BaseTexture *src, int src_subres_idx, int src_x, int src_y, int src_z, int src_w, int src_h, int src_d,
    int dest_subres_idx, int dest_x, int dest_y, int dest_z) override;
  // Only difference between this and ref implementation is the use of updateSubRegionImpl to bypass copy dest flag check.
  BaseTexture *downSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
    unsigned level_offset) override;
  // Only difference between this and ref implementation is the use of updateSubRegionImpl to bypass copy dest flag check.
  BaseTexture *upSize(int new_width, int new_height, int new_depth, int new_mips, unsigned start_src_level,
    unsigned level_offset) override;
  void clear();

  void destroyObject();

  int texmiplevel(int minlev, int maxlev);
  void setReadStencil(bool on) override
  {
    read_stencil = on && is_stencil_format_flg(cflg);
  } // for depth stencil textures, if on will read stencil, not depth. Currently either of two

  ID3D11View *getRtView(uint32_t face, uint32_t mip_level, uint32_t mip_count, bool srgb_write);
  // mip_level == 0, mip_count == 0 ==> use all resource (face ignored)
  ID3D11ShaderResourceView *getResView(uint32_t face, uint32_t mip_level, uint32_t mip_count);
  ID3D11UnorderedAccessView *getExistingUaView(uint32_t face, uint32_t mip_level, bool as_uint);
  ID3D11UnorderedAccessView *getUaView(uint32_t face, uint32_t mip_level, bool as_uint);

  BaseTexture *makeTmpTexResCopy(int w, int h, int d, int l, bool staging_tex) override;
  void replaceTexResObject(BaseTexture *&other_tex) override;

  bool allocateTex() override;
  void discardTex() override;

  inline bool isTexResEqual(BaseTexture *bt) const;

  int lockimg(void **p, int &stride, int lev, unsigned flags);
  int lockimg(void **p, int &stride, int face, int level, unsigned flags);
  int unlockimg();

  int lockbox(void **data, int &row_pitch, int &slice_pitch, int level, unsigned flags);
  int unlockbox();

  int generateMips();
  bool isCubeArray() const { return cube_array; }
  void setModified(int shader_stage)
  {
    G_ASSERT(shader_stage >= 0 && shader_stage < STAGE_MAX_EXT);
    modified |= (1 << shader_stage);
  }
  void unsetModified(int shader_stage)
  {
    G_ASSERT(shader_stage >= 0 && shader_stage < STAGE_MAX_EXT);
    modified &= ~(1 << shader_stage);
  }
  bool getModified(int shader_stage)
  {
    G_ASSERT(shader_stage >= 0 && shader_stage < STAGE_MAX_EXT);
    return modified & (1 << shader_stage);
  }
  bool isSampledAsFloat() const;

private:
  BaseTex(uint32_t cflg_, int type_);
  BaseTex(const BaseTex &) = delete;
  BaseTex &operator=(const BaseTex &) = delete;
  ~BaseTex();

protected:
  int texaddrImpl(int a) override;
  int texaddruImpl(int a) override;
  int texaddrvImpl(int a) override;
  int texaddrwImpl(int a) override;
  int texbordercolorImpl(E3DCOLOR c) override;
  int texfilterImpl(int m) override;
  int texmipmapImpl(int m) override;
  int texlodImpl(float mipmaplod) override;
  int setAnisotropyImpl(int level) override;
};
}; // namespace drv3d_dx11
