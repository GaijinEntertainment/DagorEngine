#include <shaders/dag_renderScene.h>
#include <shaders/sh_vars.h>
#include <shaders/dag_shaderBlock.h>
#include "scriptSElem.h"

#include <3d/dag_texMgr.h>
#include <3d/dag_materialData.h>
#include <obsolete/dag_cfg.h>
#include <workCycle/dag_gameSettings.h>

#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <util/dag_loadingProgress.h>
#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_objVisDebug.h>
#include <generic/dag_sort.h>
#include <supp/dag_prefetch.h>
#include <math/dag_mathUtils.h>

#define PERF_SCENE_RENDER 0

#if PERF_SCENE_RENDER
#include <workCycle/dag_workCyclePerf.h>
#endif

#include <debug/dag_debug.h>
#include <debug/dag_log.h>


#define START_MIXING_LODS_PERCENT 0.18
// you can increase this number, when change scene (*.scn) format
// and users must rebuild all scenes.
// DON'T CHANGE THIS NUMBER IF YOU DON'T KNOW WHAT YOU ARE DOING!
// AFTER THIS ALL SCENES IN ALL PROJECTS MUST BE REBUILT BEFORE GAME ALLOWS TO LOAD THEM!
// static const int currentVersion = _MAKE4C('1.11');

bool RenderScene::enable_prerender = false;
bool RenderScene::should_build_optscene_data = true;


namespace core
{
extern int memGraphId, actGraphId;
extern int brGraphId;
extern int clocksPerUsec;
} // namespace core

RenderScene::RenderScene() : robjMem(NULL)
{
  optScn.inited = false;
  optScn.prepareDone = false;
  hasReflectionRefraction_ = false;
}

RenderScene::~RenderScene()
{
  for (int i = 0; i < MAX_CVIS; i++)
    cVisData[i].clear();

  clearRenderObjects();
}

void RenderScene::clearRenderObjects()
{
  if (!robjMem)
    return;

  for (int i = 0; i < obj.size(); i++)
    obj[i].freeData();

  obj.init(NULL, 0);
  memfree(robjMem, midmem);
}


struct RenderScene::OptimizedScene::Elem
{
  GlobalVertexData *vd;
  int sv, si;
  unsigned short numv, numf;
};
struct RenderScene::OptimizedScene::Mat
{
  ShaderElement *e;
  unsigned short se, ee;
};

static int sort_mesh_elems(ShaderMesh::RElem *const *_a, ShaderMesh::RElem *const *_b)
{
  const ShaderMesh::RElem &a = *_a[0];
  const ShaderMesh::RElem &b = *_b[0];
  int diff = a.vertexData - b.vertexData;
  if (!diff)
    diff = a.vdOrderIndex - b.vdOrderIndex;
  return diff;
}

