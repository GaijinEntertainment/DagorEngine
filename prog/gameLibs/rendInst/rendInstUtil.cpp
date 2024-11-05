// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "riGen/riUtil.h"
#include "riGen/genObjUtil.h"
#include "riGen/riGenData.h"

#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <osApiWrappers/dag_critSec.h>
#include <math/dag_mathUtils.h>


#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("ri_world", draw_world_updates, false);
#endif

#define NUM_WORLDVERSION_BOXES 64

volatile int riutil::worldVersion = 0;
static WinCritSec worldVersionLock;
static carray<bbox3f, NUM_WORLDVERSION_BOXES> worldVersionModBoxes;

void riutil::world_version_inc(bbox3f_cref mod_aabb)
{
  bbox3f tmp;
  // Expand the box a bit, if this is moving / rotating rendinst we better
  // invalidate what's around it.
  tmp.bmin = v_sub(mod_aabb.bmin, V_C_TWO);
  tmp.bmax = v_add(mod_aabb.bmax, V_C_TWO);
#if DAGOR_DBGLEVEL > 0
  if (draw_world_updates)
  {
    BBox3 box;
    v_stu_bbox3(box, tmp);
    draw_debug_box_buffered(box, E3DCOLOR_MAKE(0, 0, 255, 255), 600);
  }
#endif
  WinAutoLock lock(worldVersionLock);
  worldVersionModBoxes[interlocked_increment(worldVersion) % NUM_WORLDVERSION_BOXES] = tmp;
}

bool riutil::world_version_check_slow(int &version, const BBox3 &query_aabb)
{
  WinAutoLock lock(worldVersionLock);
  int worldVersionCur = world_version_get(); // we're holding a lock, world version can't change.
  int diff = worldVersionCur - version;
  if (diff == 0)
    return true;
  if ((diff > NUM_WORLDVERSION_BOXES) || (diff < 0))
    return false; // Version is too old, force invalidation.
  int idx = worldVersionCur % NUM_WORLDVERSION_BOXES;
  int startPos = idx - diff + 1;
  if (startPos >= 0)
  {
    // We only need to check one half of the ring buffer [startPos, idx].
    for (int i = startPos; i <= idx; ++i)
      if (v_bbox3_test_box_intersect(v_ldu_bbox3(query_aabb), worldVersionModBoxes[i]))
      {
        // Optimization: force version to some outdated so the next time we check it
        // we're not going to go through all these box checks again.
        version = worldVersionCur - NUM_WORLDVERSION_BOXES - 1;
        return false;
      }
  }
  else
  {
    // Need to check 2 halves, check [0, idx] first then [N - startPos, N] in order to access memory
    // in linear fasion.
    for (int i = 0; i <= idx; ++i)
      if (v_bbox3_test_box_intersect(v_ldu_bbox3(query_aabb), worldVersionModBoxes[i]))
      {
        version = worldVersionCur - NUM_WORLDVERSION_BOXES - 1;
        return false;
      }
    for (int i = NUM_WORLDVERSION_BOXES + startPos; i < NUM_WORLDVERSION_BOXES; ++i)
      if (v_bbox3_test_box_intersect(v_ldu_bbox3(query_aabb), worldVersionModBoxes[i]))
      {
        version = worldVersionCur - NUM_WORLDVERSION_BOXES - 1;
        return false;
      }
  }
  // World was invalidated, but somewhere far away from us, consider we still have
  // the right world version. We also need to update 'version' of course in order to not
  // force invalidate later.
  version = worldVersionCur;
  return true;
}

int16_t *riutil::get_data_by_desc(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell)
{
  G_ASSERT(!desc.isRiExtra());
  if (!get_cell_by_desc(desc, cell))
    return nullptr;

  RendInstGenData::CellRtData &crt = *cell->rtData;
  G_ASSERTF(!crt.scsRemap.empty(), "Please make full dump");
  if (crt.scsRemap.empty())
    return nullptr;
  const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(desc.pool, desc.idx);
  if (!scs.sz || int(desc.offs) < 0 ||
      (int16_t *)(crt.sysMemData.get() + scs.ofs + desc.offs) >= (int16_t *)(crt.sysMemData.get() + scs.ofs + scs.sz))
    return nullptr;

  return (int16_t *)(crt.sysMemData.get() + scs.ofs + desc.offs);
}

bool riutil::is_rendinst_data_destroyed(const rendinst::RendInstDesc &desc)
{
  RendInstGenData::Cell *cell = nullptr;
  int16_t *data = riutil::get_data_by_desc(desc, cell);
  if (!data)
    return true;
  RendInstGenData *rgl = RendInstGenData::getGenDataByLayer(desc);
  return rgl->rtData->riPosInst[desc.pool] ? rendinst::is_pos_rendinst_data_destroyed(data)
                                           : rendinst::is_tm_rendinst_data_destroyed(data);
}

