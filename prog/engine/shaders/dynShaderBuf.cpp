#include <shaders/dag_dynShaderBuf.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_quadIndexBuffer.h>

//
// Dynamic VB/IB for mesh rendering with arbitrary shader
//
void DynShaderMeshBuf::setRingBuf(RingDynamicVB *_vb, RingDynamicIB *_ib)
{
  G_ASSERT(stride == 0 || (_vb && _vb->getStride() == stride));

  if (vb)
    vb->delRef();
  if (ib)
    ib->delRef();

  vb = _vb;
  ib = _ib;

  if (vb)
    vb->addRef();
  if (ib)
    ib->addRef();
}

void DynShaderMeshBuf::init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_v, int max_f)
{
  G_ASSERT(ch.data() && ch.size());
  currentShader = NULL;
  vUsed = iUsed = 0;

  vdecl = dynrender::addShaderVdecl(ch.data(), ch.size());
  stride = dynrender::getStride(ch.data(), ch.size());

  G_ASSERT(vb && ib);
  G_ASSERT(stride == vb->getStride());
  clear_and_resize(vBuf, max_v * stride);
  clear_and_resize(iBuf, max_f * 3);
}
void DynShaderMeshBuf::initCopy(const DynShaderMeshBuf &b)
{
  currentShader = NULL;
  vUsed = iUsed = 0;

  vdecl = b.vdecl;
  stride = b.stride;

  G_ASSERT(vb && ib);
  G_ASSERT(stride == vb->getStride());
  clear_and_resize(vBuf, b.vBuf.size());
  clear_and_resize(iBuf, b.iBuf.size());
}
void DynShaderMeshBuf::close()
{
  if (vb)
    vb->delRef();
  if (ib)
    ib->delRef();
  vb = NULL;
  ib = NULL;

  clear_and_shrink(vBuf);
  clear_and_shrink(iBuf);
  vdecl = BAD_VDECL;
  stride = 0;
  currentShader = NULL;
  vUsed = iUsed = 0;
}
bool DynShaderMeshBuf::fillRawFaces(void **__restrict vdata, int num_v, uint16_t **__restrict ind, int num_f, int &vbase)
{
  if (!stride)
    return false;

  G_ASSERT(num_v && num_f);

  int vdata_sz = num_v * stride;
  if (vdata_sz > data_size(vBuf))
    fatal("too many verts: %d, max=%u", num_v, unsigned(data_size(vBuf) / stride));
  if (num_f * 3 > iBuf.size())
    fatal("too many faces: %d, max=%d", num_f, iBuf.size() / 3);

  if ((vUsed + num_v) * stride > data_size(vBuf) || iUsed + num_f * 3 > iBuf.size())
    flush();

  *vdata = vBuf.data() + vUsed * stride;
  *ind = iBuf.data() + iUsed;
  vbase = vUsed;

  vUsed += num_v;
  iUsed += num_f * 3;
  return true;
}
void DynShaderMeshBuf::addFaces(const void *__restrict v_data, int num_v, const uint16_t *__restrict i_data, int num_f)
{
  void *vp;
  uint16_t *ip;
  int vbase;

  if (fillRawFaces(&vp, num_v, &ip, num_f, vbase))
  {
    memcpy(vp, v_data, num_v * stride);
    if (!vUsed)
      memcpy(ip, i_data, num_f * 3 * elem_size(iBuf));
    else
      for (uint16_t *end = ip + num_f * 3; ip < end; ip++, i_data++)
        *ip = *i_data + vbase;
  }
}

void DynShaderMeshBuf::flush(bool clear_used)
{
  if (!vUsed || !iUsed)
    return;

  if (vb->bufLeft() < vUsed || ib->bufLeft() < iUsed)
  {
    vb->resetPos();
    ib->resetPos();
  }

  last_start_v = vb->addData(vBuf.data(), vUsed);
  last_start_i = ib->addData(iBuf.data(), iUsed);
  if (last_start_v < 0 || last_start_i < 0)
    fatal("start_v=%d start_i=%d, when add faces (%d vert, %d ind); buf size=%d, %d", last_start_v, last_start_i, vUsed, iUsed,
      vb->bufSize(), ib->bufSize());

  render(vUsed, iUsed);
  if (clear_used)
    clear();
}

void DynShaderMeshBuf::render(int vertices, int indices)
{
  if (!vertices || !indices)
    return;
  if (!currentShader || !stride)
    return;

  currentShader->setStates(0, true);

  d3d_err(d3d::setind(ib->getBuf()));
  d3d_err(d3d::setvsrc(0, vb->getBuf(), stride));
  d3d_err(d3d::drawind(PRIM_TRILIST, last_start_i, indices / 3, last_start_v));
}


