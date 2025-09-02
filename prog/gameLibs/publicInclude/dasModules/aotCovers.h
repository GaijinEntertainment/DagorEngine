//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/aotEcs.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasManagedTab.h>
#include <dasModules/aotDagorMath.h>
#include <covers/covers.h>
#include <walkerAi/coverComponent.h>
#include <osApiWrappers/dag_files.h>

using CoverSlots = eastl::fixed_vector<CoverSlot, 8, false>;
using CoverDescs = eastl::vector<CoverDesc>;
using CoverExtras = eastl::vector<CoverExtra>;
using CoversTab = Tab<covers::Cover>;

MAKE_TYPE_FACTORY(Cover, covers::Cover);
MAKE_TYPE_FACTORY(CoverSlot, CoverSlot);
MAKE_TYPE_FACTORY(CoverDesc, CoverDesc);
MAKE_TYPE_FACTORY(CoverExtra, CoverExtra);
MAKE_TYPE_FACTORY(CoversComponent, CoversComponent);
MAKE_TYPE_FACTORY(GlobalVisibleCoversMap, GlobalVisibleCoversMap)

DAS_BIND_VECTOR(CoverSlots, CoverSlots, CoverSlot, " ::CoverSlots")
DAS_BIND_VECTOR(CoverDescs, CoverDescs, CoverDesc, " ::CoverDescs")
DAS_BIND_VECTOR(CoverExtras, CoverExtras, CoverExtra, " ::CoverExtras")

DAS_BIND_VECTOR(CoversTab, CoversTab, covers::Cover, " ::CoversTab")

namespace bind_dascript
{
inline void covers_box_cull(CoversComponent &covers, const bbox3f &box,
  const das::TBlock<void, int, const das::TTemporary<mat44f>> &block, das::Context *context, das::LineInfoArg *at)
{
  vec4f args[2];
  context->invokeEx(
    block, args, nullptr,
    [&](das::SimNode *code) {
      covers.boxCull(box, [&](int id, mat44f_cref tm) {
        args[0] = das::cast<int>::from(id);
        args[1] = das::cast<mat44f_cref>::from(tm);
        code->eval(*context);
      });
    },
    at);
}

inline bool glob_vis_covers_map_has_other_teams(GlobalVisibleCoversMap *visible_map, int cover_index, int team)
{
  return visible_map ? visible_map->hasOtherTeams(cover_index, team) : false;
}

const int SAVE_CVEX_BY_EXACT_IDS = 0;    // don't filter anything even if zero pos or info
const int SAVE_CVEX_IGNORE_ZERO_POS = 1; // save only extra with non-zero pos and any info

inline bool save_covers_extra_to_file(const char *file_name, const CoversComponent &covers, int save_cvex_mode)
{
  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(file_name, DF_WRITE | DF_CREATE), &df_close);
  if (!h)
    return false;

