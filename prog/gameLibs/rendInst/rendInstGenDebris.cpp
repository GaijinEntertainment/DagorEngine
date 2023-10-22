#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include "riGen/riGenData.h"
#include "riGen/genObjUtil.h"
#include "riGen/riGenExtra.h"
#include "riGen/riUtil.h"

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <EASTL/algorithm.h>
#include <startup/dag_globalSettings.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_Point4.h>
#include <math/dag_mathUtils.h>
#include <util/dag_oaHashNameMap.h>
#include <memory/dag_framemem.h>
#include <phys/dag_physResource.h>


#define LOGLEVEL_DEBUG _MAKE4C('RGEN')

static float rendinstDamageDelay = 1;
static float rendinstFallBlendTime = 1;
static float rendinstGravitation = 9.81f;
static float rendinstDamageFxScale = 1.0f;
static float rendinstAccumulatedPowerMult = 3.0f;
static float rendinstAccumulatedRotation = 0.5f;
static int (*debris_get_fx_type_by_name)(const char *name);
static void play_riextra_destroy_effect(rendinst::riex_handle_t id, mat44f_cref tm, rendinst::ri_damage_effect_cb fxCB, bool bbScale,
  bool restorable = false, const Point3 *coll_point = nullptr);

static inline void prepareDestrExcl(RendInstGenData *rgl)
{
  if (rendinst::gen::destrExcl.bm.getW())
  {
    if (rendinst::gen::destrExcl.bm.getW() > 0 && rendinst::gen::destrExcl.bm.getW() <= 65536 &&
        rendinst::gen::destrExcl.bm.getH() > 0 && rendinst::gen::destrExcl.bm.getH() <= 65536)
      return;

    fatal("rendinst::gen::destrExcl is broken: size=%d,%d", rendinst::gen::destrExcl.bm.getW(), rendinst::gen::destrExcl.bm.getH());
    rendinst::gen::destrExcl.clear();
  }
  rendinst::gen::destrExcl.initExplicit(rgl->world0x(), rgl->world0z(), 0.5f, rgl->grid2world * rgl->cellSz * rgl->cellNumW,
    rgl->grid2world * rgl->cellSz * rgl->cellNumH);
}

static int sweepRIGenInBoxByMask(RendInstGenData *rgl, const BBox3 &_box, unsigned frameNo, rendinst::ri_damage_effect_cb effect_cb,
  const Point3 &axis, bool create_debris)
{
  static constexpr int SUBCELL_DIV = RendInstGenData::SUBCELL_DIV;
  float subcellSz = rgl->grid2world * rgl->cellSz / SUBCELL_DIV;
  int removed_cnt = 0;
  bbox3f box = v_ldu_bbox3(_box);

  ScopedLockRead lock(rgl->rtData->riRwCs); // since we do not make changes that affect other readers
  dag::ConstSpan<int> ld = rgl->rtData->loaded.getList();
  for (auto ldi : ld)
  {
    RendInstGenData::Cell &cell = rgl->cells[ldi];
    RendInstGenData::CellRtData *crt_ptr = cell.isReady();
    if (!crt_ptr)
      continue;
    RendInstGenData::CellRtData &crt = *crt_ptr;

    if (!v_bbox3_test_box_intersect(crt.bbox[0], box))
      continue;

    int pcnt = crt.pools.size();
    float cell_x0 = as_point4(&crt.cellOrigin).x, cell_z0 = as_point4(&crt.cellOrigin).z;
    vec3f v_cell_add = crt.cellOrigin;
    vec3f v_cell_mul =
      v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(subcellSz * SUBCELL_DIV, crt.cellHeight, subcellSz * SUBCELL_DIV, 0));

    int x0 = (int)floorf((_box[0].x - cell_x0) / subcellSz), x1 = (int)floorf((_box[1].x - cell_x0) / subcellSz);
    int z0 = (int)floorf((_box[0].z - cell_z0) / subcellSz), z1 = (int)floorf((_box[1].z - cell_z0) / subcellSz);
    if (x0 < 0)
      x0 = 0;
    if (x1 >= SUBCELL_DIV)
      x1 = SUBCELL_DIV - 1;
    if (z0 < 0)
      z0 = 0;
    if (z1 >= SUBCELL_DIV)
      z1 = SUBCELL_DIV - 1;

    int prev_removed_cnt = removed_cnt;
    for (; z0 <= z1; z0++)
      for (int x2 = x0; x2 <= x1; x2++)
      {
        int idx = z0 * SUBCELL_DIV + x2 + 1;

        if (!v_bbox3_test_box_intersect(crt.bbox[idx], box))
          continue;
        idx--;

        for (int p = 0; p < pcnt; p++)
        {
          const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(p, idx);
          if (!scs.sz || rgl->rtData->riProperties[p].immortal || rgl->rtData->riDestr[p].destructable)
            continue;

          mat44f tm;
          bool palette_rotation = rgl->rtData->riPaletteRotation[p];
          if (rgl->rtData->riPosInst[p])
          {
            int16_t *data_s = (int16_t *)(crt.sysMemData + scs.ofs);
            int stride_w = RIGEN_POS_STRIDE_B(rgl->perInstDataDwords) / 2;
            for (int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
            {
              if (rendinst::is_pos_rendinst_data_destroyed(data))
                continue;
              float x3 = data[0] * subcellSz * SUBCELL_DIV / 32767.0f + cell_x0;
              float z3 = data[2] * subcellSz * SUBCELL_DIV / 32767.0f + cell_z0;
              if (rendinst::gen::destrExcl.isMarked(x3, z3))
              {
                if (RendInstGenData::renderResRequired)
                {
                  rendinst::gen::unpack_tm_pos(tm, data, v_cell_add, v_cell_mul, palette_rotation);
                  rendinst::destroy_pos_rendinst_data(data);
                  removed_cnt++;
                  if (create_debris)
                  {
                    if (axis.lengthSq() < 0.001f)
                    {
                      Point3 newAxis = as_point3(&tm.col3) - _box.center();
                      newAxis.y = 0.0f;
                      const float axisMultiplier = max(1 - (length(newAxis) / (_box.width().x * 0.5f)), 0.1f);
                      newAxis = normalize(newAxis);
                      newAxis *= axisMultiplier;
                      rgl->rtData->addDebris(tm, p, frameNo, false, effect_cb, newAxis, axisMultiplier * rendinstAccumulatedPowerMult);
                    }
                    else
                      rgl->rtData->addDebris(tm, p, frameNo, false, effect_cb, axis);
                  }
                }
                else
                {
                  if (create_debris && rendinst::isRgLayerPrimary(rgl->rtData->layerIdx))
                  {
                    rendinst::RendInstDesc riDesc(ldi, idx, p, int(intptr_t(data) - intptr_t(data_s)), rgl->rtData->layerIdx);
                    rendinst::RendInstBufferData riBuffer;
                    rendinst::RendInstDesc offsetedDesc = riDesc;
                    rendinst::riex_handle_t generatedHandle = rendinst::RIEX_HANDLE_NULL;
                    rgl->rtData->riRwCs.unlockRead(); // Note: add_destroyed_data that called in doRiGenDestr acquires write lock
                                                      // within
                    rendinst::doRIGenDestr(riDesc, riBuffer, offsetedDesc, 0, 0, &generatedHandle);
                    rgl->rtData->riRwCs.lockRead();
                  }
                  rendinst::destroy_pos_rendinst_data(data);
                  removed_cnt++;
                }
              }
            }
          }
          else
          {
            int stride_w = RIGEN_TM_STRIDE_B(rgl->perInstDataDwords) / 2;
            for (int16_t *data = (int16_t *)(crt.sysMemData + scs.ofs), *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
            {
              if (rendinst::is_tm_rendinst_data_destroyed(data))
                continue;
              float x3 = data[3] * subcellSz * SUBCELL_DIV / 32767.0f + cell_x0;
              float z3 = data[11] * subcellSz * SUBCELL_DIV / 32767.0f + cell_z0;
              if (rendinst::gen::destrExcl.isMarked(x3, z3))
              {
                if (RendInstGenData::renderResRequired)
                {
                  rendinst::gen::unpack_tm_full(tm, data, v_cell_add, v_cell_mul);
                  rendinst::destroy_tm_rendinst_data(data);
                  removed_cnt++;
                  if (create_debris)
                    rgl->rtData->addDebris(tm, p, frameNo, false, effect_cb, axis);
                }
                else
                {
                  rendinst::destroy_tm_rendinst_data(data);
                  removed_cnt++;
                }
              }
            }
          }
        }
      }
    if (prev_removed_cnt < removed_cnt)
    {
      riutil::world_version_inc(crt.bbox[0]);
      if (RendInstGenData::renderResRequired)
        rgl->updateVb(crt, ldi);
    }
  }
  return removed_cnt;
}

static String get_name_for_skeleton_res(SimpleString s)
{
  static const char *suffix_to_remove = "phobj";
  G_ASSERT(s.length() >= strlen(suffix_to_remove));
  String res_name;
  res_name.setStr(s.str(), s.length() - strlen(suffix_to_remove));
  return res_name + "skeleton";
}

bool rendinst::destroyRIGenWithBullets(const Point3 &from, const Point3 &dir, float &dist, Point3 &norm, bool killBuildings,
  unsigned frameNo, ri_damage_effect_cb effect_cb)
{
  (void)from;
  (void)dir;
  (void)dist;
  (void)norm;
  (void)killBuildings;
  (void)frameNo;
  (void)effect_cb;
  return false;
}
bool rendinst::destroyRIGenInSegment(const Point3 &p0, const Point3 &p1, bool trees, bool buildings, Point4 &contactPt, bool doKill,
  unsigned frameNo, ri_damage_effect_cb effect_cb)
{
  (void)p0;
  (void)p1;
  (void)trees;
  (void)buildings;
  (void)contactPt;
  (void)doKill;
  (void)frameNo;
  (void)effect_cb;
  return false;
}
void rendinst::doRIGenDamage(const BSphere3 &sphere, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis,
  bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markCircle(sphere.c.x, sphere.c.z, sphere.r);
    }
    sweepRIGenInBoxByMask(rgl, sphere, frameNo, effect_cb, axis, create_debris);
  }
}
void rendinst::doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis, bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markBox(box[0].x, box[0].z, box[1].x, box[1].z);
    }
    sweepRIGenInBoxByMask(rgl, box, frameNo, effect_cb, axis, create_debris);
  }
}

