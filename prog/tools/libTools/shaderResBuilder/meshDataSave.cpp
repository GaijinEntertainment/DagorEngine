// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMatData.h>
#include <shaders/dag_shaderMesh.h>
#include <3d/fileTexFactory.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_zlibIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_btagCompr.h>
#include <debug/dag_debug.h>
#include <util/dag_oaHashNameMap.h>
#include <meshoptimizer/include/meshoptimizer.h>

bool shadermeshbuilder_strip_d3dres = false;

extern bool shader_mats_are_equal(ShaderMaterial *m1, ShaderMaterial *m2);

ShaderTexturesSaver::ShaderTexturesSaver() : textures(tmpmem), texNames(tmpmem) {}

int ShaderTexturesSaver::addTexture(TEXTUREID tex)
{
  if (tex == BAD_TEXTUREID)
    return -1;

  int i = gettexindex(tex);
  if (i >= 0)
    return i;
  i = append_items(textures, 1, &tex);
  return i;
}

int ShaderTexturesSaver::gettexindex(TEXTUREID tex)
{
  if (tex == BAD_TEXTUREID)
    return -1;

  for (int i = 0; i < textures.size(); ++i)
    if (textures[i] == tex)
      return i;
  return -1;
}


void ShaderTexturesSaver::prepareTexNames(dag::ConstSpan<String> tex_names)
{
  texNames.resize(textures.size());

  for (int i = 0; i < textures.size(); ++i)
  {
    texNames[i] = get_managed_texture_name(textures[i]);

    for (int j = 0; j < tex_names.size(); ++j)
      if (stricmp(tex_names[j], texNames[i]) == 0)
      {
        texNames[i].printf(32, "#%d", j);
        break;
      }
  }
}

void ShaderTexturesSaver::writeTexStr(mkbindump::BinDumpSaveCB &cwr)
{
  mkbindump::PatchTabRef pt;
  SmallTab<int, TmpmemAlloc> ofs;

  cwr.beginBlock();
  cwr.setOrigin();

  pt.reserveTab(cwr);

  clear_and_resize(ofs, textures.size());
  for (int i = 0; i < texNames.size(); ++i)
  {
    ofs[i] = cwr.tell();
    cwr.writeRaw((char *)texNames[i], texNames[i].length() + 1);
  }

  cwr.align4();
  pt.startData(cwr, ofs.size());
  cwr.writeTabData32ex(ofs);
  pt.finishTab(cwr);

  cwr.popOrigin();
  cwr.endBlock();
}
void ShaderTexturesSaver::writeTexIdx(mkbindump::BinDumpSaveCB &cwr, dag::Span<TEXTUREID> map)
{
  int i, j;

  for (i = 0; i < textures.size(); ++i)
  {
    for (j = 0; j < map.size(); j++)
      if (textures[i] == map[j])
      {
        cwr.writeInt32e(j);
        break;
      }
    if (j >= map.size())
      cwr.writeInt32e(-1);
  }
}
void ShaderTexturesSaver::writeTexToBlk(DataBlock &dest)
{
  DataBlock *b = dest.addBlock("tex");
  b->clearData();
  for (auto &tex : texNames)
    b->addStr("tex", tex);
}


// ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ//


ShaderMaterialsSaver::ShaderMaterialsSaver() : mats(tmpmem_ptr()), vdata(tmpmem_ptr()), hdrPos(-1) {}

int ShaderMaterialsSaver::addMaterial(ShaderMaterial *m, MaterialData *src)
{
  if (!m)
    return -1;

  int i = getmatindex(m);
  if (i >= 0)
    return i;
  i = append_items(mats, 1);
  mats[i].mat = m;
  mats[i].par = src ? src->matScript.c_str() : "";
  return i;
}

int ShaderMaterialsSaver::addMaterial(ShaderMaterial *m, ShaderTexturesSaver &tex_saver, MaterialData *src)
{
  if (!m)
    return -1;

  int i = getmatindex(m);
  if (i >= 0)
    return i;
  i = append_items(mats, 1);
  mats[i].mat = m;
  mats[i].par = src ? src->matScript.c_str() : "";

  int numTex = m->get_num_textures();
  for (int j = 0; j < numTex; ++j)
    tex_saver.addTexture(m->get_texture(j));

  return i;
}

