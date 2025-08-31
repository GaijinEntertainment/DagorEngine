// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMeshTexLoadCtrl.h>
#include <3d/fileTexFactory.h>
#include <drv/3d/dag_info.h>
#include "scriptSMat.h"
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_lzmaIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_readToUncached.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_btagCompr.h>
#include <osApiWrappers/dag_files.h>
#include <util/fnameMap.h>
#include <osApiWrappers/dag_basePath.h>
#include <perfMon/dag_perfTimer.h>
// #include <debug/dag_debug.h>
#include <supp/dag_alloca.h>

namespace matvdata
{
void (*hook_on_add_vdata)(ShaderMatVdata *vd, int add_vdata_sz) = nullptr;
}

ShaderMatVdata::ModelLoadStats ShaderMatVdata::modelLoadStats[3];

#define MAX_STACK_SIZE         512
#define VDATA_RELOAD_SUPPORTED _TARGET_PC_WIN | _TARGET_PC_LINUX | _TARGET_ANDROID

struct ShaderMatVdata::MatVdataHdr
{
  struct VdataHdr
  {
    uint32_t vertNum, vertStride : 8, packedIdxSizeLo : 24, idxSize : 28, packedIdxSizeHi : 4, flags;
    PatchableTab<CompiledShaderChannelId> vDecl;

    unsigned packedIdxSize() const { return (flags & VDATA_PACKED_IB) ? (packedIdxSizeLo | (packedIdxSizeHi << 24)) : idxSize; }
  };

  PatchableTab<const ShaderMaterialProperties> mat;
  PatchableTab<VdataHdr> vdata;
};

struct ShaderMatVdata::TexStrHdr
{
  PatchableTab<int32_t> names; //< actually, name offsets in dump (not names itself)
};

void ShaderMatVdata::loadTexStr(IGenLoad &crd, bool sym_tex, const char *base_path)
{
  int sz = crd.readInt();
  TexStrHdr *hdr = (TexStrHdr *)(sz > MAX_STACK_SIZE ? memalloc(sz, tmpmem) : alloca(sz));

  // read and patch headers
  crd.read(hdr, sz);

  hdr->names.patch(hdr);
  G_ASSERT(hdr->names.size() == tex.size());

  String tmp_storage;
  for (int i = 0; i < hdr->names.size(); i++)
  {
    const char *name = ((char *)hdr) + hdr->names[i];

    if (name[0] == '<' || name[0] == '>')
    {
      logwarn("support for special marker \'%c\' in \"%s\" dropped, ignored", name[0], name);
      name++;
    }
    name = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(name, tmp_storage);
    if (!name)
    {
      tex[i] = D3DRESID(D3DRESID::INVALID_ID2);
      continue;
    }

    if (sym_tex)
    {
      tex[i] = get_managed_texture_id(name);
      if (tex[i] == BAD_TEXTUREID)
        tex[i] = add_managed_texture(name, &SymbolicTextureFactory::self);
    }
    else if (name[0] == '.' && name[1] == '/')
    {
      static String tmp;
      tmp.printf(0, "%s%s", base_path, &name[2]);
      tex[i] = get_managed_texture_id(tmp);
      if (tex[i] == BAD_TEXTUREID)
        tex[i] = add_managed_texture(tmp, NULL);
    }
    else
    {
      tex[i] = get_managed_texture_id(name);
      if (tex[i] == BAD_TEXTUREID)
        tex[i] = add_managed_texture(name, NULL);
    }
  }

  // free headers
  if (sz > MAX_STACK_SIZE)
    memfree(hdr, tmpmem);
}

void ShaderMatVdata::loadTexIdx(IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap)
{
  crd.readTabData(tex);
  for (int i = 0; i < tex.size(); i++)
    if ((int)unsigned(tex[i]) != -1)
    {
      G_ASSERT(unsigned(tex[i]) < texMap.size());
      tex[i] = texMap[unsigned(tex[i])];
    }
}

