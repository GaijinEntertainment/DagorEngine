// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <libTools/util/makeBindump.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <math/dag_mesh.h>
#include <3d/dag_render.h>
#include <generic/dag_tab.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_TMatrix4.h>
#include <debug/dag_debug.h>
#include <perfMon/dag_cpuFreq.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/vector.h>
#include <util/dag_hash.h>
#include <EASTL/fixed_vector.h>
#include <dag/dag_vector.h>

#define MAX_VERTEX_32 0x7FFFFFFF
#define MAX_INDEX_32  0x7FFFFFFF

#define MAX_VERTEX_16 65536
#define MAX_INDEX_16  0x7FFFFFFF

static constexpr int SCALE_DISCARD_THRESHOLD = 2000;

static __forceinline uint8_t real_to_uchar(real a)
{
  if (a <= 0)
    return 0;
  if (a >= 1)
    return 255;
  return real2int(a * 255);
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

IdentColorConvert IdentColorConvert::object;
unsigned ShaderMeshData::buildForTargetCode = _MAKE4C('PC');
bool ShaderMeshData::forceZlibPacking = false;
bool ShaderMeshData::preferZstdPacking = false;
bool ShaderMeshData::allowOodlePacking = false;
bool ShaderMeshData::fastNoPacking = false;
unsigned ShaderMeshData::zstdMaxWindowLog = 0;
int ShaderMeshData::zstdCompressionLevel = 18;


bool can_combine_elems(const ShaderMeshData::RElem &left, const ShaderMeshData::RElem &right, bool allow_32_bit,
  int additional_verices_num, int additional_faces_num)
{
  if (left.mat.get() != right.mat.get())
    return false;

  if (left.flags != right.flags)
    return false;

  if (!left.vertexData->checkChannels(right.vertexData->vDesc.data(), right.vertexData->vDesc.size()))
    return false;

  unsigned int maxVertex = allow_32_bit ? MAX_VERTEX_32 : MAX_VERTEX_16;
  unsigned int maxIndex = allow_32_bit ? MAX_INDEX_32 : MAX_INDEX_16;

  if (left.numv + additional_verices_num + right.numv > maxVertex)
    return false;

  if ((left.numf + additional_faces_num + right.numf) * 3 > maxIndex)
    return false;

  return true;
}

static void addVertices2(Mesh &m, int sf, int numf, const ShaderChannelId *vDescSrc, int channels_count, ShaderMeshData::RElem &re,
  IColorConvert &color_convert, bool allow_32_bit, int node_id);
static void addPointCloudVertices(Mesh &m, const ShaderChannelId *vDescSrc, int channels_count, ShaderMeshData::RElem &re,
  IColorConvert &converter, int node_id, bool allow_32_bit, int point_count, int point_offset);

static float calculateTextureScale(Mesh &m, int starf, int numf, const ShaderChannelId *vDescSrc, int channels_count);

/*********************************
 *
 * class GlobalVertexDataSrc
 *
 *********************************/

// ctor/dtor
GlobalVertexDataSrc::GlobalVertexDataSrc() :
  vDesc(tmpmem), stride(0), vData(tmpmem), iData(tmpmem), iData32(tmpmem), baseVertSegCount(0), numv(0), numf(0), partCount(0)
{}


GlobalVertexDataSrc::~GlobalVertexDataSrc() {}


// construct from channels
GlobalVertexDataSrc::GlobalVertexDataSrc(const CompiledShaderChannelId *ch, int count) :
  vDesc(tmpmem), stride(0), vData(tmpmem), iData(tmpmem), iData32(tmpmem), baseVertSegCount(0), numv(0), numf(0), partCount(0)
{
  // copy channels
  vDesc = make_span_const(ch, count);

  // calc stride
  for (int i = 0; i < vDesc.size(); ++i)
  {
    stride += shader_channel_type_size(vDesc[i].t);
  }

  //  debug("GlobalVertexDataSrc: n=%d (count=%d) stride=%d", vDesc.size(), count, stride);
  //  for (int k = 0; k < vDesc.size(); k++)
  //  {
  //    debug("%d: t=%X (%s) vbu=%d (%s) vbui=%d", k,
  //      ch[k].t, ShUtils::channel_type_name(ch[k].t),
  //      ch[k].vbu, ShUtils::channel_usage_name(ch[k].vbu),
  //      ch[k].vbui);
  //  }
}


// check channels for equality. return true, if channels are checked OK
bool GlobalVertexDataSrc::checkChannels(const CompiledShaderChannelId *ch, int count) const
{
  return (vDesc.size() == count) && (memcmp(&vDesc[0], ch, count * sizeof(CompiledShaderChannelId)) == 0);
}

// check for free space into vertex- & indexbuffers.
// return true, if can add secified count of vertices & indices
bool GlobalVertexDataSrc::checkFreeSpace(int add_numv, int add_numf, bool allow_32_bit) const
{
  if (allow_32_bit)
    return ((numv + add_numv) <= MAX_VERTEX_32) && ((numf + add_numf) * 3 <= MAX_INDEX_32);
  else
    return ((numv + add_numv) <= MAX_VERTEX_16) && ((numf + add_numf) * 3 <= MAX_INDEX_16);
  //  return ((numv + add_numv) * stride <= 0xFFFF) && ((numf + add_numf) * 3 <= 0xFFFF);
}

// check for attach other vertex data
bool GlobalVertexDataSrc::canAttachData(const GlobalVertexDataSrc &other_data, bool allow_32_bit, int _numv, int _numf) const
{
  if (!checkChannels(other_data.vDesc.data(), other_data.vDesc.size()))
    return false;
  if (!checkFreeSpace(_numv >= 0 ? _numv : other_data.numv, _numf >= 0 ? _numf : other_data.numf, allow_32_bit))
    return false;
  return true;
}

bool GlobalVertexDataSrc::canAttachData(const GlobalVertexDataSrc &other_data, bool allow_32_bit) const
{
  return canAttachData(other_data, allow_32_bit, -1, -1);
}

void GlobalVertexDataSrc::convertToIData32()
{
  if (!iData.size())
    return;

  iData32.resize(iData.size());
  for (int i = iData32.size(); --i >= 0;)
    iData32[i] = iData[i];
  clear_and_shrink(iData);
}

// attach data
void GlobalVertexDataSrc::attachData(const GlobalVertexDataSrc &other_data)
{
  // add vertices
  append_items(vData, other_data.vData.size(), other_data.vData.data());

  // add remaped indices
  if ((numv + other_data.numv) <= MAX_VERTEX_16)
  {
    int startIndex = append_items(iData, other_data.iData.size());
    for (int i = 0; i < other_data.iData.size(); i++)
      iData[startIndex + i] = other_data.iData[i] + numv;
  }
  else
  {
    convertToIData32();
    if (other_data.iData.size())
    {
      int startIndex = append_items(iData32, other_data.iData.size());
      for (int i = 0; i < other_data.iData.size(); i++)
        iData32[startIndex + i] = other_data.iData[i] + numv;
    }
    else
    {
      int startIndex = append_items(iData32, other_data.iData32.size());
      for (int i = 0; i < other_data.iData32.size(); i++)
        iData32[startIndex + i] = other_data.iData32[i] + numv;
    }
  }
  numv += other_data.numv;
  numf += other_data.numf;

  partCount += other_data.partCount;
}

void GlobalVertexDataSrc::attachDataPart(const GlobalVertexDataSrc &other_data, int _sv, int _numv, int _si, int _numf, int num_parts)
{
  // add vertices
  G_ASSERT(data_size(other_data.vData) >= stride * _numv + _sv);
  G_ASSERT(other_data.numf * 3 >= _numf * 3 + _si);

  if (other_data.iData.size())
  {
    G_ASSERT(other_data.iData.size() >= _numf * 3 + _si);
  }
  else if (other_data.iData32.size())
  {
    G_ASSERT(other_data.iData32.size() >= _numf * 3 + _si);
  }
  else
    DAG_FATAL("GlobalVertexDataSrc::iData and GlobalVertexDataSrc::iData32 are empty");

  append_items(vData, stride * _numv, &other_data.vData[stride * _sv]);

  // add remaped indices
  int indexOffset = numv - _sv;
  if ((numv + _numv) <= MAX_VERTEX_16)
  {
    int startIndex = append_items(iData, _numf * 3);
    if (other_data.iData.size())
      for (int i = 0; i < _numf * 3; i++)
        iData[startIndex + i] = (int)other_data.iData[i + _si] + indexOffset;
    else
      for (int i = 0; i < _numf * 3; i++)
        iData[startIndex + i] = (int)other_data.iData32[i + _si] + indexOffset;
  }
  else
  {
    convertToIData32();
    if (other_data.iData.size())
    {
      int startIndex = append_items(iData32, _numf * 3);
      for (int i = 0; i < _numf * 3; i++)
        iData32[startIndex + i] = (int)other_data.iData[i + _si] + indexOffset;
    }
    else
    {
      int startIndex = append_items(iData32, _numf * 3);
      for (int i = 0; i < _numf * 3; i++)
        iData32[startIndex + i] = (int)other_data.iData32[i + _si] + indexOffset;
    }
  }

  numv += _numv;
  numf += _numf;

  partCount += num_parts;
}

// class GlobalVertexDataSrc
//


/*********************************
 *
 * class ShaderMeshData
 *
 *********************************/
class GatherChanCB : public ShaderChannelsEnumCB
{
public:
  Tab<ShaderChannelId> chan;
  Tab<CompiledShaderChannelId> chanComp;

  GatherChanCB() : chan(tmpmem), chanComp(tmpmem) {}

  void enum_shader_channel(int u, int ui, int t, int vbu, int vbui, ChannelModifier mod, int stream) override
  {
    int i;
    for (i = 0; i < chan.size(); ++i)
      if (chan[i].u == u && chan[i].ui == ui && chan[i].t == t && chan[i].vbu == vbu && chan[i].vbui == vbui && (chan[i].mod == mod) &&
          chan[i].streamId == stream)
        return;
    i = append_items(chan, 1);
    append_items(chanComp, 1);

    chan[i].u = u;
    chan[i].ui = ui;
    chan[i].t = t;
    chan[i].vbu = vbu;
    chan[i].vbui = vbui;
    chan[i].mod = mod;
    chan[i].streamId = stream;

    chanComp[i].convertFrom(chan[i]);
    //    debug("%d %d %d - %d %d %d",
    //      chan[i].t, chan[i].vbu, chan[i].vbui, chanComp[i].t, chanComp[i].vbu, chanComp[i].vbui);
  }
};


// ctor/dtor
ShaderMeshData::ShaderMeshData() { memset(stageEndElemIdx, 0, sizeof(stageEndElemIdx)); }

ShaderMeshData::~ShaderMeshData() {}


// build meshdata
bool ShaderMeshData::build(class Mesh &m, ShaderMaterial **mats, int nummats, IColorConvert &color_convert, bool allow_32_bit,
  int node_id)
{
  // Calculate dU, dV (tangent, binormal).

  int chIdDu = m.find_extra_channel(SCUSAGE_EXTRA, 50);
  int chIdDv = m.find_extra_channel(SCUSAGE_EXTRA, 51);
  if (chIdDu < 0 || chIdDv < 0)
  {
    bool needDuDv = false;
    for (unsigned int materialNo = 0; materialNo < nummats; materialNo++)
    {
      GatherChanCB ccb;
      ccb.chan.clear();
      ccb.chanComp.clear();
      int flags;

      if (!mats[materialNo])
        continue;

      if (mats[materialNo]->enum_channels(ccb, flags))
      {
        for (unsigned int channelNo = 0; channelNo < ccb.chan.size(); channelNo++)
        {
          if (ccb.chan[channelNo].u == SCUSAGE_EXTRA && (ccb.chan[channelNo].ui == 50 || ccb.chan[channelNo].ui == 51))
          {
            needDuDv = true;
            goto Found;
          }
        }
      }
    }
  Found:;
    if (needDuDv)
    {
      if (!m.createTextureSpaceData(SCUSAGE_EXTRA, 50, 51))
      {
        DAG_FATAL("cannot create texture space data:\n"
                  "  tface[0].size()=%d face.size()=%d\n"
                  "  chIdDu=%d chIdDv=%d",
          m.getTFace(0).size(), m.getFace().size(), m.find_extra_channel(SCUSAGE_EXTRA, 50), m.find_extra_channel(SCUSAGE_EXTRA, 51));
        return false;
      }
    }
  }


  dag::ConstSpan<Face> face = m.getFace();

  // fix wrong matids
  m.clampMaterials(nummats - 1);

  // sort by matid
  G_VERIFY(m.sort_faces_by_mat());

  // gather channels into vertex groups
  GatherChanCB ccb;

  Tab<GlobalVertexDataSrc *> vDataRef(tmpmem);
  int atest_varId = VariableMap::getVariableId("atest", true);
  const auto addRElem = [&](ShaderMaterial *sm) -> ShaderMeshData::RElem * {
    if (sm)
    {
      ccb.chan.clear();
      ccb.chanComp.clear();
      int flags;
      if (sm->enum_channels(ccb, flags))
      {
        int stage_idx = (flags & SC_STAGE_IDX_MASK);
        int atest_val = -1;
        if (stage_idx == ShaderMesh::STG_opaque && sm->getIntVariable(atest_varId, atest_val) && atest_val > 0)
          stage_idx = ShaderMesh::STG_atest;

        RElem *re = addElem(stage_idx);
        re->flags = stage_idx | (re->flags & ~SC_STAGE_IDX_MASK);
        // add new vgroup & global buffer
        int vgrpId = append_items(vDataRef, 1);
        vDataRef[vgrpId] = new (tmpmem) GlobalVertexDataSrc(ccb.chanComp.data(), ccb.chanComp.size());

        re->vertexData = vDataRef[vgrpId];
        G_ASSERT(re->vertexData);

        re->vdOrderIndex = re->vertexData->partCount;
        re->vertexData->partCount++;

        re->mat = sm;
        return re;
      }
    }
    return nullptr;
  };
  for (int i = 0; i < face.size();)
  {
    int mi = face[i].mat;
    int nf;
    for (nf = i + 1; nf < face.size(); ++nf)
    {
      if (face[nf].mat != mi)
        break;

      if (!allow_32_bit && 3 * (nf - i + 1) >= MAX_VERTEX_16)
        break;
    }
    ShaderMaterial *sm = mats && nummats ? mats[mi] : 0;
    RElem *re = addRElem(sm);
    if (re)
    {
      int numf = nf - i;
      addVertices2(m, i, numf, ccb.chan.data(), ccb.chan.size(), *re, color_convert, allow_32_bit, node_id);
      re->textureScale = calculateTextureScale(m, i, numf, ccb.chan.data(), ccb.chan.size());
    }
    i = nf;
  }
  if (face.empty() && mats && nummats)
  {
    ShaderMaterial *sm = mats[0];
    if (sm && strstr(sm->getShaderClassName(), "plod"))
    {
      uint32_t pointCount = m.getVert().size();
      const uint32_t pointsPerElem = allow_32_bit ? MAX_VERTEX_32 / 4 : MAX_VERTEX_16 / 4; // 4 unique indices per quad
      for (uint32_t pointStart = 0; pointStart < pointCount; pointStart += pointsPerElem)
      {
        RElem *re = addRElem(sm);
        if (re)
        {
          const auto count = eastl::min(pointsPerElem, pointCount - pointStart);
          addPointCloudVertices(m, ccb.chan.data(), ccb.chan.size(), *re, color_convert, node_id, allow_32_bit, pointStart, count);
          re->textureScale = 1.0f;
          re->optDone = 1;
          re->vertexData->allowVertexMerge = false;
        }
      }
    }
  }
  return true;
}

template <class T>
T clamp_log(int &errors, T val, T min, T max)
{
  T val2 = clamp(val, min, max);
  if (val2 != val)
  {
    logwarn("chan value=%d doesn\'t fit to [%d,%d]", val, min, max);
    errors++;
  }
  return val2;
}

#define SWITCH_NAME(n) \
  case n: return #n;
static const char *usage_name(int i)
{
  switch (i)
  {
    SWITCH_NAME(SCUSAGE_POS)
    SWITCH_NAME(SCUSAGE_NORM)
    SWITCH_NAME(SCUSAGE_VCOL)
    SWITCH_NAME(SCUSAGE_TC)
    SWITCH_NAME(SCUSAGE_LIGHTMAP)
    SWITCH_NAME(SCUSAGE_EXTRA)
    default: return "unknown usage";
  }
}

static const char *type_name(int i)
{
  switch (i)
  {
    SWITCH_NAME(SCTYPE_FLOAT1)
    SWITCH_NAME(SCTYPE_FLOAT2)
    SWITCH_NAME(SCTYPE_FLOAT3)
    SWITCH_NAME(SCTYPE_FLOAT4)
    SWITCH_NAME(SCTYPE_E3DCOLOR)
    SWITCH_NAME(SCTYPE_UBYTE4)
    SWITCH_NAME(SCTYPE_SHORT2)
    SWITCH_NAME(SCTYPE_SHORT4)
    SWITCH_NAME(SCTYPE_HALF2)
    SWITCH_NAME(SCTYPE_HALF4)
    SWITCH_NAME(SCTYPE_SHORT2N)
    SWITCH_NAME(SCTYPE_SHORT4N)
    SWITCH_NAME(SCTYPE_USHORT2N)
    SWITCH_NAME(SCTYPE_USHORT4N)
    SWITCH_NAME(SCTYPE_UDEC3)
    SWITCH_NAME(SCTYPE_DEC3N)
    default: return "unknown type";
  }
}
#undef SWITCH_NAME

static int pos_chan_cvt_err = 0, chan_cvt_err = 0;

static __forceinline int convert_vertex(uint8_t *p, Color4 val, uint32_t mod, uint32_t type, const Color4 &mul, const Color4 &ofs,
  int node_idUse)
{
  static const Color4 C4ONE(1, 1, 1, 1);

  switch (mod)
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
    case CMOD_BOUNDING_PACK: val = val * mul + ofs; break;
  }
  int ce = 0;
  switch (type)
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
      p[3] = node_idUse != -1 ? node_idUse : real_to_uchar(val[3]);
      p += 4;
      break;
    case SCTYPE_UBYTE4:
      p[0] = real_to_uchar(val[0]);
      p[1] = real_to_uchar(val[1]);
      p[2] = real_to_uchar(val[2]);
      p[3] = node_idUse != -1 ? (uint8_t)node_idUse : real_to_uchar(val[3]);
      p += 4;
      break;
    case SCTYPE_SHORT2:
      *(short *)p = clamp_log(ce, real2int(val[0]), -32768, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[1]), -32768, 32767);
      p += 2;
      break;
    case SCTYPE_SHORT4:
      *(short *)p = clamp_log(ce, real2int(val[0]), -32768, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[1]), -32768, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[2]), -32768, 32767);
      p += 2;
      *(short *)p = node_idUse != -1 ? (int16_t)node_idUse : clamp_log(ce, real2int(val[3]), -32768, 32767);
      p += 2;
      break;
    case SCTYPE_HALF2:
      *(unsigned short *)p = float_to_half(val[0]);
      p += 2;
      *(unsigned short *)p = float_to_half(val[1]);
      p += 2;
      break;
    case SCTYPE_HALF4:
      *(unsigned short *)p = float_to_half(val[0]);
      p += 2;
      *(unsigned short *)p = float_to_half(val[1]);
      p += 2;
      *(unsigned short *)p = float_to_half(val[2]);
      p += 2;
      *(unsigned short *)p = node_idUse != -1 ? (uint16_t)node_idUse : float_to_half(val[3]);
      p += 2;
      break;
    case SCTYPE_USHORT2N:
      *(unsigned short *)p = clamp_log(ce, real2int(val[0] * 65535.0), 0, 65535);
      p += 2;
      *(unsigned short *)p = clamp_log(ce, real2int(val[1] * 65535.0), 0, 65535);
      p += 2;
      break;
    case SCTYPE_USHORT4N:
      *(unsigned short *)p = clamp_log(ce, real2int(val[0] * 65535.0), 0, 65535);
      p += 2;
      *(unsigned short *)p = clamp_log(ce, real2int(val[1] * 65535.0), 0, 65535);
      p += 2;
      *(unsigned short *)p = clamp_log(ce, real2int(val[2] * 65535.0), 0, 65535);
      p += 2;
      *(unsigned short *)p = node_idUse != -1 ? (uint16_t)node_idUse : clamp_log(ce, real2int(val[3] * 65535.0), 0, 65535);
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
      // pack to -32767..32767 shorts here (PC/XBOX360 format)
      *(short *)p = clamp_log(ce, real2int(val[0] * 32767.0f), -32767, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[1] * 32767.0f), -32767, 32767);
      p += 2;
      break;
    case SCTYPE_SHORT4N:
      // pack to -32767..32767 shorts here (PC/XBOX360 format)
      *(short *)p = clamp_log(ce, real2int(val[0] * 32767.0f), -32767, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[1] * 32767.0f), -32767, 32767);
      p += 2;
      *(short *)p = clamp_log(ce, real2int(val[2] * 32767.0f), -32767, 32767);
      p += 2;
      *(short *)p = node_idUse != -1 ? (int16_t)node_idUse : clamp_log(ce, real2int(val[3] * 32767.0f), -32767, 32767);
      p += 2;
      break;
    default: G_ASSERT(0);
  }
  return ce;
}

