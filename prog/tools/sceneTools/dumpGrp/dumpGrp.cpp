// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <windows.h>
#include <d3d9types.h>
#endif
#include <libTools/util/binDumpReader.h>
#include <libTools/util/strUtil.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_gameResHooks.h>
#include <gameRes/dag_stdGameRes.h>
#include <scene/dag_loadLevelVer.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_dynSceneRes.h>
#include <drv/3d/dag_driver.h>
#include <3d/ddsxTex.h>
#include <3d/ddsxTexMipOrder.h>
#include <3d/ddsFormat.h>
#include <3d/dag_texMgr.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_roDataBlock.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <startup/dag_restart.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_fastIntList.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_roNameMap.h>
#include <util/dag_globDef.h>
#include <util/dag_texMetaData.h>
#include <gameRes/grpData.h>
#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

extern bool dump_grp_tex_refs(const char *grp_fn, bool detailed);

static bool quiet_fatal_handler(const char *msg, const char *call_stack, const char *fn, int ln) { return false; }
static bool stub_drv_inited = false;

void __cdecl ctrl_break_handler(int) { quit_game(0); }

namespace gameresprivate
{
extern int curRelFnOfs;

void scanGameResPack(const char *filename);
void scanDdsxTexPack(const char *filename);
} // namespace gameresprivate

struct LevelBinData
{
  FastNameMapEx res;
  FastNameMapEx tex;
};

struct DxpBinData
{
  struct Rec
  {
    TEXTUREID texId;
    int ofs;
    int packedDataSize;
  };
  struct Dump
  {
    RoNameMap texNames;
    PatchableTab<ddsx::Header> texHdr;
    PatchableTab<Rec> texRec;
  };

  Dump *data;

public:
  DxpBinData() : data(NULL) {}
  ~DxpBinData()
  {
    memfree(data, tmpmem);
    data = NULL;
  }
};
DAG_DECLARE_RELOCATABLE(DxpBinData);

static void gather_tex_refs(Tab<Tab<int> *> &texIdPerAsset, FastNameMapEx &texList, gamerespackbin::GrpData &grp)
{
  FastNameMap list;
  bool (*prev_handler)(const char *msg, const char *call_stack, const char *fn, int ln) = dgs_fatal_handler;
  dgs_fatal_handler = quiet_fatal_handler;
  for (int k = 0; k < grp.resData.size(); k++)
    switch (grp.resData[k].classId)
    {
      case DynModelGameResClassId:
      case RendInstGameResClassId:
      case EffectGameResClassId:
        list.reset();
        list.addNameId(grp.getName(grp.resData[k].resId));
        set_required_res_list_restriction(list);
        GameResource *res = get_game_resource(grp.resData[k].resId);
        Tab<int> *texForAsset = texIdPerAsset[k] = new Tab<int>;
        for (TEXTUREID i = first_managed_texture(1); i != BAD_TEXTUREID; i = next_managed_texture(i, 1))
          texForAsset->push_back(texList.addNameId(TextureMetaData::decodeFileName(get_managed_texture_name(i))));
        reset_required_res_list_restriction();
        release_game_resource(res);
        release_all_not_required_res();
        if (!texForAsset->size())
        {
          delete texForAsset;
          texIdPerAsset[k] = NULL;
        }
        break;
    }
  dgs_fatal_handler = prev_handler;
}