void ShaderMatVdata::makeTexAndMat(const DataBlock &texBlk, const DataBlock &matBlk)
{
  G_ASSERTF_RETURN(texBlk.paramCount() == tex.size(), , "texBlk.paramCount=%d tex.count=%d", texBlk.paramCount(), tex.size());
  G_ASSERTF_RETURN(matBlk.blockCount() == mat.size(), , "matBlk.blockCount=%d mat.count=%d", matBlk.blockCount(), mat.size());

  String tmp_storage;
  for (int i = 0; i < tex.size(); i++)
  {
    const char *name = texBlk.getStr(i);
    if (name[0] == '<' || name[0] == '>')
    {
      logwarn("support for special marker \'%c\' in \"%s\" dropped, ignored", name[0], name);
      name++;
    }
    name = IShaderMatVdataTexLoadCtrl::preprocess_tex_name(name, tmp_storage);
    if (!name)
    {
      tex[i] = D3DRESID(D3DRESID::INVALID_ID2);
      continue;
    }
    tex[i] = get_managed_texture_id(name);
    if (tex[i] == BAD_TEXTUREID)
      tex[i] = add_managed_texture(name, NULL);
  }

  Ptr<MaterialData> m = new MaterialData;
  for (int i = 0; i < mat.size(); i++)
  {
    const DataBlock &bMat = *matBlk.getBlock(i);
    m->className = bMat.getStr("cls");
    m->matScript = bMat.getStr("par");
    m->mat.diff.set_xyzw(bMat.getPoint4("diff", Point4(1, 1, 1, 1)));
    m->mat.spec.set_xyzw(bMat.getPoint4("spec", Point4(1, 1, 1, 1)));
    m->mat.emis.set_xyzw(bMat.getPoint4("emis", Point4(0, 0, 0, 1)));
    m->mat.amb.set_xyzw(bMat.getPoint4("amb", Point4(1, 1, 1, 1)));
    m->mat.power = 1.0f;
    m->flags = bMat.getBool("two_sided", false) ? MATF_2SIDED : 0;
    for (int slot = 0; slot < MAXMATTEXNUM; slot++)
    {
      tmp_storage.printf(0, "t%d", slot);
      int texIdx = bMat.getInt(tmp_storage, -1);
      m->mtex[slot] = texIdx >= 0 && texIdx < tex.size() ? tex[texIdx] : BAD_TEXTUREID;
    }
    mat[i] = new_shader_material(*m);
    if (mat[i])
      mat[i]->addRef();
  }
}
void ShaderMatVdata::loadMatVdata(const char *name, IGenLoad &crd, unsigned flags)
{
  int compr_type = 0; // none
  if ((matVdataHdrSz & 0xE0000000) == 0xC0000000)
  {
    compr_type = 3; // zstd / oodle
    matVdataHdrSz &= ~0xC0000000;
  }
  else if (matVdataHdrSz < 0)
  {
    compr_type = 1; // zlib
    matVdataHdrSz = -matVdataHdrSz;
  }
  else if (matVdataHdrSz & 0x40000000)
  {
    compr_type = 2; // lzma
    matVdataHdrSz &= ~0x40000000;
  }
#if VDATA_RELOAD_SUPPORTED
  if (!(flags & VDATA_SRC_ONLY) && (flags & VDATA_NO_IBVB) != VDATA_NO_IBVB && strcmp(crd.getTargetName(), "(mem)") != 0)
    flags |= VDATA_D3D_RESET_READY;
#endif

  matVdataSrcRef.fileOfs = crd.tell();
  matVdataSrcRef.dataSz = 0;
  matVdataSrcRef.comprType = compr_type;
  matVdataSrcRef.packTag = 0;
  G_ASSERTF(matVdataSrcRef.comprType == compr_type, "matVdataSrcRef.comprType=%d compr_type=%d", matVdataSrcRef.comprType, compr_type);

  MatVdataHdr *hdr = (MatVdataHdr *)(matVdataHdrSz > MAX_STACK_SIZE ? memalloc(matVdataHdrSz, tmpmem) : alloca(matVdataHdrSz));
  G_ASSERTF(!(flags & VDATA_SRC_ONLY) || (compr_type == 0 || compr_type == 3), "flags=0x%X compr_type=%d", flags, compr_type);
  IGenLoad *zcrd = &crd;
  InPlaceMemLoadCB *mcrd = NULL;
  if (compr_type == 3)
  {
    unsigned tag = 0;
    crd.beginBlock(&tag);
    int compr_data_sz = crd.getBlockRest();
    matVdataSrcRef.fileOfs = crd.tell();
    matVdataSrcRef.dataSz = compr_data_sz;
    matVdataSrcRef.packTag = tag;
    G_ASSERTF(matVdataSrcRef.dataSz == compr_data_sz, "matVdataSrcRef.dataSz=%d compr_data_sz=%d=0x%X (overflow?)",
      matVdataSrcRef.dataSz, compr_data_sz, compr_data_sz);
    G_ASSERTF(matVdataSrcRef.packTag == tag, "matVdataSrcRef.packTag=0x%X tag=%d", matVdataSrcRef.packTag, tag);
    if (flags & VDATA_SRC_ONLY)
    {
      matVdataSrcRef.fname = dagor_fname_map_add_fn(crd.getTargetName());
      if (matVdataSrc.resize(compr_data_sz))
      {
        crd.read(matVdataSrc.data(), data_size(matVdataSrc));
        mcrd = new (alloca(sizeof(InPlaceMemLoadCB)), _NEW_INPLACE) InPlaceMemLoadCB(matVdataSrc.data(), data_size(matVdataSrc));
      }
    }

    if (tag == btag_compr::ZSTD)
      zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(mcrd ? *mcrd : crd, compr_data_sz);
    else if (tag == btag_compr::OODLE)
      zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE)
        OodleLoadCB(mcrd ? *mcrd : crd, compr_data_sz - 4, mcrd ? mcrd->readInt() : crd.readInt());
  }
  else if (compr_type == 1)
    zcrd = new (alloca(sizeof(ZlibLoadCB)), _NEW_INPLACE) ZlibLoadCB(crd, 0x7FFFFFFF /*unlimited*/);
  else if (compr_type == 2)
    zcrd = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, 0x7FFFFFFF /*unlimited*/);

  // read and patch headers
  zcrd->read(hdr, matVdataHdrSz);
  hdr->mat.patch(hdr);
  hdr->vdata.patch(hdr);
  for (int i = hdr->vdata.size() - 1; i >= 0; i--)
    hdr->vdata[i].vDecl.patch(hdr);

  // create materials
  G_ASSERTF(hdr->mat.size() == mat.size() || !hdr->mat.size(), "mat.count=%d hdr->mat.count=%d", mat.size(), hdr->mat.size());
  for (int i = 0; i < hdr->mat.size(); i++)
  {
    const_cast<ShaderMaterialProperties &>(hdr->mat[i]).patchNonSharedData(hdr, tex);
    mat[i] = ScriptedShaderMaterial::create(hdr->mat[i]);
    if (mat[i])
      mat[i]->addRef();
  }

  if (matvdata::hook_on_add_vdata)
  {
    int vdsize = 0;
    for (int i = 0; i < vdata.size(); i++)
      vdsize += hdr->vdata[i].vertNum * hdr->vdata[i].vertStride + hdr->vdata[i].idxSize;
    matvdata::hook_on_add_vdata(this, vdsize);
  }

  if (compr_type == 0)
  {
    matVdataSrcRef.fileOfs = crd.tell();
    unsigned sz = 0;
    for (int i = 0; i < vdata.size(); i++)
      sz += hdr->vdata[i].vertNum * hdr->vdata[i].vertStride + hdr->vdata[i].packedIdxSize();
    matVdataSrcRef.dataSz = sz;
    G_ASSERTF(matVdataSrcRef.dataSz == sz, "matVdataSrcRef.dataSz=%d sz=%d=0x%X (overflow?)", matVdataSrcRef.dataSz, sz, sz);

    if (flags & VDATA_SRC_ONLY)
    {
      if (strcmp(crd.getTargetName(), "(mem)") != 0)
        matVdataSrcRef.fname = dagor_fname_map_add_fn(crd.getTargetName());
      if (matVdataSrc.resize(matVdataSrcRef.dataSz))
      {
        crd.read(matVdataSrc.data(), data_size(matVdataSrc));
        zcrd = new (alloca(sizeof(InPlaceMemLoadCB)), _NEW_INPLACE) InPlaceMemLoadCB(matVdataSrc.data(), data_size(matVdataSrc));
      }
      else if (matVdataSrcRef.dataSz)
        crd.seekrel(matVdataSrcRef.dataSz); // Note: `GlobalVertexData::initGvd` read nothing on `VDATA_SRC_ONLY` code path
    }
  }

  // read and create vb/ib
  Tab<uint8_t> tmp_decoder_stor;
  for (int i = 0; i < vdata.size(); i++)
    vdata[i].initGvd(name, hdr->vdata[i].vertNum, hdr->vdata[i].vertStride, hdr->vdata[i].packedIdxSize(), hdr->vdata[i].idxSize,
      flags | hdr->vdata[i].flags, zcrd, tmp_decoder_stor);
  if (vdataFullCount && (vdata.data() + 0)->getLodIndex() != 0)
    setLodsAreSplit();

  if (zcrd != &crd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
  }
  if (mcrd)
    mcrd->~InPlaceMemLoadCB();
  if (compr_type == 3)
    crd.endBlock();

  // free headers
  if (matVdataHdrSz > MAX_STACK_SIZE)
    memfree(hdr, tmpmem);

