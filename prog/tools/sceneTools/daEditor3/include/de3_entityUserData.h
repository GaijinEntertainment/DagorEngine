//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IObjEntityUserDataHolder
{
public:
  static constexpr unsigned HUID = 0x5B740457u; // IObjEntityUserDataHolder

  virtual DataBlock *getUserDataBlock(bool create_if_not_exist) = 0;
  virtual void resetUserDataBlock() = 0;
  virtual void setSuperEntityRef(const char *ref) {}
};