void RenderScene::buildOptSceneData()
{

  if (optScn.inited)
  {
    clear_and_shrink(optScn.mats);
    clear_and_shrink(optScn.elems);
    clear_and_shrink(optScn.elemIdxStor);
    optScn.visData.clear();
    optScn.visTree.nodes.reset();
    optScn.visTree.leavesCount = 0;
    clear_and_shrink(optScn.buildVisTree.nodes);
    memset(optScn.stageEndMatIdx, 0, sizeof(optScn.stageEndMatIdx));
    optScn.inited = false;
    optScn.prepareDone = false;
  }

  PtrTab<ShaderMaterial> mats(tmpmem);
  Tab<int> perMatElems(tmpmem);
  Tab<int> perMatCnt(tmpmem);
  Tab<ShaderMesh::RElem *> elems(tmpmem);
  unsigned elemPerStage[SC_STAGE_IDX_MASK + 1];
  unsigned facePerStage[SC_STAGE_IDX_MASK + 1];

  memset(optScn.stageEndMatIdx, 0, sizeof(optScn.stageEndMatIdx));
  memset(elemPerStage, 0, sizeof(elemPerStage));
  memset(facePerStage, 0, sizeof(facePerStage));
  for (int stage = ShaderMesh::STG_opaque; stage < ShaderMesh::STG_COUNT; stage++)
  {
    // debug("--- RenderScene: stage=%d", stage);
    for (int i = 0; i < obj.size(); ++i)
    {
      G_ASSERT_CONTINUE(obj[i].lods.size() == 1);
      dag::Span<ShaderMesh::RElem> el = obj[i].lods[0].mesh->getElems(stage);
      for (int k = 0; k < el.size(); k++)
      {
        ShaderMaterial *m = el[k].mat;
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
          // debug("mat[%d] %s", idx, m->getInfo());
          G_ASSERT(perMatElems.size() == idx);
          perMatElems.push_back(0);
        }
        perMatElems[idx]++;
        elemPerStage[stage]++;
        facePerStage[stage] += el[k].numf;
      }
    }
    optScn.stageEndMatIdx[stage] = mats.size();
    // debug("== total %d elems (%d faces)", elemPerStage[stage], facePerStage[stage]);
  }
  for (int stage = ShaderMesh::STG_COUNT; stage <= SC_STAGE_IDX_MASK; stage++)
    optScn.stageEndMatIdx[stage] = optScn.stageEndMatIdx[stage - 1];

  int elemnum = 0;
  perMatCnt.resize(perMatElems.size());
  for (int i = 0; i < perMatElems.size(); i++)
  {
    perMatCnt[i] = elemnum;
    elemnum += perMatElems[i];
  }
  elems.resize(elemnum);
  mem_set_0(elems);
  debug("RenderScene: %d objs -> %d mats, %d elems [%d opaque (%d faces), %d atest (%d faces),"
        " %d trans (%d faces), %d distortion (%d faces)]",
    obj.size(), mats.size(), elemnum, elemPerStage[ShaderMesh::STG_opaque], facePerStage[ShaderMesh::STG_opaque],
    elemPerStage[ShaderMesh::STG_atest], facePerStage[ShaderMesh::STG_atest], elemPerStage[ShaderMesh::STG_trans],
    facePerStage[ShaderMesh::STG_trans], elemPerStage[ShaderMesh::STG_distortion], facePerStage[ShaderMesh::STG_distortion]);

  for (int i = 0; i < obj.size(); ++i)
    for (int stage = ShaderMesh::STG_opaque; stage < ShaderMesh::STG_COUNT; stage++)
    {
      dag::Span<ShaderMesh::RElem> el = obj[i].lods[0].mesh->getElems(stage);
      for (int k = 0; k < el.size(); k++)
      {
        ShaderMaterial *m = el[k].mat;
        for (int l = 0; l < mats.size(); l++)
          if (mats[l] == m)
          {
            elems[perMatCnt[l]] = &el[k];
            perMatCnt[l]++;
            break;
          }
      }
    }

  for (int i = 0; i < elems.size(); ++i)
    if (!elems[i])
      DAG_FATAL("not filled elem %d", i);

  elemnum = 0;
  clear_and_resize(optScn.mats, mats.size());
  for (int i = 0; i < perMatElems.size(); i++)
  {
    optScn.mats[i].e = mats[i]->make_elem();
    optScn.mats[i].se = elemnum;
    optScn.mats[i].ee = elemnum + perMatElems[i];
    sort(elems, elemnum, perMatElems[i], &sort_mesh_elems);
    elemnum += perMatElems[i];
  }

  clear_and_resize(optScn.elems, elems.size());
  for (int i = 0; i < elems.size(); i++)
  {
    optScn.elems[i].vd = elems[i]->vertexData;
    optScn.elems[i].sv = elems[i]->sv;
    optScn.elems[i].si = elems[i]->si;
    G_ASSERT(elems[i]->numv < 0xFFFF);
    G_ASSERT(elems[i]->numf < 0xFFFF);
    G_ASSERTF(elems[i]->baseVertex == 0, "elems[%d]->baseVertex=%d", i, elems[i]->baseVertex);
    optScn.elems[i].numv = elems[i]->numv;
    optScn.elems[i].numf = elems[i]->numf;
  }
  clear_and_resize(optScn.elemIdxStor, elems.size());
  optScn.usedElemsStride = (elems.size() + 31) / 32;

  clear_and_resize(optScn.visData.usedElems, optScn.usedElemsStride);
  optScn.visData.scn = &optScn;

  SmallTab<HierVisBuildNode, TmpmemAlloc> objs;

  clear_and_resize(objs, obj.size());
  elemnum = 0;
  for (int i = 0; i < obj.size(); ++i)
  {
    objs[i].flags = obj[i].flags;
    objs[i].bsph = obj[i].bsph;
    objs[i].bbox = obj[i].bbox;
    if (obj[i].maxSubobjRad > 0)
      objs[i].maxSubobjRad = obj[i].maxSubobjRad;
    else
      objs[i].maxSubobjRad = obj[i].bsph.r;

    dag::ConstSpan<ShaderMesh::RElem> relems = obj[i].lods[0].mesh->getAllElems();
    objs[i].first = elemnum;
    objs[i].last = elemnum + relems.size() - 1;
    for (int j = 0; j < relems.size(); j++)
    {
      const ShaderMesh::RElem *e = &relems[j];
      for (int k = 0; k < elems.size(); k++)
        if (elems[k] == e)
        {
          optScn.elemIdxStor[elemnum] = k;
          elemnum++;
          break;
        }
    }
  }
  G_ASSERT(elemnum == optScn.elemIdxStor.size());

  optScn.buildVisTree.build(objs.data(), objs.size(), false);

  optScn.visTree.nodes = make_span(optScn.buildVisTree.nodes);
  optScn.visTree.leavesCount = optScn.buildVisTree.leavesCount;
  optScn.buildVisTree.clearLastPlaneWord();

  optScn.prepareDone = false;
  optScn.inited = true;
}

