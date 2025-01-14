// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstDebris.h>
#include <rendInst/rendInstFx.h>
#include <rendInst/rendInstAccess.h>
#include <rendInst/rendInstExtraAccess.h>
#include <shaders/dag_dynSceneRes.h>
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


#define debug(...) logmessage(_MAKE4C('RGEN'), __VA_ARGS__)

static float rendinstDamageDelay = 1;
static float rendinstFallBlendTime = 1;
static float rendinstGravitation = 9.81f;
static float rendinstDamageFxScale = 1.0f;
static float rendinstAccumulatedPowerMult = 3.0f;
static float rendinstAccumulatedRotation = 0.5f;
static int rendinstTreeRndSeed = 0;

static int (*debris_get_fx_type_by_name)(const char *name);
static void play_riextra_destroy_effect(rendinst::riex_handle_t id, mat44f_cref tm, rendinst::ri_damage_effect_cb fxCB, bool bbScale,
  bool restorable = false, const Point3 *coll_point = nullptr);

static inline void prepareDestrExcl(RendInstGenData *rgl)
{
  if (rendinst::gen::destrExcl.bm.getW() == 0)
    rendinst::gen::destrExcl.initExplicit(rgl->world0x(), rgl->world0z(), 0.5f, rgl->grid2world * rgl->cellSz * rgl->cellNumW,
      rgl->grid2world * rgl->cellSz * rgl->cellNumH);
}

static int sweepRIGenInBoxByMask(RendInstGenData *rgl, const BBox3 &_box, unsigned frameNo, const Point3 &axis, bool create_debris)
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

    auto mrange = make_span_const(crt.sysMemData.get(), crt.vbSize);
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
            int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
            int stride_w = RIGEN_POS_STRIDE_B(rgl->rtData->riZeroInstSeeds[p], rgl->perInstDataDwords) / 2;
            for (int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
            {
              if (rendinst::is_pos_rendinst_data_destroyed(data))
                continue;
              float x3 = data[0] * subcellSz * SUBCELL_DIV / 32767.0f + cell_x0;
              float z3 = data[2] * subcellSz * SUBCELL_DIV / 32767.0f + cell_z0;
              if (rendinst::gen::destrExcl.isMarked(x3, z3))
              {
                rendinst::RendInstDesc riDesc(ldi, idx, p, int(intptr_t(data) - intptr_t(data_s)), rgl->rtData->layerIdx);
                if (rendinst::sweep_rendinst_cb)
                  rendinst::sweep_rendinst_cb(riDesc);

                if (RendInstGenData::renderResRequired)
                {
                  rendinst::gen::unpack_tm_pos(tm, data, v_cell_add, v_cell_mul, palette_rotation);
                  rendinst::destroy_pos_rendinst_data(data, mrange);
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
                      rgl->rtData->addDebris(tm, p, frameNo, newAxis, axisMultiplier * rendinstAccumulatedPowerMult);
                    }
                    else
                      rgl->rtData->addDebris(tm, p, frameNo, axis);
                  }
                }
                else
                {
                  if (create_debris && rendinst::isRgLayerPrimary(rgl->rtData->layerIdx))
                  {
                    rendinst::RendInstBufferData riBuffer;
                    rgl->rtData->riRwCs.unlockRead(); // Note: add_destroyed_data that called in doRiGenDestr acquires write lock
                                                      // within
                    rendinst::riex_handle_t destroyedRiexHandle;
                    rendinst::doRIGenDestr(riDesc, riBuffer, true /*create_destr_effects*/, nullptr /*callback*/, destroyedRiexHandle,
                      0 /*userdata*/);
                    rgl->rtData->riRwCs.lockRead();
                  }
                  rendinst::destroy_pos_rendinst_data(data, mrange);
                  removed_cnt++;
                }
              }
            }
          }
          else
          {
            int stride_w = RIGEN_TM_STRIDE_B(rgl->rtData->riZeroInstSeeds[p], rgl->perInstDataDwords) / 2;
            int16_t *data_s = (int16_t *)(crt.sysMemData.get() + scs.ofs);
            for (int16_t *data = data_s, *data_e = data + scs.sz / 2; data < data_e; data += stride_w)
            {
              if (rendinst::is_tm_rendinst_data_destroyed(data))
                continue;
              float x3 = data[3] * subcellSz * SUBCELL_DIV / 32767.0f + cell_x0;
              float z3 = data[11] * subcellSz * SUBCELL_DIV / 32767.0f + cell_z0;
              if (rendinst::gen::destrExcl.isMarked(x3, z3))
              {
                rendinst::RendInstDesc riDesc(ldi, idx, p, int(intptr_t(data) - intptr_t(data_s)), rgl->rtData->layerIdx);
                if (rendinst::sweep_rendinst_cb)
                  rendinst::sweep_rendinst_cb(riDesc);

                if (RendInstGenData::renderResRequired)
                {
                  rendinst::gen::unpack_tm_full(tm, data, v_cell_add, v_cell_mul);
                  rendinst::destroy_tm_rendinst_data(data, mrange);
                  removed_cnt++;
                  if (create_debris)
                    rgl->rtData->addDebris(tm, p, frameNo, axis);
                }
                else
                {
                  rendinst::destroy_tm_rendinst_data(data, mrange);
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

void rendinst::doRIGenDamage(const BSphere3 &sphere, unsigned frameNo, const Point3 &axis, bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markCircle(sphere.c.x, sphere.c.z, sphere.r);
    }
    sweepRIGenInBoxByMask(rgl, sphere, frameNo, axis, create_debris);
  }
}
void rendinst::doRIGenDamage(const BBox3 &box, unsigned frameNo, const Point3 &axis, bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markBox(box[0].x, box[0].z, box[1].x, box[1].z);
    }
    sweepRIGenInBoxByMask(rgl, box, frameNo, axis, create_debris);
  }
}

