// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/binDumpHierBitmap.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/hash.h>
#include <math/dag_Point2.h>
#include <de3_bitMaskMgr.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_btagCompr.h>
#include <debug/dag_debug.h>
#include <libTools/util/twoStepRelPath.h>
#include "../asset_ref_hlp.h"

static const char *TYPE = "land";
static IBitMaskImageMgr *bmSrv = NULL;
static bool preferZstdPacking = false;
static bool report_broken_landclass_tex_refs = false;

static TwoStepRelPath sdkRoot;

extern void *get_tiff_bit_mask_image_mgr();

BEGIN_DABUILD_PLUGIN_NAMESPACE(land)

class LandClassExporter : public IDagorAssetExporter
{
  struct Bitmap1
  {
    Bitmap1(const IBitMaskImageMgr::BitmapMask &_img) : img(_img), ymax(img.getHeight() - 1) {}
    int getW() const { return img.getWidth(); }
    int getH() const { return img.getHeight(); }
    int get(int x, int y) const { return img.getMaskPixel1(x, ymax - y); }
    const IBitMaskImageMgr::BitmapMask &img;
    int ymax;
  };
  struct Bitmap8
  {
    Bitmap8(const IBitMaskImageMgr::BitmapMask &_img) : img(_img), ymax(img.getHeight() - 1) {}
    int getW() const { return img.getWidth(); }
    int getH() const { return img.getHeight(); }
    int get(int x, int y) const { return img.getMaskPixel8(x, ymax - y) > 127; }
    const IBitMaskImageMgr::BitmapMask &img;
    int ymax;
  };

public:
  const char *__stdcall getExporterIdStr() const override { return "land exp"; }