static void build_vertToFaceVertMap(F2V_Map &map, const int *vbuffer, int stride, int numf, int vnum, int vstride)
{
  int i, data_sz = 0;

  data_sz = numf * 3;

  map.data.clear();
  map.index.clear();
  map.data.resize(data_sz);
  map.index.resize(vnum * 2);
  memset(&map.index[0], 0, 4 * 2 * vnum);

  for (i = 0; i < numf; i++)
  {
    int v0, v1, v2;
    v0 = vbuffer[(i * 3 + 0) * stride] / vstride;
    v1 = vbuffer[(i * 3 + 1) * stride] / vstride;
    v2 = vbuffer[(i * 3 + 2) * stride] / vstride;
    map.index[v0]++;
    map.index[v1]++;
    map.index[v2]++;
  }

  data_sz = 0;
  for (i = 0; i < vnum; i++)
  {
    int inc = map.index[i];
    map.index[i] = data_sz;
    data_sz += inc;
  }
  for (i = 0; i < numf; ++i)
  {
    int v0, v1, v2;
    v0 = vbuffer[(i * 3 + 0) * stride] / vstride;
    v1 = vbuffer[(i * 3 + 1) * stride] / vstride;
    v2 = vbuffer[(i * 3 + 2) * stride] / vstride;

    map.data[map.index[v0] + map.index[v0 + vnum]] = i * 3 + 0;
    map.index[v0 + vnum]++;
    map.data[map.index[v1] + map.index[v1 + vnum]] = i * 3 + 1;
    map.index[v1 + vnum]++;
    map.data[map.index[v2] + map.index[v2 + vnum]] = i * 3 + 2;
    map.index[v2 + vnum]++;
  }

  map.index.resize(vnum);
}