static gamerespackbin::GrpData *load_grp(BinDumpReader &crd, int file_size)
{
  gamerespackbin::GrpHeader ghdr;
  gamerespackbin::GrpData *gdata = NULL;

  crd.read32ex(&ghdr, sizeof(ghdr));
  crd.convert32(&ghdr.label, 4);

  if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
    return NULL;
  if (ghdr.restFileSize + sizeof(ghdr) != file_size)
    if (!trail_stricmp(crd.getRawReader().getTargetName(), ".grpcache.bin") || ghdr.fullDataSize + sizeof(ghdr) != file_size)
      return NULL;

  gdata = (gamerespackbin::GrpData *)memalloc(ghdr.fullDataSize, tmpmem);
  crd.getRawReader().read(gdata, ghdr.fullDataSize);

  void *base = gdata->dumpBase();
  crd.convert32(gdata, sizeof(gamerespackbin::GrpData));
  gdata->nameMap.patch(base);
  crd.convert32(gdata->nameMap.data(), data_size(gdata->nameMap));

  gdata->resTable.patch(base);
  gdata->resData.patch(base);
  if (ghdr.label == _MAKE4C('GRP2'))
    gamerespackbin::ResData::cvtRecInplaceVer2to3(gdata->resData.data(), gdata->resData.size());
  for (int i = 0; i < gdata->resTable.size(); i++)
  {
    gdata->resTable[i].classId = crd.convert32(gdata->resTable[i].classId);
    gdata->resTable[i].offset = crd.convert32(gdata->resTable[i].offset);
    gdata->resTable[i].resId = crd.convert16(gdata->resTable[i].resId);
  }
  for (int i = 0; i < gdata->resData.size(); i++)
  {
    gdata->resData[i].classId = crd.convert32(gdata->resData[i].classId);
    gdata->resData[i].resId = crd.convert16(gdata->resData[i].resId);
    gdata->resData[i].refResIdCnt = crd.convert16(gdata->resData[i].refResIdCnt);
    crd.convert32(&gdata->resData[i].refResIdPtr, sizeof(gdata->resData[i].refResIdPtr));
    gdata->resData[i].refResIdPtr.patchNonNull(base);
  }

  return gdata;
}
static LevelBinData *load_level_bin(BinDumpReader &crd)
{
  int tag;

  if (crd.readFourCC() != _MAKE4C('DBLD'))
    return NULL;
  if (crd.readFourCC() != DBLD_Version)
    return NULL;
  crd.readInt32e(); // total tex number

  LevelBinData *lbd = new LevelBinData;
  for (;;)
  {
    tag = crd.beginTaggedBlock();

    if (tag == _MAKE4C('END'))
    {
      // valid end of binary dump
      crd.endBlock();
      return lbd;
    }
    else if (tag == _MAKE4C('RqRL'))
    {
      RoNameMap *rqrl_ronm = (RoNameMap *)memalloc(crd.getBlockRest(), tmpmem);
      crd.getRawReader().read(rqrl_ronm, crd.getBlockRest());
      crd.convert32(rqrl_ronm, 8);
      crd.convert32(((char *)rqrl_ronm) + (intptr_t)rqrl_ronm->map.data(), rqrl_ronm->map.size() * 4);
      rqrl_ronm->patchData(rqrl_ronm);

      for (int i = 0; i < rqrl_ronm->map.size(); i++)
        lbd->res.addNameId(rqrl_ronm->map[i]);
      memfree(rqrl_ronm, tmpmem);
    }
    else if (tag == _MAKE4C('DxP2'))
    {
      String str;
      int tex_count;

      crd.readDwString(str);
      tex_count = crd.readInt32e();

      for (int i = 0; i < tex_count; i++)
      {
        crd.readDwString(str);
        lbd->tex.addNameId(str);
      }
    }

    crd.endBlock();
  }
  return lbd;
}
static DxpBinData *load_tex_pack(BinDumpReader &crd)
{
  DxpBinData *dxp = new DxpBinData;

  unsigned hdr[4];
  crd.read32ex(hdr, sizeof(hdr));

  if (hdr[0] != _MAKE4C('DxP2') || (hdr[1] != 2 && hdr[1] != 3))
    return NULL;

  dxp->data = (DxpBinData::Dump *)memalloc(hdr[3], tmpmem);
  crd.getRawReader().read(dxp->data, hdr[3]);
  crd.convert32(dxp->data, 8 * 3);
  crd.convert32(((char *)dxp->data) + (intptr_t)dxp->data->texNames.map.data(), dxp->data->texNames.map.size() * 4);
  dxp->data->texNames.patchData(dxp->data);
  dxp->data->texHdr.patch(dxp->data);
  dxp->data->texRec.patch(dxp->data);
  if (hdr[1] == 2) // repack texRec (skip int64 at struct begin and int32 and struct end)
  {
    int *dst = (int *)dxp->data->texRec.data();
    int *src = dst + 2;
    for (auto dst_e = (int *)dxp->data->texRec.cend(); dst < dst_e; dst += 3, src += 6)
      dst[0] = 0 /*BAD_TEXTUREID*/, dst[1] = src[1], dst[2] = src[2];
  }
  return dxp;
}

static void dump_grp_external(gamerespackbin::GrpData &grp, bool read_be, DataBlock &out_blk)
{
  Tab<Tab<int> *> texIdPerAsset;
  texIdPerAsset.resize(grp.resData.size());
  mem_set_0(texIdPerAsset);
  FastNameMapEx texList;

  if (stub_drv_inited)
    gather_tex_refs(texIdPerAsset, texList, grp);

  FastIntList localRes, localGameRes;
  FastIntList extRes;
  for (int i = 0; i < grp.resTable.size(); i++)
    localRes.addInt(grp.resTable[i].resId);
  for (int i = 0; i < grp.resData.size(); i++)
    localGameRes.addInt(grp.resData[i].resId);

  printf("external res:\n");
  for (int i = 0; i < grp.resData.size(); i++)
  {
    int resId = grp.resData[i].resId;
    int id = grp.resData[i].resId;

    if (!localRes.hasInt(id))
      if (extRes.addInt(id))
      {
        printf("  [%d] %s  <- [%d] %s\n", id, grp.getName(id), resId, grp.getName(resId));
        out_blk.addStr("ext_res", grp.getName(id));
      }

    for (int j = 0; j < grp.resData[i].refResIdCnt; j++)
    {
      id = grp.resData[i].refResIdPtr[j];
      if (read_be)
        id = BinDumpReader::le2be16(id);

      if (id > grp.nameMap.size())
      {
        // printf("bad ref: %d (%d:%d)\n", id, i, j);
        continue;
      }
      if (!localGameRes.hasInt(id))
        if (extRes.addInt(id))
        {
          printf("  [%d] %s  <- [%d] %s, ref:%d\n", id, grp.getName(id), resId, grp.getName(resId), j);
          out_blk.addStr("ext_res", grp.getName(id));
        }
    }
  }
  if (texList.nameCount())
    printf("\n");
  for (int i = 0; i < texList.nameCount(); i++)
  {
    printf("  [T%4d] %-40s  <- ", i, texList.getName(i));
    for (int j = 0; j < grp.resData.size(); j++)
      if (texIdPerAsset[j] && find_value_idx(*texIdPerAsset[j], i) >= 0)
        printf(" %s", grp.getName(grp.resData[j].resId));
    printf("\n");
  }
}
static void dump_lev_external(LevelBinData &lev, bool read_be, DataBlock &out_blk)
{
  printf("external res:\n");
  for (int i = 0; i < lev.res.nameCount(); i++)
  {
    printf("  [%d] %s\n", i, lev.res.getName(i));
    out_blk.addStr("ext_res", lev.res.getName(i));
  }
  printf("external tex:\n");
  for (int i = 0; i < lev.tex.nameCount(); i++)
  {
    printf("  [%d] %s\n", i, lev.tex.getName(i));
    out_blk.addStr("ext_tex", lev.tex.getName(i));
  }
}

