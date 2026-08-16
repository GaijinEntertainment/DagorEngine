// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

class DataBlock;


struct UvValidationSettings
{
  bool validate = true;
  bool warnOnly = true;

public:
  void loadValidationSettings(const DataBlock &blk);
};