struct Vdata16
{
  uint64_t v[2];
  bool operator==(const Vdata16 &a) const { return v[0] == a.v[0] && v[1] == a.v[1]; }
};
struct Vdata16Hasher
{
  size_t operator()(const Vdata16 &a) { return (a.v[0] * FNV1Params<64>::prime) ^ a.v[1]; }
};

struct ChannelVertices
{
  eastl::vector<uint8_t> encodedData; // encoded data - encodedData.size()/encodedSize == number of unique different vertices
  unsigned vertexOffset = 0;          // offset in vertex
  unsigned encodedSize = 0;           // size of a channel
  uint32_t uniqueVertsCount() const { return uint32_t(encodedData.size() / encodedSize); }
};

static void process_channel_data(Mesh &m, ShaderMeshData::RElem &re, const ShaderChannelId *desc, int channel, int node_id,
  int sc_type, int &count, int &type, const uint8_t *&vert_data, size_t &vert_stride, const uint8_t *&face_data, size_t &face_stride,
  int &node_id_use, bool &color_convert, bool need_face = true)
{
  switch (desc[channel].u)
  {
    case SCUSAGE_POS:
      if (desc[channel].ui != 0)
        DAG_FATAL("%s: chan[%d]: unknown pos channel %d", re.mat->getShaderClassName(), channel, desc[channel].ui);
      count = m.getVert().size();
      type = MeshData::CHT_FLOAT3;
      vert_data = (const uint8_t *)m.getVert().data();
      vert_stride = elem_size(m.getVert());
      if (need_face)
      {
        face_data = m.getFaceCount() ? (const uint8_t *)&m.getFace()[0].v[0] : nullptr;
        face_stride = elem_size(m.getFace());
      }
      break;
    case SCUSAGE_NORM:
      node_id_use = (sc_type == SCTYPE_SHORT4 || sc_type == SCTYPE_SHORT4N || sc_type == SCTYPE_USHORT4N)
                      ? (node_id % 32768)
                      : ((sc_type == SCTYPE_E3DCOLOR || sc_type == SCTYPE_UBYTE4)
                            ? (node_id % 256)
                            : (sc_type == SCTYPE_HALF4 ? float_to_half(node_id & 2048) : -1));
      count = m.getVertNorm().size();
      type = MeshData::CHT_FLOAT3;
      vert_data = (const uint8_t *)m.getVertNorm().data();
      vert_stride = elem_size(m.getVertNorm());
      if (need_face)
      {
        if (!m.getNormFace().size())
          DAG_FATAL("%s: no normals in mesh", re.mat->getShaderClassName());
        face_data = m.getNormFace().size() ? (const uint8_t *)&m.getNormFace()[0][0] : nullptr;
        face_stride = elem_size(m.getNormFace());
      }
      break;
    case SCUSAGE_VCOL:
      if (desc[channel].ui != 0)
        DAG_FATAL("%s: chan[%d]: unknown vcol channel %d", re.mat->getShaderClassName(), channel, desc[channel].ui);
      count = m.getCVert().size();
      type = MeshData::CHT_FLOAT4;
      color_convert = true;
      vert_data = (const uint8_t *)m.getCVert().data();
      vert_stride = elem_size(m.getCVert());
      if (need_face)
      {
        face_data = m.getCFace().size() ? (const uint8_t *)m.getCFace().data() : nullptr;
        face_stride = elem_size(m.getCFace());
      }
      break;
    case SCUSAGE_TC:
      if (desc[channel].ui < 0 || desc[channel].ui >= NUMMESHTCH)
        DAG_FATAL("%s: chan[%d]: unknown tc channel %d", re.mat->getShaderClassName(), channel, desc[channel].ui);
      count = m.getTVert(desc[channel].ui).size();
      type = MeshData::CHT_FLOAT2;
      vert_data = (const uint8_t *)m.getTVert(desc[channel].ui).data();
      vert_stride = elem_size(m.getTVert(desc[channel].ui));
      if (need_face)
      {
        face_data = m.getTFace(desc[channel].ui).size() ? (const uint8_t *)m.getTFace(desc[channel].ui).data() : nullptr;
        face_stride = elem_size(m.getTFace(desc[channel].ui));
      }
      break;
    default:
      DAG_FATAL("%s: unknown chan[%d] #%d (vbu=%d vbui=%d t=0x%x), check mesh vertices & vcolor", //
        re.mat->getShaderClassName(), channel, desc[channel].u, desc[channel].vbu, desc[channel].vbui, desc[channel].t);
  }
}