int ShaderMaterialsSaver::getmatindex(ShaderMaterial *m)
{
  if (!m)
    return -1;

  for (int i = 0; i < mats.size(); ++i)
    if (::shader_mats_are_equal(mats[i].mat, m))
      return i;
  return -1;
}

void ShaderMaterialsSaver::writeMatVdataHdr(mkbindump::BinDumpSaveCB &cwr, ShaderTexturesSaver &tex)
{
  debug("tex count %d, mat count %d", tex.textures.size(), mats.size());

  cwr.writeInt32e(tex.textures.size());
  cwr.writeInt32e(mats.size());
  cwr.writeInt32e(vdata.size());
  hdrPos = cwr.tell();
  cwr.writeInt32e(0);
}
void ShaderMaterialsSaver::writeMatVdataCombinedNoMat(mkbindump::BinDumpSaveCB &cwr)
{
  debug("MatVdata in sep-mat mode (mat count %d)", mats.size());

  cwr.writeInt32e(0xFFFFFFFF);
  cwr.writeInt32e(0xFFFFFFFF);
  cwr.writeInt32e(vdata.size());
  hdrPos = cwr.tell();
  cwr.writeInt32e(0);

  ShaderTexturesSaver empty_tex_saver;
  Tab<MatData> mats_backup;
  mats.swap(mats_backup);

  writeMatVdata(cwr, empty_tex_saver);

  mats_backup.swap(mats);
}
void ShaderMaterialsSaver::writeMatToBlk(DataBlock &dest, ShaderTexturesSaver &texSaver, const char *mat_block_name)
{
  int varNumBonesId = VariableMap::getVariableId("num_bones", true);
  DataBlock *b = dest.addBlock(mat_block_name);
  b->clearData();
  for (auto &mat : mats)
  {
    Ptr<MaterialData> m = new MaterialData;
    int num_bones = 0;
    if (!mat.mat->getIntVariable(varNumBonesId, num_bones))
      num_bones = 0;
    mat.mat->buildMaterialData(*m, num_bones ? ("num_bones=?\n" + mat.par).str() : mat.par.str());

    DataBlock *b2 = b->addNewBlock("mat");
    b2->setStr("cls", m->className);
    b2->setStr("par", m->matScript);
    if (m->mat.diff != D3dColor(1, 1, 1, 1))
      b2->setPoint4("diff", Point4::rgba(m->mat.diff));
    if (m->mat.spec != D3dColor(1, 1, 1, 1))
      b2->setPoint4("spec", Point4::rgba(m->mat.spec));
    if (m->mat.emis != D3dColor(0, 0, 0, 1))
      b2->setPoint4("emis", Point4::rgba(m->mat.emis));
    if (m->mat.amb != D3dColor(1, 1, 1, 1))
      b2->setPoint4("amb", Point4::rgba(m->mat.amb));
    if (m->flags & MATF_2SIDED)
      b2->setBool("two_sided", true);
    for (int slot = 0; slot < MAXMATTEXNUM; slot++)
      if (m->mtex[slot] != BAD_TEXTUREID)
        b2->setInt(String(0, "t%d", slot), find_value_idx(texSaver.textures, m->mtex[slot]));
  }
}