void rendinst::doRIGenDamage(const BBox3 &box, unsigned frameNo, ri_damage_effect_cb effect_cb, const TMatrix &check_itm,
  const Point3 &axis, bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markTM(box[0].x, box[0].z, box[1].x, box[1].z, check_itm);
    }
    sweepRIGenInBoxByMask(rgl, box, frameNo, effect_cb, axis, create_debris);
  }
}

void rendinst::doRIGenDamage(const Point3 &pos, unsigned frameNo, ri_damage_effect_cb effect_cb, const Point3 &axis, float dmg_pts,
  bool create_debris)
{
  DECL_ALIGN16(BBox3, box) = BBox3(pos, 1.f);
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markPoint(pos.x, pos.z);
    }
    sweepRIGenInBoxByMask(rgl, box, frameNo, effect_cb, axis, create_debris);
  }

  riex_collidable_t ri_h;
  rendinst::gatherRIGenExtraCollidable(ri_h, box, true /*read_lock*/);
  for (int i = 0; i < ri_h.size(); i++)
  {
    rendinst::riex_handle_t original_id = ri_h[i], id = original_id;
    mat44f tm;
    if (rendinst::damageRIGenExtra(id, dmg_pts, &tm, nullptr, nullptr, DestrOptionFlag::AddDestroyedRi))
    {
      play_riextra_destroy_effect(original_id, tm, effect_cb, false);
      uint32_t res_idx = handle_to_ri_type(original_id);
      if (rendinst::riExtra[res_idx].destroyedPhysRes)
        logwarn("doRIGenDamage: %s doesn't produce physobj", rendinst::riExtraMap.getName(res_idx));
    }
  }
}

bool rendinst::applyDamageRIGenExtra(const rendinst::RendInstDesc &desc, float dmg_pts, float *absorbed_dmg_impulse,
  bool ignore_damage_threshold)
{
  return rendinst::applyDamageRIGenExtra(make_handle(desc.pool, desc.idx), dmg_pts, absorbed_dmg_impulse, ignore_damage_threshold);
}

rendinst::DestroyedRi *rendinst::doRIGenExternalControl(const rendinst::RendInstDesc &desc, bool rem_rendinst)
{
  if (desc.isRiExtra())
  {
    const int pool = rendinst::riExtra[desc.pool].riPoolRef;
    if (pool < 0)
      return nullptr;

    RendInstGenData *rgl = rendinst::getRgLayer(rendinst::riExtra[desc.pool].riPoolRefLayer);
    G_ASSERT(rgl && pool < rgl->rtData->riProperties.size());

    const bool treeBehaviour = rgl ? rgl->rtData->riProperties[pool].treeBehaviour : false;

    if (!treeBehaviour)
      return nullptr;

    TMatrix tm = rendinst::getRIGenMatrix(desc);
    mat44f tm44;
    v_mat44_make_from_43cu_unsafe(tm44, tm.array);

    return rgl->rtData->addExternalDebris(tm44, pool);
  }

  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = desc.idx == 0 ? riutil::get_data_by_desc_no_subcell(desc, cell) : riutil::get_data_by_desc(desc, cell);
  if (!data)
    return nullptr;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);

  rendinst::RendInstBufferData buffer;
  if (!riutil::extract_buffer_data(desc, rgl, data, buffer))
    return nullptr;
  mat44f tm;
  if (!riutil::get_rendinst_matrix(desc, rgl, data, cell, tm))
    return nullptr;
  if (rem_rendinst)
    riutil::remove_rendinst(desc, rgl, cell->rtData->bbox[0], data);

  DestroyedRi *ri = rgl->rtData->addExternalDebris(tm, desc.pool);
  if (ri)
    ri->savedData = buffer; // just POD copy
  if (RendInstGenData::renderResRequired)
    rgl->updateVb(*cell->rtData, desc.cellIdx);
  return ri;
}

#if DEBUG_RI_DESTR
static void print_debug_destr_data()
{
  RendInstGenData *rgl = RendInstGenData::rgl;
  debug("==================================================");
  for (int i = 0; i < rgl->rtData->riDestrCellData.size(); ++i)
  {
    rendinst::DestroyedCellData &cell = rgl->rtData->riDestrCellData[i];

    debug("cell (%d)", cell.cellId);
    for (int j = 0; j < cell.destroyedPoolInfo.size(); ++j)
    {
      rendinst::DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
      debug(" pool (%d)", pool.poolIdx);
      for (int k = 0; k < pool.destroyedInstances.size(); ++k)
      {
        rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
        debug("   range (%d)-(%d)", range.startOffs, range.endOffs);
      }
    }
  }
  debug("==================================================");
}
#endif

static void debug_verify_destroy_pool_data(const rendinst::DestroyedPoolData &destrPoolData)
{
#if DAGOR_DBGLEVEL > 0
  for (int i = 0, lastI = destrPoolData.destroyedInstances.size() - 1; i <= lastI; ++i)
  {
    G_ASSERT(destrPoolData.destroyedInstances[i].startOffs < destrPoolData.destroyedInstances[i].endOffs);
    G_ASSERT(i == lastI || destrPoolData.destroyedInstances[i].endOffs < destrPoolData.destroyedInstances[i + 1].startOffs);
  }
#else
  G_UNUSED(destrPoolData);
#endif
}