static float encode_vertex(const uint8_t *from, uint8_t to[16], int vert_type, int type, int mod, int node_id_use, bool color_convert,
  IColorConvert &converter, const Color4 &encode_ofs, const Color4 &encode_mul)
{
  const float *vertFloat = (const float *)from;
  Color4 val = {0, 0, 0, 1};
  switch (vert_type)
  {
    case MeshData::CHT_FLOAT4: val[3] = vertFloat[3]; // intentional fallthrough
    case MeshData::CHT_FLOAT3: val[2] = vertFloat[2]; // intentional fallthrough
    case MeshData::CHT_FLOAT2: val[1] = vertFloat[1]; // intentional fallthrough
    case MeshData::CHT_FLOAT1:
      val[0] = vertFloat[0]; // intentional fallthrough
      break;
    case MeshData::CHT_E3DCOLOR:
    {
      const E3DCOLOR &p = *(E3DCOLOR *)from;
      val[0] = p.r / 255.0f;
      val[1] = p.g / 255.0f;
      val[2] = p.b / 255.0f;
      val[3] = p.a / 255.0f;
    }
    break;
    default: G_ASSERT(0);
  }
  if (color_convert)
    val = converter.convert(val);
  return convert_vertex(to, val, mod, type, encode_mul, encode_ofs, node_id_use);
}

static void check_clamp_errors(float clamp_errors, const ShaderChannelId &desc, ShaderMeshData::RElem &re, int mod)
{
  if (clamp_errors)
  {
    chan_cvt_err += clamp_errors;
    if (desc.u == SCUSAGE_POS)
      pos_chan_cvt_err += clamp_errors;
    logmessage(desc.u == SCUSAGE_POS ? LOGLEVEL_ERR : LOGLEVEL_WARN,
      "shader <%s> channel type %s (usage %s[%d] pack=%d) has %d errors in packing", re.mat->getShaderClassName(), type_name(desc.t),
      usage_name(desc.u), desc.ui, mod, clamp_errors);
  }
}

