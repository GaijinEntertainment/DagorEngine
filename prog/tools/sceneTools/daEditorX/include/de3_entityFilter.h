//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>

class IObjEntityFilter
{
public:
  static constexpr unsigned HUID = 0x4916C3E9u; // IObjEntityFilter
  enum
  {
    STMASK_TYPE_RENDER,
    STMASK_TYPE_COLLISION,
    STMASK_TYPE_EXPORT
  };

  virtual bool allowFiltering(int mask_type) = 0;
  virtual void applyFiltering(int mask_type, bool use = true) = 0;

  virtual bool isFilteringActive(int mask_type) = 0;

  static void setSubTypeMask(int mask_type, unsigned mask);
  static unsigned getSubTypeMask(int mask_type);

  static void setLayerHiddenMask(uint64_t lhmask);
  static uint64_t getLayerHiddenMask();

  static void setShowInvalidAsset(bool show);
  static bool getShowInvalidAsset();
};
