// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstAccess.h>

#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "riGen/genObjUtil.h"
#include "riGen/riUtil.h"
#include "render/genRender.h"
#include "visibility/genVisibility.h"
#include <vecmath/dag_vecMath.h>
#include <math/dag_mathUtils.h>

rendinst::AutoLockReadPrimaryAndExtra::AutoLockReadPrimaryAndExtra()
{
  for (int l = 0; l < rendinst::rgPrimaryLayers; l++)
    if (RendInstGenData *rgl = rendinst::rgLayer[l])
      rgl->rtData->riRwCs.lockRead();
  rendinst::ccExtra.lockRead();
}

rendinst::AutoLockReadPrimaryAndExtra::~AutoLockReadPrimaryAndExtra()
{
  for (int l = rendinst::rgPrimaryLayers - 1; l >= 0; l--)
    if (RendInstGenData *rgl = rendinst::rgLayer[l])
      rgl->rtData->riRwCs.unlockRead();
  rendinst::ccExtra.unlockRead();
}

int rendinst::getRIGenMaterialId(const RendInstDesc &desc, bool need_lock)
{
  int pool = desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRef : desc.pool;
  if (pool < 0)
    return -1;
  RendInstGenData *rgl = rendinst::getRgLayer(desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRefLayer : desc.layer);
  if (rgl)
  {
    ScopedLockRead lock(need_lock ? &rgl->rtData->riRwCs : nullptr);
    G_ASSERTF(pool < rgl->rtData->riProperties.size(), "getRIGenMaterialId failed pool %i < %i", pool,
      rgl->rtData->riProperties.size());
    return rgl->rtData->riProperties[pool].matId;
  }
  return -1;
}


bool rendinst::getRIGenCanopyBBox(const RendInstDesc &desc, const TMatrix &tm, BBox3 &out_canopy_bbox, bool need_lock)
{
  int pool = desc.isRiExtra() ? riExtra[desc.pool].riPoolRef : desc.pool;
  if (pool < 0)
    return false;

  RendInstGenData *rgl = rendinst::getRgLayer(desc.isRiExtra() ? rendinst::riExtra[desc.pool].riPoolRefLayer : desc.layer);
  if (!rgl)
    return false;

  ScopedLockRead lock(need_lock ? &rgl->rtData->riRwCs : nullptr);

  const auto &riProps = rgl->rtData->riProperties;
  if (pool >= riProps.size())
  {
    logerr("getRIGenCanopyBBox: no 'riProperties' for an invalid pool id '%i', totalPoolsCount=%i", pool, riProps.size());
    return false;
  }

  const auto &riRes = rgl->rtData->riRes[pool];
  if (!riRes)
  {
    logerr("getRIGenCanopyBBox: no 'riRes' for an invalid pool id '%i', totalPoolsCount=%i", pool, riProps.size());
    return false;
  }

  mat44f riTm;
  v_mat44_make_from_43cu_unsafe(riTm, tm.array);

  bbox3f allBBox;
  v_bbox3_init(allBBox, riTm, v_ldu_bbox3(riRes->bbox));

  BBox3 worldBBox;
  v_stu_bbox3(worldBBox, allBBox);

  return getRIGenCanopyBBox(riProps[pool], worldBBox, out_canopy_bbox);
}

CollisionResource *rendinst::getRIGenCollInfo(const rendinst::RendInstDesc &desc)
{
  RendInstGenData *v = RendInstGenData::getGenDataByLayer(desc);
  if (!v)
    return nullptr;
  return v->rtData->riCollRes[desc.pool].collRes;
}

void *rendinst::getCollisionResourceHandle(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
    return (desc.pool >= 0 && desc.pool < riExtra.size()) ? riExtra[desc.pool].collHandle : nullptr;
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return (rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riCollRes.size()) ? rgl->rtData->riCollRes[desc.pool].handle : nullptr;
}

const CollisionResource *rendinst::getRiGenCollisionResource(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
    return (desc.pool >= 0 && desc.pool < riExtra.size()) ? riExtra[desc.pool].collRes : nullptr;
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return (rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riCollRes.size()) ? rgl->rtData->riCollRes[desc.pool].collRes : nullptr;
}

bool rendinst::isRIGenPosInst(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
    return desc.pool >= 0 && desc.pool < riExtra.size() && riExtra[desc.pool].isPosInst();
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riPosInst.size() && rgl->rtData->riPosInst[desc.pool];
}