// add vertices from mesh
static void addVertices2(Mesh &m, int sf, int numf, const ShaderChannelId *vDescSrc, int channels_count, ShaderMeshData::RElem &re,
  IColorConvert &color_convert, bool allow_32_bit, int node_id)
{
  eastl::vector<int> remapVerts; // remapped verts[3]
  eastl::vector<ChannelVertices> channels;
  channels.resize(channels_count);
  Tab<int> vBufferIndex;                          // each vertex is encoded as index to ChannelVertices[channel].encodedData.data()
  vBufferIndex.resize(numf * 3 * channels_count); // worst case scenario, no reusing
  unsigned vertexSize = 0;
  ska::flat_hash_map<uint32_t, uint32_t> map4Data;
  ska::flat_hash_map<uint64_t, uint32_t> map8Data;
  ska::flat_hash_map<Vdata16, uint32_t, Vdata16Hasher> map16Data;
  // int64_t reft = ref_time_ticks();

  for (int c = 0; c < channels_count; ++c)
  {
    map4Data.clear();
    map8Data.clear();
    map16Data.clear();
    int eci = m.find_extra_channel(vDescSrc[c].u, vDescSrc[c].ui);
    const uint32_t type = vDescSrc[c].t, mod = vDescSrc[c].mod;
    bool chan_norm = eci < 0 && vDescSrc[c].u == SCUSAGE_NORM;
    int node_idUse = -1;
    int vertCount = 0;
    const uint8_t *indices = nullptr;
    size_t faceStride = 0;
    const uint8_t *vertices = nullptr;
    size_t vertStride = 0;
    int vertType = MeshData::CHT_UNKNOWN;
    bool colorConvert = false;
    if (eci >= 0)
    {
      vertCount = m.getExtra(eci).vt.size();
      vertType = m.getExtra(eci).type;
      vertices = (const uint8_t *)m.getExtra(eci).vt.data();
      vertStride = m.getExtra(eci).vsize;
      indices = m.getExtra(eci).fc.size() ? (const uint8_t *)&m.getExtra(eci).fc[0].t[0] : nullptr;
      faceStride = elem_size(m.getExtra(eci).fc);
    }
    else
      process_channel_data(m, re, vDescSrc, c, node_id, type, vertCount, vertType, vertices, vertStride, indices, faceStride,
        node_idUse, colorConvert);
    Color4 encodeOfs(0, 0, 0, 0), encodeMul(1, 1, 1, 1);
    if (vDescSrc[c].mod == CMOD_BOUNDING_PACK)
    {
      if (vDescSrc[c].u == SCUSAGE_POS)
      {
        Point3 offset, mul;
        ShaderChannelId::getPosBounding(offset, mul);
        ShaderChannelId::setBoundingUsed();
        encodeOfs = Color4(offset.x, offset.y, offset.z, 0);
        encodeMul = Color4(mul.x, mul.y, mul.z, 1);
        encodeOfs *= encodeMul;
        if (channel_signed(vDescSrc[c].t))
        {
          encodeOfs = encodeOfs * 2 - Color4(1, 1, 1, 1);
          encodeMul = encodeMul * 2;
        }
      }
      else
        logerr("other channels except position do not support position bounding, yet");
    }
    ChannelVertices &cv = channels[c];
    if (!channel_size(vDescSrc[c].t, cv.encodedSize))
      DAG_FATAL("unknown shader channel type #%d (u=%d vbu=%d vbui=%d)", //
        vDescSrc[c].t, vDescSrc[c].u, vDescSrc[c].vbu, vDescSrc[c].vbui);
    remapVerts.clear();
    Color4 cval{0, 0, 0, 1};
    if (!indices)
    {
      vertCount = 1;
      vertices = (const uint8_t *)&cval;
      vertStride = sizeof(Color4);
    }
    remapVerts.resize(vertCount, -1);
    cv.encodedData.reserve(vertCount * cv.encodedSize);
    cv.vertexOffset = vertexSize;
    vertexSize += cv.encodedSize;
    int clampErrors = 0;
    auto *channelPtr = vBufferIndex.data() + c;
    for (int vfi = 0, vfiE = numf * 3; vfi < vfiE; ++vfi, channelPtr += channels_count)
    {
      const int fi = vfi / 3, fv = vfi % 3;
      const int *faceInd = indices ? (const int *)(indices + (sf + fi) * faceStride) : nullptr;
      const int orginalVertIndex = indices ? faceInd[fv] : 0;
      int encodedDataIndex = remapVerts[orginalVertIndex];
      if (encodedDataIndex < 0)
      {
        const uint8_t *cVert = vertices + vertStride * orginalVertIndex; //-V769
        alignas(16) uint8_t vertexData[16];
        clampErrors +=
          encode_vertex(cVert, vertexData, vertType, type, mod, node_idUse, colorConvert, color_convert, encodeOfs, encodeMul);
        if (type == SCTYPE_FLOAT1 || type == SCTYPE_FLOAT2 || type == SCTYPE_FLOAT3 || type == SCTYPE_FLOAT4)
        {
          // intentionally empty
          // makes no sense to look for equality, data is uncompressed, mesh.optimize(0) will get rid of duplicates anyway
        }
        if (cv.encodedSize == 4)
        {
          auto ins = map4Data.emplace(*(uint32_t *)vertexData, (uint32_t)cv.encodedData.size());
          if (!ins.second)
          {
            encodedDataIndex = ins.first->second;
            goto skip_adding;
          }
        }
        else if (cv.encodedSize == 8)
        {
          auto ins = map8Data.emplace(*(uint64_t *)vertexData, (uint32_t)cv.encodedData.size());
          if (!ins.second)
          {
            encodedDataIndex = ins.first->second;
            goto skip_adding;
          }
        }
        else if (cv.encodedSize == 16 || cv.encodedSize == 12)
        {
          if (cv.encodedSize == 12)
            ((int *)vertexData)[3] = 0;
          auto ins = map16Data.emplace(*(Vdata16 *)vertexData, (uint32_t)cv.encodedData.size());
          if (!ins.second)
          {
            encodedDataIndex = ins.first->second;
            goto skip_adding;
          }
        }
        else
        {
          for (const uint8_t *data = cv.encodedData.begin(), *end = cv.encodedData.end(); data != end; data += cv.encodedSize)
            if (memcmp(data, vertexData, cv.encodedSize) == 0)
            {
              encodedDataIndex = data - cv.encodedData.begin();
              goto skip_adding;
            }
        }
        // todo: find if exactly binary same channel
        encodedDataIndex = (int)cv.encodedData.size();
        cv.encodedData.insert(cv.encodedData.end(), vertexData, vertexData + cv.encodedSize);
      skip_adding:
        remapVerts[orginalVertIndex] = encodedDataIndex;
      }
      *channelPtr = encodedDataIndex;
    }
    check_clamp_errors(clampErrors, vDescSrc[c], re, mod);
  }
  // debug("step1 %dus", get_time_usec(reft));reft = ref_time_ticks();

  G_ASSERT(vertexSize == re.vertexData->stride);

  Tab<int> vertexBufferUniqueIndices;
  vertexBufferUniqueIndices.resize(vBufferIndex.size()); // worst case scenario, will be no reusing
  int *uniquePtr = vertexBufferUniqueIndices.begin();
  uint32_t uniqueTotal = 0;
  Tab<int> vertexRemapping; // each index is pointing to vBufferIndex[index*channels_count, (index+1)*channels_count]
  vertexRemapping.resize(numf * 3);

  {
    // find best channel to split
    int bestSplitChannel = 0, bestVerts = 0;
    for (int c = 0; c != channels_count; ++c)
    {
      ChannelVertices &cv = channels[c];
      if (bestVerts < channels[c].uniqueVertsCount())
        bestVerts = channels[bestSplitChannel = c].uniqueVertsCount();
    }
    VertToFaceVertMap f2vmap;
    build_vertToFaceVertMap(f2vmap, vBufferIndex.begin() + bestSplitChannel, channels_count, numf, bestVerts,
      channels[bestSplitChannel].encodedSize);

    int vertSize = channels_count * sizeof(int);
    for (int vi = 0; vi < bestVerts; ++vi)
    {
      const int nf = f2vmap.getNumFaces(vi);
      const uint32_t startUnique = uniqueTotal;
      for (int i = 0; i < nf; ++i)
      {
        int vfi = f2vmap.getVertFaceIndex(vi, i);
        const int *checkVertex = vBufferIndex.data() + vfi * channels_count;
        for (const int *check = uniquePtr + startUnique, *end = uniquePtr + uniqueTotal; check != end; check += channels_count)
        {
          if (memcmp(check, checkVertex, vertSize) == 0)
          {
            vertexRemapping[vfi] = (check - uniquePtr) / channels_count;
            goto continueOut;
          }
        }

        // debug("vfi = %d[%d](%d) uniqueTotal = %d(%d)", vfi/3, vfi%3, numf, uniqueTotal/channels_count,
        // vBufferIndex.size()/channels_count);
        vertexRemapping[vfi] = uniqueTotal / channels_count;
        memcpy(uniquePtr + uniqueTotal, checkVertex, vertSize);
        uniqueTotal += channels_count;
      continueOut:;
      }
    }
  }

  // debug("step2 %dus", get_time_usec(reft));reft = ref_time_ticks();
  vertexBufferUniqueIndices.resize(uniqueTotal); // worst case scenario, will be no reusing

  GlobalVertexDataSrc &vertexData = *re.vertexData;
  re.sv = vertexData.numv;
  re.numv = uniqueTotal / channels_count;
  re.numf = numf;
  vertexData.numf += numf;
  vertexData.numv += re.numv;

  auto append_indices = [&](auto &iData) {
    // prepare index data placement
    re.si = append_items(iData, numf * 3);
    auto *iBuf = &iData[re.si];
    for (int vi = 0, ve = numf * 3; vi < ve; vi++, ++iBuf)
      *iBuf = vertexRemapping[vi] + re.sv;
  };
  if (vertexData.numv <= MAX_VERTEX_16)
  {
    append_indices(vertexData.iData);
  }
  else
  {
    G_ASSERT(allow_32_bit);
    vertexData.convertToIData32();
    append_indices(vertexData.iData32);
  }
  // debug("step3 %dus", get_time_usec(reft));reft = ref_time_ticks();

  int ofs = append_items(vertexData.vData, re.numv * vertexData.stride);
  uint8_t *dest = &vertexData.vData[ofs];
  for (int channelNo = 0; channelNo < channels_count; ++channelNo)
  {
    const ChannelVertices &cv = channels[channelNo];
    const uint32_t channelStride = cv.encodedSize;
    uint8_t *channelDest = dest + cv.vertexOffset;
    for (auto *unique = vertexBufferUniqueIndices.data() + channelNo, *end = vertexBufferUniqueIndices.end(); unique < end;
         unique += channels_count, channelDest += vertexSize)
    {
      memcpy(channelDest, cv.encodedData.begin() + *unique, channelStride);
    }
  }
  // debug("step4 %dus", get_time_usec(reft));reft = ref_time_ticks();
}

