// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "projectiveDecals/projectiveDecals.h"


static uint32_t packTidMidFlags(uint16_t texture_id, uint16_t matrix_id, uint16_t flags)
{
  uint32_t tex_id_matrix_id = (texture_id & 0x7f) | (matrix_id << 7);
  return (tex_id_matrix_id << 16) | flags;
}

DefaultDecalData::DefaultDecalData() : DecalDataBase(), up(Point3::ZERO), tid_mid_flags(UPDATE_ALL), params(Point4::ZERO) {}

DefaultDecalData::DefaultDecalData(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t texture_id, uint16_t matrix_id,
  const Point4 &params, uint16_t flags) :
  DecalDataBase(Point4::xyzV(tm.getcol(3), rad), normalize(tm.getcol(1)), decal_id),
  up(normalize(tm.getcol(2))),
  params(params),
  tid_mid_flags(packTidMidFlags(texture_id, matrix_id, flags))
{}

DefaultDecalData::DefaultDecalData(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t matrix_id, const Point4 &params,
  uint16_t flags) :
  DecalDataBase(Point4::xyzV(tm.getcol(3), rad), normalize(tm.getcol(1)), decal_id),
  up(normalize(tm.getcol(2))),
  params(params),
  tid_mid_flags(packTidMidFlags(0, matrix_id, flags))
{}

DefaultDecalData::DefaultDecalData(uint32_t id, const Point4 &params) :
  DecalDataBase(Point4::ZERO, Point3::ZERO, id), params(params), up(Point3::ZERO), tid_mid_flags(UPDATE_PARAM_Z)
{}

DefaultDecalData::DefaultDecalData(uint32_t id) :
  DecalDataBase(Point4::ZERO, Point3::ZERO, id), params(Point4::ZERO), up(Point3::ZERO), tid_mid_flags(UPDATE_ALL)
{}

void DefaultDecalData::partialUpdate(dag::Span<Point4> buff_update_data) const
{
  if ((tid_mid_flags & UPDATE_ALL) == UPDATE_ALL)
    G_ASSERT_LOG(0, "Don't use partial update with update flag 'UPDATE_ALL'");
  else if ((tid_mid_flags & UPDATE_PARAM_Z) == UPDATE_PARAM_Z)
  {
    DefaultDecalData *updateData = reinterpret_cast<DefaultDecalData *>(buff_update_data.data());
    updateData->params.z = params.z;
    updateData->decal_id = decal_id;
    updateData->tid_mid_flags |= UPDATE_PARAM_Z;
  }
}