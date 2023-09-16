#pragma once

#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_ioUtils.h>
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zstdIo.h>
#include <generic/dag_tab.h>
#include <generic/dag_sort.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_vromfs.h>
#include <util/dag_simpleString.h>
#include <util/dag_fastIntList.h>
#include <gameRes/grpData.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <util/dag_strUtil.h>
#include <libTools/util/strUtil.h>
#include <stddef.h> // offsetof
#include <supp/dag_zstdObfuscate.h>

class PackListMgr
{
public:
  struct PackOptions
  {
    bool perFileOptRes = false, perFileOptTex = false, patchMode = false;
  };

  PackListMgr() : list(midmem) {}

  bool loadVromfs(const char *fn_vromfs, const char *fn, unsigned targetCode)
  {
    list.clear();
    VirtualRomFsData *fs = read_vromfs(fn_vromfs, targetCode);
    if (!fs)
      return false;
    String tmpStr(fn);
    simplify_fname(tmpStr);
    strlwr(tmpStr);
    int id = fs->files.getNameId(tmpStr);
    if (id >= 0)
    {
      InPlaceMemLoadCB crd(fs->data[id].data(), data_size(fs->data[id]));
      DataBlock blk;
      if (!blk.loadFromStream(crd))
        goto err;
      memfree(fs, tmpmem);

      loadFrom(blk);
      return true;
    }

  err:
    memfree(fs, tmpmem);
    return false;
  }
  bool load(const char *fn)
  {
    list.clear();

    DataBlock blk;
    if (!dd_file_exist(fn))
      return false;
    if (!blk.load(fn))
      return false;
    loadFrom(blk);
    return true;
  }
  void loadFrom(const DataBlock &blk)
  {
    const DataBlock *b;
    int nid = blk.getNameId("pack");
    b = blk.getBlockByName("gameResPacks");
    if (b)
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
          registerGoodPack(b->getStr(i), true, NULL);

    b = blk.getBlockByName("ddsxTexPacks");
    if (b)
      for (int i = 0; i < b->paramCount(); i++)
        if (b->getParamType(i) == DataBlock::TYPE_STRING && b->getParamNameId(i) == nid)
          registerGoodPack(b->getStr(i), false, NULL);
  }
  void sort() { ::sort(list, &filename_cmp); }
  void save(const char *fn, const PackOptions &opt)
  {
    DataBlock blk;
    saveTo(blk, opt);
    blk.saveToTextFile(fn);
  }
  void saveTo(DataBlock &blk, const PackOptions &opt)
  {
    DataBlock *b;

    Tab<const char *> list_fn;
    list_fn.reserve(list.size());
    for (auto &e : list)
      list_fn.push_back(e.marked ? e.fn.c_str() : nullptr);

    b = blk.addNewBlock("gameResPacks");
    for (int i = 0; i < list.size(); i++)
      if (list[i].grp && list_fn[i])
      {
        b->addStr("pack", list_fn[i]);
        list_fn[i] = nullptr;
      }

    b = blk.addNewBlock("ddsxTexPacks");
    for (int i = 0; i < list.size(); i++)
      if (!list[i].grp && list_fn[i] && strstr(list_fn[i], "_tq_pack"))
      {
        b->addStr("pack", list_fn[i]);
        list_fn[i] = nullptr;
      }
    for (int i = 0; i < list.size(); i++)
      if (!list[i].grp && list_fn[i] && !trail_strcmp(list_fn[i], "-hq.dxp.bin"))
      {
        b->addStr("pack", list_fn[i]);
        list_fn[i] = nullptr;
      }
    for (int i = 0; i < list.size(); i++)
      if (!list[i].grp && list_fn[i])
      {
        b->addStr("pack", list_fn[i]);
        list_fn[i] = nullptr;
      }

    b = blk.addNewBlock("hash_md5");
    unsigned char hash[AssetExportCache::HASH_SZ];
    String stor;
    AssetExportCache gdc;
    DagorAssetMgr amgr;
    for (int i = 0; i < list.size(); i++)
      if (list[i].marked)
        if (gdc.load(list[i].cacheFn, amgr) && gdc.getTargetFileHash(hash))
          b->addStr(list[i].fn, data_to_str_hex(stor, hash, sizeof(hash)));
    if (opt.perFileOptRes)
      blk.setBool("perFileOptRes", opt.perFileOptRes);
    if (opt.perFileOptTex)
      blk.setBool("perFileOptTex", opt.perFileOptTex);
    if (opt.patchMode)
      blk.setBool("patchMode", opt.patchMode);
  }

