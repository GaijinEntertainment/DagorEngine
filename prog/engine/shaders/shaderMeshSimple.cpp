#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaders.h>
#include <math/dag_mesh.h>
#include <math/dag_mathBase.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <generic/dag_smallTab.h>
#include "shadersBinaryData.h"


static Point3 encode_mul(1, 1, 1), encode_offset(0, 0, 0);
static bool pos_bounding_used = false;
void ShaderChannelId::setPosBounding(const Point3 &offset, const Point3 &mul)
{
  encode_offset = offset;
  encode_mul = mul;
}
void ShaderChannelId::getPosBounding(Point3 &offset, Point3 &mul)
{
  offset = encode_offset;
  mul = encode_mul;
}
void ShaderChannelId::setBoundingUsed() { pos_bounding_used = true; }
bool ShaderChannelId::getBoundingUsed() { return pos_bounding_used; }
void ShaderChannelId::resetBoundingUsed() { pos_bounding_used = false; }

static __forceinline uint8_t real_to_uchar(real a)
{
  if (a <= 0)
    return 0;
  if (a >= 1)
    return 255;
  return real2int(a * 255);
}

static void appendIndexData(const Mesh &m, int sf, int numf, VertToFaceVertMap &f2vmap, const ShaderChannelId *vDescSrc,
  Tab<int> &bvert, Tab<int> &cvert, uint16_t *iBuf, int &append_vertex_count)
{
  const int ef = sf + numf;
  //  debug("sf=%d ef=%d (total faces=%d)", sf, ef, m.face.size());

  for (int vi = 0; vi < m.getVert().size(); ++vi)
  {
    int lv = bvert.size();
    int lbv = append_vertex_count;

    int nf = f2vmap.getNumFaces(vi);

    for (int i = 0; i < nf; ++i)
    {
      int fi = f2vmap.getVertFaceIndex(vi, i);

      int fv = fi % 3;
      fi /= 3;

      if (fi < sf || fi >= ef)
        continue;

      // get indices
      for (int c = 0; c < cvert.size(); ++c)
      {
        int ind = -1;
        int eci = m.find_extra_channel(vDescSrc[c].u, vDescSrc[c].ui);
        if (eci >= 0)
        {
          // this is extra cnannel
          if (m.getExtra(eci).fc.size())
            ind = m.getExtra(eci).fc[fi].t[fv];
        }
        else
          switch (vDescSrc[c].u)
          {
            // this is standart cnannel
            case SCUSAGE_POS:
              if (vDescSrc[c].ui != 0)
                DAG_FATAL("unknown pos channel %d", vDescSrc[c].ui);
              ind = vi;
              break;
            case SCUSAGE_NORM:
              if (vDescSrc[c].ui != 0)
                DAG_FATAL("unknown norm channel %d", vDescSrc[c].ui);
              if (!m.getNormFace().size())
                DAG_FATAL("no normals in mesh");
              ind = m.getNormFace()[fi][fv];
              break;
            case SCUSAGE_VCOL:
              if (vDescSrc[c].ui != 0)
                DAG_FATAL("unknown vcol channel %d", vDescSrc[c].ui);
              if (m.getCFace().size())
                ind = m.getCFace()[fi].t[fv];
              break;
            case SCUSAGE_TC:
              if (vDescSrc[c].ui >= NUMMESHTCH)
                DAG_FATAL("unknown tc channel %d", vDescSrc[c].ui);
              if (m.getTFace(vDescSrc[c].ui).size())
                ind = m.getTFace(vDescSrc[c].ui)[fi].t[fv];
              break;
            default: DAG_FATAL("unknown shader channel #%d", vDescSrc[c].u);
          }
        cvert[c] = ind;
      }

      // find the same vertex
      int id, bi;
      for (id = lv, bi = lbv; id < bvert.size(); id += cvert.size(), ++bi)
        if (mem_eq(cvert, &bvert[id]))
          break;

      if (id >= bvert.size())
      {
        append_items(bvert, cvert.size(), &cvert[0]);
        bi = append_vertex_count++;
      }

      iBuf[(fi - sf) * 3 + fv] = bi;
    }
  }
}

static unsigned real2int10(real a)
{
  int ret = real2int(a);
  if (ret >= 0)
    return ((unsigned)ret) & ((1 << 9) - 1);
  else
  {
    return ((unsigned)ret & ((1 << 9) - 1)) | (1 << 10);
  }
}

