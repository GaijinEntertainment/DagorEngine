// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshManager.h>
#include "lmeshRendererGlue.h"
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderMesh.h>
#include <generic/dag_sort.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_driver.h>
#include <generic/dag_ptrTab.h>
#include <perfMon/dag_cpuFreq.h>
#include <supp/dag_prefetch.h>

#define PERF_SCENE_RENDER 0

#if PERF_SCENE_RENDER
#include <workCycle/dag_workCyclePerf.h>
#endif

static int sort_mesh_elems(ShaderMesh::RElem *const *_a, ShaderMesh::RElem *const *_b)
{
  const ShaderMesh::RElem &a = **_a;
  const ShaderMesh::RElem &b = **_b;
  if (ptrdiff_t vdDiff = a.vertexData - b.vertexData)
    return vdDiff < 0 ? -1 : 1;
  int diff = a.baseVertex - b.baseVertex;
  if (!diff)
    diff = a.si - b.si;
  return diff;
}

void landmesh::VisibilityData::init(landmesh::OptimizedScene *s)
{
  scn = s;
  clear_and_resize(usedElems, s->usedElemsStride);
  mem_set_0(usedElems);
  elemCount = s->elems.size();
}

void landmesh::buildOptSceneData(landmesh::OptimizedScene &optScn, LandMeshManager &provider, int lod)
{
  static int bottomVarId = get_shader_variable_id("bottom", true);

  if (optScn.inited)
  {
    clear_and_shrink(optScn.mats);
    clear_and_shrink(optScn.elems);
    optScn.visData.clear();
    optScn.inited = false;
    optScn.prepareDone = false;
  }

  Tab<ShaderMesh *> obj(tmpmem);
  PtrTab<ShaderMaterial> mats(tmpmem);
  Tab<int> perMatElems(tmpmem);
  Tab<int> perMatCnt(tmpmem);
  Tab<ShaderMesh::RElem *> elems(tmpmem);

  for (int y = 0; y < provider.getNumCellsY(); ++y)
    for (int x = 0; x < provider.getNumCellsX(); ++x)
      if (ShaderMesh *landm = provider.getCellLandShaderMeshRaw(x, y, lod))
        obj.push_back(landm);

  for (int i = 0; i < obj.size(); ++i)
  {
    for (int k = 0; k < obj[i]->getAllElems().size(); k++)
    {
      ShaderMaterial *m = obj[i]->getAllElems()[k].mat;
      int idx = -1;
      for (int l = 0; l < mats.size(); l++)
        if (mats[l] == m)
        {
          idx = l;
          break;
        }
      if (idx == -1)
      {
        idx = mats.size();
        mats.push_back(m);
        G_ASSERT(perMatElems.size() == idx);
        perMatElems.push_back(0);
      }
      perMatElems[idx]++;
    }
  }

  int elemnum = 0;
  perMatCnt.resize(perMatElems.size());
  for (int i = 0; i < perMatElems.size(); i++)
  {
    perMatCnt[i] = elemnum;
    elemnum += perMatElems[i];
  }
  elems.resize(elemnum);
  mem_set_0(elems);
  debug("optScn[%d]: %d objs -> %d mats, %d elems", lod, obj.size(), mats.size(), elemnum);

  for (int i = 0; i < obj.size(); ++i)
  {
    for (int k = 0; k < obj[i]->getAllElems().size(); k++)
    {
      ShaderMaterial *m = obj[i]->getAllElems()[k].mat;
      for (int l = 0; l < mats.size(); l++)
        if (mats[l] == m)
        {
          elems[perMatCnt[l]] = const_cast<ShaderMesh::RElem *>(&obj[i]->getAllElems()[k]);
          perMatCnt[l]++;
          break;
        }
    }
  }

  optScn.vData = elems.size() ? elems[0]->vertexData : NULL;
  for (int i = 0; i < elems.size(); ++i)
    if (!elems[i])
      DAG_FATAL("not filled elem %d", i);
    else if (optScn.vData != elems[i]->vertexData)
      DAG_FATAL("elem %d refers to different vData", i);

  elemnum = 0;
  clear_and_resize(optScn.mats, mats.size());
  for (int i = 0; i < perMatElems.size(); i++)
  {
    optScn.mats[i].e = mats[i]->make_elem();
    optScn.mats[i].se = elemnum;
    optScn.mats[i].ee = elemnum + perMatElems[i];
    if (bottomVarId >= 0)
    {
      optScn.mats[i].bottomType = -1;
      mats[i]->getIntVariable(bottomVarId, optScn.mats[i].bottomType);
      optScn.mats[i].bottomType++;
    }
    else
      optScn.mats[i].bottomType = 0;
    sort(elems, elemnum, perMatElems[i], &sort_mesh_elems);
    elemnum += perMatElems[i];
  }

  clear_and_resize(optScn.elems, elems.size());
  for (int i = 0; i < elems.size(); i++)
  {
    optScn.elems[i].sv = elems[i]->sv;
    optScn.elems[i].si = elems[i]->si;
    G_ASSERT(elems[i]->numv < 0xFFFF);
    G_ASSERT(elems[i]->numf < 0xFFFF);
    optScn.elems[i].numv = elems[i]->numv;
    optScn.elems[i].numf = elems[i]->numf;
    optScn.elems[i].baseVertex = elems[i]->baseVertex;
    const_cast<ShaderMesh::RElem *>(elems[i])->vdOrderIndex = i;
  }
  optScn.usedElemsStride = (elems.size() + 31) / 32;
  optScn.visData.init(&optScn);
  optScn.prepareDone = false;
  optScn.inited = true;
}

