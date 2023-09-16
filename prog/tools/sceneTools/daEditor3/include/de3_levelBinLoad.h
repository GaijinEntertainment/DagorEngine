//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class ILevelBinLoader
{
public:
  static constexpr unsigned HUID = 0x80F0FED7u; // ILevelBinLoader

  virtual void changeLevelBinary(const char *bin_fn) = 0;
  virtual const char *getLevelBinary() = 0;
};
