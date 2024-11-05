//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