  const char *__stdcall getAssetType() const override { return TYPE; }
  unsigned __stdcall getGameResClassId() const override { return 0x03FB59C4u; }
  unsigned __stdcall getGameResVersion() const override { return preferZstdPacking ? 16 : 15; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    int tifmask_atype = a.getMgr().getAssetTypeId("tifMask");

    int nid_obj_plant_generate = a.props.getNameId("obj_plant_generate");
    for (int i = 0; i < a.props.blockCount(); i++)
      if (a.props.getBlock(i)->getBlockNameId() == nid_obj_plant_generate)
      {
        const char *densMap = a.props.getBlock(i)->getStr("densityMap", NULL);
        if (!densMap)
          continue;

        DagorAsset *tm_a = get_asset_by_name(a.getMgr(), densMap, tifmask_atype);
        if (!tm_a)
          continue;
        SimpleString fn(tm_a->getTargetFilePath());
        if (find_value_idx(files, fn) < 0)
          files.push_back(fn);
      }
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    DataBlock landBlk;
    TwoStepRelPath::storage_t tmp_stor;

    // clean up land BLK
    landBlk = a.props;
    Tab<IDagorAssetRefProvider::Ref> unused_refs;
    processAssetBlk(landBlk, a.getMgr(), unused_refs, true, a.getName(), &log);
    if (!validateLandBlk(landBlk, log, a.getName(), a.props.getBool("allowEqualSeed", false)))
      return false;
    landBlk.setBool("hasHash", true);

    mkbindump::BinDumpSaveCB lcwr(8 << 20, cwr);
    // write land BLK
    write_ro_datablock(lcwr, landBlk);

    // gather and write density maps
    Tab<SimpleString> files(tmpmem);
    gatherSrcDataFiles(a, files);

    lcwr.writeInt32e(files.size());
    for (int i = 0; i < files.size(); i++)
    {
      IBitMaskImageMgr::BitmapMask img;
      if (bmSrv->loadImage(img, NULL, files[i]))
      {
        if ((img.getWidth() & 15) | (img.getHeight() & 15))
        {
          log.addMessage(ILogWriter::ERROR, "density map <%s> size must multiple of 16; it is %dx%d %dbpp", files[i], img.getWidth(),
            img.getHeight(), img.getBitsPerPixel());
          return false;
        }

        lcwr.writeDwString(sdkRoot.mkRelPath(files[i], tmp_stor));
        if (img.getBitsPerPixel() == 1)
          mkbindump::build_ro_hier_bitmap(lcwr, Bitmap1(img), 4, 3);
        else if (img.getBitsPerPixel() == 8)
          mkbindump::build_ro_hier_bitmap(lcwr, Bitmap8(img), 4, 3);
        else
        {
          log.addMessage(ILogWriter::ERROR, "bad TIFF: %s", files[i].str());
          return false;
        }
        for (int j = 0, nid = landBlk.getNameId("obj_plant_generate"); j < landBlk.blockCount(); j++)
          if (landBlk.getBlock(j)->getBlockNameId() == nid)
            if (const char *map_nm = landBlk.getBlock(j)->getStr("densityMap", NULL))
              if (dd_stricmp(map_nm, sdkRoot.mkRelPath(files[i], tmp_stor)) == 0)
              {
                static const int CSZ = 1 << 4;
                float land_density = landBlk.getBlock(j)->getReal("density", 0);
                float cellSz_x = landBlk.getBlock(j)->getPoint2("mapSize", Point2(32, 32)).x / img.getWidth();
                float cellSz_y = landBlk.getBlock(j)->getPoint2("mapSize", Point2(32, 32)).y / img.getHeight();
                float density = land_density * cellSz_x * cellSz_y / 100.0 * CSZ * CSZ;
                if (land_density > 0 && density < 1.0 / 64.0 && cellSz_x * cellSz_y < 1)
                  log.addMessage(ILogWriter::WARNING,
                    "%s:land\n"
                    "  obj_plant_generate[%d]: density=%g or density map cell size=%gx%g is too low (per %dx%d-block density=%g < "
                    "1/64)\n"
                    "  densityMap=\"%s\"",
                    a.getName(), j, land_density, cellSz_x, cellSz_y, CSZ, CSZ, density, map_nm);
              }
      }
      else
      {
        log.addMessage(ILogWriter::ERROR, "failed to read TIFF: %s", files[i].str());
        return false;
      }
    }

    cwr.beginBlock();
    MemoryLoadCB mcrd(lcwr.getMem(), false);
    if (preferZstdPacking)
      zstd_compress_data(cwr.getRawWriter(), mcrd, lcwr.getSize());
    else
      lzma_compress_data(cwr.getRawWriter(), 9, mcrd, lcwr.getSize());
    cwr.endBlock(preferZstdPacking ? btag_compr::ZSTD : btag_compr::UNSPECIFIED);

    cwr.beginBlock();
    char hash[32];
    IGenSave *hcb = create_hash_computer_cb(HASH_SAVECB_SHA1);
    lcwr.copyDataTo(*hcb);
    cwr.writeRaw(hash, get_computed_hash(hcb, hash, sizeof(hash)));
    destory_hash_computer_cb(hcb);
    cwr.endBlock();

    return true;
  }

  static void processDetailsDataBlock(const DataBlock &blk, Tab<IDagorAssetRefProvider::Ref> &refs, DagorAssetMgr &amgr,
    const char *a_name)
  {
    const int detailTypeNameCount = 3;
    static const char *detailTypeNames[detailTypeNameCount] = {"albedo", "normal", "reflectance"};

    const int blockCount = blk.blockCount();
    for (int blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
      const DataBlock &subBlock = *blk.getBlock(blockIndex);
      for (int detailTypeIndex = 0; detailTypeIndex < detailTypeNameCount; ++detailTypeIndex)
      {
        const int paramIndex = subBlock.findParam(detailTypeNames[detailTypeIndex]);
        if (paramIndex >= 0 && subBlock.getParamType(paramIndex) == DataBlock::TYPE_STRING)
        {
          const char *textureName = subBlock.getStr(paramIndex);
          if (textureName && *textureName)
          {
            const int refCount = refs.size();
            add_asset_ref(refs, amgr, textureName, false, false, -1, true);
            if (refs.size() != refCount && refs.back().getBrokenRef() && !report_broken_landclass_tex_refs)
            {
              refs.pop_back();
              logwarn("Asset '%s' has broken reference: '%s'", a_name, textureName);
            }
          }
        }
      }
    }
  }

