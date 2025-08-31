// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_texMgr.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <3d/ddsxTex.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <image/dag_loadImage.h>
#include <assets/asset.h>
#include <assets/assetHlp.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <ioSys/dag_memIo.h>
#include <EASTL/hash_map.h>
#include <memory/dag_framemem.h>

#include "assetManager.h"
#include <generic/dag_smallTab.h>
#include <util/dag_string.h>
#include <util/dag_texMetaData.h>
#include <3d/ddsxTexMipOrder.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <mutex>
#include "assetStatus.h"

#include "../lib3d/texMgrData.h"
using texmgr_internal::RMGR;

namespace texmgr_internal
{
extern bool disable_add_warning;
}
namespace texmgr_internal
{
TexQL max_allowed_q = TQL_high;
}


static void free_unused_textures()
{
  if (auto managerPtr = get_asset_manager())
  {
    const DagorAssetMgr &asset__manager = *managerPtr;
    int texTypeId = asset__manager.getTexAssetTypeId();
    for (auto assetId : asset__manager.getFilteredAssets(make_span_const(&texTypeId, 1)))
    {
      const DagorAsset &asset = asset__manager.getAsset(assetId);
      String texName(0, "%s*", asset.getName());
      D3DRESID texId = get_managed_texture_id(texName);
      if (texId != BAD_TEXTUREID)
      {
        int refCount = get_managed_texture_refcount(texId);
        if (refCount == 0)
          evict_managed_tex_id(texId);
      }
    }
  }
}


class AssetsImportTexFactory final : public TextureFactory
{
public:
  static void mark_cur_ql(unsigned idx, TexQL ql)
  {
    RMGR.levDesc[idx] = 0;
    RMGR.setLevDesc(idx, ql, 2);
    RMGR.resQS[idx].setQLev(2);
    RMGR.resQS[idx].setLdLev(ql == TQL_stub ? 1 : 2);
    RMGR.resQS[idx].setMaxQL(ql, ql);
  }