static void addPointCloudVertices(Mesh &m, const ShaderChannelId *vDescSrc, int channels_count, ShaderMeshData::RElem &re,
  IColorConvert &converter, int node_id, bool allow_32_bit, int point_offset, int point_count)
{
  eastl::vector<ChannelVertices> channels;
  channels.resize(channels_count);
  unsigned vertexSize = 0;
  for (int c = 0; c < channels_count; ++c)
  {
    const uint32_t type = vDescSrc[c].t, mod = vDescSrc[c].mod;
    int node_idUse = -1;
    int vertCount = 0;
    const uint8_t *vertices = nullptr;
    const uint8_t *faces = nullptr;
    size_t vertStride = 0;
    size_t faceStride = 0;
    int vertType = MeshData::CHT_UNKNOWN;
    bool isColorConvert = false;
    process_channel_data(m, re, vDescSrc, c, node_id, type, vertCount, vertType, vertices, vertStride, faces, faceStride, node_idUse,
      isColorConvert, false);
    if (vDescSrc[c].u == SCUSAGE_TC)
      vertType = MeshData::CHT_E3DCOLOR;

    ChannelVertices &cv = channels[c];
    if (!channel_size(vDescSrc[c].t, cv.encodedSize))
      DAG_FATAL("unknown shader channel type #%d (u=%d vbu=%d vbui=%d)", vDescSrc[c].t, vDescSrc[c].u, vDescSrc[c].vbu,
        vDescSrc[c].vbui);
    cv.encodedData.reserve(vertCount * cv.encodedSize);
    cv.vertexOffset = vertexSize;
    vertexSize += cv.encodedSize;
    int clampErrors = 0;
    G_ASSERT(vertices);
    G_ASSERT(point_offset + point_count <= vertCount);
    for (int i = 0; i < point_count; ++i)
    {
      const uint8_t *cVert = vertices + vertStride * (i + point_offset); //-V769
      alignas(16) uint8_t vertexData[16];
      Color4 encodeOfs{0, 0, 0, 0};
      Color4 encodeMul{1, 1, 1, 1};
      clampErrors +=
        encode_vertex(cVert, vertexData, vertType, type, mod, node_idUse, isColorConvert, converter, encodeOfs, encodeMul);
      cv.encodedData.insert(cv.encodedData.end(), vertexData, vertexData + cv.encodedSize);
    }
    check_clamp_errors(clampErrors, vDescSrc[c], re, mod);
  }
  G_ASSERT(vertexSize == re.vertexData->stride);

  Tab<uint32_t> indices(tmpmem);
  indices.reserve(point_count * 6);
  for (uint32_t i = 0, j = 0; i < point_count; i++, j += 4)
  {
    indices.push_back(j);     //  i      j+1 -- j+3
    indices.push_back(j + 1); //  .  ->   |  \   |
    indices.push_back(j + 2); //          j  -- j+2
    indices.push_back(j + 2);
    indices.push_back(j + 1);
    indices.push_back(j + 3);
  }

  GlobalVertexDataSrc &vertexData = *re.vertexData;
  re.sv = vertexData.numv;
  re.numv = point_count;
  re.numf = point_count * 2; // one quad is a two triangle faces
  vertexData.numf += re.numf;
  vertexData.numv += re.numv;

  const auto append_indices = [&](auto &iData) {
    re.si = append_items(iData, re.numf * 3);
    auto *iBuf = &iData[re.si];
    for (int vi = 0, ve = re.numf * 3; vi < ve; vi++, ++iBuf)
      *iBuf = re.sv + indices[vi];
  };
  if (re.numv / 4 <= MAX_VERTEX_16) // one quad contains 4 unique indices
    append_indices(vertexData.iData);
  else
  {
    G_ASSERT(allow_32_bit);
    vertexData.convertToIData32();
    append_indices(vertexData.iData32);
  }

  int ofs = append_items(vertexData.vData, re.numv * vertexData.stride);
  uint8_t *dest = &vertexData.vData[ofs];
  for (int channelNo = 0; channelNo < channels.size(); ++channelNo)
  {
    const auto &cv = channels[channelNo];
    const uint32_t channelStride = cv.encodedSize;
    uint8_t *channelDest = dest + cv.vertexOffset;
    for (int i = 0; i < point_count; ++i, channelDest += re.vertexData->stride)
      memcpy(channelDest, cv.encodedData.begin() + cv.encodedSize * i, channelStride);
  }
}

static float calculateTextureScale(Mesh &m, int startf, int numf, const ShaderChannelId *vDescSrc, int channels_count)
{
  dag::ConstSpan<Point3> pos;
  Tab<dag::ConstSpan<Point2>> tc;
  dag::ConstSpan<Face> indicesPos;
  Tab<dag::ConstSpan<TFace>> indicesTc;

  for (int c = 0; c < channels_count; ++c)
  {
    int eci = m.find_extra_channel(vDescSrc[c].u, vDescSrc[c].ui);
    const uint32_t type = vDescSrc[c].t, mod = vDescSrc[c].mod;
    bool chan_norm = eci < 0 && vDescSrc[c].u == SCUSAGE_NORM;
    int vertType = MeshData::CHT_UNKNOWN;
    bool colorConvert = false;
    if (eci >= 0)
    {
      continue;
    }
    switch (vDescSrc[c].u)
    {
      case SCUSAGE_POS:
        pos = m.getVert();
        indicesPos = m.getFace();
        break;
      case SCUSAGE_TC:
        if (m.getTFace(vDescSrc[c].ui).size())
        {
          tc.push_back(m.getTVert(vDescSrc[c].ui));
          indicesTc.push_back(m.getTFace(vDescSrc[c].ui));
        }
        break;
      default: break;
    }
  }
  if (!pos.size() || !tc.size() || !indicesPos.size() || !indicesTc.size())
  {
    return FLT_MAX;
  }
  dag::Vector<float> scales;
  for (int tc_channel = 0; tc_channel < indicesTc.size(); tc_channel++)
  {
    if (!indicesTc[tc_channel].size() || !tc[tc_channel].size())
      continue;
    for (int face = startf; face < numf; face++)
    {
      const Face &fp = indicesPos[face];
      const TFace &ft = indicesTc[tc_channel][face];
      for (int v = 0; v < 3; v++)
      {
        int vsi = fp.v[v];
        int vei = fp.v[(v + 1) % 3];
        if (vsi == vei)
          continue;
        Point3 vs = pos[vsi];
        Point3 ve = pos[vei];
        int tsi = ft.t[v];
        int tei = ft.t[(v + 1) % 3];
        if (tsi == tei)
          continue;
        Point2 ts = tc[tc_channel][tsi];
        Point2 te = tc[tc_channel][tei];

        float vertexDelta = (vs - ve).length();
        if (vertexDelta < FLT_EPSILON)
          continue;

        float texDelta = (ts - te).length();
        float scale = texDelta / vertexDelta;

        // SCALE_DISCARD_THRESHOLD is an arbitrary number (2000 seems to be a sweet spot)
        // to check for outliers that shoot the avg waaaay up
        // (Most valid scales are below 1000)
        if (scale != 0.0 && !check_nan(scale) && scale < SCALE_DISCARD_THRESHOLD)
          scales.push_back(scale);
      }
    }
  }
  if (scales.empty())
    return FLT_MAX;
  double avg = 0.0;
  for (float scale : scales)
    avg += scale;
  avg /= scales.size();
  return avg;
}

void ShaderMeshData::reset_channel_cvt_errors() { pos_chan_cvt_err = chan_cvt_err = 0; }
int ShaderMeshData::get_channel_cvt_errors() { return chan_cvt_err; }
int ShaderMeshData::get_channel_cvt_critical_errors() { return pos_chan_cvt_err; }

// save
void ShaderMeshData::RElem::save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb)
{
  cwr.writePtr64e(0);
  cwr.writePtr64e(mdcb.getmatindex(mat));
  cwr.writePtr64e(mdcb.getGlobVData(vertexData));

  cwr.writeInt32e(vdOrderIndex);
  cwr.writeInt32e(sv);
  cwr.writeInt32e(numv);
  cwr.writeInt32e(si);
  cwr.writeInt32e(numf);
  cwr.writeInt32e(bv);
}