  static void processAssetBlk(DataBlock &blk, DagorAssetMgr &amgr, Tab<IDagorAssetRefProvider::Ref> &ri_list, bool cleanup,
    const char *a_name, ILogWriter *log)
  {
    Tab<IDagorAssetRefProvider::Ref> other_refs(tmpmem);
    FastNameMap ri_nm;
    int nid_detail = blk.getNameId("detail");
    int nid_resources = blk.getNameId("resources");
    int nid_obj_plant_generate = blk.getNameId("obj_plant_generate");
    int nid_placeAboveHt = blk.getNameId("placeAboveHt");
    int nid_secondaryLayer = blk.getNameId("secondaryLayer");
    int nid_layerIdx = blk.getNameId("layerIdx");
    int nid_object = blk.getNameId("object");
    int ri_atype = amgr.getAssetTypeId("rendInst");
    int coll_atype = amgr.getAssetTypeId("collision");
    int tex_atype = amgr.getTexAssetTypeId();
    int tifmask_atype = amgr.getAssetTypeId("tifMask");
    Point2 area0 = blk.getPoint2("landBBoxLim0", Point2(0, 0));
    Point2 area1 = blk.getPoint2("landBBoxLim1", blk.getBlockByNameEx("detail")->getPoint2("size", Point2(0, 0)));
    TwoStepRelPath::storage_t tmp_stor;

    ri_list.clear();
    other_refs.clear();

    for (int blki = 0; blki < blk.blockCount(); ++blki)
    {
      DataBlock &dataBlk = *blk.getBlock(blki);
      if (dataBlk.getBlockNameId() != nid_obj_plant_generate)
        continue;
      const int densmap = dataBlk.getNameId("densityMap");
      if (densmap == -1)
        continue;
      int firstDenseMapIdx = -1;
      for (int i = 0; i < dataBlk.paramCount(); ++i)
      {
        if (dataBlk.getParamNameId(i) != densmap)
          continue;
        firstDenseMapIdx = i;
        break;
      }
      for (int i = dataBlk.paramCount() - 1; i > firstDenseMapIdx; --i)
      {
        if (dataBlk.getParamNameId(i) != densmap)
          continue;
        DataBlock *secondaryDensityMapData = blk.addNewBlock(&dataBlk, "obj_plant_generate");
        secondaryDensityMapData->removeParam("densityMap");
        secondaryDensityMapData->setStr("densityMap", dataBlk.getStr(i));
        secondaryDensityMapData->setInt("rseed", secondaryDensityMapData->getInt("rseed", 12345) + i);
        dataBlk.removeParam(i);
      }
    }

    for (int i = blk.blockCount() - 1; i >= 0; i--)
      if (blk.getBlock(i)->getBlockNameId() == nid_resources || blk.getBlock(i)->getBlockNameId() == nid_obj_plant_generate)
      {
        DataBlock &b = *blk.getBlock(i);
        b.removeBlock("colliders");

        for (int j = b.blockCount() - 1; j >= 0; j--)
          if (b.getBlock(j)->getBlockNameId() == nid_object)
          {
            const char *nm = b.getBlock(j)->getStr("name", NULL);
            if (is_explicit_asset_name_non_exportable(amgr, nm))
            {
              add_asset_ref(other_refs, amgr, nm, false, false, -1, true);
              if (cleanup)
                blk.getBlock(i)->getBlockNameId() == nid_resources ? b.removeBlock(j) : b.getBlock(j)->removeParam("name");
              continue;
            }

            DagorAsset *ri_a = nm ? get_asset_by_name(amgr, nm, -1) : NULL;
            if (!ri_a)
            {
              ri_nm.addNameId(nm);
              IDagorAssetRefProvider::Ref &r1 = ri_list.push_back();
              r1.flags = IDagorAssetRefProvider::RFLG_EXTERNAL;
              r1.setBrokenRef(nm);

              IDagorAssetRefProvider::Ref &r2 = ri_list.push_back();
              r2.refAsset = NULL;
              r2.flags = IDagorAssetRefProvider::RFLG_EXTERNAL | IDagorAssetRefProvider::RFLG_OPTIONAL;
            }
            if (!ri_a || ri_a->getType() != ri_atype)
            {
              if (cleanup)
                blk.getBlock(i)->getBlockNameId() == nid_resources ? b.removeBlock(j) : b.getBlock(j)->removeParam("name");
              continue;
            }

            int idx = ri_nm.addNameId(ri_a->getName());
            if (idx >= ri_list.size() / 2)
            {
              IDagorAssetRefProvider::Ref &r = ri_list.push_back();
              r.refAsset = ri_a;
              r.flags = IDagorAssetRefProvider::RFLG_EXTERNAL;

              IDagorAssetRefProvider::Ref &r2 = ri_list.push_back();
              r2.refAsset = get_asset_by_name(amgr, String(128, "%s_collision", ri_a->getName()), coll_atype);
              r2.flags = IDagorAssetRefProvider::RFLG_EXTERNAL | IDagorAssetRefProvider::RFLG_OPTIONAL;
            }
            if (cleanup)
            {
              b.getBlock(j)->removeParam("name");
              b.getBlock(j)->setInt("riResId", idx);
            }
          }
        if (b.getBlockNameId() == nid_obj_plant_generate)
        {
          const char *densMap = b.getStr("densityMap", NULL);
          if (!add_asset_ref(other_refs, amgr, densMap, false, false, tifmask_atype, false))
            continue;

          if (!cleanup)
            continue;
          if (DagorAsset *tm_a = other_refs.back().getAsset())
            b.setStr("densityMap", sdkRoot.mkRelPath(tm_a->getTargetFilePath(), tmp_stor));
          else
            b.removeParam("densityMap");
        }
      }
      else if (blk.getBlock(i)->getBlockNameId() == nid_detail)
      {
        static const char *texRef[] = {"texture", "splattingmap", "detail"};

        bool editable = blk.getBlock(i)->getBool("editable", false);
        for (int pi = 0; pi < blk.getBlock(i)->paramCount(); ++pi)
        {
          if (blk.getBlock(i)->getParamType(pi) != DataBlock::TYPE_STRING)
            continue;
          const char *name = blk.getBlock(i)->getParamName(pi);
          for (int j = 0; j < sizeof(texRef) / sizeof(texRef[0]); j++)
          {
            if (j < 2 && strcmp(name, texRef[j]) != 0)
              continue;
            if (j >= 2 && strstr(name, texRef[j]) == 0)
              continue;
            const char *tex = blk.getBlock(i)->getStr(pi);
            if (tex[0] == '*' && j < 2 && editable)
              continue;
            if (add_asset_ref(other_refs, amgr, tex, false, false, tex_atype, false))
            {
              blk.getBlock(i)->setStr(pi, other_refs.back().getAsset() ? other_refs.back().getAsset()->getName() : ""); //-V522
              if (!other_refs.back().getAsset() && log)
                log->addMessage(ILogWriter::ERROR, "%s: missing <%s:tex>", a_name, tex);
            }
            break;
          }
        }

        const int subSubBlockCount = blk.getBlock(i)->blockCount();
        for (int subSubBlockIndex = 0; subSubBlockIndex < subSubBlockCount; ++subSubBlockIndex)
        {
          const DataBlock &subSubBlock = *blk.getBlock(i)->getBlock(subSubBlockIndex);
          if (strcmp(subSubBlock.getBlockName(), "details") == 0)
            processDetailsDataBlock(subSubBlock, other_refs, amgr, a_name);
        }

        // leave data in BLK as is
      }
      else if (cleanup)
        blk.removeBlock(i);

    if (cleanup)
    {
      for (int i = blk.paramCount() - 1; i >= 0; i--)
        if (blk.getParamNameId(i) != nid_placeAboveHt && blk.getParamNameId(i) != nid_secondaryLayer &&
            blk.getParamNameId(i) != nid_layerIdx)
          blk.removeParam(i);
      blk.setPoint2("landBBoxLim0", area0);
      blk.setPoint2("landBBoxLim1", area1);
      blk.setInt("riResCount", ri_list.size() / 2);
      DataBlock &b = *blk.addBlock("posInst");
    }
    append_items(ri_list, other_refs.size(), other_refs.data());
  }