//
// Dynamic VB for quad list rendering with arbitrary shader
//
static constexpr int IB_MAX_QUAD = 8192;

void DynShaderQuadBuf::setRingBuf(RingDynamicVB *_vb)
{
  G_ASSERT(qStride == 0 || (_vb && _vb->getStride() * 4 == qStride));

  if (vb)
    vb->delRef();

  vb = _vb;

  if (vb)
    vb->addRef();

  if (vb)
    index_buffer::init_quads_16bit();
  else
    index_buffer::release_quads_16bit();
}

void DynShaderQuadBuf::init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_q)
{
  G_ASSERT(ch.data() && ch.size());
  currentShader = NULL;
  qUsed = 0;

  vdecl = dynrender::addShaderVdecl(ch.data(), ch.size());
  qStride = dynrender::getStride(ch.data(), ch.size()) * 4;

  G_ASSERT(vb);
  G_ASSERT(qStride == vb->getStride() * 4);
  clear_and_resize(vBuf, max_q * qStride);
}
void DynShaderQuadBuf::initCopy(const DynShaderQuadBuf &b)
{
  currentShader = NULL;
  qUsed = 0;

  vdecl = b.vdecl;
  qStride = b.qStride;

  G_ASSERT(vb);
  G_ASSERT(qStride == vb->getStride() * 4);
  clear_and_resize(vBuf, b.vBuf.size());
}
void DynShaderQuadBuf::close()
{
  if (vb)
    vb->delRef();
  vb = NULL;
  index_buffer::release_quads_16bit();

  clear_and_shrink(vBuf);
  vdecl = BAD_VDECL;
  qStride = 0;
  currentShader = NULL;
  qUsed = 0;
}
bool DynShaderQuadBuf::fillRawQuads(void **vdata, int num_q)
{
  if (!qStride)
    return false;

  G_ASSERT(num_q);

  int vdata_sz = num_q * qStride;
  if (vdata_sz > data_size(vBuf))
    fatal("too many quads: %d, max=%u", num_q, unsigned(data_size(vBuf) / qStride));

  if ((qUsed + num_q) * qStride > data_size(vBuf))
    flush();

  *vdata = vBuf.data() + qUsed * qStride;

  qUsed += num_q;
  return true;
}
void DynShaderQuadBuf::addQuads(const void *v_data, int num_q)
{
  void *vp;

  if (fillRawQuads(&vp, num_q))
    memcpy(vp, v_data, num_q * qStride);
}

void *DynShaderQuadBuf::lockData(int max_quad_num)
{
  if (max_quad_num == 0)
    return NULL;

  if (vb->bufLeft() < max_quad_num * 4)
    vb->resetPos();

  return vb->lockData(max_quad_num * 4);
}

void DynShaderQuadBuf::unlockDataAndFlush(int quad_num)
{
  if (!currentShader || !qStride || quad_num == 0)
    return;
  currentShader->setStates(0, true);
  qUsed = 0;
  int start_v = vb->getPos();
  vb->unlockData(quad_num * 4);
  d3d_err(d3d::setvsrc(0, vb->getBuf(), qStride / 4));
  while (quad_num > IB_MAX_QUAD)
  {
    index_buffer::use_quads_16bit();
    d3d_err(d3d::drawind(PRIM_TRILIST, 0, IB_MAX_QUAD * 2, start_v));
    start_v += IB_MAX_QUAD * 4;
    quad_num -= IB_MAX_QUAD;
  }
  index_buffer::use_quads_16bit();
  d3d_err(d3d::drawind(PRIM_TRILIST, 0, quad_num * 2, start_v));
}

