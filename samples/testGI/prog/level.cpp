// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "level.h"
#include <ioSys/dag_fileIo.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <scene/dag_visibility.h>
#include <memory/dag_framemem.h>

#define GLOBAL_VARS_OPT_LIST     \
  VAR(heightmap_region)          \
  VAR(heightmap_parent_edges_at) \
  VAR(heightmap_has_morph)       \
  VAR(heightmap_morph)           \
  VAR(heightmap_texels)          \
  VAR(heightmap_edges)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPT_LIST
#undef VAR

CONSOLE_BOOL_VAL("hmap", hmap_render, true);
CONSOLE_INT_VAL("hmap", displacement_quality, 1, 1, 4);

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
  if (!heightmap.loadDump(crd, true))
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

float hmap_water_level = -10000;

bool load_heightmap_raw32(HeightmapHandler &heightmap, const char *fn, float cell_size, const Point2 *at);
void StrmSceneHolder::loadHeightmapRaw(const char *fn, float cell_size)
{
  Point2 at = dgs_get_settings()->getPoint2("heightmap_at", Point2(0, 0));
  if (!load_heightmap_raw32(heightmap, fn, cell_size, dgs_get_settings()->paramExists("heightmap_at") ? &at : nullptr))
    return;
  heightmap.initRender(false, hmap_water_level);
  heightmap.setMaxUpwardDisplacement(1);
  debug("loaded heightmap from %s", fn);
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
  heightmap.initRender(false, hmap_water_level);
  debug("loaded heightmap from %s", fn);
  state = Loaded;
}

void StrmSceneHolder::initOnDemand()
{
  if (state == Loaded)
  {
    if (lMesh)
      init_lmesh(*lMesh);
    if (bool(heightmap.getCompressedData()))
    {
      heightmap.init(dgs_get_settings()->getInt("heightmap_dim_bits", 4));
      heightmap.invalidateCulling(IBBox2{{0, 0}, {heightmap.hmapWidth.x - 1, heightmap.hmapWidth.y - 1}});
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

  DA_PROFILE_GPU;

  camera_height = 0;
  heightmap.makeBookKeeping();
  heightmap.setVars();
  if (heightmap.prepare(viewPos, camera_height, hmap_water_level))
  {
    LodGridCullData cullData;
    HeightmapMetricsQuality mq = {proj_to_distance_scale(proj)};
    if (mq.distanceScale == 0)
      mq.maxRelativeTexelTess = -4;
    HeightmapFrustumCullingInfo fi{viewPos, camera_height, 0, frustum, NULL, NULL, 0, 0, 1, mq};
    heightmap.frustumCulling(cullData, fi); // Independent from prepare for multithreading.
    heightmap.renderCulled(cullData);
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