bool rendinst::isRIGenOnlyPosInst(int layer_ix, int pool_ix)
{
  RendInstGenData *rgl = getRgLayer(layer_ix);
  return rgl && pool_ix >= 0 && pool_ix < rgl->rtData->riPosInst.size() && rgl->rtData->riPosInst[pool_ix];
}

bool rendinst::isValidRILayerAndPool(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
    return unsigned(desc.pool) < rendinst::riExtra.size();
  const RendInstGenData *rgl = rendinst::getRgLayer(desc.layer);
  return rgl != nullptr && rgl->rtData != nullptr && unsigned(desc.pool) < rgl->rtData->rtPoolData.size();
}

bool rendinst::isDestroyedRIExtraFromNextRes(const RendInstDesc &desc)
{
  return desc.isRiExtra() && riExtra[desc.pool].parentForDestroyedRiIdx > 0;
}

int rendinst::getRIExtraNextResIdx(int pool_id)
{
  return (pool_id >= 0 && pool_id < riExtra.size()) ? riExtra[pool_id].destroyedRiIdx : -1;
}

static TMatrix get_ri_matrix_from_data(const rendinst::RendInstDesc &desc, const RendInstGenData::Cell *cell, const int16_t *data)
{
  const RendInstGenData::CellRtData &crt = *cell->rtData;

  const RendInstGenData *rgl = rendinst::getRgLayer(desc.layer);
  float cell_dh = crt.cellHeight;
  float cell_x0 = as_point4(&crt.cellOrigin).x;
  float cell_h0 = as_point4(&crt.cellOrigin).y;
  float cell_z0 = as_point4(&crt.cellOrigin).z;
  TMatrix tm = TMatrix::IDENT;
  if (!rgl)
    return tm;
  bool palette_rotation = rgl->rtData->riPaletteRotation[desc.pool];
  if (rgl->rtData->riPosInst[desc.pool])
  {
    int32_t paletteId;
    if (!rendinst::gen::unpack_tm_pos_fast(tm, data, cell_x0, cell_h0, cell_z0, rgl->grid2world * rgl->cellSz, cell_dh,
          palette_rotation, &paletteId))
      return TMatrix::IDENT;
    if (palette_rotation)
    {
      rendinst::gen::RotationPaletteManager::Palette palette =
        rendinst::gen::get_rotation_palette_manager()->getPalette({desc.layer, desc.pool});
      TMatrix rotTm = rendinst::gen::RotationPaletteManager::get_tm(palette, paletteId);
      tm = tm * rotTm;
    }
  }
  else
  {
    if (!rendinst::gen::unpack_tm_full(tm, data, cell_x0, cell_h0, cell_z0, rgl->grid2world * rgl->cellSz, cell_dh))
      return TMatrix::IDENT;
  }
  return tm;
}

template <bool bLock = true>
static inline TMatrix getRIGenMatrixImpl(const rendinst::RendInstDesc &desc)
{
  TMatrix tm;
  if (desc.isRiExtra())
  {
    if (bLock)
      rendinst::ccExtra.lockRead();
    mat44f tm44;
    if (desc.idx >= rendinst::riExtra[desc.pool].riTm.size()) // Temporary apex RI proxy may be removed at any time.
    {
      if (bLock)
        rendinst::ccExtra.unlockRead();
      tm = TMatrix::IDENT;
      return tm;
    }
    v_mat43_transpose_to_mat44(tm44, rendinst::riExtra[desc.pool].riTm[desc.idx]);
    v_mat_43cu_from_mat44(tm.m[0], tm44);
    if (bLock)
      rendinst::ccExtra.unlockRead();
    return tm;
  }

  if (bLock)
    rendinst::rgLayer[desc.layer]->rtData->riRwCs.lockRead();

  RendInstGenData::Cell *cell = nullptr;
  if (const int16_t *data = riutil::get_data_by_desc(desc, cell))
    tm = get_ri_matrix_from_data(desc, cell, data);
  else
    tm = TMatrix::IDENT;

  if (bLock)
    rendinst::rgLayer[desc.layer]->rtData->riRwCs.unlockRead();

  return tm;
}

TMatrix rendinst::getRIGenMatrix(const RendInstDesc &desc) { return getRIGenMatrixImpl(desc); }
TMatrix rendinst::getRIGenMatrixNoLock(const RendInstDesc &desc) { return getRIGenMatrixImpl</*bLock*/ false>(desc); }