static void dump_blk_external_resolve(const DataBlock &res_lst, BinDumpReader src_crd, const DataBlock &lst)
{
  FastNameMap resName, texName;
  FastNameMapEx grpName, dxpName;
  Tab<int> resGrp(tmpmem), texDxp(tmpmem);
  String path_base(lst.resolveFilename());
  location_from_path(path_base);

  int nid = lst.getNameId("pack");
  const DataBlock *b = lst.getBlockByNameEx("gameResPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      int fnid = grpName.addNameId(b->getStr(i));
      String fn = path_base + grpName.getName(fnid);
      FullFileLoadCB base_crd(fn.str());
      if (!base_crd.fileHandle)
      {
        printf("ERR: can't open %s", fn.str());
        return;
      }
      BinDumpReader crd(&base_crd, src_crd.getTarget(), src_crd.readBE());
      gamerespackbin::GrpData *g2 = load_grp(crd, df_length(base_crd.fileHandle));
      if (!g2)
      {
        printf("ERR: invalid GRP %s", fn.str());
        return;
      }

      FastIntList localRes;
      for (int i = 0; i < g2->resData.size(); i++)
      {
        int id = g2->resData[i].resId;
        if (localRes.addInt(id))
        {
          int rnid = resName.addNameId(g2->getName(id));
          if (rnid == resGrp.size())
            resGrp.push_back(fnid);
        }
      }
      if (g2)
        memfree(g2, tmpmem);
    }

  b = lst.getBlockByNameEx("ddsxTexPacks");
  for (int i = 0; i < b->paramCount(); i++)
    if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
    {
      int fnid = dxpName.addNameId(b->getStr(i));
      String fn = path_base + dxpName.getName(fnid);
      FullFileLoadCB base_crd(fn.str());
      if (!base_crd.fileHandle)
      {
        printf("ERR: can't open %s", fn.str());
        return;
      }
      BinDumpReader crd(&base_crd, src_crd.getTarget(), src_crd.readBE());
      DxpBinData *dxp = load_tex_pack(crd);
      if (!dxp)
      {
        printf("ERR: invalid DXP %s", fn.str());
        return;
      }

      for (int i = 0; i < dxp->data->texNames.map.size(); i++)
      {
        int rnid = texName.addNameId(dxp->data->texNames.map[i]);
        if (rnid == texDxp.size())
          texDxp.push_back(fnid);
      }
      del_it(dxp);
    }

  printf("\nresolving external res:\n");
  for (int i = 0; i < res_lst.paramCount(); i++)
    if (res_lst.getParamType(i) == DataBlock::TYPE_STRING)
    {
      if (trail_strcmp(res_lst.getStr(i), "*"))
      {
        // tex
        int rnid = texName.getNameId(res_lst.getStr(i));
        int gid = rnid >= 0 ? texDxp[rnid] : -1;
        if (gid >= 0)
          printf("  %s -> %s\n", dxpName.getName(gid), res_lst.getStr(i));
        else
          printf("  unresolved -> %s\n", res_lst.getStr(i));
      }
      else
      {
        // res
        int rnid = resName.getNameId(res_lst.getStr(i));
        int gid = rnid >= 0 ? resGrp[rnid] : -1;
        if (gid >= 0)
          printf("  %s -> %s\n", grpName.getName(gid), res_lst.getStr(i));
        else
          printf("  unresolved -> %s\n", res_lst.getStr(i));
      }
    }
}

static void dump_grp_local(gamerespackbin::GrpData &grp, bool read_be)
{
  FastIntList localRes;
  printf("local real res:\n");
  for (int i = 0; i < grp.resData.size(); i++)
  {
    int id = grp.resData[i].resId;
    if (localRes.addInt(id))
      printf("  [%d] %s\n", id, grp.getName(id));
  }
}
static void dump_grp_unref(gamerespackbin::GrpData &grp, bool read_be, DataBlock &out_blk)
{
  FastIntList refRes;

  for (int i = 0; i < grp.resData.size(); i++)
  {
    for (int j = 0; j < grp.resData[i].refResIdCnt; j++)
    {
      int id = grp.resData[i].refResIdPtr[j];
      if (read_be)
        id = BinDumpReader::le2be16(id);

      if (id > grp.nameMap.size())
      {
        // printf("bad ref: %d (%d:%d)\n", id, i, j);
        continue;
      }
      refRes.addInt(id);
    }
  }

  printf("unreferenced res:\n");
  for (int i = 0; i < grp.resData.size(); i++)
  {
    int id = grp.resData[i].resId;
    if (!refRes.hasInt(id))
    {
      printf("  [%d] %s\n", id, grp.getName(id));
      out_blk.addStr("unref_res", grp.getName(id));
    }
  }
}
static void dump_grp_contents(gamerespackbin::GrpData &grp, bool read_be, DataBlock &out_blk)
{
  for (int i = 0; i < grp.resTable.size(); i++)
  {
    printf("%5d: \"%s\" %-*s cls=0x%08X  ofs=0x%08X (%9d) sz=%7d\n", i, grp.getName(grp.resTable[i].resId),
      max(50 - (int)strlen(grp.getName(grp.resTable[i].resId)), 0), "", grp.resTable[i].classId, grp.resTable[i].offset,
      grp.resTable[i].offset, i + 1 < grp.resTable.size() ? grp.resTable[i + 1].offset - grp.resTable[i].offset : -1);
    out_blk.addStr("res", grp.getName(grp.resTable[i].resId));
  }
}

