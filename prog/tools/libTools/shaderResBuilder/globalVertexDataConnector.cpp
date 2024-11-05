// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <libTools/shaderResBuilder/globalVertexDataConnector.h>
#include <libTools/shaderResBuilder/shaderMeshData.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderCommon.h>
#include <shaders/dag_shaderMesh.h>
#include <math/dag_Point3.h>
#include <generic/dag_tabUtils.h>
#include <EASTL/hash_map.h>
#include <hash/mum_hash.h>
// #include <debug/dag_debug.h>

// #define ENABLE_LOG_CONNECT_MESHES
#if defined(ENABLE_LOG_CONNECT_MESHES)
#include <debug/dag_debug.h>
#define DEBUGCM debug
#else
__forceinline bool DEBUGCM_F(...) { return false; };
#define DEBUGCM 0 && DEBUGCM_F
#endif

bool GlobalVertexDataConnector::allowVertexMerge = false;
bool GlobalVertexDataConnector::allowBaseVertex = false;

struct VDataRemap
{
  GlobalVertexDataSrc *data;
  int sv;             // vertex origin in new vertex data
  int si;             // index origin in new vertex data
  int baseOrderIndex; // new base index of the vertex data

  VDataRemap() : data(NULL), si(0), sv(0), baseOrderIndex(0) {}
  VDataRemap(GlobalVertexDataSrc *_ni, int _sv, int _si, int _bi) : data(_ni), sv(_sv), si(_si), baseOrderIndex(_bi) {}
};

// distance between 2 spheres
static inline float opt_2cr(const Point4 &cr1, const Point4 &cr2)
{
  return length(Point3(cr1.x, cr1.y, cr1.z) - Point3(cr2.x, cr2.y, cr2.z)) - cr1.w - cr2.w;
}

// reorders object strip in obj so that spatialy near objects lie near in strip
static void alignObjStrip(Tab<int> &obj, Point4 *cr, Tab<int> &tmp)
{
  if (obj.size() < 3)
    return;

  tmp.clear();
  tmp.reserve(obj.size());

  int cur1 = obj.back(), cur2 = cur1;

  obj.pop_back();
  tmp.push_back(cur1);

  for (int i = obj.size() - 1; i >= 0; i--)
  {
    int best_idx = 0, best_type = +1;
    float best_opt = opt_2cr(cr[cur2], cr[obj[best_idx]]);
    float opt2;

    if (cur1 != cur2)
    {
      opt2 = opt_2cr(cr[cur1], cr[obj[best_idx]]);
      if (opt2 < best_opt)
      {
        best_type = -1;
        best_opt = opt2;
      }
    }

    for (int j = obj.size() - 1; j > 0; j--)
    {
      opt2 = opt_2cr(cr[cur2], cr[obj[j]]);
      if (opt2 < best_opt)
      {
        best_idx = j;
        best_type = +1;
        best_opt = opt2;
      }
    }

    if (cur1 != cur2)
    {
      for (int j = obj.size() - 1; j > 0; j--)
      {
        opt2 = opt_2cr(cr[cur1], cr[obj[j]]);
        if (opt2 < best_opt)
        {
          best_idx = j;
          best_type = -1;
          best_opt = opt2;
        }
      }
    }

    if (best_type == +1)
    {
      cur2 = obj[best_idx];
      tmp.push_back(cur2);
    }
    else
    {
      cur1 = obj[best_idx];
      insert_item_at(tmp, 0, cur1);
    }
    erase_items(obj, best_idx, 1);
  }

  obj.push_back(tmp.back());
  append_items(obj, tmp.size() - 1, tmp.data());
}
/*
static void writeStrip ( int idx, const char *txt, dag::Span<int> strip, Point4 *cr )
{
  debug ( "[%d] %s(%d):", idx, txt, strip.size());
  if ( strip.size() < 2)
    return;

  debug_ ( "  %d -%.2f-", strip[0], opt_2cr(cr[strip[0]], cr[strip.back()]));
  for ( int i = strip.size()-1; i > 0 ; i -- )
    if ( i > 1 )
      debug_ ( " %d -%.2f-", strip[i], opt_2cr(cr[strip[i]], cr[strip[i-1]]));
    else
      debug ( " %d", strip[i] );
}
*/

