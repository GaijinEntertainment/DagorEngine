//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

class IHDRDecoder
{
public:
  virtual void Decode() = 0;
  virtual bool IsActive() = 0;
  virtual char *LockResult(int &stride, int &w, int &h) = 0;
};