static void add_destroyed_data(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen)
{
  rendinst::DestroyedCellData *destrCellData = nullptr;
#if DEBUG_RI_DESTR
  print_debug_destr_data();
  debug("adding ridestr cell (%d) pool (%d) offs (%d)", desc.cellIdx, desc.pool, desc.offs);
#endif

  ScopedLockWrite lock(ri_gen->rtData->riRwCs);
  for (int i = 0; i < ri_gen->rtData->riDestrCellData.size(); ++i)
  {
    if (ri_gen->rtData->riDestrCellData[i].cellId != desc.cellIdx)
      continue;

    destrCellData = &ri_gen->rtData->riDestrCellData[i];
    break;
  }

  if (!destrCellData)
  {
    destrCellData = &ri_gen->rtData->riDestrCellData.push_back();
    destrCellData->cellId = desc.cellIdx;
  }

  rendinst::DestroyedPoolData *destrPoolData = nullptr;
  for (int i = 0; i < destrCellData->destroyedPoolInfo.size(); ++i)
  {
    if (destrCellData->destroyedPoolInfo[i].poolIdx != desc.pool)
      continue;

    destrPoolData = &destrCellData->destroyedPoolInfo[i];
    break;
  }

  if (!destrPoolData)
  {
    destrPoolData = &destrCellData->destroyedPoolInfo.push_back();
    destrPoolData->poolIdx = desc.pool;
  }

  uint16_t stride = rendinst::getRIGenStride(desc.layer, desc.cellIdx, desc.pool);
  uint32_t offs = desc.offs;

  bool found = false;
  for (int i = 0; i < destrPoolData->destroyedInstances.size(); ++i)
  {
    // Inside already added range
    if (offs >= destrPoolData->destroyedInstances[i].startOffs && offs + stride <= destrPoolData->destroyedInstances[i].endOffs)
    {
      found = true;
      break;
    }

    if (offs < destrPoolData->destroyedInstances[i].startOffs)
    {
      found = true;
      if (offs + stride >= destrPoolData->destroyedInstances[i].startOffs)
      {
        // It expands from head
        if (i > 0 && offs == destrPoolData->destroyedInstances[i - 1].endOffs)
        {
          // And merges two ranges
          destrPoolData->destroyedInstances[i - 1].endOffs = destrPoolData->destroyedInstances[i].endOffs;
          erase_items(destrPoolData->destroyedInstances, i, 1);
        }
        else
        {
          destrPoolData->destroyedInstances[i].startOffs = offs; // Just expand
          destrPoolData->destroyedInstances[i].endOffs = max(destrPoolData->destroyedInstances[i].endOffs, offs + stride);
          for (int j = i + 1; j < destrPoolData->destroyedInstances.size(); ++j) // Append next overlapped ranges
          {
            if (destrPoolData->destroyedInstances[j].startOffs <= destrPoolData->destroyedInstances[i].endOffs)
            {
              destrPoolData->destroyedInstances[i].endOffs =
                max(destrPoolData->destroyedInstances[i].endOffs, destrPoolData->destroyedInstances[j].endOffs);
              erase_items(destrPoolData->destroyedInstances, j, 1);
            }
          }
        }
      }
      else if (i > 0 && offs == destrPoolData->destroyedInstances[i - 1].endOffs)
        destrPoolData->destroyedInstances[i - 1].endOffs = offs + stride; // It merges from tail of previous one
      else
        insert_item_at(destrPoolData->destroyedInstances, i, rendinst::DestroyedInstanceRange(offs, offs + stride)); // It's just
                                                                                                                     // in-between
      break;
    }
  }
  if (!found) // it should be last then
  {
    rendinst::DestroyedInstanceRange *lastRange = destrPoolData->destroyedInstances.end() - 1;
    if (!destrPoolData->destroyedInstances.empty() && lastRange->endOffs == offs)
      lastRange->endOffs = offs + stride; // merges with last one
    else
      insert_item_at(destrPoolData->destroyedInstances, destrPoolData->destroyedInstances.size(),
        rendinst::DestroyedInstanceRange(offs, offs + stride));
  }

#if DAGOR_DBGLEVEL > 0
#if DEBUG_RI_DESTR
  print_debug_destr_data();
#endif
  debug_verify_destroy_pool_data(*destrPoolData);
#endif
}

void rendinst::updateRiGenVbCell(int layer_idx, int cell_idx)
{
  if (!RendInstGenData::renderResRequired)
    return;

  RendInstGenData *rgl = getRgLayer(layer_idx);
  if (!rgl || !rgl->rtData)
    return;

  if (cell_idx < 0 || cell_idx >= rgl->cells.size())
    return;

  RendInstGenData::Cell &cell = rgl->cells[cell_idx];
  if (RendInstGenData::CellRtData *crt_ptr = cell.isReady())
    rgl->updateVb(*crt_ptr, cell_idx);
}

void rendinst::play_riextra_dmg_fx(rendinst::riex_handle_t id, const Point3 &pos, ri_damage_effect_cb effect_cb)
{
  uint32_t res_idx = rendinst::handle_to_ri_type(id);
  G_ASSERTF(res_idx < rendinst::riExtra.size(), "res_idx=%d", res_idx);
  rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (effect_cb && (pool.dmgFxType != -1 || !pool.dmgFxTemplate.empty()))
  {
    TMatrix tm = TMatrix::IDENT;
    for (int i = 0; i < 3; ++i)
      tm.setcol(i, pool.dmgFxScale * tm.getcol(i));
    tm.setcol(3, pos);

    effect_cb(pool.dmgFxType, TMatrix::IDENT, tm, res_idx, false, nullptr, pool.dmgFxTemplate.c_str());
  }
}

static void play_destroy_effect(const RendInstGenData::RtData *rt_data, unsigned pool_idx, mat44f_cref tm,
  rendinst::ri_damage_effect_cb effect_cb, bool scale_with_bb, const Point3 *coll_point = nullptr)
{
  if (effect_cb && (pool_idx < rt_data->riDebrisMap.size()) &&
      (rt_data->riDebrisMap[pool_idx].fxType != -1 || !rt_data->riDebrisMap[pool_idx].fxTemplate.empty()))
  {
    const auto &ri = rt_data->riDebrisMap[pool_idx];
    DECL_ALIGN16(TMatrix, s_scaleTm);
    mat44f scaleTm;
    vec3f size = V_C_ONE;
    if (scale_with_bb)
      size = v_bbox3_size(rt_data->riResBb[pool_idx]);
    size = v_max(v_mul(size, v_splat4(&ri.fxScale)), rendinst::gen::VC_1div10);

    scaleTm.col0 = v_mul(size, V_C_UNIT_1000);
    scaleTm.col1 = v_mul(size, V_C_UNIT_0100);
    scaleTm.col2 = v_mul(size, V_C_UNIT_0010);
    scaleTm.col3 = v_zero();

    v_mat44_mul43(scaleTm, tm, scaleTm);
    v_mat_43cu_from_mat44(s_scaleTm.m[0], scaleTm);
    if (coll_point)
      s_scaleTm.setcol(3, Point3::xVz(*coll_point, s_scaleTm.getcol(3).y));
    effect_cb(ri.fxType, TMatrix::IDENT, s_scaleTm, pool_idx, false, nullptr, ri.fxTemplate.c_str());
  }
}

static void play_riextra_destroy_effect(rendinst::riex_handle_t id, mat44f_cref tm, rendinst::ri_damage_effect_cb fxCB, bool bbScale,
  bool restorable /*=false*/, const Point3 *coll_point /*=nullptr*/)
{
  uint32_t res_idx = rendinst::handle_to_ri_type(id);
  G_ASSERTF(res_idx < rendinst::riExtra.size(), "res_idx=%d", res_idx);
  rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (fxCB && (pool.destrFxType != -1 || pool.destrCompositeFxId != -1 || !pool.destrFxTemplate.empty()))
  {
    DECL_ALIGN16(TMatrix, s_scaleTm);
    mat44f scaleTm;
    vec3f size = V_C_ONE;
    if (bbScale)
      size = v_bbox3_size(pool.lbb);
    size = v_max(v_mul(size, v_splat4(&pool.destrFxScale)), rendinst::gen::VC_1div10);

    scaleTm.col0 = v_mul(size, V_C_UNIT_1000);
    scaleTm.col1 = v_mul(size, V_C_UNIT_0100);
    scaleTm.col2 = v_mul(size, V_C_UNIT_0010);
    scaleTm.col3 = v_zero();

    v_mat44_mul43(scaleTm, tm, scaleTm);
    v_mat_43cu_from_mat44(s_scaleTm.m[0], scaleTm);
    if (coll_point)
      s_scaleTm.setcol(3, Point3::xVz(*coll_point, s_scaleTm.getcol(3).y));

    if (pool.destrFxType != -1 || !pool.destrFxTemplate.empty())
      fxCB(pool.destrFxType, TMatrix::IDENT, s_scaleTm, res_idx, false, nullptr, pool.destrFxTemplate.c_str());

    if (pool.destrCompositeFxId != -1)
      rifx::composite::spawnEntitiy(id, pool.destrCompositeFxId, s_scaleTm, restorable);
  }
}

