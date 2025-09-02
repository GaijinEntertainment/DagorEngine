// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "level.h"
#include <ioSys/dag_fileIo.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <scene/dag_visibility.h>
#include <memory/dag_framemem.h>

CONSOLE_BOOL_VAL("hmap", hmap_render, true);
CONSOLE_BOOL_VAL("hmap", metrics, true);
CONSOLE_BOOL_VAL("render", flexgrid, false);
CONSOLE_INT_VAL("hmap", displacement_quality, 1, 1, 4);
CONSOLE_BOOL_VAL("render", flexgrid_debug, false);

void set_lmesh_normalmap(LMesh *l, TEXTUREID id);
bool load_lmesh_hmap(LMesh &l, IGenLoad &crd);
bool init_lmesh(LMesh &l);
bool get_lmesh_min_max(const LMesh &l, const BBox3 &box, float &minHt, float &maxHt);
bool get_height(const LMesh &l, const Point2 &p, float &ht, Point3 *normal);
const HeightmapHandler *get_lmesh_hmap(const LMesh &l);

const HeightmapHandler *StrmSceneHolder::getHeightmap() const { return lMesh ? get_lmesh_hmap(*lMesh) : &heightmap; }

void StrmSceneHolder::bdlTextureMap(unsigned /*bindump_id*/, dag::ConstSpan<TEXTUREID> texId)
{
  for (int idx = texId.size() - 1; idx >= 0; idx--)
  {
    TEXTUREID tid = texId[idx];
    if (tid == BAD_TEXTUREID)
      continue;
    if (strstr(get_managed_texture_name(tid), "_lightmap*"))
    {
      normalmapTexId = tid;
      break;
    }
  }
}

void StrmSceneHolder::loadHeightmap(IGenLoad &crd)
{
  if (!heightmap.loadDump(crd, true, nullptr))
    logerr("can't load %s", crd.getTargetName());
  else
  {
    debug("loaded heightmap from %s", crd.getTargetName());
    state = Loaded;
  }
}

bool StrmSceneHolder::bdlCustomLoad(unsigned bindump_id, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> tex)
{
  if (tag == _MAKE4C('lmap'))
  {
    loadLMesh(crd);
    return true;
  }

  if (tag == _MAKE4C('HM2'))
  {
    if (lMesh)
    {
      if (load_lmesh_hmap(*lMesh, crd))
        state = Loaded;
    }
    else
      loadHeightmap(crd);
    return true;
  }
  return BaseStreamingSceneHolder::bdlCustomLoad(bindump_id, tag, crd, tex);
}

void StrmSceneHolder::loadHeightmap(const char *fn)
{
  if (!fn)
    return;
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    logerr("can't open %s", fn);
    return;
  }
  if (crd.beginTaggedBlock() != _MAKE4C('HM2'))
  {
    logerr("incorrect block");
  }
  loadHeightmap(crd);
  return;
}

void StrmSceneHolder::loadHeightmapRaw(const char *fn, float cell_size)
{
  if (!fn)
    return;
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    logerr("can't open %s", fn);
    return;
  }
  uint64_t sz = crd.getTargetDataSize();
  uint32_t w = sqrt(sz / 4);
  if (w * w * 4 != sz || w == 0)
  {
    logerr("%s is not raw32 file", fn);
    return;
  }
  Tab<float> r32;
  r32.resize(w * w);
  crd.read(r32.data(), w * w * 4);
  Tab<uint16_t> r16;
  r16.resize(w * w);
  float hMin = r32[0], hMax = r32[0];
  for (auto f : r32)
  {
    hMin = min(f, hMin);
    hMax = max(f, hMax);
  }
  hMax = max(hMax, hMin + 1e-32f);
  float scale = 65535.f / (hMax - hMin);
  auto to = r16.data();
  for (int y = 0; y < w; ++y)
  {
    auto from = r32.data() + (w - 1 - y) * w;
    for (int x = 0; x < w; ++x, ++to, ++from)
      *to = clamp<int>((*from - hMin) * scale + 0.5f, 0, 65535);
  }

  heightmap.initRaw(r16.data(), cell_size, w, w, hMin, hMax - hMin, -0.5 * cell_size * Point2(w, w));
  heightmap.initRender();
  debug("loaded heightmap from %s", crd.getTargetName());
  state = Loaded;
}

void StrmSceneHolder::loadHeightmapRaw16(const char *fn, float cell_size, float hMin, float hMax)
{
  if (!fn)
    return;
  FullFileLoadCB crd(fn);
  if (!crd.fileHandle)
  {
    logerr("can't open %s", fn);
    return;
  }
  uint64_t sz = crd.getTargetDataSize();
  uint32_t w = sqrt(sz / 2);
  if (w * w * 2 != sz || w == 0)
  {
    logerr("%s is not raw32 file", fn);
    return;
  }
  Tab<uint16_t> r16;
  r16.resize(w * w);
  for (int y = 0; y < w; ++y)
  {
    auto to = r16.data() + (w - 1 - y) * w;
    crd.read(to, w * 2);
  }

  heightmap.initRaw(r16.data(), cell_size, w, w, hMin, hMax - hMin, -0.5 * cell_size * Point2(w, w));
  heightmap.initRender(false);
  debug("loaded heightmap from %s", crd.getTargetName());
  state = Loaded;
}

