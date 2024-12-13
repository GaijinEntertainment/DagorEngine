// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <util/dag_string.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/asset.h>
#include <assets/assetMgr.h>
#include <assets/assetExpCache.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <image/dag_loadImage.h>
#include <image/dag_texPixel.h>
#include <nvtt/nvtt.h>
#include <nvimage/image.h>
#include <nvimage/floatImage.h>
#include <nvimage/filter.h>
#include <nvcore/ptr.h>
#include <nvimage/directDrawSurface.h>
#include <nvmath/color.h>
#include <vecmath/dag_vecMathDecl.h>
#include <libTools/util/iesReader.h>
#include <util/dag_base32.h>
#include <math/dag_Point3.h>
#include "roughnessMipMapping.h"
#if !TEX_CANNOT_USE_ISPC
#include <ispc_texcomp.h>
#endif
#include <libTools/dtx/astcenc.h>
#if !_TARGET_STATIC_LIB
ASTCENC_DECLARE_STATIC_DATA();
#endif
static unsigned astcenc_jobs_limit = 0;

#include <3d/ddsFormat.h>
#undef ERROR // after #include <supp/_platform.h>
#include <ioSys/dag_ioUtils.h>
#include <util/dag_texMetaData.h>
#include <util/dag_hierBitMap2d.h>
#include <startup/dag_startupTex.h>
#include <math/dag_adjpow2.h>
#include <math/dag_color.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <debug/dag_debug.h>
#include <debug/dag_log.h>
#include <libTools/dtx/makeDDS.h>
#include <3d/ddsxTex.h>

#include <math/dag_imageFunctions.h>

#include <render/renderLightsConsts.hlsli>

BEGIN_DABUILD_PLUGIN_NAMESPACE(tex)

static const char *TYPE = "tex";
static DataBlock appBlkCopy;
static DataBlock blkAssetsBuildTex;
static bool texRtMipGenAllowed = true;
static bool preferZstdPacking = false, allowOodlePacking = false;
static bool pcAllowsASTC = false, pcAllowsASTC_to_ARGB = false;
static int(__stdcall *write_built_dds_final)(IGenSave &, DagorAsset &, unsigned, const char *, ILogWriter *) = NULL;
static DataBlock *buildResultsBlk = NULL;

template <class T>
inline void swap(T &a, T &b)
{
  T c = a;
  a = b;
  b = c;
}
static int addr(int i, int w, bool clamp) { return clamp ? max(0, min(i, w - 1)) : (i < 0 ? w + i : i % w); }
static NormalLengthToAlpha lenToAlphaLut(useMicroSurface);

struct FilterBase
{
  float *fdata;
  float gamma;

  FilterBase(int w, int h, float g)
  {
    gamma = g;
    fdata = isGammaSpace() ? NULL : (float *)memalloc(align_4k(w * h * 16), tmpmem);
  }
  ~FilterBase() { memfree(fdata, tmpmem); }

  bool isGammaSpace() { return fabsf(gamma - 1.0f) < 0.01f; }
  static int align_4k(int sz) { return (sz + 0xFFF) & ~0xFFF; }

  template <class Filter>
  static void create_mip_chain(nv::Image *image, dag::ConstSpan<nv::Image *> imageMips, Filter &f)
  {
    int w = image->width(), h = image->height();

    if (f.isGammaSpace())
    {
      nv::Image *last = image;
      for (int i = 0; i < imageMips.size(); ++i)
      {
        w >>= 1, h >>= 1;
        f.downsample4x((unsigned char *)imageMips[i]->pixels(), (unsigned char *)last->pixels(), w, h);
        last = imageMips[i];
      }
    }
    else
    {
      imagefunctions::exponentiate4_c((unsigned char *)image->pixels(), f.fdata, w * h, f.gamma);
      float invgamma = 1.0f / f.gamma;

      for (int i = 0; i < imageMips.size(); ++i)
      {
        w >>= 1, h >>= 1;
        f.downsample4x(f.fdata, f.fdata, w, h);
        imagefunctions::convert_to_linear_simda(f.fdata, (unsigned char *)imageMips[i]->pixels(), (w && h) ? w * h : w + h, invgamma);
      }
    }
  }
};

struct BoxFilter : public FilterBase
{
public:
  BoxFilter(int w, int h, float gamma) : FilterBase(w, h, gamma) {}

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

  static inline void downsample4x(float *destData, float *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else
      imagefunctions::downsample4x_simda(destData, srcData, destW, destH);
  }
};

struct KaiserFilter : public FilterBase
{
  float alpha, stretch;
  void *temp;
  vec4f kernel[4];

  KaiserFilter(int w, int h, float gamma, float a, float s) : FilterBase(w, h, gamma), alpha(a), stretch(s)
  {
    imagefunctions::prepare_kaiser_kernel(alpha, stretch, (float *)&kernel[0]);
    temp = memalloc(align_4k(isGammaSpace() ? w * h * 4 / 2 : w * h * 16 / 2), tmpmem);
  }
  ~KaiserFilter() { memfree(temp, tmpmem); }

  inline void downsample4x(unsigned char *destData, unsigned char *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else if (destW <= 8 && destH <= 8)
      imagefunctions::downsample4x_simdu(destData, srcData, destW, destH);
    else
      imagefunctions::kaiser_downsample(destW, destH, srcData, destData, (float *)kernel, temp);
  }

  inline void downsample4x(float *destData, float *srcData, int destW, int destH)
  {
    if (!destW || !destH)
      imagefunctions::downsample2x_simdu(destData, srcData, destW + destH);
    else if (destW <= 8 && destH <= 8)
      imagefunctions::downsample4x_simda(destData, srcData, destW, destH);
    else
      imagefunctions::kaiser_downsample(destW, destH, srcData, destData, (float *)kernel, temp);
  }
};

static TexImage32 *create_ies_image(DagorAsset &a, ILogWriter &log, const IesReader::IesParams &params, const TextureMetaData &tmd)
{
  IesReader ies = IesReader(params);
  if (!ies.read(a.getTargetFilePath(), log))
    return nullptr;

  int width = a.props.getInt("textureWidth", -1);
  int height = a.props.getInt("textureHeight", -1);

  if (width == -1 || height == -1)
  {
    log.addMessage(ILogWriter::WARNING, "The resolution of the photometry textures is not set (textureWidth; "
                                        "textureHeight).");
    width = width != -1 ? width : 128;
    height = height != -1 ? height : 128;
  }

  IesReader::TransformParams transformParams;
  transformParams.zoom = tmd.getIesScale();
  transformParams.rotated = bool(tmd.flags & tmd.FLG_IES_ROT);
#if USE_OCTAHEDRAL_MAPPING
  IesReader::TransformParams optimalParams = ies.getOctahedralOptimalTransform(width, height);
  bool isValid = ies.isOctahedralTransformValid(width, height, transformParams);
  float waste = ies.getOctahedralRelativeWaste(width, height, transformParams);
#else
  IesReader::TransformParams optimalParams = ies.getSphericalOptimalTransform(width, height);
  bool isValid = ies.isSphericalTransformValid(width, height, transformParams);
  float waste = ies.getSphericalRelativeWaste(width, height, transformParams);
#endif

  if (!isValid)
  {
    log.addMessage(log.ERROR,
      "Invalid transform params for ies texture: <%s>. The optimum is "
      "iesScale:r=%.5f, "
      "iesRotation:b=%s",
      a.getName(), floor(optimalParams.zoom * 10000) / 10000, optimalParams.rotated ? "yes" : "no");
    return nullptr;
  }
  if (waste >= 0.3f)
  {
    log.addMessage(log.WARNING,
      "Suboptimal transform params for ies texture: <%s>. The optimum is "
      "iesScale:r=%.5f, "
      "iesRotation:b=%s",
      a.getName(), floor(optimalParams.zoom * 10000) / 10000, optimalParams.rotated ? "yes" : "no");
  }

#if USE_OCTAHEDRAL_MAPPING
  IesReader::ImageData img = ies.generateOctahedral(width, height, transformParams);
#else
  IesReader::ImageData img = ies.generateSpherical(width, height, transformParams);
#endif

  TexImage32 *ret = TexImage32::create(img.width, img.height, nullptr);
  for (int i = 0; i < img.height; i++)
  {
    for (int j = 0; j < img.width; ++j)
    {
      IesReader::Real intensity = img.data[i * img.width + j];
      // ApplySRGBCurve_Fast
      intensity = intensity < 0.0031308 ? 12.92 * intensity : 1.13005 * sqrt(intensity - 0.00228) - 0.13448 * intensity + 0.005719;
      uint8_t value = uint8_t(intensity * eastl::numeric_limits<uint8_t>::max() + 0.5);
      TexPixel32 texel;
      texel.r = texel.g = texel.b = texel.a = value;
      ret->getPixels()[i * img.width + j] = texel;
    }
  }
  return ret;
}

inline uint8_t convert_wt_smoothness_to_inv_roughness(uint8_t &s)
{
  double sm = s / 255.0f;
  double r = (1 - 0.7 * sm);
  r = r * r * r; // Crytek formula
  // WT:  r*=r;
  // linearization is sqrt(r)
  // linearization(WT(r)) == crytek
  return s = clamp(int(255. - r * 255.0), 0, 255);
}

class TexExporter : public IDagorAssetExporter //-V553
{
  enum ImgBitFormat
  {
    IMG_BITFMT__NONE = 0,
    IMG_BITFMT_R8,
    IMG_BITFMT_R8G8,
    IMG_BITFMT_A8R8,
    IMG_BITFMT_RGB565,
    IMG_BITFMT_RGBA5551,
    IMG_BITFMT_RGBA4444,
  };

  enum BC67Format
  {
    BC67_FORMAT_NONE,
    BC67_FORMAT_BC6H,
    BC67_FORMAT_BC7
  };

  struct ImageSurface
  {
    nv::AutoPtr<nv::Image> image, image0;
    TexImage32 *img = nullptr;
    Tab<nv::Image *> imageMips;
    ImageSurface() { imageMips.reserve(16); }
    ~ImageSurface()
    {
      clear_all_ptr_items_and_shrink(imageMips);
      memfree(img, tmpmem);
    }
    ImageSurface(ImageSurface &&is)
    {
      memcpy(this, &is, sizeof(*this));
      memset(&is, 0, sizeof(is));
    }
  };

public:
  virtual const char *__stdcall getExporterIdStr() const { return "tex exp"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }
  virtual unsigned __stdcall getGameResClassId() const { return 0; }
  virtual unsigned __stdcall getGameResVersion() const { return preferZstdPacking ? (allowOodlePacking ? 11 : 10) : 9; }