int ShaderMeshData::save(mkbindump::BinDumpSaveCB &cwr, ShaderMeshDataSaveCB &mdcb, bool new_orig)
{
  int start = cwr.tell(), pos;
  bool new_format = elems.size() > getElems(ShaderMesh::STG_opaque).size() + getElems(ShaderMesh::STG_trans).size(); //==

  if (new_orig)
    cwr.setOrigin();

  pos = cwr.tell();
  if (new_format)
  {
    cwr.writeRef(0, elems.size());
    cwr.write16ex(stageEndElemIdx, sizeof(stageEndElemIdx));
    G_ASSERTF(cwr.tell() - pos == 16 * 2, "cwr.tell()-pos=%d", cwr.tell() - pos); // to be binary-compatible with older format
  }
  else
  {
    cwr.writeRef(0, getElems(ShaderMesh::STG_opaque).size());
    cwr.writeRef(0, getElems(ShaderMesh::STG_trans).size());
  }
  cwr.writeInt32e(new_format ? 0x80000001 : 1);
  cwr.writeInt32e(0);

  if (new_format)
  {
    cwr.writePtr32eAt(cwr.tell(), pos);
    for (int i = 0; i < elems.size(); ++i)
      elems[i].save(cwr, mdcb);
  }
  else
  {
    dag::Span<RElem> elem = getElems(ShaderMesh::STG_opaque);
    dag::Span<RElem> telem = getElems(ShaderMesh::STG_trans);

    cwr.writePtr32eAt(cwr.tell(), pos);
    for (int i = 0; i < elem.size(); ++i)
      elem[i].save(cwr, mdcb);

    cwr.writePtr32eAt(cwr.tell(), pos + cwr.TAB_SZ);
    for (int i = 0; i < telem.size(); ++i)
      telem[i].save(cwr, mdcb);
  }

  if (new_orig)
    cwr.popOrigin();

  return cwr.tell() - start;
}

ShaderMesh *ShaderMeshData::makeShaderMesh(const PtrTab<GlobalVertexDataSrc> vdata, dag::Span<GlobalVertexData> builtVdata)
{
  size_t sz = sizeof(ShaderMesh) + sizeof(ShaderMesh::RElem) * (elems.size());
  void *mem = memalloc(sz, midmem);
  ShaderMesh *sm = new (mem, _NEW_INPLACE) ShaderMesh;

  sm->elems.init((char *)mem + sizeof(ShaderMesh), elems.size());

  mem_set_0(sm->elems);
  memcpy(sm->stageEndElemIdx, stageEndElemIdx, sizeof(sm->stageEndElemIdx));
  for (int i = 0; i < elems.size(); i++)
  {
    const ShaderMeshData::RElem &de = elems[i];
    ShaderMesh::RElem &re = sm->elems[i];

    re.mat = de.mat;
    re.e = re.mat->make_elem("makeShaderMesh:elem");
    re.sv = de.sv;
    re.numv = de.numv;
    re.si = de.si;
    re.numf = de.numf;
    re.vdOrderIndex = de.vdOrderIndex;

    re.vertexData = NULL;
    for (int j = 0; j < vdata.size(); j++)
      if (de.vertexData.get() == vdata[j].get())
      {
        re.vertexData = &builtVdata[j];
        break;
      }
    G_ASSERT(re.vertexData && "vdata -> buildVdata not resolved?");
  }

  return sm;
}

// change texture by texture ID
bool ShaderMeshData::replaceTexture(TEXTUREID tex_id_old, TEXTUREID tex_id_new)
{
  int i;
  bool replaced = false;

  for (i = 0; i < elems.size(); i++)
    if (elems[i].mat != NULL)
      replaced |= elems[i].mat->replaceTexture(tex_id_old, tex_id_new);

  return replaced;
}


// replace one matrial with other. no channel checking performed
void ShaderMeshData::replaceMaterial(ShaderMaterial *exists_mat, ShaderMaterial *new_mat)
{
  int i;
  for (i = 0; i < elems.size(); i++)
    if (elems[i].mat == exists_mat)
      elems[i].mat = new_mat;
}


// add all used vertex data to CB
void ShaderMeshData::enumVertexData(ShaderMeshDataSaveCB &mdcb) const
{
  for (int i = 0; i < elems.size(); i++)
    mdcb.addGlobVData(elems[i].vertexData);
}

bool ShaderMeshData::canAttachDataElem(const Tab<RElem> &elem, const Tab<RElem> &other_elem, bool allow_32_bit) const
{
  for (int i = 0; i < other_elem.size(); i++)
  {
    const RElem &otherElem = other_elem[i];

    // find render element with same material & vertex data
    for (int j = 0; j < elem.size(); j++)
    {
      const RElem &thisElem = elem[j];
      if ((thisElem.mat.get() == otherElem.mat.get()) && (thisElem.flags == otherElem.flags) &&
          thisElem.vertexData->canAttachData(*otherElem.vertexData, allow_32_bit, otherElem.numv, otherElem.numf))
      {
        return true;
      }
    }
  }

  return false;
}


// check for attach other mesh data
bool ShaderMeshData::canAttachData(const ShaderMeshData &other_data, bool allow_32_bit) const
{
  return canAttachDataElem(elems, other_data.elems, allow_32_bit);
}


// attach data. mix elements from current & other mesh datas.
void ShaderMeshData::attachData(const ShaderMeshData &other_data, bool allow_32_bit)
{
  AddRElems(elems, other_data.elems, allow_32_bit);

  //  debug("");
}

struct AttachedRElems
{
  Tab<const ShaderMeshData::RElem *> others;
  int numv;
  int numf;
  inline AttachedRElems() : others(tmpmem), numv(0), numf(0) {}
};

// add elems
void ShaderMeshData::AddRElems(Tab<RElem> &elem, const Tab<RElem> &other_elem, bool allow_32_bit)
{
  Tab<AttachedRElems> elemMap(tmpmem);
  elemMap.resize(elem.size());

  int i;

  for (i = 0; i < elem.size(); i++)
  {
    elemMap[i].numv = elem[i].numv;
    elemMap[i].numf = elem[i].numf;
  }

  // search equal relems
  for (i = 0; i < other_elem.size(); i++)
  {
    const RElem &otherElem = other_elem[i];

    // find render element with same material & vertex data & free space
    int j;
    for (j = 0; j < elem.size(); j++)
    {
      const RElem &thisElem = elem[j];
      AttachedRElems &eMap = elemMap[j];

      if (can_combine_elems(thisElem, otherElem, allow_32_bit, eMap.numv - thisElem.numv, eMap.numf - thisElem.numf))
      {
        eMap.others.push_back(&otherElem);
        eMap.numv += otherElem.numv;
        eMap.numf += otherElem.numf;
        break;
      }
    }

    if (j >= elem.size())
    {
      // not found - add new element
      RElem &thisElem = elem.push_back();
      AttachedRElems &eMap = elemMap.push_back();
      G_ASSERT(elem.size() == elemMap.size());

      thisElem.mat = otherElem.mat;
      thisElem.flags = otherElem.flags;
      thisElem.vertexData = otherElem.vertexData;
      thisElem.vertexDataId = -1;
      thisElem.vdOrderIndex = -1;
      thisElem.si = otherElem.si;
      thisElem.sv = otherElem.sv;
      eMap.numv = thisElem.numv = otherElem.numv;
      eMap.numf = thisElem.numf = otherElem.numf;
    }
  }

  // make & fill vdatas
  Tab<GlobalVertexDataSrc *> vDataRef(tmpmem);

  G_ASSERT(elem.size() == elemMap.size());

  for (i = 0; i < elem.size(); i++)
  {
    RElem &thisElem = elem[i];
    AttachedRElems &eMap = elemMap[i];
    const GlobalVertexDataSrc &srcVd = *thisElem.vertexData;

    // find channels
    int vgrpId;
    for (vgrpId = 0; vgrpId < vDataRef.size(); ++vgrpId)
    {
      GlobalVertexDataSrc &vd = *vDataRef[vgrpId];

      if (vd.checkChannels(srcVd.vDesc.data(), srcVd.vDesc.size()) && vd.checkFreeSpace(eMap.numv, eMap.numf, allow_32_bit))
        break;
    }

    // add new vgroup & global buffer
    if (vgrpId >= vDataRef.size())
    {
      vgrpId = append_items(vDataRef, 1);
      vDataRef[vgrpId] = new (tmpmem) GlobalVertexDataSrc(srcVd.vDesc.data(), srcVd.vDesc.size());
    }

    // move data to new vbuffer
    const int oldsv = thisElem.sv;
    const int oldsi = thisElem.si;
    const int oldnumv = thisElem.numv;
    const int oldnumf = thisElem.numf;

    GlobalVertexDataSrc &dstVd = *vDataRef[vgrpId];

    thisElem.sv = dstVd.numv;
    thisElem.si = dstVd.iData.size() ? dstVd.iData.size() : dstVd.iData32.size();
    thisElem.numv = eMap.numv;
    thisElem.numf = eMap.numf;
    thisElem.vdOrderIndex = dstVd.partCount;

    dstVd.attachDataPart(srcVd, oldsv, oldnumv, oldsi, oldnumf, 1);

    for (int j = 0; j < eMap.others.size(); j++)
    {
      const RElem &otherElem = *eMap.others[j];
      G_ASSERT(thisElem.mat.get() == otherElem.mat.get());
      dstVd.attachDataPart(*otherElem.vertexData, otherElem.sv, otherElem.numv, otherElem.si, otherElem.numf, 0);
    }

    thisElem.vertexData = vDataRef[vgrpId];
  }

  for (i = 0; i < vDataRef.size(); ++i)
  {
    vDataRef[i]->vData.shrink_to_fit();
    vDataRef[i]->iData.shrink_to_fit();
  }
}