void ShaderMaterialsSaver::writeMatVdata(mkbindump::BinDumpSaveCB &cwr, ShaderTexturesSaver &tex)
{
  mkbindump::PatchTabRef mat_pt, vdh_pt;
  mkbindump::SharedStorage<ShaderMatData::VarValue> vals;
  mkbindump::SharedStorage<CompiledShaderChannelId> chans;
  Tab<mkbindump::Ref> ref(tmpmem);
  Tab<int> ofs(tmpmem);
  NameMap shname;
  int matVdataHdrSz;
  int final_sz, raw_sz;
  Tab<VdataHdr> vd_hdr;
  Tab<unsigned char> ib_packed;

  G_ASSERT(hdrPos != -1 && "writeMatVdataHdr not called?");

  // write MatVdataHdr
  {
    mkbindump::BinDumpSaveCB cwr_d(64 << 20, cwr.getTarget(), cwr.WRITE_BE);

    mat_pt.reserveTab(cwr_d);
    vdh_pt.reserveTab(cwr_d);

    ofs.resize(mats.size());
    for (int i = 0; i < mats.size(); ++i)
    {
      ShaderMatData smd;
      mats[i].mat->getMatData(smd);
      ofs[i] = shname.addNameId(smd.sclassName);
    }
    ofs.resize(mats.size() + shname.nameCount());
    for (int i = 0; i < shname.nameCount(); ++i)
    {
      ofs[mats.size() + i] = cwr_d.tell();
      cwr_d.writeRaw(shname.getStringDataUnsafe(i), i_strlen(shname.getStringDataUnsafe(i)) + 1);
    }
    cwr_d.align8();

    ref.resize(mats.size());
    for (int i = 0; i < mats.size(); ++i)
    {
      ShaderMatData smd;
      mats[i].mat->getMatData(smd);

      vals.getRef(ref[i], smd.stvar.data(), smd.stvar.size(), 512);
    }
    cwr_d.writeStorage32ex(vals);

    cwr_d.align8();
    mat_pt.startData(cwr_d, mats.size());
    for (int i = 0; i < mats.size(); ++i)
    {
      ShaderMatData smd;
      mats[i].mat->getMatData(smd);

      cwr_d.write32ex(&smd.d3dm, sizeof(smd.d3dm));
      cwr_d.writeInt32e(ofs[mats.size() + ofs[i]]);
      cwr_d.writePtr64e(0);

      for (int j = 0; j < MAXMATTEXNUM; j++)
        cwr_d.writeInt32e(tex.gettexindex(smd.textureId[j]));

      vals.refIndexToOffset(ref[i]);
      cwr_d.writeRef(ref[i]);

      cwr_d.writeInt32e(0);

      cwr_d.writeInt16e(smd.matflags);
      cwr_d.writeInt16e(1);
    }
    mat_pt.finishTab(cwr_d);


    ref.resize(vdata.size());
    for (int i = 0; i < vdata.size(); ++i)
      chans.getRef(ref[i], vdata[i]->vDesc.data(), vdata[i]->vDesc.size(), 64);
    cwr_d.writeStorage32ex(chans);

    cwr_d.align8();
    vdh_pt.startData(cwr_d, vdata.size());
    vd_hdr.resize(vdata.size());
    mem_set_0(vd_hdr);
    int vd_hdr_ofs = cwr_d.tell();
    for (int i = 0; i < vdata.size(); ++i)
    {
      vd_hdr[i].vertStride = vdata[i]->stride;
      vd_hdr[i].flags = vdata[i]->gvdFlags;
      if (!shadermeshbuilder_strip_d3dres)
      {
        vd_hdr[i].vertNum = vdata[i]->numv;
        vd_hdr[i].idxSize = data_size(vdata[i]->iData) ? data_size(vdata[i]->iData) : data_size(vdata[i]->iData32);
      }
      if (vdata[i]->numf < 6)
        vd_hdr[i].flags &= ~VDATA_PACKED_IB;

      G_ASSERTF(vdata[i]->numv <= 65536 || !vdata[i]->iData.size() || vdata[i]->baseVertSegCount,
        "vdata[%d]->numv=%d idata=%d idata32=%d baseVertSegCount=%d", i, vdata[i]->numv, vdata[i]->iData.size(),
        vdata[i]->iData32.size(), vdata[i]->baseVertSegCount);
      G_ASSERTF(
        vd_hdr[i].vertStride == vdata[i]->stride && data_size(vdata[i]->iData) < (1 << 28) && data_size(vdata[i]->iData32) < (1 << 28),
        "vdata[%d]->numv=%d stride=%d idata=%d idata32=%d", i, vdata[i]->numv, vdata[i]->stride, vdata[i]->iData.size(),
        vdata[i]->iData32.size());

      cwr_d.write32ex(&vd_hdr[i], sizeof(vd_hdr[i]));
      chans.refIndexToOffset(ref[i]);
      cwr_d.writeRef(ref[i]);
    }
    int vd_hdr_stride = vdata.size() ? (cwr_d.tell() - vd_hdr_ofs) / vdata.size() : 0;
    vdh_pt.finishTab(cwr_d);

    matVdataHdrSz = cwr_d.tell();

    // write VB/IB
    Tab<uint8_t> vd_tmp(tmpmem);
    if (!shadermeshbuilder_strip_d3dres)
      for (int i = 0; i < vdata.size(); ++i)
      {
        cwr_d.writeRaw(vdata[i]->vData.data(), data_size(vdata[i]->vData));

        if (vdata[i]->numf > 0)
        {
          if (vd_hdr[i].flags & VDATA_PACKED_IB)
          {
            if (vdata[i]->iData.size())
            {
              ib_packed.resize(data_size(vdata[i]->iData));
              ib_packed.resize(
                meshopt_encodeIndexSequence(ib_packed.data(), data_size(ib_packed), vdata[i]->iData.data(), vdata[i]->iData.size()));
            }
            else
            {
              ib_packed.resize(data_size(vdata[i]->iData32));
              ib_packed.resize(meshopt_encodeIndexSequence(ib_packed.data(), data_size(ib_packed), vdata[i]->iData32.data(),
                vdata[i]->iData32.size()));
            }
            G_ASSERTF(data_size(ib_packed) > 0 && data_size(ib_packed) < (1 << 28),
              "vd->numf=%d vd->iData=%d vd->iData32=%d idxSize=%d", vdata[i]->numf, vdata[i]->iData.size(), vdata[i]->iData32.size(),
              vd_hdr[i].idxSize);
            vd_hdr[i].packedIdxSizeLo = data_size(ib_packed);
            vd_hdr[i].packedIdxSizeHi = data_size(ib_packed) >> 24;
            cwr_d.writeTabDataRaw(ib_packed);
          }
          else if (vdata[i]->iData.size())
            cwr_d.writeTabData16e(vdata[i]->iData);
          else
            cwr_d.writeTabData32ex(vdata[i]->iData32);
        }

        int pos = cwr_d.tell();
        cwr_d.seekto(vd_hdr_ofs + vd_hdr_stride * i);
        cwr_d.write32ex(&vd_hdr[i], sizeof(vd_hdr[i]));
        cwr_d.seekto(pos);
      }

    final_sz = raw_sz = cwr_d.getSize();

    if (cwr_d.getSize() < (2 << 10) || matVdataHdrSz == 0 || ShaderMeshData::fastNoPacking)
      cwr.copyRawFrom(cwr_d);
    else if (ShaderMeshData::preferZstdPacking)
    {
      matVdataHdrSz = matVdataHdrSz | 0xC0000000;
      int start_pos = cwr.tell();

      MemoryLoadCB mcrd(cwr_d.getMem(), false);
      cwr.beginBlock();
      if (ShaderMeshData::allowOodlePacking && dagor_target_code_store_packed(cwr.getTarget()))
      {
        cwr.writeInt32e(cwr_d.getSize());
        oodle_compress_data(cwr.getRawWriter(), mcrd, cwr_d.getSize());
        cwr.endBlock(btag_compr::OODLE);
      }
      else
      {
        zstd_compress_data(cwr.getRawWriter(), mcrd, cwr_d.getSize(), 1 << 20, ShaderMeshData::zstdCompressionLevel,
          ShaderMeshData::zstdMaxWindowLog);
        cwr.endBlock(btag_compr::ZSTD);
      }

      final_sz = cwr.tell() - start_pos;
    }
    else
    {
      matVdataHdrSz = -matVdataHdrSz;
      int start_pos = cwr.tell();

      ZlibSaveCB z_cwr(cwr.getRawWriter(), ZlibSaveCB::CL_BestCompression);
      cwr_d.copyDataTo(z_cwr);
      z_cwr.finish();

      final_sz = cwr.tell() - start_pos;
    }
  }
  cwr.writeInt32eAt(matVdataHdrSz, hdrPos);
  hdrPos = -1;

  debug("saved: %d vdatas, %d mats, matVdataHdrSz=0x%08X compressed/raw size= %d/%d", vdata.size(), mats.size(), matVdataHdrSz,
    final_sz, raw_sz);
}

// add global vertex data
void ShaderMaterialsSaver::addGlobVData(GlobalVertexDataSrc *v)
{
  for (int i = 0; i < vdata.size(); i++)
    if (vdata[i] == v)
      return;
  vdata.push_back(v);
}

// get global vertex data index
int ShaderMaterialsSaver::getGlobVData(GlobalVertexDataSrc *v)
{
  int i;
  for (i = 0; i < vdata.size(); i++)
    if (vdata[i] == v)
      break;
  if (i == vdata.size())
    DAG_FATAL("vdata %p is not found! vdata.size()=%d", v, vdata.size());
  return i;
}
