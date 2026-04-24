//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <de3_interface.h>
#include <assets/asset.h>
#include <generic/dag_tab.h>
#include <util/dag_string.h>

struct CircularDependenceChecker
{
  CircularDependenceChecker(int &static_dcnt, Tab<const DagorAsset *> &static_a) : depthCnt(static_dcnt), assets(static_a)
  {
    if (depthCnt == 0)
      assets.clear();
    depthCnt++;
  }
  ~CircularDependenceChecker()
  {
    G_ASSERT(depthCnt > 0);
    depthCnt--;
    assets.pop_back();
  }

  bool isCyclicError(const DagorAsset &a)
  {
    for (int i = assets.size() - 1; i >= 0; i--)
      if (assets[i] == &a)
      {
        DAEDITOR3.conError("asset <%s> contains circular dependencies, e.g. to %s", assets[0]->getNameTypified(), a.getNameTypified());
        assets.push_back(nullptr);
        return true;
      }
    assets.push_back(&a);
    return false;
  }

  int &depthCnt;
  Tab<const DagorAsset *> &assets;
};