DynamicPhysObjectData *rendinst::doRIGenDestr(const RendInstDesc &desc, RendInstBufferData &out_buffer, RendInstDesc &out_desc,
  float /*dmg_pts*/, ri_damage_effect_cb effect_cb, riex_handle_t *out_gen_riex, bool restorable /*=false*/, int32_t user_data,
  const Point3 *coll_point, bool *ri_removed, DestrOptionFlags destroy_flags)
{
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (desc.isRiExtra())
  {
    if (!riExtra[desc.pool].isInGrid(desc.idx))
      return nullptr; // it can be already destroyed

    const int cellId = riExtra[desc.pool].riUniqueData[desc.idx].cellId;
    riex_handle_t id = rendinst::make_handle(desc.pool, desc.idx);
    bool isDynamicRi = riExtra[desc.pool].isDynamicRendinst || cellId < 0;
    rendinst::onRiExtraDestruction(id, isDynamicRi, user_data);
    if (isDynamicRi)
      return nullptr;

    G_ASSERT(cellId >= 0);
    if (cellId < 0)
      return nullptr;

    int instanceOffset = riExtra[desc.pool].riUniqueData[desc.idx].offset;
    rendinst::riex_handle_t original_id = id;
    mat44f tm;
    if (rendinst::damageRIGenExtra(id, FLT_MAX, &tm, nullptr, nullptr, destroy_flags)) // we want to definitely
                                                                                       // destroy it if external
                                                                                       // system says that we should
                                                                                       // do it.
    {
      if (id != original_id && out_gen_riex)
        *out_gen_riex = id;
      if (rgl)
      {
        out_buffer.tm = tm;
        play_destroy_effect(rgl->rtData, riExtra[desc.pool].riPoolRef, tm, effect_cb, false, coll_point);
      }
      out_desc.cellIdx = -(cellId + 1);
      out_desc.offs = instanceOffset;
      if (rgl)
        add_destroyed_data(out_desc, rgl);

      play_riextra_destroy_effect(original_id, tm, effect_cb, false, restorable, coll_point);
      if (ri_removed)
        *ri_removed = true;

      if (rendinst::riExtra[desc.pool].destroyedPhysRes)
        return rendinst::riExtra[desc.pool].destroyedPhysRes;
      else
      {
        RendInstGenData::DestrProps *dp = rgl ? safe_at(rgl->rtData->riDestr, riExtra[desc.pool].riPoolRef) : nullptr;
        return dp ? dp->res : nullptr;
      }
    }
    return nullptr;
  }
  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc(desc, cell);
  if (!data)
    return nullptr;

  if (!riutil::extract_buffer_data(desc, rgl, data, out_buffer))
    return nullptr;
  if (effect_cb)
  {
    mat44f tm;
    if (riutil::get_rendinst_matrix(desc, rgl, data, cell, tm))
      play_destroy_effect(rgl->rtData, desc.pool, tm, effect_cb, false);
  }
  riutil::remove_rendinst(desc, rgl, cell->rtData->bbox[0], data);
  if (ri_removed)
    *ri_removed = true;

  if (desc.pool >= rgl->rtData->riDestr.size() || desc.pool < 0)
    return nullptr;

  DynamicPhysObjectData *res = rgl->rtData->riDestr[desc.pool].res;

  if (RendInstGenData::renderResRequired)
    rgl->updateVb(*cell->rtData, desc.cellIdx);

  int offs = riutil::get_data_offs_from_start(desc);
  G_ASSERTF(offs >= 0 && offs / 8 < 0xffff, "Offs '%d' out of range of 0x0000-0xffff", offs / 8); // fits in our range
  if (offs < 0 || unsigned(offs / 8) > 0xffff)
    return nullptr;

  out_desc.idx = 0;
  out_desc.offs = offs;
  add_destroyed_data(out_desc, rgl);

  return res;
}

DynamicPhysObjectData *rendinst::doRIGenDestrEx(const RendInstDesc &desc, float /*dmg_pts*/, ri_damage_effect_cb effect_cb,
  int user_data)
{
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (desc.isRiExtra())
  {
    const RiExtraPool::ElemUniqueData *uniqueData = safe_at(riExtra[desc.pool].riUniqueData, desc.idx);
    bool isDynamicRi = riExtra[desc.pool].isDynamicRendinst || (uniqueData && uniqueData->cellId < 0);
    rendinst::onRiExtraDestruction(rendinst::make_handle(desc.pool, desc.idx), isDynamicRi, user_data);
    if (isDynamicRi)
      return nullptr;

    //== what should it DO?
    // Search for this cellid/offset in unique data
    int idx = -1;
    for (int i = 0; i < riExtra[desc.pool].riUniqueData.size(); ++i)
    {
      if (riExtra[desc.pool].riUniqueData[i].cellId != -(desc.cellIdx + 1) || riExtra[desc.pool].riUniqueData[i].offset != desc.offs)
        continue;
      idx = i;
      break;
    }
    if (idx < 0)
    {
      if (rgl)
        add_destroyed_data(desc, rgl);
      return nullptr; // We cannot find it. It's okay, maybe it's not generated yet
    }
    riex_handle_t id = rendinst::make_handle(desc.pool, idx);
    mat44f tm;
    rendinst::riex_handle_t original_id = id;
    DestrOptionFlags flags = DestrOptionFlag::AddDestroyedRi | DestrOptionFlag::ForceDestroy;
    if (rendinst::damageRIGenExtra(id, FLT_MAX, &tm, nullptr, nullptr, flags) && rgl) // we want to definitely destroy it if external
                                                                                      // system says that we should do it.
    {
      add_destroyed_data(desc, rgl);
      play_riextra_destroy_effect(original_id, tm, effect_cb, false);

      if (rendinst::riExtra[desc.pool].destroyedPhysRes)
        return rendinst::riExtra[desc.pool].destroyedPhysRes;
      else
      {
        RendInstGenData::DestrProps *dp = safe_at(rgl->rtData->riDestr, riExtra[desc.pool].riPoolRef);
        return dp ? dp->res : nullptr;
      }
    }
    return nullptr;
  }
  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc_no_subcell(desc, cell);
  if (!data)
  {
    // Cell is not loaded
    if (rgl)
      add_destroyed_data(desc, rgl);
    return nullptr;
  }

  if (effect_cb)
  {
    mat44f tm;
    if (riutil::get_rendinst_matrix(desc, rgl, data, cell, tm))
      play_destroy_effect(rgl->rtData, desc.pool, tm, effect_cb, false);
  }
  riutil::remove_rendinst(desc, rgl, cell->rtData->bbox[0], data);

  if (desc.pool >= rgl->rtData->riDestr.size() || desc.pool < 0)
    return nullptr;

  add_destroyed_data(desc, rgl);
  return rgl->rtData->riDestr[desc.pool].res;
}

DynamicPhysObjectData *rendinst::doRIExGenDestrEx(rendinst::riex_handle_t riex_handle, ri_damage_effect_cb effect_cb)
{
  mat44f tm;
  mat43f m43 = rendinst::getRIGenExtra43(riex_handle);
  v_mat43_transpose_to_mat44(tm, m43);
  play_riextra_destroy_effect(riex_handle, tm, effect_cb, false);
  uint32_t idx = handle_to_ri_type(riex_handle);
  return rendinst::riExtra[idx].destroyedPhysRes;
}

bool rendinst::getDestrCellData(int layer_idx, const eastl::fixed_function<64, bool(const Tab<DestroyedCellData> &)> &callback)
{
  RendInstGenData *rgl = getRgLayer(layer_idx);
  if (rgl && rgl->rtData)
  {
    ScopedLockRead lock(rgl->rtData->riRwCs);
    return callback(rgl->rtData->riDestrCellData);
  }
  return false;
}

bool rendinst::restoreRiGen(const RendInstDesc &desc, const RendInstBufferData &buffer)
{
  if (desc.isRiExtra())
    return false;
  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc(desc, cell);
  if (!data)
    return false;

  RendInstGenData::CellRtData &crt = *cell->rtData;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  int maxData = RIGEN_STRIDE_B(rgl->rtData->riPosInst[desc.pool], rgl->perInstDataDwords) / 2;
  for (int i = 0; i < maxData; ++i)
    data[i] = buffer.data[i];

  if (RendInstGenData::renderResRequired)
    rgl->updateVb(crt, desc.cellIdx);

  riutil::world_version_inc(crt.bbox[0]);

  return true;
}

