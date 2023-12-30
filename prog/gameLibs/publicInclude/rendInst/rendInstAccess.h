//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <rendInst/rendInstDesc.h>
#include <rendInst/layerFlags.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>


// The functions in this file provide access to various properties of rendInsts.

class CollisionResource;
struct RendInstGenData;

namespace rendinst
{


int getRIGenMaterialId(const RendInstDesc &desc, bool need_lock = true);
bool getRIGenCanopyBBox(const RendInstDesc &desc, const TMatrix &tm, BBox3 &out_canopy_bbox, bool need_lock = true);
CollisionResource *getRIGenCollInfo(const RendInstDesc &desc);
void *getCollisionResourceHandle(const RendInstDesc &desc);
const CollisionResource *getRiGenCollisionResource(const RendInstDesc &desc);
bool isRIGenPosInst(const RendInstDesc &desc);
bool isDestroyedRIExtraFromNextRes(const RendInstDesc &desc);
int getRIExtraNextResIdx(int pool_id);

TMatrix getRIGenMatrix(const RendInstDesc &desc);
// Assumes that everything required is already locked
TMatrix getRIGenMatrixNoLock(const RendInstDesc &desc);
TMatrix getRIGenMatrixDestr(const RendInstDesc &desc);
const char *getRIGenResName(const RendInstDesc &desc);
const char *getRIGenDestrName(const RendInstDesc &desc);
const char *getRIGenDestrFxTemplateName(const RendInstDesc &desc);
bool isRIGenDestr(const RendInstDesc &desc);

int getRIGenStrideRaw(int layer_idx, int pool_id);
int getRIGenStride(int layer_idx, int cell_id, int pool_id);
BBox3 getRIGenBBox(const RendInstDesc &desc);
BBox3 getRIGenFullBBox(const RendInstDesc &desc);

int get_debris_fx_type_id(RendInstGenData *rgl, const rendinst::RendInstDesc &ri_desc);

struct RiDestrData
{
  float collisionHeightScale;
  bool bushBehaviour;

  int fxType;
  float fxScale;
  const char *fxTemplate;
};

RiDestrData gather_ri_destr_data(const RendInstDesc &ri_desc);
bool ri_gen_has_collision(const RendInstDesc &ri_desc);

RendInstDesc get_restorable_desc(const RendInstDesc &ri_desc);
int find_restorable_data_index(const RendInstDesc &desc);

} // namespace rendinst
