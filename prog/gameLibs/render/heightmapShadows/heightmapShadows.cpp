// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <util/dag_convar.h>
#include <render/heightmapShadows/heightmapShadows.h>
#include "shaders/heightmapShadows.hlsli"
#include <shaders/dag_computeShaders.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_resPtr.h>
#include <perfMon/dag_statDrv.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderVar.h>
#include <generic/dag_carray.h>
// + soft penumbra
// + different sun angles
// todo: toroidal
// + z-pos
// + x-axis
// + x + z-pos -axis
// + z-neg sparse (several steps)
// + valid box sampling
// + light change support
// + invalidate all (everything)
// + reset device support
// * partial invalidate box areas (using sampling of current cascade values!)
// - light resampling change support (to reduce popping)
// + split read from previous/current slice -> write to next slice
// + sparse update
// + fade between cascades
// + dither between cascades
// + rasterize lmesh depth of higher quality
// todo: different cascades types (with/without depth above)
// todo: interval for warp32 (should be almost twice faster in real cases)
// todo: check shadows quality on hmap

CONSOLE_BOOL_VAL("shadow", hshd_toroidal, true);

enum
{
  FILTER_SIZE = 3
}; // must match shader
enum
{
  MAX_CASCADES_VARS = MAX_HMAP_SHADOW_CASCADES
}; // must match shader

#define GLOBAL_VARS_LIST                \
  VAR(heightmap_shadow_height_encoding) \
  VAR(heightmap_shadow_tc_ofs_sz)       \
  VAR(heightmap_shadow_world_lt_szi)    \
  VAR(heightmap_shadow_szi_np2)         \
  VAR(heightmap_shadow_move)            \
  VAR(heightmap_shadow_calc_z)          \
  VAR(heightmap_shadow_cascade)         \
  VAR(heightmap_shadow_render_penumbra) \
  VAR(heightmap_shadow_shadow_step)     \
  VAR(heightmap_shadow_dir_from_sun)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR


static Point2 rotate_point(const Point2 &p, const Point2 &dir_xz)
{
  return Point2(dir_xz.y * p.x + dir_xz.x * p.y, -dir_xz.x * p.x + dir_xz.y * p.y);
}

static BBox2 rotate_box(const BBox2 &b, const Point2 &dir_xz)
{
  Point2 p0 = rotate_point(b[0], dir_xz), p1 = rotate_point(b[1], dir_xz), p2 = rotate_point(Point2(b[0].x, b[1].y), dir_xz),
         p3 = rotate_point(Point2(b[1].x, b[0].y), dir_xz);
  return BBox2(min(min(p0, p1), min(p2, p3)), max(max(p0, p1), max(p2, p3)));
}

static BBox2 cascade_box(const IBBox2 &texels, float texelSize, const Point2 &dir_xz, float border = 0.5)
{
  Point2 b(border, border);
  const IPoint2 w = texels.width();
  BBox2 ret = rotate_box(BBox2(b * texelSize, (point2(w) - b) * texelSize), dir_xz);
  const Point2 lt = rotate_point(point2(texels[0]) * texelSize, dir_xz);
  ret[0] += lt;
  ret[1] += lt;
  return ret;
}


