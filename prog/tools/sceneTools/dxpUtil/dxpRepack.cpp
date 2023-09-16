#include <libTools/dtx/ddsxPlugin.h>
#include <libTools/util/conLogWriter.h>
#include <libTools/util/progressInd.h>
#include <libTools/util/strUtil.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/binDumpUtil.h>
#include <libTools/util/binDumpReader.h>
#include <3d/ddsxTex.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_stdint.h>
#include <util/dag_string.h>
#include <util/dag_roNameMap.h>
#include <util/dag_texMetaData.h>
#include <debug/dag_debug.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>

#define DO_UNPACK_DDSX 1
#define DO_REMOVE_MIPS 1

class DDSxTexPack2Serv
{
  struct Rec
  {
    int ofs, tex, texId, packedDataSize;
  };

public:
  RoNameMap texNames;
  PatchableTab<ddsx::Header> hdr;
  PatchableTab<Rec> rec;
  IGenLoad *crd;

public:
  static DDSxTexPack2Serv *make(BinDumpReader &crd)
  {
    if (crd.readFourCC() != _MAKE4C('DxP2'))
      return NULL;
    if (crd.readInt32e() != 1)
      return NULL;
    int tex_num = crd.readInt32e();

    crd.beginBlock();
    DDSxTexPack2Serv *pack = (DDSxTexPack2Serv *)memalloc(crd.getBlockRest(), tmpmem);
    crd.getRawReader().read(pack, crd.getBlockRest());
    crd.endBlock();

    crd.convert32(pack, sizeof(*pack));
    pack->texNames.map.patch(pack);
    crd.convert32(pack->texNames.map.data(), data_size(pack->texNames.map));
    pack->texNames.map.patch((void *)-(int)pack);
    pack->texNames.patchData(pack);

    pack->hdr.patch(pack);
    pack->rec.patch(pack);
    crd.convert32(pack->rec.data(), data_size(pack->rec));

    pack->crd = &crd.getRawReader();
    return pack;
  }

  int getTexRecId(const char *name) const { return texNames.getNameId(name); }

  bool getDDSx(const char *name, ddsx::Buffer &b)
  {
    int id = texNames.getNameId(name);
    if (id < 0)
      return false;
    if (rec[id].packedDataSize < 0)
      return false;

    if (!DO_UNPACK_DDSX || !hdr[id].compressionType())
    {
      b.len = rec[id].packedDataSize + sizeof(ddsx::Header);
      b.ptr = memalloc(b.len, tmpmem);
      memcpy(b.ptr, &hdr[id], sizeof(ddsx::Header));

      crd->seekto(rec[id].ofs);
      crd->read((char *)b.ptr + sizeof(ddsx::Header), rec[id].packedDataSize);
    }
    else
    {
      int sz = hdr[id].memSz;
      int lev = hdr[id].levels;

      if (DO_REMOVE_MIPS)
      {
        sz = hdr[id].getSurfaceSz(0);
        lev = 1;
      }

      b.len = sz + sizeof(ddsx::Header);
      b.ptr = memalloc(b.len, tmpmem);
      memcpy(b.ptr, &hdr[id], sizeof(ddsx::Header));
      ((ddsx::Header *)b.ptr)->flags &= ~ddsx::Header::FLG_COMPR_MASK;
      ((ddsx::Header *)b.ptr)->levels = lev;
      ((ddsx::Header *)b.ptr)->memSz = sz;
      ((ddsx::Header *)b.ptr)->packedSz = sz;

      crd->seekto(rec[id].ofs);
      ZlibLoadCB zcrd(*crd, hdr[id].packedSz);
      zcrd.read((char *)b.ptr + sizeof(ddsx::Header), sz);
      zcrd.ceaseReading();
    }

    return true;
  }
  void removeDDSx(const char *name)
  {
    int namelen = strlen(name);
    for (int i = 0; i < texNames.map.size(); i++)
      if (strnicmp(texNames.map[i], name, namelen) == 0 && texNames.map[i][namelen] == '*')
      {
        debug("removed changed %s: %s", name, texNames.map[i]);
        rec[i].packedDataSize = -1;
        return;
      }
  }
  bool hasDDSx(const char *name)
  {
    int namelen = strlen(name);
    for (int i = 0; i < texNames.map.size(); i++)
      if (strnicmp(texNames.map[i], name, namelen) == 0 && texNames.map[i][namelen] == '*')
        return true;
    return false;
  }
};

class Pack2TexGatherHelper
{
  Tab<String> texPath;
  struct PackRec
  {
    int ofs, tex, texId, packedDataSize;
  };

public:
  mkbindump::StrCollector texName;

public:
  Pack2TexGatherHelper() : texPath(tmpmem) {}