float hmap_water_level = -10000;
void StrmSceneHolder::initOnDemand()
{
  if (state == Loaded)
  {
    if (lMesh)
      init_lmesh(*lMesh);
    if (bool(heightmap.getCompressedData()))
    {
      heightmap.init();
      heightmap.invalidateCulling(IBBox2{{0, 0}, {heightmap.hmapWidth.x - 1, heightmap.hmapWidth.y - 1}});

      if (!flexGridRenderer.isInited())
      {
        debug("initializing flexGridRenderer");
        flexGridConfig.maxInstanceCount = 8192;
        flexGridRenderer.init(flexGridConfig, true);         // NOTE: use flexGridRenderer.close() on level close
        heightmap.prepare(Point3(), 0.0f, hmap_water_level); // to set hmap_water_level to hmap_handler
        flexGridRenderer.prepareSubdivisionForHeightmap(heightmap);
      }
    }
    state = Inited;
  }
}

void StrmSceneHolder::onSceneLoaded()
{
  set_lmesh_normalmap(lMesh.get(), normalmapTexId);
  initOnDemand();
}


void frustumCulling(LodGridCullData &cull_data, const Point3 &world_pos, float water_level, const Frustum &frustum,
  const Occlusion *occlusion, const HeightmapHandler &handler, const TMatrix4 &proj);

void StrmSceneHolder::renderHeightmap(const vec4f &vp, const Frustum &frustum, const TMatrix4 &proj)
{
  if (state == No)
    return;
  initOnDemand();
  if (!bool(heightmap.getCompressedData()))
    return;
  if (!hmap_render)
    return;

  Point3_vec4 viewPos;
  v_st(&viewPos.x, vp);
  TIME_D3D_PROFILE(strm_heightmap);
  float camera_height = 4.0f, hmin = 0, htb = 0;
  if (heightmap.heightmapHeightCulling)
  {
    int lod = floorf(heightmap.heightmapHeightCulling->getLod(128.f));
    heightmap.heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(viewPos), hmin, htb);
    camera_height = max(camera_height, viewPos.y - htb);
  }

  if (flexgrid && !frustum.testBoxB(heightmap.vecbox.bmin, heightmap.vecbox.bmax))
    return;
  DA_PROFILE_GPU;

  if (flexgrid)
  {
    int subdivCacheUid = 0;
    bool forceLowQuality = false; // use it with auxiliary render passes: probes baking, reflection, etc
    TMatrix viewTm;
    d3d::gettm(TM_VIEW, viewTm);
    TMatrix4 viewTm4 = TMatrix4(viewTm);
    heightmap.prepare(viewPos, camera_height, hmap_water_level); // to set hmap_water_level to hmap_handler
    heightmap.setVars();
    flexGridRenderer.prepareSubdivision(subdivCacheUid, viewPos, viewTm4, proj, frustum, heightmap, nullptr,
      displacement_quality.get(), forceLowQuality, flexGridConfig);
    flexGridRenderer.render(subdivCacheUid, flexGridConfig, flexgrid_debug.get());
  }
  else if (!metrics)
  {
    camera_height = 0;
    if (heightmap.prepare(viewPos, camera_height))
      heightmap.render(0);
  }
  else
  {
    heightmap.makeBookKeeping();
    heightmap.setVars();
    LodGridCullData defaultCullData(framemem_ptr());
    frustumCulling(defaultCullData, viewPos, hmap_water_level, frustum, NULL, heightmap, proj);
    heightmap.renderCulled(defaultCullData);
  }
}

bool StrmSceneHolder::getMinMax(float &minHt, float &maxHt) const
{
  if (state == No)
    return false;
  if (lMesh)
  {
    BBox3 box = get_lmesh_box(*lMesh);
    if (!box.isempty())
    {
      minHt = box[0].y;
      maxHt = box[1].y;
      return true;
    }
  }
  minHt = heightmap.worldBox[0].y;
  maxHt = heightmap.worldBox[1].y;
  return true;
}

bool StrmSceneHolder::getMinMax(const BBox3 &box, float &minHt, float &maxHt)
{
  if (state == No)
    return false;
  if (lMesh)
    if (get_lmesh_min_max(*lMesh, box, minHt, maxHt))
      return true;

  initOnDemand();
  if (!heightmap.heightmapHeightCulling)
  {
    minHt = heightmap.worldBox[0].y;
    maxHt = heightmap.worldBox[1].y;
    return true;
  }
  const float patchSz = max(box.width().z, box.width().x);
  const int lod = floorf(heightmap.heightmapHeightCulling->getLod(patchSz));
  heightmap.heightmapHeightCulling->getMinMax(lod, Point2::xz(box[0]), patchSz, minHt, maxHt);
  return true;
}

bool StrmSceneHolder::getHeight(const Point2 &p, float &ht, Point3 *normal) const
{
  if (!lMesh)
    return false;
  return get_height(*lMesh, p, ht, normal);
}

void StrmSceneHolder::render(const VisibilityFinder &vf, Hmap hmap, Lmesh lmesh, const TMatrix4 &proj)
{
  base::render(vf);
  if (bool(hmap))
    renderHeightmap(vf.getViewerPos(), vf.getFrustum(), proj);
  if (bool(lmesh))
    renderLmesh();
}
