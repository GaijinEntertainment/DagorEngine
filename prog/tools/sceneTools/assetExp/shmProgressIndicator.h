// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <libTools/util/progressInd.h>
#include <assets/daBuildProgressShm.h>

class ShmProgressIndicator final : public IGenericProgressIndicator
{
  IGenericProgressIndicator &inner;
  DaBuildProgressShm *shm;

public:
  ShmProgressIndicator(IGenericProgressIndicator &inner_, DaBuildProgressShm *shm_) : inner(inner_), shm(shm_) {}

  void setTotal(int n) override
  {
    inner.setTotal(n);
    shm->writeAsset(0, n);
  }

  void setDone(int n) override
  {
    inner.setDone(n);
    shm->setAssetDone(n);
  }

  void incDone(int inc = 1) override
  {
    inner.incDone(inc);
    shm->addAssetDone(inc);
  }

  void setActionDescFmt(const char *, const DagorSafeArg *, int) override {}
  void redrawScreen() override { inner.redrawScreen(); }
  void startProgress(IProgressCB *cb = nullptr) override { inner.startProgress(cb); }
  void endProgress() override { inner.endProgress(); }

  void destroy() override
  {
    inner.destroy();
    delete this;
  }
};