TMatrix rendinst::getRIGenMatrixDestr(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
  {
    rendinst::ccExtra.lockRead();
    int idx = desc.cellIdx == -1 ? desc.idx : -1;
    for (int i = 0; i < riExtra[desc.pool].riUniqueData.size(); ++i)
    {
      if (riExtra[desc.pool].riUniqueData[i].cellId != -(desc.cellIdx + 1) || riExtra[desc.pool].riUniqueData[i].offset != desc.offs)
        continue;
      idx = i;
      break;
    }
    if (idx >= 0)
    {
      TMatrix tm;
      mat44f tm44;
      v_mat43_transpose_to_mat44(tm44, riExtra[desc.pool].riTm[idx]);
      v_mat_43cu_from_mat44(tm.m[0], tm44);
      rendinst::ccExtra.unlockRead();
      return tm;
    }
    rendinst::ccExtra.unlockRead();
    return TMatrix::IDENT;
  }

  ScopedLockRead lock(rendinst::rgLayer[desc.layer]->rtData->riRwCs);

  TMatrix tm;
  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc_no_subcell(desc, cell);
  if (data)
    tm = get_ri_matrix_from_data(desc, cell, data);
  else
    tm = TMatrix::IDENT;

  return tm;
}


const char *rendinst::getRIGenResName(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
    return rendinst::riExtraMap.getName(desc.pool);
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riResName.size() ? rgl->rtData->riResName[desc.pool] : nullptr;
}

const char *rendinst::getRIGenDestrFxTemplateName(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
  {
    RendInstGenData *rgl = getRgLayer(riExtra[desc.pool].riPoolRefLayer);
    int pool = riExtra[desc.pool].riPoolRef;
    return (rgl && pool) ? rgl->rtData->riDebrisMap[pool].fxTemplate.str() : nullptr;
  }
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riDebrisMap.size() ? rgl->rtData->riDebrisMap[desc.pool].fxTemplate.str()
                                                                              : nullptr;
}

const char *rendinst::getRIGenDestrName(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
  {
    RendInstGenData *rgl = getRgLayer(riExtra[desc.pool].riPoolRefLayer);
    int pool = riExtra[desc.pool].riPoolRef;
    return (rgl && pool) ? rgl->rtData->riDestr[pool].destrName.str() : nullptr;
  }
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riDestr.size() ? rgl->rtData->riDestr[desc.pool].destrName.str() : nullptr;
}

bool rendinst::isRIGenDestr(const RendInstDesc &desc)
{
  if (desc.isRiExtra())
  {
    if (riExtra[desc.pool].initialHP >= 0)
      return true;
    RendInstGenData *rgl = getRgLayer(riExtra[desc.pool].riPoolRefLayer);
    int pool = riExtra[desc.pool].riPoolRef;
    return (rgl && pool >= 0) ? rgl->rtData->riDestr[pool].destructable : false;
  }
  RendInstGenData *rgl = getRgLayer(desc.layer);
  return rgl && desc.pool >= 0 && desc.pool < rgl->rtData->riDestr.size() && rgl->rtData->riDestr[desc.pool].destructable;
}

int rendinst::getRIGenStrideRaw(int layer_idx, int pool_id)
{
  RendInstGenData *rgl = getRgLayer(layer_idx);
  if (!rgl)
    return RIGEN_STRIDE_B(false, false, 0);
  return RIGEN_STRIDE_B(rgl->rtData && pool_id < rgl->rtData->riPosInst.size() ? rgl->rtData->riPosInst[pool_id] : false,
    rgl->rtData && pool_id < rgl->rtData->riZeroInstSeeds.size() ? rgl->rtData->riZeroInstSeeds[pool_id] : false,
    rgl->perInstDataDwords);
}

int rendinst::getRIGenStride(int layer_idx, int cell_id, int pool_id)
{
  return cell_id < 0 ? 16 : getRIGenStrideRaw(layer_idx, pool_id);
}

BBox3 rendinst::getRIGenBBox(const RendInstDesc &desc)
{
  BBox3 box;
  G_ASSERT(desc.isValid());
  if (!desc.isValid())
    return box;
  if (desc.isRiExtra())
  {
    if (const rendinst::RiExtraPool *pool = safe_at(riExtra, desc.pool))
      v_stu_bbox3(box, pool->collBb);
    return box;
  }
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (rgl && desc.pool < rgl->rtData->riRes.size())
    v_stu_bbox3(box, rgl->rtData->riCollResBb[desc.pool]);
  return box;
}

