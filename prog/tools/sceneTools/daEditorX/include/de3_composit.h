//
// DaEditorX
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
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
  virtual const Props &getCompositSubEntityProps(int idx) = 0;
  virtual int getCompositSubEntityIdxByLabel(const char *label) = 0;

  virtual void setCompositPlaceTypeOverride(int placeType) = 0;
  virtual int getCompositPlaceTypeOverride() = 0;
};