  void __stdcall onRegister() override { buildResultsBlk = NULL; }
  void __stdcall onUnregister() override { buildResultsBlk = NULL; }
  void __stdcall setBuildResultsBlk(DataBlock *b) override { buildResultsBlk = b; }

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();

#define GET_PROP(TYPE, PROP, DEF) a.props.get##TYPE(PROP, DEF)
    if (const char *base_tex = GET_PROP(Str, "baseTex", NULL))
    {
      DagorAsset *a_base = a.getMgr().findAsset(base_tex, a.getType());
      if (a_base)
        files.push_back() = a_base->getTargetFilePath();
      else
      {
        logerr("missing tex asset <%s> (to be used as base for <%s>)", base_tex, a.getName());
        files.push_back() = String(0, "*missing*%s", base_tex);
      }
    }
    if (GET_PROP(Bool, "baseImageAlphaPatch", false))
    {
      const char *bimg_nm = GET_PROP(Str, "baseImage", NULL);
      if (bimg_nm && *bimg_nm)
      {
        String bfn(a.getFolderPath());
        if (bimg_nm[0] == '*')
        {
          bfn.resize(DAGOR_MAX_PATH);
          dd_get_fname_location(bfn, a.getTargetFilePath());
          bfn.updateSz();
          bimg_nm++;
        }

        if (bfn.length())
          bfn.aprintf(0, "/%s", bimg_nm);
        else
          bfn = bimg_nm;
        simplify_fname(bfn);
        files.push_back() = bfn;
      }
    }
#undef GET_PROP
  }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    if (int max_sz = a.props.getInt("thumbnailMaxDataSize", 0))
    {
      int aname_len = i_strlen(a.getName());
      if (!write_built_dds_final || aname_len < 4 || strcmp(a.getName() + aname_len - 3, "$tq") != 0)
      {
        log.addMessage(log.ERROR, "%s: internal error, thumbnailMaxDataSize=%d while write_built_dds_final=%p or wrong asset",
          a.getName(), max_sz, (void *)write_built_dds_final);
        return false;
      }
      DagorAsset *ma = a.getMgr().findAsset(String(0, "%.*s", aname_len - 3, a.getName()), a.getType());
      if (!ma)
      {
        log.addMessage(log.ERROR, "%s: internal error, failed to find main asset", a.getName());
        return false;
      }

      mkbindump::BinDumpSaveCB mcwr(17 << 20, cwr.getTarget(), false);
      write_built_dds_final(mcwr.getRawWriter(), *ma, cwr.getTarget(), cwr.getProfile(), &log);

      MemoryLoadCB mcrd(mcwr.getRawWriter().takeMem(), true);
      DDSURFACEDESC2 base_dds_hdr;
      mcrd.seekto(4);
      mcrd.read(&base_dds_hdr, sizeof(base_dds_hdr));
      uint32_t rest_size = mcrd.getTargetDataSize() - sizeof(base_dds_hdr) - 4;

      enum
      {
        FORCETQ_no = 0,
        FORCETQ_use_base = 1,
        FORCETQ_downsample_base = -1
      };
      int force_tq = a.props.getInt("forceTQ", FORCETQ_no);

      ddsx::Header hdr;
      if ((rest_size <= max_sz && force_tq == FORCETQ_no) || !make_ddsx_hdr(hdr, base_dds_hdr))
      {
        // unsupported format
        if (force_tq == FORCETQ_no)
          store_empty_dds(cwr);
        else
        {
          mcrd.seekto(0);
          copy_stream_to_stream(mcrd, cwr.getRawWriter(), mcrd.getTargetDataSize());
        }
        return true;
      }

      if (force_tq == FORCETQ_downsample_base && !ma->props.getBool("convert", false))
        force_tq = FORCETQ_use_base;

      if (base_dds_hdr.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
      {
        int face_size = rest_size / 6, skip_size = 0;
        G_ASSERTF(face_size * 6 == rest_size, "rest_size=%d", rest_size);
        for (int l = 0; l < hdr.levels; l++)
        {
          if ((face_size - skip_size) * 6 <= max_sz || (force_tq == FORCETQ_use_base && l + 1 == hdr.levels))
          {
            base_dds_hdr.dwWidth = max(hdr.w >> l, 1);
            base_dds_hdr.dwHeight = max(hdr.h >> l, 1);
            base_dds_hdr.dwMipMapCount = hdr.levels - l;
            cwr.writeFourCC(MAKEFOURCC('D', 'D', 'S', ' '));
            cwr.writeRaw(&base_dds_hdr, sizeof(base_dds_hdr));
            for (int a = 0; a < 6; a++)
            {
              mcrd.seekrel(skip_size);
              copy_stream_to_stream(mcrd, cwr.getRawWriter(), face_size - skip_size);
            }
            return true;
          }
          skip_size += hdr.getSurfaceSz(l);
        }

        if (force_tq == FORCETQ_no)
        {
          store_empty_dds(cwr);
          return true;
        }
      }

      auto downsample_tex = [&]() {
        uintptr_t a_storage[sizeof(DagorAsset) / sizeof(uintptr_t)];
        memcpy(a_storage, ma, sizeof(a_storage)); //-V780
        G_STATIC_ASSERT(sizeof(a_storage) == sizeof(DagorAsset));

        DagorAsset &cloned_ma = *(DagorAsset *)a_storage;
        new (&cloned_ma.props, _NEW_INPLACE) DataBlock(ma->props);
        cloned_ma.props.removeParam("baseTex");
        cloned_ma.props.removeParam("rtMipGen");
        cloned_ma.props.removeParam("rtMipGenBQ");
        cloned_ma.props.setInt("maxTexSz", get_bigger_pow2(int(sqrt(max_sz * 2))));
        cloned_ma.props.setInt("mipCnt", -1);
        if (ma->props.getBool("baseTexAlphaPatch", false) && ma->props.getStr("baseTex", nullptr))
        {
          cloned_ma.props.setStr("baseImage", String(0, "@%s", ma->props.getStr("baseTex")));
          cloned_ma.props.setBool("baseImageAlphaPatch", true);
        }

        mcwr.reset(max_sz * 8);
        bool build_ok = buildTexAsset(cloned_ma, mcwr, log);
        cloned_ma.props.~DataBlock();

        if (!build_ok)
          return false;

        mcrd.setMem(mcwr.getRawWriter().takeMem(), true);
        mcrd.seekto(4);
        mcrd.read(&base_dds_hdr, sizeof(base_dds_hdr));
        rest_size = mcrd.getTargetDataSize() - sizeof(base_dds_hdr) - 4;
        return true;
      };

      if (hdr.levels == 1 && ma->props.getBool("rtMipGen", false) && ma->props.getBool("convert", false) &&
          !(base_dds_hdr.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) &&
          (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == D3DFMT_DXT5 || hdr.d3dFormat == D3DFMT_DXT3 ||
            hdr.d3dFormat == D3DFMT_A8R8G8B8 || hdr.d3dFormat == D3DFMT_A8B8G8R8 || hdr.d3dFormat == D3DFMT_X8R8G8B8 ||
            force_tq == FORCETQ_downsample_base))
      {
        if (downsample_tex() && make_ddsx_hdr(hdr, base_dds_hdr))
        {
          if (force_tq == FORCETQ_downsample_base)
            force_tq = FORCETQ_use_base;
        }
        else
        {
          store_empty_dds(cwr);
          return true;
        }
      }

      auto store_thumb_mips = [&]() {
        for (int l = 0; l < hdr.levels; l++)
        {
          if (rest_size <= max_sz || (force_tq == FORCETQ_use_base && l + 1 == hdr.levels))
          {
            base_dds_hdr.dwWidth = max(hdr.w >> l, 1);
            base_dds_hdr.dwHeight = max(hdr.h >> l, 1);
            if (base_dds_hdr.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME)
              base_dds_hdr.dwDepth = max(hdr.depth >> l, 1);
            base_dds_hdr.dwMipMapCount = hdr.levels - l;
            cwr.writeFourCC(MAKEFOURCC('D', 'D', 'S', ' '));
            cwr.writeRaw(&base_dds_hdr, sizeof(base_dds_hdr));
            copy_stream_to_stream(mcrd, cwr.getRawWriter(), rest_size);
            return true;
          }
          uint32_t surf_sz = hdr.getSurfaceSz(l);
          if (base_dds_hdr.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME)
            surf_sz *= max(hdr.depth >> l, 1);
          rest_size -= surf_sz;
          mcrd.seekrel(surf_sz);
        }
        return false;
      };

      if (store_thumb_mips())
        return true;

      if (force_tq == FORCETQ_downsample_base)
        if (downsample_tex() && make_ddsx_hdr(hdr, base_dds_hdr))
        {
          force_tq = FORCETQ_use_base;
          if (store_thumb_mips())
            return true;
        }

      // no thumbnail mips
      store_empty_dds(cwr);
      return true;
    }
    return buildTexAsset(a, cwr, log);
  }
  bool getAssetSourceHash(SimpleString &dest_hash, DagorAsset &a, void *cache_shared_data_ptr, unsigned target_code) override
  {
    AssetExportCache::setSharedDataPtr(cache_shared_data_ptr);
    unsigned char hash[15];
    memset(hash, 0, sizeof(hash));
    bool ret = AssetExportCache::sharedDataGetFileHash(a.getTargetFilePath(), hash);
    AssetExportCache::setSharedDataPtr(nullptr);
    if (ret)
    {
      String fmt_spec;
      const DataBlock &props = a.getProfileTargetProps(target_code, nullptr);

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
      const char *fmt = GET_PROP(Str, "fmt", "ARGB");
      if (const DataBlock *b = blkAssetsBuildTex.getBlockByName(mkbindump::get_target_str(target_code)))
        fmt = b->getBlockByNameEx("fmtSubst")->getStr(fmt, fmt);

      fmt_spec.aprintf(0, "%s;", fmt);
      fmt_spec.aprintf(0, "%s;", GET_PROP(Str, "swizzleARGB", "ARGB"));
      fmt_spec.aprintf(0, "%s;", GET_PROP(Str, "texType", ""));
      fmt_spec.aprintf(0, "%d;%d;%d;%d;%d;%d", GET_PROP(Bool, "valveAlpha", false), GET_PROP(Int, "valveAlphaFilter", 4),
        GET_PROP(Int, "valveAlphaThres", GET_PROP(Int, "alphaThres", 128)), GET_PROP(Int, "valveAlphaUpscale", 4),
        GET_PROP(Bool, "alphaCoverage", false), max(GET_PROP(Int, "valveAlphaAfterCoverage", 1025) - 1, 0));
#undef GET_PROP
      AssetExportCache::sharedDataAppendHash(fmt_spec.data(), fmt_spec.length(), hash);
      data_to_str_b32(dest_hash, hash, 15);
    }
    return ret;
  }
  static void removeDescRecord(DagorAsset &a, unsigned tc)
  {
    if (buildResultsBlk->blockCount())
    {
      const char *pkname = a.getCustomPackageName(mkbindump::get_target_str(tc), nullptr);
      if (DataBlock *b = buildResultsBlk->getBlockByName(pkname ? pkname : "."))
        b->removeBlock(a.getName());
    }
  }
  bool updateBuildResultsBlk(DagorAsset &a, void *cache_shared_data_ptr, unsigned tc) override
  {
    G_ASSERT_RETURN(buildResultsBlk, true);
    if (const char *p = strrchr(a.getName(), '$'))
      if (strcmp(p, "$hq") == 0 || strcmp(p, "$uhq") == 0)
      {
        removeDescRecord(a, tc);
        return true;
      }
    DagorAsset *a_uhq = a.getMgr().findAsset(String::mk_str_cat(a.getName(), "$uhq"));
    if (!a_uhq || !a_uhq->props.getBool("useIndividualPackWithSrcHash", false))
    {
      removeDescRecord(a, tc);
      return true;
    }

    SimpleString hash;
    if (getAssetSourceHash(hash, a, cache_shared_data_ptr, tc))
    {
      const char *pkname = a.getCustomPackageName(mkbindump::get_target_str(tc), nullptr);
      buildResultsBlk->addBlock(pkname ? pkname : ".")->addBlock(a.getName())->setStr("src", hash);
    }
    // debug("updateBuildResultsBlk(%s) %s", a.getName(), hash);
    return true;
  }

  enum
  {
    MMF_none,
    MMF_point,
    MMF_stripe,
    MMF_box,
    MMF_kaizer
  };
  virtual int __stdcall getBqTexResolution(DagorAsset &a, unsigned target, const char *profile, ILogWriter &log)
  {
    if (a.props.getInt("thumbnailMaxDataSize", 0))
    {
      return 32;
    }
#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
    const DataBlock &props = a.getProfileTargetProps(target, profile);
    const char *texTypeStr = GET_PROP(Str, "texType", nullptr);
    if (!a.props.getBool("convert", false) || (texTypeStr && (strcmp(texTypeStr, "tex2D") != 0)))
      return 512;
    TexImage32 *img = nullptr;
    bool iesTexture = GET_PROP(Bool, "buildIES", false);
    int mipCustomFilt = MMF_none;
    bool alpha_used = false;
    int mipCnt = 0;
    TextureMetaData tmd;
    tmd.read(a.props, &a.props == &props ? "" : props.getBlockName());
    String targetFilePath(a.getTargetFilePath());
    if (iesTexture)
    {
      IesReader::IesParams iesParams;
      iesParams.blurRadius = GET_PROP(Real, "blurRadius", iesParams.blurRadius * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.phiMin = GET_PROP(Real, "phiMin", iesParams.phiMin * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.phiMax = GET_PROP(Real, "phiMax", iesParams.phiMax * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.thetaMin = GET_PROP(Real, "thetaMin", iesParams.thetaMin * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.thetaMax = GET_PROP(Real, "thetaMax", iesParams.thetaMax * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.edgeFadeout = GET_PROP(Real, "edgeFadeout", iesParams.edgeFadeout * RAD_TO_DEG) * DEG_TO_RAD;
      img = create_ies_image(a, log, iesParams, tmd);
    }
    else
      img = ::load_image(targetFilePath, tmpmem, &alpha_used);
    if (!img && dd_get_fname_ext(targetFilePath) && dd_stricmp(dd_get_fname_ext(targetFilePath), ".dds") == 0)
    {
      bool allow_mipstripe = GET_PROP(Bool, "useDDSmips", true);
      int initial_mipCnt = mipCnt;
      img = decode_image_from_dds(a, allow_mipstripe, mipCnt, mipCustomFilt, alpha_used, log);
      if (allow_mipstripe && initial_mipCnt != mipCnt && mipCnt > 0)
        mipCnt -= tmd.hqMip;
    }
    if (!img)
      return 512;
    int w = img->w;
    int h = img->h;
    memfree(img, tmpmem);
    int splitAt = GET_PROP(Int, "splitAt", 0);
    bool splitOverrided = GET_PROP(Bool, "splitAtOverride", false);
    if (!splitOverrided)
      splitAt = a.props.getBlockByNameEx(mkbindump::get_target_str(target))->getInt("splitAtSize", splitAt);

    if (!splitAt)
      return min(w, h);

    while (w > splitAt || h > splitAt)
    {
      w >>= 1;
      h >>= 1;
    }
    return min(w, h);
#undef GET_PROP
  }

  bool buildTexAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log, bool mipmap_none = false,
    Tab<TexPixel32> *out_base_img = NULL)
  {
    if (out_base_img && !write_built_dds_final)
    {
      log.addMessage(log.ERROR, "%s: internal error, write_built_dds_final=NULL", a.getName());
      return false;
    }

    if (buildResultsBlk)
      updateBuildResultsBlk(a, nullptr, cwr.getTarget());

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
    G_ASSERT(!cwr.WRITE_BE);
    G_ASSERT(a.props.getBool("convert", false));

    enum AutoAlphaFmt
    {
      AAF_off = 0,
      AAF_dxt1dxt5,
      AAF_dxt1bc7,
      AAF_astcXA
    };
    AutoAlphaFmt auto_alpha_fmt = AAF_off;
    const DataBlock &props = a.getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
    const char *swizzle = GET_PROP(Str, "swizzleARGB", "ARGB");
    const char *fmt = GET_PROP(Str, "fmt", "ARGB");
    bool postCopyAtoR = GET_PROP(Bool, "postCopyAtoR", false);
    const char *quality = GET_PROP(Str, "quality", "");
    bool iesTexture = GET_PROP(Bool, "buildIES", false);
    ImgBitFormat pix_format = IMG_BITFMT__NONE;

    nvtt::TextureType texType = nvtt::TextureType_2D;
    const char *texTypeStr = GET_PROP(Str, "texType", nullptr);
    if (!texTypeStr || strcmp(texTypeStr, "tex2D") == 0)
      texType = nvtt::TextureType_2D;
    else if (strcmp(texTypeStr, "cube") == 0)
      texType = nvtt::TextureType_Cube;
    else if (strcmp(texTypeStr, "tex3D") == 0)
      texType = nvtt::TextureType_3D;
    else
    {
      log.addMessage(log.ERROR, "%s: undefined texType=<%s>, supported are: %s, %s, %s", a.getName(), texTypeStr, "tex2D", "tex3D",
        "cube");
      return false;
    }

    bool rtMipGen = GET_PROP(Bool, "rtMipGen", false) && cwr.getTarget() == _MAKE4C('PC'); // for now only PC ddsx packer supports
                                                                                           // realtime mip generation
    if (rtMipGen && !texRtMipGenAllowed)
    {
      log.addMessage(log.ERROR, "%s: tex is rtMipGen, prohibited", a.getName());
      return false;
    }
    if (rtMipGen && texType != nvtt::TextureType_2D)
    {
      log.addMessage(log.ERROR, "%s: rtMipGen is not supported for texType=%s", a.getName(), texTypeStr);
      return false;
    }
    if (texType != nvtt::TextureType_2D && GET_PROP(Int, "splitAt", 0))
    {
      log.addMessage(log.ERROR, "%s: splitAt=%d is not supported for texType=%s", a.getName(), GET_PROP(Int, "splitAt", 0),
        texTypeStr);
      return false;
    }

    const char *mipmap = GET_PROP(Str, "mipmap", "gen");
    const char *mipFilt = GET_PROP(Str, "mipFilter", "");
    int mipCnt = GET_PROP(Int, "mipCnt", -1);
    const float mipFiltWidth = 3;
    const float mipFiltAlpha = GET_PROP(Real, "mipFilterAlpha", 4);
    const float mipFiltStretch = GET_PROP(Real, "mipFilterStretch", 1);
    const float gamma = GET_PROP(Real, "gamma", 2.2);

    const char *targetPlatform = mkbindump::get_target_str(cwr.getTarget());

    const DataBlock *targetBlock = blkAssetsBuildTex.getBlockByNameEx(targetPlatform);

    const int defMaxTexSz = targetBlock->getInt("defMaxTexSz", blkAssetsBuildTex.getInt("defMaxTexSz", 0));
    // maxTexSz from .folder.blk for the current platform should be in the priority
    const int maxTexSz = props.getBlockByNameEx(targetPlatform)->getInt("maxTexSz", GET_PROP(Int, "maxTexSz", defMaxTexSz));

    const int defMipBias = targetBlock->getInt("mipBias", blkAssetsBuildTex.getInt("mipBias", 0));
    const int mipBias = props.getBlockByNameEx(targetPlatform)->getInt("mipBias", GET_PROP(Int, "mipBias", defMipBias));

    const E3DCOLOR diff_tolerance = GET_PROP(E3dcolor, "baseTexTolerance", E3DCOLOR(0, 0, 0, 0));
    bool ablend_btex = GET_PROP(Bool, "baseTexAlphaPatch", false);
    int mipCustomFilt = MMF_none;
    bool mipStripeProduced = false;

    bool mipValveAlpha = GET_PROP(Bool, "valveAlpha", false);
    int valveAlphaFilter = GET_PROP(Int, "valveAlphaFilter", 4);
    int valveAlphaThres = GET_PROP(Int, "valveAlphaThres", GET_PROP(Int, "alphaThres", 128));
    int valveAlphaUpscale = GET_PROP(Int, "valveAlphaUpscale", 4);
    bool valveAlphaWrapU = GET_PROP(Bool, "valveAlphaWrapU", false);
    bool valveAlphaWrapV = GET_PROP(Bool, "valveAlphaWrapV", false);

    bool mipAlphaCoverage = GET_PROP(Bool, "alphaCoverage", false);
    int mipValveAlphaAfterCoverage = max(GET_PROP(Int, "valveAlphaAfterCoverage", 1025) - 1, 0);
    int mipValveAlphaSkipLast = GET_PROP(Int, "mipValveAlphaSkipLast", 0);
    static const int A_REF_MIP_CNT = 16;
    float a_ref_mip[A_REF_MIP_CNT];
    float a_coverage[A_REF_MIP_CNT];

    if (mipAlphaCoverage && texType != nvtt::TextureType_2D)
    {
      log.addMessage(log.ERROR, "%s: alphaCoverage is not supported for texType=%s", a.getName(), texTypeStr);
      return false;
    }
    a_ref_mip[0] = GET_PROP(Int, "alphaCoverageThresMip0", GET_PROP(Int, "alphaThres", 127)) / 255.0f;
    for (int i = 1; i < A_REF_MIP_CNT; i++)
      a_ref_mip[i] = GET_PROP(Int, String(0, "alphaCoverageThresMip%d", i), a_ref_mip[i - 1] * 255.0f) / 255.0f;
    memset(a_coverage, 0, sizeof(a_coverage));

    if (const DataBlock *b = blkAssetsBuildTex.getBlockByName(targetPlatform))
      fmt = b->getBlockByNameEx("fmtSubst")->getStr(fmt, fmt);

    nvtt::Compressor compressor;
    nvtt::InputOptions inpOptions;
    nvtt::CompressionOptions comprOptions;
    nvtt::OutputOptions outOptions;
    ErrorHandler errHandler;
    OutputHandler outHandler;

    compressor.enableCudaAcceleration(false);
    outOptions.setErrorHandler(&errHandler);
    outOptions.setOutputHandler(&outHandler);

    inpOptions.setGamma(gamma, gamma); //== will change all non-gamma-correct textures

    if (strcmp(quality, "") == 0)
      comprOptions.setQuality(nvtt::Quality_Production);
    else if (strcmp(quality, "fastest") == 0)
      comprOptions.setQuality(nvtt::Quality_Fastest);
    else if (strcmp(quality, "normal") == 0)
      comprOptions.setQuality(nvtt::Quality_Normal);
    else if (strcmp(quality, "production") == 0)
      comprOptions.setQuality(nvtt::Quality_Production);
    else if (strcmp(quality, "highest") == 0)
      comprOptions.setQuality(nvtt::Quality_Highest);

    unsigned tc_format = 0;
    if (cwr.getTarget() == _MAKE4C('iOS') || cwr.getTarget() == _MAKE4C('and') || (cwr.getTarget() == _MAKE4C('PC') && pcAllowsASTC))
    {
      const char *suf = "";
      if (stricmp(fmt, "ASTC") == 0)
        tc_format = 0x114, auto_alpha_fmt = AAF_astcXA;
      if (strnicmp(fmt, "ASTC:4", 6) == 0)
        tc_format = 0x114, suf = fmt + 6;
      else if (strnicmp(fmt, "ASTC:8", 6) == 0)
        tc_format = 0x118, suf = fmt + 6;
      else if (strnicmp(fmt, "ASTC:12", 7) == 0)
        tc_format = 0x11C, suf = fmt + 7;

      static char new_swizzle[16];
      memset(new_swizzle, 0, sizeof(new_swizzle));
      strncpy(new_swizzle, swizzle, 15);
      new_swizzle[15] = 0;
      if (!suf[0] || strcmp(suf, ":rgba") == 0)
        ; // do nothing
      else if (strcmp(suf, ":r") == 0)
      {
        if (postCopyAtoR)
          new_swizzle[1] = new_swizzle[2] = new_swizzle[3] = '0';
        else
          new_swizzle[0] = '1', new_swizzle[2] = '0', new_swizzle[3] = '0';
      }
      else if (strcmp(suf, ":rg") == 0)
        new_swizzle[0] = '1', new_swizzle[3] = '0';
      else if (strcmp(suf, ":rgb") == 0)
        new_swizzle[0] = '1';

      if (strcmp(swizzle, new_swizzle) != 0)
        swizzle = new_swizzle;
    }

    BC67Format bc67_fmt = BC67_FORMAT_NONE;

    if (stricmp(fmt, "ARGB") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA);
    else if (stricmp(fmt, "RGB") == 0)
      comprOptions.setFormat(nvtt::Format_RGB);
    else if (tc_format)
      ; //= do nothing here
    else if (stricmp(fmt, "DXT1") == 0)
      comprOptions.setFormat(nvtt::Format_DXT1);
    else if (stricmp(fmt, "DXT1a") == 0)
      comprOptions.setFormat(nvtt::Format_DXT1a);
    else if (stricmp(fmt, "DXT3") == 0)
      comprOptions.setFormat(nvtt::Format_DXT3);
    else if (stricmp(fmt, "DXT5") == 0)
      comprOptions.setFormat(nvtt::Format_DXT5);
    else if (stricmp(fmt, "DXT1|DXT5") == 0)
      auto_alpha_fmt = AAF_dxt1dxt5;
    else if (stricmp(fmt, "DXT1|BC7") == 0)
      auto_alpha_fmt = AAF_dxt1bc7;
    else if (stricmp(fmt, "ATI1N") == 0 || stricmp(fmt, "BC4") == 0)
      comprOptions.setFormat(nvtt::Format_BC4);
    else if (stricmp(fmt, "ATI2N") == 0 || stricmp(fmt, "BC5") == 0)
      comprOptions.setFormat(nvtt::Format_BC5);
    else if (stricmp(fmt, "BC6H") == 0)
      bc67_fmt = BC67_FORMAT_BC6H;
    else if (stricmp(fmt, "BC7") == 0)
      bc67_fmt = BC67_FORMAT_BC7;
    else if (stricmp(fmt, "R8") == 0 || stricmp(fmt, "L8") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_R8;
    else if (stricmp(fmt, "A8R8") == 0 || stricmp(fmt, "A8L8") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_A8R8;
    else if (stricmp(fmt, "R8G8") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_R8G8;
    else if (stricmp(fmt, "RGB565") == 0 || stricmp(fmt, "R5G6B5") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_RGB565;
    else if (stricmp(fmt, "RGBA5551") == 0 || stricmp(fmt, "A1R5G5B5") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_RGBA5551;
    else if (stricmp(fmt, "RGBA4444") == 0 || stricmp(fmt, "A4R4G4B4") == 0)
      comprOptions.setFormat(nvtt::Format_RGBA), pix_format = IMG_BITFMT_RGBA4444;
    else
    {
      log.addMessage(log.ERROR, "%s: bad fmt=<%s>", a.getName(), fmt);
      return false;
    }

    if (!ablend_btex && (stricmp(fmt, "DXT1") == 0 || stricmp(fmt, "RGB") == 0))
    {
      // force ALPHA=1 for texture formats without alpha
      static char new_swizzle[16];
      memset(new_swizzle, 0, sizeof(new_swizzle));
      strncpy(new_swizzle, swizzle, 15);
      new_swizzle[15] = 0;
      new_swizzle[0] = '1';
      swizzle = new_swizzle;
    }

    if (mipCnt == 0)
      mipmap = "none";
    if (stricmp(mipmap, "gen") == 0)
      inpOptions.setMipmapGeneration(true, mipCnt > 0 ? mipCnt : -1);
    else if (stricmp(mipmap, "none") == 0)
    {
      inpOptions.setMipmapGeneration(false);
      mipCnt = 0;
    }
    else
    {
      log.addMessage(log.ERROR, "%s: bad mipmap=<%s>", a.getName(), mipmap);
      return false;
    }

    if (rtMipGen || stricmp(mipFilt, "") == 0 || texType == nvtt::TextureType_3D)
      mipCustomFilt = MMF_box, inpOptions.setMipmapFilter(nvtt::MipmapFilter_Box);
    else if (stricmp(mipFilt, "filterBox") == 0)
      mipCustomFilt = MMF_box, inpOptions.setMipmapFilter(nvtt::MipmapFilter_Box);
    else if (stricmp(mipFilt, "filterTriangle") == 0)
      inpOptions.setMipmapFilter(nvtt::MipmapFilter_Triangle);
    else if (stricmp(mipFilt, "filterKaiser") == 0)
    {
      mipCustomFilt = MMF_kaizer;
      inpOptions.setMipmapFilter(nvtt::MipmapFilter_Kaiser);
      inpOptions.setKaiserParameters(mipFiltWidth, mipFiltAlpha, mipFiltStretch);
    }
    else if (stricmp(mipFilt, "filterPoint") == 0)
      mipCustomFilt = MMF_point;
    else if (stricmp(mipFilt, "filterStripe") == 0)
      mipCustomFilt = MMF_stripe;
    else
    {
      log.addMessage(log.ERROR, "%s: bad mipFilter=<%s>", a.getName(), mipFilt);
      return false;
    }

    unsigned rs, gs, bs, as, orm, andm;
    if (!decodeSwizzleRGBA32(swizzle, rs, gs, bs, as, andm, orm))
    {
      log.addMessage(log.ERROR, "%s: bad swizzleARGB=<%s>", a.getName(), swizzle);
      return false;
    }

    TextureMetaData tmd;
    tmd.read(a.props, &a.props == &props ? "" : props.getBlockName());

    bool alpha_used = false;
    TexImage32 *img = nullptr;
    String targetFilePath(a.getTargetFilePath());
    if (iesTexture)
    {
      IesReader::IesParams iesParams;
      iesParams.blurRadius = GET_PROP(Real, "blurRadius", iesParams.blurRadius * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.phiMin = GET_PROP(Real, "phiMin", iesParams.phiMin * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.phiMax = GET_PROP(Real, "phiMax", iesParams.phiMax * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.thetaMin = GET_PROP(Real, "thetaMin", iesParams.thetaMin * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.thetaMax = GET_PROP(Real, "thetaMax", iesParams.thetaMax * RAD_TO_DEG) * DEG_TO_RAD;
      iesParams.edgeFadeout = GET_PROP(Real, "edgeFadeout", iesParams.edgeFadeout * RAD_TO_DEG) * DEG_TO_RAD;
      img = create_ies_image(a, log, iesParams, tmd);
    }
    else
      img = ::load_image(targetFilePath, tmpmem, &alpha_used);
    if (!img && dd_get_fname_ext(targetFilePath) && dd_stricmp(dd_get_fname_ext(targetFilePath), ".dds") == 0)
    {
      bool allow_mipstripe = GET_PROP(Bool, "useDDSmips", true);
      int initial_mipCnt = mipCnt;
      img = decode_image_from_dds(a, allow_mipstripe, mipCnt, mipCustomFilt, alpha_used, log);
      if (allow_mipstripe && initial_mipCnt != mipCnt && mipCnt > 0)
        mipCnt -= tmd.hqMip;
    }

    if (!img)
    {
      log.addMessage(log.ERROR, "%s: cannot load image %s", a.getName(), targetFilePath);
      return false;
    }
    if (GET_PROP(Bool, "smoothnessToInvRoughness.R", false))
      for (TexPixel32 *p = img->getPixels(), *pe = p + img->w * img->h; p < pe; p++)
        convert_wt_smoothness_to_inv_roughness(p->r);

    int eff_img_w = img->w;
    int eff_img_h = img->h;
    if (mipCustomFilt == MMF_stripe)
      eff_img_w = (eff_img_w + 1) / 2;
    if (texType == nvtt::TextureType_Cube)
      eff_img_h = eff_img_h / 6;
    else if (texType == nvtt::TextureType_3D)
    {
      int d = GET_PROP(Int, "texDepth", 1);
      eff_img_h = eff_img_h / d;
      if (mipCnt == -1)
        mipCnt = max(max(get_log2i(eff_img_w), get_log2i(eff_img_h)), get_log2i(d));
    }

    const char *pow2mode = is_uncompressed_fmt(fmt) && (!is_pow_of2(eff_img_h) || !is_pow_of2(eff_img_w)) ? "none" : "round";
    pow2mode = GET_PROP(Str, "pow2", pow2mode);
    if (strcmp(pow2mode, "none") == 0)
      inpOptions.setRoundMode(nvtt::RoundMode_None);
    else if (strcmp(pow2mode, "round") == 0)
      inpOptions.setRoundMode(nvtt::RoundMode_ToNearestPowerOfTwo);
    else if (strcmp(pow2mode, "prev") == 0)
      inpOptions.setRoundMode(nvtt::RoundMode_ToPreviousPowerOfTwo);
    else if (strcmp(pow2mode, "next") == 0)
      inpOptions.setRoundMode(nvtt::RoundMode_ToNextPowerOfTwo);

    if (tc_format && (!is_pow_of2(eff_img_w) || !is_pow_of2(eff_img_h)) && strcmp(pow2mode, "none") == 0 &&
        (cwr.getTarget() == _MAKE4C('PC') && pcAllowsASTC_to_ARGB))
    {
      fmt = "ARGB";
      comprOptions.setFormat(nvtt::Format_RGBA);
      tc_format = 0;
      auto_alpha_fmt = AAF_off;
    }

    if (GET_PROP(Bool, "bcFmtExpandImageTransp", false))
      if (strstr(fmt, "DXT") || strstr(fmt, "BC") || strstr(fmt, "ATI"))
      {
        int align = 4;
        for (int i = 0; i < mipCnt; i++)
          align *= 2;
        int nw = (eff_img_w + align - 1) / align * align, nh = (eff_img_h + align - 1) / align * align;
        if (nw != eff_img_w || nh != eff_img_h)
        {
          if (texType != nvtt::TextureType_2D)
          {
            log.addMessage(log.ERROR, "%s: bcFmtExpandImageTransp is not supported for texType=%s", a.getName(), texTypeStr);
            return false;
          }
          if (eff_img_w != img->w)
          {
            log.addMessage(log.ERROR, "%s: bad image size=%dx%d for %s=%s with fmt=%s", a.getName(), img->w, img->h, "mipFilter",
              "filterStripe", fmt);
            return false;
          }
          G_ASSERTF(nw >= img->w && nh >= img->h, "new=%dx%d src=%dx%d", nw, nh, img->w, img->h);
          TexImage32 *newImg = TexImage32::create(nw, nh, tmpmem);
          memset(newImg->getPixels(), 0, newImg->w * newImg->h * 4);
          TexPixel32 *sp = img->getPixels(), *dp = newImg->getPixels();
          for (int i = img->h; i > 0; dp += newImg->w, sp += img->w, i--)
            memcpy(dp, sp, img->w * 4);
          memfree(img, tmpmem);

          img = newImg;
          alpha_used = true;
          eff_img_w = img->w;
          eff_img_h = img->h;
        }
      }

    if (strcmp(pow2mode, "none") == 0 && (!is_pow_of2(eff_img_w) || !is_pow_of2(eff_img_h)))
    {
      int align = 4;
      for (int i = 0; i < mipCnt; i++)
        align *= 2;
      if ((strstr(fmt, "DXT") || strstr(fmt, "BC") || strstr(fmt, "ATI")) &&
          (((eff_img_w % align) && !is_pow_of2(eff_img_w)) || ((eff_img_h % align) && !is_pow_of2(eff_img_h))))
      {
        log.addMessage(log.ERROR, "%s: bad image size %dx%d, should be %d-aligned for \"%s\", %d mips", a.getName(), eff_img_w,
          eff_img_h, align, fmt, mipCnt);
        return false;
      }
    }

    if (const char *pat = GET_PROP(Str, "imageMergePattern", NULL))
    {
      String bfn;
      const char *bimg_nm = GET_PROP(Str, "baseImage", NULL);
      if (!resolve_base_image_fname(a, bimg_nm, bfn, "imageMergePattern", log))
        return false;
      if (!ablend_image(img, bfn, alpha_used, a.getName(), bimg_nm, log, "image", pat))
        return false;
    }

    if (GET_PROP(Bool, "baseImageAlphaPatch", false))
    {
      String bfn;
      const char *bimg_nm = GET_PROP(Str, "baseImage", NULL);
      if (!resolve_base_image_fname(a, bimg_nm, bfn, "baseImageAlphaPatch", log))
        return false;
      if (!ablend_image(img, bfn, alpha_used, a.getName(), bimg_nm, log, "image", "ablend"))
        return false;
    }

    bool splitHigh = GET_PROP(Bool, "splitHigh", false);
    int splitAt = GET_PROP(Int, "splitAt", 0);
    bool splitOverrided = GET_PROP(Bool, "splitAtOverride", false);

    if (!splitOverrided)
      splitAt = a.props.getBlockByNameEx(targetPlatform)->getInt("splitAtSize", splitAt);
    if (maxTexSz && splitAt)
    {
      tmd.hqMip += mipBias;
      while ((eff_img_w >> tmd.hqMip) > maxTexSz || (eff_img_h >> tmd.hqMip) > maxTexSz)
        tmd.hqMip++;
    }

    int skip_levels = tmd.hqMip;
    mipCnt++;
    if (!process_split(skip_levels, mipCnt, eff_img_w, eff_img_h, splitAt, splitHigh, outHandler.hqPartLev))
    {
      memfree(img, tmpmem);
      store_empty_dds(cwr);
      return true;
    }
    bool splitBasePart = !splitHigh && outHandler.hqPartLev > 0;
    if (splitBasePart && rtMipGen)
      rtMipGen = GET_PROP(Bool, "rtMipGenBQ", false);
    if (rtMipGen && !texRtMipGenAllowed)
    {
      log.addMessage(log.ERROR, "%s: tex is rtMipGen, prohibited", a.getName());
      return false;
    }

    if (const char *base_tex = GET_PROP(Str, "baseTex", NULL))
      if (*base_tex && ablend_btex && (splitBasePart || mipmap_none || !dagor_target_code_supports_tex_diff(cwr.getTarget())))
      {
        if (texType != nvtt::TextureType_2D)
        {
          log.addMessage(log.ERROR, "%s: baseTex is not supported for texType=%s", a.getName(), texTypeStr);
          return false;
        }
        DagorAsset *a_base = a.getMgr().findAsset(base_tex, a.getType());
        if (!a_base)
        {
          log.addMessage(log.ERROR, "%s: failed to find base tex asset <%s>", a.getName(), base_tex);
          return false;
        }

#define GET_PROP2(TYPE, PROP, DEF) props2.get##TYPE(PROP, &props2 != &a_base->props ? a_base->props.get##TYPE(PROP, DEF) : DEF)
        const DataBlock &props2 = a_base->getProfileTargetProps(cwr.getTarget(), cwr.getProfile());
        const char *swizzle_base = GET_PROP2(Str, "swizzleARGB", "ARGB");
        if (!a_base->props.getBool("convert", false) || strcmp(swizzle, swizzle_base) != 0)
        {
          log.addMessage(log.ERROR, "%s: base asset <%s> is not convertible or swizzle doesn't match: %s, %s", a.getName(), base_tex,
            swizzle, swizzle_base);
          return false;
        }
        if (GET_PROP2(Bool, "baseTexAlphaPatch", false) || GET_PROP2(Bool, "baseImageAlphaPatch", false))
        {
          log.addMessage(log.ERROR, "%s: base asset <%s> [%s %s]", a.getName(), base_tex,
            GET_PROP2(Bool, "baseTexAlphaPatch", false) ? "baseTexAlphaPatch" : "",
            GET_PROP2(Bool, "baseImageAlphaPatch", false) ? "baseImageAlphaPatch" : "");
          return false;
        }
#undef GET_PROP2

        if (!ablend_image(img, a_base->getTargetFilePath(), alpha_used, a.getName(), base_tex, log, "asset", "ablend"))
          return false;
        ablend_btex = false;
      }

    tmd.hqMip = skip_levels;
    mipCnt--;
    if (mipCnt > 0)
      inpOptions.setMipmapGeneration(true, mipCnt);
    else if (mipCnt == 0)
    {
      mipmap = "none";
      if (mipCustomFilt != MMF_stripe)
        mipCustomFilt = MMF_none;
      inpOptions.setMipmapGeneration(false);
    }

    if (alpha_used && GET_PROP(Bool, "colorPadding", false))
    {
      TexImage32 *newImg = TexImage32::create(img->w, img->h, tmpmem);
      *newImg = *img;
      update_image(newImg, img->getPixels(), GET_PROP(Int, "alphaThres", 127), tmd.addrU == tmd.ADDR_CLAMP,
        tmd.addrV == tmd.ADDR_CLAMP);
      memfree(img, tmpmem);
      img = newImg;
    }

    unsigned testPix[2] = {0xFF010203, 0x00040506};
    if (!alpha_used)
      testPix[1] |= 0xFF000000;
    // debug("alpha_used=%d pix: %08X %08X", alpha_used, testPix[0], testPix[1]);
    recodeRGBA32(testPix, 2, rs, gs, bs, as, andm, orm);
    if (!alpha_used && (testPix[0] >> 24) != (testPix[1] >> 24))
      alpha_used = true;
    else if (alpha_used && (testPix[0] >> 24) == 0xFF && (testPix[1] >> 24) == 0xFF)
      alpha_used = false;
    // debug("recoded pix: %08X %08X -> alpha_used=%d", testPix[0], testPix[1], alpha_used);

    if (auto_alpha_fmt != AAF_off && !tc_format)
      switch (auto_alpha_fmt)
      {
        case AAF_dxt1dxt5: comprOptions.setFormat(alpha_used ? nvtt::Format_DXT5 : nvtt::Format_DXT1); break;
        case AAF_dxt1bc7:
          if (alpha_used)
            bc67_fmt = BC67_FORMAT_BC7;
          else
            comprOptions.setFormat(nvtt::Format_DXT1);
          break;

        default:
          log.addMessage(log.WARNING, "%s: unexpected auto_alpha_fmt=%d, fallback to DXT5", a.getName(), auto_alpha_fmt);
          comprOptions.setFormat(nvtt::Format_DXT5);
          return false;
      }
    else if (auto_alpha_fmt != AAF_off && tc_format >= 0x100)
      tc_format = alpha_used ? 0x114 : 0x118;

    if (!dagor_target_code_supports_tex_diff(cwr.getTarget()))
      tmd.baseTexName = NULL;

    nv::FloatImage::WrapMode addrMode;
    if (tmd.addrU == tmd.ADDR_WRAP && tmd.addrV == tmd.ADDR_WRAP)
    {
      inpOptions.setWrapMode(nvtt::WrapMode_Repeat);
      addrMode = nv::FloatImage::WrapMode_Repeat;
    }
    else if ((tmd.addrU == tmd.ADDR_MIRROR && tmd.addrV == tmd.ADDR_MIRROR) ||
             (tmd.addrU == tmd.ADDR_MIRRORONCE && tmd.addrV == tmd.ADDR_MIRRORONCE))
    {
      inpOptions.setWrapMode(nvtt::WrapMode_Mirror);
      addrMode = nv::FloatImage::WrapMode_Mirror;
    }
    else
    {
      inpOptions.setWrapMode(nvtt::WrapMode_Clamp);
      addrMode = nv::FloatImage::WrapMode_Clamp;
    }

    Tab<ImageSurface> imgSurf;
    imgSurf.resize(img->h / eff_img_h);
    if (imgSurf.size() < 1)
    {
      log.addMessage(log.ERROR, "%s: bas slice count=%d for texType=%s", a.getName(), imgSurf.size(), texTypeStr);
      return false;
    }
    int voltex_depth = texType == nvtt::TextureType_3D ? imgSurf.size() : 1;
    for (int slice = 0; slice < imgSurf.size(); slice++)
    {
      imgSurf[slice].img = TexImage32::create(img->w, eff_img_h, tmpmem);
      memcpy(imgSurf[slice].img->getPixels(), img->getPixels() + slice * img->w * eff_img_h, img->w * eff_img_h * sizeof(TexPixel32));
    }
    memfree(img, tmpmem);
    img = nullptr;

    for (int slice = 0, mipCustomFilt0 = mipCustomFilt; slice < imgSurf.size(); slice++)
    {
      ImageSurface &aim = imgSurf[slice];
      nv::AutoPtr<nv::Image> &image = aim.image;
      nv::AutoPtr<nv::Image> &image0 = aim.image0;
      image = new nv::Image;
      img = aim.img;
      mipCustomFilt = mipCustomFilt0;
      mipStripeProduced = false;

      int img_w = img->w, img_h = img->h;

      if (texType == nvtt::TextureType_3D)
      {
        image->allocate(img_w, img_h);
        image->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
        memcpy(image->pixels(), img->getPixels(), img_w * img_h * 4);
      }
      else if (mipCustomFilt == MMF_stripe)
      {
        img_w = (img_w + 1) / 2;
        image->allocate(img_w, img_h);
        image->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
        for (int i = 0; i < img_h; i++)
          memcpy(image->scanline(i), img->getPixels() + img->w * i, img_w * 4);

        int ofs = img_w, full_w = img->w;
        int mip_w = img_w / 2, mip_h = img_h / 2;
        if (mip_w < 1)
          mip_w = 1;
        if (mip_h < 1)
          mip_h = 1;
        while (ofs + mip_w <= full_w)
        {
          if (mipCnt >= 0 && aim.imageMips.size() >= tmd.hqMip + mipCnt)
            break;
          nv::Image *mip = new nv::Image;
          aim.imageMips.push_back(mip);
          mip->allocate(mip_w, mip_h);
          mip->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
          for (int i = 0; i < mip_h; i++)
            memcpy(mip->scanline(i), img->getPixels() + img->w * i + ofs, mip_w * 4);

          if (mip_w == 1 && mip_h == 1)
            break;
          ofs += mip_w;
          mip_w /= 2;
          if (mip_w < 1)
            mip_w = 1;
          mip_h /= 2;
          if (mip_h < 1)
            mip_h = 1;
        }
      }
      else if ((mipCustomFilt == MMF_box || mipCustomFilt == MMF_kaizer || mipCustomFilt == MMF_point) && is_pow_of2(img_w) &&
               is_pow_of2(img_h))
      {
        // fast path of converting image to mip-stripe using our fast downsamplers (instead of NVidia's ones)
        image->allocate(img_w, img_h);
        image->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
        memcpy(image->pixels(), img->getPixels(), img_w * img_h * 4);
        memfree(img, tmpmem);
        img = NULL;

        int mips = mipCnt > 0 ? tmd.hqMip + mipCnt : max(get_log2i(img_w), get_log2i(img_h));
        int mip_w = img_w > 1 ? img_w / 2 : 1, mip_h = img_h > 1 ? img_h / 2 : 1;
        if (rtMipGen && mips > tmd.hqMip)
          mips = tmd.hqMip;
        while (mips > 0)
        {
          nv::Image *mip = new nv::Image;
          aim.imageMips.push_back(mip);

          mip->wrap(nv::mem::realloc(NULL, mip_w * mip_h * sizeof(TexPixel32) + 32), mip_w, mip_h);

          mip->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
          if (mipCustomFilt == MMF_point)
          {
            int ofs_x = img_w / mip_w / 2;
            int ofs_y = img_h / mip_h / 2;
            for (int y = 0; y < mip_h; y++)
            {
              TexPixel32 *dest = (TexPixel32 *)mip->scanline(y);
              TexPixel32 *src = ((TexPixel32 *)image->scanline(y * img_h / mip_h + ofs_y)) + ofs_x;
              for (int x = 0; x < mip_w; x++)
                dest[x] = src[x * img_w / mip_w];
            }
          }

          if (mip_w == 1 && mip_h == 1)
            break;
          mip_w = mip_w > 1 ? mip_w / 2 : 1;
          mip_h = mip_h > 1 ? mip_h / 2 : 1;
          mips--;
        }

        if (mipCustomFilt == MMF_box)
        {
          BoxFilter f(img_w, img_h, gamma);
          FilterBase::create_mip_chain(image.ptr(), aim.imageMips, f);
        }
        else if (mipCustomFilt == MMF_kaizer)
        {
          KaiserFilter f(img_w, img_h, gamma, mipFiltAlpha, mipFiltStretch);
          FilterBase::create_mip_chain(image.ptr(), aim.imageMips, f);
        }
        else
          G_ASSERT(mipCustomFilt == MMF_point);

        mipCustomFilt = MMF_stripe;
        mipStripeProduced = true;
      }
      else if (a.props.getBlockByName("processMips"))
      {
        // slow path of converting image to mip-stripe due to non-pow-2 or unusual filters
        image->allocate(img_w, img_h);
        image->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
        memcpy(image->pixels(), img->getPixels(), img_w * img_h * 4);
        memfree(img, tmpmem);
        img = NULL;

        nv::AutoPtr<nv::Filter> filter;
        if (!mipFilt || stricmp(mipFilt, "") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(mipFilt, "filterBox") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(mipFilt, "filterTriangle") == 0)
          filter = new nv::TriangleFilter();
        else if (stricmp(mipFilt, "filterKaiser") == 0)
        {
          filter = new nv::KaiserFilter(3);
          ((nv::KaiserFilter *)filter.ptr())->setParameters(mipFiltAlpha, mipFiltStretch);
        }
        else
          filter = new nv::BoxFilter();

        int mips = mipCnt > 0 ? tmd.hqMip + mipCnt : max(get_log2i(img_w), get_log2i(img_h));
        int mip_w = img_w > 1 ? img_w / 2 : 1, mip_h = img_h > 1 ? img_h / 2 : 1;
        while (mips > 0)
        {
          if (mipCustomFilt == MMF_point)
          {
            nv::Image *mip = new nv::Image;
            aim.imageMips.push_back(mip);
            mip->allocate(mip_w, mip_h);
            mip->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);

            int ofs_x = img_w / mip_w / 2;
            int ofs_y = img_h / mip_h / 2;
            for (int y = 0; y < mip_h; y++)
            {
              TexPixel32 *dest = (TexPixel32 *)mip->scanline(y);
              TexPixel32 *src = ((TexPixel32 *)image->scanline(y * img_h / mip_h + ofs_y)) + ofs_x;
              for (int x = 0; x < mip_w; x++)
                dest[x] = src[x * img_w / mip_w];
            }
          }
          else
          {
            nv::FloatImage fimage(aim.imageMips.size() ? aim.imageMips.back() : image.ptr());
            fimage.toLinear(0, 3, gamma);

            nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, mip_w, mip_h, addrMode));
            aim.imageMips.push_back(fresult->createImageGammaCorrect(gamma));
          }

          if (mip_w == 1 && mip_h == 1)
            break;
          mip_w = mip_w > 1 ? mip_w / 2 : 1;
          mip_h = mip_h > 1 ? mip_h / 2 : 1;
          mips--;
        }
        mipCustomFilt = MMF_stripe;
      }
      else
      {
        image->allocate(img_w, img_h);
        image->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
        memcpy(image->pixels(), img->getPixels(), img_w * img_h * 4);
        if (mipCustomFilt == MMF_box || mipCustomFilt == MMF_kaizer) // reset back for later checks
          mipCustomFilt = MMF_none;
      }
      aim.img = img;

      if (const DataBlock *b = a.props.getBlockByName("processMips"))
      {
        // process mips (that were previously converted to stripe)
        G_ASSERT(mipCustomFilt == MMF_stripe || mipCustomFilt == MMF_none);
        const char *original_swizzle = swizzle;
        if (stricmp(swizzle, "ARGB") != 0)
        {
          recodeRGBA32((unsigned *)image->pixels(), image->width() * image->height(), rs, gs, bs, as, andm, orm);
          for (int i = 0; i < aim.imageMips.size(); i++)
            recodeRGBA32((unsigned *)aim.imageMips[i]->pixels(), aim.imageMips[i]->width() * aim.imageMips[i]->height(), rs, gs, bs,
              as, andm, orm);
          swizzle = "ARGB";
        }

        if (!processMips(image.ptr(), aim.imageMips, *b, original_swizzle, a, log))
          return false;
      }

      if (const DataBlock *b = a.props.getBlockByName("mipFade"))
      {
        Point4 mipFadeColor = b->getPoint4("mipFadeColor", Point4(-1, -1, -1, -1)) * 255;
        E3DCOLOR mipFadeStart = b->getE3dcolor("mipFadeStart", E3DCOLOR(0, 0, 0, 0));
        E3DCOLOR mipFadeEnd = b->getE3dcolor("mipFadeEnd", E3DCOLOR(0, 0, 0, 0));

        Point4 mipStepValue =
          Point4((safeinv(float(mipFadeEnd.r) - float(mipFadeStart.r))), (safeinv(float(mipFadeEnd.g) - float(mipFadeStart.g))),
            safeinv((float(mipFadeEnd.b) - float(mipFadeStart.b))), (safeinv(float(mipFadeEnd.a) - float(mipFadeStart.a))));
        Point4 mipWeightValue = Point4(0.0f, 0.0f, 0.0f, 0.0f);
        if (stricmp(swizzle, "ARGB") != 0)
        {
          recodeRGBA32((unsigned *)image->pixels(), image->width() * image->height(), rs, gs, bs, as, andm, orm);
          for (int i = 0; i < aim.imageMips.size(); i++)
            recodeRGBA32((unsigned *)aim.imageMips[i]->pixels(), aim.imageMips[i]->width() * aim.imageMips[i]->height(), rs, gs, bs,
              as, andm, orm);
          swizzle = "ARGB";
        }
        for (int i = 0; i < aim.imageMips.size(); i++)
        {
          if (i > mipFadeStart.r && i <= mipFadeEnd.r)
            mipWeightValue.x += mipStepValue.x;
          else if ((i == mipFadeStart.r) && (mipFadeEnd.r == mipFadeStart.r))
            mipWeightValue.x = 1.0;

          if (i > mipFadeStart.g && i <= mipFadeEnd.g)
            mipWeightValue.y += mipStepValue.y;
          else if ((i == mipFadeStart.g) && (mipFadeEnd.g == mipFadeStart.g))
            mipWeightValue.y = 1.0;

          if (i > mipFadeStart.b && i <= mipFadeEnd.b)
            mipWeightValue.z += mipStepValue.z;
          else if ((i == mipFadeStart.b) && (mipFadeEnd.b == mipFadeStart.b))
            mipWeightValue.z = 1.0;

          if (i > mipFadeStart.a && i <= mipFadeEnd.a)
            mipWeightValue.w += mipStepValue.w;
          else if ((i == mipFadeStart.a) && (mipFadeEnd.a == mipFadeStart.a))
            mipWeightValue.w = 1.0;

          int size = aim.imageMips[i]->width() * aim.imageMips[i]->height();
          mip_fade_image((TexPixel32 *)aim.imageMips[i]->pixels(), size, mipWeightValue, mipFadeColor);
        }
      }

#define CALC_ALPHA_COVERAGE()                                    \
  for (int i = 0; i < A_REF_MIP_CNT; i++)                        \
  {                                                              \
    bool found = false;                                          \
    for (int j = 0; j < i; j++)                                  \
      if (a_ref_mip[i] == a_ref_mip[j])                          \
      {                                                          \
        a_coverage[i] = a_coverage[j];                           \
        found = true;                                            \
        break;                                                   \
      }                                                          \
    if (!found)                                                  \
      a_coverage[i] = fimage.alphaTestCoverage(a_ref_mip[i], 3); \
  }

      if (
        mipCustomFilt == MMF_stripe && aim.imageMips.size() && (tmd.hqMip > 0 || (maxTexSz && (img_w > maxTexSz || img_h > maxTexSz))))
      {
        if (mipAlphaCoverage)
        {
          nv::FloatImage fimage(image.ptr());
          CALC_ALPHA_COVERAGE();
        }
        int skip = min<int>(tmd.hqMip, aim.imageMips.size()), mipw = img_w >> skip, miph = img_h >> skip;
        int maxw = img_w >> tmd.hqMip, maxh = img_h >> tmd.hqMip;
        if (maxTexSz)
        {
          if (maxw > maxTexSz)
            maxw = maxTexSz;
          if (maxh > maxTexSz)
            maxh = maxTexSz;
        }
        while (skip < aim.imageMips.size() && (mipw > maxw || miph > maxh))
          skip++, mipw >>= 1, miph >>= 1;

        G_ASSERT(skip <= aim.imageMips.size());
        if (mipValveAlpha && !(mipAlphaCoverage && splitBasePart))
        {
          debug("save image 0 for valve processing");
          image0 = image.release();
        }
        image = aim.imageMips[skip - 1];
        erase_ptr_items(aim.imageMips, 0, skip - 1);
        erase_items(aim.imageMips, 0, 1);
        img_w = image->width();
        img_h = image->height();
      }
      else if (tmd.hqMip > 0 || (maxTexSz && (img_w > maxTexSz || img_h > maxTexSz)))
      {
        //== we strip unused mips here to reduce processing time required for conversion;
        //   texconvcache::get_tex_asset_built_ddsx() (or any other consumer) must be aware of this
        const char *dsFilt = GET_PROP(Str, "downscaleFilter", mipFilt);

        if (tmd.hqMip > 0)
        {
          img_w >>= tmd.hqMip;
          img_h >>= tmd.hqMip;
        }
        if (img_w > maxTexSz || img_h > maxTexSz)
          debug("%s: %dx%d (%d)", a.getName(), img_w, img_h, maxTexSz);
        if (maxTexSz && img_w > maxTexSz && (!splitHigh || img_h >= img_w / maxTexSz))
        {
          if (splitHigh)
            img_h /= img_w / maxTexSz;
          img_w = maxTexSz;
          debug(" -> %dx%d", img_w, img_h);
        }
        if (maxTexSz && img_h > maxTexSz && (!splitHigh || img_w >= img_h / maxTexSz))
        {
          if (splitHigh)
            img_w /= img_h / maxTexSz;
          img_h = maxTexSz;
          debug(" -> %dx%d", img_w, img_h);
        }

        if (!img_w)
          img_w = 1;
        if (!img_h)
          img_h = 1;

        nv::AutoPtr<nv::Filter> filter;
        if (stricmp(dsFilt, "") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(dsFilt, "filterBox") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(dsFilt, "filterTriangle") == 0)
          filter = new nv::TriangleFilter();
        else if (stricmp(dsFilt, "filterKaiser") == 0)
        {
          filter = new nv::KaiserFilter(3);
          ((nv::KaiserFilter *)filter.ptr())->setParameters(mipFiltAlpha, mipFiltStretch);
        }
        else
          filter = new nv::BoxFilter();

        nv::FloatImage fimage(image.ptr());
        if (mipAlphaCoverage)
        {
          CALC_ALPHA_COVERAGE();
        }
        fimage.toLinear(0, 3, gamma);

        nv::AutoPtr<nv::FloatImage> fresult(fimage.resize(*filter, img_w, img_h, addrMode));
        if (mipValveAlpha && !(mipAlphaCoverage && splitBasePart))
        {
          image0 = image.release();
          image = fresult->createImageGammaCorrect(gamma);
        }
        else
          image = fresult->createImageGammaCorrect(gamma);
        G_ASSERT(image->width() == img_w && image->height() == img_h);
      }
      else if (mipAlphaCoverage)
      {
        nv::FloatImage fimage(image.ptr());
        CALC_ALPHA_COVERAGE();
      }
#undef CALC_ALPHA_COVERAGE

      if (stricmp(swizzle, "ARGB") != 0)
      {
        recodeRGBA32((unsigned *)image->pixels(), image->width() * image->height(), rs, gs, bs, as, andm, orm);
        if (image0.ptr())
          recodeRGBA32((unsigned *)image0->pixels(), image0->width() * image0->height(), rs, gs, bs, as, andm, orm);
        for (int i = 0; i < aim.imageMips.size(); i++)
          recodeRGBA32((unsigned *)aim.imageMips[i]->pixels(), aim.imageMips[i]->width() * aim.imageMips[i]->height(), rs, gs, bs, as,
            andm, orm);
      }
      if (image0.ptr())
      {
        nv::Image *img1 = image0.ptr();
        nv::Image *img2 = image.ptr();
        generate_texture_alpha((TexPixel32 *)image0->pixels(), image0->width(), image0->height(), image0->width(),
          (TexPixel32 *)image->pixels(), image->width(), image->height(), image->width(),
          valveAlphaFilter * image0->width() / image->width(), valveAlphaThres, valveAlphaWrapU, valveAlphaWrapV);

        if (mipmap_none || stricmp(mipmap, "none") == 0)
          image0 = NULL;
      }

      if (!mipmap_none && dagor_target_code_supports_tex_diff(cwr.getTarget()) && !splitBasePart)
        if (const char *base_tex = GET_PROP(Str, "baseTex", NULL))
          if (*base_tex)
          {
            if (texType != nvtt::TextureType_2D)
            {
              log.addMessage(log.ERROR, "%s: baseTex is not supported for texType=%s", a.getName(), texTypeStr);
              return false;
            }
            if (mipCustomFilt == MMF_stripe && mipStripeProduced)
            {
              // treat as simple image, remove mips that will not be used anymore
              mipCustomFilt = MMF_none;
              clear_all_ptr_items_and_shrink(aim.imageMips);
            }

            if (mipCustomFilt != MMF_none || mipValveAlpha || mipAlphaCoverage)
            {
              log.addMessage(log.ERROR, "%s: unsupported basetex with mipCustomFilt=%d or mipValveAlpha=%d or mipAlphaCoverage=%d",
                a.getName(), mipCustomFilt, mipValveAlpha, mipAlphaCoverage);
              return false;
            }
            DagorAsset *a_base = a.getMgr().findAsset(base_tex, a.getType());
            if (!a_base)
            {
              log.addMessage(log.ERROR, "%s: failed to find base tex asset <%s>", a.getName(), base_tex);
              return false;
            }
            if (a_base->props.getInt("splitAt", 0) > 0 && !a_base->props.getBool("splitHigh", false))
            {
              a_base = a.getMgr().findAsset(String(0, "%s$hq", base_tex), a.getType());
              if (!a_base)
              {
                log.addMessage(log.ERROR, "%s: failed to find base tex asset <%s$hq>", a.getName(), base_tex);
                return false;
              }
            }

            String a_name(a.getName());
            if (splitHigh)
              erase_items(a_name, a_name.size() - 4, 3); //< trim $hq

            if (!checkPaired(*a_base, a_base->getProfileTargetProps(cwr.getTarget(), cwr.getProfile()), a_name))
            {
              log.addMessage(log.ERROR, "%s: base tex asset <%s> doesn't refer to us with 'pairedToTex'", a.getName(),
                a_base->getName());
              return false;
            }

            mkbindump::BinDumpSaveCB mcwr_b(17 << 20, cwr.getTarget(), false);
            Tab<TexPixel32> base_img(tmpmem);
            if (!buildTexAsset(*a_base, mcwr_b, log, true, (diff_tolerance.u == 0 && !ablend_btex) ? NULL : &base_img))
            {
              log.addMessage(log.ERROR, "%s: failed to export base tex asset <%s>", a.getName(), base_tex);
              return false;
            }
            MemoryLoadCB mcrd_b(mcwr_b.getRawWriter().getMem(), false);
            DDSURFACEDESC2 base_dds_hdr;
            int base_dxt_format = 0;

            mcrd_b.seekto(4);
            mcrd_b.read(&base_dds_hdr, sizeof(base_dds_hdr));
            if (base_dds_hdr.dwWidth == 0 && base_dds_hdr.dwHeight == 0 && a_base->props.getBool("splitHigh", false))
            {
              // HQ part is empty, let us fallback to BQ part...
              a_base = a.getMgr().findAsset(base_tex, a.getType());

              if (!checkPaired(*a_base, a_base->getProfileTargetProps(cwr.getTarget(), cwr.getProfile()), a_name))
              {
                log.addMessage(log.ERROR, "%s: base tex asset <%s> doesn't refer to us with 'pairedToTex'", a.getName(),
                  a_base->getName());
                return false;
              }

              mcwr_b.reset(17 << 20);
              if (!buildTexAsset(*a_base, mcwr_b, log, true, (diff_tolerance.u == 0 && !ablend_btex) ? NULL : &base_img))
              {
                log.addMessage(log.ERROR, "%s: failed to export base tex asset <%s>", a.getName(), base_tex);
                return false;
              }
              mcrd_b.setMem(mcwr_b.getRawWriter().getMem(), false);
              mcrd_b.seekto(4);
              mcrd_b.read(&base_dds_hdr, sizeof(base_dds_hdr));
            }

            if (base_dds_hdr.dwWidth != image->width() || base_dds_hdr.dwHeight != image->height())
            {
              log.addMessage(log.ERROR, "%s: base tex asset <%s> resolution mismatch: %dx%d != %dx%d", a.getName(), base_tex,
                base_dds_hdr.dwWidth, base_dds_hdr.dwHeight, image->width(), image->height());
              return false;
            }
            if (base_dds_hdr.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
              base_dxt_format = 1;
            else if (base_dds_hdr.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
              base_dxt_format = 5;

            if (!base_dxt_format)
            {
              log.addMessage(log.ERROR, "%s: base tex asset <%s> must be DXT1/DXT5 format", a.getName(), base_tex);
              return false;
            }

            if (ablend_btex)
            {
              int pcnt = image->width() * image->height();
              for (TexPixel32 *d = (TexPixel32 *)image->pixels(), *de = d + pcnt, *b = base_img.data(); d < de; d++, b++)
                if (d->a == 255)
                  d->a = b->a;
                else if (!d->a)
                  *d = *b;
                else
                {
                  d->r = int(b->r) + (int(d->r) - int(b->r)) * d->a / 255;
                  d->g = int(b->g) + (int(d->g) - int(b->g)) * d->a / 255;
                  d->b = int(b->b) + (int(d->b) - int(b->b)) * d->a / 255;
                  d->a = b->a;
                }
            }

            MemorySaveCB mcwr_p;
            outHandler.cwr = &mcwr_p;

            inpOptions.setMipmapGeneration(false);
            inpOptions.setTextureLayout(texType, image->width(), image->height());
            inpOptions.setMipmapData(image->pixels(), image->width(), image->height());
            G_VERIFY(compressor.process(inpOptions, comprOptions, outOptions));

            MemoryLoadCB mcrd_p(mcwr_p.getMem(), false);
            DDSURFACEDESC2 dds_hdr;
            mcrd_p.seekto(4);
            mcrd_p.read(&dds_hdr, sizeof(dds_hdr));

            int dxt_format = 0;
            if (dds_hdr.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
              dxt_format = 1;
            else if (dds_hdr.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
              dxt_format = 5;
            if (!dxt_format)
            {
              log.addMessage(log.ERROR, "%s: must be DXT1/DXT5 format", a.getName(), base_tex);
              return false;
            }
            dds_hdr.dwMipMapCount =
              stricmp(mipmap, "gen") == 0 ? (mipCnt > 0 ? mipCnt : max(get_log2i(image->width()), get_log2i(image->height()))) + 1 : 1;
            dds_hdr.dwFlags |= DDSD_MIPMAPCOUNT;

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

            Tab<unsigned> map(tmpmem);
            Tab<unsigned char> dxt_blocks_a(tmpmem);
            Tab<unsigned char> dxt_blocks_c(tmpmem);
            int dxt_block_len = (dxt_format == 1) ? 8 : 16;
            int dxt_color_ofs = (dxt_format == 1) ? 0 : 8;

            map.resize(((image->width() / 4) * (image->height() / 4) + 31) / 32);
            mem_set_0(map);

            dxt_blocks_a.reserve(mcwr_p.getSize() - 128);
            dxt_blocks_c.reserve(mcwr_p.getSize() - 128);
            int empty_block_cnt = 0, total_block_cnt = 0;

            for (int y = 0, ye = image->height(), xe = image->width(), b = 0; y < ye; y += 4)
              for (int x = 0; x < xe; x += 4, b++)
              {
                unsigned char dxt_data_p[16];
                unsigned char dxt_data_b[16];
                if (dxt_format == 5)
                  mcrd_p.read(dxt_data_p, 8);
                mcrd_p.read(dxt_data_p + dxt_color_ofs, 8);

                if (base_dxt_format == 5)
                  mcrd_b.read(dxt_data_b, 8);
                else if (dxt_format == 5)
                {
                  memset(dxt_data_b, 0xFF, 2);
                  memset(dxt_data_b + 2, 0, 6);
                }
                mcrd_b.read(dxt_data_b + dxt_color_ofs, 8);

                if (memcmp(dxt_data_p, dxt_data_b, dxt_block_len) == 0)
                  empty_block_cnt++;
                else if (diff_tolerance.u && pixelDiffLessThan(image, base_img.data(), x, y, diff_tolerance))
                  empty_block_cnt++;
                else
                {
                  Dxt1ColorBlock &pc = *reinterpret_cast<Dxt1ColorBlock *>(dxt_data_p + dxt_color_ofs);
                  Dxt1ColorBlock &bc = *reinterpret_cast<Dxt1ColorBlock *>(dxt_data_b + dxt_color_ofs);

                  pc.c0r -= bc.c0r;
                  pc.c0g -= bc.c0g;
                  pc.c0b -= bc.c0b;
                  pc.c1r -= bc.c1r;
                  pc.c1g -= bc.c1g;
                  pc.c1b -= bc.c1b;
                  pc.idx = bc.idx ^ pc.idx;

                  if (dxt_format == 5)
                  {
                    Dxt5AlphaBlock &pa = *(Dxt5AlphaBlock *)dxt_data_p;
                    Dxt5AlphaBlock &ba = *(Dxt5AlphaBlock *)dxt_data_b;
                    if (base_dxt_format == 5)
                    {
                      pa.a0 -= ba.a0;
                      pa.a1 -= ba.a1;
                      pa.idx[0] = ba.idx[0] ^ pa.idx[0];
                      pa.idx[1] = ba.idx[1] ^ pa.idx[1];
                      pa.idx[2] = ba.idx[2] ^ pa.idx[2];
                    }
                    append_items(dxt_blocks_a, 8, (unsigned char *)&pa);
                  }
                  append_items(dxt_blocks_c, 8, (unsigned char *)&pc);

                  map[b / 32] |= 1 << (b % 32);
                }
                total_block_cnt++;
              }

            cwr.writeFourCC(_MAKE4C('DDS '));
            cwr.writeRaw(&dds_hdr, sizeof(dds_hdr));

            cwr.writeInt32e(empty_block_cnt);
            if (empty_block_cnt != 0 && empty_block_cnt != total_block_cnt)
              cwr.write32ex(map.data(), data_size(map));

            cwr.writeRaw(dxt_blocks_a.data(), data_size(dxt_blocks_a));
            cwr.writeRaw(dxt_blocks_c.data(), data_size(dxt_blocks_c));

            debug("%s: map=%d dxt_blocks=%d (%d/%d)", a.getName(), data_size(map), data_size(dxt_blocks_a) + data_size(dxt_blocks_c),
              total_block_cnt - empty_block_cnt, total_block_cnt);
            return true;
          }

      if (mipCustomFilt == MMF_point && !mipmap_none && !aim.imageMips.size())
      {
        int mcnt = mipCnt > 0 ? mipCnt : max(get_log2i(img_w), get_log2i(img_h));
        int mip_w = img_w / 2, mip_h = img_h / 2;
        if (mip_w < 1)
          mip_w = 1;
        if (mip_h < 1)
          mip_h = 1;
        for (int i = 0; i < mcnt; i++)
        {
          nv::Image *mip = new nv::Image;
          aim.imageMips.push_back(mip);
          mip->allocate(mip_w, mip_h);
          mip->setFormat(alpha_used ? image->Format_ARGB : image->Format_RGB);
          int ofs_x = img_w / mip_w / 2;
          int ofs_y = img_h / mip_h / 2;
          for (int y = 0; y < mip_h; y++)
          {
            TexPixel32 *dest = (TexPixel32 *)mip->scanline(y);
            TexPixel32 *src = ((TexPixel32 *)image->scanline(y * img_h / mip_h + ofs_y)) + ofs_x;
            for (int x = 0; x < mip_w; x++)
              dest[x] = src[x * img_w / mip_w];
          }

          if (mip_w == 1 && mip_h == 1)
            break;
          mip_w /= 2;
          if (mip_w < 1)
            mip_w = 1;
          mip_h /= 2;
          if (mip_h < 1)
            mip_h = 1;
        }
      }

      if ((mipValveAlpha || mipAlphaCoverage) && mipCustomFilt == MMF_none)
      {
        int mcnt = mipCnt > 0 ? mipCnt : max(get_log2i(img_w), get_log2i(img_h));
        int mip_w = img_w / 2, mip_h = img_h / 2;
        if (mip_w < 1)
          mip_w = 1;
        if (mip_h < 1)
          mip_h = 1;

        nv::AutoPtr<nv::Filter> filter;

        if (rtMipGen || stricmp(mipFilt, "") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(mipFilt, "filterBox") == 0)
          filter = new nv::BoxFilter();
        else if (stricmp(mipFilt, "filterTriangle") == 0)
          filter = new nv::TriangleFilter();
        else if (stricmp(mipFilt, "filterKaiser") == 0)
        {
          filter = new nv::KaiserFilter(mipFiltWidth);
          ((nv::KaiserFilter *)filter.ptr())->setParameters(mipFiltAlpha, mipFiltStretch);
        }

        nv::AutoPtr<nv::FloatImage> fimage(new nv::FloatImage(image.ptr()));
        fimage->toLinear(0, 3, gamma);
        for (int i = 0; i < mcnt; i++)
        {
          fimage = fimage->resize(*filter, mip_w, mip_h, addrMode);
          aim.imageMips.push_back(fimage->createImageGammaCorrect(gamma));

          if (mip_w == 1 && mip_h == 1)
            break;
          mip_w /= 2;
          if (mip_w < 1)
            mip_w = 1;
          mip_h /= 2;
          if (mip_h < 1)
            mip_h = 1;
        }
      }

      if (mipValveAlpha && !mipAlphaCoverage)
      {
        if (image0.ptr() && aim.imageMips.size() && aim.imageMips[0]->width())
        {
          valveAlphaFilter = max(int(valveAlphaFilter * aim.imageMips[0]->width() / image0->width()), int(1));
          debug("valveAlphaFilter = %d", valveAlphaFilter);
        }
        for (int i = 0; i < aim.imageMips.size(); i++)
        {
          nv::Image *img1 = image0.ptr() ? image0.ptr() : image.ptr();
          nv::Image *img2 = aim.imageMips[i];
          debug("%d: gen valve filter = %dx%d from %dx%d", i, img2->width(), img2->height(), img1->width(), img1->height());

          generate_texture_alpha((TexPixel32 *)img1->pixels(), img1->width(), img1->height(), img1->width(),
            (TexPixel32 *)img2->pixels(), img2->width(), img2->height(), img2->width(),
            valveAlphaFilter * img1->width() / img2->width(), valveAlphaThres, valveAlphaWrapU, valveAlphaWrapV);
        }
      }
      if (mipAlphaCoverage)
      {
        debug("%s: %dx%d  improving alpha coverage", a.getName(), image->width(), image->height());
        for (int i = 0; i < aim.imageMips.size() && i < A_REF_MIP_CNT; i++)
        {
          nv::FloatImage mip(aim.imageMips[i]);
          float cov = mip.alphaTestCoverage(a_ref_mip[i], 3);
          if (cov < a_coverage[i])
          {
            debug_("mip[%s] %dx%d coverage=%.7f -> %.7f", i, mip.width(), mip.height(), cov, a_coverage[i]);
            mip.scaleAlphaToCoverage(a_coverage[i], a_ref_mip[i], 3);
            debug("  -> %.7f  (ref=%.3f)", mip.alphaTestCoverage(a_ref_mip[i], 3), a_ref_mip[i]);
            aim.imageMips[i] = mip.createImage();
          }
          else
            debug("mip[%s] %dx%d coverage=%.7f >= %.7f, skipping", i, mip.width(), mip.height(), cov, a_coverage[i]);
        }

        for (int i = mipValveAlphaAfterCoverage; i < (int)aim.imageMips.size() - mipValveAlphaSkipLast; i++)
        {
          nv::Image *img2 = aim.imageMips[i];
          int src_w = img2->width(), src_h = img2->height();
          int upscale = GET_PROP(Int, String(0, "valveAlphaUpscaleMip%d", i + 1), valveAlphaUpscale);

          nv::Image *img1 = new nv::Image;
          img1->allocate(src_w * upscale, src_h * upscale);
          img1->setFormat(image->Format_ARGB);

          TexPixel32 *big = (TexPixel32 *)img1->pixels();
          TexPixel32 *src = (TexPixel32 *)img2->pixels();
          for (int y = 0; y < img1->height(); ++y)
            for (int x = 0; x < img1->width(); ++x, big++)
              *big = texpixel32(getBilinearFilteredPixelColor(src, double(x) / upscale, double(y) / upscale, src_w, src_h));

          generate_texture_alpha((TexPixel32 *)img1->pixels(), img1->width(), img1->height(), img1->width(),
            (TexPixel32 *)img2->pixels(), src_w, src_h, src_w, valveAlphaFilter * img1->width() / src_w, valveAlphaThres,
            valveAlphaWrapU, valveAlphaWrapV);
          delete img1;
        }
      }

      if (postCopyAtoR)
        for (int i = -1, ie = aim.imageMips.size(); i < ie; i++)
        {
          nv::Image *img = i < 0 ? image.ptr() : aim.imageMips[i];
          for (TexPixel32 *pix = (TexPixel32 *)img->pixels(), *pix_e = pix + img->width() * img->height(); pix < pix_e; pix++)
            pix[0].u = (pix[0].u & 0xFF00FFFF) | (pix[0].a << 16);
        }
    }

    if (mipmap_none)
      inpOptions.setMipmapGeneration(false);

    if (out_base_img)
    {
      nv::AutoPtr<nv::Image> &image = imgSurf[0].image;
      out_base_img->resize(image->width() * image->height());
      mem_copy_from(*out_base_img, image->pixels());
      write_built_dds_final(cwr.getRawWriter(), a, cwr.getTarget(), cwr.getProfile(), &log);
      return true;
    }

    if (texType == nvtt::TextureType_3D)
    {
      if (mipCustomFilt != MMF_box && mipCustomFilt != MMF_none)
      {
        log.addMessage(log.ERROR, "%s: mipFilter=<%s> doesn't suit texType=%s, supported are: %s", a.getName(), mipFilt, texTypeStr,
          "filterBox");
        return false;
      }
      nv::Image *image3D_mip = new nv::Image;
      int w = imgSurf[0].image->width(), h = imgSurf[0].image->height(), d = imgSurf.size();
      image3D_mip->allocate(imgSurf[0].image->width(), imgSurf[0].image->height() * d);
      image3D_mip->setFormat(imgSurf[0].image->format());
      for (int i = 0; i < d; i++)
        memcpy(image3D_mip->scanline(h * i), imgSurf[i].image->scanline(0), w * h * 4);

      imgSurf.clear();
      imgSurf.resize(1);
      imgSurf[0].image = image3D_mip;

      for (int m = 1; m <= mipCnt; m++)
      {
        nv::Image *mip = new nv::Image;
        int mip_w = max(w / 2, 1), mip_h = max(h / 2, 1), mip_d = max(d / 2, 1);
        mip->allocate(mip_w, mip_h * mip_d);
        mip->setFormat(image3D_mip->format());

        nv::Color32 pcol[8];
        for (int y = 0; y < mip_h * mip_d; y++)
        {
          nv::Color32 *dest = mip->scanline(y);
          int y0, y1, d0, d1;
          y0 = y1 = (y % mip_h);
          d0 = d1 = (y / mip_h);

          if (h != mip_h)
            y0 = y0 * 2, y1 = y1 * 2 + 1;
          if (d != mip_d)
            d0 = d0 * 2, d1 = d1 * 2 + 1;

#define GET_COLORS(F) F(0, 0, 0), F(1, 0, 0), F(0, 1, 0), F(1, 1, 0), F(0, 0, 1), F(1, 0, 1), F(0, 1, 1), F(1, 1, 1)
#define CALC_AVG(X, C) \
  dest[X].C = (unsigned(pcol[0].C) + pcol[1].C + pcol[2].C + pcol[3].C + pcol[4].C + pcol[5].C + pcol[6].C + pcol[7].C) / 8
#define CALC_MIN(X, C) \
  dest[X].C = min(min(min(pcol[0].C, pcol[1].C), min(pcol[2].C, pcol[3].C)), min(min(pcol[4].C, pcol[5].C), min(pcol[6].C, pcol[7].C)))
#define CALC_MAX(X, C) \
  dest[X].C = max(max(max(pcol[0].C, pcol[1].C), max(pcol[2].C, pcol[3].C)), max(max(pcol[4].C, pcol[5].C), max(pcol[6].C, pcol[7].C)))
          if (w != mip_w)
          {
#define GET_COLOR2(X, Y, D) pcol[X + Y * 2 + D * 4] = image3D_mip->scanline(y##Y + h * d##D)[x * 2 + X]
            for (int x = 0; x < mip_w; x++)
            {
              GET_COLORS(GET_COLOR2);
              CALC_AVG(x, r);
              CALC_AVG(x, g);
              CALC_AVG(x, b);
              CALC_AVG(x, a);
            }
#undef GET_COLOR
          }
          else
          {
#define GET_COLOR1(X, Y, D) pcol[X + Y * 2 + D * 4] = image3D_mip->scanline(y##Y + h * d##D)[x]
            for (int x = 0; x < mip_w; x++)
            {
              GET_COLORS(GET_COLOR1);
              CALC_AVG(x, r);
              CALC_AVG(x, g);
              CALC_AVG(x, b);
              CALC_AVG(x, a);
            }
#undef GET_COLOR
          }
#undef CALC_AVG
#undef CALC_MIN
#undef CALC_MAX
#undef GET_COLORS
        }

        imgSurf[0].imageMips.push_back(mip);
        image3D_mip = mip;
        w = mip_w, h = mip_h, d = mip_d;
      }
    }

#if !TEX_CANNOT_USE_ISPC
    if (bc67_fmt == BC67_FORMAT_BC6H)
      return convert_to_bc6(cwr.getRawWriter(), imgSurf, texType, voltex_depth, quality, outHandler.hqPartLev);
    else if (bc67_fmt == BC67_FORMAT_BC7)
      return convert_to_bc7(cwr.getRawWriter(), imgSurf, texType, voltex_depth, quality, outHandler.hqPartLev);
#endif
    if (tc_format)
    {
      nv::Image *image = imgSurf[0].image.ptr();
      if (!is_pow_of2(image->width()) || !is_pow_of2(image->height()))
      {
        log.addMessage(log.ERROR, "%s: image %dx%d is NPOT - ASTC failed for iOS", a.getName(), image->width(), image->height());
        return false;
      }
      // save_tga32(String(0, "%s-pre.tga", a.getName()),(TexPixel32*)image->pixels(), image->width(), image->height(),
      // image->width()*4);
      return convert_to_astc(cwr.getRawWriter(), imgSurf, texType, voltex_depth, fabs(gamma - 1.0) < 1e-3, tc_format,
        outHandler.hqPartLev, a.getName());
    }

    if (texType == nvtt::TextureType_3D)
    {
      ImageSurface &surf = imgSurf[0];
      int w = surf.image->width(), h = surf.image->height() / voltex_depth, d = voltex_depth;
      inpOptions.setTextureLayout(texType, w, h, d);
      inpOptions.setMipmapData(surf.image->pixels(), w, h, d);
      for (int m = 0; m < surf.imageMips.size(); m++)
        inpOptions.setMipmapData(surf.imageMips[m]->pixels(), w = max(w / 2, 1), h = max(h / 2, 1), d = max(d / 2, 1), 0, m + 1);
    }
    else
      for (int face = 0; face < (texType == nvtt::TextureType_Cube ? 6 : 1); face++)
      {
        ImageSurface &surf = imgSurf[face];
        if (face == 0)
          inpOptions.setTextureLayout(texType, surf.image->width(), surf.image->height(), 1);
        inpOptions.setMipmapData(surf.image->pixels(), surf.image->width(), surf.image->height(), 1, face, 0);
        if (surf.imageMips.size())
        {
          inpOptions.setMipmapGeneration(true, surf.imageMips.size());
          for (int i = 0; i < surf.imageMips.size(); i++)
            inpOptions.setMipmapData(surf.imageMips[i]->pixels(), surf.imageMips[i]->width(), surf.imageMips[i]->height(), 1, face,
              i + 1);
        }
      }

    if (pix_format)
    {
      MemorySaveCB mcwr(1 << 20);
      outHandler.cwr = &mcwr;
      if (!compressor.process(inpOptions, comprOptions, outOptions))
        return false;
      clear_and_shrink(imgSurf);
      MemoryLoadCB crd(mcwr.getMem(), false);
      return convertPixFmt(cwr.getRawWriter(), crd, pix_format);
    }

    outHandler.cwr = &cwr.getRawWriter();
    return compressor.process(inpOptions, comprOptions, outOptions);
#undef GET_PROP
  }

  static bool is_uncompressed_fmt(const char *fmt)
  {
    return stricmp(fmt, "ARGB") == 0 || stricmp(fmt, "RGB") == 0 || stricmp(fmt, "R8") == 0 || stricmp(fmt, "L8") == 0 ||
           stricmp(fmt, "A8R8") == 0 || stricmp(fmt, "A8L8") == 0 || stricmp(fmt, "R8G8") == 0 || stricmp(fmt, "RGB565") == 0 ||
           stricmp(fmt, "R5G6B5") == 0 || stricmp(fmt, "RGBA5551") == 0 || stricmp(fmt, "A1R5G5B5") == 0 ||
           stricmp(fmt, "RGBA4444") == 0 || stricmp(fmt, "A4R4G4B4") == 0;
  }
  void recodeRGBA32(unsigned *pixels, int dwnum, unsigned r, unsigned g, unsigned b, unsigned a, unsigned andm, unsigned orm)
  {
    for (; dwnum > 0; pixels++, dwnum--)
    {
      unsigned p = *pixels;
      *pixels =
        (((((p >> a) & 0xFF) << 24) | (((p >> r) & 0xFF) << 16) | (((p >> g) & 0xFF) << 8) | (((p >> b) & 0xFF))) & andm) | orm;
    }
  }

  bool decodeSwizzleRGBA32(const char *swizzle, unsigned &r, unsigned &g, unsigned &b, unsigned &a, unsigned &andm, unsigned &orm)
  {
    unsigned shift[4];
    if (strlen(swizzle) != 4)
      return false;

    orm = 0;
    andm = 0xFFFFFFFF;
    for (int i = 0; i < 4; i++)
      switch (swizzle[i])
      {
        case 'A': shift[i] = 24; break;
        case 'R': shift[i] = 16; break;
        case 'G': shift[i] = 8; break;
        case 'B': shift[i] = 0; break;
        case '0':
          shift[i] = 0;
          andm &= ~(0xFF000000 >> (i * 8));
          break;
        case '1':
          shift[i] = 0;
          andm &= ~(0xFF000000 >> (i * 8));
          orm |= 0xFF000000 >> (i * 8);
          break;
        default: return false;
      }
    a = shift[0];
    r = shift[1];
    g = shift[2];
    b = shift[3];
    return true;
  }

  int getMipSize(int h, int w)
  {
    int bw = w >> 2;

    if (bw < 1)
    {
      bw = 1;
    }

    int bh = h >> 2;

    if (bh < 1)
    {
      bh = 1;
    }

    return (bw * bh) << 4;
  }

#if !TEX_CANNOT_USE_ISPC
  void chooseBC6HEncodeSettings(bc6h_enc_settings *settings, const char *quality)
  {
    if (strcmp(quality, "") == 0)
    {
      GetProfile_bc6h_slow(settings);
    }
    else if (strcmp(quality, "fastest") == 0)
    {
      GetProfile_bc6h_fast(settings);
    }
    else if (strcmp(quality, "normal") == 0)
    {
      GetProfile_bc6h_basic(settings);
    }
    else if (strcmp(quality, "production") == 0)
    {
      GetProfile_bc6h_slow(settings);
    }
    else if (strcmp(quality, "highest") == 0)
    {
      GetProfile_bc6h_veryslow(settings);
    }
  }

  void swapRGBA16(Tab<uint16_t> &pix_argb, nv::Image *image)
  {
    uint16_t *d = pix_argb.data();
    TexPixel32 *s = (TexPixel32 *)image->pixels();

    for (int i = 0; i < image->height(); i++)
    {
      for (int j = 0; j < image->width(); j++)
      {
        d[0] = float_to_half((float)s->r / 256.0f);
        d[1] = float_to_half((float)s->g / 256.0f);
        d[2] = float_to_half((float)s->b / 256.0f);

        d += 4;
        s++;
      }
    }
  }

  void EncodeBC6(uint8_t *src, uint8_t *dst, nv::Image *image, bc6h_enc_settings *settings)
  {
    rgba_surface input;
    input.ptr = src;
    input.stride = image->width() * 8;
    input.width = image->width();
    input.height = image->height();

    CompressBlocksBC6H(&input, dst, settings);
  }
#endif

  void save2DDS(IGenSave &cwr, nvtt::TextureType ttype, int w, int h, int d, int mips, DWORD fourcc, Tab<char> &packed, int hq_part)
  {
    DDSURFACEDESC2 targetHeader;
    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;

    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    targetHeader.dwHeight = h;
    targetHeader.dwWidth = w;
    targetHeader.dwDepth = d;

    targetHeader.dwMipMapCount = mips | (hq_part << 16);

    if (ttype == nvtt::TextureType_2D)
      targetHeader.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;
    else if (ttype == nvtt::TextureType_Cube)
    {
      targetHeader.ddsCaps.dwCaps |= 0x00000008U /*DDSCAPS_COMPLEX*/;
      targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP | 0x0000FC00U /*DDSCAPS2_CUBEMAP_ALL_FACES*/;
    }
    else if (ttype == nvtt::TextureType_3D)
    {
      targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_VOLUME;
      targetHeader.dwFlags |= DDSD_DEPTH;
    }
    pf.dwSize = sizeof(DDPIXELFORMAT);
    pf.dwFlags = DDPF_FOURCC;
    pf.dwFourCC = fourcc;

    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cwr.write(&FourCC, sizeof(FourCC));
    cwr.write(&targetHeader, sizeof(targetHeader));

    cwr.write(packed.data(), data_size(packed));
  }

#if !TEX_CANNOT_USE_ISPC
  bool convert_to_bc6(IGenSave &cwr, dag::ConstSpan<ImageSurface> imgSurf, nvtt::TextureType ttype, int voltex_d, const char *quality,
    int hq_part)
  {
    bc6h_enc_settings bc6h_settings;
    chooseBC6HEncodeSettings(&bc6h_settings, quality);

    Tab<uint16_t> pix_argb(tmpmem);
    Tab<TexPixel32> tmp_argb(tmpmem);
    Tab<char> packed(tmpmem);
    int w = imgSurf[0].image->width(), h = imgSurf[0].image->height(), mips = imgSurf[0].imageMips.size() + 1;

    packed.reserve(imgSurf.size() * w * h * 3 / 2);
    for (int f = 0; f < imgSurf.size(); f++)
    {
      nv::Image *image = imgSurf[f].image.ptr();

      pix_argb.resize(w * h * 8);
      swapRGBA16(pix_argb, image);

      tmp_argb.resize(w * h);

      int p_base = append_items(packed, w * h);
      EncodeBC6((uint8_t *)pix_argb.data(), (uint8_t *)&packed[p_base], image, &bc6h_settings);

      for (int i = 0; i < imgSurf[f].imageMips.size(); i++)
      {
        swapRGBA16(pix_argb, imgSurf[f].imageMips[i]);
        EncodeBC6((uint8_t *)pix_argb.data(), (uint8_t *)tmp_argb.data(), imgSurf[f].imageMips[i], &bc6h_settings);

        append_items(packed, getMipSize(imgSurf[f].imageMips[i]->width(), imgSurf[f].imageMips[i]->height()), (char *)tmp_argb.data());
      }
    }
    clear_and_shrink(pix_argb);
    clear_and_shrink(tmp_argb);
    packed.shrink_to_fit();

    save2DDS(cwr, ttype, w, h / voltex_d, voltex_d, mips, MAKEFOURCC('B', 'C', '6', 'H'), packed, hq_part);
    return true;
  }

  void chooseBC7EncodeSettings(bc7_enc_settings *settings, const char *quality)
  {
    if (strcmp(quality, "") == 0)
    {
      GetProfile_alpha_basic(settings);
    }
    else if (strcmp(quality, "fastest") == 0)
    {
      GetProfile_alpha_veryfast(settings);
    }
    else if (strcmp(quality, "normal") == 0)
    {
      GetProfile_alpha_fast(settings);
    }
    else if (strcmp(quality, "production") == 0)
    {
      GetProfile_alpha_basic(settings);
    }
    else if (strcmp(quality, "highest") == 0)
    {
      GetProfile_alpha_slow(settings);
    }
  }

  void swapRGBA(Tab<TexPixel32> &pix_argb, nv::Image *image)
  {
    TexPixel32 *d = pix_argb.data();

    for (TexPixel32 *s = (TexPixel32 *)image->pixels(), *se = s + image->width() * image->height(), *d = pix_argb.data(); s < se;
         s++, d++)
    {
      d->u = s->r | (unsigned(s->g) << 8) | (unsigned(s->b) << 16) | (unsigned(s->a) << 24);
    }
  }

  void EncodeBC7(uint8_t *src, uint8_t *dst, nv::Image *image, bc7_enc_settings *settings)
  {
    rgba_surface input;
    input.ptr = src;
    input.stride = image->width() * 4;
    input.width = image->width();
    input.height = image->height();

    CompressBlocksBC7(&input, dst, settings);
  }

  bool convert_to_bc7(IGenSave &cwr, dag::ConstSpan<ImageSurface> imgSurf, nvtt::TextureType ttype, int voltex_d, const char *quality,
    int hq_part)
  {
    bc7_enc_settings bc7_settings;
    chooseBC7EncodeSettings(&bc7_settings, quality);

    Tab<TexPixel32> pix_argb(tmpmem);
    Tab<TexPixel32> tmp_argb(tmpmem);
    Tab<char> packed(tmpmem);
    int w = imgSurf[0].image->width(), h = imgSurf[0].image->height(), mips = imgSurf[0].imageMips.size() + 1;

    packed.reserve(imgSurf.size() * w * h * 3 / 2);
    for (int f = 0; f < imgSurf.size(); f++)
    {
      nv::Image *image = imgSurf[f].image.ptr();

      pix_argb.resize(w * h);
      swapRGBA(pix_argb, image);

      tmp_argb.resize(w * h);

      int p_base = append_items(packed, w * h);
      EncodeBC7((uint8_t *)pix_argb.data(), (uint8_t *)&packed[p_base], image, &bc7_settings);

      for (int i = 0; i < imgSurf[f].imageMips.size(); i++)
      {
        swapRGBA(pix_argb, imgSurf[f].imageMips[i]);
        EncodeBC7((uint8_t *)pix_argb.data(), (uint8_t *)tmp_argb.data(), imgSurf[f].imageMips[i], &bc7_settings);

        append_items(packed, getMipSize(imgSurf[f].imageMips[i]->width(), imgSurf[f].imageMips[i]->height()), (char *)tmp_argb.data());
      }
    }
    clear_and_shrink(pix_argb);
    clear_and_shrink(tmp_argb);
    packed.shrink_to_fit();

    save2DDS(cwr, ttype, w, h / voltex_d, voltex_d, mips, MAKEFOURCC('B', 'C', '7', ' '), packed, hq_part);
    return true;
  }
#endif
#ifdef _TARGET_EXPORTERS_STATIC
  bool convert_to_astc(IGenSave &, dag::ConstSpan<ImageSurface>, nvtt::TextureType, int, bool, unsigned, int, const char *)
  {
    return false;
  }
#else
  bool convert_to_astc(IGenSave &cwr, dag::ConstSpan<ImageSurface> imgSurf, nvtt::TextureType ttype, int voltex_d, bool gamma1,
    unsigned astc_format, int hq_part, const char *a_name)
  {
    Tab<char> packed(tmpmem);
    int w = imgSurf[0].image->width(), h = imgSurf[0].image->height(), mips = imgSurf[0].imageMips.size() + 1;
    int fourcc = 0, blk_w = 0, blk_h = 0;
    switch (astc_format)
    {
      case 0x114:
        fourcc = MAKEFOURCC('A', 'S', 'T', '4');
        blk_w = blk_h = 4;
        break;
      case 0x118:
        fourcc = MAKEFOURCC('A', 'S', 'T', '8');
        blk_w = blk_h = 8;
        break;
      case 0x11C:
        fourcc = MAKEFOURCC('A', 'S', 'T', 'C');
        blk_w = blk_h = 12;
        break;
      default: return false;
    }

    ASTCEncoderHelperContext astcenc_ctx;
    if (!astcenc_ctx.prepareTmpFilenames(a_name))
      return false;
    packed.reserve(((w + blk_w - 1) / blk_w) * ((h + blk_h - 1) / blk_w) * 16 * 3 / 2);

    String astc_block_str(0, "%dx%d", blk_w, blk_h);
    for (int f = 0; f < imgSurf.size(); f++)
    {
      nv::Image *image = imgSurf[f].image.ptr();

      for (int y0 = 0, slice_h = h / voltex_d; y0 < h; y0 += slice_h)
      {
        if (!astcenc_ctx.writeTga(((TexPixel32 *)image->pixels()) + w * y0, w, slice_h))
          return false;
        if (!astcenc_ctx.buildOneSurface(packed, astc_block_str, astcenc_jobs_limit))
          return false;
      }
      for (int i = 0; i < imgSurf[f].imageMips.size(); i++)
      {
        nv::Image *mip = imgSurf[f].imageMips[i];
        int mipw = mip->width(), miph = mip->height();
        for (int y0 = 0, slice_h = miph / eastl::max(voltex_d >> (i + 1), 1); y0 < miph; y0 += slice_h)
        {
          if (!astcenc_ctx.writeTga(((TexPixel32 *)mip->pixels()) + mipw * y0, mipw, slice_h))
            return false;
          if (!astcenc_ctx.buildOneSurface(packed, astc_block_str, astcenc_jobs_limit))
            return false;
        }
      }
    }

    save2DDS(cwr, ttype, w, h / voltex_d, voltex_d, mips, fourcc, packed, hq_part);
    return true;
  }
#endif
  bool pixelDiffLessThan(nv::AutoPtr<nv::Image> &image, TexPixel32 *base_img, int x0, int y0, E3DCOLOR diff_tolerance)
  {
    TexPixel32 *img = (TexPixel32 *)image->pixels();
    base_img += y0 * image->width() + x0;
    img += y0 * image->width() + x0;
    for (int i = 0; i < 4; i++, img += image->width() - 4, base_img += image->width() - 4)
      for (int j = 0; j < 4; j++, img++, base_img++)
        if (abs(int(img->r) - int(base_img->r)) > diff_tolerance.r || abs(int(img->g) - int(base_img->g)) > diff_tolerance.g ||
            abs(int(img->b) - int(base_img->b)) > diff_tolerance.b || abs(int(img->a) - int(base_img->a)) > diff_tolerance.a)
          return false;
    return true;
  }

  bool checkPaired(const DagorAsset &a, const DataBlock &props, const char *name)
  {
#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
    if (const char *paired = GET_PROP(Str, "pairedToTex", NULL))
      if (*paired)
      {
        int nid = a.props.getNameId("pairedToTex");
        for (int i = 0; i < props.paramCount(); i++)
          if (props.getParamNameId(i) == nid && props.getParamType(i) == DataBlock::TYPE_STRING)
            if (stricmp(props.getStr(i), name) == 0)
              return true;
        if (&a.props != &props)
          for (int i = 0; i < a.props.paramCount(); i++)
            if (a.props.getParamNameId(i) == nid && a.props.getParamType(i) == DataBlock::TYPE_STRING)
              if (stricmp(a.props.getStr(i), name) == 0)
                return true;
      }
#undef GET_PROP
    return false;
  }

  static inline unsigned short to_4444(unsigned p)
  {
    unsigned a = p >> 24, r = (p >> 16) & 0xff, g = (p >> 8) & 0xff, b = p & 0xff;
    return ((a + 15) * 15 / 255) | (((b + 15) * 15 / 255) << 4) | (((g + 15) * 15 / 255) << 8) | (((r + 15) * 15 / 255) << 12);
  }
  static inline unsigned short to_5551(unsigned p)
  {
    unsigned a = p >> 24, r = (p >> 16) & 0xff, g = (p >> 8) & 0xff, b = p & 0xff;
    return (a >= 127 ? 1 : 0) | (((b + 7) * 31 / 255) << 1) | (((g + 7) * 31 / 255) << 6) | (((r + 7) * 31 / 255) << 11);
  }
  static inline unsigned short to_565(unsigned p)
  {
    unsigned r = (p >> 16) & 0xff, g = (p >> 8) & 0xff, b = p & 0xff;
    return ((b + 7) * 31 / 255) | (((g + 3) * 63 / 255) << 5) | (((r + 7) * 31 / 255) << 11);
  }
  static inline unsigned short to_r8g8(unsigned p)
  {
    unsigned short r = (p >> 16) & 0xff, g = (p >> 8) & 0xff;
    return r | (g << 8);
  }

  static bool convertPixFmt(MemorySaveCB &cwr, MemoryLoadCB &crd, ImgBitFormat pix_format)
  {
    DDSURFACEDESC2 targetHeader;
    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;

    cwr.writeInt(crd.readInt());
    crd.read(&targetHeader, sizeof(targetHeader));
    if (!(targetHeader.dwMipMapCount & 0x7F))
      targetHeader.dwMipMapCount++;

    switch (pix_format)
    {
      case IMG_BITFMT_R8:
        pf.dwFlags = DDPF_RGB;
        pf.dwRGBBitCount = 8;
        pf.dwRGBAlphaBitMask = 0x0000;
        pf.dwBBitMask = 0x0000;
        pf.dwGBitMask = 0x0000;
        pf.dwRBitMask = 0x00FF;
        break;
      case IMG_BITFMT_RGBA4444:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0x000F;
        pf.dwBBitMask = 0x00F0;
        pf.dwGBitMask = 0x0F00;
        pf.dwRBitMask = 0xF000;
        break;
      case IMG_BITFMT_RGBA5551:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0x0001;
        pf.dwBBitMask = 0x003E;
        pf.dwGBitMask = 0x07C0;
        pf.dwRBitMask = 0xF800;
        break;
      case IMG_BITFMT_RGB565:
        pf.dwFlags = DDPF_RGB;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0;
        pf.dwBBitMask = 0x001F;
        pf.dwGBitMask = 0x07E0;
        pf.dwRBitMask = 0xF800;
        break;
      case IMG_BITFMT_R8G8:
        pf.dwFlags = DDPF_RGB;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0x0000;
        pf.dwBBitMask = 0x0000;
        pf.dwGBitMask = 0xFF00;
        pf.dwRBitMask = 0x00FF;
        break;
      case IMG_BITFMT_A8R8:
        pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        pf.dwRGBBitCount = 16;
        pf.dwRGBAlphaBitMask = 0xFF00;
        pf.dwBBitMask = 0x0000;
        pf.dwGBitMask = 0x0000;
        pf.dwRBitMask = 0x00FF;
        break;
      default: return false;
    }
    cwr.write(&targetHeader, sizeof(targetHeader));

    int w = targetHeader.dwWidth, h = targetHeader.dwHeight;
    for (int i = 0; i < (targetHeader.dwMipMapCount & 0x7F); i++)
    {
      int num_pix = w * h;
      unsigned pix32;

      switch (pix_format)
      {
        case IMG_BITFMT_R8:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<1>(crd.readInt() >> 16);
          break;
        case IMG_BITFMT_RGBA4444:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<2>(to_4444(crd.readInt()));
          break;
        case IMG_BITFMT_RGBA5551:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<2>(to_5551(crd.readInt()));
          break;
        case IMG_BITFMT_RGB565:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<2>(to_565(crd.readInt()));
          break;
        case IMG_BITFMT_A8R8:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<2>(crd.readInt() >> 16);
          break;
        case IMG_BITFMT_R8G8:
          for (int j = num_pix; j > 0; j--)
            cwr.writeIntP<2>(to_r8g8(crd.readInt()));
          break;
        default: return false;
      }
      if (w > 1)
        w >>= 1;
      if (h > 1)
        h >>= 1;
    }
    return true;
  }

  struct ErrorHandler : public nvtt::ErrorHandler
  {
    virtual void error(nvtt::Error e) { logerr("nvtt: '%s'", nvtt::errorString(e)); }
  };
  struct OutputHandler : public nvtt::OutputHandler
  {
    MemorySaveCB *cwr;
    int hqPartLev;

    OutputHandler() : cwr(NULL), hqPartLev(0) {}
    virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) {}
    virtual bool writeData(const void *data, int size)
    {
      cwr->write(data, size);
      if (hqPartLev && cwr->tell() > sizeof(DDSURFACEDESC2) + 4)
      {
        int pos = cwr->tell();
        cwr->seekto(4 + offsetof(DDSURFACEDESC2, dwMipMapCount) + 2);
        cwr->write(&hqPartLev, sizeof(short));
        // debug("DDS: written %d, seekto %d, write %d", pos, cwr->tell()-2, hqPartLev);
        cwr->seekto(pos);
        hqPartLev = 0;
      }
      return true;
    }
  };

  static int normalize(int val, int max_val)
  {
    if (val < 0)
      return val + max_val;
    if (val >= max_val)
      return val - max_val;
    return val;
  }

  //==============================================================================
  static Color4 color4(TexPixel32 c)
  {
    return Color4(c.r * (1.0f / 255.f), c.g * (1.0f / 255.f), c.b * (1.0f / 255.f), c.a * (1.0f / 255.f));
  }
  static TexPixel32 texpixel32(const Color4 &c)
  {
    TexPixel32 res;
    res.r = real2uchar(c.r);
    res.g = real2uchar(c.g);
    res.b = real2uchar(c.b);
    res.a = real2uchar(c.a);
    return res;
  }
  static Color4 getBilinearFilteredPixelColor(TexPixel32 *tex, double u, double v, int w, int h)
  {
    int x = floor(u), y = floor(v);
    double u_ratio = u - x, v_ratio = v - y;
    double u_opposite = 1 - u_ratio, v_opposite = 1 - v_ratio;
    int nx = clamp(x + 1, 0, w - 1), ny = clamp(y + 1, 0, h - 1);
    Color4 result = (color4(tex[x + y * w]) * u_opposite + color4(tex[nx + y * w]) * u_ratio) * v_opposite +
                    (color4(tex[x + ny * w]) * u_opposite + color4(tex[nx + ny * w]) * u_ratio) * v_ratio;
    return result;
  }

  static void generate_texture_alpha(TexPixel32 *src, int srcW, int srcH, int srcPitch, TexPixel32 *dst, int dstW, int dstH,
    int dstPitch, int filter, int atest, bool wrapU, bool wrapV)
  {
    float blockSizeX = (srcW / (float)dstW);
    float blockSizeY = (srcH / (float)dstH);

    float offsetX = (blockSizeX - 1) * 0.5f;
    float offsetY = (blockSizeY - 1) * 0.5f;
    for (int y = 0; y < dstH; ++y)
    {
      // int yOrig = y*srcH/dstH;
      float orig_y = ((float)(srcH - blockSizeY) * (float)y / (float)((dstH > 1 ? dstH : 2) - 1)) + offsetY;
      int yOrig = int(orig_y);
      for (int x = 0; x < dstW; ++x, ++dst)
      {
        // int xOrig = x*srcW/dstW;
        float orig_x = ((float)(srcW - blockSizeX) * (float)x / (float)((dstW > 1 ? dstW : 2) - 1)) + offsetX;
        int xOrig = int(orig_x);
        bool isTransp = src[xOrig + yOrig * srcPitch].a <= atest;
        int nearestDist2 = filter * filter;
        for (int fi = 1; fi <= filter; ++fi)
        {
          int lj = yOrig - fi, hj = yOrig + fi;
          int li = xOrig - fi, hi = xOrig + fi;
          if (!wrapV)
          {
            lj = max(0, lj), hj = min(srcH - 1, hj);
          }
          if (!wrapU)
          {
            li = max(0, li), hi = min(srcW - 1, hi);
          }
          int nhj = normalize(hj, srcH), nlj = normalize(lj, srcH);
          int nhi = normalize(hi, srcW), nli = normalize(li, srcW);
#define BODY(xi, yj, dxi, dyj)                                                    \
  {                                                                               \
    if ((src[xi + yj * srcPitch].a <= atest) != isTransp)                         \
    {                                                                             \
      int distSq = (dxi - xOrig) * (dxi - xOrig) + (dyj - yOrig) * (dyj - yOrig); \
      if (distSq < nearestDist2)                                                  \
        nearestDist2 = distSq;                                                    \
    }                                                                             \
  }

          for (int j = lj; j <= hj; ++j)
          {
            int nj = normalize(j, srcH);
            BODY(nli, nj, li, j)
            BODY(nhi, nj, hi, j)
          }
          for (int i = li + 1; i <= hi - 1; ++i)
          {
            int ni = normalize(i, srcW);
            BODY(ni, nlj, i, lj)
            BODY(ni, nhj, i, hj)
          }
          if (nearestDist2 < fi * fi)
            break;
        }
        float dist = sqrtf(nearestDist2) / filter;
        if (isTransp)
          dist = -dist;
        int distI = 127 + dist * 127;
        if (distI < 0)
          distI = 0;
        else if (distI > 255)
          distI = 255;
        dst->a = distI;
      }
      dst += dstPitch - dstW;
    }
  }

  static void update_image(TexImage32 *img, TexPixel32 *src, int atest, bool clampu, bool clampv, bool mipFast = true,
    bool saveAlpha = false)
  {
    TexPixel32 *dst = img->getPixels();
    SmallTab<uint8_t, TmpmemAlloc> updateda, updatedb;
    clear_and_resize(updateda, img->h * img->w);
    memcpy(dst, src, img->w * img->h * 4);
    uint8_t *updated = updateda.data();

    for (int i = 0, y = 0; y < img->h; y++)
      for (int x = 0; x < img->w; x++, ++i)
        if (src[i].a >= atest)
          updateda[i] = 255;
        else
          updateda[i] = 0;
    int current_gen = 255;
    for (int mip = 2, iter = 0; iter < 50; mip = mipFast ? mip * 2 : mip + 1, ++iter)
    {
      int updated_pixels = 0;
      for (int y = 0; y < img->h; y += mip)
        for (int x = 0; x < img->w; x += mip)
        {
          int filtered = 0, r = 0, g = 0, b = 0, a = 0;
          int start = mipFast ? 0 : -mip / 2, end = mipFast ? mip - 1 : mip / 2;
          for (int j = start; j <= end; j++)
            for (int i = start; i <= end; i++)
            {
              int paddr = addr(x + i, img->w, clampu) + addr(y + j, img->h, clampv) * img->w;
              TexPixel32 *pix = dst + paddr;
              if (updated[paddr] >= current_gen)
              {
                r += pix->r;
                g += pix->g;
                b += pix->b;
                a += pix->a;
                filtered++;
                continue;
              }
            }
          if (!filtered || filtered == (end - start + 1) * (end - start + 1))
            continue;
          r /= filtered;
          g /= filtered;
          b /= filtered;
          a /= filtered;
          for (int j = start; j <= end; j++)
            for (int i = start; i <= end; i++)
            {
              int paddr = addr(x + i, img->w, clampu) + addr(y + j, img->h, clampv) * img->w;
              TexPixel32 *pix = dst + paddr;
              if (!updated[paddr])
              {
                pix->r = r;
                pix->g = g;
                pix->b = b;
                if (saveAlpha)
                  pix->a = a;
                updated[paddr] = current_gen - 1;
                updated_pixels++;
                continue;
              }
            }
        }
      if (!updated_pixels)
        break;
      current_gen--;
    }
  }

  static void mip_fade_image(TexPixel32 *img, int size, Point4 fadeFactor, Point4 mipFadeColor)
  {
    for (TexPixel32 *de = img + size; img < de; img++)
    {
      if (mipFadeColor.x > 0)
        img->r = int(lerp(float(img->r), mipFadeColor.x, fadeFactor.x));
      if (mipFadeColor.y > 0)
        img->g = int(lerp(float(img->g), mipFadeColor.y, fadeFactor.y));
      if (mipFadeColor.z > 0)
        img->b = int(lerp(float(img->b), mipFadeColor.z, fadeFactor.z));
      if (mipFadeColor.w > 0)
        img->a = int(lerp(float(img->a), mipFadeColor.w, fadeFactor.w));
    }
  }

  static bool resolve_base_image_fname(DagorAsset &a, const char *bimg_nm, String &bfn, const char *tag, ILogWriter &log)
  {
    String bimg_nm_stor;
    if (bimg_nm && *bimg_nm)
    {
      bfn = a.getFolderPath();
      if (bimg_nm[0] == '*')
      {
        bfn.resize(DAGOR_MAX_PATH);
        dd_get_fname_location(bfn, a.getTargetFilePath());
        bfn.updateSz();
        bimg_nm++;
      }
      else if (bimg_nm[0] == '@')
      {
        bimg_nm++;
        DagorAsset *a_bimg = a.getMgr().findAsset(bimg_nm, a.getType());
        if (!a_bimg)
        {
          log.addMessage(log.ERROR, "%s: invalid base image asset %s for %s", a.getName(), bimg_nm, tag);
          return false;
        }
        bimg_nm_stor = a_bimg->getTargetFilePath();
        bimg_nm = bimg_nm_stor;
        bfn = "";
      }

      if (bfn.length())
        bfn.aprintf(0, "/%s", bimg_nm);
      else
        bfn = bimg_nm;
      simplify_fname(bfn);
      return true;
    }
    log.addMessage(log.ERROR, "%s: invalid base image %s for %s", a.getName(), bimg_nm, tag);
    return false;
  }

  static bool ablend_image(TexImage32 *img, const char *base_fpath, bool &alpha_used, const char *a_name, const char *b_name,
    ILogWriter &log, const char *base_type, const char *blend_type)
  {
    TexImage32 *bimg = ::load_image(base_fpath, tmpmem, &alpha_used);
    if (!bimg)
    {
      log.addMessage(log.ERROR, "%s: cannot load base %s %s (path=%s)", a_name, base_type, b_name, base_fpath);
      return false;
    }
    if (img->w != bimg->w || img->h != bimg->h)
    {
      log.addMessage(log.ERROR, "%s: base %s %s size mismatch %dx%d != %dx%d", a_name, base_type, b_name, bimg->w, bimg->h, img->w,
        img->h);
      tmpmem->free(bimg);
      return false;
    }
    int pcnt = img->w * img->h;
    if (strcmp(blend_type, "ablend") == 0)
    {
      for (TexPixel32 *d = img->getPixels(), *de = d + pcnt, *b = bimg->getPixels(); d < de; d++, b++)
        if (d->a == 255)
          d->a = b->a;
        else if (!d->a)
          *d = *b;
        else
        {
          d->r = int(b->r) + (int(d->r) - int(b->r)) * d->a / 255;
          d->g = int(b->g) + (int(d->g) - int(b->g)) * d->a / 255;
          d->b = int(b->b) + (int(d->b) - int(b->b)) * d->a / 255;
          d->a = b->a;
        }
    }
    else if (strlen(blend_type) == 4 && strchr("argbARGB01", blend_type[0]) && strchr("argbARGB01", blend_type[1]) &&
             strchr("argbARGB01", blend_type[2]) && strchr("argbARGB01", blend_type[3]))
    {
      unsigned andm[2][4], orm[4], shr[2][4];
      memset(andm, 0, sizeof(andm));
      memset(orm, 0, sizeof(orm));
      memset(shr, 0, sizeof(shr));
      for (int i = 0; i < 4; i++)
        switch (blend_type[i])
        {
          case 'a':
            andm[0][i] = 0xFF000000;
            shr[0][i] = 24;
            break;
          case 'r':
            andm[0][i] = 0x00FF0000;
            shr[0][i] = 16;
            break;
          case 'g':
            andm[0][i] = 0x0000FF00;
            shr[0][i] = 8;
            break;
          case 'b':
            andm[0][i] = 0x000000FF;
            shr[0][i] = 0;
            break;
          case 'A':
            andm[1][i] = 0xFF000000;
            shr[1][i] = 24;
            break;
          case 'R':
            andm[1][i] = 0x00FF0000;
            shr[1][i] = 16;
            break;
          case 'G':
            andm[1][i] = 0x0000FF00;
            shr[1][i] = 8;
            break;
          case 'B':
            andm[1][i] = 0x000000FF;
            shr[1][i] = 0;
            break;
          case '1': orm[i] = 0xFF; break;
          case '0': /*leave zeroes*/ break;
        }
      for (TexPixel32 *d = img->getPixels(), *de = d + pcnt, *b = bimg->getPixels(); d < de; d++, b++)
      {
        unsigned img0 = d->u;
        unsigned img1 = b->u;
        d->a = ((img0 & andm[0][0]) >> shr[0][0]) | ((img1 & andm[1][0]) >> shr[1][0]) | orm[0];
        d->r = ((img0 & andm[0][1]) >> shr[0][1]) | ((img1 & andm[1][1]) >> shr[1][1]) | orm[1];
        d->g = ((img0 & andm[0][2]) >> shr[0][2]) | ((img1 & andm[1][2]) >> shr[1][2]) | orm[2];
        d->b = ((img0 & andm[0][3]) >> shr[0][3]) | ((img1 & andm[1][3]) >> shr[1][3]) | orm[3];
      }
    }
    else
    {
      log.addMessage(log.ERROR, "%s: unsupported imageMergePattern=\"%s\"", a_name, blend_type);
      tmpmem->free(bimg);
      return false;
    }
    tmpmem->free(bimg);
    return true;
  }

  static bool process_split(int &inout_skip_levels, int &inout_levels, int w, int h, int splitAt, bool splitHigh, int &out_hq_lev)
  {
    if (!splitAt)
      return true;
    // debug("process_split(inout_skip_levels=%d, inout_levels=%d, w=%d, h=%d, cp=%d,%d)",
    //   inout_skip_levels, inout_levels, w, h, splitAt, splitHigh);

    if (splitHigh)
    {
      if ((w >> inout_skip_levels) <= splitAt && (h >> inout_skip_levels) <= splitAt)
        return false;

      if (!inout_levels)
        inout_levels = max(get_log2i(w), get_log2i(h)) + 1;
      if ((w >> (inout_skip_levels + inout_levels - 1)) > splitAt || (h >> (inout_skip_levels + inout_levels - 1)) > splitAt)
        return false;

      for (int i = 0; i < inout_levels; i++)
      {
        if (w <= splitAt && h <= splitAt)
        {
          inout_levels = i + 1; // we generate +1 level (upper mip of LQ-part) to show DDSx-cvt that texture WAS split
          break;
        }
        w = w >> 1;
        h = h >> 1;
        if (w < 1)
          w = 1;
        if (h < 1)
          h = 1;
      }
    }
    else
    {
      for (int i = 0; i < inout_skip_levels; i++)
      {
        w = w >> 1;
        h = h >> 1;
        if (w < 1)
          w = 1;
        if (h < 1)
          h = 1;
      }
      if (!inout_levels)
        inout_levels = max(get_log2i(w), get_log2i(h)) + 1;
      if ((w >> (inout_levels - 1)) > splitAt || (h >> (inout_levels - 1)) > splitAt)
        return true;
      while (inout_levels > 1)
      {
        if (w > splitAt || h > splitAt)
        {
          inout_skip_levels++;
          out_hq_lev++;
          inout_levels--;
        }
        else
          break;
        w = w >> 1;
        h = h >> 1;
        if (w < 1)
          w = 1;
        if (h < 1)
          h = 1;
      }
    }
    // debug("  -> inout_skip_levels=%d, inout_levels=%d, hq_lev=%d", inout_skip_levels, inout_levels, out_hq_lev);
    return true;
  }


  static int getImageCompIdx(const char *param_nm, const DataBlock &props, const char *img_swz, DagorAsset &a, ILogWriter &log,
    bool optional = false)
  {
    const char *param_val = props.getStr(param_nm, "");
    if (!param_val[0] && optional)
      return -1;
    if (strncmp(param_val, "src:", 4) == 0 && param_val[4] && !param_val[5])
    {
      const char *p = strchr(img_swz, param_val[4]);
      if (p)
        return img_swz + 3 - p;
      log.addMessage(log.ERROR, "%s: bad %s=%s in processMips, swizzle=%s", a.getName(), param_nm, param_val, img_swz);
      return -1;
    }
    else if (strncmp(param_val, "swizzled:", 9) == 0 && param_val[9] && !param_val[10])
      switch (param_val[9])
      {
        case 'A': return 3;
        case 'R': return 2;
        case 'G': return 1;
        case 'B': return 0;
      }
    log.addMessage(log.ERROR, "%s: bad %s=%s in processMips, expecting <src|swizzled>:<R|G|B|A>", a.getName(), param_nm, param_val);
    return -1;
  }

  static __forceinline void packNorm(float x, float y, uint8_t &pnx, uint8_t &pny)
  {
    pnx = clamp(int(128.f + x * 127.0f), 0, 255);
    pny = clamp(int(128.f + y * 127.0f), 0, 255);
  }
  static __forceinline bool unpackNorm(float &x, float &y, float &z, uint8_t pnx, uint8_t pny)
  {
    x = clamp((pnx - 128.0f) / 127.0f, -1.0f, 1.0f);
    y = clamp((pny - 128.0f) / 127.0f, -1.0f, 1.0f);
    const float x2y2 = x * x + y * y;
    if (x2y2 < 1)
    {
      z = safe_sqrt(1.0f - x2y2);
      return true;
    }
    else
    {
      const float len = sqrtf(x2y2);
      x /= len;
      y /= len;
      z = 0;
      return false;
    }
  }
  static void addUnpackedNorm(float *n, uint8_t pnx, uint8_t pny, float wt)
  {
    float x, y, z;
    unpackNorm(x, y, z, pnx, pny);
    n[0] += x * wt;
    n[1] += y * wt;
    n[2] += z * wt;
  }

  static bool downsample_float4(Point3_vec4 *normals, int w, int h, int dst_w, int dst_h)
  {
    if (dst_w * 2 == w && dst_h * 2 == h)
      imagefunctions::downsample4x_simdu(&normals->x, &normals->x, dst_w, dst_h);
    else if (dst_w * 2 == w && dst_h == h)
      imagefunctions::downsample2x_simdu(&normals->x, &normals->x, dst_w);
    else if (dst_w == w && dst_h * 2 == h)
      for (int y = 0, j = 0; y < dst_h; y++)
      {
        auto *pln0 = &normals[(y * 2 + 0) * w];
        auto *pln1 = &normals[(y * 2 + 1) * w];
        for (int x = 0; x < dst_w; x++, pln0++, pln1++)
          *pln0 = 0.5f * (*pln0 + *pln1);
      }
    else
      return false;
    return true;
  }
  static bool processMips(nv::Image *image, dag::ConstSpan<nv::Image *> imageMips, const DataBlock &props, const char *img_swz,
    DagorAsset &a, ILogWriter &log)
  {
    if (!imageMips.size())
      return true;

    int nxc = getImageCompIdx("Nx", props, img_swz, a, log), nyc = getImageCompIdx("Ny", props, img_swz, a, log),
        trc = getImageCompIdx("translucency", props, img_swz, a, log, true),
        smc = getImageCompIdx("smoothness", props, img_swz, a, log, true),
        rhc = getImageCompIdx("roughness", props, img_swz, a, log, true);

    if (nxc < 0 || nyc < 0)
      return false;
    if (smc < 0 && rhc < 0)
    {
      log.addMessage(log.ERROR, "%s: neither %s nor %s defined in processMips", a.getName(), "smoothness", "roughness");
      return false;
    }
    if (smc >= 0 && rhc >= 0)
    {
      log.addMessage(log.ERROR, "%s: both %s and %s defined in processMips", a.getName(), "smoothness", "roughness");
      return false;
    }
    int destc = rhc;
    // our smoothness is 1-linearRoughness
    float rn_a = 1.0f, rn_b = 0.0f; // channel-to-roughness coefs (and back too - they are supposed to be symmetrical)
    if (destc < 0)
    {
      destc = smc;
      rn_a = -1.0f, rn_b = 1.0f;
    }

    SmallTab<Point3_vec4, MidmemAlloc> shortenedNormals;
    clear_and_resize(shortenedNormals, image->width() * image->height());
    for (int y = 0, j = 0, ye = image->height(); y < ye; y++)
    {
      uint8_t *pln = (uint8_t *)image->scanline(y + 0);
      for (int x = 0, xe = image->width(); x < xe; x++, pln += 4, ++j)
      {
        Point3 n;
        if (!unpackNorm(n.x, n.y, n.z, pln[nxc], pln[nyc])) // not normalized
          packNorm(n.x, n.y, pln[nxc], pln[nyc]);           // resave original normal, so it is normalized
        const float linearRoughness = (rn_a * pln[destc] / 255.0f + rn_b);
        const float ggxAlpha = linearRoughness * linearRoughness;
        const float normal_len = (useMicroSurface ? AlphaToMicroNormalLength(ggxAlpha) : AlphaToMacroNormalLength(ggxAlpha));
        shortenedNormals[j] = normal_len * n;
      }
    }

    bool translucencyMaxMipFilter = props.getBool("translucencyMaxMipFilter", false);
    if (translucencyMaxMipFilter && trc < 0)
    {
      log.addMessage(log.ERROR, "%s: %s:b=yes but %s:t channel not set", a.getName(), "translucencyMaxMipFilter", "translucency");
      return false;
    }
    int translucencyStartMip = props.getInt("translucencyStartMip", 0);
    float translucencyWhitoOutBaseVal = props.getInt("translucencyWhitoOutBaseVal", 100) / 255.0f;
    float translucencyWhiteoutMips = props.getInt("translucencyWhitoOutMipCnt", 0);
    float translucencyScale = (translucencyWhitoOutBaseVal > 0 && translucencyWhiteoutMips > 0)
                                ? pow(1.0f / translucencyWhitoOutBaseVal, 1.0f / translucencyWhiteoutMips)
                                : 1.0f;
    translucencyScale = props.getReal("translucencyScale", translucencyScale);
    bool translucencyScaleUsed = fabsf(translucencyScale - 1.0f) > 1e-6;
    if (translucencyScaleUsed && trc < 0)
    {
      log.addMessage(log.ERROR, "%s: %s requested but %s:t channel not set", a.getName(), "translucencyScale", "translucency");
      return false;
    }

    // debug("Nc=%d,%d DSTc=%d, dest_val=%d, var_s=%.3f var_min=%g var_pow=%.3f",
    //   nxc, nyc, destc, dest_a, variance_s, variance_min, variance_pow);
    Tab<uint8_t> max_transl;
    if (imageMips[0]->width() * 2 != image->width() || imageMips[0]->height() * 2 != image->height())
    {
      log.addMessage(log.ERROR, "%s: img=%dx%d mip0=%dx%d not supported", a.getName(), image->width(), image->height(),
        imageMips[0]->width(), imageMips[0]->height());
      return false;
    }
    if (translucencyMaxMipFilter)
    {
      max_transl.resize(imageMips[0]->width() * imageMips[0]->height());
      for (int y = 0, j = 0, img_h = imageMips[0]->height(); y < img_h; y++)
      {
        uint8_t *pln0 = (uint8_t *)image->scanline(y * 2 + 0);
        uint8_t *pln1 = (uint8_t *)image->scanline(y * 2 + 1);
        for (int x = 0, img_w = imageMips[0]->width(); x < img_w; x++, j++, pln0 += 8, pln1 += 8)
          max_transl[j] = max(max(pln0[0 + trc], pln0[4 + trc]), max(pln1[0 + trc], pln1[4 + trc]));
      }
    }

    float rhScale = 1.0f;
    float transl_scale = 1.0f;
    for (int i = 0; i < imageMips.size(); i++)
    {
      const int src_w = i == 0 ? image->width() : imageMips[i - 1]->width(),
                src_h = i == 0 ? image->height() : imageMips[i - 1]->height();
      const int dst_w = imageMips[i]->width(), dst_h = imageMips[i]->height();
      if (!downsample_float4(&shortenedNormals[0], src_w, src_h, dst_w, dst_h))
        log.addMessage(log.ERROR, "%s: mipPrev=%dx%d mipNext=%dx%d (non-pow-2?)", a.getName(), src_w, src_h, dst_w, dst_h);
      if (i >= translucencyStartMip)
        transl_scale *= translucencyScale;
      rhScale = props.getReal(String(0, "roughnessScale%d", i), rhScale);
      float varScale = props.getReal(String(0, "varianceScale%d", i), 1.0f);
      float destcScale = props.getReal(String(0, "destCompScale%d", i), 1.0f);
      // debug("mip %d", i);
      for (int y = 0, ni = 0; y < dst_h; y++)
      {
        uint8_t *pix = (uint8_t *)imageMips[i]->scanline(y);
        // debug(" line %d", y);
        for (int x = 0; x < dst_w; x++, ++ni)
        {
          if (i >= translucencyStartMip)
          {
            if (translucencyMaxMipFilter)
              pix[x * 4 + trc] = max_transl[ni];
            if (translucencyScaleUsed)
              pix[x * 4 + trc] = clamp(int(pix[x * 4 + trc] * transl_scale), 0, 255);
          }
          const Point3 normal = shortenedNormals[ni];
          const float len = length(normal);
          const float ggxAlpha = lenToAlphaLut.lenToAlpha(len);
          const float linearRoughness = sqrtf(ggxAlpha) * rhScale;
          pix[x * 4 + destc] = clamp(int((rn_a * linearRoughness + rn_b) * 255.0f), 0, 255);
          packNorm(normal.x / max(len, 1e-6f), normal.y / max(len, 1e-6f), pix[x * 4 + nxc], pix[x * 4 + nyc]);
          // debug("  %.3f,%.3f,%.3f->%.3f (%.4f) %02X->%02X (%.4f -> %.4f)",
          //   norm[j+0], norm[j+1], norm[j+2], avgNormLen, variance, c, pix[x*4+destc], rn_a*c/255.0f + rn_b, roughness);
        }
      }

      if (i + 1 == imageMips.size())
        continue;

      const int img_w = imageMips[i + 1]->width(), img_h = imageMips[i]->height();
      const int img2_w = imageMips[i + 1]->width(), img2_h = imageMips[i + 1]->height();
      if (translucencyMaxMipFilter)
      {
        Tab<uint8_t> max_transl2;
        max_transl2.resize(img2_w * img2_h);
        mem_set_0(max_transl2);
        if (img2_w * 2 == img_w && img2_h * 2 == img_h)
          for (int y = 0, j = 0; y < img2_h; y++)
          {
            uint8_t *pln0 = &max_transl[(y * 2 + 0) * img_w];
            uint8_t *pln1 = &max_transl[(y * 2 + 1) * img_w];
            for (int x = 0; x < img2_w; x++, pln0 += 2, pln1 += 2, j++)
              max_transl2[j] = max(max(pln0[0], pln0[1]), max(pln1[0], pln1[1]));
          }
        else if (img2_w == img_w && img2_h * 2 == img_h)
          for (int y = 0, j = 0; y < img2_h; y++)
          {
            uint8_t *pln0 = &max_transl[(y * 2 + 0) * img_w];
            uint8_t *pln1 = &max_transl[(y * 2 + 1) * img_w];
            for (int x = 0; x < img2_w; x++, pln0 += 1, pln1 += 1, j++)
              max_transl2[j] = max(pln0[0], pln1[0]);
          }
        else if (img2_w * 2 == img_w && img2_h == img_h)
          for (int y = 0, j = 0; y < img2_h; y++)
          {
            uint8_t *pln0 = &max_transl[y * img_w];
            for (int x = 0; x < img2_w; x++, pln0 += 2, j++)
              max_transl2[j] = max(pln0[0], pln0[1]);
          }
        max_transl = eastl::move(max_transl2);
      }
    }

    return true;
  }

  static TexImage32 *decode_image_from_dds(DagorAsset &a, bool allow_mipstripe, int &out_mipCnt, int &mipCustomFilt, bool &alpha_used,
    ILogWriter &log)
  {
    nv::DirectDrawSurface dds(a.getTargetFilePath());
    if (!dds.isValid())
    {
      log.addMessage(log.ERROR, "%s: invalid DDS %s %s", a.getName(), a.getTargetFilePath(),
        dd_file_exists(a.getTargetFilePath()) ? "" : "(file not found)");
      return NULL;
    }
    if (!dds.isTexture2D())
    {
      log.addMessage(log.ERROR, "%s: not Tex2D %s", a.getName(), a.getTargetFilePath());
      return NULL;
    }
    TexImage32 *img = TexImage32::create(dds.width() * (allow_mipstripe && dds.mipmapCount() > 1 ? 2 : 1), dds.height());
    if (dds.mipmapCount() > 1 && allow_mipstripe)
      mipCustomFilt = MMF_stripe;

    nv::Image image;
    for (int i = 0, x = 0; i < dds.mipmapCount(); i++)
    {
      dds.mipmap(&image, 0, i);
      if (i == 0)
        for (int y = 0; y < image.height(); y++)
        {
          TexPixel32 *pix = (TexPixel32 *)image.scanline(y);
          for (int x = 0; x < image.width(); x++, pix++)
            if (pix->a != 255)
            {
              alpha_used = true;
              break;
            }
          if (alpha_used)
            break;
        }

      for (int y = 0; y < image.height(); y++)
        memcpy(img->getPixels() + x + img->w * y, image.scanline(y), image.width() * 4);
      if (!allow_mipstripe)
        break;
      x += image.width();
    }
    if (allow_mipstripe && (out_mipCnt < 0 || out_mipCnt >= dds.mipmapCount()))
      out_mipCnt = dds.mipmapCount() > 1 ? dds.mipmapCount() - 1 : 0;
    return img;
  }
  static void store_empty_dds(mkbindump::BinDumpSaveCB &cwr)
  {
    // store empty DDS
    DDSURFACEDESC2 targetHeader;

    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    targetHeader.dwHeight = 0;
    targetHeader.dwWidth = 0;
    targetHeader.dwDepth = 1;
    targetHeader.dwMipMapCount = 0;
    targetHeader.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    targetHeader.ddsCaps.dwCaps2 = 0;

    // dds output
    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cwr.writeRaw(&FourCC, sizeof(FourCC));
    cwr.writeRaw(&targetHeader, sizeof(targetHeader));
  }

  static bool make_ddsx_hdr(ddsx::Header &hdr, const DDSURFACEDESC2 &dds)
  {
    static struct BitMaskFormat
    {
      uint32_t bitCount;
      uint32_t alphaMask;
      uint32_t redMask;
      uint32_t greenMask;
      uint32_t blueMask;
      uint32_t format;
      uint32_t flags;
    } bitMaskFormat[] = {
      {24, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_R8G8B8, DDPF_RGB},
      {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8, DDPF_RGB | DDPF_ALPHA},
      {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8, DDPF_RGB | DDPF_ALPHAPIXELS},
      {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8, DDPF_RGB},
      {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5, DDPF_RGB},
      {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5, DDPF_RGB | DDPF_ALPHA},
      {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5, DDPF_RGB},
      {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4, DDPF_RGB | DDPF_ALPHA},
      {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2, DDPF_RGB},
      {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8, DDPF_ALPHA},
      {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8, DDPF_LUMINANCE},
      {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8, DDPF_RGB},
      {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2, DDPF_RGB | DDPF_ALPHA},
      {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4, DDPF_RGB},
      {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8B8G8R8, DDPF_RGB | DDPF_ALPHA},
      {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8B8G8R8, DDPF_RGB},
      {16, 0x00000000, 0x0000ff, 0xff00, 0x00, D3DFMT_V8U8, DDPF_BUMPDUDV},
      {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8, DDPF_LUMINANCE | DDPF_ALPHA},
      {16, 0x00000000, 0xFFFF, 0x00000000, 0, D3DFMT_L16, DDPF_LUMINANCE},
    };
    static const int dxgi_format_bc4_unorm = 80; // DXGI_FORMAT_BC4_UNORM
    static const int dxgi_format_bc5_unorm = 83; // DXGI_FORMAT_BC5_UNORM

    memset(&hdr, 0, sizeof(hdr));
    hdr.w = dds.dwWidth;
    hdr.h = dds.dwHeight;
    hdr.depth = dds.dwDepth;
    hdr.levels = (dds.dwFlags & DDSD_MIPMAPCOUNT) ? dds.dwMipMapCount : 1;

    if (dds.ddpfPixelFormat.dwFlags & DDPF_FOURCC)
      hdr.d3dFormat = dds.ddpfPixelFormat.dwFourCC;
    else
      for (const auto &bmf : bitMaskFormat)
        if (dds.ddpfPixelFormat.dwFlags == bmf.flags && dds.ddpfPixelFormat.dwRGBBitCount == bmf.bitCount &&
            dds.ddpfPixelFormat.dwRBitMask == bmf.redMask && dds.ddpfPixelFormat.dwGBitMask == bmf.greenMask &&
            dds.ddpfPixelFormat.dwBBitMask == bmf.blueMask && dds.ddpfPixelFormat.dwRGBAlphaBitMask == bmf.alphaMask)
        {
          hdr.d3dFormat = bmf.format;
          hdr.bitsPerPixel = bmf.bitCount;
          break;
        }

    if (hdr.d3dFormat == MAKEFOURCC('A', 'S', 'T', '4'))
      hdr.dxtShift = 5, hdr.bitsPerPixel = 8, hdr.flags |= ddsx::Header::FLG_MOBILE_TEXFMT;
    else if (hdr.d3dFormat == MAKEFOURCC('A', 'S', 'T', '8'))
      hdr.dxtShift = 6, hdr.bitsPerPixel = 2, hdr.flags |= ddsx::Header::FLG_MOBILE_TEXFMT;
    else if (hdr.d3dFormat == MAKEFOURCC('A', 'S', 'T', 'C'))
      hdr.dxtShift = 7, hdr.bitsPerPixel = 0, hdr.flags |= ddsx::Header::FLG_MOBILE_TEXFMT;
    if (hdr.d3dFormat == D3DFMT_DXT1 || hdr.d3dFormat == _MAKE4C('ATI1') || hdr.d3dFormat == dxgi_format_bc4_unorm)
      hdr.dxtShift = 3;
    else if (hdr.d3dFormat == D3DFMT_DXT2 || hdr.d3dFormat == D3DFMT_DXT3 || hdr.d3dFormat == D3DFMT_DXT4 ||
             hdr.d3dFormat == D3DFMT_DXT5 || hdr.d3dFormat == _MAKE4C('ATI2') || hdr.d3dFormat == dxgi_format_bc5_unorm ||
             hdr.d3dFormat == _MAKE4C('BC6H') || hdr.d3dFormat == _MAKE4C('BC7 '))
      hdr.dxtShift = 4;

    switch (hdr.d3dFormat)
    {
      case D3DFMT_A16B16G16R16:
      case D3DFMT_A16B16G16R16F:
      case TEXFMT_A16B16G16R16S:
      case TEXFMT_A16B16G16R16UI: hdr.bitsPerPixel = 64; break;

      case D3DFMT_A32B32G32R32F:
      case TEXFMT_A32B32G32R32UI: hdr.bitsPerPixel = 128; break;

      case D3DFMT_R32F:
      case D3DFMT_A2B10G10R10:
      case D3DFMT_G16R16F:
      case D3DFMT_G16R16:
      case TEXFMT_R32UI:
      case TEXFMT_R11G11B10F: hdr.bitsPerPixel = 32; break;

      case D3DFMT_R16F:
      case D3DFMT_L16:
      case TEXFMT_R8G8: hdr.bitsPerPixel = 16; break;
    }
    if (!hdr.bitsPerPixel && !hdr.dxtShift)
      debug("unrecognized %s format: 0x%x flg=0x%x bits=%d R=0x%x G=0x%x B=0x%x A=0x%x (%dx%d,L%d)",
        (dds.ddpfPixelFormat.dwFlags & DDPF_FOURCC) ? "4CC" : "RGBA", hdr.d3dFormat, dds.ddpfPixelFormat.dwFlags,
        dds.ddpfPixelFormat.dwRGBBitCount, dds.ddpfPixelFormat.dwRBitMask, dds.ddpfPixelFormat.dwGBitMask,
        dds.ddpfPixelFormat.dwBBitMask, dds.ddpfPixelFormat.dwRGBAlphaBitMask, dds.dwWidth, dds.dwHeight, dds.dwMipMapCount);
    return hdr.bitsPerPixel || hdr.dxtShift;
  }
};

class TexRefs : public IDagorAssetRefProvider
{
public:
  virtual const char *__stdcall getRefProviderIdStr() const { return "tex refs"; }

  virtual const char *__stdcall getAssetType() const { return TYPE; }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  dag::ConstSpan<Ref> __stdcall getAssetRefs(DagorAsset &a) override { return {}; }
  dag::ConstSpan<Ref> __stdcall getAssetRefsEx(DagorAsset &a, unsigned target, const char *profile) override
  {
    static Ref ref;

    TextureMetaData tmd;
    tmd.read(a.props, a.resolveEffProfileTargetStr(mkbindump::get_target_str(target), profile));
    if (!dagor_target_code_supports_tex_diff(target))
      tmd.baseTexName = NULL;
    if (tmd.baseTexName.empty())
      return {};

#define GET_PROP(TYPE, PROP, DEF) props.get##TYPE(PROP, &props != &a.props ? a.props.get##TYPE(PROP, DEF) : DEF)
    const DataBlock &props = a.getProfileTargetProps(target, profile);
    bool splitHigh = GET_PROP(Bool, "splitHigh", false);
    bool splitBasePart = !splitHigh && a.props.getInt("splitAt", 0) > 0;

    if (splitBasePart && !GET_PROP(Bool, "rtMipGenBQ", false))
      return {}; // BQ doesn't require baseTex

    ref.flags = RFLG_EXTERNAL;
    ref.refAsset = a.getMgr().findAsset(String(0, "%s%s", tmd.baseTexName, splitHigh ? "$hq" : ""), a.getType());
    if (!ref.refAsset && splitHigh)
      ref.refAsset = a.getMgr().findAsset(tmd.baseTexName, a.getType());
    if (!ref.refAsset)
      ref.setBrokenRef(String(128, "%s:%s", tmd.baseTexName, TYPE));

    return make_span_const(&ref, 1);
  }
};

class TexExporterPlugin : public IDaBuildPlugin
{
public:
  virtual bool __stdcall init(const DataBlock &appblk)
  {
    appBlkCopy.setFrom(&appblk, appblk.resolveFilename());
    blkAssetsBuildTex.setFrom(appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("tex"));
    texRtMipGenAllowed = appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("export")->getBool("rtMipGenAllowed", true);
    register_tga_tex_load_factory();
    register_tiff_tex_load_factory();
    register_psd_tex_load_factory();
    register_jpeg_tex_load_factory();
    register_png_tex_load_factory();
    register_avif_tex_load_factory();

    const DataBlock &build_blk = *appBlkCopy.getBlockByNameEx("assets")->getBlockByNameEx("build");
    if (build_blk.getBool("preferZSTD", false))
    {
      preferZstdPacking = true;
      unsigned zstdMaxWindowLog = build_blk.getInt("zstdMaxWindowLog", 0);
      int zstdCompressionLevel = build_blk.getInt("zstdCompressionLevel", 18);
      debug("texExp prefers ZSTD (compressionLev=%d %s)", zstdCompressionLevel,
        zstdMaxWindowLog ? String(0, "maxWindow=%u", 1 << zstdMaxWindowLog).c_str() : "defaultWindow");
    }
    if (build_blk.getBool("allowOODLE", false))
    {
      allowOodlePacking = true;
      debug("texExp allows OODLE");
    }
    if (auto *b = build_blk.getBlockByNameEx("tex")->getBlockByName("PC"))
    {
      if (b->getBool("allowASTC", false))
      {
        pcAllowsASTC = true;
        debug("texExp allows ASTC for PC");
      }
      if (b->getBool("allowASTC_fallback_to_ARGB_non_pow_2", false))
      {
        pcAllowsASTC_to_ARGB = true;
        debug("texExp allows fallback from ASTC to ARGB for non-pow-2 tex for PC");
      }
    }

    ASTCEncoderHelperContext::setupAstcEncExePathname();
    if (int jobs = appblk.getInt("dabuildJobCount", 0))
    {
      unsigned core_count = 0;
      if (cpujobs::is_inited())
        core_count = cpujobs::get_core_count();
      else
      {
        cpujobs::init(0, false);
        core_count = cpujobs::get_core_count();
        cpujobs::term(true, 0);
      }
      astcenc_jobs_limit = core_count / jobs;
    }
    DEBUG_DUMP_VAR(astcenc_jobs_limit);
    return true;
  }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int idx) { return TYPE; }
  virtual IDagorAssetExporter *__stdcall getExp(int idx) { return &exp; }

  virtual int __stdcall getRefProvCount() { return 1; }
  virtual const char *__stdcall getRefProvType(int idx) { return TYPE; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return &ref; }

protected:
  TexExporter exp;
  TexRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) TexExporterPlugin; }
DABUILD_PLUGIN_API void __stdcall dabuild_plugin_install_dds_helper(
  int(__stdcall *hlp)(IGenSave &, DagorAsset &, unsigned, const char *, ILogWriter *))
{
  write_built_dds_final = hlp;
}
END_DABUILD_PLUGIN_NAMESPACE(tex)
USING_DABUILD_PLUGIN_NAMESPACE(tex)
DAG_DECLARE_RELOCATABLE(TexExporter::ImageSurface);
REGISTER_DABUILD_PLUGIN(tex, dabuild_plugin_install_dds_helper)