BBox3 rendinst::getRIGenFullBBox(const RendInstDesc &desc)
{
  BBox3 box;
  G_ASSERT(desc.isValid());
  if (!desc.isValid())
    return box;
  if (desc.isRiExtra())
  {
    if (const rendinst::RiExtraPool *pool = safe_at(riExtra, desc.pool))
      v_stu_bbox3(box, pool->lbb);
    return box;
  }
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  if (rgl && desc.pool < rgl->rtData->riRes.size())
    v_stu_bbox3(box, rgl->rtData->riResBb[desc.pool]);
  return box;
}

int rendinst::get_debris_fx_type_id(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc)
{
  return rgl->rtData->riDebrisMap[ri_desc.pool].fxType;
}

rendinst::RiDestrData rendinst::gather_ri_destr_data(const RendInstDesc &ri_desc)
{
  RiDestrData result{1, false, -1, 1, ""};

  if (ri_desc.layer < rgLayer.size() && rgLayer[ri_desc.layer] != nullptr)
    if (const auto *rtData = rgLayer[ri_desc.layer]->rtData)
    {
      result.collisionHeightScale = rtData->riDestr[ri_desc.pool].collisionHeightScale;
      result.bushBehaviour = rtData->riProperties[ri_desc.pool].bushBehaviour;
      if (ri_desc.pool < rtData->riDebrisMap.size())
      {
        result.fxScale = rtData->riDebrisMap[ri_desc.pool].fxScale;
        result.fxType = rtData->riDebrisMap[ri_desc.pool].fxType;
        result.fxTemplate = rtData->riDebrisMap[ri_desc.pool].fxTemplate.c_str();
      }
    }

  return result;
}

bool rendinst::ri_gen_has_collision(const RendInstDesc &ri_desc)
{
  G_FAST_ASSERT(!ri_desc.isRiExtra());

  if (ri_desc.layer >= rgLayer.size() || rgLayer[ri_desc.layer] == nullptr || rgLayer[ri_desc.layer]->rtData == nullptr)
    return false;

  return rgLayer[ri_desc.layer]->rtData->riCollRes[ri_desc.pool].collRes != nullptr;
}

rendinst::RendInstDesc rendinst::get_restorable_desc(const RendInstDesc &ri_desc)
{
  RendInstDesc result = ri_desc;
  result.idx = 0;
  if (ri_desc.isRiExtra())
  {
    const auto &rd = rendinst::riExtra[ri_desc.pool].riUniqueData[ri_desc.idx];
    if (rd.cellId < 0)
      return {};

    result.cellIdx = -(rd.cellId + 1);
    result.offs = rd.offset;
  }
  else
    result.offs = riutil::get_data_offs_from_start(ri_desc);
  return result;
}

int rendinst::find_restorable_data_index(const RendInstDesc &desc)
{
  if (unsigned(desc.pool) >= riExtra.size())
    return -1;
  const auto &ud = riExtra[desc.pool].riUniqueData;
  for (int r = 0; r < ud.size(); ++r)
    if (ud[r].cellId == -(desc.cellIdx + 1) && ud[r].offset == desc.offs)
      return r;

  return -1;
}

RenderableInstanceLodsResource *rendinst::getRIGenRes(RendInstGenData *rgl, const RendInstDesc &desc)
{
  if (rgl && desc.pool < rgl->rtData->riRes.size())
    return rgl->rtData->riRes[desc.pool];
  return nullptr;
}

RenderableInstanceLodsResource *rendinst::getRIGenRes(int layer_ix, int pool_ix)
{
  RendInstGenData *rgl = getRgLayer(layer_ix);

  if (!rgl || !rgl->rtData || pool_ix >= rgl->rtData->riRes.size())
    return nullptr;

  return rgl->rtData->riRes[pool_ix];
}

static inline vec4f v_frac(vec4f value) { return v_sub(value, v_round(value)); }

static inline void unpack_entity_data(vec4f pos, vec4f &scale, vec4i &palette_id)
{
  pos = v_splat_w(pos);

  // palette_id = frac(pos * PALETTE_SCALE_MULTIPLIER) * PALETTE_ID_MULTIPLIER
  palette_id = v_cvti_vec4i(v_mul(v_frac(v_mul(pos, v_splats(PALETTE_SCALE_MULTIPLIER))), v_splats(PALETTE_ID_MULTIPLIER)));

  // scale = floor(pos * PALETTE_SCALE_MULTIPLIER) / PALETTE_SCALE_MULTIPLIER;
  scale = v_mul(v_floor(v_mul(pos, v_splats(PALETTE_SCALE_MULTIPLIER))), v_splats(1.0f / PALETTE_SCALE_MULTIPLIER));
}

