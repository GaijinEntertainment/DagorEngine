// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaders.h>
#include "scriptSElem.h"
#include "scriptSMat.h"
#include <3d/dag_render.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_genIo.h>
#include <ioSys/dag_readToUncached.h>
#include <debug/dag_debug.h>
#include <meshoptimizer/include/meshoptimizer.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_info.h>


#if DAGOR_DBGLEVEL > 0
bool ShaderMesh::dbgRenderStarted = false;
#endif

/*********************************
 *
 * class GlobalVertexData
 *
 *********************************/
void GlobalVertexData::initGvd(const char *name, unsigned vNum, unsigned vStride, unsigned idxPacked, unsigned idxSize, unsigned flags,
  IGenLoad *crd, Tab<uint8_t> &tmp_decoder_stor)
{
  if (vNum > 65536 && !(flags & VDATA_I16))
    flags |= VDATA_I32;

  vstride = vStride;
  cflags = flags;

  vbMem = NULL;
  ibMem = NULL;
  vbIdx = vOfs = iOfs = 0;
  iPackedSz = (flags & VDATA_PACKED_IB) ? idxPacked : 0;
  vCnt = vNum;
  iCnt = idxSize / getIbElemSz();

  if (flags & VDATA_SRC_ONLY)
  {
    G_ASSERTF(!(flags & VDATA_I32), "vNum=%d flags=0x%X", vNum, flags);
    return;
  }

  // create vertex buffer
  unsigned vbFlags = 0;
  if (flags & VDATA_BIND_SHADER_RES)
    vbFlags |= SBCF_BIND_SHADER_RES;
  if (flags & VDATA_NO_VB)
  {
    vbMem = memalloc(vNum * vStride + sizeof(int), tmpmem);
    *(int *)vbMem = vNum;
  }
  else
  {
    vb = d3d::create_vb(vNum * vStride, vbFlags, name);
    d3d_err(vb);
  }

  // create index buffer
  if (idxSize)
  {
    unsigned ibFlags = 0;
    if (flags & VDATA_I32)
      ibFlags |= SBCF_INDEX32;
    if (flags & VDATA_BIND_SHADER_RES)
      ibFlags |= SBCF_BIND_SHADER_RES;
    if (flags & VDATA_NO_IB)
    {
      ibMem = memalloc(idxSize + sizeof(int), tmpmem);
      *(int *)ibMem = idxSize;
    }
    else
    {
      String ibName(0, "%s_globalVertexData_ib", name);
      indices = d3d::create_ib(idxSize, ibFlags, ibName);
      d3d_err(indices);
    }
  }
  else
    indices = NULL;

  if (crd)
    unpackToBuffers(*crd, false, tmp_decoder_stor);
}
void GlobalVertexData::initGvdMem(int vNum, int vStride, int idxSize, unsigned flags, const void *vb_data, const void *ib_data)
{
  Tab<uint8_t> tmp_decoder_stor;
  bool need_set_rld = !(flags & VDATA_D3D_RESET_READY) && vNum && vb_data;
  initGvd("mem", vNum, vStride, 0, idxSize, flags | (vb_data && ib_data ? VDATA_D3D_RESET_READY : 0), nullptr, tmp_decoder_stor);

  if (vb_data && vb)
    d3d_err(vb->updateDataWithLock(0, getVbSize(), vb_data, VBLOCK_WRITEONLY)); // load vertex buffer data

  // create index buffer
  if (ib_data && getIbSize())
    if (auto _ib = getIB())
      d3d_err(_ib->updateDataWithLock(0, getIbSize(), ib_data, VBLOCK_WRITEONLY)); // load index buffer data

  if (need_set_rld && vb && (d3d::get_driver_code().is(d3d::dx11) || d3d::get_driver_code().is(d3d::dx12)))
  {
#if _TARGET_PC_WIN
    G_ASSERT(!(flags & VDATA_NO_IBVB));
    G_ASSERT(vb_data || !getVbSize());
    G_ASSERT(ib_data || !getIbSize());

    struct VdataReloadFromMem : public Sbuffer::IReloadData
    {
      GlobalVertexData &gvd;
      Tab<uint8_t> buf;
      VdataReloadFromMem(GlobalVertexData &_gvd, const void *vb_data, const void *ib_data) : gvd(_gvd)
      {
        buf.resize(gvd.getVbSize() + gvd.getIbSize());
        memcpy(buf.data(), vb_data, gvd.getVbSize());
        if (ib_data)
          memcpy(buf.data() + gvd.getVbSize(), ib_data, gvd.getIbSize());
      }
      void reloadD3dRes(Sbuffer *) override
      {
        if (gvd.vb)
          d3d_err(gvd.vb->updateDataWithLock(0, gvd.getVbSize(), buf.data(), VBLOCK_WRITEONLY));
        if (auto _ib = gvd.getIB())
          d3d_err(_ib->updateDataWithLock(0, gvd.getIbSize(), buf.data() + gvd.getVbSize(), VBLOCK_WRITEONLY));
      }
      void destroySelf() override { delete this; }
    };

    auto rld = new VdataReloadFromMem(*this, vb_data, ib_data);
    auto _ib = getIB();
    Sbuffer *latest_buf = _ib ? _ib : vb;
    if (!latest_buf->setReloadCallback(rld))
    {
      logerr("failed to install d3d buffer reload callback");
      delete rld;
    }
#endif
  }
}

