//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <projectiveDecals/projectiveDecals.h>

MAKE_TYPE_FACTORY(ProjectiveDecalsBase, ProjectiveDecalsBase);
MAKE_TYPE_FACTORY(RingBufferDecalsBase, RingBufferDecalsBase);
MAKE_TYPE_FACTORY(ResizableDecalsBase, ResizableDecalsBase);
MAKE_TYPE_FACTORY(RingBufferDecalManager, RingBufferDecalManager);
MAKE_TYPE_FACTORY(ResizableDecalManager, ResizableDecalManager);
MAKE_TYPE_FACTORY(DecalDataInterface, DecalDataBase);
MAKE_TYPE_FACTORY(DefaultDecalData, DefaultDecalData);

namespace bind_dascript
{
inline DefaultDecalData make_default_decal_data(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t texture_id,
  uint16_t matrix_id, const Point4 &params, uint16_t flags)
{
  return DefaultDecalData(decal_id, tm, rad, texture_id, matrix_id, params, flags);
}
inline DefaultDecalData make_update_params_decal_data(uint32_t decal_id, const Point4 params)
{
  return DefaultDecalData(decal_id, params);
}
} // namespace bind_dascript