static inline IGenLoad &ptr_to_ref(IGenLoad *crd) { return *crd; }
static void extract_grp_contents(gamerespackbin::GrpData &grp, BinDumpReader &crd, const char *out_dir, int file_sz)
{
  String fn;
  dd_mkdir(out_dir);
  for (int i = 0; i < grp.resTable.size(); i++)
  {
    const char *res_name = grp.getName(grp.resTable[i].resId);
    fn.printf(0, "%s/%s.%08X", out_dir, res_name, grp.resTable[i].classId);
    int ofs = grp.resTable[i].offset;
    int sz = (i + 1 < grp.resTable.size() ? grp.resTable[i + 1].offset : file_sz) - ofs;
    FullFileSaveCB cwr(fn);
    if (!cwr.fileHandle)
    {
      printf("ERROR: cannot create %s\n", fn.str());
      return;
    }
    crd.seekto(ofs);
    copy_stream_to_stream(crd.getRawReader(), cwr, sz);
    cwr.close();

    switch (grp.resTable[i].classId)
    {
      case 0x03FB59C4u: // rendinst::gen::land::HUID_LandClass:
      {
        String prefix(0, "%s/%s.%08X.land", out_dir, res_name, grp.resTable[i].classId);
        crd.seekto(ofs);
        unsigned fmt = 0;
        crd.beginBlock(&fmt);
        IGenLoad &zcrd = (fmt == btag_compr::ZSTD) ? ptr_to_ref(new ZstdLoadCB(crd.getRawReader(), crd.getBlockRest()))
                                                   : ptr_to_ref(new LzmaLoadCB(crd.getRawReader(), crd.getBlockRest()));
        RoDataBlock *roblk = RoDataBlock::load(zcrd);
        dd_mkdir(prefix);
        DataBlock blk;
        blk = *roblk;
        blk.saveToTextFile(prefix + "/land.blk");
        if (roblk->getBlockByName("obj_plant_generate"))
        {
          int dm_cnt = zcrd.readInt();
          for (int i = 0; i < dm_cnt; i++)
          {
            SimpleString s;
            zcrd.readString(s);
            for (char *p = s; *p; p++)
              if (strchr("/\\?*:", *p))
                *p = '_';

            zcrd.readInt();
            zcrd.readInt();
            int dump_sz = zcrd.readInt(), w = zcrd.readInt(), h = zcrd.readInt();

            FullFileSaveCB fcwr(String(0, "%s/%s-%dx%d.bin", prefix, s, w, h));
            fcwr.writeInt(w);
            fcwr.writeInt(h);
            copy_stream_to_stream(zcrd, fcwr, dump_sz - 8);
          }
        }
        memfree(roblk, tmpmem);
        zcrd.ceaseReading();
        delete &zcrd;
        crd.endBlock();
      }
      break;

      case RendInstGameResClassId:
        if (stub_drv_inited)
        {
          crd.seekto(ofs);
          Ptr<RenderableInstanceLodsResource> srcRes =
            RenderableInstanceLodsResource::loadResource(crd.getRawReader(), SRLOAD_SRC_ONLY, res_name);

          DataBlock riBlk;
          riBlk.setInt("lods", srcRes->lods.size());
          riBlk.setPoint3("bbox.0", srcRes->bbox[0]);
          riBlk.setPoint3("bbox.1", srcRes->bbox[1]);
          riBlk.setPoint3("bsphCenter", srcRes->bsphCenter);
          riBlk.setReal("bsphRad", srcRes->bsphRad);
          riBlk.setReal("bound0rad", srcRes->bound0rad);

          Tab<ShaderMaterial *> mat_list;
          srcRes->gatherUsedMat(mat_list);
          for (int m = 0; m < mat_list.size(); m++)
            riBlk.addStr(String(0, "mat%02d", m), mat_list[m]->getInfo());

          for (int lod = 0; lod < srcRes->lods.size(); lod++)
            riBlk.setReal(String(0, "lod%d_range", lod), srcRes->lods[lod].range);

          riBlk.saveToTextFile(String(0, "%s/%s.%08X.rendInst.blk", out_dir, res_name, grp.resTable[i].classId));
        }
        break;

      case DynModelGameResClassId:
        if (stub_drv_inited)
        {
          crd.seekto(ofs);
          Ptr<DynamicRenderableSceneLodsResource> srcRes =
            DynamicRenderableSceneLodsResource::loadResource(crd.getRawReader(), SRLOAD_NO_TEX_REF);

          DataBlock dmBlk;
          dmBlk.setInt("lods", srcRes->lods.size());
          dmBlk.setPoint3("bbox.0", srcRes->bbox[0]);
          dmBlk.setPoint3("bbox.1", srcRes->bbox[1]);
          if (srcRes->isBoundPackUsed())
          {
            dmBlk.setPoint4("bpC254", *(Point4 *)srcRes->bpC254);
            dmBlk.setPoint4("bpC255", *(Point4 *)srcRes->bpC255);
          }

          Tab<ShaderMaterial *> mat_list;
          srcRes->gatherUsedMat(mat_list);
          for (int m = 0; m < mat_list.size(); m++)
            dmBlk.addStr(String(0, "mat%02d", m), mat_list[m]->getInfo());

          for (int lod = 0; lod < srcRes->lods.size(); lod++)
            dmBlk.setReal(String(0, "lod%d_range", lod), srcRes->lods[lod].range);

          dmBlk.saveToTextFile(String(0, "%s/%s.%08X.dynModel.blk", out_dir, res_name, grp.resTable[i].classId));
        }
        break;
    }
  }
}

