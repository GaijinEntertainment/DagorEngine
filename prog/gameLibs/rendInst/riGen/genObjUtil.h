#pragma once

#include <rendInst/rendInstGen.h>
#include <rendInst/rotation_palette_consts.hlsli>
#include "riGenExtra.h"
#include "riRotationPalette.h"

#include <util/dag_roHugeHierBitMap2d.h>
#include <math/integer/dag_IBBox2.h>
#include <math/random/dag_random.h>
#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include "multiPointData.h"
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_spinlock.h>

namespace rendinstgen
{
extern float custom_max_trace_distance;
extern bool custom_trace_ray(const Point3 &src, const Point3 &dir, real &dist, Point3 *out_norm = NULL);
extern bool custom_get_height(Point3 &pos, Point3 *out_norm = NULL);
extern vec3f custom_update_pregen_pos_y(vec4f pos, int16_t *dest_packed_y, float csz_y, float oy);
extern void custom_get_land_min_max(BBox2 bbox_xz, float &out_min, float &out_max);

alignas(16) static const vec4f_const VC_1div256 = v_splats(1.0f / 256.0f);
alignas(16) static const vec4f_const VC_1div256m64K = v_splats(1.0f / 256.0f / 65536.0f);
alignas(16) static const vec4f_const VC_1div64K = v_splats(1.0f / 65536.0f);
alignas(16) static const vec4f_const VC_1div32767 = v_splats(1.0f / 32767.0f);
alignas(16) static const vec4f_const VC_256div32767 = v_splats(256.0f / 32767.0f);
alignas(16) static const vec4f_const VC_1div10 = v_splats(0.1f);
static constexpr float MAX_SCALE = 20;
static const uint16_t PALETTE_BITS = PALETTE_ID_BIT_COUNT;
static const uint16_t PALETTE_ID_MASK = (1 << PALETTE_BITS) - 1;
#if _TARGET_SIMD_SSE
#define REPLICATE4(X) X, X, X, X
alignas(16) static const vec4i_const VCI_PALETTE_ID_MASK = {REPLICATE4(PALETTE_ID_MASK)};
alignas(16) static const vec4i_const VCI_SCALE_MASK = {REPLICATE4(0xFFFF & ~PALETTE_ID_BIT_COUNT)};
#undef REPLICATE4
#elif _TARGET_SIMD_NEON
alignas(16) static const vec4i_const VCI_PALETTE_ID_MASK = v_splatsi(PALETTE_ID_MASK);
alignas(16) static const vec4i_const VCI_SCALE_MASK = v_splatsi(0xFFFF & ~PALETTE_ID_BIT_COUNT);
#endif

struct InstancePackData
{
  float ox, oy, oz, cell_xz_sz, cell_y_sz;
  int per_inst_data_dwords;
};

static inline vec4f pack_entity_data(vec4f pos, vec4f scale, vec4i palette_id)
{
  palette_id = v_maxi(palette_id, v_splatsi(0)); // palette_id = -1 if the RI has no palette id
  G_ASSERT(v_extract_wi(palette_id) <= PALETTE_ID_MASK);
  scale = v_mul(v_floor(v_mul(scale, v_splats(1 << (8 - PALETTE_ID_BIT_COUNT)))), v_splats(1.0f / (1 << (8 - PALETTE_ID_BIT_COUNT))));
  return v_perm_xyzd(pos, v_add(scale, v_mul(v_cvt_vec4f(palette_id), VC_1div256)));
}

static inline int16_t pack_scale_palette_id_16(float scale, int32_t palette_id)
{
  G_ASSERT(palette_id <= PALETTE_ID_MASK);
  return (static_cast<int16_t>(scale * 256.0f) & ~PALETTE_ID_MASK) | min(uint16_t(palette_id), PALETTE_ID_MASK);
}

static inline void unpack_scale_palette_id_16(uint16_t value, float &scale, int32_t &palette_id)
{
  palette_id = value & PALETTE_ID_MASK;
  scale = (value & ~PALETTE_ID_MASK) / 256.f;
}

static inline void unpack_scale_palette_id_v4f_16(vec4i value, vec4f &scale, vec4i &palette_id)
{
  palette_id = v_andi(value, VCI_PALETTE_ID_MASK);
  scale = v_mul(v_cvt_vec4f(v_andi(value, VCI_SCALE_MASK)), VC_1div256);
}


static inline void unpack_tm_full(TMatrix &tm, const int16_t *data, float x0, float y0, float z0, float dxz, float dy)
{
  tm[0][0] = data[0] / 256.0f;
  tm[1][0] = data[1] / 256.0f;
  tm[2][0] = data[2] / 256.0f;
  tm[3][0] = data[3] * dxz / 32767.0 + x0;
  tm[0][1] = data[4] / 256.0f;
  tm[1][1] = data[5] / 256.0f;
  tm[2][1] = data[6] / 256.0f;
  tm[3][1] = data[7] * dy / 32767.0 + y0;
  tm[0][2] = data[8] / 256.0f;
  tm[1][2] = data[9] / 256.0f;
  tm[2][2] = data[10] / 256.0f;
  tm[3][2] = data[11] * dxz / 32767.0 + z0;
}
static inline void unpack_tm_pos_fast(TMatrix &tm, const int16_t *data, float x0, float y0, float z0, float dxz, float dy,
  bool palette_rotation, int32_t *palette_id = nullptr)
{
  memset(tm.array, 0, sizeof(TMatrix));
  if (palette_rotation)
  {
    float scale;
    int32_t paletteId;
    unpack_scale_palette_id_16(data[3], scale, paletteId);
    tm[0][0] = tm[1][1] = tm[2][2] = scale;
    if (palette_id)
      *palette_id = paletteId;
  }
  else
  {
    if (palette_id)
      *palette_id = -1;
    tm[0][0] = tm[1][1] = tm[2][2] = data[3] / 256.0f;
  }
  tm[3][0] = data[0] * dxz / 32767.0 + x0;
  tm[3][1] = data[1] * dy / 32767.0 + y0;
  tm[3][2] = data[2] * dxz / 32767.0 + z0;
}
static inline void unpack_tm32_full(TMatrix &tm, const int32_t *data, float x0, float y0, float z0, float dxz, float dy)
{
  tm[0][0] = data[0] / 256.0f / 65536.f;
  tm[1][0] = data[1] / 256.0f / 65536.f;
  tm[2][0] = data[2] / 256.0f / 65536.f;
  tm[3][0] = data[3] * dxz / 65536.f / 32767.0f + x0;
  tm[0][1] = data[4] / 256.0f / 65536.f;
  tm[1][1] = data[5] / 256.0f / 65536.f;
  tm[2][1] = data[6] / 256.0f / 65536.f;
  tm[3][1] = data[7] * dy / 65536.f / 32767.0f + y0;
  tm[0][2] = data[8] / 256.0f / 65536.f;
  tm[1][2] = data[9] / 256.0f / 65536.f;
  tm[2][2] = data[10] / 256.0f / 65536.f;
  tm[3][2] = data[11] * dxz / 65536.f / 32767.0f + z0;
}

static inline void unpack_tm_full(mat44f &out_tm, const int16_t *data, vec3f add, vec3f mul)
{
  mat43f m43;
  m43.row0 = v_cvt_vec4f(v_ldush(data + 0));
  m43.row1 = v_cvt_vec4f(v_ldush(data + 4));
  m43.row2 = v_cvt_vec4f(v_ldush(data + 8));

  mat44f m;
  v_mat43_transpose_to_mat44(m, m43);
  out_tm.col0 = v_mul(m.col0, VC_1div256);
  out_tm.col1 = v_mul(m.col1, VC_1div256);
  out_tm.col2 = v_mul(m.col2, VC_1div256);
  out_tm.col3 = v_madd(m.col3, mul, add);
}

static inline void unpack_tm_pos(mat44f &tm, const int16_t *data, vec3f add, vec3f mul, bool palette_rotation,
  vec4i *palette_id = nullptr)
{
  vec4i vi = v_ldush(data);
  vec4f v = v_cvt_vec4f(vi);
  vec4f scale;
  vec4i paletteId;
  if (palette_rotation)
  {
    unpack_scale_palette_id_v4f_16(v_splat_wi(vi), scale, paletteId);
    if (palette_id)
      *palette_id = paletteId;
  }
  else
  {
    if (palette_id)
      *palette_id = v_splatsi(-1);
    scale = v_mul(v_splat_w(v), VC_1div256);
  }
  tm.col0 = v_and(scale, (vec4f)V_CI_MASK1000);
  tm.col1 = v_and(scale, (vec4f)V_CI_MASK0100);
  tm.col2 = v_and(scale, (vec4f)V_CI_MASK0010);
  tm.col3 = v_madd(v, mul, add);
}
static inline void unpack_tm_pos(vec4f &pos, vec4f &scale, const int16_t *data, vec3f add, vec3f mul, bool palette_rotation,
  vec4i *palette_id = nullptr)
{
  vec4i vi = v_ldush(data);
  vec4f v = v_cvt_vec4f(vi);
  vec4i paletteId;
  if (palette_rotation)
  {
    unpack_scale_palette_id_v4f_16(v_splat_wi(vi), scale, paletteId);
    if (palette_id)
      *palette_id = paletteId;
  }
  else
  {
    if (palette_id)
      *palette_id = v_splatsi(-1);
    scale = v_mul(v_splat_w(v), VC_1div256);
  }
  pos = v_madd(v, mul, add);
}

static inline void unpack_tm32_full(mat44f &out_tm, const int32_t *data, vec3f add, vec3f mul)
{
  mat43f m43;
  m43.row0 = v_cvt_vec4f(v_cast_vec4i(v_ldu((float *)data + 0)));
  m43.row1 = v_cvt_vec4f(v_cast_vec4i(v_ldu((float *)data + 4)));
  m43.row2 = v_cvt_vec4f(v_cast_vec4i(v_ldu((float *)data + 8)));

  mat44f m;
  v_mat43_transpose_to_mat44(m, m43);
  out_tm.col0 = v_mul(m.col0, VC_1div256m64K);
  out_tm.col1 = v_mul(m.col1, VC_1div256m64K);
  out_tm.col2 = v_mul(m.col2, VC_1div256m64K);
  out_tm.col3 = v_madd(m.col3, v_mul(mul, VC_1div64K), add);
}

static inline size_t pack_entity_pos_inst_16(const InstancePackData &data, const Point3 &pos, float scale, int32_t palette_id,
  void *target_ptr)
{
  G_ASSERT_LOG(scale < MAX_SCALE, "RI scale=%f, pos=(%f,%f,%f)", scale, pos.x, pos.y, pos.z);
  int16_t *ptr = reinterpret_cast<int16_t *>(target_ptr);
  if (ptr)
  {
    ptr[0] = short((pos.x - data.ox) / data.cell_xz_sz * 32767.0);
    ptr[1] = short(clamp((pos.y - data.oy) / data.cell_y_sz, -1.f, 1.f) * 32767.0);
    ptr[2] = short((pos.z - data.oz) / data.cell_xz_sz * 32767.0);
    ptr[3] = palette_id >= 0 ? pack_scale_palette_id_16(scale, palette_id) : short(scale * 256);
  }
  return sizeof(*ptr) * 4;
}

static inline size_t pack_entity_tm_16(const InstancePackData &data, const TMatrix &tm, void *target_ptr)
{
  int16_t *ptr = reinterpret_cast<int16_t *>(target_ptr);
  if (ptr)
  {
    int index = 0;
    ptr[index++] = short(tm[0][0] * 256.0f);
    ptr[index++] = short(tm[1][0] * 256.0f);
    ptr[index++] = short(tm[2][0] * 256.0f);
    ptr[index++] = short((tm.m[3][0] - data.ox) / data.cell_xz_sz * 32767.0);
    ptr[index++] = short(tm[0][1] * 256.0f);
    ptr[index++] = short(tm[1][1] * 256.0f);
    ptr[index++] = short(tm[2][1] * 256.0f);
    ptr[index++] = short(clamp((tm.m[3][1] - data.oy) / data.cell_y_sz, -1.f, 1.f) * 32767.0);
    ptr[index++] = short(tm[0][2] * 256.0f);
    ptr[index++] = short(tm[1][2] * 256.0f);
    ptr[index++] = short(tm[2][2] * 256.0f);
    ptr[index++] = short((tm.m[3][2] - data.oz) / data.cell_xz_sz * 32767.0);

#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
    if (data.per_inst_data_dwords > 0)
    {
      // We assume that we have only 1 additional vector per instance
      // That means: perInstDataDwords <= 4
      int instSeed = mem_hash_fnv1((const char *)&tm[3][0], 12);
      ptr[index++] = instSeed & 0xFFFF; // set low 16bit of hashVal
      ptr[index++] = 0;
      ptr[index++] = 0;
      ptr[index++] = 0;
    }
#endif
  }
#if RIGEN_PERINST_ADD_DATA_FOR_TOOLS
  if (data.per_inst_data_dwords > 0)
    return sizeof(*ptr) * 16;
#endif
  return sizeof(*ptr) * 12;
}

static inline size_t pack_entity_tm_32(const InstancePackData &data, const TMatrix &tm, void *target_ptr)
{
  int32_t *ptr = reinterpret_cast<int32_t *>(target_ptr);
  if (ptr)
  {
    int index = 0;
    ptr[index++] = int32_t(tm[0][0] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[1][0] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[2][0] * 256.0f * 65536.0f);
    ptr[index++] = int32_t((tm.m[3][0] - data.ox) / data.cell_xz_sz * 32767.0 * 65536.0f);
    ptr[index++] = int32_t(tm[0][1] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[1][1] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[2][1] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(clamp((tm.m[3][1] - data.oy) / data.cell_y_sz, -1.f, 1.f) * 32767.0 * 65536.0f);
    ptr[index++] = int32_t(tm[0][2] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[1][2] * 256.0f * 65536.0f);
    ptr[index++] = int32_t(tm[2][2] * 256.0f * 65536.0f);
    ptr[index++] = int32_t((tm.m[3][2] - data.oz) / data.cell_xz_sz * 32767.0 * 65536.0f);
  }
  return sizeof(*ptr) * 12;
}

static inline size_t pack_entity_16(const InstancePackData &data, const TMatrix &tm, bool pos_inst, int32_t palette_id,
  void *target_ptr)
{
  if (pos_inst)
  {
    float scale = tm.getcol(1).length();
    G_ASSERT_LOG(scale < MAX_SCALE, "RI scale=%f, tm=(%f,%f,%f; %f,%f,%f; %f,%f,%f; %f,%f,%f)", scale, tm.m[0][0], tm.m[0][1],
      tm.m[0][2], tm.m[1][0], tm.m[1][1], tm.m[1][2], tm.m[2][0], tm.m[2][1], tm.m[2][2], tm.m[3][0], tm.m[3][1], tm.m[3][2]);
    return pack_entity_pos_inst_16(data, tm.getcol(3), scale, palette_id, target_ptr);
  }
  G_ASSERT(palette_id == -1);
  return pack_entity_tm_16(data, tm, target_ptr);
}

struct SingleEntityPool
{
  void *basePtr;
  void *topPtr;
  int avail;
  int shortage;

  static float ox, oy, oz, cell_xz_sz, cell_y_sz;
  static bbox3f bbox;
  static bbox3f *per_pool_local_bb;
  static dag::ConstSpan<const TMatrix *> sweep_boxes_itm;
  static dag::ConstSpan<E3DCOLOR> ri_col_pair;
  static int cur_cell_id, cur_ri_extra_ord;
  static int per_inst_data_dwords;

  static bool intersectWithSweepBoxes(float x, float z)
  {
    for (int i = 0; i < sweep_boxes_itm.size(); i++)
    {
      Point2 p;
      p.set_xz(sweep_boxes_itm[i][0] * Point3(x, 0, z));
      if (fabsf(p.x) < 1.f && fabsf(p.y) < 1.f)
        return true;
    }
    return false;
  }

  bool tryAdd(float x, float z)
  {
    if (intersectWithSweepBoxes(x, z))
      return false;

    if (avail != 0)
      return true;
    shortage++;
    return false;
  }
  void addEntity(const TMatrix &tm, bool posInst, int32_t pool_idx, int32_t palette_id)
  {
    if (avail > 0)
    {
      int16_t *ptr = reinterpret_cast<int16_t *>(topPtr);
      InstancePackData data{ox, oy, oz, cell_xz_sz, cell_y_sz, per_inst_data_dwords};
      size_t addedSize = pack_entity_16(data, tm, posInst, palette_id, ptr);
      G_ASSERT(addedSize % sizeof(*ptr) == 0);
      ptr += addedSize / sizeof(*ptr);

      mat44f m;
      v_mat44_make_from_43cu_unsafe(m, tm.m[0]);
      v_bbox3_add_transformed_box(bbox, m, per_pool_local_bb[pool_idx]);
      topPtr = ptr;
      avail--;
    }
    else if (avail < 0)
    {
      mat44f m;
      v_mat44_make_from_43cu_unsafe(m, tm.m[0]);
      if (!posInst && per_inst_data_dwords > 0)
      {
        int instSeed = mem_hash_fnv1((const char *)&tm[3][0], 12);
        rendinst::addRIGenExtra44(-avail - 1, false, m, true, cur_cell_id, cur_ri_extra_ord, 1, &instSeed);
      }
      else
        rendinst::addRIGenExtra44(-avail - 1, false, m, true, cur_cell_id, cur_ri_extra_ord);
      cur_ri_extra_ord += 16 * (rendinst::riExtra[-avail - 1].destrDepth + 1);
    }
  }
};

typedef RoHugeHierBitMap2d<4, 3> HugeBitmask;
typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> EditableHugeBitmask;

struct WorldHugeBitmask
{
  HugeBitmask *bm;
  float ox, oz, scale;
  int w, h; // in fact, just duplicates dimensions stored in bm

  WorldHugeBitmask() : bm(NULL), ox(0), oz(0), scale(1), w(0), h(0) {}
  ~WorldHugeBitmask() { clear(); }

  void initExplicit(float min_x, float min_z, float s, HugeBitmask *_bm)
  {
    clear();
    ox = min_x;
    oz = min_z;
    scale = s;
    bm = _bm;
    w = bm->getW();
    h = bm->getH();
  }

  void clear()
  {
    bm = NULL;
    ox = oz = 0;
    scale = 1;
    w = h = 0;
  }

  inline bool isMarked(float px, float pz)
  {
    if (!bm)
      return false;
    int ix = (int)floor((double(px) - ox) * scale);
    int iz = (int)floor((double(pz) - oz) * scale);

    if (ix < 0 || iz < 0 || ix >= w || iz >= h)
      return false;

    return bm->get(ix, iz);
  }
};

struct WorldEditableHugeBitmask
{
  EditableHugeBitmask bm;
  float ox, oz, scale;
  os_spinlock_t spinLock; // non-recursive
  bool constBm;

  WorldEditableHugeBitmask() : ox(0), oz(0), scale(1), constBm(false) { os_spinlock_init(&spinLock); }
  ~WorldEditableHugeBitmask() { os_spinlock_destroy(&spinLock); }

  void initExplicit(float min_x, float min_z, float s, float sz_x, float sz_z)
  {
    G_ASSERT(!constBm);
    os_spinlock_lock(&spinLock);

    clear(false);
    ox = min_x;
    oz = min_z;
    scale = s;
    bm.resize(int(ceilf(sz_x * s)), int(ceilf(sz_z * s)));

    os_spinlock_unlock(&spinLock);
  }

  void clear(bool lock_it = true)
  {
    if (lock_it)
      os_spinlock_lock(&spinLock);

    if (constBm)
      memset(&bm, 0, sizeof(bm));
    else
      bm.resize(0, 0);
    ox = oz = 0;
    scale = 1;
    constBm = false;

    if (lock_it)
      os_spinlock_unlock(&spinLock);
  }

  inline bool isMarked(float px, float pz)
  {
    os_spinlock_lock(&spinLock);

    bool ret = false;
    if (bm.getW())
    {
      int ix = (int)floor((double(px) - ox) * scale), iz = (int)floor((double(pz) - oz) * scale);
      ret = (ix < 0 || iz < 0 || ix >= bm.getW() || iz >= bm.getH()) ? false : bm.get(ix, iz);
    }

    os_spinlock_unlock(&spinLock);

    return ret;
  }

  void markBox(float x0, float z0, float x1, float z1)
  {
    os_spinlock_lock(&spinLock);

    int ix0 = (int)floorf((x0 - ox) * scale), ix1 = (int)floorf((x1 - ox) * scale);
    int iy = (int)floorf((z0 - oz) * scale), iy1 = (int)floorf((z1 - oz) * scale);
    if (ix0 < 0)
      ix0 = 0;
    if (iy < 0)
      iy = 0;
    if (ix1 >= bm.getW())
      ix1 = bm.getW() - 1;
    if (iy1 >= bm.getH())
      iy1 = bm.getH() - 1;

    for (; iy <= iy1; iy++)
      for (int ix = ix0; ix <= ix1; ix++)
        bm.set(ix, iy);

    os_spinlock_unlock(&spinLock);
  }

  void markTM(float x0, float z0, float x1, float z1, const TMatrix &check_itm)
  {
    os_spinlock_lock(&spinLock);
    BBox3 checkBox(Point3(-0.5f, 0.f, -0.5f), Point3(0.5f, 1.f, 0.5f));

    int ix0 = (int)floorf((x0 - ox) * scale), ix1 = (int)floorf((x1 - ox) * scale);
    int iy = (int)floorf((z0 - oz) * scale), iy1 = (int)floorf((z1 - oz) * scale);
    if (ix0 < 0)
      ix0 = 0;
    if (iy < 0)
      iy = 0;
    if (ix1 >= bm.getW())
      ix1 = bm.getW() - 1;
    if (iy1 >= bm.getH())
      iy1 = bm.getH() - 1;

    for (; iy <= iy1; iy++)
      for (int ix = ix0; ix <= ix1; ix++)
      {
        Point3 p = check_itm * Point3(ix / scale + ox, 0.f, iy / scale + oz);
        p.y = 0.5f;
        if (checkBox & p)
          bm.set(ix, iy);
      }

    os_spinlock_unlock(&spinLock);
  }

  void markCircle(float x0, float z0, float rad)
  {
    os_spinlock_lock(&spinLock);

    int ixc = (int)floorf((x0 - ox) * scale), iyc = (int)floorf((z0 - oz) * scale);
    int ix0 = (int)floorf((x0 - rad - ox) * scale), ix1 = (int)floorf((x0 + rad - ox) * scale);
    int iy = (int)floorf((z0 - rad - oz) * scale), iy1 = (int)floorf((z0 + rad - oz) * scale);
    int irad2 = max((int)floorf(rad * scale), 1);
    irad2 *= irad2;

    if (ix0 < 0)
      ix0 = 0;
    if (iy < 0)
      iy = 0;
    if (ix1 >= bm.getW())
      ix1 = bm.getW() - 1;
    if (iy1 >= bm.getH())
      iy1 = bm.getH() - 1;

    for (; iy <= iy1; iy++)
      for (int ix = ix0; ix <= ix1; ix++)
        if ((ix - ixc) * (ix - ixc) + (iy - iyc) * (iy - iyc) <= irad2)
          bm.set(ix, iy);

    os_spinlock_unlock(&spinLock);
  }

  void markPoint(float x0, float z0)
  {
    os_spinlock_lock(&spinLock);

    int ix = (int)floorf((x0 - ox) * scale);
    int iz = (int)floorf((z0 - oz) * scale);
    if (ix >= 0 && iz >= 0 && ix < bm.getW() && iz < bm.getH())
      bm.set(ix, iz);

    os_spinlock_unlock(&spinLock);
  }
};

extern WorldHugeBitmask lcmapExcl;
extern WorldEditableHugeBitmask destrExcl;

static inline void align_place_xz(Point3 &p)
{
  static const float eps = 0.00390625f; // 1/128
  float fx = floorf(p.x), fz = floorf(p.z);

  if (p.x - fx < eps)
    p.x = fx;
  else if (p.x - fx > 1.0f - eps)
    p.x = fx + 1;

  if (p.z - fz < eps)
    p.z = fz;
  else if (p.z - fz > 1.0f - eps)
    p.z = fz + 1;
}
static inline bool is_place_allowed(float px, float pz)
{
  return destrExcl.constBm ? !destrExcl.isMarked(px, pz) : !lcmapExcl.isMarked(px, pz);
}

static inline void place_on_ground(Point3 &p, float start_above = 0)
{
  real dist = custom_max_trace_distance + start_above;
  p.y += start_above;
  if (custom_trace_ray(p, Point3(0, -1, 0), dist, NULL))
    p.y -= dist;
  else
    p.y -= start_above;
}

static inline void place_on_ground(Point3 &p, Point3 &out_norm, float start_above = 0)
{
  float dist = custom_max_trace_distance + start_above;
  Point3 orig_norm = out_norm;
  p.y += start_above;
  if (custom_trace_ray(p, Point3(0, -1, 0), dist, &out_norm))
    p.y -= dist;
  else
  {
    p.y -= start_above;
    out_norm = orig_norm;
  }
}

__forceinline Matrix3 rotx_tm(float a)
{
  Matrix3 m;
  m.identity();
  m.m[1][1] = m.m[2][2] = cosf(a);
  m.m[1][2] = -(m.m[2][1] = sinf(a));
  return m;
}

__forceinline Matrix3 roty_tm(float a)
{
  Matrix3 m;
  m.identity();
  m.m[2][2] = m[0][0] = cosf(a);
  m.m[2][0] = -(m[0][2] = sinf(a));
  return m;
}

__forceinline Matrix3 rotz_tm(float a)
{
  Matrix3 m;
  m.identity();
  m.m[0][0] = m.m[1][1] = cosf(a);
  m.m[0][1] = -(m.m[1][0] = sinf(a));
  return m;
}
template <class T>
static void calc_matrix_33(T &obj, TMatrix &tm, int &seed, const RotationPaletteManager::Palette &palette, int32_t &palette_id)
{
  Point3 rotation;
  _rnd_svec(seed, rotation.z, rotation.y, rotation.x);
  float s = obj.scale[0] + _srnd(seed) * obj.scale[1];
  rotation.x = obj.rotX[0] + rotation.x * obj.rotX[1];
  rotation.y = obj.rotY[0] + rotation.y * obj.rotY[1];
  rotation.z = obj.rotZ[0] + rotation.z * obj.rotZ[1];
  rotation = RotationPaletteManager::clamp_euler_angles(palette, rotation, &palette_id);
  float sy = obj.yScale[0] + _srnd(seed) * obj.yScale[1];
  int mask = (float_nonzero(rotation.x) << 0) | (float_nonzero(rotation.y) << 1) | (float_nonzero(rotation.z) << 2) |
             (float_nonzero(s - 1) << 3) | (float_nonzero(sy - 1) << 4);
  if (!mask)
    return;

  Matrix3 &m3 = *(Matrix3 *)&tm;

  if (mask & 4)
    m3 = m3 * rotz_tm(rotation.z);
  if (mask & 1)
    m3 = m3 * rotx_tm(rotation.x);
  if (mask & 2)
    m3 = m3 * roty_tm(rotation.y);

  if (mask & (8 | 16))
  {
    Matrix3 stm;
    stm.setcol(0, Point3(s, 0, 0));
    stm.setcol(1, Point3(0, s * sy, 0));
    stm.setcol(2, Point3(0, 0, s));
    m3 = m3 * stm;
  }
}


static inline void place_multipoint(MpPlacementRec &mppRec, Point3 &worldPos, TMatrix &rot_tm, float start_above = 0)
{
  mppRec.mpPoint1 = rot_tm * mppRec.mpPoint1 + worldPos;
  place_on_ground(mppRec.mpPoint1, start_above);

  mppRec.mpPoint2 = rot_tm * mppRec.mpPoint2 + worldPos;
  place_on_ground(mppRec.mpPoint2, start_above);

  if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_3POINT)
  {
    mppRec.mpPoint3 = rot_tm * mppRec.mpPoint3 + worldPos;
    place_on_ground(mppRec.mpPoint3, start_above);
    worldPos = mppRec.mpPoint1 + (mppRec.mpPoint2 - mppRec.mpPoint1) * mppRec.pivotBc21 +
               (mppRec.mpPoint3 - mppRec.mpPoint1) * mppRec.pivotBc31;
  }
  else
    place_on_ground(worldPos, start_above);
}


static inline void rotate_multipoint(TMatrix &tm, MpPlacementRec &mppRec)
{
  Point3 v_orient = tm.getcol(1);
  TMatrix mpp_tm = TMatrix::IDENT;
  Point3 place_vec = normalize(mppRec.mpPoint1 - mppRec.mpPoint2);

  if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_YZ)
  {
    mpp_tm.setcol(0, normalize(v_orient % place_vec));
    mpp_tm.setcol(1, normalize(place_vec % mpp_tm.getcol(0)));
    mpp_tm.setcol(2, normalize(mpp_tm.getcol(0) % mpp_tm.getcol(1)));
  }
  else if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_XY)
  {
    mpp_tm.setcol(2, normalize(place_vec % v_orient));
    mpp_tm.setcol(1, normalize(mpp_tm.getcol(2) % place_vec));
    mpp_tm.setcol(0, normalize(mpp_tm.getcol(1) % mpp_tm.getcol(2)));
  }
  else if (mppRec.mpOrientType == MpPlacementRec::MP_ORIENT_3POINT)
  {
    Point3 place_vec2 = normalize(mppRec.mpPoint3 - mppRec.mpPoint2);
    Point3 v_norm = normalize(place_vec2 % place_vec);
    if (v_norm.y < 0)
      v_norm *= -1;
    Quat rot_quat = quat_rotation_arc(v_orient, v_norm);
    mpp_tm = makeTM(rot_quat);
    mpp_tm = mpp_tm * tm;
  }

  tm = mpp_tm;
}
} // namespace rendinstgen
