// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/riGenExtra.h"

#include <rendInst/ccExtra.h>
#include <rendInst/riSceneDump.h>
#include <scene/dag_occlusion.h>
#include <util/dag_console.h>
#include <debug/dag_log.h>
#include <ioSys/dag_fileIo.h>
#include <ioSys/dag_genIo.h>
#include <string.h>


namespace rendinst
{

// One scene's reconstructable state. Runs as a member so it can reach the SimpleScene internals
// (poolBox / poolSphereVerticalCenter) directly; the node list is walked alive-only via the scene
// iterator so the reader can allocate exactly the live nodes (dead slots carry no useful state).
void RendinstTiledScene::dumpForBenchmark(IGenSave &cb) const
{
  cb.writeReal(getTileSize());

  const int poolCnt = (int)poolBox.size();
  cb.writeInt(poolCnt);
  for (int i = 0; i < poolCnt; ++i)
  {
    cb.write(&poolBox[i], sizeof(bbox3f)); // bmin.w = distScaleSq, bmax.w = sphere radius
    const float vcenter = (i < (int)poolSphereVerticalCenter.size()) ? poolSphereVerticalCenter[i] : 0.f;
    cb.writeReal(vcenter);
  }

  cb.writeInt((int)getNodesAliveCount());
  for (auto ni : *this)
  {
    const mat44f node = getNode(ni);
    cb.write(&node, sizeof(mat44f));
  }
}

static uint32_t pack_pool_meta_flags(const RiExtraPool &p)
{
  uint32_t f = 0;
  if (p.posInst)
    f |= RI_SDPF_POS_INST;
  if (p.isTree)
    f |= RI_SDPF_IS_TREE;
  if (p.immortal)
    f |= RI_SDPF_IMMORTAL;
  if (p.isDynamicRendinst)
    f |= RI_SDPF_DYNAMIC;
  if (p.hasOccluder)
    f |= RI_SDPF_HAS_OCCLUDER;
  if (p.largeOccluder)
    f |= RI_SDPF_LARGE_OCCLUDER;
  if (p.isWalls)
    f |= RI_SDPF_IS_WALLS;
  if (p.usingClipmap)
    f |= RI_SDPF_USING_CLIPMAP;
  if (p.isGrassify)
    f |= RI_SDPF_IS_GRASSIFY;
  if (p.underwaterOnly)
    f |= RI_SDPF_UNDERWATER_ONLY;
  if (p.useShadow)
    f |= RI_SDPF_USE_SHADOW;
  if (p.patchesHeightmap)
    f |= RI_SDPF_PATCHES_HMAP;
  return f;
}

// Dumps the whole riExtra scene set: header, group config, per-pool object metadata, group
// scene-pool info + occluders, then each scene's node block. Layout is owned by rendInst/riSceneDump.h.
// Caller holds the riExtra read lock (see the public wrappers below).
static void dump_all_scenes_locked(IGenSave &cb)
{
  const int sceneCount = riExTiledScenes.size();
  const int poolCount = (int)riExtra.size();

  cb.writeInt((int)RI_SCENE_DUMP_MAGIC);
  cb.writeInt(RI_SCENE_DUMP_VERSION);
  cb.writeInt(sceneCount);
  cb.writeInt(poolCount);

  // group-level per-scene config
  for (int i = 0; i < sceneCount; ++i)
  {
    RiSceneDumpSceneConfig sc;
    sc.tileSize = riExTiledScenes[i].getTileSize();
    sc.maxDist = riExTiledSceneMaxDist[i];
    sc.maxSize = riExTiledSceneMaxSize[i];
    sc.nodeCount = riExTiledScenes[i].getNodesAliveCount();
    cb.write(&sc, sizeof(sc));
  }

  // per-pool object metadata (bbox + bsphere + LODs + flags + name)
  for (int i = 0; i < poolCount; ++i)
  {
    const RiExtraPool &p = riExtra[i];
    const char *name = riExtraMap.getName(i);

    RiSceneDumpPoolMeta m;
    memset(&m, 0, sizeof(m)); // keep tail/interior padding deterministic across dumps
    m.lbb = p.lbb;
    m.collBb = p.collBb;
    m.fullWabb = p.fullWabb;
    m.bsphXYZR = p.bsphXYZR;
    memcpy(m.distSqLOD, p.distSqLOD, sizeof(m.distSqLOD));
    m.lodLimits = p.lodLimits;
    m.tsIndex = p.tsIndex;
    m.flags = pack_pool_meta_flags(p);
    m.sphereRadius = p.sphereRadius;
    m.sphCenterY = p.sphCenterY;
    m.instanceCount = (int)p.riTm.size();
    m.nameLen = name ? (uint32_t)strlen(name) : 0;

    cb.write(&m, sizeof(m));
    if (m.nameLen)
      cb.write(name, m.nameLen);
  }

  // group-level tiled scene-pool info + occluders. Each array is prefixed with [count][elemSize] so a
  // reader can skip it without knowing the (private) TiledScenePoolInfo layout.
  const auto &tiledPools = riExTiledScenes.getPools();
  cb.writeInt((int)tiledPools.size());
  cb.writeInt((int)sizeof(TiledScenePoolInfo));
  if (!tiledPools.empty())
    cb.write(tiledPools.data(), (int)(tiledPools.size() * sizeof(TiledScenePoolInfo)));

  const auto &boxOccluders = riExTiledScenes.getBoxOccluders();
  cb.writeInt((int)boxOccluders.size());
  cb.writeInt((int)sizeof(bbox3f));
  if (!boxOccluders.empty())
    cb.write(boxOccluders.data(), (int)(boxOccluders.size() * sizeof(bbox3f)));

  const auto &quadOccluders = riExTiledScenes.getQuadOccluders();
  cb.writeInt((int)quadOccluders.size());
  cb.writeInt((int)sizeof(mat44f));
  if (!quadOccluders.empty())
    cb.write(quadOccluders.data(), (int)(quadOccluders.size() * sizeof(mat44f)));

  // per-scene node blocks
  for (int i = 0; i < sceneCount; ++i)
    riExTiledScenes[i].dumpForBenchmark(cb);
}

static void dump_culling_reference_locked(IGenSave &cb, mat44f_cref globtm, vec4f vpos_distscale, Occlusion *occl)
{
  const int sceneCount = riExTiledScenes.size();

  cb.writeInt((int)RI_CULLREF_DUMP_MAGIC);
  cb.writeInt(RI_CULLREF_DUMP_VERSION);
  cb.writeInt(sceneCount);
  cb.write(&globtm, sizeof(globtm));
  cb.write(&vpos_distscale, sizeof(vpos_distscale));

  for (int i = 0; i < sceneCount; ++i)
  {
    const RendinstTiledScene &tiledScene = riExTiledScenes[i];

    RiCullRefSceneRec rec;
    memset(&rec, 0, sizeof(rec));
    rec.totalNodes = tiledScene.getNodesAliveCount();

    // frustum + distance cull, no occlusion (use_flags=false, use_pools=true, use_occlusion=false)
    tiledScene.frustumCull<false, true, false>(globtm, vpos_distscale, 0, 0, nullptr, [&](scene::node_index, mat44f_cref m, vec4f) {
      rec.frustumVisible++;
      rec.frustumVisibleHash += ri_scene_dump_hash_node(m);
    });

    if (occl)
    {
      // frustum + distance + HZB occlusion cull
      tiledScene.frustumCull<false, true, true>(globtm, vpos_distscale, 0, 0, occl, [&](scene::node_index, mat44f_cref m, vec4f) {
        rec.frustumOccludedVisible++;
        rec.occludedVisibleHash += ri_scene_dump_hash_node(m);
      });
    }
    else
    {
      // no HZB available: the occlusion pass degenerates to the frustum-only result
      rec.frustumOccludedVisible = rec.frustumVisible;
      rec.occludedVisibleHash = rec.frustumVisibleHash;
    }

    cb.write(&rec, sizeof(rec));
  }
}

void dumpAllScenes(IGenSave &cb)
{
  // riExtra pools and the tiled scenes mutate under the RIExtra write lock; hold the read lock so the
  // whole dump (pool metadata, names, raw scene node walks) is one coherent snapshot on any thread.
  ScopedRIExtraReadLock rd;
  dump_all_scenes_locked(cb);
}

void dumpCullingReference(IGenSave &cb, mat44f_cref globtm, vec4f vpos_distscale, Occlusion *occl)
{
  // same lock ordering as the real cull (extraVisibility holds it around frustumCull); also keeps the
  // frustum and occlusion passes on the same node set, so the reference counts and hashes are comparable
  ScopedRIExtraReadLock rd;
  dump_culling_reference_locked(cb, globtm, vpos_distscale, occl);
}

void dumpBenchmarkCapture(IGenSave &scenes_cb, IGenSave &cull_cb, mat44f_cref globtm, vec4f vpos_distscale, Occlusion *occl)
{
  // ONE read lock across both writes: a writer on another thread cannot slip between the scene dump and
  // the culling reference, so the pair is byte-consistent - separate dumpAllScenes + dumpCullingReference
  // calls only guarantee per-file consistency.
  ScopedRIExtraReadLock rd;
  dump_all_scenes_locked(scenes_cb);
  dump_culling_reference_locked(cull_cb, globtm, vpos_distscale, occl);
}

} // namespace rendinst

static bool ri_scene_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("ri", "dump_scenes", 1, 2)
  {
    const char *fn = argc < 2 ? "ri_scenes.bin" : argv[1];
    console::print("dumping all riExtra scenes (%d scenes, %d pools) to %s", rendinst::riExTiledScenes.size(),
      (int)rendinst::riExtra.size(), fn);
    FullFileSaveCB cb(fn);
    if (cb.fileHandle)
    {
      rendinst::dumpAllScenes(cb);
      console::print("done");
    }
    else
      console::print_d("file %s can't be open for save", fn);
  }
  return found;
}

using namespace console;
REGISTER_CONSOLE_HANDLER(ri_scene_console_handler);