static void dump_grp_contents_refs(gamerespackbin::GrpData &grp, bool read_be)
{
  Tab<Tab<int> *> texIdPerAsset;
  texIdPerAsset.resize(grp.resData.size());
  mem_set_0(texIdPerAsset);
  FastNameMapEx texList;

  if (stub_drv_inited)
    gather_tex_refs(texIdPerAsset, texList, grp);

  for (int k = 0; k < grp.resData.size(); k++)
  {
    int id = grp.resData[k].resId, i = -1;
    if (read_be)
      id = BinDumpReader::le2be16(i);

    for (int j = 0; j < grp.resTable.size(); j++)
      if (grp.resTable[j].resId == id)
      {
        i = j;
        break;
      }
    if (i < 0)
      continue;

    const char *res_name = grp.getName(grp.resTable[i].resId);
    printf("%5d: \"%s\" %-*s cls=0x%08X  ofs=0x%08X (%9d) sz=%7d\n", i, res_name, max(50 - (int)strlen(res_name), 0), "",
      grp.resTable[i].classId, grp.resTable[i].offset, grp.resTable[i].offset,
      i + 1 < grp.resTable.size() ? grp.resTable[i + 1].offset - grp.resTable[i].offset : -1);

    for (int j = 0; j < grp.resData[k].refResIdCnt; j++)
    {
      id = grp.resData[k].refResIdPtr[j];
      if (read_be)
        id = BinDumpReader::le2be16(id);
      if (id != 0xFFFF)
        printf("       [%2d] %s\n", j, grp.getName(id));
    }
    if (texIdPerAsset[k])
      for (int j = 0; j < texIdPerAsset[k]->size(); j++)
        printf("       tex  %s\n", texList.getName(texIdPerAsset[k][0][j]));
  }
}

static void unpack_ddsx_data(Tab<char> &data, BinDumpReader &crd, const ddsx::Header &hdr, bool short_read = false)
{
  data.resize(short_read ? 32 : hdr.memSz);
  if (hdr.isCompressionZSTD())
  {
    ZstdLoadCB zcrd(crd.getRawReader(), hdr.packedSz);
    zcrd.read(data.data(), data_size(data));
    zcrd.ceaseReading();
  }
  else if (hdr.isCompressionOODLE())
  {
    OodleLoadCB zcrd(crd.getRawReader(), hdr.packedSz, hdr.memSz);
    zcrd.read(data.data(), data_size(data));
    zcrd.ceaseReading();
  }
  else if (hdr.isCompressionZLIB())
  {
    ZlibLoadCB zlib_crd(crd.getRawReader(), hdr.packedSz);
    zlib_crd.read(data.data(), data_size(data));
    zlib_crd.ceaseReading();
  }
  else if (hdr.isCompression7ZIP())
  {
    LzmaLoadCB lzma_crd(crd.getRawReader(), hdr.packedSz);
    lzma_crd.read(data.data(), data_size(data));
    lzma_crd.ceaseReading();
  }
  else
    crd.getRawReader().read(data.data(), data_size(data));
}

static void dump_dxp_contents(DxpBinData &dxp, BinDumpReader &crd, DataBlock &out_blk)
{
  Tab<char> tex_data(tmpmem);
  for (int i = 0; i < dxp.data->texNames.map.size(); i++)
  {
    TextureMetaData tmd;
    tmd.decodeData(dxp.data->texNames.map[i]);
    if (dxp.data->texHdr[i].flags & ddsx::Header::FLG_VOLTEX)
      printf("  [%2d] voltex  f=%08X %4dx%-4dx%d q=%d/%d/%d %s\n", i, dxp.data->texHdr[i].flags, dxp.data->texHdr[i].w,
        dxp.data->texHdr[i].h, dxp.data->texHdr[i].depth, dxp.data->texHdr[i].mQmip, dxp.data->texHdr[i].lQmip,
        dxp.data->texHdr[i].uQmip, dxp.data->texNames.map[i].get());
    else if (dxp.data->texHdr[i].flags & ddsx::Header::FLG_CUBTEX)
      printf("  [%2d] cubemap f=%08X %4dx%-4d q=%d/%d/%d  %s\n", i, dxp.data->texHdr[i].flags, dxp.data->texHdr[i].w,
        dxp.data->texHdr[i].h, dxp.data->texHdr[i].mQmip, dxp.data->texHdr[i].lQmip, dxp.data->texHdr[i].uQmip,
        dxp.data->texNames.map[i].get());
    else
      printf("  [%2d] f=%08X %4dx%-4d q=%d/%d/%d  %s\n", i, dxp.data->texHdr[i].flags, dxp.data->texHdr[i].w, dxp.data->texHdr[i].h,
        dxp.data->texHdr[i].mQmip, dxp.data->texHdr[i].lQmip, dxp.data->texHdr[i].uQmip, dxp.data->texNames.map[i].get());

    if (dxp.data->texHdr[i].flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX)
    {
      ddsx::Header hdr = dxp.data->texHdr[i];
      crd.seekto(dxp.data->texRec[i].ofs);
      unpack_ddsx_data(tex_data, crd, hdr, true);

      int blk_total = (hdr.w / 4) * (hdr.h / 4);
      int blk_empty = *(unsigned *)&tex_data[12];

      printf("       changes=%d/%d  %d%%  of base=%s\n", blk_total - blk_empty, blk_total, (blk_total - blk_empty) * 100 / blk_total,
        tmd.baseTexName.str());
    }
    out_blk.addStr("tex", dxp.data->texNames.map[i]);
  }
}
static void extract_dxp_contents(DxpBinData &dxp, BinDumpReader &crd, const char *out_dir, bool unpack)
{
  Tab<char> tex_data(tmpmem);
  dd_mkdir(out_dir);
  for (int i = 0; i < dxp.data->texNames.map.size(); i++)
  {
    printf("  [%2d] f=%08X %4dx%-4d %-60s  0x%08X, %d\n", i, dxp.data->texHdr[i].flags, dxp.data->texHdr[i].w, dxp.data->texHdr[i].h,
      dxp.data->texNames.map[i].get(), dxp.data->texRec[i].ofs, dxp.data->texRec[i].packedDataSize);

    TextureMetaData tmd;
    String out_fn(260, "%s/%s", out_dir, tmd.decode(dxp.data->texNames.map[i]));
    if (*out_fn.end(1) == '*')
      erase_items(out_fn, out_fn.size() - 2, 1);
    out_fn += ".ddsx";

    FullFileSaveCB cwr(out_fn);
    if (!cwr.fileHandle)
    {
      printf("  failed to write output DDSx: %s\n", out_fn.str());
      continue;
    }
    crd.seekto(dxp.data->texRec[i].ofs);
    if (!unpack)
    {
      cwr.write(&dxp.data->texHdr[i], sizeof(ddsx::Header));
      copy_stream_to_stream(crd.getRawReader(), cwr, dxp.data->texRec[i].packedDataSize);
    }
    else
    {
      ddsx::Header hdr = dxp.data->texHdr[i];
      unpack_ddsx_data(tex_data, crd, hdr);
      hdr.flags &= ~hdr.FLG_COMPR_MASK;
      cwr.write(&hdr, sizeof(ddsx::Header));
      cwr.write(tex_data.data(), data_size(tex_data));
    }
  }
}