  char xbinHeader[4] = {'X', 'B', 'I', 'N'};
  if (df_write(h.get(), &xbinHeader, 4) != 4)
    return false;
  uint32_t zeroFlags = 0;
  if (df_write(h.get(), &zeroFlags, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  if (sizeof(CoverExtra) != 16) // CVX1 requirement
    return false;
  char dataType[4] = {'C', 'V', 'X', '1'};
  if (df_write(h.get(), &dataType, 4) != 4)
    return false;

  if (save_cvex_mode == SAVE_CVEX_BY_EXACT_IDS)
  {
    uint32_t dataSize = covers.coverExtra.size() * sizeof(CoverExtra);
    if (df_write(h.get(), &dataSize, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;

    if (df_write(h.get(), &covers.coverExtra[0], dataSize) != dataSize)
      return false;
  }
  else if (save_cvex_mode == SAVE_CVEX_IGNORE_ZERO_POS)
  {
    int numFiltered = 0;
    const int numExtra = (int)covers.coverExtra.size();
    for (int i = 0; i < numExtra; ++i)
      if (covers.coverExtra[i].pos != Point3::ZERO)
        ++numFiltered;
    uint32_t dataSize = numFiltered * sizeof(CoverExtra);
    if (df_write(h.get(), &dataSize, sizeof(uint32_t)) != sizeof(uint32_t))
      return false;
    for (int i = 0; i < numExtra; ++i)
      if (covers.coverExtra[i].pos != Point3::ZERO)
        if (df_write(h.get(), &covers.coverExtra[i], sizeof(CoverExtra)) != sizeof(CoverExtra))
          return false;
  }
  else
    return false;

  uint32_t zeroType = 0;
  if (df_write(h.get(), &zeroType, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;
  uint32_t zeroSize = 0;
  if (df_write(h.get(), &zeroSize, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  return true;
}

const int LOAD_CVEX_BY_EXACT_IDS = 0;   // requires same amount in covers and in file, else will return 'false'
const int LOAD_CVEX_TRY_TO_NEAREST = 1; // try to spread to nearest non-removed covers around in radius (param)

inline bool load_covers_extra_from_file(CoversComponent &covers, const char *file_name, int load_cvex_mode, float param = 0.0f)
{
  covers.coverExtra.clear();
  covers.coverExtra.resize(covers.list.size());
  memset(&covers.coverExtra[0], 0, sizeof(CoverExtra) * covers.coverExtra.size());

  eastl::unique_ptr<void, decltype(&df_close)> h(df_open(file_name, DF_READ | DF_IGNORE_MISSING), &df_close);
  if (!h)
    return false;

  char header[4];
  if (df_read(h.get(), header, 4) != 4)
    return false;
  if (header[0] != 'X' || header[1] != 'B' || header[2] != 'I' || header[3] != 'N')
    return false;
  uint32_t flags = 0;
  if (df_read(h.get(), &flags, sizeof(uint32_t)) != sizeof(uint32_t))
    return false;

  uint32_t chunkType = 0;
  while (df_read(h.get(), &chunkType, sizeof(uint32_t)) == sizeof(uint32_t))
  {
    uint32_t chunkSize = 0;
    if (df_read(h.get(), &chunkSize, sizeof(uint32_t)) != sizeof(uint32_t))
      break;
    if (chunkType == '1XVC') // CVX1
    {
      if (chunkSize % 16 != 0)
        return false;
      const int TOO_MANY_RECONS = 1024 * 1024;
      if (chunkSize >= TOO_MANY_RECONS * 16)
        return false;
      int numExtra = chunkSize / 16;
      if (load_cvex_mode == LOAD_CVEX_BY_EXACT_IDS)
      {
        if (numExtra != (int)covers.list.size())
          return false;
        const uint32_t dataSize = numExtra * 16;
        if (df_read(h.get(), &covers.coverExtra[0], dataSize) != dataSize)
          return false;
        return true;
      }
      else if (load_cvex_mode == LOAD_CVEX_TRY_TO_NEAREST)
      {
        eastl::vector<CoverExtra> tmpExtra;
        tmpExtra.resize(numExtra);
        const uint32_t dataSize = numExtra * 16;
        if (df_read(h.get(), &tmpExtra[0], dataSize) != dataSize)
          return false;
        const float radius = param;
        for (int i = 0; i < numExtra; ++i)
        {
          const CoverExtra &extra = tmpExtra[i];
          if (extra.pos == Point3::ZERO)
            continue;

          vec3f pos = v_ldu_p3(&extra.pos.x);
          vec3f rad = v_splat_x(v_ldu_x(&radius));
          bbox3f box;
          box.bmin = v_sub(pos, rad);
          box.bmax = v_add(pos, rad);

          int nearest = -1;
          float bestDistSq = 0.f;
          covers.boxCull(box, [&](int id, mat44f_cref tm) {
            Point3 tmPos(v_extract_x(tm.col3), v_extract_y(tm.col3), v_extract_z(tm.col3));
            if (tmPos == Point3::ZERO)
              return;
            if (covers.coverExtra[id].info != 0)
              return;
            const Point3 delta = tmPos - extra.pos;
            const float distSq = delta.lengthSq();
            if (nearest < 0 || distSq < bestDistSq)
            {
              nearest = id;
              bestDistSq = distSq;
            }
          });
          if (nearest >= 0)
            covers.coverExtra[nearest] = extra;
        }
        return true;
      }
      return false;
    }
    if (df_seek_rel(h.get(), chunkSize) != 0)
      return false;
  }
  return false;
}
} // namespace bind_dascript