// class ShaderMeshData


void create_vertex_color_data(Mesh &m, int usage, int usage_index)
{
  bool has_alpha = m.tface[7].size() != 0;
  if (!m.cface.size())
    return;
  int chId = m.find_extra_channel(usage, usage_index);
  if (chId >= 0)
    return;

  chId = m.add_extra_channel(Mesh::CHT_FLOAT4, usage, usage_index);
  if (chId < 0)
    return;

  Mesh::ExtraChannel &vcolChan = m.extra[chId];
  const int vertCount = m.face.size() * 3;

  vcolChan.resize_verts(vertCount);
  vcolChan.fc.resize(m.face.size());

  dag::Span<TFace> vcolface = make_span(vcolChan.fc);
  Point4 *vcolvert = (Point4 *)vcolChan.vt.data();

  for (int i = 0; i < vcolface.size(); i++)
    for (int j = 0; j < 3; j++)
    {
      vcolface[i].t[j] = i * 3 + j;
      vcolvert[i * 3 + j].set(m.cvert[m.cface[i].t[j]].r, m.cvert[m.cface[i].t[j]].g, m.cvert[m.cface[i].t[j]].b,
        has_alpha ? m.tvert[7][m.tface[7][i].t[j]].x : 1.0f);
    }

  m.optimize_extra(chId, 0);
}

void add_per_vertex_domain_uv(Mesh &m, int usage, int usage_index_0, int usage_index_1)
{
  static constexpr float VERTEX_POS_DIFF_TOLERANCE_SQ = 1e-20f;

  if (!m.tface[0].size())
    return;
  if (m.face.size() != m.tface[0].size())
    return;

  int chId0 = m.add_extra_channel(Mesh::CHT_FLOAT4, usage, usage_index_0);
  if (chId0 < 0)
    return;
  int chId1 = m.add_extra_channel(Mesh::CHT_FLOAT4, usage, usage_index_1);
  if (chId1 < 0)
    return;

  const int vertCount = m.face.size() * 3;

  Mesh::ExtraChannel &vcolChan0 = m.extra[chId0];
  vcolChan0.resize_verts(vertCount);
  vcolChan0.fc.resize(m.face.size());

  Mesh::ExtraChannel &vcolChan1 = m.extra[chId1];
  vcolChan1.resize_verts(vertCount);
  vcolChan1.fc.resize(m.face.size());

  dag::Span<TFace> vcolface0 = make_span(vcolChan0.fc);
  Point4 *vcolvert0 = (Point4 *)vcolChan0.vt.data();

  dag::Span<TFace> vcolface1 = make_span(vcolChan1.fc);
  Point4 *vcolvert1 = (Point4 *)vcolChan1.vt.data();

  struct DomVertex
  {
    Point3 vertex;
    Point2 uv;
  };

  Tab<DomVertex> domVertices;

  auto get_unique_vertex_id = [&domVertices](const Point3 &vertex) -> int {
    int uniqueVertId = 0;
    for (; uniqueVertId < domVertices.size(); ++uniqueVertId)
      if ((vertex - domVertices[uniqueVertId].vertex).lengthSq() < VERTEX_POS_DIFF_TOLERANCE_SQ)
        break;
    return uniqueVertId;
  };

  Tab<int> uniqueVertexIndices;
  uniqueVertexIndices.resize(vertCount);
  for (int faceId = 0; faceId < m.face.size(); ++faceId)
  {
    const Face face = m.face[faceId];
    const TFace tface = m.tface[0][faceId];
    for (int inFaceVertId = 0; inFaceVertId < 3; ++inFaceVertId)
    {
      int flatIndex = 3 * faceId + inFaceVertId;
      Point3 vertex = m.vert[face.v[inFaceVertId]];
      Point2 uv = m.tvert[0][tface.t[inFaceVertId]];
      int uniqueVertId = get_unique_vertex_id(vertex);
      if (uniqueVertId == domVertices.size())
      {
        DomVertex domVertex;
        domVertex.vertex = vertex;
        domVertex.uv = uv;
        domVertices.push_back(domVertex);
      }
      uniqueVertexIndices[flatIndex] = uniqueVertId;
    }
  }

  struct DomEdge
  {
    int uniqueIndex0;
    Point2 uv0;
    Point2 uv1;
  };

  auto makeEdgeKey = [](int vertexId0, int vertexId1) -> uint64_t {
    return ((uint64_t)min(vertexId0, vertexId1) << 32) | (uint64_t)max(vertexId0, vertexId1);
  };

  ska::flat_hash_map<uint64_t, DomEdge> domEdgeMap;

  for (int faceId = 0; faceId < m.face.size(); ++faceId)
  {
    const Face face = m.face[faceId];
    const TFace tface = m.tface[0][faceId];

    for (int inFaceVertId = 0; inFaceVertId < 3; ++inFaceVertId)
    {
      int flatIndex = 3 * faceId + inFaceVertId;

      int i0 = (inFaceVertId + 1) % 3;
      int i1 = (inFaceVertId + 2) % 3;

      Point3 vertex0 = m.vert[face.v[i0]];
      Point3 vertex1 = m.vert[face.v[i1]];
      Point2 uv0 = m.tvert[0][tface.t[i0]];
      Point2 uv1 = m.tvert[0][tface.t[i1]];

      int uniqueIndex0 = uniqueVertexIndices[3 * faceId + i0];
      int uniqueIndex1 = uniqueVertexIndices[3 * faceId + i1];

      uint64_t edgeId = makeEdgeKey(uniqueIndex0, uniqueIndex1);

      DomEdge domEdge;
      auto edgeIt = domEdgeMap.find(edgeId);
      if (edgeIt != domEdgeMap.end())
      {
        domEdge = edgeIt->second;
      }
      else
      {
        domEdge.uniqueIndex0 = uniqueIndex0;
        domEdge.uv0 = uv0;
        domEdge.uv1 = uv1;
        domEdgeMap[edgeId] = domEdge;
      }

      Point2 domVertexUv = domVertices[uniqueVertexIndices[flatIndex]].uv;
      Point4 domEdgeUv = domEdge.uniqueIndex0 == uniqueIndex0 ? Point4(domEdge.uv0.x, domEdge.uv0.y, domEdge.uv1.x, domEdge.uv1.y)
                                                              : Point4(domEdge.uv1.x, domEdge.uv1.y, domEdge.uv0.x, domEdge.uv0.y);

      vcolface0[faceId].t[inFaceVertId] = flatIndex;
      vcolface1[faceId].t[inFaceVertId] = flatIndex;

      vcolvert0[flatIndex] = Point4(domVertexUv.x, domVertexUv.y, 0, 0);
      vcolvert1[flatIndex] = domEdgeUv;
    }
  }

  m.optimize_extra(chId0, 0);
  m.optimize_extra(chId1, 0);
}
