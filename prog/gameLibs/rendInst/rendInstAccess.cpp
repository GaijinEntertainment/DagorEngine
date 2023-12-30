#include <rendInst/rendInstAccess.h>

#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "riGen/genObjUtil.h"
#include "riGen/riUtil.h"


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
    if (rendinst::is_pos_rendinst_data_destroyed(data))
      return TMatrix::IDENT;

    int32_t paletteId;
    rendinst::gen::unpack_tm_pos_fast(tm, data, cell_x0, cell_h0, cell_z0, rgl->grid2world * rgl->cellSz, cell_dh, palette_rotation,
      &paletteId);
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
    if (rendinst::is_tm_rendinst_data_destroyed(data))
      return TMatrix::IDENT;

    rendinst::gen::unpack_tm_full(tm, data, cell_x0, cell_h0, cell_z0, rgl->grid2world * rgl->cellSz, cell_dh);
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
    return RIGEN_STRIDE_B(false, 0);
  return RIGEN_STRIDE_B(rgl->rtData && pool_id < rgl->rtData->riPosInst.size() ? rgl->rtData->riPosInst[pool_id] : false,
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
  const auto &rd = rendinst::riExtra[ri_desc.pool].riUniqueData[ri_desc.idx];

  if (rd.cellId < 0)
    return {};

  RendInstDesc result = ri_desc;
  result.cellIdx = -(rd.cellId + 1);
  result.offs = rd.offset;
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