#if VDATA_RELOAD_SUPPORTED
  if ((flags & VDATA_D3D_RESET_READY) && (d3d::get_driver_code().is(d3d::dx11 || d3d::dx12 || d3d::vulkan)))
  {
    static struct ShaderVdataReloadStub : public Sbuffer::IReloadData
    {
      virtual void reloadD3dRes(Sbuffer *) {}
      virtual void destroySelf() {}
    } stub;
    Sbuffer::IReloadData *rld = this;
    unsigned used_d3d_buf_cnt = 0;

    for (int i = vdata.size() - 1; i >= 0; i--)
    {
      if ((!vdata[i].testFlags(VDATA_NO_VB) && vdata[i].getVB()) || (!vdata[i].testFlags(VDATA_NO_IB) && vdata[i].getIB()))
        used_d3d_buf_cnt++;
      if (!vdata[i].testFlags(VDATA_NO_IB) && vdata[i].getIB() && vdata[i].getIB()->setReloadCallback(rld))
        rld = &stub;
      if (!vdata[i].testFlags(VDATA_NO_VB) && vdata[i].getVB() && vdata[i].getVB()->setReloadCallback(rld))
        rld = &stub;
    }
    if (rld != this)
      matVdataSrcRef.fname = dagor_fname_map_add_fn(crd.getTargetName());
    else if (used_d3d_buf_cnt)
      logerr("failed to setReloadCallback() for %s:0x%x, contents (%d buffers) will be lost on d3d reset", //
        crd.getTargetName(), matVdataSrcRef.fileOfs, used_d3d_buf_cnt);
  }
