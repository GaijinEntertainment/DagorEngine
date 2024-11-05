//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class ILevelBinLoader
{
public:
  static constexpr unsigned HUID = 0x80F0FED7u; // ILevelBinLoader

  virtual void changeLevelBinary(const char *bin_fn) = 0;
  virtual const char *getLevelBinary() = 0;
};
