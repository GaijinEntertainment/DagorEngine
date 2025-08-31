// Copyright (C) Gaijin Games KFT.  All rights reserved.

// ddsx/streamed textures ctors implementation
#include <drv/3d/dag_texture.h>
#include <ioSys/dag_memIo.h>
#include "texture.h"
#include "frontend.h"
#include "resource_upload_limit.h"
#include <EASTL/unique_ptr.h>

using namespace drv3d_vulkan;

namespace
{

static BaseTexture *retry_load_ddsx_tex_contents(BaseTexture *tex, const ddsx::Header &hdr, int quality_id, const void *ptr, int sz,
  const char *stat_name)
{
  // this retry should always complete with OK status, as there is no higher level retry logic on use places
  // so just use RUB overallocation
  drv3d_vulkan::Frontend::resUploadLimit.setNoFailOnThread(true);
  InPlaceMemLoadCB mcrd(ptr, sz);
  TexLoadRes ldRet = d3d::load_ddsx_tex_contents(tex, hdr, mcrd, quality_id);
  Frontend::resUploadLimit.setNoFailOnThread(false);
  if (ldRet != TexLoadRes::OK)
    D3D_ERROR("vulkan: retry failure at texture %s ddsx load code %u", stat_name, (int)ldRet);
  // caller don't expect nullptr, so complain, but not crash!
  return tex;
}

} // namespace

// load compressed texture
BaseTexture *d3d::create_ddsx_tex(IGenLoad &crd, int flg, int quality_id, int levels, const char *stat_name)
{
  ddsx::Header hdr;
  if (!crd.readExact(&hdr, sizeof(hdr)) || !hdr.checkLabel())
  {
    debug("invalid DDSx format");
    return nullptr;
  }

  BaseTexture *tex = alloc_ddsx_tex(hdr, flg, quality_id, levels, stat_name);
  if (tex)
  {
    BaseTex *bt = (BaseTex *)tex;
    G_ASSERTF_AND_DO(hdr.hqPartLevels == 0, bt->pars.flg &= ~TEXCF_SYSTEXCOPY,
      "cannot use TEXCF_SYSTEXCOPY with base part of split texture!");
    const int dataSz = hdr.packedSz ? hdr.packedSz : hdr.memSz;

    // Use temporary buffer to read compressed texture data
    // crd could be a FastSeqReadCB which has a limit to seekto back in the stream
    // and mught end up with fatal error.
    eastl::unique_ptr<uint8_t[]> localTmpBuffer;
    uint8_t *tmpBuffer = bt->allocSysCopyForDDSX(hdr, quality_id);
    if (!tmpBuffer) //-V547
    {
      localTmpBuffer.reset(new uint8_t[dataSz]);
      tmpBuffer = localTmpBuffer.get();
    }

    if (!crd.readExact(tmpBuffer, dataSz))
    {
      LOGERR_CTX("Cannot read %d of texture %s", dataSz, stat_name);
      del_d3dres(tex);
      return nullptr;
    }

    InPlaceMemLoadCB tmpCrd(tmpBuffer, dataSz);
    TexLoadRes ldRet = load_ddsx_tex_contents(tex, hdr, tmpCrd, quality_id);
    if (ldRet == TexLoadRes::OK)
      return tex;

    return retry_load_ddsx_tex_contents(tex, hdr, quality_id, tmpBuffer, dataSz, stat_name);
  }
  return nullptr;
}

BaseTexture *d3d::alloc_ddsx_tex(const ddsx::Header &hdr, int flg, int q_id, int levels, const char *stat_name, int stub_tex_idx)
{
  flg = implant_d3dformat(flg, hdr.d3dFormat);
  if (hdr.d3dFormat == ::D3DFMT_A4R4G4B4 || hdr.d3dFormat == ::D3DFMT_X4R4G4B4 || hdr.d3dFormat == ::D3DFMT_R5G6B5)
    flg = implant_d3dformat(flg, ::D3DFMT_A8R8G8B8);
  D3D_CONTRACT_ASSERT((flg & TEXCF_RTARGET) == 0);
  flg |= (hdr.flags & hdr.FLG_GAMMA_EQ_1) ? 0 : TEXCF_SRGBREAD;

  if (levels <= 0)
    levels = hdr.levels;

  D3DResourceType type;
  if (hdr.flags & ddsx::Header::FLG_CUBTEX)
    type = D3DResourceType::CUBETEX;
  else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    type = D3DResourceType::VOLTEX;
  else if (hdr.flags & ddsx::Header::FLG_ARRTEX)
    type = D3DResourceType::ARRTEX;
  else
    type = D3DResourceType::TEX;

  int skip_levels = hdr.getSkipLevels(hdr.getSkipLevelsFromQ(q_id), levels);
  int w = max(hdr.w >> skip_levels, 1), h = max(hdr.h >> skip_levels, 1), d = max(hdr.depth >> skip_levels, 1);
  if (!(hdr.flags & hdr.FLG_VOLTEX))
    d = (hdr.flags & hdr.FLG_ARRTEX) ? hdr.depth : 1;

  auto bt = allocate_texture(w, h, d, levels, type, flg, stat_name);

  bt->stubTexIdx = stub_tex_idx;
  bt->preallocBeforeLoad = bt->delayedCreate = true;

  if (stub_tex_idx >= 0)
  {
    // static analysis says this could be null
    auto subtex = bt->getStubTex();
    if (subtex)
      bt->image = subtex->image;
  }

  return bt;
}