bool rendinst::returnRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri)
{
  if (!ri || !restoreRiGen(desc, ri->savedData))
    return false;

  return removeRIGenExternalControl(desc, ri);
}

bool rendinst::removeRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri)
{
  if (!ri)
    return false;

  G_ASSERT(!rgLayer.empty()); // make sure that riExtra is not shutdowned
  int pool = desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRef : desc.pool;
  RendInstGenData *rgl = rendinst::getRgLayer(desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRefLayer : desc.layer);
  const int debrisNo = rgl->rtData->riDebrisMap[pool].delayedPoolIdx;
  auto &delayedRi = rgl->rtData->riDebris[debrisNo].delayedRi;
  auto it = eastl::find(delayedRi.begin(), delayedRi.end(), ri);
  if (it != delayedRi.end())
  {
    erase_items(delayedRi, eastl::distance(delayedRi.begin(), it), 1);
    del_it(ri);
    rgl->rtData->curDebris[1]--;
    return true;
  }

  G_ASSERTF(0, "DestroyedRi inst %p not found in debrisNo %d/pool %d", ri, debrisNo, pool);
  return false;
}


rendinst::riex_handle_t rendinst::restoreRiGenDestr(const RendInstDesc &desc, const RendInstBufferData &buffer)
{
  riex_handle_t h = RIEX_HANDLE_NULL;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (!rgl)
    return RIEX_HANDLE_NULL;

  if (desc.isRiExtra())
  {
    if (desc.pool < 0 || desc.pool >= riExtra.size())
      return RIEX_HANDLE_NULL;
    RendInstDesc nonExtraDesc = desc;
    nonExtraDesc.cellIdx = -(nonExtraDesc.cellIdx + 1);
    nonExtraDesc.pool = riExtra[desc.pool].riPoolRef;
    h = addRIGenExtra44(desc.pool, false, buffer.tm, true, nonExtraDesc.cellIdx, desc.offs);
    if (h == RIEX_HANDLE_NULL)
      return h;
  }
  else
  {
    RendInstGenData::Cell *cell = nullptr;
    int16_t *data = riutil::get_data_by_desc_no_subcell(desc, cell);
    if (!data)
      return RIEX_HANDLE_NULL;

    RendInstGenData::CellRtData &crt = *cell->rtData;
    int maxData = RIGEN_STRIDE_B(rgl->rtData->riPosInst[desc.pool], rgl->perInstDataDwords) / 2;
    for (int i = 0; i < maxData; ++i)
      data[i] = buffer.data[i];

    riutil::world_version_inc(crt.bbox[0]);

    if (RendInstGenData::renderResRequired)
      rgl->updateVb(crt, desc.cellIdx);
  }

#if DEBUG_RI_DESTR
  debug("restoreRiGenDestr cell (%d) pool (%d) offs (%d)", desc.cellIdx, desc.pool, desc.offs);
  print_debug_destr_data();
#endif

  ScopedLockWrite lock(rgl->rtData->riRwCs);
  for (int i = 0; i < rgl->rtData->riDestrCellData.size(); ++i)
  {
    DestroyedCellData &cell = rgl->rtData->riDestrCellData[i];
    if (cell.cellId != desc.cellIdx)
      continue;

    for (int j = 0; j < cell.destroyedPoolInfo.size(); ++j)
    {
      DestroyedPoolData &pool = cell.destroyedPoolInfo[j];
      if (pool.poolIdx != desc.pool)
        continue;

      int stride = getRIGenStride(desc.layer, desc.cellIdx, desc.pool);
      for (int k = 0; k < pool.destroyedInstances.size(); ++k)
      {
        DestroyedInstanceRange &range = pool.destroyedInstances[k];

        if (desc.offs >= range.endOffs)
          continue;

        if (desc.offs < range.startOffs)
          break;

        // If it's in the range
        if (desc.offs == range.startOffs)
        {
          if (range.startOffs + stride == range.endOffs)
          {
            // Range could be erased if it's the only one
            erase_items(pool.destroyedInstances, k, 1);
          }
          else
          {
            // Nope - just move it by the stride
            range.startOffs = range.startOffs + stride;
          }
#if DEBUG_RI_DESTR
          print_debug_destr_data();
#endif
          break;
        }
        if (desc.offs + stride == range.endOffs)
        {
          // It's at the end of range.
          range.endOffs = range.endOffs - stride;
#if DEBUG_RI_DESTR
          print_debug_destr_data();
#endif
          break;
        }
        // Ok, then it's in the middle. Add new range... Fragmentation
        int endOffs = range.endOffs;
        range.endOffs = desc.offs;
        insert_item_at(pool.destroyedInstances, k + 1, rendinst::DestroyedInstanceRange(desc.offs + stride, endOffs));
#if DEBUG_RI_DESTR
        print_debug_destr_data();
#endif
        break;
      }

      debug_verify_destroy_pool_data(pool);
      return h;
    }
  }
  return h;
}

struct ScopeSetRestrictionList
{
  ScopeSetRestrictionList(const FastNameMap &nm) { set_required_res_list_restriction(nm); }
  ~ScopeSetRestrictionList() { reset_required_res_list_restriction(); }
};