#endif

  _resv = 'L';
  G_ASSERTF(_resv == 'L', "_resv=%d", _resv);
}

// compute end offset: align on stride boundary and payload size
static inline int calc_end_ofs(int ofs, int s, int vbsz) { return (ofs + s - 1) / s * s + vbsz; }
static FullFileLoadCB *reload_files_open(const char *fn, int ofs);

void ShaderMatVdata::unpackBuffersTo(dag::Span<Sbuffer *> buf, int *buf_byte_ofs, dag::Span<int> start_end_stride,
  Tab<uint8_t> &buf_stor)
{
  int64_t reft = profile_ref_ticks();
  ModelLoadStats &ml_stats = ShaderMatVdata::modelLoadStats[modelType <= 2 ? modelType : 0];

  FullFileLoadCB *fcrd = nullptr;
  if (matVdataSrc.empty() && isReloadable())
    fcrd = reload_files_open(matVdataSrcRef.fname, matVdataSrcRef.fileOfs);
  G_ASSERTF_RETURN(data_size(matVdataSrc) || fcrd, , "%p.matVdataSrcRef.fname=%s", this, matVdataSrcRef.fname);
  G_ASSERTF(buf_byte_ofs || start_end_stride.size(), "buf.count=%d buf_byte_ofs=%p start_end_stride.count=%d", buf.size(),
    buf_byte_ofs, start_end_stride.size());
  unsigned compr_type = matVdataSrcRef.comprType;
  unsigned pack_tag = matVdataSrcRef.packTag;

  InPlaceMemLoadCB mcrd(matVdataSrc.data(), data_size(matVdataSrc));
  IGenLoad *zcrd = fcrd ? static_cast<IGenLoad *>(fcrd) : &mcrd;
  if (compr_type == 3)
  {
    if (pack_tag == btag_compr::ZSTD)
      zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(*zcrd, matVdataSrcRef.dataSz);
    else if (pack_tag == btag_compr::OODLE)
      zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(*zcrd, matVdataSrcRef.dataSz - 4, zcrd->readInt());
    zcrd->seekrel(matVdataHdrSz);
  }
  else
    G_ASSERTF_RETURN(compr_type == 0, , "compr_type=%d pack_tag=%d", compr_type, pack_tag);

  if (buf_byte_ofs)
  {
    for (GlobalVertexData &vd : vdata)
      vd.unpackToSharedBuffer(*zcrd, buf[vd.getVbIdx()], buf[0], buf_byte_ofs[vd.getVbIdx()], buf_byte_ofs[0], buf_stor);
  }
  else
  {
    G_ASSERTF(start_end_stride.data(), "start_end_stride=%p,%d buf_byte_ofs=%p", start_end_stride.data(), start_end_stride.size(),
      buf_byte_ofs);
    int *cur_triple = start_end_stride.data() + 3;
    for (GlobalVertexData &vd : vdata)
    {
      while (vd.getStride() != cur_triple[2] || calc_end_ofs(cur_triple[0], vd.getStride(), vd.getVbSize()) > cur_triple[1])
        if (cur_triple + 3 < start_end_stride.end())
          cur_triple += 3;
        else
          break;
      G_ASSERTF(vd.getStride() == cur_triple[2] && cur_triple < start_end_stride.end(), "vd.getStride()=%d cur_triple=%d stride=%d",
        vd.getStride(), cur_triple - start_end_stride.data(), cur_triple[2]);
      vd.unpackToSharedBuffer(*zcrd, buf[vd.getVbIdx()], buf[0], cur_triple[0], start_end_stride[0], buf_stor);
      G_ASSERTF(cur_triple[0] <= cur_triple[1], "overrun ofs=%d > end=%d stride=%d vbIdx=%d", cur_triple[0], cur_triple[1],
        cur_triple[2], vd.getVbIdx());
    }
  }

  if (zcrd != fcrd && zcrd != &mcrd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
  }
  if (matVdataSrc.empty() && isReloadable())
  {
    interlocked_add(ml_stats.reloadDataSizeKb, matVdataSrcRef.dataSz > (1 << 10) ? (matVdataSrcRef.dataSz >> 10) : 1);
    interlocked_add(ml_stats.reloadTimeMsec, (profile_time_usec(reft) + 500) / 1000);
    interlocked_increment(ml_stats.reloadDataCount);
  }
}

