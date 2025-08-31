// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/texAssetBuilderTextureFactory.h>
#include <assets/assetHlp.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <libTools/dtx/ddsxPlugin.h>
#include <image/dag_dds.h>
#include <3d/dag_buildOnDemandTexFactory.h>
#include <3d/dag_texMgr.h>
#include <3d/ddsxTex.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_texMetaData.h>
#include <util/dag_string.h>

struct TexConvBuildOnDemandHelper : public ITexBuildOnDemandHelper
{
  TexConvBuildOnDemandHelper(DagorAssetMgr &m, ILogWriter *c) : assetMgr(&m), conWriter(c) {}

  bool checkTexNameHandled(const char *tex_fn) override
  {
    const char *plast = strchr(tex_fn, '*');
    return plast && (plast[1] == '?' || plast[1] == '\0');
  }

  asset_handle_t *resolveTex(const char *tex_fn, TextureMetaData &out_tmd) override
  {
    if (!tex_fn || !*tex_fn)
      return nullptr;

    String name(DagorAsset::fpath2asset(tex_fn));
    DagorAsset *a = assetMgr->findAsset(name, assetMgr->getTexAssetTypeId());
    if (!a && !dd_get_fname_ext(tex_fn))
    {
      name = DagorAsset::fpath2asset(String::mk_str_cat(tex_fn, ".xxx"));
      a = assetMgr->findAsset(name, assetMgr->getTexAssetTypeId());
    }

    if (a)
      out_tmd.read(a->props, a->resolveEffProfileTargetStr("PC", nullptr));
    return reinterpret_cast<asset_handle_t *>(a);
  }

  bool getConvertibleTexHeader(asset_handle_t *h, ddsx::Header &out_hdr, unsigned &out_q_lev_desc, unsigned *out_name_ids) override
  {
    auto *a = reinterpret_cast<DagorAsset *>(h);
    if (!a)
      return false;
    bool cvt = a->props.getBool("convert", false);
    if (cvt && texconvcache::get_convertible_tex_asset_header(*a, out_hdr, out_q_lev_desc))
    {
      const char *suff[] = {"$tq", nullptr, "$hq", "$uhq"};
      for (TexQL ql = TQL_thumb; ql <= TQL_uhq; ql = TexQL(ql + 1))
        out_name_ids[ql] = ((out_q_lev_desc >> (ql * 4)) & 0xF)
                             ? (suff[ql] ? assetMgr->findAsset(String::mk_str_cat(a->getName(), suff[ql])) : a)->getNameId()
                             : ~0u;
      return true;
    }
    else if (!cvt)
    {
      String fname(a->getTargetFilePath());
      const char *ext = dd_get_fname_ext(fname);
      if (!ext || stricmp(ext, ".dds") != 0)
        return false;

      ImageInfoDDS dds_info;
      if (read_dds_info(fname, dds_info))
      {
        memset(&out_hdr, 0, sizeof(out_hdr));
        out_hdr.label = _MAKE4C('DDSx');
        out_hdr.d3dFormat = dds_info.d3dFormat;
        out_hdr.w = dds_info.width;
        out_hdr.h = dds_info.height;
        out_hdr.depth = dds_info.depth;
        out_hdr.levels = dds_info.nlevels;
        if (dds_info.cube_map)
          out_hdr.flags |= out_hdr.FLG_CUBTEX;
        else if (dds_info.volume_tex)
          out_hdr.flags |= out_hdr.FLG_VOLTEX;

        float gamma = a->getProfileTargetProps(_MAKE4C('PC'), nullptr).getReal("gamma", a->props.getReal("gamma", 2.2));
        if (fabsf(gamma - 1.0f) < 1e-3f)
          out_hdr.flags |= out_hdr.FLG_GAMMA_EQ_1;

        out_q_lev_desc = get_log2i(max(out_hdr.w, out_hdr.h)) << 4;
        out_name_ids[TQL_thumb] = out_name_ids[TQL_high] = out_name_ids[TQL_uhq] = ~0u;
        out_name_ids[TQL_base] = a->getNameId();
        return true;
      }
    }
    return false;
  }
  bool getSimpleTexProps(asset_handle_t *h, String &out_fn, float &out_gamma) override
  {
    auto *a = reinterpret_cast<DagorAsset *>(h);
    if (!a)
      return false;
    out_fn = a->getTargetFilePath();
    out_gamma = a->getProfileTargetProps(_MAKE4C('PC'), nullptr).getReal("gamma", a->props.getReal("gamma", 2.2));
    return true;
  }

  const char *getTexName(unsigned nameId) override { return assetMgr->getAssetName(nameId); }

  bool prebuildTex(unsigned nameId, bool &out_cache_was_updated) override
  {
    out_cache_was_updated = false;
    if (DagorAsset *a = find_asset_by_name_id(nameId))
    {
      bool cvt = a->props.getBool("convert", false);
      bool is_prod_ready = false;
      if (cvt && !texconvcache::is_tex_built_and_actual(*a, _MAKE4C('PC'), nullptr, is_prod_ready))
      {
        ddsx::Buffer tex_buf;
        if (texconvcache::get_tex_asset_built_ddsx(*a, tex_buf, _MAKE4C('PC'), nullptr, conWriter))
          out_cache_was_updated = true;
        else
          debug("failed to update tex %s", a->getName());
        tex_buf.free();
      }
      else if (!cvt)
      {
        String fname(a->getTargetFilePath());
        const char *ext = dd_get_fname_ext(fname);
        return ext && stricmp(ext, ".dds") == 0;
      }
      return true;
    }
    return false;
  }