void GlobalVertexData::unpackToBuffers(IGenLoad &zcrd, bool update_ib_vb_only, Tab<uint8_t> &tmp_decoder_stor)
{
  if (!testFlags(VDATA_NO_VB))
  {
    bool cached_dest_mem = testFlags(VDATA_NO_VB);
#if _TARGET_PC_WIN
    cached_dest_mem = true;
#endif

    void *p = nullptr;
    d3d_err(vb->lock(0, 0, &p, VBLOCK_WRITEONLY));
    if (p)
    {
      read_to_device_mem(zcrd, p, getVbSize(), cached_dest_mem);
      d3d_err(vb->unlock());
    }
    else
      zcrd.seekrel(getVbSize());
  }
  else if (!update_ib_vb_only)
    zcrd.read((void *)((char *)vbMem + sizeof(int)), getVbSize());

  if (!getIbSize())
    return;

  int decode_result = 0;
  G_UNUSED(decode_result);
  if (testFlags(VDATA_PACKED_IB))
  {
    tmp_decoder_stor.resize(iPackedSz);
    zcrd.read(tmp_decoder_stor.data(), iPackedSz);
  }

  if (!testFlags(VDATA_NO_VB))
  {
    bool cached_dest_mem = testFlags(VDATA_NO_IB);
#if _TARGET_PC_WIN
    cached_dest_mem = true;
#endif

    void *p = nullptr;
    d3d_err(getIB()->lock(0, 0, &p, VBLOCK_WRITEONLY));
    // debug("%p.unpackToBuffers iCnt=%d iPackedSz=%d flags=0x%X", this, iCnt, iPackedSz, cflags);
    if (p)
    {
      if (testFlags(VDATA_PACKED_IB))
        decode_result = meshopt_decodeIndexSequence(p, iCnt, getIbElemSz(), tmp_decoder_stor.data(), iPackedSz);
      else
        read_to_device_mem(zcrd, p, getIbSize(), cached_dest_mem);
      d3d_err(getIB()->unlock());
    }
    else if (!testFlags(VDATA_PACKED_IB))
      zcrd.seekrel(getIbSize());
  }
  else if (!update_ib_vb_only)
  {
    void *p = (void *)((char *)ibMem + sizeof(int));
    if (testFlags(VDATA_PACKED_IB))
      decode_result = meshopt_decodeIndexSequence(p, iCnt, getIbElemSz(), tmp_decoder_stor.data(), iPackedSz);
    else
      zcrd.read(p, getIbSize());
  }
  G_ASSERTF(decode_result == 0, "error unpacking IB (packedSz=%d sz=%d flags=0x%X) decode_result=%d", iPackedSz, getIbSize(),
    getFlags(), decode_result);
}