void landmesh::renderOptSceneData(landmesh::OptimizedScene &optScn, const IRosdSetStatesCB &cb)
{
  if (!optScn.inited)
    return;
  if (optScn.visData.minUsed >= optScn.elems.size())
    return;

#if PERF_SCENE_RENDER
  int64_t ref_time = ref_time_ticks();
  int ft = workcycleperf::get_frame_timepos_usec();
  int rt = get_time_usec(ref_time);
#endif

  int dprim = 0, el = 0;

  for (int i = 0; i < optScn.mats.size(); i++)
  {
    int sv = 0, si = 0, numv = 0, numf = 0, baseVertex = -1;
    int ei = optScn.mats[i].se, ei_end = optScn.mats[i].ee;
    bool first = true;

    if (ei_end < optScn.visData.minUsed)
      continue;
    unsigned bit = 0x80000000 >> (ei & 0x1F);
    unsigned *wpos = &optScn.visData.usedElems[(ei >> 5)];
    unsigned *wposEnd = &optScn.visData.usedElems[0] + optScn.visData.usedElems.size();
    if (!*wpos)
    {
      ei += 32 - (ei & 0x1F);
      bit = 0x80000000;
      wpos++;
    }
    if (bit == 0x80000000)
      while (wpos < wposEnd && !*wpos)
      {
        wpos++;
        ei += 32;
        if (ei >= ei_end)
          break;
      }

    PREFETCH_DATA(64, wpos);
    for (; ei < ei_end; ei++)
    {
      if (*wpos & bit)
      {
        const OptimizedScene::Elem *__restrict e = &optScn.elems[ei];
        PREFETCH_DATA(64, e);
        if (first)
        {
          if (!cb.applyMat(optScn.mats[i].e, optScn.mats[i].bottomType))
            break;
          optScn.vData->setToDriver();
          first = false;
        }
        el++;
        if (e->si == si + numf * 3 && e->baseVertex == baseVertex)
        {
          int newSv = min(e->sv, sv);
          numv = max(e->sv + e->numv, sv + numv) - newSv;
          sv = newSv;
          numf += e->numf;
        }
        else
        {
          if (baseVertex >= 0)
            d3d::drawind(PRIM_TRILIST, si, numf, baseVertex);
          sv = e->sv;
          si = e->si;
          numv = e->numv;
          numf = e->numf;
          baseVertex = e->baseVertex;
          dprim++;
        }
      }

      bit >>= 1;
      if (!bit)
      {
        wpos++;
        bit = 0x80000000;
        PREFETCH_DATA(64, wpos);
        while (wpos < wposEnd && !*wpos)
        {
          wpos++;
          ei += 32;
          if (ei >= ei_end)
            break;
        }
      }
    }
    if (!first)
      d3d::drawind(PRIM_TRILIST, si, numf, baseVertex);
    if (ei_end > optScn.visData.maxUsed)
      break;
  }

#if PERF_SCENE_RENDER
  // if (workcycleperf::debug_on)
  debug(" -- %d: renderOptSceneData(): %d / %d us, %d (%d) elems, %d dip (%d..%d)", ft, rt, get_time_usec(ref_time), el,
    optScn.elems.size(), dprim, optScn.visData.minUsed, optScn.visData.maxUsed);
#endif
  optScn.visData.reset();
}
