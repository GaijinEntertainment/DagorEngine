//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_mathBase.h>
#include <math/dag_Point3.h>
#include <generic/dag_functionRef.h>

class DagorAsset;
class IObjEntity;

class ICompositObj
{
public:
  static constexpr unsigned HUID = 0x9143C0F6u; // ICompositObj

  struct Props
  {
    enum
    {
      PT_none,
      PT_coll,
      PT_collNorm,
      PT_3pod,
      PT_fnd,
      PT_flt,
      PT_riColl
    };
    int placeType;
    float aboveHt;
  };


  virtual int getCompositSubEntityCount() = 0;
  virtual IObjEntity *getCompositSubEntity(int idx) = 0;
  // This can be significantly faster for huge composites than using getCompositSubEntityCount() and
  // getCompositSubEntity() if you iterate over everything.
  virtual dag::Span<IObjEntity *> getCompositSubEntities() = 0;
  virtual const Props &getCompositSubEntityProps(int idx) = 0;
  virtual int getCompositSubEntityIdxByLabel(const char *label) = 0;

  virtual void setCompositPlaceTypeOverride(int placeType) = 0;
  virtual int getCompositPlaceTypeOverride() = 0;

  //! marks the composit as following the mouse, so per-move bookkeeping that only the final position
  //! needs (the riExtra collision grid) can be batched until it is cleared again
  virtual void setCompositInteractiveMove(bool /*on*/) {}

  //! the asset's own autoInstSeed:b, off when a placed object of this asset wants a per-instance seed that
  //! does not follow its position. Giving it one is the editor's job, so nothing here acts on this value
  virtual bool getCompositAutoInstSeed() { return true; }

  //! false when the asset discards an explicit per-instance seed (forceInstSeed0:b), so there is no point
  //! giving a placed object one whatever autoInstSeed:b says
  virtual bool canCompositUseInstSeed() { return true; }
};

class ICompositLeafEnumerator
{
public:
  // Calls leaf_cb for each leaf entity of the asset's composit tree, with its tm in the composit space. From a random
  // node, the walk selects one entity, and makes the random tm of each node, as for a new composit entity at the
  // origin. Placement on collision or water is skipped. Leaf is the virtual entity of the sub-asset, or of a spline
  // or polygon node, which has no asset.
  //
  // No entity is added to the scene. The call creates the entity pool of the composit and of each sub-asset when the
  // pool does not exist, and the services keep the pools.
  //
  // Returns false when the asset is not a composit, when a composit in the tree has require blocks, or when leaf_cb
  // returns false, which stops the walk. You have to discard what leaf_cb collected when the function returns false.
  virtual bool enumCompositLeaves(const DagorAsset &asset, dag::FunctionRef<bool(IObjEntity &leaf, const TMatrix &tm)> leaf_cb) = 0;

  static constexpr unsigned HUID = 0xF8D3B00Bu; // ICompositLeafEnumerator
};

// In a composite the Y axis scale is scale * yScale.
inline real composit_y_scale_param(real scale, real y_axis_scale) { return scale > 0 ? y_axis_scale / scale : 1.f; }
inline real composit_y_axis_scale(real scale, real y_scale_param) { return scale * y_scale_param; }

// In a composite the rot_x/rot_y/rot_z parameters have the opposite sign to the euler angles of the node matrix.
inline Point3 composit_rotation_params(const Point3 &matrix_rotation) { return -matrix_rotation; }
inline Point3 composit_matrix_rotation(const Point3 &rotation_params) { return -rotation_params; }