void GlobalVertexData::unpackToSharedBuffer(IGenLoad &zcrd, Vbuffer *shared_vb, Ibuffer *shared_ib, int &vb_byte_pos, int &ib_byte_pos,
  Tab<uint8_t> &buf_stor)
{
  vb = shared_vb;
  indices = shared_ib;

  vb_byte_pos = (vb_byte_pos + vstride - 1) / vstride * vstride; // align on stride boundary
  vOfs = vb_byte_pos / vstride;
  iOfs = ib_byte_pos / 2;
#if _TARGET_PC_WIN
  // FIXME: dx11 is seems to be the only driver impl which doesn't support partial buf updates via temp memory
  const bool forceIntermediateBufUpdate = d3d::get_driver_code().is(d3d::dx11);
#else
  constexpr bool forceIntermediateBufUpdate = false;
#endif
  auto updateSBufData = [&](Sbuffer *sbuf, int ofs, int size, auto writecb) {
    if (!forceIntermediateBufUpdate)
    {
      void *xbdata = nullptr;
      if (DAGOR_LIKELY(sbuf->lock(ofs, size, &xbdata, VBLOCK_WRITEONLY)))
      {
        G_FAST_ASSERT(xbdata);
        writecb(xbdata);
        sbuf->unlock();
      }
      else
        d3d_err(xbdata);
    }
    else
    {
      buf_stor.resize(buf_stor.size() + size);
      writecb(buf_stor.end() - size);
      d3d_err(sbuf->updateData(ofs, size, buf_stor.end() - size, VBLOCK_WRITEONLY));
    }
  };

  {
    int sz = vCnt * vstride;
    G_ASSERTF((vb_byte_pos % 4) == 0 && (sz % 4) == 0, "broken VB alignment: vb_byte_pos==%d sz=%d", vb_byte_pos, sz);
    buf_stor.clear();
    updateSBufData(shared_vb, vb_byte_pos, sz, [&](void *data) { zcrd.read(data, sz); });
    vb_byte_pos += sz;
  }

  {
    clear_and_resize(buf_stor, iPackedSz);
    if (iPackedSz)
      zcrd.read(buf_stor.data(), iPackedSz);
    int sz = iCnt * sizeof(uint16_t), sza = sz + (iCnt & 1) * 2 /* align on 4-byte */;
    updateSBufData(shared_ib, ib_byte_pos, sza, [&](void *data) {
      if (iPackedSz)
      {
        int ret = meshopt_decodeIndexSequence((uint16_t *)data, iCnt, buf_stor.data(), iPackedSz);
        G_ASSERTF(ret == 0, "error unpacking IB (packedSz=%d sz=%d flags=0x%X)", iPackedSz, sz, cflags);
        G_UNUSED(ret);
      }
      else
        zcrd.read(data, sz);
      if (iCnt & 1)
        ((char *)data)[sz + 1] = ((char *)data)[sz] = 0;
    });
    G_ASSERTF(!(ib_byte_pos & 3) && !(sza & 3), "broken IB alignment: ib_byte_pos==%d sz=%d", ib_byte_pos, sz);
    ib_byte_pos += sza;
  }
}

void GlobalVertexData::free()
{
  if (cflags & VDATA_SRC_ONLY)
  {
    if (vbIdx == 0 && indices && iOfs == 0 && vb && vOfs == 0) // previously allocated tight vb/ib for single res, release it here
      del_d3dres(indices), del_d3dres(vb);
    vb = nullptr;
    indices = nullptr;
    return;
  }

  if (cflags & VDATA_NO_IB)
  {
    if (ibMem)
      memfree(ibMem, tmpmem);
    ibMem = NULL;
  }
  else
  {
    del_d3dres(indices);
  }

  if (cflags & VDATA_NO_VB)
  {
    if (vbMem)
      memfree(vbMem, tmpmem);
    vbMem = NULL;
  }
  else
  {
    del_d3dres(vb);
  }
}