void RendInstGenData::RtData::initDebris(const DataBlock &ri_blk, int (*get_fx_type_by_name)(const char *name))
{
  debris_get_fx_type_by_name = get_fx_type_by_name;
  rendinstDamageDelay = ::dgs_get_game_params()->getReal("rendinstDamageDelay", 1.f);
  rendinstDamageFxScale = ::dgs_get_game_params()->getReal("rendinstDamageFxScale", 1.f);
  rendinstFallBlendTime = ::dgs_get_game_params()->getReal("rendinstFallBlendTime", 1.f);
  rendinstGravitation = ::dgs_get_game_params()->getReal("rendinstGravitation", 2.f);
  rendinstAccumulatedPowerMult = ::dgs_get_game_params()->getReal("rendinstAccumulatedPowerMult", 3.f);
  rendinstAccumulatedRotation = ::dgs_get_game_params()->getReal("rendinstAccumulatedRotation", 0.5f);

  FastNameMap debris_nm;
  FastNameMap destr_nm;
  Tab<SimpleString> destrNames(framemem_ptr());
  maxDebris[0] = ri_blk.getInt("maxDebrisCount", 2000);
  maxDebris[1] = ri_blk.getInt("maxDelayedCount", 2000);

  // Note: we overcommit here a bit in order to be able to add riDestr in runtime without realloc
  int riResCnt = riRes.size(), riResResv = (riResCnt * 3 + 1) / 2;
  riProperties.reserve(riResResv);
  riProperties.resize(riResCnt);
  mem_set_0(riProperties);
  riDestr.reserve(riResResv);
  riDestr.resize(riResCnt);
  riDebrisMap.resize(riResCnt);
  riDebris.reserve(32);
  destrNames.resize(riResCnt);

  const DataBlock *riExtraBlk = ri_blk.getBlockByNameEx("riExtra");
  const DataBlock *commonTreeBlk = ri_blk.getBlockByNameEx("tree_common");
  int errDuplicateBlocks = 0;
  for (unsigned int blockNo = 0; blockNo < ri_blk.blockCount(); blockNo++)
  {
    const DataBlock *blk = ri_blk.getBlock(blockNo);
    if (stricmp(blk->getBlockName(), "dmg") == 0)
    {
      const char *name = blk->getStr("name", nullptr);
      if (name && riExtraBlk->blockExists(name))
      {
        logerr("Duplicate desc for [%s] found in config/_*_dmg.blk and config/rendinst_dmg.blk", name);
        errDuplicateBlocks++;
      }
    }
  }
  G_ASSERTF(!errDuplicateBlocks, "Remove %d duplicate dmg{} descriptions in config/_*_dmg.blk", errDuplicateBlocks);

  for (int i = 0; i < riResCnt; i++)
  {
    const DataBlock *objectDmgBlk = riExtraBlk->getBlockByName(riResName[i]);
    for (unsigned int blockNo = 0; !objectDmgBlk && blockNo < ri_blk.blockCount(); blockNo++)
    {
      const DataBlock *blk = ri_blk.getBlock(blockNo);
      if (stricmp(blk->getBlockName(), "dmg") == 0)
      {
        const char *name = blk->getStr("name", nullptr);
        if (stricmp(name, riResName[i]) == 0)
        {
          objectDmgBlk = blk;
          break;
        }
      }
    }

    const DataBlock *defaultDmgBlk = ri_blk.getBlockByNameEx(riPosInst[i] ? "default_tree" : "default_building");
    if (!objectDmgBlk)
      objectDmgBlk = defaultDmgBlk;

    if (objectDmgBlk)
    {
      riProperties[i].immortal = objectDmgBlk->getBool("immortal", false);
      riProperties[i].stopsBullets = objectDmgBlk->getBool("stopsBullets", true);
      riProperties[i].matId = PhysMat::getMaterialId(objectDmgBlk->getStr("material", defaultDmgBlk->getStr("material", nullptr)));
      riProperties[i].bushBehaviour = objectDmgBlk->getBool("bushBehaviour", false);
      riProperties[i].treeBehaviour = objectDmgBlk->getBool("treeBehaviour", false);
      riProperties[i].canopyTriangle = objectDmgBlk->getBool("canopyTriangle", false);
      riProperties[i].canopyTopOffset = objectDmgBlk->getReal("canopyTopOffset", 0.1f);
      riProperties[i].canopyTopPart = objectDmgBlk->getReal("canopyTopPart", riProperties[i].canopyTriangle ? 0.75f : 0.4f);
      riProperties[i].canopyWidthPart = objectDmgBlk->getReal("canopyWidthPart", 0.3f);
      bool useCanopy = objectDmgBlk->paramExists("canopyTopPart") || objectDmgBlk->paramExists("canopyWidthPart") ||
                       objectDmgBlk->paramExists("canopyTriangle") || objectDmgBlk->paramExists("canopyTopOffset");
      riProperties[i].canopyOpacity = objectDmgBlk->getReal("canopyOpacity", useCanopy ? 1.0f : 0.f);

      riDestr[i].destrFxName = objectDmgBlk->getStr("destrFx", "");
      riDestr[i].destrFxTemplate = objectDmgBlk->getStr("destrFxTemplate", nullptr);
      riDestr[i].destructionImpulse = objectDmgBlk->getReal("destructionImpulse", 0.f);
      riDestr[i].collisionHeightScale = objectDmgBlk->getReal("destrCollisionHeightScale", 1.f);
      const char *destroyedModelName = objectDmgBlk->getStr("destroyedModelName", nullptr);
      riDestr[i].destructable = destroyedModelName != nullptr;
      riDestr[i].isParent = objectDmgBlk->getBool("isParent", false);
      riDestr[i].destructibleByParent = objectDmgBlk->getBool("destructibleByParent", !riDestr[i].isParent);
      riDestr[i].destroyNeighbourDepth = objectDmgBlk->getInt("destroyNeighbourDepth", 1);
      riDestr[i].apexDestructible = objectDmgBlk->getBool("apexDestructible", false);

      riDestr[i].apexDestructionOptionsPresetName = objectDmgBlk->getStr("apexDestructionOptionsPresetName", nullptr);

      if (rendinst::enable_apex && riDestr[i].apexDestructible)
      {
        riDestr[i].destructable = true;
        riDestr[i].destrName = riResName[i];
      }

      CollisionResource *collRes = riCollRes[i].collRes;
      if (collRes && riProperties[i].matId == PHYSMAT_DEFAULT)
      {
        int collMatId = PHYSMAT_INVALID;
        for (const auto &n : collRes->getAllNodes())
        {
          if (collMatId == PHYSMAT_INVALID)
            collMatId = n.physMatId;
          else if (collMatId != n.physMatId)
          {
            collMatId = PHYSMAT_INVALID;
            break;
          }
        }
        if (collMatId != PHYSMAT_INVALID)
          riProperties[i].matId = collMatId;
      }

      for (unsigned int paramNo = 0; paramNo < objectDmgBlk->paramCount(); paramNo++)
        if (stricmp(objectDmgBlk->getParamName(paramNo), "debris") == 0)
        {
          const char *debrisName = objectDmgBlk->getStr(paramNo);
          int idx = debris_nm.addNameId(debrisName);
          if (idx == riDebris.size())
            riDebris.push_back().props = &riDebrisMap[i];
          riDebrisMap[i].debrisPoolIdx.push_back(idx);
        }

      riDebrisMap[i].fxType = -1;
      riDebrisMap[i].submersion =
        objectDmgBlk->getReal("submersion", riPosInst[i] ? commonTreeBlk->getReal("submersion", 0.05f) : 4.0f);
      riDebrisMap[i].inclination =
        objectDmgBlk->getReal("inclination", riPosInst[i] ? commonTreeBlk->getReal("inclination", 0.42f) : 0.2f);
      riDebrisMap[i].impulseOnExplosion =
        objectDmgBlk->getReal("impulseOnExplosion", riPosInst[i] ? commonTreeBlk->getReal("impulseOnExplosion", 1.0f) : 0.0f);
      riDebrisMap[i].damageDelay = objectDmgBlk->getReal("damageDelay",
        riPosInst[i] ? commonTreeBlk->getReal("damageDelay", rendinstDamageDelay) : rendinstDamageDelay);
      riDebrisMap[i].fxScale = objectDmgBlk->getReal("fxScale", rendinstDamageFxScale);

      v_mat44_ident(riDebrisMap[i].debrisTm);
      if (objectDmgBlk->getBool("scaleDebris", false))
      {
        Point3 scale(1, 1, 1);
        if (riRes[i])
          scale = 0.5f * riRes[i]->bbox.width();
        else
          logerr("missing riResName[%d]=%s mentioned with scaleDebris", i, riResName[i]);
        as_point4(&riDebrisMap[i].debrisTm.col0).x *= scale.x;
        as_point4(&riDebrisMap[i].debrisTm.col1).y *= scale.y * 0.5f;
        as_point4(&riDebrisMap[i].debrisTm.col2).z *= scale.z;
      }
      else
      {
        float scale = objectDmgBlk->getReal("scale", 1.f);
        mat33f m33;
        v_mat33_make_rot_cw_y(m33, v_splats(RadToDeg(objectDmgBlk->getReal("angle", 0.f))));
        riDebrisMap[i].debrisTm.set33(m33);
        as_point4(&riDebrisMap[i].debrisTm.col0).x *= scale;
        as_point4(&riDebrisMap[i].debrisTm.col1).y *= scale;
        as_point4(&riDebrisMap[i].debrisTm.col2).z *= scale;
      }

      if (debris_get_fx_type_by_name)
      {
        const char *fxName = objectDmgBlk->getStr("fx", nullptr);
        if (fxName)
          riDebrisMap[i].fxType = debris_get_fx_type_by_name(fxName);
      }

      if (const char *fxName = objectDmgBlk->getStr("fxTemplate", nullptr))
        riDebrisMap[i].fxTemplate = fxName;

      if (destroyedModelName)
      {
        destrNames[i] = destroyedModelName;
        destr_nm.addNameId(destroyedModelName);
        const String skeletonResName = get_name_for_skeleton_res(destrNames[i]);
        destr_nm.addNameId(skeletonResName);
      }

      if (riDebrisMap[i].submersion > 0.f || riDebrisMap[i].inclination > 0.f)
      {
        int idx = debris_nm.addNameId(riResName[i]);
        if (idx == riDebris.size())
          riDebris.push_back().props = &riDebrisMap[i];
        riDebrisMap[i].delayedPoolIdx = idx;
      }
    }
  }

  if (rendinst::isRgLayerPrimary(layerIdx))
    addDebrisForRiExtraRange(ri_blk, 0, rendinst::riExtra.size());

  riDebris.shrink_to_fit();
  FastNameMap debris_nm_full;
  iterate_names(debris_nm, [&](int, const char *name) {
    debris_nm_full.addNameId(name);
    String coll_name(128, "%s" RI_COLLISION_RES_SUFFIX, name);
    if (get_resource_type_id(coll_name) == CollisionGameResClassId)
      debris_nm_full.addNameId(coll_name);
  });

  ScopeSetRestrictionList resListGuard(debris_nm_full);

  iterate_names(debris_nm, [&](int id, const char *name) {
    int ref = -1;
    for (int j = 0; j < riResName.size(); j++)
      if (strcmp(riResName[j], name) == 0)
      {
        ref = j;
        break;
      }
    int riExId = rendinst::addRIGenExtraResIdx(name, ref, layerIdx, {});
    riDebris[id].resIdx = riExId;
    if (riExId >= 0 && !RendInstGenData::renderResRequired)
      if (rendinst::riExtra[riExId].destroyedPhysRes)
      {
        release_game_resource((GameResource *)rendinst::riExtra[riExId].destroyedPhysRes);
        rendinst::riExtra[riExId].destroyedPhysRes = nullptr;
      }
  });

  if (!RendInstGenData::renderResRequired) // after rendinst::addRIGenExtraResIdx() to ensure that riExtraMap is the same on client &
                                           // server
  {
    clear_and_shrink(riDebris);
    clear_and_shrink(riDebrisMap);
    return;
  }

  set_required_res_list_restriction(destr_nm);
  for (int i = 0; i < destrNames.size(); ++i)
  {
    if (destrNames[i].empty())
      continue;
    DynamicPhysObjectData *model =
      (DynamicPhysObjectData *)::get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(destrNames[i].str()), PhysObjGameResClassId);
    riDestr[i].res = model;
    const String skeletonResName = get_name_for_skeleton_res(destrNames[i]);
    const GameResHandle skeletonHandle = GAMERES_HANDLE_FROM_STRING(skeletonResName.str());
    if (model && get_resource_type_id(skeletonResName))
      model->skeleton = (GeomNodeTree *)::get_game_resource_ex(skeletonHandle, GeomNodeTreeGameResClassId);
    if (model)
    {
      for (int modelNo = 0; modelNo < model->models.size(); modelNo++)
        model->models[modelNo]->acquireTexRefs();
    }
  }
  if (rendinst::isRgLayerPrimary(layerIdx))
  {
    for (int i = 0; i < rendinst::riExtra.size(); i++)
    {
      rendinst::RiExtraPool &curRi = rendinst::riExtra[i];
      if ((size_t)curRi.riPoolRef >= (size_t)riDestr.size())
        continue;

      if (riProperties[curRi.riPoolRef].immortal)
        curRi.immortal = true;

      const DestrProps &curRiDestr = riDestr[curRi.riPoolRef];
      // NOTE: apex destructible buildings might be immortal if 1.apex disabled & 2.they don't have physObj
      if (curRiDestr.apexDestructible)
        curRi.immortal = !curRiDestr.destructable;

      if (!curRi.destroyedPhysRes && curRiDestr.res)
      {
        curRi.destroyedPhysRes = curRiDestr.res;
        ::game_resource_add_ref((GameResource *)curRi.destroyedPhysRes);
      }
    }
  }
}


