#include <generic/dag_tabWithLock.h>
#include <render/burningDecals.h>
#include <render/clipmapDecals.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <shaders/dag_shaders.h>
#include <generic/dag_range.h>
#include <debug/dag_log.h>
#include <debug/dag_debug.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_render.h>
#include <3d/dag_materialData.h>
#include <math/dag_bounds2.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_frustum.h>
#include <math/integer/dag_IPoint2.h>
#include <gameRes/dag_gameResources.h>
#include <perfMon/dag_statDrv.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_direct.h>
#include <generic/dag_staticTab.h>
#include <memory/dag_framemem.h>
#include <math/dag_bounds2.h>
#include <3d/dag_quadIndexBuffer.h>
#include <util/dag_console.h>
#include <rendInst/rendInstGen.h>
#include <vecmath/dag_vecMathDecl.h>


BurningDecals::BurningDecals()
{
  mem_set_0(instData);
  inited = false;

  decals.resize(decals_count);

  material.init("burning_decal_base", NULL, 0, "burning decal shader");

  clipmapDecalType = clipmap_decals_mgr::createDecalType(::get_tex_gameres("explosion_patch_ship_soot_c", false), BAD_TEXTUREID,
    BAD_TEXTUREID, 1000, "clipmap_decal_burn");
  clipmap_decals_mgr::createDecalSubType(clipmapDecalType, Point4(0, 0, 1, 1), 1);

  resolution = 1024;
}

BurningDecals::~BurningDecals()
{
  material.close();
  clear();
  if (inited)
    release();
}

