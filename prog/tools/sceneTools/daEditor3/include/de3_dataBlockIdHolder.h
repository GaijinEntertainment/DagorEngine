//
// DaEditor3
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IDataBlockIdHolder
{
public:
  static constexpr unsigned HUID = 0x0B6D92B2u; // IDataBlockIdHolder

  virtual void setDataBlockId(unsigned id) = 0;
  virtual unsigned getDataBlockId() = 0;

  static const int invalid_id = 0;
};
