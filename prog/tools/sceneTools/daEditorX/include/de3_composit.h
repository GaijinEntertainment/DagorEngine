//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

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