void DynShaderQuadBuf::flush()
{
  if (!qUsed)
    return;
  if (!currentShader || !qStride)
    return;

  if (!currentShader->setStates(0, true))
    return;

  if (vb->bufLeft() < qUsed * 4)
    vb->resetPos();

  int start_v = vb->addData(vBuf.data(), qUsed * 4);
  if (start_v < 0)
    fatal("start_v=%d when %d vertices; buf size=%d", start_v, qUsed * 4, vb->bufSize());

#define D3D_SAFE(c)                                                          \
  {                                                                          \
    bool res = (c);                                                          \
    G_ANALYSIS_ASSUME(res);                                                  \
    if (!res)                                                                \
    {                                                                        \
      debug("%s:Driver3d error: \n%s", __FUNCTION__, d3d::get_last_error()); \
      goto err;                                                              \
    }                                                                        \
  }

  {
    d3d::setvsrc(0, vb->getBuf(), qStride / 4);
    while (qUsed > IB_MAX_QUAD)
    {
      index_buffer::use_quads_16bit();
      // d3d::setind(quadIB);
      D3D_SAFE(d3d::drawind(PRIM_TRILIST, 0, IB_MAX_QUAD * 2, start_v));
      start_v += IB_MAX_QUAD * 4;
      qUsed -= IB_MAX_QUAD;
    }
    // d3d::setind(quadIB);
    index_buffer::use_quads_16bit();
    D3D_SAFE(d3d::drawind(PRIM_TRILIST, 0, qUsed * 2, start_v));
  }

err:

  qUsed = 0;

#undef D3D_SAFE
}

//
// Dynamic VB for tristrip rendering with arbitrary shader
//
void DynShaderStripBuf::setRingBuf(RingDynamicVB *_vb)
{
  G_ASSERT(pStride == 0 || (_vb && _vb->getStride() * 2 == pStride));

  if (vb)
    vb->delRef();
  vb = _vb;
  if (vb)
    vb->addRef();
}

void DynShaderStripBuf::init(dag::ConstSpan<CompiledShaderChannelId> ch, int max_pair_num)
{
  G_ASSERT(ch.data() && ch.size());
  currentShader = NULL;
  pUsed = 0;

  vdecl = dynrender::addShaderVdecl(ch.data(), ch.size());
  pStride = dynrender::getStride(ch.data(), ch.size()) * 2;

  G_ASSERT(vb);
  G_ASSERT(pStride == vb->getStride() * 2);
  clear_and_resize(vBuf, max_pair_num * pStride);
}
void DynShaderStripBuf::initCopy(const DynShaderStripBuf &b)
{
  currentShader = NULL;
  pUsed = 0;

  vdecl = b.vdecl;
  pStride = b.pStride;

  G_ASSERT(vb);
  G_ASSERT(pStride == vb->getStride() * 2);
  clear_and_resize(vBuf, b.vBuf.size());
}
void DynShaderStripBuf::close()
{
  if (vb)
    vb->delRef();
  vb = NULL;

  clear_and_shrink(vBuf);
  vdecl = BAD_VDECL;
  pStride = 0;
  currentShader = NULL;
  pUsed = 0;
}
bool DynShaderStripBuf::fillRawVertPairs(void **vdata, int v_pair_num)
{
  if (!pStride)
    return false;

  G_ASSERT(v_pair_num);

  int vdata_sz = v_pair_num * pStride;
  if (vdata_sz > data_size(vBuf))
    fatal("too many vert pairs: %d, max=%u", v_pair_num, unsigned(data_size(vBuf) / pStride));

  if ((pUsed + v_pair_num) * pStride > data_size(vBuf))
    flush();

  *vdata = vBuf.data() + pUsed * pStride;

  pUsed += v_pair_num;
  return true;
}
void DynShaderStripBuf::addVertPairs(const void *v_data, int v_pair_num, bool detached)
{
  char *vp;
  if (detached && pUsed && (pUsed + v_pair_num) * pStride <= data_size(vBuf))
  {
    void *last = &vBuf[pUsed * pStride - pStride / 2];
    if (!fillRawVertPairsEx(&vp, v_pair_num + 1))
      return;
    memcpy(vp, last, pStride / 2);
    vp += pStride / 2;
    memcpy(vp, v_data, pStride / 2);
    vp += pStride / 2;
  }
  else if (!fillRawVertPairsEx(&vp, v_pair_num))
    return;

  memcpy(vp, v_data, v_pair_num * pStride);
}

void DynShaderStripBuf::flush()
{
  if (!pUsed)
    return;
  if (!currentShader || !pStride)
    return;

  currentShader->setStates(0, true);

  if (vb->bufLeft() < pUsed * 2)
    vb->resetPos();

  int start_v = vb->addData(vBuf.data(), pUsed * 2);
  if (start_v < 0)
    fatal("start_v=%d when %d vertices; buf size=%d", start_v, pUsed * 4, vb->bufSize());

  d3d_err(d3d::setvsrc(0, vb->getBuf(), pStride / 2));
  d3d_err(d3d::draw(PRIM_TRISTRIP, start_v, (pUsed - 1) * 2));

  pUsed = 0;
}