void RendInstGenData::RtData::addDebris(mat44f &tm, int pool_idx, unsigned frameNo, bool effect,
  rendinst::ri_damage_effect_cb effect_cb, const Point3 &axis, float accumulated_power)
{
  int curTotal = curDebris[0] + curDebris[1], maxTotal = maxDebris[0] + maxDebris[1];
  if (curTotal < maxTotal)
  {
    if (curDebris[0] < maxDebris[0])
      if (int dcnt = riDebrisMap[pool_idx].debrisPoolIdx.size())
      {
        int debrisNo = riDebrisMap[pool_idx].debrisPoolIdx[(grnd() % dcnt)];
        mat44f m;
        mat43f m43;
        v_mat44_mul43(m, tm, riDebrisMap[pool_idx].debrisTm);
        v_mat44_transpose_to_mat43(m43, m);
        rendinst::addRIGenExtra43(riDebris[debrisNo].resIdx, false, m43, false, -1, -1);
        // debug("addDebris[%d] to %d %p", curDebris[0], debrisNo, riDebris[debrisNo].resIdx);
        curDebris[0]++;
        curTotal++;
      }
  }
  else
    logerr("overflow0 %d >= %d (maxDebris: %d+%d) (curDebris: %d+%d) ri[%d]=%s", curTotal, maxTotal, maxDebris[0], maxDebris[1],
      curDebris[0], curDebris[1], pool_idx, riResName[pool_idx]);

  if (curTotal < maxTotal)
  {
    int debrisNo = riDebrisMap[pool_idx].delayedPoolIdx;
    if (debrisNo >= 0)
    {
      rendinst::DestroyedRi *ri = new rendinst::DestroyedRi();
      riDebris[debrisNo].delayedRi.push_back(ri);
      rendinst::riExtra[riDebris[debrisNo].resIdx].useShadow = true;
      ri->riHandle = rendinst::addRIGenExtra44(riDebris[debrisNo].resIdx, false, tm, false, -1, -1);
      ri->frameAdded = frameNo;
      ri->timeToDamage = riDebrisMap[pool_idx].damageDelay;
      ri->shouldUpdate = true;
      ri->accumulatedPower = accumulated_power;
      ri->rotationPower = rnd_float(-rendinstAccumulatedRotation, rendinstAccumulatedRotation);
      if (axis.lengthSq() < FLT_EPSILON)
      {
        as_point4(&ri->axis).set(gsrnd(), 0.f, gsrnd(), 0);
        normalizeDef(as_point3(&ri->axis), Point3(1.f, 0.f, 0.f));
      }
      else
        as_point4(&ri->axis).set(axis.x, axis.y, axis.z, 0.f);
      as_point4(&ri->normal).set(0.f, 1.f, 0.f, 0);
      // debug("addDelayed[%d] to %d %p [%d  %08X-%08X]", curDebris[1], debrisNo, riDebris[debrisNo].resIdx, pool_idx, c0, c1);
      curDebris[1]++;
      curTotal++;
    }
  }
  else
    logerr("overflow1 %d >= %d (maxDebris: %d+%d) (curDebris: %d+%d) ri[%d]=%s", curTotal, maxTotal, maxDebris[0], maxDebris[1],
      curDebris[0], curDebris[1], pool_idx, riResName[pool_idx]);

  if (effect)
    play_destroy_effect(this, pool_idx, tm, effect_cb, true);
}

rendinst::DestroyedRi *RendInstGenData::RtData::addExternalDebris(mat44f &tm, int pool_idx)
{
  if (pool_idx >= riDebrisMap.size() || pool_idx < 0)
    return nullptr;
  int debrisNo = riDebrisMap[pool_idx].delayedPoolIdx;
  if (debrisNo < 0)
    return nullptr;

  rendinst::DestroyedRi *ri = new rendinst::DestroyedRi;
  riDebris[debrisNo].delayedRi.push_back(ri);
  rendinst::riExtra[riDebris[debrisNo].resIdx].useShadow = true;
  ri->riHandle = rendinst::addRIGenExtra44(riDebris[debrisNo].resIdx, false, tm, false, -1, -1);
  ri->frameAdded = 0;
  ri->timeToDamage = FLT_MAX;
  ri->accumulatedPower = 0.0f;
  ri->rotationPower = 0.0f;
  ri->shouldUpdate = false;
  ri->fxType = riDebrisMap[pool_idx].fxType;
  as_point4(&ri->axis).set(1.f, 0.f, 0.f, 0);
  as_point4(&ri->normal).set(0.f, 1.f, 0.f, 0);
  curDebris[1]++;
  return ri;
}