static void extract_dxp_contents_dds(DxpBinData &dxp, BinDumpReader &crd, const char *out_dir)
{
  struct BitMaskFormat
  {
    uint32_t bitCount;
    uint32_t alphaMask;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t format;
    uint32_t flags;
  };
  static const BitMaskFormat bitMaskFormat[] = {
    {24, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_R8G8B8, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8R8G8B8, DDPF_RGB | DDPF_ALPHAPIXELS},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8R8G8B8, DDPF_RGB},
    {16, 0x00000000, 0x00f800, 0x07e0, 0x1f, D3DFMT_R5G6B5, DDPF_RGB},
    {16, 0x00008000, 0x007c00, 0x03e0, 0x1f, D3DFMT_A1R5G5B5, DDPF_RGB | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0x007c00, 0x03e0, 0x1f, D3DFMT_X1R5G5B5, DDPF_RGB},
    {16, 0x0000f000, 0x000f00, 0x00f0, 0x0f, D3DFMT_A4R4G4B4, DDPF_RGB | DDPF_ALPHAPIXELS},
    {8, 0x00000000, 0x0000e0, 0x001c, 0x03, D3DFMT_R3G3B2, DDPF_RGB},
    {8, 0x000000ff, 0x000000, 0x0000, 0x00, D3DFMT_A8, DDPF_ALPHA | DDPF_ALPHAPIXELS},
    {8, 0x00000000, 0x0000FF, 0x0000, 0x00, D3DFMT_L8, DDPF_LUMINANCE},
    {16, 0x0000ff00, 0x0000e0, 0x001c, 0x03, D3DFMT_A8R3G3B2, DDPF_RGB | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0x000f00, 0x00f0, 0x0f, D3DFMT_X4R4G4B4, DDPF_RGB},
    {32, 0xff000000, 0xff0000, 0xff00, 0xff, D3DFMT_A8B8G8R8, DDPF_RGB | DDPF_ALPHAPIXELS},
    {32, 0x00000000, 0xff0000, 0xff00, 0xff, D3DFMT_X8B8G8R8, DDPF_RGB},
    {16, 0x00000000, 0x0000ff, 0xff00, 0x00, D3DFMT_V8U8, DDPF_BUMPDUDV},
    {16, 0x0000ff00, 0x0000ff, 0x0000, 0x00, D3DFMT_A8L8, DDPF_LUMINANCE | DDPF_ALPHAPIXELS},
    {16, 0x00000000, 0xFFFF, 0x00000000, 0, D3DFMT_L16, DDPF_LUMINANCE},
  };

  Tab<char> tex_data(tmpmem);
  dd_mkdir(out_dir);
  for (int i = 0; i < dxp.data->texNames.map.size(); i++)
  {
    printf("  [%2d] f=%08X %4dx%-4d %-60s  0x%08X, %d\n", i, dxp.data->texHdr[i].flags, dxp.data->texHdr[i].w, dxp.data->texHdr[i].h,
      dxp.data->texNames.map[i].get(), dxp.data->texRec[i].ofs, dxp.data->texRec[i].packedDataSize);
    if (dxp.data->texHdr[i].flags & ddsx::Header::FLG_NEED_PAIRED_BASETEX)
      continue;

    TextureMetaData tmd;
    String out_fn(260, "%s/%s", out_dir, tmd.decode(dxp.data->texNames.map[i]));
    if (*out_fn.end(1) == '*')
      erase_items(out_fn, out_fn.size() - 2, 1);
    out_fn += ".dds";

    FullFileSaveCB cwr(out_fn);
    if (!cwr.fileHandle)
    {
      printf("  failed to write output DDSx: %s\n", out_fn.str());
      continue;
    }
    crd.seekto(dxp.data->texRec[i].ofs);

    ddsx::Header hdr = dxp.data->texHdr[i];
    unpack_ddsx_data(tex_data, crd, hdr);
    ddsx_forward_mips_inplace(hdr, tex_data.data(), data_size(tex_data));

    // dds header preparing
    DDSURFACEDESC2 targetHeader;
    memset(&targetHeader, 0, sizeof(DDSURFACEDESC2));
    targetHeader.dwSize = sizeof(DDSURFACEDESC2);
    targetHeader.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    targetHeader.dwHeight = hdr.h;
    targetHeader.dwWidth = hdr.w;
    targetHeader.dwDepth = hdr.depth;
    targetHeader.dwMipMapCount = hdr.levels;
    targetHeader.ddsCaps.dwCaps = 0;
    targetHeader.ddsCaps.dwCaps2 = 0;

    if (hdr.flags & ddsx::Header::FLG_CUBTEX)
      targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES;
    else if (hdr.flags & ddsx::Header::FLG_VOLTEX)
    {
      targetHeader.dwFlags |= DDSD_DEPTH;
      targetHeader.ddsCaps.dwCaps2 |= DDSCAPS2_VOLUME;
    }
    else
      targetHeader.ddsCaps.dwCaps |= DDSCAPS_TEXTURE;

    DDPIXELFORMAT &pf = targetHeader.ddpfPixelFormat;
    pf.dwSize = sizeof(DDPIXELFORMAT);

    // pixel format search
    int j;
    for (j = 0; j < sizeof(bitMaskFormat) / sizeof(BitMaskFormat); j++)
      if (hdr.d3dFormat == bitMaskFormat[j].format)
        break;

    if (j < sizeof(bitMaskFormat) / sizeof(BitMaskFormat))
    {
      pf.dwFlags = bitMaskFormat[j].flags;
      pf.dwRGBBitCount = bitMaskFormat[j].bitCount;
      pf.dwRBitMask = bitMaskFormat[j].redMask;
      pf.dwGBitMask = bitMaskFormat[j].greenMask;
      pf.dwBBitMask = bitMaskFormat[j].blueMask;
      pf.dwRGBAlphaBitMask = bitMaskFormat[j].alphaMask;
    }
    else
    {
      pf.dwFlags = DDPF_FOURCC;
      pf.dwFourCC = hdr.d3dFormat;
    }


    // dds output
    uint32_t FourCC = MAKEFOURCC('D', 'D', 'S', ' ');
    cwr.write(&FourCC, sizeof(FourCC));
    cwr.write(&targetHeader, sizeof(targetHeader));
    cwr.write(tex_data.data(), data_size(tex_data));
  }
}

