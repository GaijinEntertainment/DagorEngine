//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <generic/dag_span.h>

class Point4;

struct DecalDataBase
{
  Point4 pos_size;
  Point3 normal;
  uint32_t decal_id;

  DecalDataBase() : pos_size(Point4::ZERO), normal(Point3::ZERO), decal_id(0) {}
  DecalDataBase(const Point4 &pos_size, const Point3 &normal, uint32_t decal_id) :
    pos_size(pos_size), normal(normal), decal_id(decal_id)
  {}
};

struct DefaultDecalData : public DecalDataBase
{
  Point3 up;
  uint32_t tid_mid_flags; // 7 bits texture id, 9 bit matrix id, 16 bits flags
  Point4 params;

  DefaultDecalData();
  DefaultDecalData(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t texture_id, uint16_t matrix_id, const Point4 &params,
    uint16_t flags);
  DefaultDecalData(uint32_t decal_id, const TMatrix &tm, float rad, uint16_t matrix_id, const Point4 &params, uint16_t flags);
  // For update param Z
  DefaultDecalData(uint32_t id, const Point4 &params);
  // For delete params
  DefaultDecalData(uint32_t id);

  void partialUpdate(dag::Span<Point4> buff_update_data) const;
};