bool RenderScene::replaceTexture(TEXTUREID old_texid, TEXTUREID new_texid)
{
  bool res = false;

  for (int i = 0; i < obj.size(); i++)
  {
    RenderObject &o = obj[i];

    for (int li = 0; li < o.lods.size(); ++li)
      if (o.lods[li].mesh && o.lods[li].mesh->replaceTexture(old_texid, new_texid))
        res = true;
  }

  return res;
}


void RenderScene::sort_objects(const Point3 &sort_point)
{
  if (optScn.inited)
    return; //== sorting has no effect

  if (objIndices.size() != obj.size())
  {
    clear_and_resize(objIndices, obj.size());
    for (int i = 0; i < objIndices.size(); ++i)
      objIndices[i] = i;
  }
  if (check_nan(sort_point))
    return;
  // debug("total %d %d", objIndices.size(), obj.size());
  stlsort::sort(objIndices.data(), objIndices.data() + objIndices.size(), [this, sort_point](int o1, int o2) {
    if (o1 == o2)
      return false;
    const RenderObject &obj1 = obj[o1];
    const RenderObject &obj2 = obj[o2];
    int trans1 = obj1.flags & RenderObject::ROF_HasTrans;
    int trans2 = obj2.flags & RenderObject::ROF_HasTrans;
    if (trans1 != trans2)
      return trans1 < trans2;

    if (trans1 && obj1.priority != obj2.priority)
      return obj2.priority < obj1.priority;
    // int trans1 = 0;
    real d1 = (sort_point - obj1.bsph.c).lengthSq();
    real d2 = (sort_point - obj2.bsph.c).lengthSq();
    if (d1 < d2)
      return trans1 ? false : true;
    else
      return trans1 ? true : false;
  });
}