  bool scheduleTexLoading(TEXTUREID id, TexQL ql) override
  {
    if (get_managed_texture_refcount(id) == 0)
      return false;
    if (texmgr_internal::max_allowed_q == TQL_stub)
      ql = TQL_stub;
    else
      ql = min(texmgr_internal::max_allowed_q, ql);

    const DagorAssetMgr *aMgr = get_asset_manager();
    if (!aMgr)
      return false;

    const char *textureName = get_managed_texture_name(id);
    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));
    erase_items(fpath, fpath.size() - 2, 1);

    if (DagorAsset *a = aMgr->findAsset(fpath, aMgr->getTexAssetTypeId()))
      if (ql != RMGR.resQS[id.index()].getCurQL())
      {
        if (BaseTexture *bt = acquire_managed_tex(id))
        {
          if (ql != TQL_stub)
            load_tex_data(bt, id, nullptr, nullptr, a, tmd, ql);
          else
          {
            mark_cur_ql(id.index(), TQL_stub);
            bt->discardTex();
          }
        }
        release_managed_tex(id);
      }
    return true;
  }

  BaseTexture *createTexture(TEXTUREID id) override
  {
    const char *textureName = get_managed_texture_name(id);

    if (!textureName)
      return nullptr;

    const DagorAssetMgr *aMgr = get_asset_manager();
    if (!aMgr)
      return nullptr;

    int idx = id.index();
    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));

    mark_cur_ql(idx, TQL_base);
    if (!isPackTex(textureName))
      return defFactory->createTexture(id);

    erase_items(fpath, fpath.size() - 2, 1);

    int tex_atype = aMgr->getTexAssetTypeId();
    DagorAsset *a = aMgr->findAsset(fpath, tex_atype);

    setTextureStatus(fpath, AssetLoadingStatus::Loading);

    if (!a && dd_get_fname_ext(fpath))
      a = aMgr->findAsset(fpath + ".xxx", tex_atype);
    if (a)
      tmd.read(a->props, a->resolveEffProfileTargetStr("PC", nullptr));
    TexQL ql = texmgr_internal::max_allowed_q;

    if (a && a->props.getBool("convert", false))
    {
      std::lock_guard<std::mutex> scopedLock(texLock);
      ddsx::Buffer b;
      ddsx::Buffer hq_b;

      if (ql == TQL_thumb)
        if (DagorAsset *tq_a = aMgr->findAsset(fpath + "$tq", tex_atype))
          if (!texconvcache::get_tex_asset_built_ddsx(*tq_a, b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
            logerr("cannot convert <%s> tex asset", tq_a->getName());

      if (!b.ptr && texconvcache::get_tex_asset_built_ddsx(*a, b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
      {
        if (ql == TQL_uhq)
          if (DagorAsset *uhq_a = aMgr->findAsset(fpath + "$uhq", tex_atype))
            if (!texconvcache::get_tex_asset_built_ddsx(*uhq_a, hq_b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
              logerr("cannot convert <%s> tex asset", uhq_a->getName());

        if (!hq_b.ptr && ql >= TQL_high)
          if (DagorAsset *hq_a = aMgr->findAsset(fpath + "$hq", tex_atype))
            if (!texconvcache::get_tex_asset_built_ddsx(*hq_a, hq_b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
              logerr("cannot convert <%s> tex asset", hq_a->getName());

        if (b.ptr && hq_b.ptr)
        {
          ddsx::Header &hq_hdr = *(ddsx::Header *)hq_b.ptr;
          if (hq_hdr.levels)
          {
            ddsx::Header hdr = hq_hdr;
            if (hdr.lQmip < hdr.levels)
              hdr.lQmip = hdr.levels;
            hdr.levels += ((ddsx::Header *)b.ptr)->levels;
            hdr.flags &= ~hdr.FLG_HQ_PART;

            BaseTexture *bt =
              d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, a->getName(), tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr)));
            if (load_tex_data(bt, id, &b, &hq_b, a, tmd, ql))
              set_texture_separate_sampler(id, get_sampler_info(tmd));
            else
              del_d3dres(bt);
            setTextureStatus(fpath, AssetLoadingStatus::Loaded);
            return bt;
          }
          hq_b.free();
        }
      }

      if (b.ptr && !hq_b.ptr)
      {
        auto &hdr = *(ddsx::Header *)b.ptr;
        if (!hdr.d3dFormat && !hdr.memSz)
        {
          b.free();
          setTextureStatus(fpath, AssetLoadingStatus::LoadedWithErrors);
          return nullptr;
        }

        if (!(hdr.flags & hdr.FLG_HQ_PART) && hdr.hqPartLevels)
        {
          hdr.mQmip = hdr.mQmip > hdr.hqPartLevels ? (hdr.mQmip - hdr.hqPartLevels) : 0;
          hdr.lQmip = hdr.lQmip > hdr.hqPartLevels ? (hdr.lQmip - hdr.hqPartLevels) : 0;
          hdr.hqPartLevels = 0;
        }

        BaseTexture *bt =
          d3d::alloc_ddsx_tex(hdr, TEXCF_LOADONCE, 0, 0, a->getName(), tql::getEffStubIdx(tmd.stubTexIdx, tql::get_tex_type(hdr)));
        if (load_tex_data(bt, id, &b, nullptr, a, tmd, ql))
          set_texture_separate_sampler(id, get_sampler_info(tmd));
        else
          del_d3dres(bt);
        setTextureStatus(fpath, AssetLoadingStatus::Loaded);
        return bt;
      }

      logerr("cannot convert <%s> tex asset", a->getName());
      setTextureStatus(fpath, AssetLoadingStatus::LoadedWithErrors);
      return nullptr;
    }

    logerr("failed to load <%s>, asset=%p convert=%d", fpath, a, a ? a->props.getBool("convert", false) : false);
    setTextureStatus(fpath, AssetLoadingStatus::LoadedWithErrors);
    return nullptr;
  }

  void releaseTexture(BaseTexture *texture, TEXTUREID id) override
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return;

    if (!isPackTex(textureName))
      return defFactory->releaseTexture(texture, id);

    if (texture)
      texture->destroy();
  }

  static void unpackDdsxBuf(ddsx::Buffer &b)
  {
    ddsx::Header &hdr = *(ddsx::Header *)b.ptr;
    InPlaceMemLoadCB fcrd((char *)b.ptr + sizeof(hdr), hdr.packedSz);
    if (hdr.isCompressionZSTD())
    {
      ZstdLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompressionOODLE())
    {
      OodleLoadCB crd(fcrd, hdr.packedSz, hdr.memSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompressionZLIB())
    {
      ZlibLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
    else if (hdr.isCompression7ZIP())
    {
      LzmaLoadCB crd(fcrd, hdr.packedSz);
      void *p = memalloc(b.len = sizeof(hdr) + hdr.memSz, tmpmem);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      hdr.packedSz = 0;
      memcpy(p, &hdr, sizeof(hdr));
      crd.read((char *)p + sizeof(hdr), hdr.memSz);
      memfree(b.ptr, tmpmem);
      b.ptr = p;
      crd.ceaseReading();
    }
  }

  bool getTextureDDSx(TEXTUREID id, Tab<char> &out_ddsx) override
  {
    const char *textureName = get_managed_texture_name(id);
    if (!textureName)
      return false;

    if (!isPackTex(textureName))
      return false;

    const DagorAssetMgr *aMgr = get_asset_manager();
    if (!aMgr)
      return false;

    TextureMetaData tmd;
    String tmd_stor, fpath(tmd.decode(textureName, &tmd_stor));
    erase_items(fpath, fpath.size() - 2, 1);

    int tex_atype = aMgr->getTexAssetTypeId();
    DagorAsset *a = aMgr->findAsset(fpath, tex_atype);
    if (a)
      tmd.read(a->props, a->resolveEffProfileTargetStr("PC", nullptr));
    if (a && a->props.getBool("convert", false))
    {
      std::lock_guard<std::mutex> scopedLock(texLock);
      ddsx::Buffer b;
      const int hdr_sz = sizeof(ddsx::Header);
      if (texconvcache::get_tex_asset_built_ddsx(*a, b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
      {
        unpackDdsxBuf(b);
        ddsx_forward_mips_inplace(*(ddsx::Header *)b.ptr, (char *)b.ptr + hdr_sz, b.len - hdr_sz);
        if (DagorAsset *hq_a = aMgr->findAsset(fpath + "$hq", tex_atype))
        {
          ddsx::Buffer hq_b;
          if (texconvcache::get_tex_asset_built_ddsx(*hq_a, hq_b, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
          {
            unpackDdsxBuf(hq_b);
            ddsx::Header &hq_hdr = *(ddsx::Header *)hq_b.ptr;
            ddsx_forward_mips_inplace(hq_hdr, (char *)hq_b.ptr + hdr_sz, hq_b.len - hdr_sz);
            if (hq_hdr.levels)
            {
              ddsx::Header hdr = hq_hdr;
              if (hdr.lQmip < hdr.levels)
                hdr.lQmip = hdr.levels;
              hdr.levels += ((ddsx::Header *)b.ptr)->levels;
              hdr.flags &= ~hdr.FLG_HQ_PART;

              if (hdr.flags & (hdr.FLG_GENMIP_BOX | hdr.FLG_GENMIP_KAIZER))
              {
                out_ddsx.resize(hq_b.len);
                mem_copy_from(out_ddsx, hq_b.ptr);
                memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
              }
              else
              {
                out_ddsx.resize(hq_b.len + b.len - hdr_sz);
                memcpy(out_ddsx.data(), hq_b.ptr, hq_b.len);
                memcpy(out_ddsx.data() + hq_b.len, (char *)b.ptr + hdr_sz, b.len - hdr_sz);
                memcpy(out_ddsx.data(), &hdr, sizeof(hdr));
              }
              b.free();
              hq_b.free();
              return true;
            }
            hq_b.free();
          }
          else
            logerr("cannot convert <%s> tex asset", hq_a->getName());
        }
        out_ddsx.resize(b.len);
        mem_copy_from(out_ddsx, b.ptr);
        b.free();
        return true;
      }
    }
    return false;
  }
  static bool isPackTex(const char *textureName)
  {
    const char *plast = strchr(textureName, '*');
    return plast && (plast[1] == '?' || plast[1] == '\0');
  }

  static bool load_tex_data(BaseTexture *bt,
    TEXTUREID tid,
    ddsx::Buffer *bq,
    ddsx::Buffer *hq,
    DagorAsset *a,
    TextureMetaData &tmd,
    TexQL ql,
    const char *tex_fn = nullptr)
  {
    const DagorAssetMgr *aMgr = get_asset_manager();
    if (!aMgr)
      return false;
    int tex_atype = aMgr->getTexAssetTypeId();
    if (!bt)
    {
      if (hq)
        hq->free();
      if (bq)
        bq->free();
      mark_cur_ql(tid.index(), TQL_stub);
      return false;
    }
    if (ql == TQL_stub)
    {
      if (hq)
        hq->free();
      if (bq)
        bq->free();
      mark_cur_ql(tid.index(), ql);
      return true;
    }
    struct AutoTexIdHolder
    {
      TEXTUREID id;

      AutoTexIdHolder(TextureMetaData &tmd) : id(BAD_TEXTUREID)
      {
        if (tmd.baseTexName.empty())
          return;
        id = get_managed_texture_id(String(64, "%s*", tmd.baseTexName.str()));
        if (id == BAD_TEXTUREID)
          return;
        RMGR.incBaseTexRc(id);
        acquire_managed_tex(id);
      }
      ~AutoTexIdHolder()
      {
        if (id == BAD_TEXTUREID)
          return;
        release_managed_tex(id);
        RMGR.decBaseTexRc(id);
      }
    };
    AutoTexIdHolder bt_ref(tmd);

    ddsx::Buffer l_bq, l_hq;
    TextureInfo ti;
    bt->getinfo(ti, 0);
    int target_lev = get_log2i(max(ti.w, ti.h));
    if (a && a->props.getBool("convert", false))
    {
      if (!bq)
      {
        bq = &l_bq;
        if (!texconvcache::get_tex_asset_built_ddsx(*a, *bq, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
        {
          logerr("cannot convert <%s> tex asset", a->getName());
          if (hq)
            hq->free();
          return false;
        }
      }

      ddsx::Header &bq_hdr = *(ddsx::Header *)bq->ptr;
      int bq_lev = get_log2i(max(bq_hdr.w, bq_hdr.h));
      bool hold_sysmem = (bq_hdr.flags & bq_hdr.FLG_HOLD_SYSMEM_COPY) != 0;
      if (DagorAsset *hq_a = ql > TQL_base ? aMgr->findAsset(String(0, "%s$hq", a->getName()), tex_atype) : nullptr)
      {
        if (!hq)
        {
          hq = &l_hq;
          if (!texconvcache::get_tex_asset_built_ddsx(*hq_a, *hq, _MAKE4C('PC'), nullptr, get_asset_manager_log_writer()))
          {
            logerr("cannot convert <%s> tex asset", hq_a->getName());
            bq->free();
            return false;
          }
        }

        ddsx::Header &hq_hdr = *(ddsx::Header *)hq->ptr;
        if (hq_hdr.levels)
        {
          hold_sysmem = (hq_hdr.flags & hq_hdr.FLG_HOLD_SYSMEM_COPY) != 0;
          if (hold_sysmem)
            RMGR.incBaseTexRc(tid);

          InPlaceMemLoadCB crd(bq->ptr, bq->len);
          crd.seekrel(sizeof(ddsx::Header));
          bt->allocateTex();
          if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1) != TexLoadRes::OK)
            logerr("error loading tex %s", a->getName());
          bq->free();

          InPlaceMemLoadCB crd_hq(hq->ptr, hq->len);
          crd_hq.seekrel(sizeof(ddsx::Header));
          if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, hq_hdr, crd_hq, 0, 0, bq_lev) != TexLoadRes::OK)
            logerr("error loading tex %s", hq_a->getName());
          hq->free();
          bt->texmiplevel(0, ti.mipLevels - 1);

          if (hold_sysmem)
            RMGR.decBaseTexRc(tid);
          mark_cur_ql(tid.index(), ql);
          return true;
        }
        hq->free();
      }
      else if (hq)
        hq->free();

      if (hold_sysmem)
        RMGR.incBaseTexRc(tid);

      InPlaceMemLoadCB crd(bq->ptr, bq->len);
      crd.seekrel(sizeof(ddsx::Header));
      bt->allocateTex();
      if (d3d_load_ddsx_tex_contents(bt, tid, bt_ref.id, bq_hdr, crd, 0, target_lev - bq_lev, 1) != TexLoadRes::OK)
        logerr("error loading tex %s", a->getName());
      bq->free();
      bt->texmiplevel(target_lev - bq_lev, ti.mipLevels - 1);

      if (hold_sysmem)
        RMGR.decBaseTexRc(tid);
      mark_cur_ql(tid.index(), ql);
      return true;
    }

    logerr("error loading tex %s", a ? a->getName() : tex_fn);
    return false;
  }

  void setTextureStatus(const char *name, AssetLoadingStatus status)
  {
    std::lock_guard<std::mutex> scopedLock(texStatusLock);
    textureStatus[name] = status;
  }

  ~AssetsImportTexFactory()
  {
    for (unsigned i = 0, ie = RMGR.getRelaxedIndexCount(); i < ie; i++)
      if (RMGR.getFactory(i) == this)
        RMGR.setFactory(i, nullptr);
  }

  TextureFactory *defFactory = nullptr;
  std::mutex texLock;
  std::mutex texStatusLock;

  eastl::hash_map<eastl::string, AssetLoadingStatus> textureStatus;
};

static eastl::unique_ptr<AssetsImportTexFactory> texture_factory;

AssetLoadingStatus get_texture_status(const char *name)
{
  if (texture_factory)
  {
    std::lock_guard<std::mutex> scopedLock(texture_factory->texStatusLock);
    auto it = texture_factory->textureStatus.find_as(name);
    if (it != texture_factory->textureStatus.end())
    {
      return it->second;
    }
  }
  return AssetLoadingStatus::NotLoaded;
}

void init_asset_import_texture_factory()
{
  free_unused_textures();
  if (::get_default_tex_factory() == texture_factory.get())
    return;
  if (!texture_factory)
    texture_factory = eastl::make_unique<AssetsImportTexFactory>();
  texmgr_internal::disable_add_warning = true;
  texture_factory->defFactory = ::get_default_tex_factory();
  ::set_default_tex_factory(texture_factory.get());
}
void reregister_texture_asset_manager_factory(const DagorAsset &a)
{
  D3DRESID tid = get_managed_texture_id(String(0, "%s*", a.getName()));
  if (!tid || !RMGR.isValidID(tid, nullptr) || RMGR.getFactory(tid.index()) == texture_factory.get())
    return;
  texmgr_internal::TexMgrAutoLock autoLock;
  unsigned idx = tid.index();
  auto &desc = RMGR.texDesc[idx];
  memset(desc.packRecIdx, 0xFF, sizeof(desc.packRecIdx));
  AssetsImportTexFactory::mark_cur_ql(idx, get_managed_res_cur_tql(tid));
  SimpleString nm(RMGR.getName(idx));
  RMGR.setName(idx, nullptr, RMGR.getFactory(idx));
  RMGR.setName(idx, nm, texture_factory.get());
  RMGR.setFactory(idx, texture_factory.get());
}