uint32_t HeightmapShadows::renderCascade(const UpdateCascade &update, int steps, const hmap_shadows_prepare_cb &cb)
{
  G_STATIC_ASSERT(MAX_POSSIBLE_CASCADES >= MAX_HMAP_SHADOW_CASCADES);
  DA_PROFILE_GPU;
  auto &c = cascades[update.cascade];
  const float texelSizeToBest = powf(c.scale, update.cascade);
  const float cascadeSize = c.dist0 * texelSizeToBest;
  const float texelSize = cascadeSize / w;

  // debug("%d: dir %@ pos %@", i, from_sun_direction_xz, newPos);
  IPoint2 move = update.newPos - update.oldPos;
  const int maxMove = max(abs(move.x), abs(move.y));
  enum
  {
    MOVE_ALL = 0,
    MOVE_X = 1,
    MOVE_Z_POS = 2,
    MOVE_Z_NEG = 3
  } moveState;
  moveState = maxMove >= w ? MOVE_ALL : move.y == 0 ? MOVE_X : move.x == 0 ? (move.y > 0 ? MOVE_Z_POS : MOVE_Z_NEG) : MOVE_ALL;
  if (!hshd_toroidal)
    moveState = MOVE_ALL;

  const int startZ = moveState == MOVE_Z_POS ? w - move.y : 0;
  const int startX = moveState == MOVE_X && move.x > 0 ? w - move.x : 0, endX = moveState == MOVE_X && move.x < 0 ? -move.x : w;
  const int bufW = (w + FILTER_SIZE * 2);
  const int bufRow = bufW * 12, lastRowOfs = bufW * 8 + update.cascade * bufRow * 2;

  // fixme:
  c.alignedPos = update.newPos;
  c.lastSunDirXZ = update.from_sun_direction_xz;
  c.fromSunDirYTan = update.fromSunDirYTan;

  const float maxGradientDist = (htMax - htMin) / max(1e-6f, fabsf(c.fromSunDirYTan));
  c.maxPenumbra = max(1e-9f, min(maxGradientDist, cascadeSize) * sunSizeTan * htScaleOfs.mul);

  setVars(); // fixme:should not be called for current

  ShaderGlobal::set_int(heightmap_shadow_cascadeVarId, update.cascade);
  ShaderGlobal::set_color4(heightmap_shadow_shadow_stepVarId, update.from_sun_direction_xz.x * texelSize,
    update.from_sun_direction_xz.y * texelSize, update.fromSunDirYTan * texelSize * htScaleOfs.mul, texelSize);

  ShaderGlobal::set_color4(heightmap_shadow_render_penumbraVarId, sunSizeTan * texelSize * htScaleOfs.mul, texelSizeToBest,
    c.maxPenumbra, 1.f / c.maxPenumbra);
  ShaderGlobal::set_int4(heightmap_shadow_moveVarId, startX, endX, moveState, 0);
  const int changedW = endX - startX;
  d3d::resource_barrier({heightmap_shadow_start.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::set_rwbuffer(STAGE_CS, 0, heightmap_shadow_start.getBuf());

  // todo: start as first step, than iterate till end
  // todo: change start so it doesn't use lt var for _current_ var

  const int effectiveSize = (calc_heightmap_shadows_cs->getThreadGroupSizes()[0] - (2 * FILTER_SIZE));
  const int startAtZ = update.updatedZRows < w ? update.updatedZRows : startZ;
  const int stepsToTrace = max<int>(w / max<int>(steps, 1), 1);
  const int stopAtZ = min<int>(startAtZ + stepsToTrace, w);
  const int preSteps = update.cascade == cascades.size() - 1 ? 32 : 4;
  G_ASSERT(stopAtZ > startAtZ);

  IBBox2 nextValid;
  const IPoint2 lt = update.newPos - IPoint2(w, w) / 2;
  if (stopAtZ == w)
    nextValid = IBBox2(lt, lt + IPoint2(w, w));
  else if (moveState == MOVE_Z_POS || moveState == MOVE_ALL)
    nextValid = IBBox2(lt, lt + IPoint2(w, stopAtZ));
  else if (moveState == MOVE_Z_NEG)
    nextValid = IBBox2(lt + IPoint2(0, -move.y), lt + IPoint2(w, w));
  else // if (moveState == MOVE_X)
  {
    G_ASSERT(moveState == MOVE_X); // -V547
    nextValid = IBBox2(lt + IPoint2(move.x > 0 ? 0 : -move.x, 0), lt + IPoint2(move.x < 0 ? w : w - move.x, w));
  }

  IBBox2 needed(lt - IPoint2(FILTER_SIZE, 0), lt + IPoint2(FILTER_SIZE + w, w));
  needed[0].y -= preSteps;
  IBBox2 neededNow(lt + IPoint2(startX - FILTER_SIZE, startAtZ), lt + IPoint2(endX + FILTER_SIZE, stopAtZ));
  if (moveState != MOVE_Z_POS && update.updatedZRows >= w)
    neededNow[0].y -= preSteps;

  cb(cascade_box(needed, texelSize, c.lastSunDirXZ, -0.5f), cascade_box(neededNow, texelSize, c.lastSunDirXZ, -0.5f), update.cascade,
    update.updatedZRows > 0 && update.updatedZRows < w);

  if (moveState != MOVE_Z_POS && update.updatedZRows >= w)
  {
    ShaderGlobal::set_int4(heightmap_shadow_calc_zVarId, startZ, preSteps, lastRowOfs + c.lastRow * bufRow,
      lastRowOfs + (1 - c.lastRow) * bufRow);
    c.lastRow = 1 - c.lastRow;

    {
      TIME_D3D_PROFILE(start);
      start_heightmap_shadows_cs->dispatchThreads(changedW + FILTER_SIZE * 2, 1, 1);
    }
  }

  d3d::set_rwtex(STAGE_CS, 1, heightmap_shadow.getArrayTex(), 0, 0, false);
  // d3d::set_rwtex(STAGE_CS, 0, heightmap_shadow.getArrayTex(), 0, 0, false);
  // if (update.cascade == 3)
  //   debug("cascade %d, move %@ updated %d, x %d..%d zStart %d/%d .. %d", update.cascade, move, update.updatedZRows, startX, endX,
  //   startZ, startAtZ, stopAtZ);
  // for (int z = startZ, e = w; z < e; z += stepsToTrace)
  {
    TIME_D3D_PROFILE(step);
    ShaderGlobal::set_int4(heightmap_shadow_calc_zVarId, startAtZ, stopAtZ, lastRowOfs + c.lastRow * bufRow,
      lastRowOfs + (1 - c.lastRow) * bufRow);
    // ShaderGlobal::set_int4(heightmap_shadow_calc_zVarId, z, min<int>(z + stepsToTrace, w), lastRowOfs + c.lastRow * bufRow,
    // lastRowOfs + (1 - c.lastRow) * bufRow);
    c.lastRow = 1 - c.lastRow;
    calc_heightmap_shadows_cs->dispatch((changedW + effectiveSize - 1) / effectiveSize, 1, 1);
  }
  d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0, false);
  if (stopAtZ == w)
  {
    if (c.finRow != c.lastRow && (startX != 0 || endX != w))
    {
      ShaderGlobal::set_int4(heightmap_shadow_calc_zVarId, startAtZ, stopAtZ, lastRowOfs + c.lastRow * bufRow,
        lastRowOfs + (1 - c.lastRow) * bufRow);
      c.lastRow = 1 - c.lastRow;
      copy_last_row_heightmap_shadows_cs->dispatchThreads(changedW, 1, 1);
    }
    else
      c.finRow = c.lastRow;
  }
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);

  auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
  d3d::resource_barrier({heightmap_shadow.getBaseTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});

  c.valid = nextValid;
  // return w;
  return stopAtZ;
}