  dag::Span<char> getDDSxTexData(unsigned nameId) override
  {
    ddsx::Buffer buf;
    if (DagorAsset *a = find_asset_by_name_id(nameId))
    {
      bool cvt = a->props.getBool("convert", false);
      if (cvt && texconvcache::get_tex_asset_built_ddsx(*a, buf, _MAKE4C('PC'), nullptr, conWriter))
        return dag::Span<char>((char *)buf.ptr, buf.len);
      else if (!cvt)
      {
        String fname(a->getTargetFilePath());
        const char *ext = dd_get_fname_ext(fname);
        if (ext && stricmp(ext, ".dds") == 0)
          if (convert_dds_to_ddsx(a, fname, buf))
            return dag::Span<char>((char *)buf.ptr, buf.len);
      }
    }
    return {};
  }
  void releaseDDSxTexData(dag::Span<char> data) override
  {
    ddsx::Buffer buf;
    buf.ptr = data.data();
    buf.free();
  }

protected:
  DagorAssetMgr *assetMgr = nullptr;
  ILogWriter *conWriter = nullptr;

  inline DagorAsset *find_asset_by_name_id(unsigned name_id) const
  {
    for (DagorAsset *a : assetMgr->getAssets())
      if (a->getNameId() == name_id && a->getType() == assetMgr->getTexAssetTypeId())
        return a;
    return nullptr;
  }

  static bool convert_dds_to_ddsx(DagorAsset *a, const char *fname, ddsx::Buffer &out_buf)
  {
    file_ptr_t fp = df_open(fname, DF_READ);
    if (!fp)
    {
      logerr("can't read ERR: cannot open <%s>", fname);
      return false;
    }
    SmallTab<char, TmpmemAlloc> dds;
    dds.resize(df_length(fp));
    df_read(fp, dds.data(), dds.size());
    df_close(fp);

    TextureMetaData tmd;
    tmd.read(a->props, a->resolveEffProfileTargetStr("PC", nullptr));

    ddsx::ConvertParams cp;
    cp.canPack = false;
    cp.packSzThres = 256 << 20;
    cp.addrU = tmd.d3dTexAddr(tmd.addrU);
    cp.addrV = tmd.d3dTexAddr(tmd.addrV);
    cp.hQMip = tmd.hqMip;
    cp.mQMip = tmd.mqMip;
    cp.lQMip = tmd.lqMip;
    cp.imgGamma = a->getProfileTargetProps(_MAKE4C('PC'), nullptr).getReal("gamma", a->props.getReal("gamma", 2.2));

    if (ddsx::convert_dds(_MAKE4C('PC'), out_buf, dds.data(), data_size(dds), cp))
      return true;
    logerr("can't convert texture <%s>: %s", fname, ddsx::get_last_error_text());
    return false;
  }
};

static TexConvBuildOnDemandHelper *helper = nullptr;
static TextureFactory *tex_factory = nullptr;

bool texconvcache::init_build_on_demand_tex_factory(DagorAssetMgr &mgr, ILogWriter *con)
{
  helper = new TexConvBuildOnDemandHelper(mgr, con);
  tex_factory = build_on_demand_tex_factory::init(helper);
  if (tex_factory)
    ::set_default_tex_factory(tex_factory);
  return tex_factory != nullptr;
}
void texconvcache::term_build_on_demand_tex_factory()
{
  build_on_demand_tex_factory::term(tex_factory);
  del_it(helper);
  tex_factory = nullptr;
}
bool texconvcache::build_on_demand_tex_factory_cease_loading(bool en)
{
  return build_on_demand_tex_factory::cease_loading(tex_factory, en);
}
bool texconvcache::get_tex_factory_current_loading_state(AsyncTextureLoadingState &out_state)
{
  return build_on_demand_tex_factory::get_current_loading_state(tex_factory, out_state);
}

void texconvcache::build_on_demand_tex_factory_limit_tql(TexQL max_tql, const TextureIdSet &tid)
{
  build_on_demand_tex_factory::limit_tql(tex_factory, max_tql, tid);
}
void texconvcache::build_on_demand_tex_factory_limit_tql(TexQL max_tql)
{
  build_on_demand_tex_factory::limit_tql(tex_factory, max_tql);
}

void texconvcache::schedule_prebuild_tex(DagorAsset *tex_a, TexQL ql)
{
  G_ASSERT(tex_factory != nullptr);
  build_on_demand_tex_factory::schedule_prebuild_tex(tex_factory, tex_a->getNameId(), ql);
}

static bool tex_loading_was_enabled_before_reset = false;
static void texconv_bft_before_reset(bool full_reset)
{
  if (full_reset)
    tex_loading_was_enabled_before_reset = build_on_demand_tex_factory::cease_loading(tex_factory, false);
}
static void texconv_bft_after_reset(bool full_reset)
{
  if (full_reset && tex_loading_was_enabled_before_reset)
    build_on_demand_tex_factory::cease_loading(tex_factory, true);
}

#include <drv/3d/dag_resetDevice.h>
REGISTER_D3D_BEFORE_RESET_FUNC(texconv_bft_before_reset);
REGISTER_D3D_AFTER_RESET_FUNC(texconv_bft_after_reset);