void rendinst::build_ri_gen_thread_accel(RiGenVisibility *visibility, dag::Vector<uint32_t> &accel1, dag::Vector<uint64_t> &accel2)
{
  accel1.clear();
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
  {
    G_ASSERT(_layer < 8);

    RiGenVisibility &layerVisibility = visibility[_layer];

    for (int lodIx = 0; lodIx < RiGenVisibility::PER_INSTANCE_LODS; ++lodIx)
      for (int cellIx = 0; cellIx < int(layerVisibility.perInstanceVisibilityCells[lodIx].size()) - 1; ++cellIx)
        accel1.push_back((_layer << 29) | (lodIx << 26) | cellIx);
  }

  accel2.clear();
  FOR_EACH_RG_LAYER_RENDER (rgl, rgRenderMaskO)
  {
    RiGenVisibility &layerVisibility = visibility[_layer];

    for (int poolIx = 0; poolIx < rgl->rtData->riRes.size(); ++poolIx)
    {
      auto pool = rgl->rtData->rtPoolData[poolIx];
      if (!pool)
        continue;

      uint32_t bvhId = rgl->rtData->riRes[poolIx]->getBvhId();
      if (bvhId == 0)
        continue;

      int lodCount = rgl->rtData->riResLodCount(poolIx);
      for (int lodIx = 0; lodIx < lodCount; ++lodIx)
      {
        auto &range = layerVisibility.renderRanges[poolIx];
        if (!range.hasCells(lodIx))
          continue;

        for (int iter = range.startCell[lodIx]; iter < range.endCell[lodIx]; ++iter)
          accel2.push_back((uint64_t(_layer) << 61) | (uint64_t(lodIx) << 58) | (uint64_t(poolIx) << 32) | iter);
      }
    }
  }
}