void RendInstGenData::RtData::addDebrisForRiExtraRange(const DataBlock &ri_blk, uint32_t res_idx, uint32_t count)
{
  // this way of riPoolRef set up is quite dangerous; we assume that riPoolRef is used to access riProperties/riDestr only
  int add_ref = 0;
  for (int i = res_idx; i < res_idx + count; i++)
    if (rendinst::riExtra[i].riPoolRef < 0 && rendinst::riExtra[i].initialHP > 0)
    {
      rendinst::riExtra[i].riPoolRef = riProperties.size() + add_ref;
      rendinst::riExtra[i].riPoolRefLayer = layerIdx;
      add_ref++;
    }

  if (add_ref)
  {
    riProperties.resize(riProperties.size() + add_ref);
    memset(riProperties.data() + (riProperties.size() - add_ref), 0, sizeof(RendinstProperties) * add_ref);
    append_items(riDestr, add_ref);
  }

  // mark riExtra with damageable model as destructible
  const DataBlock &riExBlkb = *ri_blk.getBlockByNameEx("riExtra");
  for (int i = res_idx; i < res_idx + count; i++)
  {
    if (rendinst::riExtra[i].riPoolRef >= 0 && rendinst::riExtra[i].initialHP > 0)
    {
      int ref = rendinst::riExtra[i].riPoolRef;

      if (riDestr[ref].apexDestructible)
      {
        // NOTE: apex destructible buildings might be immortal if 1.apex disabled & 2.they don't have physObj
        // if you set destructable:=true - buildings will disappear in this case (is destructable=false - keep it, if it is true -
        // asset has physObj) for other objects - keep prev logic
        riProperties[ref].immortal = !riDestr[ref].destructable;
      }
      else
      {
        riProperties[ref].immortal = false;
        riDestr[ref].destructable = true;
      }

      const DataBlock *defaultDmgBlk = ri_blk.getBlockByNameEx("default_building");
      const char *ri_res_name = rendinst::riExtraMap.getName(i);
      const DataBlock *b = riExBlkb.getBlockByName(ri_res_name);
      if (!b)
      {
        int nid = ri_blk.getNameId("dmg");
        for (int j = 0; j < ri_blk.blockCount(); j++)
          if (ri_blk.getBlock(j)->getBlockNameId() == nid)
            if (strcmp(ri_res_name, ri_blk.getBlock(j)->getStr("name", "")) == 0)
            {
              b = ri_blk.getBlock(j);
              break;
            }
      }
      G_ASSERT(b);
      if (!b)
        continue;
      if (debris_get_fx_type_by_name)
      {
        if (const char *fxName = b->getStr("fx", nullptr))
          rendinst::riExtra[i].destrFxType = debris_get_fx_type_by_name(fxName);

        if (const DataBlock *compositeFxBlk = b->getBlockByName("compositeEffect"))
        {
          rendinst::riExtra[i].destrCompositeFxId = rifx::composite::loadCompositeEntityTemplate(compositeFxBlk);
        }

        rendinst::riExtra[i].destrFxScale = b->getReal("fxScale", 1.0f);
        if (const char *fxName = b->getStr("dmgFx", nullptr))
          rendinst::riExtra[i].dmgFxType = debris_get_fx_type_by_name(fxName);
      }

      if (const char *fxName = b->getStr("fxTemplate", nullptr))
        rendinst::riExtra[i].destrFxTemplate = fxName;

      if (const char *fxName = b->getStr("dmgFxTemplate", nullptr))
        rendinst::riExtra[i].dmgFxTemplate = fxName;

      rendinst::riExtra[i].destrFxScale = b->getReal("fxScale", 1.0f);
      rendinst::riExtra[i].dmgFxScale = b->getReal("dmgFxScale", 1.0f);
      riProperties[ref].stopsBullets = rendinst::riExtra[i].destrStopsBullets = b->getBool("stopsBullets", true);
      riProperties[ref].matId = PhysMat::getMaterialId(b->getStr("material", defaultDmgBlk->getStr("material", nullptr)));

      riDestr[ref].destructionImpulse = b->getReal("destructionImpulse", 0.0f);
      riDestr[ref].isParent = b->getBool("isParent", false);
      riDestr[ref].destroyNeighbourDepth = b->getInt("destroyNeighbourDepth", 1);
      riDestr[ref].destructibleByParent = b->getBool("destructibleByParent", !riDestr[ref].isParent);

#if RI_VERBOSE_OUTPUT
      debug("riExtra[%d] %s destructible! riPoolRef=%d fxType=%d fxScale=%.3f stopsBullets=%d destructionImpulse=%f", i, ri_res_name,
        ref, rendinst::riExtra[i].destrFxType, rendinst::riExtra[i].destrFxScale, (int)rendinst::riExtra[i].destrStopsBullets,
        riDestr[ref].destructionImpulse);
#endif
    }
  }
}

void RendInstGenData::RtData::updateDebris(uint32_t /*curFrame*/, float dt)
{
  v_bbox3_init_empty(movedDebrisBbox);

  for (int i = 0; i < riDebris.size(); i++)
  {
    if (!riDebris[i].props)
      continue;

    DebrisProps &p = *riDebris[i].props;
    for (int j = riDebris[i].delayedRi.size() - 1; j >= 0; j--)
    {
      if (!riDebris[i].delayedRi[j]->shouldUpdate)
        continue;
      if (riDebris[i].delayedRi[j]->timeToDamage > dt)
        riDebris[i].delayedRi[j]->timeToDamage -= dt;
      else
      {
        del_it(riDebris[i].delayedRi[j]);
        erase_items(riDebris[i].delayedRi, j, 1);
        curDebris[1]--;
      }
    }

    if (p.submersion > 0.f || p.inclination > 0.f)
      for (int j = 0, je = riDebris[i].delayedRi.size(); j < je; j++)
      {
        rendinst::DestroyedRi *ri = riDebris[i].delayedRi[j];

        if (!ri->shouldUpdate)
          continue;

        mat44f tm;
        v_mat43_transpose_to_mat44(tm, rendinst::getRIGenExtra43(ri->riHandle));
        constexpr float minGravityMult = 3.0f;
        constexpr float maxGravityMult = -1.0f;
        float gravitation = cvt(ri->timeToDamage, 0.f, p.damageDelay, rendinstGravitation + 1.f, 1.f);
        float bumpGravitation = p.impulseOnExplosion > FLT_EPSILON
                                  ? cvt(ri->timeToDamage, 0.f, p.damageDelay, minGravityMult, maxGravityMult) * p.impulseOnExplosion
                                  : gravitation;
        tm.col3 = v_nmsub(V_C_UNIT_0100, v_splats(p.submersion * dt * bumpGravitation), tm.col3);
        tm.col3 = v_add(v_mul(ri->axis, v_splats(dt * p.impulseOnExplosion * ri->accumulatedPower)), tm.col3);

        if (v_extract_x(v_dot3_x(tm.col1, ri->normal)) > 0.f)
        {
          mat33f rot;
          v_mat33_make_rot_cw(rot, v_norm3(v_cross3(ri->normal, ri->axis)),
            v_splats(p.inclination * dt * gravitation * ri->accumulatedPower));
          v_mat44_mul33(tm, tm, rot);
          v_mat33_make_rot_cw(rot, v_norm3(ri->normal),
            v_splats(ri->rotationPower * p.inclination * dt * gravitation * ri->accumulatedPower));
          v_mat44_mul33(tm, tm, rot);

          rendinst::moveRIGenExtra44(ri->riHandle, tm, false, true);
          v_bbox3_add_pt(movedDebrisBbox, tm.col3);
        }
        else
          ri->timeToDamage = 0.f;
      }
  }
}

rendinst::DestroyedRi::~DestroyedRi()
{
  rendinst::delRIGenExtra(riHandle);
  riHandle = RIEX_HANDLE_NULL;
}
RendInstGenData::DestrProps &RendInstGenData::DestrProps::operator=(const DestrProps &p)
{
  res = p.res;
  if (res)
  {
    game_resource_add_ref((GameResource *)res);
    if (res->skeleton)
      game_resource_add_ref((GameResource *)res->skeleton);
  }
  destructionImpulse = p.destructionImpulse;
  collisionHeightScale = p.collisionHeightScale;
  destructable = p.destructable;
  apexDestructible = p.apexDestructible;
  apexDestructionOptionsPresetName = p.apexDestructionOptionsPresetName;
  isParent = p.isParent;
  destructibleByParent = p.destructibleByParent;
  destroyNeighbourDepth = p.destroyNeighbourDepth;
  destrName = p.destrName;
  destrFxId = p.destrFxId;
  destrFxName = p.destrFxName;
  destrFxTemplate = p.destrFxTemplate;
  return *this;
}
RendInstGenData::DestrProps::~DestrProps()
{
  if (res)
  {
    if (res->skeleton)
    {
      release_game_resource((GameResource *)res->skeleton);
      res->skeleton = nullptr;
    }
    release_game_resource((GameResource *)res);
    res = nullptr;
  }
}