void rendinst::doRIGenDamage(const BBox3 &box, unsigned frameNo, const TMatrix &check_itm, const Point3 &axis, bool create_debris)
{
  FOR_EACH_RG_LAYER_DO (rgl)
  {
    if (rendinst::isRgLayerPrimary(_layer))
    {
      prepareDestrExcl(rgl);
      rendinst::gen::destrExcl.markTM(box[0].x, box[0].z, box[1].x, box[1].z, check_itm);
    }
    sweepRIGenInBoxByMask(rgl, box, frameNo, axis, create_debris);
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
    sweepRIGenInBoxByMask(rgl, box, frameNo, axis, create_debris);
  }

  riex_collidable_t ri_h;
  rendinst::gatherRIGenExtraCollidable(ri_h, box, true /*read_lock*/);
  for (int i = 0; i < ri_h.size(); i++)
  {
    rendinst::riex_handle_t destroyedId;
    mat44f tm;
    if (rendinst::damageRIGenExtra(ri_h[i], dmg_pts, &tm, destroyedId, nullptr /*offs*/, DestrOptionFlag::AddDestroyedRi))
    {
      if (effect_cb)
        play_riextra_destroy_effect(ri_h[i], tm, effect_cb, false);
      uint32_t res_idx = handle_to_ri_type(ri_h[i]);
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
    riutil::remove_rendinst(desc, rgl, *cell->rtData, data);

  DestroyedRi *ri = rgl->rtData->addExternalDebris(tm, desc.pool);
  if (ri)
    ri->savedData = buffer; // just POD copy
  if (RendInstGenData::renderResRequired)
    rgl->updateVb(*cell->rtData, desc.cellIdx);
  return ri;
}

bool rendinst::fillTreeInstData(const RendInstDesc &desc, const Point2 &impact_velocity_xz, bool from_damage, TreeInstData &out_data)
{
  RendInstGenData *rgl = nullptr;
  int pool = -1;

  rendinstdestr::TreeDestr::BranchDestr *branchDestr = nullptr;
  if (desc.isRiExtra())
  {
    pool = rendinst::riExtra[desc.pool].riPoolRef;
    if (pool < 0)
      return false;

    rgl = rendinst::getRgLayer(rendinst::riExtra[desc.pool].riPoolRefLayer);
    G_ASSERT(rgl && pool < rgl->rtData->riProperties.size());
  }
  else
  {
    rgl = RendInstGenData::getGenDataByLayer(desc);
    pool = desc.pool;
  }

  if (!rgl)
    return false;

  branchDestr =
    from_damage ? &rgl->rtData->riProperties[pool].treeBranchDestrFromDamage : &rgl->rtData->riProperties[pool].treeBranchDestrOther;

  out_data.rndSeed = _rnd(rendinstTreeRndSeed);
  out_data.impactXZ = impact_velocity_xz;
  out_data.branchDestr.apply(*branchDestr);
  return true;
}

void rendinst::updateTreeDestrRenderData(const TMatrix &original_tm, riex_handle_t ri_handle, TreeInstData &tree_inst_data,
  const TreeInstDebugData *tree_inst_debug_data)
{
  if (!tree_inst_data.branchDestr.enableBranchDestruction && !tree_inst_debug_data)
    return;

  auto &branch = tree_inst_debug_data
                   ? (tree_inst_debug_data->last_object_was_from_damage ? tree_inst_debug_data->branchDestrFromDamage
                                                                        : tree_inst_debug_data->branchDestrOther)
                   : tree_inst_data.branchDestr;

  Point2 impulseXZ = tree_inst_data.impactXZ * branch.impulseMul;
  rendinst::RiExtraPerInstanceItem perInstanceData[5];

  auto fnPack = [](float v, float maxValue, int bits, int offset) -> uint32_t {
    return clamp((uint32_t)(v / maxValue * ((1 << bits) - 1)), 0u, (1u << bits) - 1u) << offset;
  };

  float impulseMagnitude = impulseXZ.length();
  float branchFallingChance = (impulseMagnitude - branch.impulseMin) / max(0.01f, branch.impulseMax - branch.impulseMin);
  if (branchFallingChance <= 0.0f)
    return;

  uint32_t randSeed = tree_inst_data.rndSeed % (1 << 8);
  uint32_t randSeed_branchFallingChance = randSeed << 16 | fnPack(branchFallingChance, 1.0f, 16, 0);

  float branchRangeInv = 1.0f / (branch.branchSizeMax - branch.branchSizeMin);
  uint32_t branchSizeData =
    fnPack(branch.branchSizeSlowDown, 1.0f, 8, 16) | fnPack(branch.branchSizeMin, 100.0f, 8, 8) | fnPack(branchRangeInv, 5.0f, 8, 0);

  perInstanceData[0] = {
    randSeed_branchFallingChance, branchSizeData, bitwise_cast<uint32_t>(impulseXZ.x), bitwise_cast<uint32_t>(impulseXZ.y)};

  float timerSec = (tree_inst_data.timer + (tree_inst_debug_data ? tree_inst_debug_data->timer_offset : 0.0f));
  float disappearTimerSec = tree_inst_data.disappearStartTime < 0.0f ? timerSec : (int)tree_inst_data.disappearStartTime;

  uint32_t timersData = fnPack(timerSec, 50.0f, 16, 16) | fnPack(disappearTimerSec, 50.0f, 16, 0);

  uint32_t rotationData = fnPack(branch.rotateRandomAngleSpread, 10.0f, 8, 24) | fnPack(branch.rotateRandomSpeedMulX, 10.0f, 8, 16) |
                          fnPack(branch.rotateRandomSpeedMulY, 10.0f, 8, 8) | fnPack(branch.rotateRandomSpeedMulZ, 10.0f, 8, 0);

  uint32_t speedData = fnPack(branch.fallingSpeedMul, 2.0f, 8, 16) | fnPack(branch.fallingSpeedRnd, 1.0f, 8, 8) |
                       fnPack(branch.horizontalSpeedMul, 20.0f, 8, 0);

  perInstanceData[1] = {timersData, speedData, rotationData, 0};

  perInstanceData[2] = {bitwise_cast<uint32_t>(original_tm.m[0][0]), bitwise_cast<uint32_t>(original_tm.m[0][1]),
    bitwise_cast<uint32_t>(original_tm.m[0][2]), bitwise_cast<uint32_t>(original_tm.m[1][0])};
  perInstanceData[3] = {bitwise_cast<uint32_t>(original_tm.m[1][1]), bitwise_cast<uint32_t>(original_tm.m[1][2]),
    bitwise_cast<uint32_t>(original_tm.m[2][0]), bitwise_cast<uint32_t>(original_tm.m[2][1])};
  perInstanceData[4] = {bitwise_cast<uint32_t>(original_tm.m[2][2]), bitwise_cast<uint32_t>(original_tm.m[3][0]),
    bitwise_cast<uint32_t>(original_tm.m[3][1]), bitwise_cast<uint32_t>(original_tm.m[3][2])};

  uint32_t perInstanceDataSize = countof(perInstanceData);
  rendinst::setRiExtraPerInstanceRenderData(ri_handle, perInstanceData, perInstanceDataSize);
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

#if DAGOR_DBGLEVEL > 0
static bool debug_verify_destroy_pool_data(const rendinst::DestroyedPoolData &pool, uint32_t offs = -1, uint32_t end = -1)
{
  for (int i = 0, lastI = pool.destroyedInstances.size() - 1; i <= lastI; ++i)
  {
    if (pool.destroyedInstances[i].startOffs > pool.destroyedInstances[i].endOffs ||
        (i != 0 && pool.destroyedInstances[i - 1].endOffs >= pool.destroyedInstances[i].startOffs))
    {
      logerr("==================================================");
      {
        {
          logerr(" pool (%d), last added %u %u", pool.poolIdx, offs, end);
          for (int k = 0; k < pool.destroyedInstances.size(); ++k)
          {
            const rendinst::DestroyedInstanceRange &range = pool.destroyedInstances[k];
            logerr("   range (%d)-(%d)", range.startOffs, range.endOffs);
          }
        }
      }
      logerr("==================================================");
      return false;
    }
  }
  return true;
}
#endif

static void add_destroyed_data(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen)
{
  G_ASSERTF(desc.isValid(), "add_destroyed_data: attemp to add invalid data cell %i idx %i pool %i offs %i", desc.cellIdx, desc.idx,
    desc.pool, desc.offs);
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
  uint32_t end = desc.offs + stride;

  bool found = false;
  for (int i = 0; i < destrPoolData->destroyedInstances.size(); ++i)
  {
    // Merge
    if ((offs >= destrPoolData->destroyedInstances[i].startOffs && offs <= destrPoolData->destroyedInstances[i].endOffs) ||
        (end >= destrPoolData->destroyedInstances[i].startOffs && end <= destrPoolData->destroyedInstances[i].endOffs))
    {
      destrPoolData->destroyedInstances[i].startOffs = min(destrPoolData->destroyedInstances[i].startOffs, offs);
      destrPoolData->destroyedInstances[i].endOffs = max(destrPoolData->destroyedInstances[i].endOffs, end);
      for (int j = i + 1; j < destrPoolData->destroyedInstances.size(); ++j) // Append next overlapped ranges
      {
        if (destrPoolData->destroyedInstances[j].startOffs <= destrPoolData->destroyedInstances[i].endOffs)
        {
          destrPoolData->destroyedInstances[i].endOffs =
            max(destrPoolData->destroyedInstances[i].endOffs, destrPoolData->destroyedInstances[j].endOffs);
          erase_items(destrPoolData->destroyedInstances, j, 1);
        }
      }
      found = true;
      break;
    }
    // Insert between
    if ((i == 0 || offs > destrPoolData->destroyedInstances[i - 1].endOffs) && end < destrPoolData->destroyedInstances[i].startOffs)
    {
      insert_item_at(destrPoolData->destroyedInstances, i, rendinst::DestroyedInstanceRange(offs, end));
      found = true;
      break;
    }
  }
  if (!found) // it should be last then
    destrPoolData->destroyedInstances.emplace_back(rendinst::DestroyedInstanceRange(offs, end));

  G_ASSERT(debug_verify_destroy_pool_data(*destrPoolData, offs, end));
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
    size = v_max(v_mul(size, v_splats(ri.fxScale)), rendinst::gen::VC_1div10);

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

static void play_riextra_destroy_effect(rendinst::riex_handle_t id, mat44f_cref tm, rendinst::ri_damage_effect_cb effect_cb,
  bool bbScale, bool restorable, const Point3 *coll_point /*=nullptr*/)
{
  uint32_t res_idx = rendinst::handle_to_ri_type(id);
  G_ASSERTF(res_idx < rendinst::riExtra.size(), "res_idx=%d", res_idx);
  rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (effect_cb && (pool.destrFxType != -1 || pool.destrCompositeFxId != -1 || !pool.destrFxTemplate.empty()))
  {
    DECL_ALIGN16(TMatrix, s_scaleTm);
    mat44f scaleTm;
    vec3f size = V_C_ONE;
    if (bbScale)
      size = v_bbox3_size(pool.lbb);
    size = v_max(v_mul(size, v_splats(pool.destrFxScale)), rendinst::gen::VC_1div10);

    scaleTm.col0 = v_mul(size, V_C_UNIT_1000);
    scaleTm.col1 = v_mul(size, V_C_UNIT_0100);
    scaleTm.col2 = v_mul(size, V_C_UNIT_0010);
    scaleTm.col3 = v_zero();

    v_mat44_mul43(scaleTm, tm, scaleTm);
    v_mat_43cu_from_mat44(s_scaleTm.m[0], scaleTm);
    if (coll_point)
      s_scaleTm.setcol(3, Point3::xVz(*coll_point, s_scaleTm.getcol(3).y));

    if (pool.destrFxType != -1 || !pool.destrFxTemplate.empty())
      effect_cb(pool.destrFxType, TMatrix::IDENT, s_scaleTm, res_idx, false, nullptr, pool.destrFxTemplate.c_str());

    if (pool.destrCompositeFxId != -1)
      rifx::composite::spawnEntitiy(id, pool.destrCompositeFxId, s_scaleTm, restorable);
  }
}

DynamicPhysObjectData *rendinst::doRIGenDestr(const RendInstDesc &desc, RendInstBufferData &out_buffer, bool create_destr_effects,
  ri_damage_effect_cb effect_cb, riex_handle_t &out_destroyed_riex, int32_t user_data, const Point3 *coll_point, bool *ri_removed,
  DestrOptionFlags destroy_flags, const Point3 &impulse, const Point3 &impulse_pos)
{
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  rendinst::RendInstDesc restorableDesc = desc;
  if (desc.isRiExtra())
  {
    if (!riExtra[desc.pool].isInGrid(desc.idx))
      return nullptr; // it can be already destroyed

    const int cellId = riExtra[desc.pool].riUniqueData[desc.idx].cellId;
    riex_handle_t id = rendinst::make_handle(desc.pool, desc.idx);
    bool isDynamicRi = riExtra[desc.pool].isDynamicRendinst || cellId < 0;
    rendinst::onRiExtraDestruction(id, isDynamicRi, create_destr_effects, user_data, impulse, impulse_pos);
    if (isDynamicRi)
      return nullptr;

    G_ASSERT(cellId >= 0);
    if (cellId < 0)
      return nullptr;

    int instanceOffset = riExtra[desc.pool].riUniqueData[desc.idx].offset;
    mat44f tm;
    if (rendinst::damageRIGenExtra(id, FLT_MAX, &tm, out_destroyed_riex, nullptr /*offs*/, destroy_flags)) // we want to definitely
                                                                                                           // destroy it if external
                                                                                                           // system says that we
                                                                                                           // should do it.
    {
      out_buffer.tm = tm;
      if (rgl)
      {
        if (effect_cb)
          play_destroy_effect(rgl->rtData, riExtra[desc.pool].riPoolRef, tm, effect_cb, false, coll_point);
      }
      restorableDesc.cellIdx = -(cellId + 1);
      restorableDesc.offs = instanceOffset;
      if (rgl)
        add_destroyed_data(restorableDesc, rgl);

      if (effect_cb)
        play_riextra_destroy_effect(id, tm, effect_cb, false /*bbScale*/, true /*restorable*/, coll_point);
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
  riutil::remove_rendinst(desc, rgl, *cell->rtData, data);
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

  restorableDesc.idx = 0;
  restorableDesc.offs = offs;
  add_destroyed_data(restorableDesc, rgl);

  return res;
}

DynamicPhysObjectData *rendinst::doRIGenDestrEx(const RendInstDesc &desc, bool create_destr_effects, ri_damage_effect_cb effect_cb,
  int user_data)
{
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (desc.isRiExtra())
  {
    const RiExtraPool::ElemUniqueData *uniqueData = safe_at(riExtra[desc.pool].riUniqueData, desc.idx);
    bool isDynamicRi = riExtra[desc.pool].isDynamicRendinst || (uniqueData && uniqueData->cellId < 0);
    rendinst::onRiExtraDestruction(rendinst::make_handle(desc.pool, desc.idx), isDynamicRi, create_destr_effects, user_data);
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
    rendinst::riex_handle_t destroyedRiexHandle;
    ;
    DestrOptionFlags flags = DestrOptionFlag::AddDestroyedRi | DestrOptionFlag::ForceDestroy;
    if (rendinst::damageRIGenExtra(id, FLT_MAX, &tm, destroyedRiexHandle, nullptr /*offs*/, flags) && rgl) // we want to definitely
                                                                                                           // destroy it
                                                                                                           // if external
    // system says that we should do it.
    {
      add_destroyed_data(desc, rgl);
      if (effect_cb)
        play_riextra_destroy_effect(id, tm, effect_cb, false /*bbScale*/, false /*restorable*/);

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
  riutil::remove_rendinst(desc, rgl, *cell->rtData, data);

  if (desc.pool >= rgl->rtData->riDestr.size() || desc.pool < 0)
    return nullptr;

  add_destroyed_data(desc, rgl);
  return rgl->rtData->riDestr[desc.pool].res;
}

DynamicPhysObjectData *rendinst::doRIExGenDestrEx(rendinst::riex_handle_t riex_handle, ri_damage_effect_cb effect_cb)
{
  if (effect_cb)
  {
    mat44f tm;
    mat43f m43 = rendinst::getRIGenExtra43(riex_handle);
    v_mat43_transpose_to_mat44(tm, m43);
    play_riextra_destroy_effect(riex_handle, tm, effect_cb, false /*bbScale*/);
  }
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
  int maxData = RIGEN_STRIDE_B(rgl->rtData->riPosInst[desc.pool], rgl->rtData->riZeroInstSeeds[desc.pool], rgl->perInstDataDwords) / 2;
  for (int i = 0; i < maxData; ++i)
    data[i] = buffer.data[i];

  if (RendInstGenData::renderResRequired)
    rgl->updateVb(crt, desc.cellIdx);

  riutil::world_version_inc(crt.bbox[0]);

  return true;
}

bool rendinst::should_clear_external_controls()
{ // make sure that riExtra is not shut down
  return rendinst::getRgLayer(0) != nullptr;
}

bool rendinst::returnRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri)
{
  RendInstGenData *rgl = rendinst::getRgLayer(desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRefLayer : desc.layer);
  ScopedLockWrite lock(rgl->rtData->riRwCs); //-V522
  if (!ri || !restoreRiGen(desc, ri->savedData))
    return false;

  return removeRIGenExternalControl(desc, ri, false /*lock*/);
}

bool rendinst::removeRIGenExternalControl(const RendInstDesc &desc, DestroyedRi *ri, bool lock)
{
  if (!ri)
    return false;

  RendInstGenData *rgl = rendinst::getRgLayer(desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRefLayer : desc.layer);
  ScopedLockWrite sl(lock ? &rgl->rtData->riRwCs : nullptr); //-V522
  G_ASSERT(should_clear_external_controls());
  int pool = desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRef : desc.pool;

  const int debrisNo = rgl->rtData->riDebrisMap[pool].delayedPoolIdx;
  auto &riDebrisDelayedRi = rgl->rtData->riDebrisDelayedRi;
  // To consider: use `riDebrisDelayedRi[debrisNo % N]` if you need speed-up this lin search
  auto it = eastl::find_if(riDebrisDelayedRi.begin(), riDebrisDelayedRi.end(), [&](auto &pri) { return pri.get() == ri; });
  if (it != riDebrisDelayedRi.end())
  {
    G_ASSERT_RETURN((*it)->debrisNo == debrisNo, false); // Deleted pointer that was re-used?
    riDebrisDelayedRi.erase(it);
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
    if (!rgl)
      return RIEX_HANDLE_NULL;
    RendInstGenData::Cell *cell = nullptr;
    int16_t *data = riutil::get_data_by_desc_no_subcell(desc, cell);
    if (!data)
      return RIEX_HANDLE_NULL;

    RendInstGenData::CellRtData &crt = *cell->rtData;
    int maxData =
      RIGEN_STRIDE_B(rgl->rtData->riPosInst[desc.pool], rgl->rtData->riZeroInstSeeds[desc.pool], rgl->perInstDataDwords) / 2;
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

  if (!rgl)
    return h;

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

      G_ASSERT(debug_verify_destroy_pool_data(pool));
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

  const rendinstdestr::TreeDestr &treeDestr = rendinstdestr::get_tree_destr();
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
    if (!objectDmgBlk) //-V1051 It's possible that the 'defaultDmgBlk' should be checked here.
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

      riProperties[i].treeBranchDestrFromDamage = treeDestr.branchDestrFromDamage;
      rendinstdestr::branch_destr_load_from_blk(riProperties[i].treeBranchDestrFromDamage,
        objectDmgBlk->getBlockByNameEx("branchDestr"));
      riProperties[i].treeBranchDestrOther = treeDestr.branchDestrOther;
      rendinstdestr::branch_destr_load_from_blk(riProperties[i].treeBranchDestrOther,
        objectDmgBlk->getBlockByNameEx("branchDestrOther"));

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

      if (riDebrisMap[i].needsUpdate())
      {
        int idx = debris_nm.addNameId(riResName[i]);
        if (idx == riDebris.size())
          riDebris.push_back().props = &riDebrisMap[i];
        riDebrisMap[i].delayedPoolIdx = idx;
      }
    }
  }

  if (const DataBlock *soundOccludersBlk = ri_blk.getBlockByName("soundOccluders"))
    if (const DataBlock *soundOccluderTypesBlk = ri_blk.getBlockByName("soundOccluderTypes"))
    {
      G_ASSERT(riResName.size() == riProperties.size());
      const int ricount = riResName.size();
      for (int i = 0; i < ricount; ++i)
      {
        float value = 0.f;
        if (const char *type = soundOccludersBlk->getStr(riResName[i], nullptr))
          value = soundOccluderTypesBlk->getReal(type, 0.f);
        riProperties[i].soundOcclusion = value;
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

      if (riProperties[curRi.riPoolRef].immortal || (rendinst::riExtra[i].initialHP < 0.f && !riDestr[curRi.riPoolRef].destructable))
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


void RendInstGenData::RtData::addDebris(mat44f &tm, int pool_idx, unsigned frameNo, const Point3 &axis, float accumulated_power)
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
      auto &debris = riDebris[debrisNo];
      G_ASSERT(debris.props);
      rendinst::DestroyedRi *ri = new rendinst::DestroyedRi();
      riDebrisDelayedRi.emplace_back(ri);
      rendinst::riExtra[debris.resIdx].useShadow = true;
      ri->debrisNo = debrisNo;
      ri->riHandle = rendinst::addRIGenExtra44(debris.resIdx, false, tm, false, -1, -1);
      ri->propsId = debris.props->needsUpdate() ? (debris.props - riDebrisMap.data()) : -1;
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
}

rendinst::DestroyedRi *RendInstGenData::RtData::addExternalDebris(mat44f &tm, int pool_idx)
{
  if (pool_idx >= riDebrisMap.size() || pool_idx < 0)
    return nullptr;
  int debrisNo = riDebrisMap[pool_idx].delayedPoolIdx;
  if (debrisNo < 0)
    return nullptr;

  auto &debris = riDebris[debrisNo];
  G_ASSERT(debris.props);
  rendinst::DestroyedRi *ri = new rendinst::DestroyedRi;
  riDebrisDelayedRi.emplace_back(ri);
  rendinst::riExtra[debris.resIdx].useShadow = true;
  ri->riHandle = rendinst::addRIGenExtra44(debris.resIdx, false, tm, false, -1, -1);
  ri->debrisNo = debrisNo;
  ri->propsId = -1; // Doesn't matter since `shouldUpdate` == false
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

void RendInstGenData::RtData::updateDelayedDebrisRi(float dt, bbox3f *movedDebrisBbox)
{
  TIME_PROFILE(updateDebris);

  for (auto it = riDebrisDelayedRi.begin(); it != riDebrisDelayedRi.end();)
  {
    auto ri = it->get();
    G_ASSERT(ri->propsId < 0 || riDebris[ri->debrisNo].props == &riDebrisMap[ri->propsId]);
    if (!ri->shouldUpdate)
    {
      it++;
      continue;
    }
    if (ri->timeToDamage > dt)
      ri->timeToDamage -= dt;
    else
    {
      it = riDebrisDelayedRi.erase(it);
      curDebris[1]--;
      continue;
    }
    if (ri->propsId < 0)
    {
      it++;
      continue;
    }
    const auto &p = riDebrisMap[ri->propsId];
    G_ASSERT(p.needsUpdate());

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
      if (movedDebrisBbox)
        v_bbox3_add_pt(*movedDebrisBbox, tm.col3);
    }
    else
      ri->timeToDamage = 0.f;
    it++;
  }
}

rendinst::DestroyedRi::~DestroyedRi() { rendinst::delRIGenExtra(riHandle); }

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
