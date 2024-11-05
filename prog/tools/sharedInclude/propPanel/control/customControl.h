//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

namespace PropPanel
{

class ICustomControl
{
public:
  virtual void customControlUpdate(int id) = 0;
};

} // namespace PropPanel