void BurningDecals::createTexturesAndBuffers()
{
  inited = true;
  mem_set_0(decals);

  burningMap.set(d3d::create_tex(NULL, resolution, resolution, TEXFMT_R8G8 | TEXCF_RTARGET, 1, "burning_map_tex"), "burning_map_tex");

  bakedBurningMap.set(d3d::create_tex(NULL, resolution, resolution, TEXFMT_R8G8 | TEXCF_RTARGET, 1, "baked_burning_map_tex"),
    "baked_burning_map_tex");

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);
  {
    SCOPE_RENDER_TARGET;

    d3d::set_render_target(bakedBurningMap.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    d3d::set_render_target(burningMap.getTex2D(), 0);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);

    // transit to SRV immediately, sometimes it used right after clear
    d3d::resource_barrier({bakedBurningMap.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::resource_barrier({burningMap.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
  }
  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);

  String bufferName;
  bufferName.printf(0, "burning_decal_data_vs");

  decalDataVS.set(d3d::create_sbuffer(sizeof(Point4), decals_count * sizeof(InstData) / sizeof(Point4),
                    SBCF_DYNAMIC | SBCF_MAYBELOST | SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE, TEXFMT_A32B32G32R32F, bufferName),
    bufferName);

  {
    Tab<InstData> data;
    data.resize(decals_count);
    mem_set_0(data);

    void *destData = 0;
    bool ret = decalDataVS.getBuf()->lock(0, 0, &destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD);
    d3d_err(ret);
    if (ret && destData)
    {
      memcpy(destData, data.data(), sizeof(InstData) * decals_count);
      decalDataVS.getBuf()->unlock();
    }
  }
}

// release system
void BurningDecals::release() { index_buffer::release_quads_32bit(); }

// remove all
void BurningDecals::clear()
{
  mem_set_0(decals);
  activeCount = 0;

  needUpdate = false;

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

  decalDataVS.close();
  bakedBurningMap.close();
  burningMap.close();

  static int burning_trees_varId = ::get_shader_variable_id("burning_trees", true);
  ShaderGlobal::set_int(burning_trees_varId, 0);

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
  inited = false;
}

void BurningDecals::afterReset() {}


void BurningDecals::createDecal(const Point2 &pos, float rot, const Point2 &size, float strength, float duration)
{
  Point2 dir;
  sincos(rot, dir.y, dir.x);
  createDecal(pos, dir, size, strength, duration);
}

void BurningDecals::createDecal(const Point2 &pos, const Point2 &dir, const Point2 &size, float strength, float duration)
{
  if (!inited)
    createTexturesAndBuffers();

  // try to add decals one to another if possible to avoid decal spam in one place if decal is small
  if (min(size.x, size.y) < 10.0f)
    for (int i = 0; i < activeCount; i++)
    {
      BurningDecal &_decal = decals[i];

      if (_decal.lifetime < 0 || _decal.clipmapDecalsCreated)
        continue;

      Point2 dist = pos - _decal.pos;
      if (dist.lengthF() > 0.75 * min(_decal.localX.lengthF(), _decal.localY.lengthF()) ||
          min(size.x, size.y) > 1.5 * min(_decal.localX.lengthF(), _decal.localY.lengthF()))
        continue;
      else
      {
        float sizeShrink = min(size.x, size.y) / min(_decal.localX.lengthF(), _decal.localY.lengthF());

        float newLenX = _decal.localX.lengthF() + 0.15 * size.x * sizeShrink * sizeShrink;
        _decal.localX.normalize();
        _decal.localX *= newLenX;
        float newLenY = _decal.localY.lengthF() + 0.15 * size.y * sizeShrink * sizeShrink;
        _decal.localY.normalize();
        _decal.localY *= newLenY;

        _decal.clipmapDecalsSize += 0.15 * min(size.x, size.y) * sizeShrink * sizeShrink;

        float additionalDuration = 0.15 * duration * sizeShrink * sizeShrink;
        _decal.lifetime += additionalDuration * _decal.lifetime / _decal.initialLifetime;
        _decal.initialLifetime += additionalDuration;

        _decal.strength += 0.5 * strength * sizeShrink * sizeShrink;

        needUpdate = true;
        return;
      }
    }

  int newDecal = -1;

  // use closest empty decal if possible
  for (int i = 0; i < activeCount; i++)
  {
    BurningDecal &_decal = decals[i];

    if (_decal.lifetime < 0)
    {
      newDecal = i;
      break;
    }
  }

  if (newDecal < 0)
  {
    if (activeCount < decals.size())
    {
      newDecal = activeCount;
    }
    else
      return; // discard decal if we already take all space
  }


  BurningDecal &decal = decals[newDecal];

  decal.pos = pos;
  decal.localX = size.x * dir;
  decal.localY = size.y * Point2(-dir.y, dir.x);
  decal.lifetime = duration;
  decal.initialLifetime = duration;
  decal.strength = strength;
  decal.clipmapDecalsCreated = false;
  decal.clipmapDecalsCount = (int)(1.5f * max(size.x / size.y, size.y / size.x));
  decal.clipmapDecalsDir = size.x > size.y ? decal.localX : decal.localY;
  decal.clipmapDecalsSize = 1.5f * min(size.x, size.y);

  activeCount = max(activeCount, newDecal + 1);

  static int burning_trees_varId = ::get_shader_variable_id("burning_trees", true);
  ShaderGlobal::set_int(burning_trees_varId, 1);

  Point3 bboxMin = Point3(decal.pos.x - size.x, -100000.0f, decal.pos.y - size.y);
  Point3 bboxMax = Point3(decal.pos.x + size.x, 100000.0f, decal.pos.y + size.y);

  bbox3f decalBbox;

  decalBbox.bmin = v_ldu_p3(&bboxMin.x);
  decalBbox.bmax = v_ldu_p3(&bboxMax.x);

  rendinst::applyBurningDecalsToRi(decalBbox);

  needUpdate = true;
}

void BurningDecals::update(float dt)
{
  if (!needUpdate)
  {
    activeCount = 0;
    return;
  }

  needUpdate = false;

  static int world_to_burn_mask_varId = ::get_shader_variable_id("world_to_burn_mask", true);
  static int world_to_hmap_low_varId = ::get_shader_variable_id("world_to_hmap_low", true);
  ShaderGlobal::set_color4(world_to_burn_mask_varId, ShaderGlobal::get_color4_fast(world_to_hmap_low_varId));

  // bbox for update
  BBox2 updateBox;

  Tab<InstData> dataStatic, dataDynamic;
  dataStatic.resize(activeCount);
  dataDynamic.resize(activeCount);

  int renderDynamicCount = 0;
  int renderStaticCount = 0;

  for (int i = 0; i < activeCount; i++)
  {
    BurningDecal &decal = decals[i];

    if (decal.lifetime < 0)
      continue;

    decal.lifetime = max(0.0f, decal.lifetime - dt);

    BBox2 box;

    box[0] = Point2(decal.pos.x - abs(decal.localX.x) - abs(decal.localY.x), decal.pos.y - abs(decal.localX.y) - abs(decal.localY.y));
    box[1] = Point2(decal.pos.x + abs(decal.localX.x) + abs(decal.localY.x), decal.pos.y + abs(decal.localX.y) + abs(decal.localY.y));

    float lifer = decal.lifetime / decal.initialLifetime;

    updateBox += box;

    if (lifer < 0.5 && !decal.clipmapDecalsCreated)
    {
      Point2 startPos = decal.pos - decal.clipmapDecalsDir;
      for (int j = 0; j < decal.clipmapDecalsCount; j++)
      {
        float rndScale = 0.8f + 0.4f * (float)rnd_int(0, 100) / 100.0f;
        Point2 pos = startPos + ((float)j + 0.5f) * decal.clipmapDecalsDir * (2.0f / decal.clipmapDecalsCount);
        pos += 0.1 * decal.localX * (0.5f - 1.0f * (float)rnd_int(0, 100) / 100.0f) +
               0.1 * decal.localY * (0.5f - 1.0f * (float)rnd_int(0, 100) / 100.0f);
        clipmap_decals_mgr::createDecal(clipmapDecalType, pos, (float)rnd_int(0, 30) / 3.1415f,
          rndScale * Point2(decal.clipmapDecalsSize, decal.clipmapDecalsSize), 0);
      }

      decal.clipmapDecalsCreated = true;
    }

    if (decal.lifetime > 0)
    {
      dataDynamic[renderDynamicCount].localX = decal.localX;
      dataDynamic[renderDynamicCount].localY = decal.localY;
      dataDynamic[renderDynamicCount].tc = decal.tc;
      dataDynamic[renderDynamicCount].pos = Point4(decal.pos.x, decal.pos.y, decal.strength * (1.0f - lifer),
        decal.strength * sqrt(lifer) * clamp(7.0f - 7.0f * lifer, 0.0f, 1.0f));

      renderDynamicCount++;
    }
    else // lifetime == 0
    {
      dataStatic[renderStaticCount].localX = decal.localX;
      dataStatic[renderStaticCount].localY = decal.localY;
      dataStatic[renderStaticCount].tc = decal.tc;
      dataStatic[renderStaticCount].pos = Point4(decal.pos.x, decal.pos.y, decal.strength, 0);

      renderStaticCount++;

      decal.lifetime = -1; // decal is not used in render anymore
    }
    needUpdate = true; // we still have dynamic decals
  }

  if (!needUpdate)
    return;

  d3d::setvsrc_ex(0, NULL, 0, 0);
  index_buffer::Quads32BitUsageLock lock;

  if (renderStaticCount > 0)
  {
    void *destData = 0;
    // lock and rewrite all buffer contents
    if (!decalDataVS.getBuf()->lock(0, 0, &destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      debug("%s can't lock buffer for decals", d3d::get_last_error());
      return;
    }

    memcpy(destData, dataStatic.data(), sizeof(InstData) * renderStaticCount);
    decalDataVS.getBuf()->unlock();

    SCOPE_RENDER_TARGET;

    d3d::set_render_target(bakedBurningMap.getTex2D(), 0);

    d3d::set_buffer(STAGE_VS, 8, decalDataVS.getBuf());

    if (!material.shader->setStates(0, true))
      return;

    d3d::drawind(PRIM_TRILIST, 0, renderStaticCount * 2, 0);

    bakedBurningMap.setVar();
  }

  d3d::stretch_rect(bakedBurningMap.getTex2D(), burningMap.getTex2D());
  d3d::resource_barrier({bakedBurningMap.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  if (renderDynamicCount > 0)
  {
    void *destData = 0;
    // lock and rewrite all buffer contents
    if (decalDataVS.getBuf()->lock(0, 0, &destData, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      memcpy(destData, dataDynamic.data(), sizeof(InstData) * renderDynamicCount);
      decalDataVS.getBuf()->unlock();

      SCOPE_RENDER_TARGET;

      d3d::set_render_target(burningMap.getTex2D(), 0);

      d3d::set_buffer(STAGE_VS, 8, decalDataVS.getBuf());

      if (material.shader->setStates(0, true))
      {
        d3d::drawind(PRIM_TRILIST, 0, renderDynamicCount * 2, 0);

        burningMap.setVar();
      }
    }
    else
      debug("%s can't lock buffer for decals", d3d::get_last_error());
  }

  d3d::resource_barrier({burningMap.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
}

namespace burning_decals_mgr
{
static eastl::unique_ptr<BurningDecals> burningDecals;
void init()
{
  if (!burningDecals)
    burningDecals = eastl::make_unique<BurningDecals>();
}

// release system
void release() { burningDecals.reset(); }
// remove all decals from map
void clear()
{
  if (burningDecals)
    burningDecals->clear();
}

void update(float dt)
{
  if (burningDecals)
    burningDecals->update(dt);
}

void createDecal(const Point2 &pos, const Point2 &dir, const Point2 &size, float strength, float duration)
{
  if (burningDecals)
    burningDecals->createDecal(pos, dir, size, strength, duration);
}

void createDecal(const Point2 &pos, float rot, const Point2 &size, float strength, float duration)
{
  if (burningDecals)
    burningDecals->createDecal(pos, rot, size, strength, duration);
}

void after_reset()
{
  if (burningDecals)
    burningDecals->afterReset();
}

void set_resolution(int res)
{
  if (burningDecals)
    burningDecals->setResolution(res);
}
} // namespace burning_decals_mgr