  bool saveTextures(DDSxTexPack2Serv &prev_pack, mkbindump::BinDumpSaveCB &cwr, const char *pack_fname)
  {
    SmallTab<char, TmpmemAlloc> buf;
    SmallTab<ddsx::Buffer, TmpmemAlloc> ddsx_buf;
    SmallTab<PackRec, TmpmemAlloc> rec;
    SmallTab<int, TmpmemAlloc> ordmap;

    clear_and_resize(ordmap, texName.strCount());
    mem_set_ff(ordmap);
    clear_and_resize(ddsx_buf, texName.strCount());
    mem_set_0(ddsx_buf);
    clear_and_resize(rec, texName.strCount());

    int ord_idx = 0;
    iterate_names_in_lexical_order(texName.getMap(), [&](int id, const char *name) {
      ordmap[id] = ord_idx++;
      const char *tex_path = texPath[id];
      ddsx::Buffer &b = ddsx_buf[ordmap[id]];

      if (!prev_pack.getDDSx(name, b))
        printf("ERR: %s NOT reused from prev pack\n", name);
    });

    mkbindump::RoNameMapBuilder b;
    mkbindump::PatchTabRef pt_ddsx, pt_rec;

    b.prepare(texName);

    cwr.writeFourCC(_MAKE4C('DxP2'));
    cwr.writeInt32e(1);
    cwr.writeInt32e(texName.strCount());

    cwr.beginBlock();
    cwr.setOrigin();

    b.writeHdr(cwr);
    pt_ddsx.reserveTab(cwr);
    pt_rec.reserveTab(cwr);
    cwr.writeInt32e(0);
    cwr.writeInt32e(0);

    texName.writeStrings(cwr);
    cwr.align8();
    b.writeMap(cwr);

    cwr.align32();
    pt_ddsx.startData(cwr, ddsx_buf.size());
    for (int i = 0; i < ddsx_buf.size(); i++)
      cwr.writeRaw(ddsx_buf[i].ptr, sizeof(ddsx::Header));
    pt_ddsx.finishTab(cwr);

    pt_rec.reserveData(cwr, ddsx_buf.size(), sizeof(PackRec));
    pt_rec.finishTab(cwr);
    cwr.popOrigin();
    cwr.endBlock();

    for (int j = 0; j < ordmap.size(); j++)
    {
      int i = ordmap[j];
      cwr.align8();
      rec[i].ofs = cwr.tell();
      rec[i].texId = -1;
      rec[i].tex = NULL;
      rec[i].packedDataSize = ddsx_buf[i].len - sizeof(ddsx::Header);
      cwr.writeRaw((char *)ddsx_buf[i].ptr + sizeof(ddsx::Header), rec[i].packedDataSize);
      ddsx_buf[i].free();
    }

    int wrpos = cwr.tell();
    cwr.seekto(pt_rec.resvDataPos + 16);
    cwr.writeTabData32ex(rec);
    cwr.seekto(wrpos);

    return true;
  }

  bool addTextures(const RoNameMap &texNames)
  {
    for (int i = 0; i < texNames.map.size(); i++)
      texName.addString(texNames.map[i]);
    return true;
  }
};


bool rebuildDdsxTexPack(mkbindump::BinDumpSaveCB &cwr, const char *pack_fname)
{
  debug_ctx("%X %d", cwr.getTarget(), cwr.WRITE_BE);
  FullFileLoadCB prev_fcrd(pack_fname);
  BinDumpReader prev_crd(&prev_fcrd, cwr.getTarget(), cwr.WRITE_BE);
  if (!prev_fcrd.fileHandle || df_length(prev_fcrd.fileHandle) < 16)
    return false;

  DDSxTexPack2Serv *prev_pack = DDSxTexPack2Serv::make(prev_crd);
  if (!prev_pack)
  {
    printf("ERR: failed to read\n");
    return false;
  }

  Pack2TexGatherHelper trh;
  trh.addTextures(prev_pack->texNames);

  // write textures (each in own block)
  if (!trh.saveTextures(*prev_pack, cwr, pack_fname))
  {
    del_it(prev_pack);
    return false;
  }

  ::dd_mkpath(pack_fname);
  FullFileSaveCB fcwr(String(260, "%s.unz", pack_fname));
  if (!fcwr.fileHandle)
  {
    printf("ERR: failed to write\n");
    del_it(prev_pack);
    return false;
  }
  cwr.copyDataTo(fcwr);
  fcwr.close();
  del_it(prev_pack);
  return true;
}

#define __UNLIMITED_BASE_PATH     1
#define __SUPPRESS_BLK_VALIDATION 1
#include <startup/dag_mainCon.inc.cpp>
#undef ERROR

int DagorWinMain(bool debugmode)
{
  if (__argc < 2)
    return 1;
  printf("rebuild %s", __argv[1]);
  mkbindump::BinDumpSaveCB cwr(8 << 20, _MAKE4C('PC'), false);
  rebuildDdsxTexPack(cwr, __argv[1]);
  return 0;
}