void GlobalVertexDataConnector::addVData(GlobalVertexDataSrc *vd, ShaderMaterial *mat, const Point4 &cr)
{
  G_ASSERT(vd);

  int di;
  for (di = 0; di < vdataSrc.size(); ++di)
    if (vdataSrc[di] == vd)
      break;

  if (di >= vdataSrc.size())
  {
    di = vdataSrc.size();
    vdataSrc.push_back(vd);
    int mi = srcMat.size();
    srcMat.push_back(mat);
    G_ASSERT(mi == di);
    mi = srcCr.size();
    srcCr.push_back(cr);
    G_ASSERT(mi == di);
  }

  // if (srcMat[di]!=mat)
  //   srcMat[di]=NULL;
}


// add meshdata to connect it's vertex datas & update sv/si indices
void GlobalVertexDataConnector::addMeshData(ShaderMeshData *md, const Point3 &c, real r)
{
  if (!md)
    return;

  if (!tabutils::addUnique(meshes, md))
    return;

  for (int ei = 0; ei < md->elems.size(); ++ei)
    addVData(md->elems[ei].vertexData, md->elems[ei].mat, Point4(c.x, c.y, c.z, r));
}


class Tabint : public Tab<int>
{
public:
  Tabint() : Tab<int>(tmpmem) {}
};