void rendinst::foreachRiGenInstance(RiGenVisibility *visibility, RiGenIterator callback, void *user_data,
  const dag::Vector<uint32_t> &accel1, const dag::Vector<uint64_t> &accel2, volatile int &cursor1, volatile int &cursor2)
{
  for (int index = interlocked_increment(cursor1) - 1; index < accel1.size(); index = interlocked_increment(cursor1) - 1)
  {
    TIME_PROFILE(process_lod_1);

    uint32_t key = accel1[index];
    int _layer = (key >> 29) & 0x7;
    int lodIx = (key >> 26) & 0x7;
    int cellIx = key & 0x3FFFFFF;

    RiGenVisibility &layerVisibility = visibility[_layer];
    RendInstGenData *rgl = rendinst::rgLayer[_layer];

    auto iter1 = layerVisibility.perInstanceVisibilityCells[lodIx][cellIx];
    auto iter2 = layerVisibility.perInstanceVisibilityCells[lodIx][cellIx + 1];

    int pool_idx = iter1.x;
    if (pool_idx < 0)
      continue;
    if (pool_idx > rgl->rtData->rtPoolData.size() || pool_idx > rgl->rtData->riRes.size())
    {
      logerr("Pool idx is out of bound pool_idx: %d, rtPoolData.size: %d, riRes.size: %d", pool_idx, rgl->rtData->rtPoolData.size(),
        rgl->rtData->riRes.size());
      continue;
    }

    if (!rgl->rtData->rtPoolData[pool_idx] || rgl->rtData->isHiddenId(pool_idx))
      continue;

    uint32_t bvhId = rgl->rtData->riRes[pool_idx]->getBvhId();
    if (bvhId == 0)
      continue;

    int realLodIx = lodIx == RiGenVisibility::PI_ALPHA_LOD ? RiGenVisibility::PI_IMPOSTOR_LOD : lodIx;
    int lodTranslation = rendinst::MAX_LOD_COUNT - rgl->rtData->riResLodCount(pool_idx);
    if (lodTranslation > 0 && !rgl->rtData->rtPoolData[pool_idx]->hasImpostor())
      lodTranslation--;

    realLodIx -= lodTranslation;

    bool isBakedImpostor = rgl->rtData->riRes[pool_idx]->isBakedImpostor();
    int lodCnt = rgl->rtData->riResLodCount(pool_idx);
    bool isImpostorLod = realLodIx >= lodCnt - 1 && rgl->rtData->riPosInst[pool_idx];

    int start = iter1.y;
    int count = iter2.y - start;

    TIME_PROFILE(process_instances);
    for (int instIx = start; instIx < start + count; ++instIx)
    {
      vec4f instance_data = layerVisibility.instanceData[lodIx][instIx];

      vec4f v_pos = v_perm_xyzd(instance_data, V_C_ONE);
      vec4f v_scale;
      vec4i paletteId;
      unpack_entity_data(instance_data, v_scale, paletteId);
      rendinst::gen::RotationPaletteManager::Palette palette =
        rendinst::gen::get_rotation_palette_manager()->getPalette({_layer, pool_idx});
      quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(paletteId));

      mat44f tm;
      v_mat44_compose(tm, v_pos, v_rot, v_scale);

      auto colors = &rgl->rtData->riColPair[pool_idx * 2];

      callback(_layer, pool_idx, realLodIx, isBakedImpostor ? lodCnt - 2 : lodCnt - 1, isBakedImpostor && isImpostorLod, tm, colors,
        bvhId, user_data);
    }
  }

  for (int index = interlocked_increment(cursor2) - 1; index < accel2.size(); index = interlocked_increment(cursor2) - 1)
  {
    TIME_PROFILE(process_lod_2);

    uint64_t key = accel2[index];
    int _layer = (key >> 61) & 0x7;
    int lodIx = (key >> 58) & 0x7;
    int poolIx = (key >> 32) & 0x3FFFFFF;
    int iter = key & 0xFFFFFFFFU;

    RiGenVisibility &layerVisibility = visibility[_layer];
    RendInstGenData *rgl = rendinst::rgLayer[_layer];

    auto pool = rgl->rtData->rtPoolData[poolIx];
    uint32_t bvhId = rgl->rtData->riRes[poolIx]->getBvhId();

    auto stride = RIGEN_STRIDE_B(rgl->rtData->riPosInst[poolIx], rgl->rtData->riZeroInstSeeds[poolIx], rgl->perInstDataDwords);
    auto palette = rendinst::gen::get_rotation_palette_manager()->getPalette({_layer, poolIx});

    bool hasImpostor = pool->hasImpostor();
    int lodCount = rgl->rtData->riResLodCount(poolIx);
    bool isImpostorLod = hasImpostor && (lodIx >= lodCount - 1) && rgl->rtData->riPosInst[poolIx];

    auto &cellRange = layerVisibility.cellsLod[lodIx][iter];
    auto cellIx = cellRange.z * rgl->cellNumW + cellRange.x;
    auto crt = rgl->cells[cellIx].rtData;
    auto ptr = crt->sysMemData.get();
    if (!ptr)
      continue;

    float subcellSz = rgl->grid2world * rgl->cellSz;
    vec3f v_cell_add = crt->cellOrigin;
    vec3f v_cell_mul = v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(subcellSz, crt->cellHeight, subcellSz, 0));

    TIME_PROFILE(process_subcells);
    for (int subCellIx = 0; subCellIx < cellRange.startSubCellCnt; ++subCellIx)
    {
      TIME_PROFILE(process_instances);
      auto &subCell = layerVisibility.subCells[subCellIx + cellRange.startSubCell];
      for (int instIx = 0; instIx < subCell.cnt; ++instIx)
      {
        const int16_t *data = (int16_t *)(ptr + cellRange.startVbOfs + (subCell.ofs + instIx) * stride);

        mat44f tm;
        if (rgl->rtData->riPosInst[poolIx])
        {
          bool palette_rotation = rgl->rtData->riPaletteRotation[poolIx];
          if (palette_rotation)
          {
            vec4f v_pos, v_scale;
            vec4i v_palette_id;
            rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, palette_rotation, &v_palette_id);

            quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(v_palette_id));

            v_mat44_compose(tm, v_pos, v_rot, v_scale);
          }
          else
          {
            rendinst::gen::unpack_tm_pos(tm, data, v_cell_add, v_cell_mul, palette_rotation);
          }
        }
        else
        {
          rendinst::gen::unpack_tm_full(tm, data, v_cell_add, v_cell_mul);
        }

        auto colors = &rgl->rtData->riColPair[poolIx * 2];

        callback(_layer, poolIx, lodIx, hasImpostor ? lodCount - 2 : lodCount - 1, isImpostorLod, tm, colors, bvhId, user_data);
      }
    }
  }
}
