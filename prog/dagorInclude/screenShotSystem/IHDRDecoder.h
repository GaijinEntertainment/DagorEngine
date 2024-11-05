//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class IHDRDecoder
{
public:
  virtual void Decode() = 0;
  virtual bool IsActive() = 0;
  virtual char *LockResult(int &stride, int &w, int &h) = 0;
};
