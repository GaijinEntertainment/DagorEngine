//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <rendInst/edHugeBitmap2d.h>

namespace rendinst::gen::land
{
class AssetData;
}

class IRendInstGenService
{
public:
  static constexpr unsigned HUID = 0xD835CE77u; // IRendInstGenService

  typedef HierBitMap2d<HierConstSizeBitMap2d<4, ConstSizeBitMap2d<5>>> EditableHugeBitmask;

  virtual rendinst::gen::land::AssetData *getLandClassGameRes(const char *name) = 0;
  virtual void releaseLandClassGameRes(rendinst::gen::land::AssetData *res) = 0;

  virtual void setCustomGetHeight(bool(__stdcall *custom_get_height)(Point3 &pos, Point3 *out_norm)) = 0;
  virtual void setSweepMask(EditableHugeBitmask *bm, float ox, float oz, float scale) = 0;

  virtual void clearRtRIGenData() = 0;
  virtual bool createRtRIGenData(float ofs_x, float ofs_z, float grid2world, int cell_sz, int cell_num_x, int cell_num_z,
    dag::ConstSpan<rendinst::gen::land::AssetData *> land_cls, float dens_map_px, float dens_map_pz) = 0;
  virtual void releaseRtRIGenData() = 0;

  virtual EditableHugeBitMap2d *getRIGenBitMask(rendinst::gen::land::AssetData *land_cls) = 0;
  virtual void discardRIGenRect(int gx0, int gz0, int gx1, int gz1) = 0;

  virtual int getRIGenLayers() const = 0;
  virtual int getRIGenLayerCellSizeDivisor(int layer_idx) const = 0;

  virtual void setReinitCallback(void(__stdcall *reinit_cb)(void *arg), void *arg) = 0;
  virtual void resetLandClassCache() = 0;

  virtual void onLevelBlkLoaded(const DataBlock &blk) = 0;

  virtual BBox3 calcWorldBBox() const = 0;
};