  void writeHdrCacheVromfs(const char *dest_base, const char *game_base, const char *fname, const char *fn_reslist,
    unsigned targetCode, bool writeGrpHdrCache, const PackOptions &opt)
  {
    if (!list.size())
    {
      dd_erase(fname);
      return;
    }

    bool write_be = dagor_target_code_be(targetCode);
    FastNameMap names;
    FastIntList grpNids;
    String tmpStr;
    mkbindump::BinDumpSaveCB cwr(8 << 20, targetCode, write_be);
    dd_erase(fname);

    mkbindump::PatchTabRef pt_names, pt_data;
    pt_names.reserveTab(cwr);
    pt_data.reserveTab(cwr);

    for (int i = 0; i < list.size(); i++)
      if (list[i].marked && writeGrpHdrCache)
      {
        const char *p = list[i].fn;
        if (strchr("/\\", *p))
          p++;
        tmpStr = p;
        simplify_fname(tmpStr);
        strlwr(tmpStr);
        int id = names.addNameId(tmpStr);
        G_ASSERT(id == names.nameCount() - 1);
        if (list[i].grp)
          grpNids.addInt(id);
      }
    int reslist_id = names.addNameId(fn_reslist);
    pt_names.reserveData(cwr, names.nameCount(), cwr.PTR_SZ);

    Tab<int> sorted_ids;
    gather_ids_in_lexical_order(sorted_ids, names);

    cwr.align16();
    int i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *name) {
      cwr.writeInt32eAt(cwr.tell(), pt_names.resvDataPos + i * cwr.PTR_SZ);
      if (id != reslist_id)
        tmpStr.printf(260, "%s/%scache.bin", game_base, name);
      else
        tmpStr.printf(260, "%s/%s", game_base, name);
      simplify_fname(tmpStr);
      strlwr(tmpStr);
      cwr.writeRaw(tmpStr, (int)strlen(tmpStr) + 1);
      i++;
    });

    cwr.align16();
    pt_data.reserveData(cwr, names.nameCount(), cwr.TAB_SZ);
    i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *) {
      if (id == reslist_id)
      {
        DataBlock blk;
        saveTo(blk, opt);

        cwr.align16();
        int p_start = cwr.tell();
        blk.saveToStream(cwr.getRawWriter());
        int p_end = cwr.tell();
        cwr.writeInt32eAt(p_start, pt_data.resvDataPos + i * cwr.TAB_SZ);
        cwr.writeInt32eAt(p_end - p_start, pt_data.resvDataPos + i * cwr.TAB_SZ + 4);
      }
      i++;
    });

    bool failed = false;
    i = 0;
    iterate_names_in_order(names, sorted_ids, [&](int id, const char *name) {
      if (id == reslist_id)
      {
        i++;
        return;
      }
      cwr.align16();
      int p_start = cwr.tell();
      if (grpNids.hasInt(id))
      {
        if (!writeGrpHdr(cwr, String(260, "%s%s", dest_base, name)))
          failed = true;
      }
      else
      {
        if (!writeDxpHdr(cwr, String(260, "%s%s", dest_base, name)))
          failed = true;
      }
      int p_end = cwr.tell();
      cwr.writeInt32eAt(p_start, pt_data.resvDataPos + i * cwr.TAB_SZ);
      cwr.writeInt32eAt(p_end - p_start, pt_data.resvDataPos + i * cwr.TAB_SZ + 4);
      i++;
    });
    if (failed)
      return;
    pt_names.finishTab(cwr);
    pt_data.finishTab(cwr);

    // finally, write file on disk
    FullFileSaveCB fcwr(fname);
    if (!fcwr.fileHandle)
      return;

    fcwr.writeInt(_MAKE4C('VRFs'));
    fcwr.writeInt(targetCode);
    fcwr.writeInt(mkbindump::le2be32_cond(cwr.getSize(), write_be));

    MemoryLoadCB crd(cwr.getMem(), false);
    DynamicMemGeneralSaveCB mcwr(tmpmem, cwr.getSize() + 4096);

    int packed_sz = zstd_compress_data(mcwr, crd, cwr.getSize(), 16 << 10, 11);
    OBFUSCATE_ZSTD_DATA(mcwr.data(), packed_sz);

    unsigned hw32 = packed_sz | 0x40000000;
    fcwr.writeInt(mkbindump::le2be32_cond(hw32, write_be));
    fcwr.write(mcwr.data(), packed_sz);
  }

  void invalidate(bool grp, bool tex)
  {
    for (int i = 0; i < list.size(); i++)
      if ((grp && list[i].grp) || (tex && !list[i].grp))
        list[i].marked = false;
  }

  inline void registerBadPack(const char *fn, bool grp, const char *cache_fn) { registerPack(fn, grp, false, cache_fn); }
  inline void registerGoodPack(const char *fn, bool grp, const char *cache_fn) { registerPack(fn, grp, true, cache_fn); }

public:
  struct Rec
  {
    SimpleString fn, cacheFn;
    bool grp, marked;
  };
  Tab<Rec> list;