// make connect
void GlobalVertexDataConnector::connectData(bool allow_32_bit, GlobalVertexDataConnector::UpdateInfoInterface *update_info,
  unsigned gdata_flg)
{
  G_ASSERT(srcMat.size() == vdataSrc.size());

  // remap from src to dest vdatas
  Tab<VDataRemap> vDataMap(tmpmem);
  Tab<GlobalVertexDataSrc *> vdataSrcWork(tmpmem);
  Tab<Tabint> eqMatStripIdx(tmpmem);
  Tab<int> vDataEmsiStart(tmpmem);

  DEBUGCM("start connecting %d datas", vdataSrc.size());
  if (update_info)
    update_info->beginConnect(vdataSrc.size());

  vDataMap.resize(vdataSrc.size());
  vdataSrcWork.resize(vdataSrc.size());
  vDataEmsiStart.resize(vdataSrc.size());
  clear_and_shrink(vdataDest);
  int i;
  for (i = 0; i < vDataMap.size(); i++)
  {
    vdataSrcWork[i] = vdataSrc[i];
    vDataMap[i].data = vdataSrc[i];
    vDataMap[i].sv = vDataMap[i].si = vDataMap[i].baseOrderIndex = 0;
    DEBUGCM("%d: index count=%d vertex count=%d", i, vdataSrc[i]->iData.size(), vdataSrc[i]->numv);
  }

  // check vertex datas for connect & connect it

  // build per material strips of data
  for (i = 0; i < vdataSrcWork.size(); i++)
  {
    if (vdataSrcWork[i] == NULL)
      continue;

    Tab<int> &strip = eqMatStripIdx.push_back();
    strip.push_back(i);

    for (int j = i + 1; j < vdataSrcWork.size(); j++)
      if (vdataSrcWork[j] && srcMat[i].get() == srcMat[j].get())
      {
        strip.push_back(j);
        vdataSrcWork[j] = NULL;
      }
  }

  {
    Tab<int> tmpIdx(tmpmem);
    mem_set_ff(vDataEmsiStart);
    for (i = eqMatStripIdx.size() - 1; i >= 0; i--)
    {
      // writeStrip ( i, "before", eqMatStripIdx[i], srcCr.data());
      alignObjStrip(eqMatStripIdx[i], srcCr.data(), tmpIdx);
      // writeStrip ( i, "after", eqMatStripIdx[i], srcCr.data());
      vDataEmsiStart[eqMatStripIdx[i][0]] = i;
    }
  }

  // restore workset
  for (i = 0; i < vDataMap.size(); i++)
    vdataSrcWork[i] = vdataSrc[i];


  // connect by material first
  for (i = 0; i < vdataSrcWork.size(); i++)
  {
    if (update_info)
      update_info->update(i, vdataSrcWork.size() * 2);

    // skip non-starting data of strips
    if (vDataEmsiStart[i] == -1)
      continue;

    G_ASSERT(vdataSrcWork[i]);

    // add original vertex data to dest list
    // vdataDest.push_back(vdataSrcWork[i]);
    vDataMap[i] = VDataRemap(vdataSrcWork[i], 0, 0, 0);
    DEBUGCM("check %d", i);

    Tab<int> &strip = eqMatStripIdx[vDataEmsiStart[i]];
    G_ASSERT(strip[0] == i);

    int strip_len = strip.size();
    for (int k = strip.size() - 1; k > 0; k--)
    {
      int j = strip[k];

      G_ASSERT(vdataSrcWork[j]);
      if (vdataSrcWork[i]->canAttachData(*vdataSrcWork[j], allow_32_bit))
      {
        vDataMap[j] = VDataRemap(vdataSrcWork[i], vdataSrcWork[i]->numv, vdataSrcWork[i]->numf * 3, vdataSrcWork[i]->partCount);
        DEBUGCM("%d %d", vDataMap[j].sv, vDataMap[j].si);

        // attach
        vdataSrcWork[i]->attachData(*vdataSrcWork[j]);

        // remove reference for connected data
        vdataSrcWork[j] = NULL;
        erase_items(strip, k, 1);
      }
    }
    if (strip.size() > 1)
    {
      // debug ( "%d: data (%d,%d, %p), strip part left: %d",
      //   i, vdataSrcWork[i]->numv, vdataSrcWork[i]->numf*3, srcMat[i].get(), strip.size()-1);
      strip[0] = strip.back();
      strip.resize(strip.size() - 1);
      vDataEmsiStart[strip[0]] = vDataEmsiStart[i];
      vDataEmsiStart[i] = -1;

      if (strip[0] < i + 1)
        i = strip[0] - 1;
    }
    else
    {
      // debug ( "%d: data (%d,%d, %p) packed fully (strip len: %d)",
      //   i, vdataSrcWork[i]->numv, vdataSrcWork[i]->numf*3, srcMat[i].get(), strip_len);
      vDataEmsiStart[i] = -1;
    }
  }


  // connect by vertex format
  for (i = 0; i < vdataSrcWork.size(); i++)
  {
    if (update_info)
      update_info->update(i + vdataSrcWork.size(), vdataSrcWork.size() * 2);

    // skip already connected datas
    if (vdataSrcWork[i] == NULL)
      continue;

    for (;;)
    {
      // find best candidate for connection
      int bestIndex = -1;

      // check for all other free datas for connection
      for (int j = i + 1; j < vdataSrcWork.size(); j++)
      {
        DEBUGCM("check %d vs %d", i, j);
        if (!vdataSrcWork[j])
          continue;

        if (!vdataSrcWork[i]->canAttachData(*vdataSrcWork[j], allow_32_bit))
          continue;

        bestIndex = j;
        break;
      }

      if (bestIndex < 0)
        break;

      DEBUGCM("connect #%d to #%d", bestIndex, i);

      // store remap values
      vDataMap[bestIndex] = VDataRemap(vdataSrcWork[i], vdataSrcWork[i]->numv, vdataSrcWork[i]->numf * 3, vdataSrcWork[i]->partCount);

      for (int j = 0; j < vdataSrcWork.size(); ++j)
        if (j != bestIndex && vDataMap[j].data == vdataSrcWork[bestIndex])
        {
          vDataMap[j].data = vdataSrcWork[i];
          vDataMap[j].sv += vDataMap[bestIndex].sv;
          vDataMap[j].si += vDataMap[bestIndex].si;
          vDataMap[j].baseOrderIndex += vDataMap[bestIndex].baseOrderIndex;
        }

      // attach
      vdataSrcWork[i]->attachData(*vdataSrcWork[bestIndex]);
      if (srcMat[i] != srcMat[bestIndex])
        srcMat[i] = NULL;

      // remove reference for connected data
      vdataSrcWork[bestIndex] = NULL;
    }
  }


  // fill vdataDest
  G_ASSERT(vdataDest.size() == 0);

  for (i = 0; i < vdataSrcWork.size(); i++)
    if (vdataSrcWork[i])
      vdataDest.push_back(vdataSrcWork[i]);

  // if we have more than source.count vertexdatas after connecting, this is reason for thinking....
  G_ASSERT(vdataDest.size() <= vdataSrcWork.size());

  if (GlobalVertexDataConnector::allowVertexMerge)
  {
    struct Chunk
    {
      const uint8_t *data = nullptr;
      int size = 0;

      size_t __forceinline operator()(const Chunk &p) const { return mum_hash(p.data, p.size, 0); }
      bool operator==(const Chunk &other) const { return size == other.size && memcmp(data, other.data, size) == 0; }
    };

    eastl::hash_map<Chunk, int, Chunk> chunks_map;
    Tab<uint8_t> new_vd;
    for (i = 0; i < vdataDest.size(); i++)
    {
      if (vdataDest[i]->gvdFlags & VDATA_NO_IB)
        continue;
      if (!vdataDest[i]->allowVertexMerge)
        continue;

      uint8_t *vd = vdataDest[i]->vData.data();
      int stride = vdataDest[i]->stride;
      vdataDest[i]->vRemap.resize(vdataDest[i]->numv);
      mem_set_ff(vdataDest[i]->vRemap);

      G_ASSERT(vdataDest[i]->numv * stride == data_size(vdataDest[i]->vData));
      G_ASSERT(stride >= 4);

      new_vd.clear();
      new_vd.reserve(vdataDest[i]->numv * stride);
      chunks_map.clear();
      chunks_map.reserve(vdataDest[i]->numv);

      for (int j = 0, je = vdataDest[i]->numv; j < je; j++)
      {
        G_ASSERT(chunks_map.size() == data_size(new_vd) / stride);
        auto insert_pair = chunks_map.emplace(Chunk{vd + stride * j, stride}, (int)chunks_map.size());
        if (insert_pair.second)
          append_items(new_vd, stride, vd + stride * j);

        vdataDest[i]->vRemap[j] = insert_pair.first->second;
      }

      if (data_size(vdataDest[i]->vData) > data_size(new_vd))
      {
        new_vd.shrink_to_fit();
        vdataDest[i]->vData = eastl::move(new_vd);
        vdataDest[i]->numv = data_size(vdataDest[i]->vData) / stride;
        vdataDest[i]->iNew.reserve(vdataDest[i]->numf * 3);
      }
      else
        clear_and_shrink(vdataDest[i]->vRemap);
    }
    // if (equal_v)
    //   debug("vertex: total=%d equal=%d (save %d bytes)", total_v, equal_v, save_vb);
  }

  // if vdata count is not changed, no shifting needed
  // if (vdataSrc.size() != vdataDest.size())
  {
    // shift indices in mesh datas
    for (i = 0; i < meshes.size(); i++)
    {
      int j;
      ShaderMeshData &md = *meshes[i];
      DEBUGCM("mesh data: %d elems", md.elems.size());

      for (j = 0; j < md.elems.size(); j++)
      {
        ShaderMeshData::RElem &re = md.elems[j];
        int vdIndex = tabutils::getIndex(vdataSrc, (GlobalVertexDataSrc *)re.vertexData);
        re.vertexData = vDataMap[vdIndex].data;
        DEBUGCM("vdIndex = %d, vDataMap.count = %d,  sv=%d si=%d, new sv=%d si=%d", vdIndex, vDataMap.size(), re.sv, re.si,
          re.sv + vDataMap[vdIndex].sv, re.si + vDataMap[vdIndex].si);
        re.sv += vDataMap[vdIndex].sv;
        re.si += vDataMap[vdIndex].si;
        re.vdOrderIndex += vDataMap[vdIndex].baseOrderIndex;
        DEBUGCM("vdata #%d used, new vdata is %X", vdIndex, (GlobalVertexDataSrc *)re.vertexData);
      }
    }
  }

  // rebuild indices
  for (i = 0; i < meshes.size(); i++)
  {
    ShaderMeshData &md = *meshes[i];
    for (int j = 0; j < md.elems.size(); j++)
    {
      ShaderMeshData::RElem &re = md.elems[j];
      GlobalVertexDataSrc *vd = re.vertexData;
      if (!vd->vRemap.size())
        continue;

      int v0 = -1, v1 = -1;
      if (vd->iData32.size())
      {
        for (int k = re.si, ke = re.si + re.numf * 3; k < ke; k++)
        {
          int v = vd->vRemap[vd->iData32[k]];
          if (v0 < 0)
            v0 = v1 = v;
          else if (v < v0)
            v0 = v;
          else if (v > v1)
            v1 = v;
          vd->iData32[k] = v;
        }
      }
      else if (vd->iData.size())
      {
        for (int k = re.si, ke = re.si + re.numf * 3; k < ke; k++)
        {
          int v = vd->vRemap[vd->iData[k]];
          if (v0 < 0)
            v0 = v1 = v;
          else if (v < v0)
            v0 = v;
          else if (v > v1)
            v1 = v;
          vd->iData[k] = v;
        }
        G_ASSERTF(v1 < 0x10000, "v0=%d v1=%d sv=%d", v0, v1, re.sv);
      }

      if (v0 < 0)
        continue;
      re.sv = v0;
      re.numv = v1 - v0 + 1;
    }
  }

  if (vdataDest.size() > 1 && GlobalVertexDataConnector::allowBaseVertex)
  {
    vDataMap.resize(vdataDest.size());
    vdataSrcWork.resize(vdataDest.size());
    for (i = 0; i < vDataMap.size(); i++)
    {
      vDataMap[i].data = vdataSrcWork[i] = vdataDest[i];
      vDataMap[i].sv = vDataMap[i].si = vDataMap[i].baseOrderIndex = 0;
    }

    for (i = 0; i + 1 < vdataSrcWork.size(); i++)
    {
      if (!vdataSrcWork[i])
        continue;
      for (int j = i + 1; j < vdataSrcWork.size(); j++)
        if (vdataSrcWork[j] && vdataSrcWork[i]->stride == vdataSrcWork[j]->stride &&
            vdataSrcWork[i]->is32bit() == vdataSrcWork[j]->is32bit() && vdataSrcWork[i]->numv + vdataSrcWork[j]->numv < (1 << 20) &&
            vdataSrcWork[i]->numf + vdataSrcWork[j]->numf < (4 << 20))
        {
          vDataMap[j].sv = vdataSrcWork[i]->numv;
          vDataMap[j].si = vdataSrcWork[i]->numf * 3;
          vDataMap[j].data = vdataSrcWork[i];
          vdataSrcWork[i]->numv += vdataSrcWork[j]->numv;
          vdataSrcWork[i]->numf += vdataSrcWork[j]->numf;
          append_items(vdataSrcWork[i]->vData, vdataSrcWork[j]->vData.size(), vdataSrcWork[j]->vData.data());
          if (vdataSrcWork[i]->is32bit())
            append_items(vdataSrcWork[i]->iData32, vdataSrcWork[j]->iData32.size(), vdataSrcWork[j]->iData32.data());
          else
            append_items(vdataSrcWork[i]->iData, vdataSrcWork[j]->iData.size(), vdataSrcWork[j]->iData.data());
          vdataSrcWork[i]->baseVertSegCount++;
          vdataSrcWork[j] = NULL;
        }
    }

    for (i = 0; i < meshes.size(); i++)
    {
      ShaderMeshData &md = *meshes[i];
      for (int j = 0; j < md.elems.size(); j++)
      {
        ShaderMeshData::RElem &re = md.elems[j];
        int vdIndex = tabutils::getIndex(vdataDest, (GlobalVertexDataSrc *)re.vertexData);
        re.vertexData = vDataMap[vdIndex].data;
        re.bv += vDataMap[vdIndex].sv;
        re.si += vDataMap[vdIndex].si;
      }
    }
  }

  DEBUGCM("vertex datas connected: before: %d, after: %d", vdataSrc.size(), vdataDest.size());
  if (update_info)
    update_info->endConnect(vdataSrc.size(), vdataDest.size());

  for (GlobalVertexDataSrc *vd : vdataDest)
    vd->gvdFlags = gdata_flg | (vd->is32bit() ? VDATA_I32 : VDATA_I16);
}