// set all params to driver
void GlobalVertexData::setToDriver() const
{
  G_ASSERT(!(cflags & VDATA_NO_IBVB));
  if (indices)
  {
    G_ASSERT(indices->getFlags() & SBCF_BIND_INDEX);
    d3d_err(d3d::setind(indices));
  }
  if (vCnt != 0 && vb == nullptr)
    logerr("GlobalVetexData is empty vcnt=%d", vCnt);
  d3d_err(d3d::setvsrc(0, vb, vstride));
}

// class GlobalVertexData
//

//*************************************************************
// desc for single element (vertex group & shader)
//*************************************************************


/*********************************
 *
 * class ShaderMesh
 *
 *********************************/

// load data and create ShaderMesh with shared vertex/index buffers
ShaderMesh *ShaderMesh::createCopy(const ShaderMesh &sm)
{
  size_t sz = sizeof(sm) + data_size(sm.elems);
  void *mem = memalloc(sz, midmem);
  ShaderMesh *newsm = new (mem, _NEW_INPLACE) ShaderMesh;

  newsm->elems.init((char *)mem + sizeof(sm), sm.elems.size());
  mem_copy_to(sm.elems, newsm->elems.data());
  memcpy(newsm->stageEndElemIdx, sm.stageEndElemIdx, sizeof(newsm->stageEndElemIdx));
  newsm->_deprecatedMaxMatPass = sm._deprecatedMaxMatPass;
  newsm->_resv = 0;

  newsm->rebaseAndClone(newsm, newsm);
  return newsm;
}

// create Shader mesh by load dump from stream
ShaderMesh *ShaderMesh::load(IGenLoad &crd, int sz, ShaderMatVdata &smvd, bool acqire_tex_refs)
{
  void *mem = memalloc(sz, midmem);
  ShaderMesh *sm = new (mem, _NEW_INPLACE) ShaderMesh;
  crd.read(sm, sz);
  sm->patchData(mem, smvd);
  if (acqire_tex_refs)
    sm->acquireTexRefs();
  return sm;
}
ShaderMesh *ShaderMesh::loadMem(const void *p, int sz, ShaderMatVdata &smvd, bool acqire_tex_refs)
{
  void *mem = memalloc(sz, midmem);
  ShaderMesh *sm = new (mem, _NEW_INPLACE) ShaderMesh;
  memcpy(sm, p, sz);
  sm->patchData(mem, smvd);
  if (acqire_tex_refs)
    sm->acquireTexRefs();
  return sm;
}