bool riutil::get_rendinst_matrix(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, const int16_t *data,
  const RendInstGenData::Cell *cell, mat44f &out_tm)
{
  G_ASSERT(!desc.isRiExtra());
  static constexpr int SUBCELL_DIV = RendInstGenData::SUBCELL_DIV;
  const RendInstGenData::CellRtData *crt_ptr = cell->isReady();
  if (!crt_ptr)
    return false;
  const RendInstGenData::CellRtData &crt = *crt_ptr;

  float subcellSz = ri_gen->grid2world * ri_gen->cellSz / SUBCELL_DIV;
  vec3f v_cell_add = crt.cellOrigin;
  vec3f v_cell_mul =
    v_mul(rendinst::gen::VC_1div32767, v_make_vec4f(subcellSz * SUBCELL_DIV, crt.cellHeight, subcellSz * SUBCELL_DIV, 0));
  bool palette_rotation = ri_gen->rtData->riPaletteRotation[desc.pool];
  if (ri_gen->rtData->riPosInst[desc.pool])
  {
    if (palette_rotation)
    {
      vec4f v_pos, v_scale;
      vec4i v_palette_id;
      if (!rendinst::gen::unpack_tm_pos(v_pos, v_scale, data, v_cell_add, v_cell_mul, palette_rotation, &v_palette_id))
        return false;

      rendinst::gen::RotationPaletteManager::Palette palette =
        rendinst::gen::get_rotation_palette_manager()->getPalette({crt.rtData->layerIdx, desc.pool});
      quat4f v_rot = rendinst::gen::RotationPaletteManager::get_quat(palette, v_extract_xi(v_palette_id));

      v_mat44_compose(out_tm, v_pos, v_rot, v_scale);
      return true;
    }
    else
    {
      return rendinst::gen::unpack_tm_pos(out_tm, data, v_cell_add, v_cell_mul, palette_rotation);
    }
  }
  else
  {
    return rendinst::gen::unpack_tm_full(out_tm, data, v_cell_add, v_cell_mul);
  }
}

bool riutil::get_cell_by_desc(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell)
{
  if (desc.isRiExtra())
    return false;
  RendInstGenData *rgl = rendinst::getRgLayer(desc.layer);
  if (!rgl || !rgl->rtData)
    return false;

  if (desc.cellIdx < 0 || desc.cellIdx >= rgl->cells.size())
    return false;

  cell = &rgl->cells[desc.cellIdx];
  if (!cell->isReady())
    return false;

  if (desc.pool < 0 || desc.pool >= rgl->rtData->riPosInst.size())
    return false;

  return true;
}

int16_t *riutil::get_data_by_desc_no_subcell(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell)
{
  G_ASSERT(!desc.isRiExtra());
  if (!riutil::get_cell_by_desc(desc, cell))
    return nullptr;

  RendInstGenData::CellRtData &crt = *cell->rtData;
  const RendInstGenData::CellRtData::SubCellSlice &firstScs = crt.getCellSlice(desc.pool, 0);
  const RendInstGenData::CellRtData::SubCellSlice &lastScs =
    crt.getCellSlice(desc.pool, RendInstGenData::SUBCELL_DIV * RendInstGenData::SUBCELL_DIV - 1);
  if (int(desc.offs) < 0 ||
      (int16_t *)(crt.sysMemData.get() + firstScs.ofs + desc.offs) >= (int16_t *)(crt.sysMemData.get() + lastScs.ofs + lastScs.sz))
    return nullptr;

  return (int16_t *)(crt.sysMemData.get() + firstScs.ofs + desc.offs);
}

int riutil::get_data_offs_from_start(const rendinst::RendInstDesc &desc)
{
  G_ASSERT(!desc.isRiExtra());
  RendInstGenData::Cell *cell = nullptr;
  if (!riutil::get_cell_by_desc(desc, cell))
    return -1;

  RendInstGenData::CellRtData &crt = *cell->rtData;
  const RendInstGenData::CellRtData::SubCellSlice &firstScs = crt.getCellSlice(desc.pool, 0);
  const RendInstGenData::CellRtData::SubCellSlice &scs = crt.getCellSlice(desc.pool, desc.idx);
  return desc.offs + scs.ofs - firstScs.ofs;
}

bool riutil::extract_buffer_data(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, int16_t *data,
  rendinst::RendInstBufferData &out_buffer)
{
  G_ASSERT(!desc.isRiExtra());
  if (ri_gen->rtData->riPosInst[desc.pool])
  {
    if (rendinst::is_pos_rendinst_data_destroyed(data))
      return false;

    for (int i = 0; i < 12; ++i)
    {
      if (i < 4)
        out_buffer.data[i] = data[i];
      else
        out_buffer.data[i] = 0;
    }
  }
  else
  {
    if (rendinst::is_tm_rendinst_data_destroyed(data))
      return false;

    for (int i = 0; i < 12; ++i)
      out_buffer.data[i] = data[i];
  }
  return true;
}

void riutil::remove_rendinst(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, RendInstGenData::CellRtData &crt,
  int16_t *data)
{
  if (desc.isRiExtra())
    return; //== what should it DO?
  auto mrange = make_span_const(crt.sysMemData.get(), crt.vbSize);
  if (ri_gen->rtData->riPosInst[desc.pool])
    rendinst::destroy_pos_rendinst_data(data, mrange);
  else
    rendinst::destroy_tm_rendinst_data(data, mrange);
  world_version_inc(crt.bbox[0]);
}