void RenderScene::render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask)
{
  G_ASSERTF_RETURN(optScn.inited || !obj.size(), , "optScn.inited=%d while calling %p.render(%d, 0x%x) obj.size()=%d", optScn.inited,
    this, render_id, render_flags_mask, obj.size());

#if PERF_SCENE_RENDER
  int64_t ref_time = ref_time_ticks();
  int ft = workcycleperf::get_frame_timepos_usec();
#endif

  prepareVisibility(vf, render_id, render_flags_mask);

#if PERF_SCENE_RENDER
  int rt = get_time_usec(ref_time);
#endif

  int dprim = 0;

  const int lod = 0;

  {
    if (!optScn.visData.lastElemNum[lod])
      return;

    for (int i = optScn.getMatStartIdx(ShaderMesh::STG_opaque); i < optScn.getMatEndIdx(ShaderMesh::STG_atest); i++)
    {
      GlobalVertexData *vd = NULL;
      int sv = 0, si = 0, numv = 0, numf = 0;
      int ei = optScn.mats[i].se, ei_end = optScn.mats[i].ee;
      bool first = true;

      unsigned bit = 0x80000000 >> (ei & 0x1F);
      unsigned *wpos = &optScn.visData.usedElems[(ei >> 5) + lod * optScn.usedElemsStride];
      if (!*wpos)
      {
        ei += 32 - (ei & 0x1F);
        bit = 0x80000000;
        wpos++;
      }
      if (bit == 0x80000000 && ei < ei_end)
        while (!*wpos)
        {
          wpos++;
          ei += 32;
          if (ei >= ei_end)
            break;
        }

      for (; ei < ei_end; ei++)
      {
        if (*wpos & bit)
        {
          const OptimizedScene::Elem *__restrict e = &optScn.elems[ei];
          if (first)
          {
            //== code to be used while get_shader_lightmap() has side effects
            if (!optScn.mats[i].e->native().setStates())
              break;
            first = false;
          }
          if (e->vd == vd && e->sv == sv + numv && e->si == si + numf * 3)
          {
            numv += e->numv;
            numf += e->numf;
          }
          else
          {
            if (vd)
              d3d::drawind(PRIM_TRILIST, si, numf, 0);
            sv = e->sv;
            si = e->si;
            numv = e->numv;
            numf = e->numf;
            if (vd != e->vd)
            {
              vd = e->vd;
              e->vd->setToDriver();
            }
            dprim++;
          }
        }

        bit >>= 1;
        if (!bit)
        {
          if (ei + 1 == ei_end) // It is the last element, it will be an error to look for the next bit.
            break;

          wpos++;
          bit = 0x80000000;
          while (!*wpos)
          {
            wpos++;
            ei += 32;
            if (ei >= ei_end)
              break;
          }
        }
      }
      if (!first)
        d3d::drawind(PRIM_TRILIST, si, numf, 0);
    }
  }
#if PERF_SCENE_RENDER
  if (workcycleperf::debug_on)
    debug(" -- %d: RenderScene(%p)::render(): %d / %d us, %d/%d elems, %d dip", ft, this, rt, get_time_usec(ref_time),
      optScn.visData.lastElemNum[0], optScn.elems.size(), dprim);
#endif
}

void RenderScene::render(int render_id, unsigned render_flags_mask) { render(*visibility_finder, render_id, render_flags_mask); }