void ShaderMesh::patchData(void *base, ShaderMatVdata &smvd)
{
  elems.patch(base);
  if (_deprecatedMaxMatPass & 0x80000000)
    _deprecatedMaxMatPass &= ~0x80000000; // modern format
  else
  {
    // older format for elem, telem
    PatchableTab<RElem> &telem = (&elems)[1];
    telem.patch(base);
    if (!elems.size())
      elems.init(telem.data(), 0);

    // validate telem
    int telem_cnt = telem.size();
    if (telem_cnt)
    {
      G_ASSERTF(elems.data() + elems.size() == telem.data(), "elem=%p,%d telem=%p,%d", elems.data(), elems.size(), telem.data(),
        telem.size());
      if (elems.data() + elems.size() != telem.data())
        telem_cnt = 0; // drop telem due to internal error in format
    }

    // build new stageEndElemIdx
    for (int i = 0; i < SC_STAGE_IDX_MASK + 1; i++)
      stageEndElemIdx[i] = (i < STG_trans) ? elems.size() : elems.size() + telem_cnt;
    elems.init(elems.data(), stageEndElemIdx[STG_trans]);
  }
  for (RElem &re : elems)
  {
    int mat_idx = (int)(intptr_t)re.mat.get();
    if (mat_idx >= smvd.getMaterialCount())
    {
      logerr("#%d material in RElem, %d material(s) in ShaderMatVdata - mismatched versions of *.grp and *desc.bin", mat_idx,
        smvd.getMaterialCount());
      mat_idx = 0;
    }
    re.mat.init(smvd.getMaterial(mat_idx));
    re.e = re.mat->make_elem(false, NULL);
    re.vertexData = smvd.getGlobVData((int)(intptr_t)re.vertexData);
  }
}
void ShaderMesh::rebaseAndClone(void *new_base, const void *old_base)
{
  elems.rebase(new_base, old_base);
  for (int i = 0; i < elems.size(); i++)
  {
    elems[i].mat.addRef();
    elems[i].e.addRef();
  }
}

void ShaderMesh::clearData()
{
  for (int i = 0; i < elems.size(); i++)
  {
    elems[i].e = NULL;
    elems[i].mat = NULL;
  }

  if (elems.size() == 1 && elems[0].vdOrderIndex == -7)
    elems[0].vertexData->free();

  elems.init(NULL, 0);
}

void ShaderMesh::acquireTexRefs()
{
  for (unsigned int i = 0; i < elems.size(); i++)
    if (elems[i].e)
      elems[i].e->native().acquireTexRefs();
}
void ShaderMesh::releaseTexRefs()
{
  for (unsigned int i = 0; i < elems.size(); i++)
    if (elems[i].e && elems[i].mat && elems[i].mat->native().isNonShared())
      elems[i].e->native().releaseTexRefs();
}
int ShaderMesh::calcTotalFaces() const
{
  int faces = 0, i;

  for (i = 0; i < elems.size(); i++)
    faces += elems[i].numf;

  return faces;
}

bool ShaderMesh::getVbInfo(RElem &relem, int usage, int usage_index, unsigned int &stride, unsigned int &offset, int &type,
  int &mod) const
{
  // Parse vertex format for normal offset and vertex buffer stride.
  const ScriptedShaderElement *shaderElem = (const ScriptedShaderElement *)relem.e.get();
  stride = 0;
  offset = 0xFFFFFFFF;
  for (unsigned int channelNo = 0; channelNo < shaderElem->code.channel.size(); channelNo++)
  {
    const ShaderChannelId &channel = shaderElem->code.channel[channelNo];
    // if (channel.u == SCUSAGE_NORM && channel.ui == 0)
    if (channel.u == usage && channel.ui == usage_index)
    {
      type = channel.t;
      offset = stride;
      mod = channel.mod;
    }

    unsigned int channelSize = 0;
    if (!channel_size(channel.t, channelSize))
      DAG_FATAL("Unknown channel type");
    stride += channelSize;
  }

  return offset != 0xFFFFFFFF;
}

// render items
void ShaderMesh::render(dag::Span<RElem> elem_array) const
{
  //  debug("render %d items", elem_array.size());
  GlobalVertexData *vertexData = NULL;
  for (const RElem &re : elem_array)
  {
    if (!re.e)
      continue;

    if (re.vertexData->isEmpty())
      continue;

    if (re.vertexData != vertexData)
    {
      vertexData = re.vertexData;
      //        debug("set VData: %X", vertexData);

      vertexData->setToDriver();
    }
    //      debug("sv=%d numv=%d si=%d numf=%d (ib=%X vb=%X)",
    //        re.sv, re.numv, re.si, re.numf,
    //        vertexData->getIB(), vertexData->getVB());

    re.render();
  }
}


