//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IPoint4.h>
#include <memory/dag_framemem.h>
#include <dag/dag_vector.h>

class DataBlock;

struct RendinstLandclassData
{
  int index;
  TMatrix matrix;
  TMatrix matrixInv;
  Point4 mapping;
  bbox3f bbox;
  float radius;
  float sortOrder;
};
using TempRiLandclassVec = dag::Vector<RendinstLandclassData, framemem_allocator>;

extern void (*get_rendinst_land_class_data)(TempRiLandclassVec &ri_landclasses);
extern bool (*get_land_class_detail_data)(DataBlock &dest_data, const char *land_cls_res_nm);
