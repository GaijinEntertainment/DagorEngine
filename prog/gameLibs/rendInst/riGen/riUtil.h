// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "riGen/riGenData.h"

#include <osApiWrappers/dag_atomic.h>


namespace riutil
{
extern volatile int worldVersion;

static inline int world_version_get() { return interlocked_acquire_load(worldVersion); }
void world_version_inc(bbox3f_cref mod_aabb);
bool world_version_check_slow(int &version, const BBox3 &query_aabb);
static inline bool world_version_check(int &version, const BBox3 &query_aabb)
{
  if (version == world_version_get())
    return true;
  return world_version_check_slow(version, query_aabb);
}

int16_t *get_data_by_desc(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell);
bool is_rendinst_data_destroyed(const rendinst::RendInstDesc &desc);
bool get_rendinst_matrix(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, const int16_t *data,
  const RendInstGenData::Cell *cell, mat44f &out_tm);
bool get_cell_by_desc(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell);

int16_t *get_data_by_desc_no_subcell(const rendinst::RendInstDesc &desc, RendInstGenData::Cell *&cell);
int get_data_offs_from_start(const rendinst::RendInstDesc &desc);
bool extract_buffer_data(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, int16_t *data,
  rendinst::RendInstBufferData &out_buffer);
void remove_rendinst(const rendinst::RendInstDesc &desc, RendInstGenData *ri_gen, RendInstGenData::CellRtData &crt, int16_t *data);
}; // namespace riutil