  bool validateLandBlk(const DataBlock &land, ILogWriter &log, const char *a_name, bool allowEqualSeed)
  {
    bool valid = true;
    int nid_resources = land.getNameId("resources");
    int nid_obj_plant_generate = land.getNameId("obj_plant_generate");
    Tab<MemoryChainedData *> tile_blk, plant_blk;

    if (nid_resources >= 0)
      for (int i = 0; i < land.blockCount(); i++)
        if (land.getBlock(i)->getBlockNameId() == nid_resources)
        {
          MemorySaveCB cwr(8 << 10);
          land.getBlock(i)->saveToTextStream(cwr);
          for (int j = 0; j < tile_blk.size(); j++)
            if (cwr.getMem()->cmpEq(plant_blk[j]))
            {
              log.addMessage(ILogWriter::ERROR, "%s: identical resources[%d] and resources[%d]", a_name, tile_blk.size(), j);
              valid = false;
              break;
            }
          tile_blk.push_back(cwr.takeMem());
        }

    if (nid_obj_plant_generate >= 0)
      for (int i = 0; i < land.blockCount(); i++)
        if (land.getBlock(i)->getBlockNameId() == nid_obj_plant_generate)
        {
          MemorySaveCB cwr(8 << 10);
          land.getBlock(i)->saveToTextStream(cwr);
          bool found_eq = false;
          for (int j = 0; j < plant_blk.size(); j++)
            if (cwr.getMem()->cmpEq(plant_blk[j]))
            {
              log.addMessage(ILogWriter::ERROR, "%s: identical obj_plant_generate[%d] and obj_plant_generate[%d]", a_name,
                plant_blk.size(), j);
              valid = false;
              found_eq = true;
              break;
            }
          plant_blk.push_back(cwr.takeMem());

          if (!found_eq && !allowEqualSeed)
            for (int j = 0; j < i; j++)
              if (land.getBlock(j)->getBlockNameId() == nid_obj_plant_generate &&
                  land.getBlock(i)->getInt("rseed", 12345) == land.getBlock(j)->getInt("rseed", 12345))
              {
                log.addMessage(ILogWriter::ERROR, "%s: equal seed=%d in obj_plant_generate blocks without allowEqualSeed:b=yes",
                  a_name, land.getBlock(i)->getInt("rseed", 12345));
                valid = false;
              }
        }

    for (int j = 0; j < plant_blk.size(); j++)
      MemoryChainedData::deleteChain(plant_blk[j]);
    for (int j = 0; j < tile_blk.size(); j++)
      MemoryChainedData::deleteChain(tile_blk[j]);
    return valid;
  }
};