void ShaderMatVdata::clearVdataSrc()
{
  if (isReloadable())
  {
    // debug("%p.clearVdataSrc(%s) sz=%d", this, matVdataSrcRef.fname, data_size(matVdataSrc));
    matVdataSrc.clear();
  }
}

struct LRUCachedFilesOpen
{
  static constexpr int entries_count = 4;
  typedef uint32_t hash_type_t;
  typedef uint32_t tick_type_t;
  struct Entry
  {
    FullFileLoadCB *crd;
    const char *fname; // Points to fnameMap (`dagor_fname_map_add_fn`)
    tick_type_t hitTick;
  };
  Entry entries[entries_count] = {};
  tick_type_t curTick = 0;
#if DAGOR_DBGLEVEL > 0
  uint32_t hits = 0, misses = 0;
  void addHit() { hits++; }
  void addMiss() { misses++; }
  void dumpHits()
  {
    if (hits + misses)
      debug("LRUCachedFilesOpen hits %.2f%% (%d/%d)", hits / float(hits + misses) * 100.f, hits, hits + misses);
  }
#else
  void addHit() {}
  void addMiss() {}
  void dumpHits() {}
#endif

  FullFileLoadCB *open(const char *fname, int ofs)
  {
    if (++curTick == 0)
      resetTick();

    uint32_t bestI = 0;
    tick_type_t bestTick = ~tick_type_t(0);
    for (uint32_t i = 0; i < entries_count; ++i)
    {
      if (entries[i].fname == fname) // Compare pointers, not content since it assumed to be pointing to one nameMap
      {
        entries[i].hitTick = curTick;
        addHit();
        entries[i].crd->seekto(ofs);
        return entries[i].crd;
      }
      // To consider: evict least used one (i.e. by number of hits) instead of oldest one
      if (entries[i].hitTick < bestTick)
        bestTick = entries[bestI = i].hitTick;
    }

    addMiss();
    auto &best = entries[bestI];
    best.fname = fname;
    best.hitTick = curTick;
    FullFileLoadCB *crd;
    if (best.crd)
    {
      (crd = best.crd)->open(fname, DF_READ | DF_IGNORE_MISSING);
    }
    else
      crd = best.crd = new FullFileLoadCB(fname, DF_READ | DF_IGNORE_MISSING);
    if (DAGOR_UNLIKELY(!crd->fileHandle))
    {
      dd_dump_base_paths();
      logerr("cannot restore buffer, file is missing: %s", fname);
      return nullptr;
    }
    crd->seekto(ofs);
    return crd;
  }
  void destroy()
  {
    for (uint32_t i = 0; i < entries_count; ++i)
    {
      del_it(entries[i].crd);
    }
    memset(entries, 0, sizeof(entries));
  }
  DAGOR_NOINLINE void resetTick()
  {
    curTick = 1;
    for (uint32_t i = 0; i < entries_count; ++i)
      entries[i].hitTick = 0;
  }
  ~LRUCachedFilesOpen()
  {
    destroy();
    dumpHits();
  }
};

