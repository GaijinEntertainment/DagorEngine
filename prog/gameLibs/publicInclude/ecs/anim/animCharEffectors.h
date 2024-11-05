//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <daECS/core/componentType.h>
#include <daECS/core/event.h>

struct EffectorData
{
  TMatrix wtm = TMatrix::IDENT;
  Point3 position = ZERO<Point3>();

  int effId = -1;
  int effWtmId = -1;

  float weight = 0.f;

  EffectorData(int eff_id, int eff_wtm_id) : effId(eff_id), effWtmId(eff_wtm_id) {}
};

ECS_DECLARE_TYPE(EffectorData);
ECS_UNICAST_EVENT_TYPE(InvalidateEffectorData);