protected:
  void registerPack(const char *fn, bool grp, bool good, const char *cache_fn)
  {
    for (int i = 0; i < list.size(); i++)
      if (grp == list[i].grp && stricmp(fn, list[i].fn) == 0)
      {
        list[i].marked = good;
        if (cache_fn)
          list[i].cacheFn = cache_fn;
        return;
      }
    Rec &r = list.push_back();
    r.fn = fn;
    r.cacheFn = cache_fn;
    r.grp = grp;
    r.marked = good;
  }

  bool writeGrpHdr(mkbindump::BinDumpSaveCB &cwr, const char *grp_fname)
  {
    FullFileLoadCB crd(grp_fname);
    if (!crd.fileHandle)
      return false;

    bool read_be = cwr.WRITE_BE;

    gamerespackbin::GrpHeader ghdr;
    crd.read(&ghdr, sizeof(ghdr));
    if (ghdr.label != _MAKE4C('GRP2') && ghdr.label != _MAKE4C('GRP3'))
      return false;

    int restSz = mkbindump::le2be32_cond(ghdr.restFileSize, read_be);
    int fullDataSize = mkbindump::le2be32_cond(ghdr.fullDataSize, read_be);
    if (restSz + sizeof(ghdr) != df_length(crd.fileHandle))
      return false;

    cwr.writeRaw(&ghdr, sizeof(ghdr));
    copy_stream_to_stream(crd, cwr.getRawWriter(), fullDataSize);
    return true;
  }

  bool writeDxpHdr(mkbindump::BinDumpSaveCB &cwr, const char *grp_fname)
  {
    FullFileLoadCB crd(grp_fname);
    if (!crd.fileHandle)
      return false;

    bool read_be = cwr.WRITE_BE;
    unsigned hdr[4];
    crd.read(hdr, sizeof(hdr));
    cwr.writeRaw(hdr, sizeof(hdr));
    int restSz = mkbindump::le2be32_cond(hdr[3], read_be);
    copy_stream_to_stream(crd, cwr.getRawWriter(), restSz);
    return true;
  }

#define FS_OFFS offsetof(VirtualRomFsData, files)

  static VirtualRomFsData *read_vromfs(const char *fname, unsigned targetCode)
  {
    VirtualRomFsDataHdr hdr;
    VirtualRomFsData *fs = NULL;
    void *base = NULL;
    bool read_be = false;

    {
      FullFileLoadCB crd(fname);
      if (!crd.fileHandle)
      {
        debug("ERR: can't open %s", fname);
        return NULL;
      }
      crd.read(&hdr, sizeof(hdr));
      if (hdr.label != _MAKE4C('VRFs'))
      {
        debug("ERR: VRFS label not found in %s", fname);
        return NULL;
      }
      if (targetCode && hdr.target != targetCode)
      {
        debug("ERR: mismatched format %c%c%c%c!=%c%c%c%c, %s", _DUMP4C(hdr.target), _DUMP4C(targetCode), fname);
        return NULL;
      }
      if (!dagor_target_code_valid(hdr.target))
      {
        debug("ERR: unknowm format %c%c%c%c, %s", _DUMP4C(hdr.target), fname);
        return NULL;
      }

      read_be = dagor_target_code_be(hdr.target);
      if (read_be)
      {
        hdr.fullSz = mkbindump::le2be32(hdr.fullSz);
        hdr.hw32 = mkbindump::le2be32(hdr.hw32);
      }

#define FS_OFFS offsetof(VirtualRomFsData, files)
      fs = (VirtualRomFsData *)memalloc(FS_OFFS + hdr.fullSz, tmpmem);
      base = (char *)fs + FS_OFFS;
      if (hdr.packedSz() && hdr.zstdPacked())
      {
        Tab<char> src;
        src.resize(hdr.packedSz());
        crd.read(src.data(), data_size(src));
        DEOBFUSCATE_ZSTD_DATA(src.data(), data_size(src));
        size_t dsz = zstd_decompress(base, hdr.fullSz, src.data(), data_size(src));
        if (dsz != hdr.fullSz)
        {
          debug("ERR: failed to decode data, ret=%d (decSz=%d), %s", dsz, hdr.fullSz, fname);
          return NULL;
        }
      }
      else
      {
        G_ASSERT(!hdr.hw32); // assume vromfs is not compressed
        crd.read(base, hdr.fullSz);
      }
    }

    if (!read_be)
    {
      fs->files.patchData(base);
      fs->data.patch(base);
    }
    else
    {
      mkbindump::le2be32_s(base, base, (sizeof(VirtualRomFsData) - FS_OFFS) / 4);
      fs->files.map.patch(base);
      fs->data.patch(base);

      mkbindump::le2be32_s(fs->files.map.data(), fs->files.map.data(), data_size(fs->files.map) / 4);
      mkbindump::le2be32_s(fs->data.data(), fs->data.data(), data_size(fs->data) / 4);
      for (int i = fs->files.map.size() - 1; i >= 0; i--)
        fs->files.map[i].patch(base);
    }
    for (int i = fs->data.size() - 1; i >= 0; i--)
      fs->data[i].patch(base);

    return fs;
  }

  static int filename_cmp(const Rec *a, const Rec *b) { return strcmp(a->fn, b->fn); }
};