static unsigned real2uint(real a) { return real2int(a < 0 ? 0 : a); }

static int addVertices(Mesh &m, int sf, int numf, VertToFaceVertMap &f2vmap, uint8_t *vb, uint16_t *ib,
  const ShaderChannelId *vDescSrc, int chan_count)
{
  Tab<int> bvert(tmpmem);
  Tab<int> cvert(tmpmem);
  Color4 val;
  int ind;

  bvert.reserve(numf * 3 * chan_count);
  cvert.resize(chan_count);

  int nv = 0;
  appendIndexData(m, sf, numf, f2vmap, vDescSrc, bvert, cvert, ib, nv);
  G_ASSERT(nv <= 65545);

  // prepare vertex data placement
  uint8_t *p = vb;
  Point3 offset, mul;
  ShaderChannelId::getPosBounding(offset, mul);
  Color4 encode_offset_, encode_mul_, encode_offset__signed, encode_mul__signed;
  encode_offset_ = Color4(offset.x, offset.y, offset.z, 0);
  encode_mul_ = Color4(mul.x, mul.y, mul.z, 1);
  encode_offset_ *= encode_mul_;
  encode_offset__signed = encode_offset_ * 2 - Color4(1, 1, 1, 1);
  encode_mul__signed = encode_mul_ * 2;

  // add vertices to vg
#define SETV(x, y, z, w) \
  {                      \
    val[0] = (x);        \
    val[1] = (y);        \
    val[2] = (z);        \
    val[3] = (w);        \
  }
  for (int i = 0; i < bvert.size();)
    for (int c = 0; c < cvert.size(); ++c, ++i)
    {
      // get vertex data
      ind = bvert[i];
      if (ind < 0)
        SETV(0, 0, 0, 1)
      else
      {
        int eci = m.find_extra_channel(vDescSrc[c].u, vDescSrc[c].ui);
        if (eci >= 0)
        {
          const uint8_t *vp = &m.getExtra(eci).vt[m.getExtra(eci).vsize * ind];
          float *fp = (float *)vp;
          E3DCOLOR &ep = *(E3DCOLOR *)vp;

          switch (m.getExtra(eci).type)
          {
            case MeshData::CHT_FLOAT1: SETV(fp[0], 0, 0, 1); break;
            case MeshData::CHT_FLOAT2: SETV(fp[0], fp[1], 0, 1); break;
            case MeshData::CHT_FLOAT3: SETV(fp[0], fp[1], fp[2], 1); break;
            case MeshData::CHT_FLOAT4: SETV(fp[0], fp[1], fp[2], fp[3]); break;
            case MeshData::CHT_E3DCOLOR: SETV(ep.r / 255.0f, ep.g / 255.0f, ep.b / 255.0f, ep.a / 255.0f); break;
          }
        }
        else
          switch (vDescSrc[c].u)
          {
            case SCUSAGE_POS: SETV(m.getVert()[ind].x, m.getVert()[ind].y, m.getVert()[ind].z, 1); break;
            case SCUSAGE_NORM: SETV(m.getVertNorm()[ind].x, m.getVertNorm()[ind].y, m.getVertNorm()[ind].z, 0); break;
            case SCUSAGE_VCOL: SETV(m.getCVert()[ind].r, m.getCVert()[ind].g, m.getCVert()[ind].b, m.getCVert()[ind].a); break;
            case SCUSAGE_TC: SETV(m.getTVert(vDescSrc[c].ui)[ind].x, m.getTVert(vDescSrc[c].ui)[ind].y, 0, 1); break;
            default:
              DAG_FATAL("unknown shader channel #%d (vbu=%d vbui=%d t=%d)", vDescSrc[c].u, vDescSrc[c].vbu, vDescSrc[c].vbui,
                vDescSrc[c].t);
          }
      }

      // apply channel modifier
      static const Color4 C4ONE(1, 1, 1, 1);

      switch (vDescSrc[c].mod)
      {
        case CMOD_SIGNED_PACK: // convert from [0..1] to [-1..1]
          val = val * 2 - C4ONE;
          break;

        case CMOD_UNSIGNED_PACK: // convert from [-1..1] to [0..1]
          val = (val + C4ONE) / 2;
          break;

        case CMOD_MUL_1K: val *= (1 << 10); break;
        case CMOD_MUL_2K: val *= (1 << 11); break;
        case CMOD_MUL_4K: val *= (1 << 12); break;
        case CMOD_MUL_8K: val *= (1 << 13); break;
        case CMOD_MUL_16K: val *= (1 << 14); break;
        case CMOD_SIGNED_SHORT_PACK:
          val = clamp(val * 32767, Color4(-32767, -32767, -32767, -32767), Color4(32767, 32767, 32767, 32767));
          break;
        case CMOD_BOUNDING_PACK:
          if (vDescSrc[c].u == SCUSAGE_POS)
          {
            if (channel_signed(vDescSrc[c].t))
              val = val * encode_mul__signed + encode_offset__signed;
            else
              val = val * encode_mul_ + encode_offset_;
            ShaderChannelId::setBoundingUsed();
          }
          break;
        case CMOD_NONE: break;
      }

      switch (vDescSrc[c].t)
      {
        case SCTYPE_FLOAT1:
          *(float *)p = val[0];
          p += 4;
          break;
        case SCTYPE_FLOAT2:
          *(float *)p = val[0];
          p += 4;
          *(float *)p = val[1];
          p += 4;
          break;
        case SCTYPE_FLOAT3:
          *(float *)p = val[0];
          p += 4;
          *(float *)p = val[1];
          p += 4;
          *(float *)p = val[2];
          p += 4;
          break;
        case SCTYPE_FLOAT4:
          *(float *)p = val[0];
          p += 4;
          *(float *)p = val[1];
          p += 4;
          *(float *)p = val[2];
          p += 4;
          *(float *)p = val[3];
          p += 4;
          break;
        case SCTYPE_E3DCOLOR:
          p[0] = real_to_uchar(val[2]);
          p[1] = real_to_uchar(val[1]);
          p[2] = real_to_uchar(val[0]);
          p[3] = real_to_uchar(val[3]);
          p += 4;
          break;
        case SCTYPE_UBYTE4:
          p[0] = real_to_uchar(val[0]);
          p[1] = real_to_uchar(val[1]);
          p[2] = real_to_uchar(val[2]);
          p[3] = real_to_uchar(val[3]);
          p += 4;
          break;
        case SCTYPE_SHORT2:
          *(short *)p = real2int(val[0]);
          p += 2;
          *(short *)p = real2int(val[1]);
          p += 2;
          break;
        case SCTYPE_SHORT4:
          *(short *)p = real2int(val[0]);
          p += 2;
          *(short *)p = real2int(val[1]);
          p += 2;
          *(short *)p = real2int(val[2]);
          p += 2;
          *(short *)p = real2int(val[3]);
          p += 2;
          break;
        case SCTYPE_HALF2:
          *(uint16_t *)p = float_to_half(val[0]);
          p += 2;
          *(uint16_t *)p = float_to_half(val[1]);
          p += 2;
          break;
        case SCTYPE_HALF4:
          *(uint16_t *)p = float_to_half(val[0]);
          p += 2;
          *(uint16_t *)p = float_to_half(val[1]);
          p += 2;
          *(uint16_t *)p = float_to_half(val[2]);
          p += 2;
          *(uint16_t *)p = float_to_half(val[3]);
          p += 2;
          break;
        case SCTYPE_USHORT2N:
          *(unsigned short *)p = real2int(val[0] * 65535.0);
          p += 2;
          *(unsigned short *)p = real2int(val[1] * 65535.0);
          p += 2;
          break;
        case SCTYPE_USHORT4N:
          *(unsigned short *)p = real2int(val[0] * 65535.0);
          p += 2;
          *(unsigned short *)p = real2int(val[1] * 65535.0);
          p += 2;
          *(unsigned short *)p = real2int(val[2] * 65535.0);
          p += 2;
          *(unsigned short *)p = real2int(val[3] * 65535.0);
          p += 2;
          break;
        case SCTYPE_DEC3N:
          *(unsigned int *)p = (real2int10(val[0] * 511.0)) | (real2int10(val[1] * 511.0) << 10) | (real2int10(val[2] * 511.0) << 20);
          p += 4;
          break;
        case SCTYPE_UDEC3:
          *(unsigned int *)p = (real2uint(val[0])) | (real2uint(val[1]) << 10) | (real2uint(val[2]) << 20);
          p += 4;
          break;
        case SCTYPE_SHORT2N:
          *(short *)p = real2int(val[0] * 32767.0);
          p += 2;
          *(short *)p = real2int(val[1] * 32767.0);
          p += 2;
          break;
        case SCTYPE_SHORT4N:
          *(short *)p = real2int(val[0] * 32767.0);
          p += 2;
          *(short *)p = real2int(val[1] * 32767.0);
          p += 2;
          *(short *)p = real2int(val[2] * 32767.0);
          p += 2;
          *(short *)p = real2int(val[3] * 32767.0);
          p += 2;
          break;
        default:
          DAG_FATAL("unknown shader channel type #%d (u=%d vbu=%d vbui=%d)", vDescSrc[c].t, vDescSrc[c].u, vDescSrc[c].vbu,
            vDescSrc[c].vbui);
      }
      // debug("ch[%d]= %g %g %g %g",c,val[0],val[1],val[2],val[3]);
    }

#undef SETV

  return nv;
}

