//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_e3dColor.h>


class IColorRangeService
{
public:
  static constexpr unsigned HUID = 0x1B356C02u; // IColorRangeService
  enum
  {
    IDX_WHITE = 0xFFFFu,
    IDX_GRAY = 0x8888u
  };

  //! registers color range and returns its index (only 16 lsb used) for later operations;
  //! some indices have special meaning: 0xFFFFU - always white, 0x8888U - always gray;
  //! other indicies (0, 1, 2, ...) map different unique color ranges
  virtual unsigned addColorRange(E3DCOLOR from, E3DCOLOR to) = 0;

  //! returns number of registered ranges
  virtual int getColorRangesNum() = 0;

  //! returns start color for color range
  virtual E3DCOLOR getColorFrom(unsigned range_idx) = 0;
  //! returns end color for color range
  virtual E3DCOLOR getColorTo(unsigned range_idx) = 0;

  virtual E3DCOLOR generateColor(unsigned range_idx, float seed_pos_x, float seed_pos_y, float seed_pos_z) = 0;
};