static void saveOutBlk(DataBlock &out_blk, const char *v)
{
  if (v[0] == ':' && v[1])
    out_blk.saveToTextFile(v + 1);
  out_blk.reset();
}

static void print_header()
{
  printf("Dump GameRes structure v1.5\n"
         "Copyright (C) Gaijin Games KFT, 2023\n\n");
}


static void shutdown_handler()
{
  reset_game_resources();
  cpujobs::term(true);
}

static void init_stub_drv()
{
  if (stub_drv_inited)
    return;
  cpujobs::init();

  void *n = NULL;
  d3d::init_driver();
  d3d::init_video(NULL, NULL, NULL, 0, n, NULL, NULL, NULL, NULL);
  stub_drv_inited = true;
  ::dgs_post_shutdown_handler = shutdown_handler;

  set_default_tex_factory(get_symbolic_tex_factory());
  set_gameres_sys_ver(2);
  register_dynmodel_gameres_factory();
  register_rendinst_gameres_factory();
  register_effect_gameres_factory();
}

int DagorWinMain(bool debugmode)
{
  if (dgs_argc < 2)
  {
    print_header();
    printf("usage: dumpGrp-dev.exe <grp_binary|level_bin|tex_pack|res_list_blk> [commands...]\n\n"
           "where commands are:\n"
           "  -dump[:out.blk]\n"
           "  -dumpR\n"
           "  -dumpTexRef[:detailed]\n"
           "  -ext[:out.blk]\n"
           "  -loc\n"
           "  -unref[:out.blk]\n"
           "  -resolve:<packlist.blk>\n"
           "  -exp[:<DIR>]\n"
           "  -expU[:<DIR>]\n"
           "  -shaders:<shdump_bin_prefix>\n"
           "\nPC only:\n"
           "  -expDDS[:<DIR>]\n"
           "\n");
    return -1;
  }
  signal(SIGINT, ctrl_break_handler);

  FullFileLoadCB base_crd(dgs_argv[1]);
  if (!base_crd.fileHandle)
  {
    print_header();
    printf("ERR: can't open %s", dgs_argv[1]);
    return 13;
  }

  unsigned targetCode = _MAKE4C('PC');
  for (int i = 2; i < dgs_argc; i++)
    if (strnicmp(dgs_argv[i], "-target:", 8) == 0)
    {
      if (strcmp(dgs_argv[i] + 8, "PC") == 0)
        targetCode = _MAKE4C('PC');
      else if (strcmp(dgs_argv[i] + 8, "iOS") == 0)
        targetCode = _MAKE4C('iOS');
      else if (strcmp(dgs_argv[i] + 8, "and") == 0)
        targetCode = _MAKE4C('and');
      else
      {
        print_header();
        printf("ERR: unsupported target code: %s", dgs_argv[i] + 8);
        return 13;
      }
    }

  bool read_be = dagor_target_code_be(targetCode);
  BinDumpReader crd(&base_crd, targetCode, read_be);

  gamerespackbin::GrpData *grp = NULL;
  LevelBinData *lev = NULL;
  DxpBinData *dxp = NULL;
  DataBlock resList;

  if (trail_stricmp(dgs_argv[1], ".grp") || trail_stricmp(dgs_argv[1], ".grpcache.bin"))
    grp = load_grp(crd, df_length(base_crd.fileHandle));
  else if (trail_stricmp(dgs_argv[1], ".dxp.bin") || trail_stricmp(dgs_argv[1], ".dxp.bincache.bin"))
    dxp = load_tex_pack(crd);
  else if (trail_stricmp(dgs_argv[1], ".bin"))
    lev = load_level_bin(crd);
  else if (trail_stricmp(dgs_argv[1], ".blk"))
    resList.loadFromStream(base_crd);

  if (!grp && !lev && !dxp && !resList.paramCount())
  {
    print_header();
    printf("ERR: unrecognized file %s", dgs_argv[1]);
    return 13;
  }
  DataBlock out_blk;
  for (int i = 2; i < dgs_argc; i++)
    if (strnicmp(dgs_argv[i], "-ext", 4) == 0)
    {
      if (grp)
      {
        if (stub_drv_inited)
          gameresprivate::scanGameResPack(dgs_argv[1]);
        dump_grp_external(*grp, crd.readBE(), out_blk);
      }
      else if (lev)
        dump_lev_external(*lev, crd.readBE(), out_blk);
      else
        printf("irrelevant command: %s", dgs_argv[i]);

      saveOutBlk(out_blk, dgs_argv[i] + 4);
    }
    else if (strnicmp(dgs_argv[i], "-resolve:", 9) == 0)
    {
      DataBlock lst;
      if (trail_stricmp(dgs_argv[i] + 9, ".vromfs.bin"))
      {
        VirtualRomFsData *vrom = load_vromfs_dump(dgs_argv[i] + 9, tmpmem);
        if (vrom)
        {
          ::add_vromfs(vrom);
          lst.load("res/resPacks.blk");
          ::remove_vromfs(vrom);
          tmpmem->free(vrom);
        }
        if (!vrom || !lst.isValid())
        {
          printf("ERR: cannot load list blk res/resPacks.blk from %s", dgs_argv[i] + 9);
          return 13;
        }
      }
      else if (!lst.load(dgs_argv[i] + 9))
      {
        printf("ERR: cannot load list blk %s", dgs_argv[i] + 9);
        return 13;
      }
      if (grp)
      {
        dump_grp_external(*grp, crd.readBE(), resList);
        dump_blk_external_resolve(resList, crd, lst);
      }
      else if (lev)
      {
        dump_lev_external(*lev, crd.readBE(), resList);
        dump_blk_external_resolve(resList, crd, lst);
      }
      else if (resList.paramCount())
      {
        dump_blk_external_resolve(resList, crd, lst);
      }
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (stricmp(dgs_argv[i], "-loc") == 0)
    {
      if (grp)
        dump_grp_local(*grp, crd.readBE());
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (strnicmp(dgs_argv[i], "-unref", 6) == 0)
    {
      if (grp)
        dump_grp_unref(*grp, crd.readBE(), out_blk);
      else
        printf("irrelevant command: %s", dgs_argv[i]);

      saveOutBlk(out_blk, dgs_argv[i] + 6);
    }
    else if (stricmp(dgs_argv[i], "-dumpR") == 0)
    {
      if (grp)
      {
        if (stub_drv_inited)
          gameresprivate::scanGameResPack(dgs_argv[1]);
        dump_grp_contents_refs(*grp, crd.readBE());
      }
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (stricmp(dgs_argv[i], "-dumpTexRef") == 0)
    {
      if (grp || crd.readBE())
      {
        if (stub_drv_inited)
        {
          gameres_final_optimize_desc(gameres_rendinst_desc, "riDesc");
          gameres_final_optimize_desc(gameres_dynmodel_desc, "dynModelDesc");
          dump_grp_tex_refs(dgs_argv[1], false);
        }
        else
          printf("-shaders:XXX must be specified for %s", dgs_argv[i]);
      }
      else
        printf("irrelevant command: %s, BE=%d", dgs_argv[i], crd.readBE());
    }
    else if (stricmp(dgs_argv[i], "-dumpTexRef:detailed") == 0)
    {
      if (grp || crd.readBE())
      {
        if (stub_drv_inited)
          dump_grp_tex_refs(dgs_argv[1], true);
        else
          printf("-shaders:XXX must be specified for %s", dgs_argv[i]);
      }
      else
        printf("irrelevant command: %s, BE=%d", dgs_argv[i], crd.readBE());
    }
    else if (strnicmp(dgs_argv[i], "-dump", 5) == 0 && (dgs_argv[i][5] == '\0' || dgs_argv[i][5] == ':'))
    {
      if (grp)
      {
        if (stub_drv_inited)
          gameresprivate::scanGameResPack(dgs_argv[1]);
        dump_grp_contents(*grp, crd.readBE(), out_blk);
      }
      else if (dxp)
        dump_dxp_contents(*dxp, crd, out_blk);
      else
        printf("irrelevant command: %s", dgs_argv[i]);

      saveOutBlk(out_blk, dgs_argv[i] + 5);
    }
    else if (strnicmp(dgs_argv[i], "-expDDS", 7) == 0)
    {
      if (dxp && targetCode == _MAKE4C('PC'))
        extract_dxp_contents_dds(*dxp, crd, dgs_argv[i][7] == ':' ? dgs_argv[i] + 8 : ".");
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (strnicmp(dgs_argv[i], "-expU", 5) == 0)
    {
      if (dxp)
        extract_dxp_contents(*dxp, crd, dgs_argv[i][5] == ':' ? dgs_argv[i] + 6 : ".", true);
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (strnicmp(dgs_argv[i], "-exp", 4) == 0)
    {
      if (dxp)
        extract_dxp_contents(*dxp, crd, dgs_argv[i][4] == ':' ? dgs_argv[i] + 5 : ".", false);
      else if (grp)
        extract_grp_contents(*grp, crd, dgs_argv[i][4] == ':' ? dgs_argv[i] + 5 : ".", df_length(base_crd.fileHandle));
      else
        printf("irrelevant command: %s", dgs_argv[i]);
    }
    else if (strnicmp(dgs_argv[i], "-target:", 8) == 0)
      ; // skip
    else if (strnicmp(dgs_argv[i], "-shaders:", 9) == 0)
    {
      init_stub_drv();
      startup_shaders(dgs_argv[i] + 9);
      startup_game(RESTART_ALL);
    }
    else if (strnicmp(dgs_argv[i], "-riDesc:", 8) == 0)
      gameres_append_desc(gameres_rendinst_desc, dgs_argv[i] + 8, dgs_argv[i] + 8, true);
    else if (strnicmp(dgs_argv[i], "-dynModelDesc:", 14) == 0)
      gameres_append_desc(gameres_dynmodel_desc, dgs_argv[i] + 14, dgs_argv[i] + 14, true);
    else
    {
      print_header();
      printf("ERR: invalid switch %s", dgs_argv[i]);
      return 13;
    }

  if (grp)
    memfree(grp, tmpmem);
  del_it(lev);
  del_it(dxp);
  gameres_rendinst_desc.reset();
  gameres_dynmodel_desc.reset();
  return 0;
}