// rendering
void ShaderMesh::renderRawImmediate(bool trans) const
{
  GlobalVertexData *vertexData = NULL;
  dag::Span<RElem> elemArray = trans ? getElems(STG_trans) : getElems(STG_opaque, STG_atest);
  for (const RElem &re : elemArray)
  {
    if (!re.e)
      continue;

    if (re.vertexData->isEmpty())
      continue;

    if (re.vertexData != vertexData)
    {
      vertexData = re.vertexData;
      vertexData->setToDriver();
    }

    d3d_err(re.drawIndTriList());
  }
}


void ShaderMesh::renderWithShader(const ShaderElement &shader_element, bool trans) const
{
  GlobalVertexData *vertexData = NULL;
  dag::Span<RElem> elemArray = trans ? getElems(STG_trans) : getElems(STG_opaque, STG_atest);
  for (const RElem &re : elemArray)
  {
    if (!re.e)
      continue;

    if (re.vertexData->isEmpty())
      continue;

    if (re.vertexData != vertexData)
    {
      vertexData = re.vertexData;
      vertexData->setToDriver();
    }

    re.renderWithElem(shader_element);
  }
}


void ShaderMesh::gatherUsedTex(TextureIdSet &tex_id_list) const
{
  for (int i = 0; i < elems.size(); i++)
    if (elems[i].mat)
      elems[i].mat->gatherUsedTex(tex_id_list);
}
void ShaderMesh::gatherUsedMat(Tab<ShaderMaterial *> &mat_list) const
{
  for (int i = 0; i < elems.size(); i++)
    if (elems[i].mat && find_value_idx(mat_list, elems[i].mat) < 0)
      mat_list.push_back(elems[i].mat);
}

// change texture by texture ID
bool ShaderMesh::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  bool replaced = false;

  int i;
  for (i = 0; i < elems.size(); i++)
    if (elems[i].mat && elems[i].mat->replaceTexture(tex_id_old, tex_id_new))
    {
      elems[i].e = elems[i].mat->make_elem(false, NULL);
      replaced = true;
    }

  return replaced;
}


void ShaderMesh::duplicateMaterial(TEXTUREID tex_id, dag::Span<RElem> elem, Tab<ShaderMaterial *> &old_mat,
  Tab<ShaderMaterial *> &new_mat)
{
  for (int i = 0; i < elem.size(); i++)
    if (find_value_idx(new_mat, elem[i].mat.get()) < 0 && (tex_id == BAD_TEXTUREID || elem[i].mat->replaceTexture(tex_id, tex_id)))
    {
      ShaderMaterial *mat = NULL;
      for (int l = 0; l < old_mat.size(); l++)
        if (old_mat[l] == elem[i].mat)
        {
          mat = new_mat[l];
          break;
        }

      if (!mat)
      {
        old_mat.push_back(elem[i].mat);
        mat = elem[i].mat->clone();
        new_mat.push_back(mat);
      }

      elem[i].mat = mat;
      elem[i].e = mat->make_elem(false, NULL);
    }
}

void ShaderMesh::duplicateMat(ShaderMaterial *prev_m, dag::Span<RElem> elem, Tab<ShaderMaterial *> &old_mat,
  Tab<ShaderMaterial *> &new_mat)
{
  G_ASSERT(find_value_idx(new_mat, prev_m) < 0);
  for (int i = 0; i < elem.size(); i++)
    if (elem[i].mat.get() == prev_m)
    {
      int idx = find_value_idx(old_mat, prev_m);
      ShaderMaterial *mat = idx < 0 ? NULL : new_mat[idx];
      if (!mat)
      {
        old_mat.push_back(prev_m);
        new_mat.push_back(mat = prev_m->clone());
      }

      elem[i].mat = mat;
      elem[i].e = mat->make_elem(false, NULL);
    }
}

void ShaderMesh::updateShaderElems()
{
  for (int i = 0; i < elems.size(); i++)
    if (elems[i].mat)
      elems[i].e = elems[i].mat->make_elem(false, NULL);
}