void HeightmapShadows::updateCascades(const Point3 &pos, const Point3 &from_sun_dir, const hmap_shadows_prepare_cb &cb, int hshd_steps)
{
  const uint32_t currentCounter = get_d3d_reset_counter();
  if (d3dResetCounter != currentCounter)
  {
    invalidate();
    d3dResetCounter = currentCounter;
  }

  DA_PROFILE_GPU;
  if (moving.cascade < cascades.size() && moving.updatedZRows < w)
  {
    moving.updatedZRows = renderCascade(moving, hshd_steps, cb);
    if (moving.updatedZRows >= w)
      --moving.cascade;
    return;
  }
  UpdateCascade uc;
  if (moving.cascade < uint32_t(cascades.size() - 1))
  {
    uc.fromSunDirYTan = cascades[moving.cascade + 1].fromSunDirYTan;
    uc.from_sun_direction_xz = cascades[moving.cascade + 1].lastSunDirXZ;
  }
  else
  {
    const float len2 = length(Point2::xz(from_sun_dir));
    uc.fromSunDirYTan = from_sun_dir.y / max(1e-12f, len2);
    uc.from_sun_direction_xz = len2 < 1e-9f ? Point2(0, 1) : Point2::xz(from_sun_dir) / max(1e-9f, len2);
  }
  //-fixme: do that once

  for (int i = cascades.size() - 1; i >= 0; --i)
  {
    auto &c = cascades[i];
    const float texelSize = dist0 / w * powf(scale, i);
    const bool invalidateTexel = c.dist0 != dist0 || c.scale != scale;
    c.dist0 = dist0;
    c.scale = scale;
    IPoint2 newPos = ipoint2(floor(Point2(uc.from_sun_direction_xz.y * pos.x - uc.from_sun_direction_xz.x * pos.z,
                                     uc.from_sun_direction_xz.x * pos.x + uc.from_sun_direction_xz.y * pos.z) /
                                     texelSize +
                                   Point2(0.5, 0.5)));
    // fixme: if only zenith(uc.fromSunDirYTan) is changed, we can lazily update without invalidation
    const bool invalidateLight =
      (fabsf(uc.fromSunDirYTan - c.fromSunDirYTan) > 1e-7f || dot(uc.from_sun_direction_xz, c.lastSunDirXZ) < 0.9999f);
    const bool buildAll = invalidateLight || invalidateTexel;
    if (buildAll)
    {
      c.alignedPos = newPos + IPoint2(w * 2, 0);
    }
    IPoint2 move = newPos - c.alignedPos, absMove = abs(move);
    if (max(absMove.x, absMove.y) <= 1)
      continue;
    if (hshd_toroidal)
    {
      if (max(absMove.x, absMove.y) < w * 3 / 4)
      {
        if (absMove.x < absMove.y)
          move.x = 0;
        else
          move.y = 0;
      }
    }
    if (max(abs(move.x), absMove.y) <= 1)
      continue;
    uc.newPos = c.alignedPos + move;
    uc.oldPos = c.alignedPos;
    uc.updatedZRows = w;
    uc.cascade = i;
    moving = uc;
    const int steps = buildAll && i == cascades.size() - 1 ? 1 : hshd_steps;
    moving.updatedZRows = renderCascade(moving, steps, cb);
    if (moving.updatedZRows >= w)
    {
      moving.cascade = i - 1;
    }
    if (moving.updatedZRows < w || hshd_steps > 1)
      break;
  }
  setVars();
}

