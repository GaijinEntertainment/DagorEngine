//
// Dagor Tech 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_simpleString.h>

struct TwoStepRelPath
{
  typedef char storage_t[512];

  void setSdkRoot(const char *root_dir, const char *subdir = nullptr);

  const char *mkRelPath(const char *fpath, storage_t &stor);

protected:
  SimpleString sdkRoot;
  int sdkRootLen = 0, sdkRootLen1 = -1;
};