void RenderScene::render_trans()
{
  G_ASSERTF_RETURN(optScn.inited || !obj.size(), , "optScn.inited=%d while calling %p.render_trans() obj.size()=%d", optScn.inited,
    this, obj.size());

  //  debug("RenderScene:rt");


  int lod = 0;

  {
    if (!optScn.visData.lastElemNum[lod])
      return;

    for (int i = optScn.getMatStartIdx(ShaderMesh::STG_trans); i < optScn.getMatEndIdx(ShaderMesh::STG_trans); i++)
    {
      if (!optScn.mats[i].e)
        continue;

      GlobalVertexData *vd = NULL;
      int sv = 0, si = 0, numv = 0, numf = 0;
      int ei = optScn.mats[i].se, ei_end = optScn.mats[i].ee;
      bool first = true;

      unsigned bit = 0x80000000 >> (ei & 0x1F);
      unsigned *wpos = &optScn.visData.usedElems[(ei >> 5) + lod * optScn.usedElemsStride];
      if (!*wpos)
      {
        ei += 32 - (ei & 0x1F);
        bit = 0x80000000;
        wpos++;
      }
      if (bit == 0x80000000 && ei < ei_end)
        while (!*wpos)
        {
          wpos++;
          ei += 32;
          if (ei >= ei_end)
            break;
        }

      for (; ei < ei_end; ei++)
      {
        if (*wpos & bit)
        {
          const OptimizedScene::Elem *__restrict e = &optScn.elems[ei];
          if (first)
          {
            //== code to be used while get_shader_lightmap() has side effects
            if (!optScn.mats[i].e->native().setStates())
              break;
            first = false;
          }
          if (e->vd == vd && e->sv == sv + numv && e->si == si + numf * 3)
          {
            numv += e->numv;
            numf += e->numf;
          }
          else
          {
            if (vd)
              d3d::drawind(PRIM_TRILIST, si, numf, 0);
            sv = e->sv;
            si = e->si;
            numv = e->numv;
            numf = e->numf;
            if (vd != e->vd)
            {
              vd = e->vd;
              e->vd->setToDriver();
            }
          }
        }

        bit >>= 1;
        if (!bit)
        {
          if (ei + 1 == ei_end) // It is the last element, it will be an error to look for the next bit.
            break;

          wpos++;
          bit = 0x80000000;
          while (!*wpos)
          {
            wpos++;
            ei += 32;
            if (ei >= ei_end)
              break;
          }
        }
      }
      if (!first)
        d3d::drawind(PRIM_TRILIST, si, numf, 0);
    }
  }
}

void RenderScene::loadTextures(IGenLoad &cb, Tab<TEXTUREID> &tex)
{
  int n, i;
  cb.read(&n, 4);
  tex.resize(n);
  Tab<char> name(tmpmem);
  for (i = 0; i < n; ++i)
  {
    int l = 0;
    cb.read(&l, 1);
    if (l == 0)
    {
      tex[i] = BAD_TEXTUREID;
      continue;
    }
    name.resize(l + 1);
    cb.read(&name[0], l);
    name[l] = 0;
    tex[i] = get_managed_texture_id(name.data());
    if (tex[i] == BAD_TEXTUREID)
      tex[i] = add_managed_texture(name.data());
    if ((n & 0x3F) == 0)
      loading_progress_point();

    acquire_managed_tex(tex[i]);
  }
}


void RenderScene::load(const char *lmdir, bool use_vis)
{
  String fn = String(lmdir) + "/scene.scn";

  ShaderMaterial::setLoadingString(fn);

  int startUpTime = get_time_msec();
// int startUpTime0=startUpTime;
#define STARTUP_POINT(name)                                                                        \
  {                                                                                                \
    /*int t=get_time_msec();*/                                                                     \
    /*debug("*** %7dms: load scene %s (line %d)", t-startUpTime, (const char*)(name), __LINE__);*/ \
    /*startUpTime=t;*/                                                                             \
    loading_progress_point();                                                                      \
  }

  debug("loading scene %s", (char *)fn);

  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
    DAG_FATAL("can't open scene file '%s'", fn.str());

  STARTUP_POINT("open");

  loading_progress_point();
  if (crd.readInt() != _MAKE4C('scnf'))
  {
    DAG_FATAL("non-scene file"); // DAGOR_THROW(IGenLoad::LoadException("non-scene file", crd.tell()));
    return;
  }

  Tab<TEXTUREID> texMap(tmpmem);
  crd.beginBlock();
  loadTextures(crd, texMap);
  crd.endBlock();

  loadBinary(crd, make_span(texMap), use_vis);

  ShaderMaterial::setLoadingString(NULL);

  debug("    %7dms total load scene %s", get_time_msec() - startUpTime, (const char *)fn);
}

void RenderScene::RenderObject::freeData()
{
  for (int i = 0; i < lods.size(); ++i)
  {
    del_it(lods[i].mesh);
  }

  flags = 0;
}


bool RenderScene::checked_frustum = false;
bool RenderScene::rendering_depth_pass = false;