void HeightmapShadows::render(const Point3 &pos, const Point3 &from_sun_dir, const hmap_shadows_prepare_cb &cb, int hshd_steps)
{
  if (!calc_heightmap_shadows_cs || !heightmap_shadow)
    return;
  updateCascades(pos, from_sun_dir, cb, clamp<int>(hshd_steps, 1, w / 4));
}

void HeightmapShadows::close()
{
  heightmap_shadow.close();
  heightmap_shadow_start.close();
  cascades.clear();
  setVars();
}

void HeightmapShadows::init(int w_, int cascades_cnt, float dist0_, float scale_, const Point2 &ht_min_max, bool use_16bit)
{
  cascades_cnt = clamp<int>(cascades_cnt, 1, MAX_HMAP_SHADOW_CASCADES);
  uint32_t oldFmt = 0;
  if (heightmap_shadow)
  {
    TextureInfo ti;
    heightmap_shadow.getArrayTex()->getinfo(ti, 0);
    oldFmt = ti.cflg & TEXFMT_MASK;
  }
  const uint32_t texFmt = use_16bit ? TEXFMT_G16R16 : TEXFMT_G32R32F;
  if (oldFmt != texFmt || w != w_ || cascades_cnt != cascades.size())
  {
    heightmap_shadow.close();
    heightmap_shadow = dag::create_array_tex(w_, w_, cascades_cnt, TEXCF_UNORDERED | texFmt, 1, "heightmap_shadow");
  }
  if (w_ != w)
  {
    const int bufW = FILTER_SIZE * 2 + w_;
    heightmap_shadow_start.close();
    heightmap_shadow_start = dag::create_sbuffer(sizeof(uint32_t), bufW * (2 + MAX_HMAP_SHADOW_CASCADES * 3 * 2),
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW, 0, "heightmap_shadow_start");
  }

  w = w_;
  cascades.resize(cascades_cnt);
  dist0 = dist0_;
  scale = scale_;
  setMinMax(ht_min_max);
  for (auto &c : cascades)
  {
    c.dist0 = dist0;
    c.scale = scale;
  }

  constexpr int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(get_shader_variable_id("heightmap_shadow_szi_np2", true), w, is_pow_of2(w) ? 0 : (max_pos / w) * w,
    cascades_cnt, cascades_cnt - 1);
#define CS(a)                        \
  {                                  \
    a.reset(new_compute_shader(#a)); \
    G_ASSERT(a);                     \
  }
  CS(copy_last_row_heightmap_shadows_cs);
  CS(calc_heightmap_shadows_cs);
  CS(start_heightmap_shadows_cs);
#undef CS

  invalidate();
}

void HeightmapShadows::setVars()
{
  carray<IPoint4, MAX_CASCADES_VARS> v;
  carray<Point4, MAX_CASCADES_VARS> tc;
  carray<Point4, MAX_CASCADES_VARS> fromSun;
  for (int i = 0; i < cascades.size(); ++i)
  {
    auto &c = cascades[i];
    const float cascadeSize = powf(c.scale, i) * c.dist0;
    const float texelSize = cascadeSize / w;
    const float invCascadeSize = 1.f / cascadeSize;
    v[i] = IPoint4(c.alignedPos.x - w / 2, c.alignedPos.y - w / 2, bitwise_cast<int>(texelSize), bitwise_cast<int>(c.maxPenumbra));
    fromSun[i] = {c.lastSunDirXZ.x, c.lastSunDirXZ.y, c.lastSunDirXZ.x * invCascadeSize, c.lastSunDirXZ.y * invCascadeSize};
    IPoint2 dim = c.valid[1] - c.valid[0];
    if (dim.x > 0 && dim.y > 0)
    {
      Point2 lt = (point2(c.valid[0]) + Point2(0.5, 0.5)) / w, rb = (point2(c.valid[1]) - Point2(0.5, 0.5)) / w;
      Point2 center = (lt + rb) * 0.5f, width = rb - center;
      tc[i] = {-center.x / width.x, -center.y / width.y, 1.f / width.x, 1.f / width.y};
    }
    else
    {
      tc[i] = {-10000000, +100000, 0, 0};
    }
  }
  for (int i = cascades.size(); i < v.size(); ++i)
  {
    v[i] = IPoint4(-10000000, 10000000, 0, 0);
    tc[i] = Point4(0, 0, -1, 0);
    fromSun[i] = Point4(0, 0, 0, 0);
  }

  ShaderGlobal::set_int4_array(heightmap_shadow_world_lt_sziVarId, v.data(), v.size());
  ShaderGlobal::set_color4_array(heightmap_shadow_tc_ofs_szVarId, tc.data(), tc.size());
  ShaderGlobal::set_color4_array(heightmap_shadow_dir_from_sunVarId, fromSun.data(), fromSun.size());
}

void HeightmapShadows::setDist(float d) { dist0 = d; }
void HeightmapShadows::setScale(float scale_) { scale = scale_; }
Point2 HeightmapShadows::getHtMinMax() const { return Point2(htMin, htMax); }
void HeightmapShadows::setMinMax(const Point2 &ht_min_max)
{
  htMin = ht_min_max.x;
  htMax = ht_min_max.y;
  const float maxTexelSize = powf(scale, cascades.size() - 1) * dist0 / w;
  Point2 htOfs(ht_min_max.y - ht_min_max.x + 2 * maxTexelSize, ht_min_max.x - maxTexelSize);
  htScaleOfs = HtScaleOfs{1.f / max(1e-6f, htOfs.x), -htOfs.y / max(1e-6f, htOfs.x)};
  ShaderGlobal::set_color4(heightmap_shadow_height_encodingVarId, htScaleOfs.mul, htScaleOfs.ofs, htScaleOfs.mul * constBlurSize, 0);
  invalidate();
}

void HeightmapShadows::invalidate()
{
  d3dResetCounter = 0;
  for (auto &c : cascades)
  {
    c.alignedPos += IPoint2(-100000, 100000);
    c.lastSunDirXZ = Point2(c.lastSunDirXZ.y, c.lastSunDirXZ.x);
    c.fromSunDirYTan = 1.f;
    c.valid = IBBox2();
    c.scale = scale + 1;
    c.dist0 = dist0 + 1;
  }
  moving.cascade = ~0u;
  moving.updatedZRows = w;
  setVars();
}

float HeightmapShadows::actualCascadeDist(uint32_t cascade) const
{
  if (cascade >= uint32_t(cascades.size()))
    return 0;
  auto &c = cascades[cascade];
  return c.dist0 * powf(c.scale, cascade);
}

float HeightmapShadows::cascadeDist(uint32_t cascade) const { return dist0 * powf(scale, cascade); }

BBox2 HeightmapShadows::cascadeBox(uint32_t cascade) const
{
  if (cascade >= uint32_t(cascades.size()))
    return BBox2();
  auto &c = cascades[cascade];
  const IPoint2 lt = c.alignedPos - IPoint2(w / 2, w / 2);
  return cascade_box(IBBox2(lt, lt + IPoint2(w, w)), c.dist0 * powf(c.scale, cascade) / w, c.lastSunDirXZ);
}

BBox2 HeightmapShadows::cascadeValidBox(uint32_t cascade) const
{
  if (cascade >= uint32_t(cascades.size()))
    return BBox2();
  auto &c = cascades[cascade];
  if (!c.isValid())
    return BBox2();
  return cascade_box(c.valid, c.dist0 * powf(c.scale, cascade) / w, c.lastSunDirXZ);
}

HeightmapShadows::HeightmapShadows() = default;
HeightmapShadows::~HeightmapShadows() = default;