// build simple single-material mesh with it own vertex/index buffers
ShaderMesh *ShaderMesh::createSimple(Mesh &m, ShaderMaterial *mat, const char * /*info_str*/)
{
  G_ASSERT(mat);

  class GatherChanCB : public ShaderChannelsEnumCB
  {
  public:
    Tab<ShaderChannelId> chan;
    Tab<CompiledShaderChannelId> chanComp;

    GatherChanCB() : chan(tmpmem), chanComp(tmpmem) {}

    void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int) override
    {
      int i;
      for (i = 0; i < chan.size(); ++i)
        if (chan[i].u == u && chan[i].ui == ui && chan[i].t == t && chan[i].vbu == vbu && chan[i].vbui == vbui && (chan[i].mod == mod))
          return;

      i = append_items(chan, 1);
      append_items(chanComp, 1);

      chan[i].u = u;
      chan[i].ui = ui;
      chan[i].t = t;
      chan[i].vbu = vbu;
      chan[i].vbui = vbui;
      chan[i].mod = mod;

      chanComp[i].convertFrom(chan[i]);
    }
  } ccb;

  Tab<uint8_t> vb(tmpmem);
  SmallTab<uint16_t, TmpmemAlloc> ib;
  int stride = 0, flags;

  if (!mat->enum_channels(ccb, flags))
  {
    G_ASSERT(0 && "can't enum material channels");
  }

  for (int i = 0; i < ccb.chan.size(); ++i)
    stride += shader_channel_type_size(ccb.chan[i].t);

  void *mem = memalloc(sizeof(ShaderMesh) + sizeof(RElem) + sizeof(GlobalVertexData), midmem);
  ShaderMesh *sm = new (mem, _NEW_INPLACE) ShaderMesh;
  sm->_resv = 0;

  if (m.getFace().empty())
  {
    sm->elems.init(nullptr, 0);
    memset(sm->stageEndElemIdx, 0, sizeof(sm->stageEndElemIdx));
    return sm;
  }
  sm->elems.init(sm + 1, 1);
  memset(sm->stageEndElemIdx, 0, sizeof(sm->stageEndElemIdx));
  for (int i = 0, stg = (flags & SC_STAGE_IDX_MASK); i < SC_STAGE_IDX_MASK + 1; i++)
    sm->stageEndElemIdx[i] = (i < stg) ? 0 : 1;

  RElem *re = sm->elems.data();
  memset(re, 0, sizeof(*re));

  re->mat = mat;
  re->e = re->mat->make_elem();
  re->vertexData = (GlobalVertexData *)((char *)mem + sizeof(ShaderMesh) + sizeof(RElem));
  re->vdOrderIndex = -7;

  VertToFaceVertMap f2vmap;
  m.buildVertToFaceVertMap(f2vmap);

  vb.reserve(stride * m.getFace().size() * 3);
  clear_and_resize(ib, m.getFace().size() * 3);

  re->si = 0;
  re->numf = m.getFace().size();
  re->sv = 0;
  re->baseVertex = 0;
  re->numv = addVertices(m, 0, m.getFace().size(), f2vmap, vb.data(), ib.data(), ccb.chan.data(), ccb.chan.size());

  re->vertexData->initGvdMem(re->numv, stride, data_size(ib), 0, vb.data(), ib.data());

  return sm;
}