static thread_local LRUCachedFilesOpen reloadFiles;
FullFileLoadCB *reload_files_open(const char *fn, int ofs) { return reloadFiles.open(fn, ofs); }

void ShaderMatVdata::closeTlsReloadCrd() {}

void ShaderMatVdata::reloadD3dRes(Sbuffer *)
{
  FullFileLoadCB crd(matVdataSrcRef.fname, DF_READ | DF_IGNORE_MISSING);
  if (!crd.fileHandle)
  {
    dd_dump_base_paths();
    logerr("cannot restore buffer, file is missing: %s", matVdataSrcRef.fname);
    return;
  }

  Tab<uint8_t> tmp_decoder_stor;
  unsigned compr_type = matVdataSrcRef.comprType;
  unsigned pack_tag = matVdataSrcRef.packTag;

  crd.seekto(matVdataSrcRef.fileOfs);
  IGenLoad *zcrd = &crd;

  if (compr_type == 3)
  {
    if (pack_tag == btag_compr::ZSTD)
      zcrd = new (alloca(sizeof(ZstdLoadCB)), _NEW_INPLACE) ZstdLoadCB(crd, matVdataSrcRef.dataSz);
    else if (pack_tag == btag_compr::OODLE)
      zcrd = new (alloca(sizeof(OodleLoadCB)), _NEW_INPLACE) OodleLoadCB(crd, matVdataSrcRef.dataSz - 4, crd.readInt());
  }
  else if (compr_type == 1)
    zcrd = new (alloca(sizeof(ZlibLoadCB)), _NEW_INPLACE) ZlibLoadCB(crd, 0x7FFFFFFF /*unlimited*/);
  else if (compr_type == 2)
    zcrd = new (alloca(sizeof(LzmaLoadCB)), _NEW_INPLACE) LzmaLoadCB(crd, 0x7FFFFFFF /*unlimited*/);

  if (compr_type != 0)
    zcrd->seekrel(matVdataHdrSz);
  for (int i = 0; i < getGlobVDataCount(); i++)
    getGlobVData(i)->unpackToBuffers(*zcrd, true, tmp_decoder_stor);

  if (zcrd != &crd)
  {
    zcrd->ceaseReading();
    zcrd->~IGenLoad();
  }
}