class LandClassRefs : public IDagorAssetRefProvider
{
public:
  const char *__stdcall getRefProviderIdStr() const override { return "land refs"; }

  const char *__stdcall getAssetType() const override { return TYPE; }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall getAssetRefs(DagorAsset &a, Tab<Ref> &refs) override
  {
    LandClassExporter::processAssetBlk(a.props, a.getMgr(), refs, false, a.getName(), nullptr);
  }
};

class LandClassExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override
  {
    bmSrv = (IBitMaskImageMgr *)get_tiff_bit_mask_image_mgr();
    sdkRoot.setSdkRoot(appblk.getStr("appDir", NULL), "develop");

    if (appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBool("preferZSTD", false))
    {
      preferZstdPacking = true;
      debug("landExp prefers ZSTD");
    }

    report_broken_landclass_tex_refs = appblk.getBlockByNameEx("assets")->getBool("report_broken_landclass_tex_refs", false);

    return bmSrv != NULL;
  }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override { return TYPE; }
  IDagorAssetExporter *__stdcall getExp(int idx) override { return &exp; }

  int __stdcall getRefProvCount() override { return 1; }
  const char *__stdcall getRefProvType(int idx) override { return TYPE; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return &ref; }

protected:
  LandClassExporter exp;
  LandClassRefs ref;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) LandClassExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(land)
REGISTER_DABUILD_PLUGIN(land, nullptr)

#include <oldEditor/de_interface.h>
IDagorEd2Engine *IDagorEd2Engine::__dagored_global_instance = NULL;
